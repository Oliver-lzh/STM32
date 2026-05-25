#include "AppRadarDebugAcquisition.h"
#include "BoardOutput.h"

#include <universal/data_definitions.h>
#include <universal/types/DataSettingsBgtRadar.h>

#include <stddef.h>

#define APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define APP_RADAR_DEBUG_ACQUISITION_SAMPLES_PER_CHIRP (64u)
#define APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME  (128u)
#define APP_RADAR_DEBUG_ACQUISITION_RX_CHANNELS       (3u)
#define APP_RADAR_DEBUG_ACQUISITION_ADC_BITS          (12u)
#define APP_RADAR_DEBUG_ACQUISITION_FIFO_ADDRESS      (0x0060u)
#define APP_RADAR_DEBUG_ACQUISITION_READOUT_SAMPLES \
    (APP_RADAR_DEBUG_ACQUISITION_SAMPLES_PER_CHIRP * APP_RADAR_DEBUG_ACQUISITION_RX_CHANNELS)
#define APP_RADAR_DEBUG_ACQUISITION_STATUS_INTERVAL_FRAMES (16u)
#define APP_RADAR_DEBUG_ACQUISITION_EXPECTED_FRAME_BYTES \
    (APP_RADAR_DEBUG_ACQUISITION_READOUT_SAMPLES * 3u / 2u * APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME)
#define APP_RADAR_DEBUG_ACQUISITION_VERBOSE (0)
#define APP_RADAR_DEBUG_ACQUISITION_LOG(...)                         \
    do                                                               \
    {                                                                \
        if (APP_RADAR_DEBUG_ACQUISITION_VERBOSE)                     \
        {                                                            \
            (void)BoardOutput_printf(__VA_ARGS__);                   \
        }                                                            \
    } while (0)

typedef struct
{
    uint8_t address;
    uint32_t value;
} AppRadarDebugAcquisition_Register_t;

static const uint32_t g_classicProfileWords[] = {
    0x11e8270u, 0x30a0210u, 0x9e967fdu, 0xb0805b4u,
    0xd1027ffu, 0xf010d00u, 0x11000000u, 0x13000000u,
    0x15000000u, 0x17000be0u, 0x19000000u, 0x1b000000u,
    0x1d000000u, 0x1f000b60u, 0x2113fc51u, 0x237ff41fu,
    0x25701ce7u, 0x2d000490u, 0x3b000480u, 0x49000480u,
    0x57000480u, 0x5911be0eu, 0x5b639c0au, 0x5d07f000u,
    0x5f787e1eu, 0x61a2a862u, 0x63000c88u, 0x65000172u,
    0x67000040u, 0x69000000u, 0x6b000000u, 0x6d000000u,
    0x6f1e3310u, 0x7f000100u, 0x8f000100u, 0x9f000100u,
    0xad000000u, 0xb7000000u,
};

static IRadarAvian *m_radar = NULL;
static IData *m_data = NULL;
static uint8_t m_dataIndex = 0u;
static bool m_isConstructed = false;
static bool m_startAttempted = false;
static bool m_isRunning = false;
static bool m_hasReceivedFrame = false;
static bool m_profileVerified = false;
static uint32_t m_receivedFrameCounter = 0u;

static AppRadarDebugAcquisition_Register_t AppRadarDebugAcquisition_decodeRegisterWord(uint32_t word)
{
    AppRadarDebugAcquisition_Register_t reg;

    reg.address = (uint8_t)((word & 0xFE000000u) >> 25);
    reg.value   = word & 0x00FFFFFFu;

    return reg;
}

static sr_t AppRadarDebugAcquisition_getRegisters(IRegisters8_32 **registers)
{
    if ((m_radar == NULL) || (m_radar->getIRegisters == NULL) || (registers == NULL))
    {
        return E_NOT_INITIALIZED;
    }

    *registers = m_radar->getIRegisters(m_radar);
    if (*registers == NULL)
    {
        return E_NOT_INITIALIZED;
    }

    return E_SUCCESS;
}

static sr_t AppRadarDebugAcquisition_verifyClassicProfile(void)
{
    uint32_t index;
    IRegisters8_32 *registers;

    RETURN_ON_ERROR(AppRadarDebugAcquisition_getRegisters(&registers));

    for (index = 0u; index < APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(g_classicProfileWords); index++)
    {
        uint32_t readValue;
        const AppRadarDebugAcquisition_Register_t reg = AppRadarDebugAcquisition_decodeRegisterWord(g_classicProfileWords[index]);
        const sr_t ret = registers->read(registers, reg.address, &readValue);

        if (ret != E_SUCCESS)
        {
            (void)BoardOutput_printf("debug-acq,reg-read,failed,i=%lu,a=%02x,ret=%d\r\n",
                                     (unsigned long)index,
                                     (unsigned int)reg.address,
                                     (int)ret);
            return ret;
        }

        if (readValue != reg.value)
        {
            (void)BoardOutput_printf("debug-acq,reg-verify,failed,i=%lu,a=%02x,w=%06lx,r=%06lx\r\n",
                                     (unsigned long)index,
                                     (unsigned int)reg.address,
                                     (unsigned long)reg.value,
                                     (unsigned long)readValue);
            return E_UNEXPECTED_VALUE;
        }
    }

    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,profile,verify,ok,count=%lu\r\n",
                                    (unsigned long)APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(g_classicProfileWords));
    m_profileVerified = true;
    return E_SUCCESS;
}

static sr_t AppRadarDebugAcquisition_writeClassicProfile(void)
{
    uint32_t index;
    IRegisters8_32 *registers;

    RETURN_ON_ERROR(AppRadarDebugAcquisition_getRegisters(&registers));

    for (index = 0u; index < APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(g_classicProfileWords); index++)
    {
        const AppRadarDebugAcquisition_Register_t reg = AppRadarDebugAcquisition_decodeRegisterWord(g_classicProfileWords[index]);

        RETURN_ON_ERROR(registers->write(registers, reg.address, reg.value));
    }

    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,profile,write,ok,count=%lu\r\n",
                                    (unsigned long)APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(g_classicProfileWords));
    return E_SUCCESS;
}

static sr_t AppRadarDebugAcquisition_configureData(void)
{
    static const uint16_t readouts[][2] = {
        {APP_RADAR_DEBUG_ACQUISITION_FIFO_ADDRESS, APP_RADAR_DEBUG_ACQUISITION_READOUT_SAMPLES},
    };
    uint8_t settings[DATA_SETTINGS_BGT_RADAR_SIZE(APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(readouts), APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME)];
    IDataProperties_t properties = {
        DataFormat_Packed12,
        APP_RADAR_DEBUG_ACQUISITION_RX_CHANNELS,
        APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME,
        APP_RADAR_DEBUG_ACQUISITION_SAMPLES_PER_CHIRP,
        0u,
        APP_RADAR_DEBUG_ACQUISITION_ADC_BITS,
    };

    if ((m_data == NULL) || (m_data->configure == NULL))
    {
        return E_NOT_INITIALIZED;
    }

    DataSettingsBgtRadar_initialize(settings,
                                    readouts,
                                    (uint16_t)APP_RADAR_DEBUG_ACQUISITION_ARRAY_SIZE(readouts),
                                    APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME);

    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,data,request,fifo=%04x,samples=%u,rx=%u,chirps=%u,fmt=packed12,expect=%lu\r\n",
                                    (unsigned int)APP_RADAR_DEBUG_ACQUISITION_FIFO_ADDRESS,
                                    (unsigned int)APP_RADAR_DEBUG_ACQUISITION_SAMPLES_PER_CHIRP,
                                    (unsigned int)APP_RADAR_DEBUG_ACQUISITION_RX_CHANNELS,
                                    (unsigned int)APP_RADAR_DEBUG_ACQUISITION_CHIRPS_PER_FRAME,
                                    (unsigned long)APP_RADAR_DEBUG_ACQUISITION_EXPECTED_FRAME_BYTES);

    return m_data->configure(m_dataIndex, &properties, settings, (uint16_t)sizeof(settings));
}

static sr_t AppRadarDebugAcquisition_start(void)
{
    if ((m_radar == NULL) || (m_data == NULL))
    {
        return E_NOT_INITIALIZED;
    }

    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,config,start\r\n");

    RETURN_ON_ERROR(m_radar->stopData(m_radar));

    m_profileVerified = false;
    RETURN_ON_ERROR(AppRadarDebugAcquisition_writeClassicProfile());
    RETURN_ON_ERROR(AppRadarDebugAcquisition_verifyClassicProfile());

    RETURN_ON_ERROR(m_radar->initialize(m_radar));
    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,radar-init,ok\r\n");

    RETURN_ON_ERROR(AppRadarDebugAcquisition_configureData());
    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,data-config,ok\r\n");

    RETURN_ON_ERROR(m_radar->startData(m_radar));

    m_isRunning = true;
    m_hasReceivedFrame = false;
    m_receivedFrameCounter = 0u;
    APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,start,ok\r\n");
    return E_SUCCESS;
}

void AppRadarDebugAcquisition_Constructor(IRadarAvian *radar, IData *data, uint8_t dataIndex)
{
    m_radar = radar;
    m_data = data;
    m_dataIndex = dataIndex;
    m_isConstructed = true;
    m_startAttempted = false;
    m_isRunning = false;
    m_hasReceivedFrame = false;
    m_profileVerified = false;
    m_receivedFrameCounter = 0u;
}

void AppRadarDebugAcquisition_run(void)
{
    sr_t ret;

    if (!m_isConstructed)
    {
        return;
    }

    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        (void)AppRadarDebugAcquisition_stop();
        return;
    }

    if (m_startAttempted)
    {
        return;
    }

    m_startAttempted = true;
    ret = AppRadarDebugAcquisition_start();
    if (ret != E_SUCCESS)
    {
        (void)BoardOutput_printf("debug-acq,start,failed,%d\r\n", (int)ret);
    }
}

void AppRadarDebugAcquisition_onFrame(uint32_t count, uint8_t channel, uint64_t timestamp)
{
    if (!m_isRunning)
    {
        return;
    }

    m_receivedFrameCounter++;

    if (!m_hasReceivedFrame)
    {
        m_hasReceivedFrame = true;
        APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,frame,ok,cnt=%lu,ch=%u,ts=%08lx%08lx\r\n",
                                        (unsigned long)count,
                                        (unsigned int)channel,
                                        (unsigned long)(timestamp >> 32),
                                        (unsigned long)(timestamp & 0xFFFFFFFFu));
    }

    if ((m_receivedFrameCounter == 1u) ||
        ((m_receivedFrameCounter % APP_RADAR_DEBUG_ACQUISITION_STATUS_INTERVAL_FRAMES) == 0u))
    {
        APP_RADAR_DEBUG_ACQUISITION_LOG("debug-acq,status,profile=%s,running=%u,frames=%lu,cnt=%lu,expect=%lu\r\n",
                                        m_profileVerified ? "verified" : "not-verified",
                                        (unsigned int)m_isRunning,
                                        (unsigned long)m_receivedFrameCounter,
                                        (unsigned long)count,
                                        (unsigned long)APP_RADAR_DEBUG_ACQUISITION_EXPECTED_FRAME_BYTES);
    }
}

sr_t AppRadarDebugAcquisition_stop(void)
{
    sr_t ret;

    if (!m_isConstructed || (m_radar == NULL))
    {
        m_isRunning = false;
        m_startAttempted = false;
        m_hasReceivedFrame = false;
        m_profileVerified = false;
        m_receivedFrameCounter = 0u;
        return E_SUCCESS;
    }

    if (!m_isRunning && !m_startAttempted)
    {
        return E_SUCCESS;
    }

    ret = m_radar->stopData(m_radar);
    if (ret == E_SUCCESS)
    {
        m_isRunning = false;
        m_startAttempted = false;
        m_hasReceivedFrame = false;
        m_profileVerified = false;
        m_receivedFrameCounter = 0u;
    }

    return ret;
}

bool AppRadarDebugAcquisition_isRunning(void)
{
    return m_isRunning;
}

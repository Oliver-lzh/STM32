/* ===========================================================================
** Copyright (C) 2021 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/

#include "AppRadarDebugAcquisition.h"
#include "AppRadarProbe.h"
#include "AppRangeFft.h"
#include "BoardOutput.h"

#include <PlatformInterfaces.h>
#include <board/Board.h>
#include <bsp/atr22.h>
#include <bsp/avian.h>
#include <bsp/connector.h>
#include <bsp/leds.h>
#include <bsp/ltr11.h>
#include <bsp/smartar.h>
#include <bsp/spi_custom.h>

#include <board/BoardInfo.h>
#include <common/typeutils.h>
#include <components/radar/Atr22.h>
#include <components/radar/Avian.h>
#include <components/radar/Ltr11.h>
#include <components/radar/Smartar.h>
#include <impl/Platform.h>
#include <impl/PlatformGpio.h>
#include <impl/PlatformInterrupt.h>
#include <impl/PlatformSpi.h>
#include <impl/SamsHelper.h>
#include <impl/thread.h>
#include <platform/DataAtr22.h>
#include <platform/DataAvian.h>
#include <platform/DataLtr11.h>
#include <platform/DataSmartar.h>
#include <platform/ShieldConnector.h>
#include <platform/led/LedSequenceRbb.h>
#include <platform/led/LedSequenceStatus.h>

#include <protocol/commands/Commands_IRadarAtr22.h>
#include <protocol/commands/Commands_IRadarAvian.h>
#include <protocol/commands/Commands_IRadarLtr11.h>
#include <protocol/commands/Commands_IRadarSmartar.h>

// this includes the implementation of getMac() and getUuid() for the Board
#include <impl/getIdsAtmel.h>

#include <fatal_error.h>
#include <string.h>  // for memcpy


#define DATA_BUFFER_SIZE         (192 * 1024)
#define DATA_ATR22_INDEX_COUNT   (ARRAY_SIZE(BoardIrqPinsAtr22[0]))
#define DATA_SMARTAR_INDEX_COUNT (ARRAY_SIZE(BoardIrqPinsSmartar[0]))
#define MACRO_BUFFER_SIZE        (8 * 1024)

/****************************************************************************
 * Variable declarations
 ****************************************************************************/
static Atr22 m_atr22;
static Avian m_avian;
static Ltr11 m_ltr11;
static Smartar m_smartar;
static IData *m_data = NULL;
static IRadarAvian *m_debugAvianRadar = NULL;
static uint8_t m_debugAvianDataIndex = DATA_INDEX_INVALID;

// specify alignment to allow this buffer to be cast to uint16_t for different implementations
static uint8_t m_dataBuffer[DATA_BUFFER_SIZE] __attribute__((aligned(sizeof(uint16_t))));
static uint8_t m_macroBuffer[MACRO_BUFFER_SIZE] __attribute__((aligned(sizeof(uint32_t))));

extern PlatformSpiDefinition_t *BoardSpiDefinition;
static uint8_t m_spiDefinitionsCount;
static ShieldConnectorDefinition_t *ShieldConnectorDefinition;
static uint8_t m_shieldConnectorCount;

/****************************************************************************
 * Private methods
 ****************************************************************************/
static void Board_acquisitionStatusCallback(bool state)
{
    if (state)
    {
        LedSequence_setStatus(LED_STATUS_MEASURING);
    }
    else
    {
        LedSequence_setStatus(LED_STATUS_OPERATING);
    }
}

static void Board_dataCallback(void *arg, uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    (void)arg;

    LedSequence_setStatus(LED_STATUS_TRANSFERRING);

    AppRadarProbe_onFrame(payload, count, channel, timestamp);

    /* Range FFT processing is deferred to Board_run() to keep this callback short. */
    if (BoardOutput_getMode() == BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        AppRangeFft_submitFrame(payload, count, channel, timestamp);
    }

    const sr_t ret = BoardOutput_onFrame(payload, count, channel, timestamp);
    if (ret != E_SUCCESS)
    {
        m_data->stop(channel);
    }

    LedSequence_setStatus(LED_STATUS_OPERATING);
}

static sr_t Board_detectShields(void)
{
    /* Check all legacy connectors for unsupported or wrongly connected shields.
     * In case no errors are detected, all connectors will be enabled,
     * in order to proceed with radar device detection.
     */
    for (uint8_t shieldId = 0; shieldId < m_shieldConnectorCount; shieldId++)
    {
        // detect the presence of a shield by probing the I2C lines
        const sr_t detection = ShieldConnector_detect(&ShieldConnectorDefinition[shieldId], shieldId);
        switch (detection)
        {
            case E_NOT_AVAILABLE:  // allow shields without I2C bus
                /*  no break */
            case E_SUCCESS:
                break;
            case E_NOT_POSSIBLE:
                LedSequence_setRbbStatus(RBB_ERROR_HARDWARE_CONNECTED_WRONG);
                return E_NOT_POSSIBLE;
                break;
            case E_NOT_SUPPORTED:
                LedSequence_setRbbStatus(RBB_ERROR_HARDWARE_NOT_SUPPORTED);
                return E_NOT_SUPPORTED;
                break;
            default:
                LedSequence_setRbbStatus(RBB_ERROR_HARDWARE_INTERNAL_ERROR);
                return detection;
                break;
        }

        // power-up shield and configure its level shifters
        ShieldConnector_enable(&ShieldConnectorDefinition[shieldId], true);
    }

    return E_SUCCESS;
}

static sr_t Board_detectShieldV9(const ShieldConnectorDefinition_t *definition)
{
    /* Detects if a V9 shield is connected (on HatvanPlus).
     * If successful, the connector will be enabled,
     * in order to proceed with radar device detection.
     */
    const uint8_t shieldId = 0;
    const sr_t detection   = ShieldConnector_detectV9(&definition[shieldId]);
    if (detection == E_SUCCESS)
    {
        // power-up shield and configure its level shifters
        ShieldConnector_enable(&definition[shieldId], true);
    }
    return detection;
}

static bool Board_detectShieldLtr11Legacy(const ShieldConnectorDefinition_t *definition)
{
    /* Detects if a connected LTR11 shield is a legacy one (only on HatvanLegacy).
     *
     * First the MMIC is put in reset by setting DIO3 to low.
     * Then the irq0 pin is read.
     * On legacy shields, this is connected to DIV_AO and should read low,
     * while on new shields, this is connected to TARGET_DET, which is pulled high.
     */
    PlatformGpio_configurePin(definition->dio3, GPIO_MODE_OUTPUT_PUSH_PULL);
    PlatformGpio_configurePin(definition->irq0, GPIO_MODE_INPUT_PULL_DOWN);
    this_thread_sleep_for(chrono_milliseconds(1));
    bool level;
    PlatformGpio_getPin(definition->irq0, &level);

    /* we do not need to revert the pin configuration here, since the
     * following constructor call will anyways configure them.
     */

    return !level;
}

static bool Board_isHatvanPlus(void)
{
    /* Detect board type: HatvanPlus or HatvanLegacy.
     * 
     * Pin used for detection:
     * on HatvanPlus labeled as BoardID and connected to a pull-down;
     * on HatvanLegacy labeled as S2_SPI_DIR and connected to a pull-up,
     * as part of 2nd level shifter circuit.
     * 
     * To ensure accurate detection, LDO2 must be enabled in order to
     * provide 3.3v for pull-up biasing on HatvanLegacy. This has no side effect
     * on HatvanPlus.
     */
    const uint16_t ldo2EnablePin = ShieldConnectorDefinitionHatvanLegacy[1].en_ldo;
    const bool hatvanPlusState   = false;  // BoardID on HatvanPlus is pull-down
    bool detectionPinState;

    // LDO2 must be enabled to ensure accurate detection
    PlatformGpio_configurePin(ldo2EnablePin, GPIO_MODE_OUTPUT_PUSH_PULL | GPIO_FLAG_OUTPUT_INITIAL_HIGH);
    PlatformGpio_configurePin(BOARD_DETECTION_PIN, GPIO_MODE_INPUT);

    this_thread_sleep_for(chrono_milliseconds(1));
    PlatformGpio_getPin(BOARD_DETECTION_PIN, &detectionPinState);

    // undo LDO2 enabling after detection
    PlatformGpio_configurePin(ldo2EnablePin, GPIO_MODE_INPUT);

    return detectionPinState == hatvanPlusState;
}

static void Board_swapSpiIds(void)
{
    // Swapping SPI ids is only needed for the first two shield connectors in order to keep devId = 0 for the connected device.
    if (ARRAY_SIZE_CHECKED(BoardSpiDefinitionHatvanLegacy) < 2)
    {
        fatal_error(0);
    }

    PlatformSpiDefinition_t tmpSpi    = BoardSpiDefinitionHatvanLegacy[0];
    BoardSpiDefinitionHatvanLegacy[0] = BoardSpiDefinitionHatvanLegacy[1];
    BoardSpiDefinitionHatvanLegacy[1] = tmpSpi;
}

static IData *Board_detectRadar(IGpio *gpio, ISpi *spi, II2c *i2c, bool hatvanPlus, bool useQspi)
{
    /* Checks both connectors for the presence of a supported radar device.
     * Only the first detected device will be instantiated.
     */
    for (uint8_t shieldId = 0; shieldId < m_shieldConnectorCount; shieldId++)
    {
        if (m_shieldConnectorCount == 2)
        {
            /* In case there is no radar device on the first connector,
             * swap the ids with the second connector to try detection again.
             * This ensures to always reach the device under devId 0.
             */
            if (shieldId == 1)
            {
                PlatformInterfaces_swapI2cIds();
                Board_swapSpiIds();
            }
        }

        BoardPinsAvian->gpioReset = ShieldConnectorDefinition[shieldId].dio3;
        BoardPinsAvian->gpioIrq   = ShieldConnectorDefinition[shieldId].irq0;
        const sr_t avianDetection = Avian_Detect(spi, gpio, BoardRadarDefinitionAvian, BoardPinsAvian);
        if (avianDetection == E_SUCCESS)
        {
            Avian_Constructor(&m_avian, &DataAvian, gpio, spi, BoardRadarDefinitionAvian, BoardPinsAvian);
            Commands_IRadarAvian_register(&m_avian.b_IRadarAvian);
            DataAvian_Constructor(Board_acquisitionStatusCallback);
            DataAvian_setBuffer(BoardRadarDefinitionAvian->dataIndex, m_dataBuffer, DATA_BUFFER_SIZE);
            BoardIrqPinsAvian->pin = PlatformGpio_getPortPin(ShieldConnectorDefinition[shieldId].irq0);
            DataAvian_initialize(BoardRadarDefinitionAvian->dataIndex, BoardIrqPinsAvian, useQspi);
            m_debugAvianRadar = &m_avian.b_IRadarAvian;
            m_debugAvianDataIndex = BoardRadarDefinitionAvian->dataIndex;
            return &DataAvian;
        }

        const sr_t ltr11Detection = Ltr11_Detect(spi, BoardRadarDefinitionLtr11);
        if (ltr11Detection == E_SUCCESS)
        {
            const uint8_t devId       = 0;
            BoardPinsLtr11->gpioReset = ShieldConnectorDefinition[shieldId].dio3;
            BoardPinsLtr11->targetDet = ShieldConnectorDefinition[shieldId].irq0;
            BoardPinsLtr11->phaseDet  = ShieldConnectorDefinition[shieldId].irq1;
            if (!hatvanPlus)
            {
                if (Board_detectShieldLtr11Legacy(&ShieldConnectorDefinition[shieldId]))
                {
                    BoardPinsLtr11->targetDet = ShieldConnectorDefinition[shieldId].dio2;
                }
            }
            BoardIrqPinsLtr11->pin = BoardSpiDefinition[devId].pins.miso;
            Ltr11_Constructor(&m_ltr11, &DataLtr11, gpio, spi, BoardRadarDefinitionLtr11, BoardPinsLtr11, BoardIrqPinsLtr11);
            Commands_IRadarLtr11_register(&m_ltr11.b_IRadarLtr11);
            DataLtr11_Constructor(Board_acquisitionStatusCallback);
            DataLtr11_initialize(BoardRadarDefinitionLtr11->dataIndex, (IProtocolLtr11 *)&m_ltr11.m_protocol, (IPinsLtr11 *)&m_ltr11.m_pins);
            uint16_t *buffer          = (uint16_t *)(uintptr_t)m_dataBuffer;
            const uint32_t bufferSize = sizeof(m_dataBuffer) / sizeof(*buffer);
            DataLtr11_setBuffer(BoardRadarDefinitionLtr11->dataIndex, buffer, bufferSize);
            return &DataLtr11;
        }

        BoardPinsSmartar->gpioReset = GPIO_ID('C', 0);  // A0
        const sr_t smartarDetection = Smartar_Detect(spi, gpio, BoardRadarDefinitionSmartar, BoardPinsSmartar);
        if (smartarDetection == E_SUCCESS)
        {
            BoardIrqPinsSmartar[0][0].pin = PlatformGpio_getPortPin(GPIO_ID('C', 12));  // A1
            BoardIrqPinsSmartar[0][1].pin = PlatformGpio_getPortPin(GPIO_ID('E', 0));   // A2
            BoardIrqPinsSmartar[0][2].pin = PlatformGpio_getPortPin(GPIO_ID('A', 19));  // A3
            BoardIrqPinsSmartar[0][3].pin = PlatformGpio_getPortPin(GPIO_ID('E', 4));   // A5
            Smartar_Constructor(&m_smartar, spi, gpio, BoardPinsSmartar, BoardRadarDefinitionSmartar);
            Commands_IRadarSmartar_register(&m_smartar.b_IRadarSmartar);
            DataSmartar_Constructor(Board_acquisitionStatusCallback);
            uint16_t *buffer          = (uint16_t *)(uintptr_t)m_dataBuffer;
            const uint32_t bufferSize = sizeof(m_dataBuffer) / sizeof(*buffer) / DATA_SMARTAR_INDEX_COUNT;
            for (uint8_t i = 0; i < DATA_SMARTAR_INDEX_COUNT; i++)
            {
                DataSmartar_initialize(i, BoardRadarDefinitionSmartar->devId, &BoardIrqPinsSmartar[0][i]);
                DataSmartar_setBuffer(i, buffer, bufferSize);
                buffer += bufferSize;
            }
            return &DataSmartar;
        }

        BoardPinsAtr22->gpioReset     = ShieldConnectorDefinition[shieldId].dio3;
        BoardPinsAtr22->gpioEnableLdo = ShieldConnectorDefinition[shieldId].en_ldo;
        Atr22_WorkaroundEs(gpio, BoardPinsAtr22);
        const sr_t atr22Detection = Atr22_Detect(i2c, RadarAtr22Definition);
        if (atr22Detection == E_SUCCESS)
        {
            BoardIrqPinsAtr22[0][0].pin = PlatformGpio_getPortPin(ShieldConnectorDefinition[shieldId].od1);
            BoardIrqPinsAtr22[0][1].pin = PlatformGpio_getPortPin(ShieldConnectorDefinition[shieldId].od2);
            BoardIrqPinsAtr22[0][2].pin = PlatformGpio_getPortPin(ShieldConnectorDefinition[shieldId].od3);
            BoardIrqPinsAtr22[0][3].pin = PlatformGpio_getPortPin(ShieldConnectorDefinition[shieldId].od4);
            Atr22_Constructor(&m_atr22, &DataAtr22, i2c, RadarAtr22Definition);
            Commands_IRadarAtr22_register(&m_atr22.b_IRadarAtr22);
            DataAtr22_Constructor(Board_acquisitionStatusCallback);
            uint16_t *buffer          = (uint16_t *)(uintptr_t)m_dataBuffer;
            const uint32_t bufferSize = sizeof(m_dataBuffer) / sizeof(*buffer) / DATA_ATR22_INDEX_COUNT;
            for (uint8_t i = 0; i < DATA_ATR22_INDEX_COUNT; i++)
            {
                DataAtr22_initialize(i, (IProtocolAtr22 *)&m_atr22.m_protocol, &BoardIrqPinsAtr22[0][i]);
                DataAtr22_setBuffer(i, buffer, bufferSize);
                buffer += bufferSize;
            }
            return &DataAtr22;
        }
    }

    // undo swapping of ids since no radar has been detected
    if (m_shieldConnectorCount == 2)
    {
        PlatformInterfaces_swapI2cIds();
        Board_swapSpiIds();
    }

    LedSequence_setRbbStatus(RBB_ERROR_HARDWARE_NOT_DETECTED);
    return NULL;
}

/****************************************************************************
 * Public methods implementation
 ****************************************************************************/
void Board_Constructor(void)
{
    // initialize platform-specifics and generic low-level platform interfaces
    Platform_Constructor();

    IGpio *gpio = &PlatformGpio;
    ISpi *spi   = &PlatformSpi;
    II2c *i2c   = &PlatformI2c;

    // initialize LedSequence to be used for potential error signaling during detection
    LedSequence_Constructor();
    LedSequence_setStatus(LED_STATUS_OPERATING);

    /* Detect which board type we are running on */
    const bool isHatvanPlus = Board_isHatvanPlus();

    /* DataAvian supports data readout over Quad SPI.
     * However, the needed pins are only available on HatvanPlus.
     * By default, do not use QSPI.
     * It can conditionally be enabled depending on "isHatvanPlus"
     */
    bool useQspi = false;

    /* The shield detection is different for different board types */
    sr_t shieldDetected;

    if (isHatvanPlus)
    {
        // useQspi = true;  // only enable this line for a build with QSPI support

        memcpy(boardInfo.name, BOARD_NAME_HATVAN_PLUS, sizeof(BOARD_NAME_HATVAN_PLUS));
        if (Sams70RevisionA())
        {
            // update character from 'B' to 'A' making name suffix " (MCU A)"
            boardInfo.name[sizeof(BOARD_NAME_HATVAN_PLUS) - 3]--;
        }
        // select connector configuration
        BoardSpiDefinition        = BoardSpiDefinitionHatvanPlus;
        ShieldConnectorDefinition = ShieldConnectorDefinitionHatvanPlus;
        m_shieldConnectorCount    = ARRAY_SIZE(ShieldConnectorDefinitionHatvanPlus);
        m_spiDefinitionsCount     = ARRAY_SIZE(BoardSpiDefinitionHatvanPlus);
        shieldDetected            = Board_detectShieldV9(ShieldConnectorDefinitionHatvanPlus);
        if (shieldDetected == E_SUCCESS)
        {
            // update deviating CSN pins for V9
            BoardSpiDefinition->pins.csn    = PlatformGpio_getPortPin(SPI_CSN_V9);
            ShieldConnectorDefinition->csn0 = SPI_CSN_V9;
            ShieldConnectorDefinition->csn1 = GPIO_NAME_NONE;
        }
    }
    else
    {
        if (Sams70RevisionA())
        {
            // update character from 'B' to 'A' making name suffix " (MCU A)"
            boardInfo.name[sizeof(BOARD_NAME_HATVAN_LEGACY) - 3]--;
        }
        // select connector configuration
        BoardSpiDefinition        = BoardSpiDefinitionHatvanLegacy;
        ShieldConnectorDefinition = ShieldConnectorDefinitionHatvanLegacy;
        m_shieldConnectorCount    = ARRAY_SIZE(ShieldConnectorDefinitionHatvanLegacy);
        m_spiDefinitionsCount     = ARRAY_SIZE(BoardSpiDefinitionHatvanLegacy);
        shieldDetected            = E_NOT_AVAILABLE;  // V9 connector not present
    }
    PlatformSpi_initialize(BoardSpiDefinition, m_spiDefinitionsCount);

    /*
    1) check for shield connector error
    mirrored connection can only be detected if the shield has I2C pull-ups
    other unexpected levels on OC pins are interpreted as unsupported shield connection
    */

    if (shieldDetected != E_SUCCESS)
    {
        shieldDetected = Board_detectShields();
    }

    if (shieldDetected == E_SUCCESS)
    {
        /*
        2) Check presence of known devices using SPI and I2C

        try reading the chip ID and instantiate the corresponding driver in the following order
        (only one device is every instantiated)

        - Avian (SPI)
        - LTR11 (SPI)
        - ATR22 (I2C)

        Note: DataAvian supports data readout over Quad SPI. Nevertheless the required HW support
              is only available on HatvanPlus board, so only then should m_useQspi be set to true.
        */
        m_data = Board_detectRadar(gpio, spi, i2c, isHatvanPlus, useQspi);
    }

    /*
    3) Continue with normal initialization, so that even without a detected device, I2C, SPI, GPIO, etc. are usable
    */

    // Select the output mode in BoardOutput.c by changing the m_mode initializer.
    //这里是二次开发串口输出新增的内容
    BoardOutput_Constructor(gpio, spi, m_data, i2c, m_macroBuffer, MACRO_BUFFER_SIZE);

    if (m_data != NULL)
    {
        // register callback function to handle arriving data
        m_data->registerCallback(Board_dataCallback, NULL);
    }

    if (m_debugAvianRadar != NULL)
    {
        AppRadarDebugAcquisition_Constructor(m_debugAvianRadar, m_data, m_debugAvianDataIndex);
    }
}

void Board_run(void)
{
    // needs to be called for state machines, etc.
    Platform_run();

    // these run functions can just be called unconditionally, since they will immediately return by default if they are not initialized
    DataAvian_run();
    DataLtr11_run();
    DataAtr22_run();
    DataSmartar_run();

    //新增部分
    BoardOutput_run();
    AppRadarDebugAcquisition_run();
    AppRangeFft_run();
   


    LedSequence_run();
}

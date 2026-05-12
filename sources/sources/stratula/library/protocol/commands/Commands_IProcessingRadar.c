/**
 * \file 	Commands_IProcessingRadar.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IProcessingRadar               IProcessingRadar Commands
 * \brief           Radar Processing interface Commands.
 *
 * @{
 */
#include "Commands_IProcessingRadar.h"
#include <common/errors.h>
#include <common/serialization.h>
#include <common/type_serialization.h>
#include <protocol/RequestHandler.h>
#include <universal/components/processing.h>
#include <universal/components/processing/iprocessingradar.h>
#include <universal/components/subinterfaces.h>
#include <universal/protocol/protocol_definitions.h>

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/


#define TYPE COMPONENT_TYPE_PROCESSING_RADAR


static IProcessingRadar *m_instances[MAX_INSTANCE_REGISTRATIONS];  // instance registrations
static bool m_registered = false;

static uint8_t _read(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
static uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
static uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


static ICommands _Commands = {
    .m_type   = TYPE,
    .m_count  = 0,  // number of currently registered instances
    .read     = _read,
    .write    = _write,
    .transfer = _transfer,
};

#define m_instanceCount _Commands.m_count


static IfxRsp_Signal m_lastOperationResult;

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/

/** Configure processing unit to process incoming data.
 *
 *  @param wLength must be sizeIProcessingRadar_Descriptor()
 *  @param payload buffer containing serialized IProcessingRadar_Descriptor.
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is incorrect value
 *                  E_FAILED if parameters were valid but execution failed
 */
static uint8_t Commands_IProcessingRadar_configure(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);
static uint8_t Commands_IProcessingRadar_getRawData(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload);


/** Do the specified operation
 *
 *  @param wLength serialized buffer length
 *  @param payload Buffer containing parameters for operation
 *
 *  @return bStatus STATUS_SUCCESS if successful, otherwise it returns an error-code
 */
static uint8_t Commands_IProcessingRadar_doFft(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);
static uint8_t Commands_IProcessingRadar_doNci(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);
static uint8_t Commands_IProcessingRadar_doPsd(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);
static uint8_t Commands_IProcessingRadar_doThresholding(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);

/** Get result of the previous operation (e.g. FFT, NCI, ...)
 *
 *  @param wLength must be sizeof_serialized_IfxRsp_Signal
 *  @param payload buffer pointer where the read-data will be stored
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not 1
 */
static uint8_t Commands_IProcessingRadar_readOperationResult(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload);

/** Write processing unit's configuration directly into Ram-configuration area
 *
 *  @param wLength 2 + length in bytes of the "n" 32-bit values in payload buffer
 *  @param payload buffer containing "n" 32-bit values (LSB first)
 *                 offset in the configuration RAM (in 32-bit words), 16bit value
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not multiple of 4
 *                  error_platform if execution was not successful for some reason
 */
static uint8_t Commands_IProcessingRadar_writeConfigRam(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);

/** Over-write window-coefficients of an already configured slot
 *
 *  @param wLength 3 + length in bytes of the "n" 32-bit values in payload buffer
 *  @param payload buffer containing "n" 32-bit values as follows (LSB first)
 *                 offset (16bit)
 *                 Number of the already configured slot (8bit)
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not multiple of 4
 *                  error_platform if execution was not successful for some reason
 */
static uint8_t Commands_IProcessingRadar_writeCustomWindowCoefficients(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);

/** Re-initialize unit (reset configuration).
 *
 *  @param wLength must be 0
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not 0
 *                  E_FAILED if parameters were valid but execution failed
 */
static uint8_t Commands_IProcessingRadar_reinit(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);

/** Start processing unit as configured.
 *
 *  @param wLength must be 0
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not 0
 *                  E_FAILED if parameters were valid but execution failed
 */
static uint8_t Commands_IProcessingRadar_start(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload);

/** Get busy state, i.e. check if all configured data has been received and processed.
 *
 *  @param wLength must be 1 (i.e. 1 byte of payload will be provided)
 *  @param payload buffer pointer where the read-data will be stored:
 *                 payload[0] = Busy state: 1=true, 0=false
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_COMMAND_WLENGTH_INVALID if wLength is not 1
 *                  E_FAILED if parameters were valid but execution failed
 */
static uint8_t Commands_IProcessingRadar_isBusy(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload);

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

bool Commands_IProcessingRadar_register(IProcessingRadar *instance)
{
    if (!m_registered)
    {
        if (!RequestHandler_registerComponentImplementation(&_Commands))
        {
            return false;
        }
        m_registered = true;
    }

    if (m_instanceCount < MAX_INSTANCE_REGISTRATIONS)
    {
        m_instances[m_instanceCount++] = instance;
        return true;
    }

    return false;
}

static inline IProcessingRadar *getInstance(uint8_t bId)
{
    if (bId < m_instanceCount)
    {
        return m_instances[bId];
    }

    return NULL;
}


/******************************************************************************/
/* Public Methods Definition -------------------------------------------------*/
/******************************************************************************/
uint8_t Commands_IProcessingRadar_doFft(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    IfxRsp_Signal input;
    IfxRsp_FftSetting fftSettings;
    uint16_t samples, sample_offset;
    uint8_t dimension, format;

    const uint16_t expectedLength = sizeof_serialized_IfxRsp_Signal() + sizeof_serialized_IfxRsp_FftSetting() + sizeof(samples) + sizeof(sample_offset) + sizeof(dimension) + sizeof(format);
    if (expectedLength != wLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    payload = serialToHost_IfxRsp_Signal(payload, &input);
    payload = serialToHost_IfxRsp_FftSetting(payload, &fftSettings);

    payload = serialToHost(payload, samples);
    payload = serialToHost(payload, sample_offset);
    payload = serialToHost(payload, dimension);
    payload = serialToHost(payload, format);

    return processingRadar->doFft(processingRadar, &input, &fftSettings, &m_lastOperationResult, samples, sample_offset, dimension, format);
}

uint8_t Commands_IProcessingRadar_doNci(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    IfxRsp_Signal input;
    uint8_t outputFormat;

    const uint16_t expectedLength = sizeof_serialized_IfxRsp_Signal() + sizeof(outputFormat);
    if (expectedLength != wLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }
    payload = serialToHost_IfxRsp_Signal(payload, &input);
    payload = serialToHost(payload, outputFormat);

    return processingRadar->doNci(processingRadar, &input, outputFormat, &m_lastOperationResult);
}

uint8_t Commands_IProcessingRadar_doPsd(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    IfxRsp_Signal input;
    uint16_t nFft;

    const uint16_t expectedLength = sizeof_serialized_IfxRsp_Signal() + sizeof(nFft);
    if (expectedLength != wLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }
    payload = serialToHost_IfxRsp_Signal(payload, &input);
    payload = serialToHost(payload, nFft);

    return processingRadar->doPsd(processingRadar, &input, nFft, &m_lastOperationResult);
}

uint8_t Commands_IProcessingRadar_doThresholding(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    IfxRsp_Signal input;
    uint8_t dimension;
    IfxRsp_ThresholdingSetting settings;

    const uint16_t expectedLength = sizeof_serialized_IfxRsp_Signal() + sizeof(dimension) +
                                    sizeof_serialized_IfxRsp_ThresholdingSetting();
    if (expectedLength != wLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }
    payload = serialToHost_IfxRsp_Signal(payload, &input);
    payload = serialToHost(payload, dimension);
    payload = serialToHost_IfxRsp_ThresholdingSetting(payload, &settings);

    return processingRadar->doThresholding(processingRadar, &input, dimension, &settings, &m_lastOperationResult);
}

uint8_t Commands_IProcessingRadar_configure(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    uint8_t dataSource;
    IDataProperties_t dataProperties;
    IProcessingRadarInput_t radarInfo;
    IfxRsp_Stages stages;
    IfxRsp_AntennaCalibration calibration[2];

    const uint16_t expectedLength = sizeof(dataSource) + sizeof_serialized_IDataProperties() + sizeof_serialized_IProcessingRadarInput() + sizeof_serialized_IfxRsp_Stages();
    const uint16_t optionalLength = expectedLength + 2 * sizeof_serialized_IfxRsp_AntennaCalibration();
    bool optional                 = false;
    if (wLength == optionalLength)
    {
        optional = true;
    }
    else if (wLength != expectedLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    payload = serialToHost(payload, dataSource);
    payload = serialToHost_IDataProperties(payload, &dataProperties);
    payload = serialToHost_IProcessingRadarInput(payload, &radarInfo);
    payload = serialToHost_IfxRsp_Stages(payload, &stages);
    if (optional)
    {
        payload = serialToHost_IfxRsp_AntennaCalibration(payload, &calibration[0]);
        serialToHost_IfxRsp_AntennaCalibration(payload, &calibration[1]);
        return processingRadar->configure(processingRadar, dataSource, &dataProperties, &radarInfo, &stages, calibration);
    }
    else
    {
        return processingRadar->configure(processingRadar, dataSource, &dataProperties, &radarInfo, &stages, NULL);
    }
}

uint8_t Commands_IProcessingRadar_getRawData(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload)
{
    if (wLength != sizeof_serialized_IfxRsp_Signal())
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    IfxRsp_Signal *rawData;
    RETURN_ON_ERROR(processingRadar->getRawData(processingRadar, &rawData));
    hostToSerial_IfxRsp_Signal(*payload, rawData);
    return STATUS_SUCCESS;
}

uint8_t Commands_IProcessingRadar_readOperationResult(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload)
{
    if (wLength != sizeof_serialized_IfxRsp_Signal())
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    hostToSerial_IfxRsp_Signal(*payload, &m_lastOperationResult);
    return STATUS_SUCCESS;
}

uint8_t Commands_IProcessingRadar_writeConfigRam(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    const uint16_t argSize = sizeof(uint16_t);
    const uint16_t length  = wLength - argSize;  //pure length of the values part without additional parameters
    //wLength (without offset) shall be a multiple of 4
    if (length & 0x0003)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const uint16_t count   = length / 4;
    const uint32_t *values = (uint32_t *)(uintptr_t)payload;
    const uint16_t offset  = serialToHost16(payload + length);

    return processingRadar->writeConfigRam(processingRadar, offset, count, values);
}

uint8_t Commands_IProcessingRadar_writeCustomWindowCoefficients(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    const uint16_t argSize = sizeof(uint16_t) + sizeof(uint8_t);
    const uint16_t length  = wLength - argSize;  //pure length of the coefficients part without additional parameters
    //wLength shall be a multiple of 4
    if (length & 0x0003)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const uint16_t count         = length / sizeof(uint32_t);
    const uint32_t *coefficients = (uint32_t *)(uintptr_t)payload;
    const uint16_t offset        = serialToHost16(payload + length);
    const uint8_t slotNr         = payload[length + sizeof(offset)];

    return processingRadar->writeCustomWindowCoefficients(processingRadar, slotNr, offset, count, coefficients);
}

uint8_t Commands_IProcessingRadar_reinit(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != 0)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    return processingRadar->reinitialize(processingRadar);
}

uint8_t Commands_IProcessingRadar_start(IProcessingRadar *processingRadar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != 0)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    return processingRadar->start(processingRadar);
}

uint8_t Commands_IProcessingRadar_isBusy(IProcessingRadar *processingRadar, uint16_t wLength, uint8_t **payload)
{
    if (wLength != sizeof(uint8_t))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    bool state    = processingRadar->isBusy(processingRadar);
    (*payload)[0] = (uint8_t)state;
    return STATUS_SUCCESS;
}

static uint8_t _read(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    IProcessingRadar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
            break;
    }

    switch (bFunction)
    {
        case FN_PROCESSING_RADAR_IS_BUSY:
            return Commands_IProcessingRadar_isBusy(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_READ_OPERATION_RESULT:
            return Commands_IProcessingRadar_readOperationResult(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_GET_RAW_DATA:
            return Commands_IProcessingRadar_getRawData(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

static uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    IProcessingRadar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
            break;
    }

    switch (bFunction)
    {
        case FN_PROCESSING_RADAR_WRITE_CONFIG_RAM:
            return Commands_IProcessingRadar_writeConfigRam(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_WRITE_CUSTOM_WINDOW_COEFFICIENTS:
            return Commands_IProcessingRadar_writeCustomWindowCoefficients(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_CONFIGURE:
            return Commands_IProcessingRadar_configure(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_DO_FFT:
            return Commands_IProcessingRadar_doFft(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_DO_NCI:
            return Commands_IProcessingRadar_doNci(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_DO_PSD:
            return Commands_IProcessingRadar_doPsd(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_DO_THRESHOLDING:
            return Commands_IProcessingRadar_doThresholding(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_REINIT:
            return Commands_IProcessingRadar_reinit(instance, wLength, payload);
            break;
        case FN_PROCESSING_RADAR_START:
            return Commands_IProcessingRadar_start(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

static uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    IProcessingRadar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
            break;
    }

    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

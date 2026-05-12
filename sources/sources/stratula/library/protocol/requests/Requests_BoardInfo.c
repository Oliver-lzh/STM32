/**
 * \file 	Requests_SystemInfo.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Requests_SystemInfo               System Information Commands
 * \brief           System Information related Commands.
 *
 * @{
 */
#include "Requests_BoardInfo.h"
#include <board/BoardInfo.h>
#include <common/errors.h>
#include <impl/custom/Bootloader.h>
#include <platform/ids/getUuid.h>
#include <universal/protocol/protocol_definitions.h>


/** Read out the board information
 *
 *  @param wValue  unused
 *  @param wIndex  unused
 *  @param wLength depending on length of BOARD_NAME
 *  @param payload buffer where the requested data is:
 *                 payload[0] = BOARD_VID (LSB)
 *                 payload[1] = BOARD_VID (MSB)
 *                 payload[2] = BOARD_PID (LSB)
 *                 payload[3] = BOARD_PID (MSB)
 *                 payload[4] to payload[n] = "BOARD_NAME" (including the terminator character '\0')
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_REQUEST_WLENGTH_INVALID if length is not 32
 */
static inline uint8_t Requests_getBoardInfo(uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    if (wLengthIn != 0)
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }
    if (*wLengthOut < sizeof(boardInfo))
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }

    *payloadOut = (uint8_t *)&boardInfo;
    *wLengthOut = sizeof(boardInfo);

    return STATUS_SUCCESS;
}

static inline uint8_t Requests_Bootloader(uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    if (wIndex != 0)
    {
        return STATUS_REQUEST_WINDEX_INVALID;
    }

    if (wLength != 0)
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }

    const sr_t ret = Bootloader_activate();

    /* If the bootloader is activated sucessfully, the following code will not be reached
     * and a timeout will occur on the host.
     * If for whatever reason the bootloader is not activated, the host will receive an error code.
     */
    if (ret == E_SUCCESS)
    {
        // the function did not return an error, but if we reach here, the bootloader was not activated
        return E_FAILED;
    }
    return ret;
}

/** Read out the software version information
 *
 *  @param wValue  unused
 *  @param wIndex  unused
 *  @param wLength must be 16
 *  @param payload buffer where the requested data is:
 *                 payload[0] = VERSION_MAJOR (LSB)
 *                 payload[1] = VERSION_MAJOR (MSB)
 *                 payload[2] = VERSION_MINOR (LSB)
 *                 payload[3] = VERSION_MINOR (MSB)
 *                 payload[4] = VERSION_PATCH (LSB)
 *                 payload[5] = VERSION_PATCH (MSB)
 *                 payload[6] = VERSION_BUILD (LSB)
 *                 payload[7] = VERSION_BUILD (MSB)
 *                 payload[8] = PROTOCOL_VERSION_MAJOR (LSB)
 *                 payload[9] = PROTOCOL_VERSION_MAJOR (MSB)
 *                 payload[A] = PROTOCOL_VERSION_MINOR (LSB)
 *                 payload[B] = PROTOCOL_VERSION_MINOR (MSB)
 *                 payload[C] - payload[F] = reserved
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_REQUEST_WLENGTH_INVALID if length is not valid
 */
static inline uint8_t Requests_getVersionInfo(uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    if (wLength != sizeof(versionInfo))
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }

    *payload = (uint8_t *)&versionInfo;

    return STATUS_SUCCESS;
}

static inline uint8_t Requests_getUuid(uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    if (wLength != UUID_LENGTH)
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }

    return getUuid(*payload);
}

/** Read out the software version information
 *
 *  @param wValue  unused
 *  @param wIndex  unused
 *  @param wLength depends on length of version string
 *  @param payload buffer where the requested data is:
 *                 payload[0] to payload[n] = "EXTENDEND_VERSION" (char string including the terminator character '\0')
 *
 *  @return bStatus STATUS_SUCCESS if parameters were valid and execution successful
 *                  STATUS_REQUEST_WLENGTH_INVALID if length is not valid
 */
static inline uint8_t Requests_getExtendedVersion(uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    if (wLengthIn != 0)
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }
    if (*wLengthOut < sizeof(extendedVersion))
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }

    *payloadOut = (uint8_t *)&extendedVersion;
    *wLengthOut = sizeof(extendedVersion);

    return STATUS_SUCCESS;
}


uint8_t Requests_BoardInfo_read(uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    switch (wValue)
    {
        case REQ_BOARD_INFO_VERSION_INFO_WVALUE:
            return Requests_getVersionInfo(wIndex, wLength, payload);
            break;
        case REQ_BOARD_INFO_UUID_WVALUE:
            return Requests_getUuid(wIndex, wLength, payload);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_WVALUE_INVALID;
}

uint8_t Requests_BoardInfo_write(uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    switch (wValue)
    {
        case REQ_BOARD_INFO_BOOTLOADER_WVALUE:
            return Requests_Bootloader(wIndex, wLength, payload);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_WVALUE_INVALID;
}

uint8_t Requests_BoardInfo_transfer(uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    switch (wValue)
    {
        case REQ_BOARD_INFO_BOARD_INFO_WVALUE:
            return Requests_getBoardInfo(wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case REQ_BOARD_INFO_EXTENDED_VERSION_WVALUE:
            return Requests_getExtendedVersion(wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
        default:
            break;
    }
    return STATUS_REQUEST_WVALUE_INVALID;
}

/*  @} */

#include "ProtocolHandlerLibUsb.h"
#include "../ProtocolHandlerData.h"
#include "../RequestHandler.h"
#include "../VendorProtocolRequests.h"

#include <common/typeutils.h>
#include <stddef.h>


/******************************************************************************/
/* Type Definitions ----------------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/

ALIGNED_BUFFER(uint8_t, m_buffer, uint32_t, LIBUSB_MAX_DATA_LENGTH);             // used to transmit data from main loop
ALIGNED_BUFFER(uint8_t, m_responseBuffer, uint32_t, LIBUSB_MAX_REQUEST_LENGTH);  // used to transmit control responses from ISR

static uint8_t m_errorInfo[4]     = {0, 0, 0, 0};  // bRequest, bStatus, wLength
static uint8_t *m_responsePayload = NULL;
static uint16_t m_responseLength  = 0;


/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/

static void ProtocolHandlerLibUsb_callbackControlOut(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[]);
static bool ProtocolHandlerLibUsb_callbackControlIn(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);


/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

static inline uint8_t getLastStatus(void)
{
    return m_errorInfo[1];
}

static inline void setRequestStatus(uint8_t bmReqType, int8_t bStatus)
{
    m_errorInfo[0] = bmReqType;
    m_errorInfo[1] = bStatus;
}

static void ProtocolHandlerLibUsb_write(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[])
{
    if (wLength > LIBUSB_MAX_REQUEST_LENGTH)
    {
        setRequestStatus(VENDOR_REQ_WRITE, STATUS_PAYLOAD_TOO_LONG);
        return;
    }

    const uint8_t bStatus = RequestHandler_write(bRequest,
                                                 wValue,
                                                 wIndex,
                                                 wLength,
                                                 data);
    setRequestStatus(VENDOR_REQ_WRITE, bStatus);
}

static bool ProtocolHandlerLibUsb_read(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    if (wLength > LIBUSB_MAX_REQUEST_LENGTH)
    {
        setRequestStatus(VENDOR_REQ_READ, STATUS_PAYLOAD_TOO_LONG);
        return false;
    }

    if ((bRequest == REQ_BOARD_INFO) &&
        (wValue == REQ_BOARD_INFO_ERROR_INFO_WVALUE) &&
        (wIndex == REQ_BOARD_INFO_ERROR_INFO_LAST_ERROR_WINDEX))
    {
        LibUsb_sendControlData(m_errorInfo, sizeof(m_errorInfo));
        return true;
    }

    m_responsePayload     = m_responseBuffer;
    const uint8_t bStatus = RequestHandler_read(bRequest,
                                                wValue,
                                                wIndex,
                                                wLength,
                                                &m_responsePayload);
    setRequestStatus(VENDOR_REQ_READ, bStatus);
    if (bStatus != E_SUCCESS)
    {
        return false;
    }
    LibUsb_sendControlData(m_responsePayload, wLength);
    return true;
}

static void ProtocolHandlerLibUsb_transfer(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[])
{
    if (wLength > LIBUSB_MAX_REQUEST_LENGTH)
    {
        setRequestStatus(VENDOR_REQ_TRANSFER, STATUS_PAYLOAD_TOO_LONG);
        return;
    }

    m_responseLength  = LIBUSB_MAX_REQUEST_LENGTH;
    m_responsePayload = m_responseBuffer;

    const uint8_t bStatus = RequestHandler_transfer(bRequest,
                                                    wValue,
                                                    wIndex,
                                                    wLength,
                                                    data,
                                                    &m_responseLength,
                                                    &m_responsePayload);

    setRequestStatus(VENDOR_REQ_TRANSFER, bStatus);
}

static sr_t ProtocolHandlerLibUsb_sendDataPacket(VendorProtocol_DataPacketHeader *header, const uint8_t *payload, uint64_t timestamp)
{
    if ((header->bmPktType & DATA_FRAME_FLAG_TIMESTAMP))
    {
        const uint16_t payloadLength = header->wLength - sizeof(timestamp);

        memcpy(m_buffer + sizeof_DataPacketHeader(), payload, payloadLength);                         // adding payload after header
        memcpy(m_buffer + sizeof_DataPacketHeader() + payloadLength, &timestamp, sizeof(timestamp));  // saving time stamp to end of the total packet
    }
    else
    {
        memcpy(m_buffer + sizeof_DataPacketHeader(), payload, header->wLength);  // adding payload after header
    }

    VendorProtocol_serializeFrameHeader(header, m_buffer);

    const uint16_t length = header->wLength + sizeof_DataPacketHeader();

    return LibUsb_sendBulkData(m_buffer, length);
}

/*
 * Invoked from ISR when device receives control endpoint IN requests
 * IN direction means: Host expects to receive a data payload from device
 */
bool ProtocolHandlerLibUsb_callbackControlIn(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    ProtocolHandlerLibUsb_setDataFrameSending();

    if (bmReqType == VENDOR_REQ_READ)
    {
        return ProtocolHandlerLibUsb_read(bRequest, wValue, wIndex, wLength);
    }
    else if (bmReqType == VENDOR_REQ_TRANSFER_2)
    {
        if (m_errorInfo[0] != VENDOR_REQ_TRANSFER)
        {
            setRequestStatus(VENDOR_REQ_TRANSFER_2, STATUS_REQUEST_NOT_AVAILABLE);
        }
        else if (getLastStatus() == E_SUCCESS)
        {
            LibUsb_sendControlData(m_responsePayload, m_responseLength);
            return true;
        }
    }
    else
    {
        setRequestStatus(bmReqType, STATUS_REQUEST_TYPE_INVALID);
    }

    return false;
}

/*
 * Invoked from ISR when device receives control endpoint OUT requests
 * OUT direction means: Host has transmitted a data payload to device
 */
void ProtocolHandlerLibUsb_callbackControlOut(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[])
{
    ProtocolHandlerLibUsb_setDataFrameSending();

    if (bmReqType == VENDOR_REQ_WRITE)
    {
        ProtocolHandlerLibUsb_write(bRequest, wValue, wIndex, wLength, data);
    }
    else if (bmReqType == VENDOR_REQ_TRANSFER)
    {
        ProtocolHandlerLibUsb_transfer(bRequest, wValue, wIndex, wLength, data);
    }
    else
    {
        setRequestStatus(bmReqType, STATUS_REQUEST_TYPE_INVALID);
    }
}

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/
void ProtocolHandlerLibUsb_Constructor(void)
{
    LibUsb_Constructor(&ProtocolHandlerLibUsb_callbackControlIn, &ProtocolHandlerLibUsb_callbackControlOut);
}

void ProtocolHandlerLibUsb_setDataFrameSending(void)
{
    ProtocolHandlerData_setSendFunction(&ProtocolHandlerLibUsb_sendDataPacket, LIBUSB_MAX_DATA_LENGTH);
}

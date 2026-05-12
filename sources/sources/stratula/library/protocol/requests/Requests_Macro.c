#include "Requests_Macro.h"

#include <common/errors.h>
#include <common/typeutils.h>
#include <impl/thread.h>
#include <protocol/RequestHandler.h>
#include <protocol/VendorProtocolRequests.h>
#include <protocol/requests/IRequests.h>
#include <universal/protocol/protocol_definitions.h>


static uint8_t _read(uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload);
static uint8_t _write(uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload);
static uint8_t _transfer(uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);

static IRequests m_requests = {
    .read     = _read,
    .write    = _write,
    .transfer = _transfer,
};

static uint8_t *m_requestBuffer;
static uint8_t *m_responseBuffer;
static uint8_t *m_request;

static uint32_t m_requestBufferSize  = 0;
static uint32_t m_responseBufferSize = 0;
static uint32_t m_responseBufferSent = 0;

static uint32_t m_requestLength  = 0;
static uint32_t m_responseLength = 0;


static inline uint32_t getPadding(uint32_t length)
{
    return (-length) % sizeof(uint32_t);
}

static void resetResponse(void)
{
    m_responseLength     = 0;
    m_responseBuffer     = m_requestBuffer + m_requestLength;
    m_responseBufferSize = m_requestBufferSize - m_requestLength;
    m_responseBufferSent = 0;
}

static void resetRequest(void)
{
    m_requestLength = 0;
}

static uint8_t *getResponseBuffer(uint16_t wLength)
{
    const uint64_t sizeRequired = m_responseLength + sizeof_ResponseHeader() + wLength + getPadding(wLength);
    if (sizeRequired > m_responseBufferSize)
    {
        return NULL;
    }

    return m_responseBuffer + m_responseLength + sizeof_ResponseHeader();
}

static void addResponse(uint8_t bmReqType, uint8_t bStatus, uint16_t wLength)
{
    VendorProtocol_ResponseHeader respHeader;
    respHeader.bmReqType = bmReqType;
    respHeader.bStatus   = bStatus;
    respHeader.wLength   = wLength;

    VendorProtocol_serializeResponseHeader(&respHeader, m_responseBuffer + m_responseLength);
    m_responseLength += sizeof_ResponseHeader() + wLength + getPadding(wLength);
}

static sr_t executeWrite(VendorProtocol_RequestHeader *reqHeader, uint8_t *requestPayload)
{
    if (reqHeader->wLength > m_requestLength)
    {
        return E_UNDERFLOW;
    }
    if (getResponseBuffer(0) == NULL)
    {
        return E_OVERFLOW;
    }

    const uint8_t bStatus = RequestHandler_write(reqHeader->bRequest,
                                                 reqHeader->wValue,
                                                 reqHeader->wIndex,
                                                 reqHeader->wLength,
                                                 requestPayload);
    addResponse(VENDOR_REQ_WRITE, bStatus, 0);
    return E_SUCCESS;
}

static sr_t executeRead(VendorProtocol_RequestHeader *reqHeader)
{
    uint8_t *responseBuffer = getResponseBuffer(reqHeader->wLength);
    if (responseBuffer == NULL)
    {
        return E_OVERFLOW;
    }

    uint8_t *responsePayload = responseBuffer;
    const uint8_t bStatus    = RequestHandler_read(reqHeader->bRequest,
                                                   reqHeader->wValue,
                                                   reqHeader->wIndex,
                                                   reqHeader->wLength,
                                                   &responsePayload);
    if (bStatus != STATUS_SUCCESS)
    {
        reqHeader->wLength = 0;
    }
    if (responsePayload != responseBuffer)
    {
        memcpy(responseBuffer, responsePayload, reqHeader->wLength);
    }
    addResponse(VENDOR_REQ_READ, bStatus, reqHeader->wLength);
    return E_SUCCESS;
}

static sr_t executeTransfer(VendorProtocol_RequestHeader *reqHeader, uint8_t *requestPayload)
{
    if (reqHeader->wLength > m_requestLength)
    {
        return E_UNDERFLOW;
    }

    const uint32_t sizeAvailable = m_responseBufferSize - m_responseLength - sizeof_ResponseHeader();
    uint16_t wResponseLength     = (sizeAvailable > UINT16_MAX) ? UINT16_MAX : sizeAvailable;
    uint8_t *responseBuffer      = getResponseBuffer(wResponseLength);
    if (responseBuffer == NULL)
    {
        return E_OVERFLOW;
    }

    uint8_t *responsePayload = responseBuffer;
    uint8_t bStatus          = RequestHandler_transfer(reqHeader->bRequest,
                                                       reqHeader->wValue,
                                                       reqHeader->wIndex,
                                                       reqHeader->wLength,
                                                       requestPayload,
                                                       &wResponseLength,
                                                       &responsePayload);
    if (bStatus != STATUS_SUCCESS)
    {
        wResponseLength = 0;
    }
    if (responsePayload != responseBuffer)
    {
        memcpy(responseBuffer, responsePayload, wResponseLength);
    }
    addResponse(VENDOR_REQ_TRANSFER, bStatus, wResponseLength);
    return E_SUCCESS;
}

static void updateRequest(uint32_t length)
{
    const uint32_t offset = length + getPadding(length);
    m_request += offset;
    m_requestLength -= offset;
}

static sr_t executeMacro(void)
{
    m_request = m_requestBuffer;
    while (m_requestLength)
    {
        const uint8_t bmReqType = *m_request;
        if (m_requestLength < sizeof_RequestHeader())
        {
            return E_UNDERFLOW;
        }
        VendorProtocol_RequestHeader reqHeader;
        VendorProtocol_unserializeRequestHeader(&reqHeader, m_request);
        updateRequest(sizeof_RequestHeader());

        switch (bmReqType)
        {
            case VENDOR_REQ_WRITE:
                RETURN_ON_ERROR(executeWrite(&reqHeader, m_request));
                updateRequest(reqHeader.wLength);
                break;
            case VENDOR_REQ_READ:
                RETURN_ON_ERROR(executeRead(&reqHeader));
                break;
            case VENDOR_REQ_TRANSFER:
                RETURN_ON_ERROR(executeTransfer(&reqHeader, m_request));
                updateRequest(reqHeader.wLength);
                break;
            default:
                return E_UNEXPECTED_VALUE;
                break;
        }
    }

    return E_SUCCESS;
}

static sr_t requestBufferHandler(uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    if (wLength == 0)
    {
        const sr_t result = executeMacro();
        resetRequest();
        return result;
    }

    const uint32_t availableSize = m_requestBufferSize - m_requestLength;
    if (availableSize < wLength)
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }

    // add bytes to the request buffer
    uint8_t *requestBuffer = m_requestBuffer + m_requestLength;
    memcpy(requestBuffer, payload, wLength);
    m_requestLength += wLength;

    // adjust response buffer
    resetResponse();

    return E_SUCCESS;
}

static sr_t extendedFunctionalityHandler(uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    if (wIndex != REQ_MACRO_EXTENDED_FUNC_DELAY_WINDEX)
    {
        return STATUS_REQUEST_WINDEX_INVALID;
    }

    // wait a specific delay in microseconds
    uint32_t delay;
    if (wLength != sizeof(delay))
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }
    serialToHost(payload, delay);
    this_thread_sleep_for(chrono_microseconds(delay));

    return E_SUCCESS;
}

/******************************************************************************/
/* Interface Methods Definition -------------------------------------------------*/
/******************************************************************************/

static uint8_t _read(uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    if (wLength != sizeof(uint32_t))
    {
        return STATUS_REQUEST_WLENGTH_INVALID;
    }

    switch (wValue)
    {
        case REQ_MACRO_REQUEST_BUFFER_WVALUE:
            *payload = (uint8_t *)&m_requestBufferSize;
            break;
        case REQ_MACRO_RESPONSE_BUFFER_WVALUE:
            *payload = (uint8_t *)&m_responseLength;
            break;
        default:
            return STATUS_REQUEST_WVALUE_INVALID;
            break;
    }

    return E_SUCCESS;
}

static uint8_t _write(uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    switch (wValue)
    {
        case REQ_MACRO_REQUEST_BUFFER_WVALUE:
            return requestBufferHandler(wIndex, wLength, payload);
            break;
        case REQ_MACRO_EXTENDED_FUNC_WVALUE:
            return extendedFunctionalityHandler(wIndex, wLength, payload);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_WVALUE_INVALID;
}

static uint8_t _transfer(uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    if (wValue != REQ_MACRO_RESPONSE_BUFFER_WVALUE)
    {
        return STATUS_REQUEST_WVALUE_INVALID;
    }

    // transmit bytes from the response buffer
    const uint32_t maxLength = *wLengthOut;
    const uint32_t bytesLeft = m_responseLength - m_responseBufferSent;
    *wLengthOut              = (bytesLeft > maxLength) ? maxLength : bytesLeft;
    *payloadOut              = m_responseBuffer + m_responseBufferSent;
    m_responseBufferSent += *wLengthOut;

    // empty response buffer after transmission completed
    if (m_responseBufferSent == m_responseLength)
    {
        resetResponse();
    }

    return E_SUCCESS;
}

/******************************************************************************/
/* Public Methods Definition -------------------------------------------------*/
/******************************************************************************/

void Requests_Macro_register(uint8_t *buffer, uint32_t bufferSize)
{
    m_requestBuffer     = buffer;
    m_requestBufferSize = bufferSize;
    RequestHandler_registerMacro(&m_requests);
}

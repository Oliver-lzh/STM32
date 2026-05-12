#include "ProtocolHandlerEthernet.h"
#include "../RequestHandler.h"
#include "../VendorProtocolRequests.h"

static uint8_t ProtocolHandlerEthernet_write(struct pbuf *request, uint16_t maxPacketSize)
{
    VendorProtocol_RequestHeader reqHeader;
    VendorProtocol_unserializeRequestHeader(&reqHeader, request->payload);

    if (reqHeader.wLength == (request->tot_len - sizeof_RequestHeader()))
    {
        //Execute write-request
        const uint8_t *requestPayload = (const uint8_t *)request->payload + sizeof_RequestHeader();
        return RequestHandler_write(reqHeader.bRequest,
                                    reqHeader.wValue,
                                    reqHeader.wIndex,
                                    reqHeader.wLength,
                                    requestPayload);
    }
    else if (reqHeader.wLength > (maxPacketSize - sizeof_RequestHeader()))
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }
    else  // if (reqHeader.wLength > (maxPacketSize - sizeof_RequestHeader()))
    {
        return STATUS_PAYLOAD_INCOMPLETE;
    }
}

static uint8_t ProtocolHandlerEthernet_read(struct pbuf *request, uint16_t *wResponseLength, uint8_t **responsePayloadPtr, uint16_t maxPacketSize)
{
    VendorProtocol_RequestHeader reqHeader;
    VendorProtocol_unserializeRequestHeader(&reqHeader, request->payload);

    uint8_t bStatus;
    if (reqHeader.wLength <= (maxPacketSize - sizeof_ResponseHeader()))
    {
        //Execute read-request

        bStatus = RequestHandler_read(reqHeader.bRequest,
                                      reqHeader.wValue,
                                      reqHeader.wIndex,
                                      reqHeader.wLength,
                                      responsePayloadPtr);
        if (bStatus == STATUS_SUCCESS)
        {
            *wResponseLength = reqHeader.wLength;
        }
        else
        {
            *wResponseLength = 0;
        }
    }
    else
    {
        bStatus = STATUS_PAYLOAD_TOO_LONG;
    }
    return bStatus;
}

static uint8_t ProtocolHandlerEthernet_transfer(struct pbuf *request, uint16_t *wResponseLength, uint8_t **responsePayloadPtr, uint16_t maxPacketSize)
{
    VendorProtocol_RequestHeader reqHeader;
    VendorProtocol_unserializeRequestHeader(&reqHeader, request->payload);

    uint8_t bStatus;
    *wResponseLength = maxPacketSize - sizeof_ResponseHeader();
    if (reqHeader.wLength == (request->tot_len - sizeof_RequestHeader()))
    {
        //Execute transfer-request
        const uint8_t *requestPayload = (const uint8_t *)request->payload + sizeof_RequestHeader();
        bStatus                       = RequestHandler_transfer(reqHeader.bRequest,
                                                                reqHeader.wValue,
                                                                reqHeader.wIndex,
                                                                reqHeader.wLength,
                                                                requestPayload,
                                                                wResponseLength,
                                                                responsePayloadPtr);
    }
    else if (reqHeader.wLength > (maxPacketSize - sizeof_RequestHeader()))
    {
        bStatus = STATUS_PAYLOAD_TOO_LONG;
    }
    else  // if (reqHeader.wLength > (maxPacketSize - sizeof_RequestHeader()))
    {
        bStatus = STATUS_PAYLOAD_INCOMPLETE;
    }

    if (bStatus != STATUS_SUCCESS)
    {
        *wResponseLength = 0;
    }
    return bStatus;
}

uint8_t ProtocolHandlerEthernet_requestReceived(uint8_t bmReqType, struct pbuf *request, uint16_t *wResponseLength, uint8_t **responsePayloadPtr, uint16_t maxPacketSize)
{
    //Proccess request
    if (request->tot_len != request->len)
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }
    if (request->tot_len > maxPacketSize)
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }
    if (request->tot_len < sizeof_RequestHeader())
    {
        return STATUS_HEADER_INCOMPLETE;
    }

    switch (bmReqType)
    {
        case VENDOR_REQ_WRITE:
            return ProtocolHandlerEthernet_write(request, maxPacketSize);
        case VENDOR_REQ_READ:
            return ProtocolHandlerEthernet_read(request, wResponseLength, responsePayloadPtr, maxPacketSize);
        case VENDOR_REQ_TRANSFER:
            return ProtocolHandlerEthernet_transfer(request, wResponseLength, responsePayloadPtr, maxPacketSize);
        default:
            return STATUS_REQUEST_TYPE_INVALID;
    }
}

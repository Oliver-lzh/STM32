#include "ProtocolHandlerTcp.h"
#include "../ProtocolHandlerData.h"
#include "../RequestHandler.h"
#include "../VendorProtocolRequests.h"
#include "ProtocolHandlerEthernet.h"
#include <platform/ethernet/lwip/SocketTcpImpl.h>
#include <platform/impl/EthernetImpl.h>

#include <common/typeutils.h>
#include <stddef.h>

///******************************************************************************/
///* Type Definitions ----------------------------------------------------------*/
///******************************************************************************/
//
///******************************************************************************/
///*Private/Public Variables ---------------------------------------------------*/
///******************************************************************************/
static SocketTcp m_socketAcceptControl;          //Control socket, where we wait for new connections
static SocketTcp m_socketAcceptData;             //Data socket, where we wait for new connections
static SocketTcp m_socketConnectionData = NULL;  //Data socket where to send data

static volatile bool m_requestActive;

ALIGNED_BUFFER(uint8_t, m_responsePayload, uint32_t, PROTOCOL_HANDLER_TCP_MAX_PACKET_SIZE - sizeof_ResponseHeader());
ALIGNED_BUFFER(uint8_t, m_responseHeader, uint32_t, sizeof_ResponseHeader());
ALIGNED_BUFFER(uint8_t, m_dataPacketHeader, uint32_t, sizeof_DataPacketHeader());

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/
static err_t ProtocolHandlerTcp_acceptCallback(void *arg, SocketTcp connectSocket, err_t err);

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

static SocketTcp ProtocolHandlerTcp_openSocket(uint16_t port, SocketTcp_acceptCb acceptCb)
{
    SocketTcp socket = SocketTcp_createSocket();
    if (socket != NULL)
    {
        if (SocketTcp_open(&socket, port) == E_SUCCESS)
        {
            SocketTcp_setAcceptCallback(socket, acceptCb, socket);
        }
        else
        {
            //Free the socket
            SocketTcp_close(socket);
            socket = NULL;
        }
    }
    return socket;
}

static void ProtocolHandlerTcp_response(SocketTcp socket, uint8_t bmReqType, uint8_t bStatus, uint16_t wLength, uint8_t *payload)
{
    VendorProtocol_ResponseHeader respHeader;
    respHeader.bmReqType = bmReqType;
    respHeader.bStatus   = bStatus;
    respHeader.wLength   = wLength;

    VendorProtocol_serializeResponseHeader(&respHeader, m_responseHeader);
    uint16_t len  = sizeof_ResponseHeader();
    bool moreData = (wLength != 0);

    SocketTcp_write(socket, m_responseHeader, len, moreData);

    if (moreData)
    {
        SocketTcp_write(socket, payload, wLength, false);
    }
}

#ifndef TEST_CEEDLING
static
#endif
    sr_t
    ProtocolHandlerTcp_sendDataPacket(VendorProtocol_DataPacketHeader *header, const uint8_t *payload, uint64_t timestamp)
{
    // Prepare and send header
    VendorProtocol_serializeFrameHeader(header, m_dataPacketHeader);
    RETURN_ON_ERROR(SocketTcp_write(m_socketConnectionData, m_dataPacketHeader, sizeof_DataPacketHeader(), true));

    // Prepare and send payload
    bool sendTimestamp = ((header->bmPktType & DATA_FRAME_FLAG_TIMESTAMP) != 0);
    uint16_t len       = sendTimestamp ? header->wLength - sizeof(timestamp) : header->wLength;
    RETURN_ON_ERROR(SocketTcp_write(m_socketConnectionData, payload, len, sendTimestamp));

    if (sendTimestamp)
    {
        RETURN_ON_ERROR(SocketTcp_write(m_socketConnectionData, (uint8_t *)(&timestamp), sizeof(timestamp), false));
    }

#ifndef CY_RTOS_AWARE
    Ethernet_run();  // on bare-metal, check if new command packet has arrived (to be able to stop streaming also mid-frame)
#endif

    return E_SUCCESS;
}


static err_t ProtocolHandlerTcp_recvCallbackControl(void *arg, SocketTcp socket, struct pbuf *request, err_t err)
{
    if (request == NULL)
    {
        // The connection was closed.
        return err;
    }

    while (m_requestActive)
        ;
    m_requestActive = true;

    ProtocolHandlerTcp_setDataFrameSending();

    //Proccess request
    uint8_t bStatus          = STATUS_SUCCESS;
    uint16_t wResponseLength = 0;
    uint8_t *responsePayload = m_responsePayload;  //local variable since the called function may change the address to return a different buffer
    const uint8_t bmReqType  = request->payload ? ((uint8_t *)request->payload)[0] : 0x00;
    bStatus                  = ProtocolHandlerEthernet_requestReceived(bmReqType, request, &wResponseLength, &responsePayload, PROTOCOL_HANDLER_TCP_MAX_PACKET_SIZE);
    ProtocolHandlerTcp_response(socket, bmReqType, bStatus, wResponseLength, responsePayload);

    m_requestActive = false;

    // Tell the socket that the data was processed
    SocketTcp_confirmReceived(socket, request->tot_len);

    //Free the memory used for request
    pbuf_free(request);
    return err;
}

static err_t ProtocolHandlerTcp_acceptCallback(void *arg, SocketTcp connectSocket, err_t err)
{
    SocketTcp acceptSocket = (SocketTcp)arg;

    //Disable nagle algorithm on each new connection to get maximum transmission speed.
    //On the control socket the request and response messages are always small, no need to wait for
    //further data as there will be none. There are never two requests or responses in sequence.
    //On the data socket packets will always be big except the last packet. No further data will be
    //sent after the last packet, so there's no need to wait.
    SocketTcp_setNagle(connectSocket, false);

    if (err == ERR_OK)
    {
        if (acceptSocket == m_socketAcceptControl)
        {
            // A new control connection just came in
            SocketTcp_setReceiveCallback(connectSocket, ProtocolHandlerTcp_recvCallbackControl, NULL);
        }
        else if (acceptSocket == m_socketAcceptData)
        {
            // A new data connection just came in
            m_socketConnectionData = connectSocket;
        }
        else
        {
            // There's a new connection on an unknown socket
            return ERR_ARG;
        }
    }

    return err;
}

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/
void ProtocolHandlerTcp_Constructor(void)
{
    m_requestActive       = false;
    m_socketAcceptControl = ProtocolHandlerTcp_openSocket(ETHERNET_CONTROL_PORT, ProtocolHandlerTcp_acceptCallback);
    m_socketAcceptData    = ProtocolHandlerTcp_openSocket(ETHERNET_DATA_PORT, ProtocolHandlerTcp_acceptCallback);
}

void ProtocolHandlerTcp_setDataFrameSending(void)
{
    ProtocolHandlerData_setSendFunction(&ProtocolHandlerTcp_sendDataPacket, PROTOCOL_HANDLER_TCP_MAX_PACKET_SIZE);
}

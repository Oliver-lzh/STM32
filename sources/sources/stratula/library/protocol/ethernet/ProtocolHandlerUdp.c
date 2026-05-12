#include "ProtocolHandlerUdp.h"
#include "../ProtocolHandlerData.h"
#include "../RequestHandler.h"
#include "../VendorProtocolRequests.h"
#include "ProtocolHandlerEthernet.h"
#include <platform/ethernet/lwip/SocketUdpImpl.h>
#include <platform/impl/EthernetImpl.h>

#include <common/typeutils.h>
#include <stddef.h>

/******************************************************************************/
/* Type Definitions ----------------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/
static SocketUdp m_socketControl;
static SocketUdp m_socketData;
static struct pbuf *m_response;
static struct pbuf *m_responsePayload;
static struct pbuf *m_dataPacket;
static struct pbuf *m_dataPacketTimeStamp;
static ip_addr_t m_remoteIp;
static uint16_t m_remotePort;
static ip_addr_t m_dataIp;
static uint16_t m_dataPort;
static volatile bool m_requestActive;

#define m_responseHeader    m_response
#define m_dataPacketHeader  m_dataPacket
#define m_dataPacketPayload m_dataPacket->next

ALIGNED_BUFFER(uint8_t, m_buffer, uint32_t, PROTOCOL_HANDLER_UDP_MAX_PACKET_SIZE);

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

static SocketUdp ProtocolHandlerUdp_openSocket(uint16_t port)
{
    SocketUdp udpSocket = SocketUdp_createSocket();
    SocketUdp_open(udpSocket, port, PROTOCOL_HANDLER_UDP_TIMEOUT, NULL, 0);
    return udpSocket;
}

static void ProtocolHandlerUdp_response(uint8_t bmReqType, uint8_t bStatus, uint16_t wLength, uint8_t *payload)
{
    VendorProtocol_ResponseHeader respHeader;
    respHeader.bmReqType = bmReqType;
    respHeader.bStatus   = bStatus;
    respHeader.wLength   = wLength;

    VendorProtocol_serializeResponseHeader(&respHeader, m_responseHeader->payload);

    m_responseHeader->len     = sizeof_ResponseHeader();
    m_responseHeader->tot_len = sizeof_ResponseHeader() + wLength;
    if (wLength)
    {
        m_responsePayload->payload = payload;
        m_responsePayload->len     = wLength;
        m_responsePayload->tot_len = wLength;
        m_responseHeader->next     = m_responsePayload;
    }
    else
    {
        m_responseHeader->next = NULL;
    }

    SocketUdp_sendTo(m_socketControl, m_response, &m_remoteIp, m_remotePort);
}

#ifndef TEST_CEEDLING
static
#endif
    sr_t
    ProtocolHandlerUdp_sendDataPacket(VendorProtocol_DataPacketHeader *header, const uint8_t *payload, uint64_t timestamp)
{
    //Update pBuf-chain info for data-packet
    m_dataPacketPayload->payload = (void *)payload;
    m_dataPacketPayload->tot_len = header->wLength;
    if (header->bmPktType & DATA_FRAME_FLAG_TIMESTAMP)
    {
        m_dataPacketPayload->len       = header->wLength - sizeof(timestamp);
        m_dataPacketPayload->next      = m_dataPacketTimeStamp;
        m_dataPacketTimeStamp->payload = (uint8_t *)&timestamp;  // time stamp is assumed to be little-endian
    }
    else
    {
        m_dataPacketPayload->len  = header->wLength;
        m_dataPacketPayload->next = NULL;
    }
    m_dataPacket->tot_len = sizeof_DataPacketHeader() + m_dataPacketPayload->tot_len;

    //Prepare header
    VendorProtocol_serializeFrameHeader(header, m_dataPacket->payload);

    //Send data packet
    RETURN_ON_ERROR(SocketUdp_sendTo(m_socketData, m_dataPacket, &m_dataIp, m_dataPort));

#ifndef CY_RTOS_AWARE
    Ethernet_run();  // on bare-metal, check if new command packet has arrived (to be able to stop streaming also mid-frame)
#endif

    return E_SUCCESS;
}

static void ProtocolHandlerUdp_recvCallbackControl(void *arg, SocketUdp socket, struct pbuf *request, const ip_addr_t *addr, uint16_t port)
{
    while (m_requestActive)
        ;
    m_requestActive = true;

    ProtocolHandlerUdp_setDataFrameSending();

    //set request IP address as response destination
    ip_addr_copy(m_remoteIp, *addr);
    m_remotePort = port;

    //Proccess request
    uint8_t bStatus          = STATUS_SUCCESS;
    uint16_t wResponseLength = 0;
    uint8_t *responsePayload = m_buffer;  //local variable since the called function may change the address to return a different buffer
    const uint8_t bmReqType  = request->payload ? ((uint8_t *)request->payload)[0] : 0x00;
    bStatus                  = ProtocolHandlerEthernet_requestReceived(bmReqType, request, &wResponseLength, &responsePayload, PROTOCOL_HANDLER_UDP_MAX_PACKET_SIZE);
    ProtocolHandlerUdp_response(bmReqType, bStatus, wResponseLength, responsePayload);

    m_requestActive = false;

    //Free the memory used for request
    pbuf_free(request);
}

static void ProtocolHandlerUdp_recvCallbackData(void *arg, SocketUdp socket, struct pbuf *request, const ip_addr_t *addr, uint16_t port)
{
    ProtocolHandlerUdp_setDataFrameSending();

    if (request->len == 0)
    {
        //set sending IP address as data destination
        ip_addr_copy(m_dataIp, *addr);
        m_dataPort = port;
    }
    //Free the memory used for message
    pbuf_free(request);
}

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/
void ProtocolHandlerUdp_Constructor(void)
{
    m_dataPort      = ETHERNET_DATA_PORT;
    m_requestActive = false;

    m_socketControl = ProtocolHandlerUdp_openSocket(ETHERNET_CONTROL_PORT);
    SocketUdp_setRecvCallback(m_socketControl, ProtocolHandlerUdp_recvCallbackControl, NULL);
    m_socketData = ProtocolHandlerUdp_openSocket(ETHERNET_DATA_PORT);
    SocketUdp_setRecvCallback(m_socketData, ProtocolHandlerUdp_recvCallbackData, NULL);

    const uint8_t dataIp[4] = PROTOCOL_HANDLER_UDP_DEFAULT_DATA_IP_ADDR;
    IP_ADDR4(&m_dataIp, dataIp[0], dataIp[1], dataIp[2], dataIp[3]);

    //Allocate a pbuf to be used for sending responses to incoming requests
    m_responseHeader  = pbuf_alloc(PBUF_RAW, sizeof_ResponseHeader(), PBUF_RAM);
    m_responsePayload = pbuf_alloc(PBUF_RAW, 0, PBUF_ROM);

    //Allocate pBufs to be used for sending data-packets
    m_dataPacketHeader    = pbuf_alloc(PBUF_RAW, sizeof_DataPacketHeader(), PBUF_RAM);
    m_dataPacketPayload   = pbuf_alloc(PBUF_RAW, 0, PBUF_ROM);
    m_dataPacketTimeStamp = pbuf_alloc(PBUF_RAW, 0, PBUF_ROM);

    m_dataPacketTimeStamp->len     = sizeof(uint64_t);
    m_dataPacketTimeStamp->tot_len = sizeof(uint64_t);
}

void ProtocolHandlerUdp_setDataFrameSending(void)
{
    ProtocolHandlerData_setSendFunction(&ProtocolHandlerUdp_sendDataPacket, PROTOCOL_HANDLER_UDP_MAX_PACKET_SIZE);
}

#ifndef PROTOCOL_HANDLER_ETHERNET_H_
#define PROTOCOL_HANDLER_ETHERNET_H_ 1

#include <lwip/pbuf.h>
#include <stdint.h>

/**
 * This file contains all commone functionality for ethernet communication
 */

uint8_t ProtocolHandlerEthernet_requestReceived(uint8_t bmReqType, struct pbuf *request, uint16_t *wResponseLength, uint8_t **responsePayloadPtr, uint16_t maxPacketSize);

#endif /* PROTOCOL_HANDLER_ETHERNET_H_ */

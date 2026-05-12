#ifndef PROTOCOL_HANDLER_TCP_H_
#define PROTOCOL_HANDLER_TCP_H_ 1

#include <universal/link_definitions.h>

#define PROTOCOL_HANDLER_TCP_MAX_PACKET_SIZE ETH_TCP_MAX_PAYLOAD
#define PROTOCOL_HANDLER_TCP_TIMEOUT         200

/** Enable the Ethernet-Tcp communication
 */
void ProtocolHandlerTcp_Constructor(void);

/** Set ProtocolHandler to use ProtocolHandlerTcp to send data frames
 */
void ProtocolHandlerTcp_setDataFrameSending(void);

#endif /* PROTOCOL_HANDLER_TCP_H_ */

#ifndef PROTOCOL_HANDLER_UDP_H_
#define PROTOCOL_HANDLER_UDP_H_ 1

#include <universal/link_definitions.h>

#define PROTOCOL_HANDLER_UDP_MAX_PACKET_SIZE ETH_UDP_MAX_PAYLOAD
#define PROTOCOL_HANDLER_UDP_TIMEOUT         200
#define PROTOCOL_HANDLER_UDP_DEFAULT_DATA_IP_ADDR \
    {                                             \
        169, 254, 255, 255                        \
    }  // broadcast by default

/** Enable the Ethernet-Udp communication
 * i.e. Establish a connection to PROTOCOL_HANDLER_UDP_REMOTEIPADDR
 * at both PROTOCOL_HANDLER_UDP_CONTROL_PORT and PROTOCOL_HANDLER_UDP_DATA_PORT
 */
void ProtocolHandlerUdp_Constructor(void);

/** Set ProtocolHandler to use ProtocolHandlerUdp to send data frames
 */
void ProtocolHandlerUdp_setDataFrameSending(void);

#endif /* PROTOCOL_HANDLER_UDP_H_ */

#ifndef PROTOCOL_HANDLER_H_
#define PROTOCOL_HANDLER_H_ 1

#include "ProtocolHandlerData.h"


#include <BoardDefinition.h>

#ifdef COMMUNICATION_SERIAL
#include "serial/ProtocolHandlerSerial.h"
#endif

#ifdef COMMUNICATION_UDP
#include "ethernet/ProtocolHandlerUdp.h"
#endif

#ifdef COMMUNICATION_TCP
#include "ethernet/ProtocolHandlerTcp.h"
#endif

#ifdef COMMUNICATION_HSSL
#include <impl/hssl/ProtocolHandlerHssl.h>
#endif

#ifdef COMMUNICATION_LIBUSB
#include "libusb/ProtocolHandlerLibUsb.h"
#endif


/**
 * Establish connection to the communication ports (i.e. Serial and Ethernet)
 *
 */
void ProtocolHandler_Constructor(void);

/**
 * Process incoming requests (if any) and sends back a response.
 * i.e. if not request available, it returns immediately.
 *
 * See VendorProtocol.h, for more info about requests and responses.
 */
void ProtocolHandler_run(void);


#endif /* PROTOCOL_HANDLER_H_ */

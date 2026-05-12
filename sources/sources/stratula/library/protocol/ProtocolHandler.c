/**
 * \file 	ProtocolHandler.c
 *
 * \addtogroup      CommunicationInterface      Communication Interface (Command-based)
 * @{
 *   \addtogroup      ProtocolHandler             Protocol Handler
 *   \brief 			Communication protocol handler.
 *   @{
 */
#include "ProtocolHandler.h"


void ProtocolHandler_Constructor(void)
{
#ifdef COMMUNICATION_UDP
    ProtocolHandlerUdp_Constructor();
#endif
#ifdef COMMUNICATION_TCP
    ProtocolHandlerTcp_Constructor();
#endif
#ifdef COMMUNICATION_SERIAL
    ProtocolHandlerSerial_Constructor();
#endif
#ifdef COMMUNICATION_HSSL
    ProtocolHandlerHssl_Constructor();
#endif
#ifdef COMMUNICATION_LIBUSB
    ProtocolHandlerLibUsb_Constructor();
#endif
}

void ProtocolHandler_run(void)
{
#ifdef COMMUNICATION_SERIAL
    ProtocolHandlerSerial_run();
#endif
#ifdef COMMUNICATION_HSSL
    ProtocolHandlerHssl_processRequest();
#endif
}


/* @}@} */

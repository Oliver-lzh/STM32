#ifndef PROTOCOL_HANDLER_LIBUSB_H_
#define PROTOCOL_HANDLER_LIBUSB_H_ 1

#include <platform/impl/LibUsbImpl.h>


void ProtocolHandlerLibUsb_Constructor(void);


/** Set ProtocolHandler to use ProtocolHandlerLibUsb to send data frames
 */
void ProtocolHandlerLibUsb_setDataFrameSending(void);


#endif /* PROTOCOL_HANDLER_LIBUSB_H_ */

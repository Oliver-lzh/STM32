#ifndef PROTOCOL_HANDLER_SERIAL_H_
#define PROTOCOL_HANDLER_SERIAL_H_ 1

#include <universal/link_definitions.h>


#define PROTOCOL_HANDLER_SERIAL_CRC_SIZE 2
#define PROTOCOL_HANDLER_SERIAL_BAUDRATE 921600
#define PROTOCOL_HANDLER_SERIAL_TIMEOUT  200


/** Enable the Serial communication
 * i.e. Open a connection through a serial-port
 *
 */
void ProtocolHandlerSerial_Constructor(void);


/** Set ProtocolHandler to use ProtocolHandlerSerial to send data frames
 */
void ProtocolHandlerSerial_setDataFrameSending(void);


/** Process incoming requests (if any) and sends back a response.
 *  i.e. if not request available, it returns immediately.
 *
 *  See VendorProtocol.h, for more info about requests and responses.
 */
void ProtocolHandlerSerial_run(void);


#endif /* PROTOCOL_HANDLER_SERIAL_H_ */

#ifndef PROTOCOL_HANDLER_DATA_H_
#define PROTOCOL_HANDLER_DATA_H_ 1

#include "VendorProtocolData.h"
#include <common/errors.h>


typedef sr_t (*ProtocolHandlerData_sendDataPacket)(VendorProtocol_DataPacketHeader *header, const uint8_t *payload, uint64_t timestamp);


void ProtocolHandlerData_setSendFunction(ProtocolHandlerData_sendDataPacket sendFunction, uint16_t maxPacketSize);


/**
 * Send data frame.
 * See VendorProtocolData.h for more info about data frames.
 *
 * @param payload the payload of the data frame
 * @param size the number of bytes of payload
 *
 * @note Requires ProtocolHandler_enableSendingDataFrames() to be called first.
 */
sr_t ProtocolHandler_sendDataFrame(const uint8_t *payload, uint32_t size, uint8_t channel, uint64_t timestamp);
#ifndef TEST_CEEDLING
sr_t ProtocolHandler_sendDataFrameArray(uint32_t data[][2], uint8_t count, uint8_t channel, uint64_t timestamp);
#endif

sr_t ProtocolHandler_sendErrorFrame(uint32_t code, uint8_t channel, uint64_t timestamp);

sr_t ProtocolHandler_sendDebugFrameImpl(char *message, uint8_t channel, uint64_t timestamp);


#endif /* PROTOCOL_HANDLER_DATA_H_ */

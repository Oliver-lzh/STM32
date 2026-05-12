#ifndef VENDOR_PROTOCOL_DATA_H_
#define VENDOR_PROTOCOL_DATA_H_ 1


#include <common/serialization.h>
#include <stdint.h>
#include <universal/protocol/protocol_definitions.h>


typedef struct
{
    uint8_t bmPktType;
    uint8_t bChannel;
    uint16_t wCounter;
    uint16_t wLength;
} VendorProtocol_DataPacketHeader;

//uint64_t timestamp (after payload)


#define sizeof_DataPacketHeader() \
    (sizeof(((VendorProtocol_DataPacketHeader *)0)->bmPktType) + sizeof(((VendorProtocol_DataPacketHeader *)0)->bChannel) + sizeof(((VendorProtocol_DataPacketHeader *)0)->wCounter) + sizeof(((VendorProtocol_DataPacketHeader *)0)->wLength))


static inline void VendorProtocol_serializeFrameHeader(const VendorProtocol_DataPacketHeader *frameHeader, uint8_t buf[])
{
    hostToSerial8(buf + 0, frameHeader->bmPktType);
    hostToSerial8(buf + 1, frameHeader->bChannel);
    hostToSerial16(buf + 2, frameHeader->wCounter);
    hostToSerial16(buf + 4, frameHeader->wLength);
}


#endif /* VENDOR_PROTOCOL_DATA_H_ */

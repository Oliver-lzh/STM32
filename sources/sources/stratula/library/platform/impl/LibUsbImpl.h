#ifndef LIBUSBIMPL_H
#define LIBUSBIMPL_H 1

#include <common/errors.h>
#include <stdbool.h>
#include <stdint.h>
#include <universal/link_definitions.h>


typedef void (*LibUsb_callbackControlOut)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[]);
typedef bool (*LibUsb_callbackControlIn)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);


void LibUsb_Constructor(LibUsb_callbackControlIn controlIn, LibUsb_callbackControlOut controlOut);

/**
 * Informs lower layer which data should be transmitted in response to control IN request
 * 
 * @param data a buffer of the specified length
 * @param length length of bytes to be written
 * 
 */
sr_t LibUsb_sendControlData(const uint8_t data[], uint16_t length);

/**
 * Write data to the remote device.
 * The write operation completes when:
 * 	- The specified data is written.
 * 	- The time specified by an internal Timeout passes (e.g. 1000 ms)
 *
 * @param data a buffer of the specified length
 * @param length number of bytes to be written
 *
 * @return Strata error code: E_TIMEOUT, if the write timeout occurs
 */
sr_t LibUsb_sendBulkData(const uint8_t data[], uint16_t length);

#endif /* LIBUSBIMPL_H */

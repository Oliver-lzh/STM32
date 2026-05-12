#ifndef IREQUESTS_H_
#define IREQUESTS_H_ 1

#include <stddef.h>
#include <stdint.h>


/**
 * This file defines the interface for request implementations
 */

typedef struct _IRequests IRequests;
struct _IRequests
{
    /** Called when a write command was received for this implementation
     *
     *  @param wValue request specific value parameter
     *  @param wIndex request specific index parameter
     *  @param wLength Length of the payload to be written
     *  @param payload buffer containing the payload
     *
     *  @return bStatus as follows:
     *  	- STATUS_REQUEST_NOT_AVAILABLE if request handler has not been registered
     *  	- bStatus returned by the write function associated with the parameters
     */
    uint8_t (*write)(uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload);

    /** Called when a read command was received for this implementation
     *
     *  @param wValue request specific value parameter
     *  @param wIndex request specific index parameter
     *  @param wLength length of payload to be read (if zero, no payload-data will be sent back to the host)
     *  @param payload buffer pointer where the read-data will be stored
     *
     *  @return bStatus as follows:
     *  	- STATUS_REQUEST_NOT_AVAILABLE if request handler has not been registered
     *  	- bStatus returned by the read function associated with the parameters
     */
    uint8_t (*read)(uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload);

    /** Called when a transfer command was received for this implementation
     *
     *  @param wValue request specific value parameter
     *  @param wIndex request specific index parameter
     *  @param wLengthIn Length of the payload to be written
     *  @param payloadIn buffer containing the payload
     *  @param wLengthOut length of returned payload (to be filled by the callee)
     *  @param payloadOut buffer pointer where the read-data will be stored
     *
     *  @return bStatus as follows:
     *  	- STATUS_REQUEST_NOT_AVAILABLE if request handler has not been registered
     *  	- bStatus returned by the read function associated with the parameters
     */
    uint8_t (*transfer)(uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);
};

#endif /* IREQUESTS_H_ */

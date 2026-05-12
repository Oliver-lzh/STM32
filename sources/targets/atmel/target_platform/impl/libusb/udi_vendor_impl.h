#ifndef _UDI_VENDOR_IMPL_H
#define _UDI_VENDOR_IMPL_H

#include <stdbool.h>
#include <stdint.h>


/*! \brief Callback function which passes setup request OUT to upper layer
 */
extern void (*vendor_request_callback_out)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[]);

/*! \brief Callback function which passes setup request IN to upper layer
 */
extern bool (*vendor_request_callback_in)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);


bool os_string_descriptor_request(void);
bool device_specific_request(void);


/*! \brief Notify via user interface that enumeration is ok
 * This is called by vendor interface when USB Host enable it.
 *
 * \retval true if vendor startup is successfully done
 */
bool usb_vendor_enable(void);

/*! \brief Notify via user interface that enumeration is disabled
 * This is called by vendor interface when USB Host disable it.
 */
void usb_vendor_disable(void);

/*! \brief Manage the reception of setup request OUT
 *
 * \retval true if request accepted
 */
bool usb_vendor_setup_out_received(void);

/*! \brief Manage the reception of setup request IN
 *
 * \retval true if request accepted
 */
bool usb_vendor_setup_in_received(void);

#endif  //UDI_VENDOR_IMPL_H

/**
 * \file
 *
 * \brief Main functions for USB Device vendor example
 *
 * Copyright (c) 2011-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "udi_vendor_impl.h"
#include "udi_vendor.h"
#include <universal/link_definitions.h>

#if (UDI_VENDOR_EP_BULK_IN != (LIBUSB_DATA_ENDPOINT | USB_EP_DIR_IN))
#error "USB bulk IN endpoint number does not match universal define expected by host! Please double-check conf_usb.h"
#endif

// Buffer for setup request payload
COMPILER_WORD_ALIGNED
static uint8_t vendor_request_buf[LIBUSB_MAX_REQUEST_LENGTH];

void (*vendor_request_callback_out)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t data[]);
bool (*vendor_request_callback_in)(uint8_t bmReqType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);


#define OS_STRING_DESCRIPTOR 0xEE
#define WCID_VENDOR_CODE     0x20


static uint8_t msft_sig_desc[] = {
    0x12,                            // length = 18 bytes
    USB_DT_STRING,                   // descriptor type string
    'M', 0, 'S', 0, 'F', 0, 'T', 0,  // 'M', 'S', 'F', 'T'
    '1', 0, '0', 0, '0', 0,          // '1', '0', '0'
    WCID_VENDOR_CODE,                // vendor code
    0                                // padding
};

static uint8_t wcid_feature_desc[] = {
    0x28, 0x00, 0x00, 0x00,                          // length = 40 bytes
    0x00, 0x01,                                      // version 1.0 (in BCD)
    0x04, 0x00,                                      // compatibility descriptor index 0x0004
    0x01,                                            // number of sections
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // reserved (7 bytes)
    0x00,                                            // interface number
    0x01,                                            // reserved
    0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00,  // Compatible ID "WINUSB\0\0"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Subcompatible ID (unused)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00               // reserved 6 bytes
};


static inline void _set_setup_payload_helper(uint8_t *payload, uint16_t payload_size)
{
    udd_g_ctrlreq.payload = payload;

    if (udd_g_ctrlreq.req.wLength < payload_size)
    {
        // trim length in case provided buffer is not sufficient
        udd_g_ctrlreq.payload_size = udd_g_ctrlreq.req.wLength;
    }
    else
    {
        udd_g_ctrlreq.payload_size = payload_size;
    }
}

// Handles Windows request for additional OS string descriptor
bool os_string_descriptor_request(void)
{
    if ((udd_g_ctrlreq.req.wValue & 0xFF) == OS_STRING_DESCRIPTOR)
    {
        // inform lower layer which data should be transmitted after ISR returns
        _set_setup_payload_helper(msft_sig_desc, sizeof(msft_sig_desc));
        return true;
    }

    return false;
}

static void out_request_callback(void)
{
    vendor_request_callback_out(udd_g_ctrlreq.req.bmRequestType,
                                udd_g_ctrlreq.req.bRequest,
                                udd_g_ctrlreq.req.wValue,
                                udd_g_ctrlreq.req.wIndex,
                                udd_g_ctrlreq.req.wLength,
                                udd_g_ctrlreq.payload);
}

static inline bool process_out_request(void)
{
    const uint16_t length = udd_g_ctrlreq.req.wLength;

    if (length > sizeof(vendor_request_buf))
    {
        // this is the only time a write request fails with a USB protocol error, which can be interpreted by the caller
        return false;  // reject data phase because payload is too large
    }

    udd_g_ctrlreq.payload      = vendor_request_buf;
    udd_g_ctrlreq.payload_size = length;
    udd_g_ctrlreq.callback     = out_request_callback;

    return true;  // acknowledge setup packet to start data phase
}

static inline bool process_in_request(void)
{
    return vendor_request_callback_in(udd_g_ctrlreq.req.bmRequestType,
                                      udd_g_ctrlreq.req.bRequest,
                                      udd_g_ctrlreq.req.wValue,
                                      udd_g_ctrlreq.req.wIndex,
                                      udd_g_ctrlreq.req.wLength);
}

/**
 * Handles incoming requests. These could be:
 * - standard vendor requests (used by LibUSB protocol implementation)
 * - feature descriptor requests (containing winusb driver information) sent by Windows
 */
bool device_specific_request(void)
{
    if (Udd_setup_type() != USB_REQ_TYPE_VENDOR)
    {
        return false;
    }
    if (Udd_setup_is_in())
    {
        if (Udd_setup_recipient() == USB_REQ_RECIP_DEVICE)
        {
            if ((udd_g_ctrlreq.req.bRequest == WCID_VENDOR_CODE) && (udd_g_ctrlreq.req.wIndex == 0x0004))
            {
                _set_setup_payload_helper(wcid_feature_desc, sizeof(wcid_feature_desc));
                return true;
            }
        }

        return process_in_request();
    }
    else  // if (Udd_setup_is_out())
    {
        return process_out_request();
    }

    return false;
}

bool usb_vendor_enable(void)
{
    // called by lower layer to inform that interface is enabled
    return true;
}

void usb_vendor_disable(void)
{
    // called by lower layer to inform that interface is disabled
}

bool usb_vendor_setup_out_received(void)
{
    // no vendor specific extensions of recipients "interface" and "endpoint"
    return false;
}

bool usb_vendor_setup_in_received(void)
{
    // no vendor specific extensions of recipients "interface" and "endpoint"
    return false;
}

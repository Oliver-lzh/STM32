/* ===========================================================================
** Copyright (C) 2021 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/
#include <platform/impl/LibUsbImpl.h>

#include "udi_vendor_impl.h"
#include <udi_vendor.h>

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

#define UDD_EP_TRANSFER_PENDING 2

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/

static volatile uint16_t m_sendBulkDataLength;
static volatile udd_ep_status_t m_sendBulkDataStatus;

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

static void vendor_bulk_in_transfer_callback(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep)
{
    m_sendBulkDataStatus = status;
    m_sendBulkDataLength = (uint16_t)nb_transfered;
}


/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/

void LibUsb_Constructor(LibUsb_callbackControlIn controlIn, LibUsb_callbackControlOut controlOut)
{
    vendor_request_callback_in  = controlIn;
    vendor_request_callback_out = controlOut;

    m_sendBulkDataStatus = UDD_EP_TRANSFER_OK;
}

sr_t LibUsb_sendBulkData(const uint8_t data[], uint16_t length)
{
    if (m_sendBulkDataStatus == UDD_EP_TRANSFER_PENDING)
    {
        return E_BUSY;
    }
    m_sendBulkDataStatus = UDD_EP_TRANSFER_PENDING;

    const bool started = udi_vendor_bulk_in_run((uint8_t *)data, length, vendor_bulk_in_transfer_callback);
    if (!started)
    {
        m_sendBulkDataStatus = UDD_EP_TRANSFER_OK;
        return E_FAILED;
    }

    while (m_sendBulkDataStatus == UDD_EP_TRANSFER_PENDING)
    {
        // wait for transfer completion
    }

    if (m_sendBulkDataStatus == UDD_EP_TRANSFER_ABORT)
    {
        return E_ABORTED;
    }

    if (m_sendBulkDataLength != length)
    {
        return E_TIMEOUT;
    }

    return E_SUCCESS;
}

sr_t LibUsb_sendControlData(const uint8_t data[], uint16_t length)
{
    // inform lower layer which data should be transmitted
    udd_set_setup_payload((uint8_t *)data, length);

    return E_SUCCESS;
}

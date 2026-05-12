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

#include <platform/impl/SerialPortImpl.h>

#include <impl/chrono.h>
#include <string.h>
#include <udi_cdc.h>

/******************************************************************************/
/*Macro Definitions ----------------------------------------------------------*/
/******************************************************************************/
#ifdef USB_DEVICE_HS_SUPPORT
#define USB_ENDPOINT_PACKET_SIZE UDI_CDC_DATA_EPS_HS_SIZE
#else
#define USB_ENDPOINT_PACKET_SIZE UDI_CDC_DATA_EPS_FS_SIZE
#endif

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/
static uint16_t com_get_data(void *data, uint16_t num_bytes)
{
    if (udi_cdc_is_rx_ready())
    {
        const iram_size_t requested = num_bytes;
        const iram_size_t available = udi_cdc_read_no_polling(data, requested);
        const iram_size_t returned  = (available < requested) ? available : requested;
        return (uint16_t)returned;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/
void SerialPort_Constructor(void)
{
}

sr_t SerialPort_open(uint32_t baudrate)
{
    return E_SUCCESS;
}

bool SerialPort_isOpened(void)
{
    return true;
}

sr_t SerialPort_close(void)
{
    return E_SUCCESS;
}


void SerialPort_clearInputBuffer(void)
{
    uint8_t rxBuffer[USB_ENDPOINT_PACKET_SIZE];
    while (com_get_data(rxBuffer, sizeof(rxBuffer)))
        ;
}

void SerialPort_flushOutputBuffer(void)
{
    udi_cdc_flush();
}

sr_t SerialPort_send(const uint8_t data[], uint16_t length)
{
    chrono_ticks_t deadline = chrono_get_timepoint(chrono_milliseconds(2000));

    while (length)
    {
        const iram_size_t freeBuf = udi_cdc_get_free_tx_buffer();
        if (freeBuf)
        {
            const iram_size_t bytesToSend = (length > freeBuf) ? freeBuf : length;
            udi_cdc_write_buf(data, bytesToSend);
            length -= bytesToSend;
            data += bytesToSend;
        }
        else if (chrono_has_passed(deadline))
        {
            return E_TIMEOUT;
        }
    }

    return E_SUCCESS;
}

sr_t SerialPort_receive(uint8_t data[], uint16_t length, uint16_t timeout, bool returnImmediately, uint16_t *count)
{
    sr_t ret                      = E_SUCCESS;
    uint16_t left                 = length;
    const chrono_ticks_t deadline = chrono_get_timepoint(chrono_milliseconds(timeout));
    while (left > 0)
    {
        const uint16_t numReceivedBytes = com_get_data(data, left);
        if ((numReceivedBytes == 0) && returnImmediately)
        {
            break;
        }

        data += numReceivedBytes;
        left -= numReceivedBytes;

        if (chrono_has_passed(deadline))
        {
            ret = E_TIMEOUT;
            break;
        }
    }

    *count = (length - left);
    return ret;
}

sr_t SerialPort_sendString(const char data[])
{
    return SerialPort_send((uint8_t *)data, strlen(data));
}

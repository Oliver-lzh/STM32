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

/*
 * \file StreamInterfaceSamUSB.c
 *
 */

#include "StreamInterface.h"
#include <universal/link_definitions.h>

#include "conf_usb.h"
#include <string.h>
#include <udi_cdc.h>


void com_init(void)
{
}

#define BUFFER_SIZE (SERIAL_MAX_PACKET_SIZE)
#define BUFFER_MASK (BUFFER_SIZE - 1)
#define THRESHOLD   256

typedef struct
{
    volatile uint32_t index_write;
    volatile uint32_t index_read;
    uint8_t buf[BUFFER_SIZE];
} buffer_t;

static buffer_t txQueue = {0, 0, {0}};

static uint32_t txForward(void)
{
    const uint32_t index_write = txQueue.index_write;
    const uint32_t index_read  = txQueue.index_read;

    uint32_t fill = (index_write - index_read);
    if (fill == 0)
    {
        return 0;
    }

    const uint32_t freeBuf = udi_cdc_get_free_tx_buffer();
    if (freeBuf == 0)
    {
        return fill;
    }

    const uint32_t read_count        = (fill > freeBuf) ? freeBuf : fill;
    const uint32_t index_read_masked = index_read & BUFFER_MASK;
    const uint32_t index_read_next   = index_read + read_count;
    if (index_read_masked + read_count > BUFFER_SIZE) /* wrap-around */
    {
        udi_cdc_write_buf(&txQueue.buf[index_read_masked], (BUFFER_SIZE - index_read_masked));
        udi_cdc_write_buf(&txQueue.buf[0], (index_read_next & BUFFER_MASK));
    }
    else
    {
        udi_cdc_write_buf(&txQueue.buf[index_read_masked], read_count);
    }
    txQueue.index_read = index_read_next;

    return fill - read_count;
}

static void txEnqueue(const uint8_t *data, uint32_t num_bytes)
{
    if (num_bytes == 0)
        return;

    uint32_t index_write = txQueue.index_write;
    do
    {
        const uint32_t index_read = txQueue.index_read;
        const uint32_t fill       = index_write - index_read;
        const uint32_t freeBuf    = BUFFER_SIZE - fill;
        if (freeBuf == 0)
        {
            txForward();
        }
        else
        {
            const uint32_t write_count        = (num_bytes > freeBuf) ? freeBuf : num_bytes;
            const uint32_t index_write_masked = index_write & BUFFER_MASK;
            const uint32_t index_write_next   = index_write + write_count;
            if (index_write_masked + write_count > BUFFER_SIZE)
            {
                const uint32_t segment_length = BUFFER_SIZE - index_write_masked;
                memcpy(&txQueue.buf[index_write_masked], data, segment_length);
                memcpy(&txQueue.buf[0], (data + segment_length), index_write_next & BUFFER_MASK);
            }
            else
            {
                memcpy(&txQueue.buf[index_write_masked], data, write_count);
            }
            index_write         = index_write_next,
            txQueue.index_write = index_write;

            if (fill + write_count > THRESHOLD)
            {
                txForward();
            }

            data += write_count;
            num_bytes = num_bytes - write_count;
        }
    } while (num_bytes > 0);
}

void com_send_data(const void *data, uint16_t num_bytes)
{
    txEnqueue(data, num_bytes);
}


uint16_t com_get_data(void *data, uint16_t num_bytes)
{
    if (udi_cdc_is_rx_ready())
    {
        const iram_size_t requested = num_bytes;
        const iram_size_t available =
            udi_cdc_read_no_polling(data, requested);
        const iram_size_t returned =
            (available < requested) ? available : requested;
        return (uint16_t)returned;
    }
    else
    {
        return 0;
    }
}

void com_flush()
{
    while (txForward() != 0)
    {
    };
    udi_cdc_flush();
}

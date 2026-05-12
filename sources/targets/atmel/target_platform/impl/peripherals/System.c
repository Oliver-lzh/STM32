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
 * \file System.c
 *
 */

#include "System.h"
#include <rstc.h>
#include <sleepmgr.h>
#include <sysclk.h>
#include <udc.h>


void System_init(void)
{
    irq_initialize_vectors();
    cpu_irq_enable();

    // Initialize the sleep manager
    sleepmgr_init();

#if !SAM0
    sysclk_init();
    SCB_EnableICache();
#ifdef CONF_BOARD_ENABLE_CACHE_AT_INIT
    SCB_EnableDCache();
#endif

    //board_init();
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOC);
    pmc_enable_periph_clk(ID_PIOD);
#else
    system_init();
#endif
}

void System_reset(void)
{
    rstc_start_software_reset(RSTC);
}

void System_disableWatchdog(void)
{
    /* disable watchdog */
    WDT->WDT_MR = 0x3fffafff;
}

void System_startUSB(void)
{
    udc_start();
}

#ifdef TEST_USB_THROUGHPUT
void System_usb_throughput_test()
{
    while (true)
    {
        static uint8_t buffer[4096];
        /* Measure USB Speed by pushing out any received packets */
        const uint32_t free         = udi_cdc_get_free_tx_buffer();
        const uint32_t received     = udi_cdc_get_nb_received_data();
        const uint32_t transferable = free < received ? free : received;
        if (transferable == 0)
            continue; /* wait until data is there to be transferred */

        const uint32_t requested = (transferable > 4096u) ? 4096u : transferable;
        const iram_size_t available =
            udi_cdc_read_no_polling(buffer, requested);
        const iram_size_t read =
            (available < requested) ? available : requested;

        udi_cdc_write_buf(buffer, read);
    }
}
#endif
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

#ifndef BOARD_OUTPUT_H_
#define BOARD_OUTPUT_H_ 1

#include <common/errors.h>
#include <platform/interfaces/IData.h>
#include <platform/interfaces/IGpio.h>
#include <platform/interfaces/II2c.h>
#include <platform/interfaces/ISpi.h>
#include <stdint.h>

#if defined(__GNUC__)
#define BOARD_OUTPUT_PRINTF_FORMAT(formatIndex, firstArg) __attribute__((format(gnu_printf, formatIndex, firstArg)))
#else
#define BOARD_OUTPUT_PRINTF_FORMAT(formatIndex, firstArg)
#endif

typedef enum
{
    BOARD_OUTPUT_MODE_HOST_PROTOCOL = 0,
    BOARD_OUTPUT_MODE_DEBUG_TEXT    = 1,
} BoardOutput_Mode_t;

void BoardOutput_Constructor(IGpio *gpio,
                             ISpi *spi,
                             IData *data,
                             II2c *i2c,
                             uint8_t *macroBuffer,
                             uint32_t macroBufferSize);

void BoardOutput_run(void);
BoardOutput_Mode_t BoardOutput_getMode(void);
sr_t BoardOutput_setMode(BoardOutput_Mode_t mode);
sr_t BoardOutput_onFrame(const uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp);
sr_t BoardOutput_printf(const char *format, ...) BOARD_OUTPUT_PRINTF_FORMAT(1, 2);

#endif /* BOARD_OUTPUT_H_ */

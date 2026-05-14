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

/**
 * @file AppRangeFft.h
 * @brief Range FFT interface for BGT60TR13C radar
 *
 * Implements single chirp Range FFT to find target range bin.
 * The data callback should only submit a small chirp copy. The actual FFT is
 * executed later from the main loop by AppRangeFft_run().
 *
 * Input:  Packed12 format (64 samples/chirp, 96 bytes/chirp)
 * Output: fft,bin=<peak_bin>,mag=<log2_mag> debug text (rate-limited)
 */

#ifndef APP_RANGE_FFT_H_
#define APP_RANGE_FFT_H_ 1

#include <stdint.h>

void AppRangeFft_initialize(void);
void AppRangeFft_submitFrame(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp);
void AppRangeFft_run(void);
void AppRangeFft_process(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp);

#endif /* APP_RANGE_FFT_H_ */

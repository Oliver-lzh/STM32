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
 * Implements fixed-point Range FFT to find target range bin.
 * The data callback submits one full Packed12 frame. The actual FFT is
 * executed later from the main loop by AppRangeFft_run().
 *
 * Input:  Packed12 frame data (64 samples/chirp, 128 chirps, 3 RX)
 * Debug path: RX1 only, full-frame Range FFT with last-chirp bin voting.
 * The voted peak bin is smoothed into a stable bin before exposing one frame of slow-time I/Q for phase processing.
 */

#ifndef APP_RANGE_FFT_H_
#define APP_RANGE_FFT_H_ 1

#include <stdbool.h>
#include <stdint.h>

#define APP_RANGE_FFT_ADC_RX_CHANNELS        (3u)
#define APP_RANGE_FFT_ADC_CHIRPS_PER_FRAME   (128u)
#define APP_RANGE_FFT_ADC_SAMPLES_PER_CHIRP  (64u)
#define APP_RANGE_FFT_PHASE_SAMPLE_COUNT     APP_RANGE_FFT_ADC_CHIRPS_PER_FRAME

typedef uint16_t AppRangeFft_AdcCube_t[APP_RANGE_FFT_ADC_RX_CHANNELS][APP_RANGE_FFT_ADC_CHIRPS_PER_FRAME][APP_RANGE_FFT_ADC_SAMPLES_PER_CHIRP];

typedef struct
{
    bool valid;
    uint32_t frameIndex;
    uint8_t rx;
    uint8_t lockedBin;
    uint8_t voteCount;
    uint16_t phaseSampleCount;
    int16_t i[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
    int16_t q[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
    /* Preview I/Q from the last phase sample; names kept for API compatibility. */
    int16_t iAvg;
    int16_t qAvg;
} AppRangeFft_Result_t;

void AppRangeFft_initialize(void);
bool AppRangeFft_convertPayloadToAdcCube(const uint8_t *payload, uint32_t byteCount, AppRangeFft_AdcCube_t adc);
void AppRangeFft_submitFrame(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp);
void AppRangeFft_run(void);
void AppRangeFft_process(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp);
bool AppRangeFft_getLatestResult(AppRangeFft_Result_t *result);

#endif /* APP_RANGE_FFT_H_ */

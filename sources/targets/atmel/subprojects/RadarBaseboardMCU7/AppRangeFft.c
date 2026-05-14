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

#include "AppRangeFft.h"
#include "BoardOutput.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define APP_RANGE_FFT_SAMPLES_PER_CHIRP     (64u)
#define APP_RANGE_FFT_RX_CHANNELS           (3u)
#define APP_RANGE_FFT_CHIRPS_PER_FRAME      (128u)
#define APP_RANGE_FFT_RX_TO_PROCESS         (0u)
#define APP_RANGE_FFT_CHIRP_SAMPLE_COUNT    (APP_RANGE_FFT_SAMPLES_PER_CHIRP * APP_RANGE_FFT_RX_CHANNELS)
#define APP_RANGE_FFT_FRAME_SAMPLE_COUNT    (APP_RANGE_FFT_CHIRPS_PER_FRAME * APP_RANGE_FFT_CHIRP_SAMPLE_COUNT)
#define APP_RANGE_FFT_FRAME_BYTE_COUNT      ((APP_RANGE_FFT_FRAME_SAMPLE_COUNT * 3u) / 2u)
#define APP_RANGE_FFT_CHIRP_BYTE_COUNT      ((APP_RANGE_FFT_SAMPLES_PER_CHIRP * 3u) / 2u)
#define APP_RANGE_FFT_DEBUG_RATE_DIVISOR    (16u)
#define APP_RANGE_FFT_INIT_DELAY_FRAMES     (8u)
#define APP_RANGE_FFT_SEARCH_START_BIN      (2u)
#define APP_RANGE_FFT_SEARCH_END_BIN        ((APP_RANGE_FFT_SAMPLES_PER_CHIRP / 2u) - 1u)
#define APP_RANGE_FFT_MAG_LOG_SHIFT         (14u)
#define APP_RANGE_FFT_SAMPLE_SCALE_SHIFT    (4u)

static const int16_t g_cosQ15[APP_RANGE_FFT_SAMPLES_PER_CHIRP] = {
     32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
     23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212,
         0,   3212,   6393,   9512,  12539,  15446,  18204,  20787,
     23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609
};

static const int16_t g_sinQ15[APP_RANGE_FFT_SAMPLES_PER_CHIRP] = {
         0,   3212,   6393,   9512,  12539,  15446,  18204,  20787,
     23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609,
     32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
     23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212
};

static uint32_t m_frameCounter = 0u;
static bool m_pendingFrame = false;
static uint8_t m_pendingPacked12[APP_RANGE_FFT_CHIRP_BYTE_COUNT];
static uint8_t m_pendingChannel = 0u;
static uint64_t m_pendingTimestamp = 0u;

static uint16_t unpack_packed12_sample(const uint8_t *packed, uint32_t sampleIndex)
{
    const uint32_t byteIndex = (sampleIndex * 3u) >> 1;

    if ((sampleIndex & 1u) == 0u)
    {
        return (uint16_t)packed[byteIndex] | ((uint16_t)(packed[byteIndex + 1u] >> 4) << 8);
    }

    return (uint16_t)(packed[byteIndex] & 0x0Fu) | ((uint16_t)packed[byteIndex + 1u] << 4);
}

static uint64_t abs_i64(int64_t value)
{
    if (value < 0)
    {
        return (uint64_t)(-value);
    }

    return (uint64_t)value;
}

static uint32_t log2_u64(uint64_t value)
{
    uint32_t result = 0u;

    while (value > 1u)
    {
        value >>= 1u;
        result++;
    }

    return result;
}

static uint32_t compress_magnitude(uint64_t value)
{
    const uint32_t logMagnitude = log2_u64(value);

    if (logMagnitude <= APP_RANGE_FFT_MAG_LOG_SHIFT)
    {
        return 0u;
    }

    return logMagnitude - APP_RANGE_FFT_MAG_LOG_SHIFT;
}

static uint64_t compute_bin_magnitude(const uint16_t *samples, int32_t mean, uint32_t bin)
{
    int64_t realSum = 0;
    int64_t imagSum = 0;

    for (uint32_t sample = 0u; sample < APP_RANGE_FFT_SAMPLES_PER_CHIRP; sample++)
    {
        const uint32_t tableIndex = (bin * sample) & (APP_RANGE_FFT_SAMPLES_PER_CHIRP - 1u);
        const int32_t centered = ((int32_t)samples[sample] - mean) >> APP_RANGE_FFT_SAMPLE_SCALE_SHIFT;

        realSum += (int64_t)centered * (int64_t)g_cosQ15[tableIndex];
        imagSum -= (int64_t)centered * (int64_t)g_sinQ15[tableIndex];
    }

    return abs_i64(realSum) + abs_i64(imagSum);
}

static void unpack_chirp(const uint8_t *packed12Data, uint16_t *samples, int32_t *mean)
{
    uint32_t sum = 0u;

    for (uint32_t index = 0u; index < APP_RANGE_FFT_SAMPLES_PER_CHIRP; index++)
    {
        samples[index] = unpack_packed12_sample(packed12Data, index);
        sum += samples[index];
    }

    *mean = (int32_t)(sum / APP_RANGE_FFT_SAMPLES_PER_CHIRP);
}

void AppRangeFft_initialize(void)
{
    m_frameCounter = 0u;
    m_pendingFrame = false;
}

void AppRangeFft_submitFrame(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp)
{
    if ((packed12Data == NULL) || (byteCount < APP_RANGE_FFT_FRAME_BYTE_COUNT))
    {
        return;
    }

    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        m_pendingFrame = false;
        return;
    }

    if (m_pendingFrame)
    {
        return;
    }

    memcpy(m_pendingPacked12, packed12Data, APP_RANGE_FFT_CHIRP_BYTE_COUNT);
    m_pendingChannel = channel;
    m_pendingTimestamp = timestamp;
    m_pendingFrame = true;
}

void AppRangeFft_run(void)
{
    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        m_pendingFrame = false;
        return;
    }

    if (!m_pendingFrame)
    {
        return;
    }

    m_pendingFrame = false;
    AppRangeFft_process(m_pendingPacked12, APP_RANGE_FFT_CHIRP_BYTE_COUNT, m_pendingChannel, m_pendingTimestamp);
}

void AppRangeFft_process(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp)
{
    uint16_t samples[APP_RANGE_FFT_SAMPLES_PER_CHIRP];
    int32_t mean = 0;
    uint32_t peakBin = APP_RANGE_FFT_SEARCH_START_BIN;
    uint64_t peakMagnitude = 0u;

    (void)timestamp;
    (void)channel;

    if ((packed12Data == NULL) || (byteCount < APP_RANGE_FFT_CHIRP_BYTE_COUNT))
    {
        return;
    }

    m_frameCounter++;

    if (m_frameCounter < APP_RANGE_FFT_INIT_DELAY_FRAMES)
    {
        return;
    }

    unpack_chirp(packed12Data, samples, &mean);

    for (uint32_t bin = APP_RANGE_FFT_SEARCH_START_BIN; bin <= APP_RANGE_FFT_SEARCH_END_BIN; bin++)
    {
        const uint64_t magnitude = compute_bin_magnitude(samples, mean, bin);

        if (magnitude > peakMagnitude)
        {
            peakMagnitude = magnitude;
            peakBin = bin;
        }
    }

    if ((m_frameCounter % APP_RANGE_FFT_DEBUG_RATE_DIVISOR) != 1u)
    {
        return;
    }

    (void)BoardOutput_printf("fft,bin=%lu,mag=%lu\r\n",
                             (unsigned long)peakBin,
                             (unsigned long)compress_magnitude(peakMagnitude));
}

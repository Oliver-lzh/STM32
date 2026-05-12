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

#include "AppRadarProbe.h"
#include "BoardOutput.h"

#include <stdbool.h>
#include <stddef.h>

#define APP_RADAR_PROBE_PACKED12_FRAME_SIZE         (18432u)
#define APP_RADAR_PROBE_RAW16_FRAME_SIZE            (24576u)
#define APP_RADAR_PROBE_CLASSIC_PACKED12_FRAME_SIZE (36864u)
#define APP_RADAR_PROBE_CLASSIC_RAW16_FRAME_SIZE    (49152u)
#define APP_RADAR_PROBE_FIRST_PRINT_COUNT           (4u)
#define APP_RADAR_PROBE_PRINT_INTERVAL              (16u)
#define APP_RADAR_PROBE_HEAD_BYTE_COUNT             (8u)

static uint32_t m_frameCounter = 0u;
static uint64_t m_lastTimestamp = 0u;
static bool m_hasLastTimestamp = false;

static const char *AppRadarProbe_classifyCount(uint32_t count)
{
    if (count == APP_RADAR_PROBE_PACKED12_FRAME_SIZE)
    {
        return "packed12";
    }

    if (count == APP_RADAR_PROBE_RAW16_FRAME_SIZE)
    {
        return "raw16";
    }

    if (count == APP_RADAR_PROBE_CLASSIC_PACKED12_FRAME_SIZE)
    {
        return "packed12-classic128";
    }

    if (count == APP_RADAR_PROBE_CLASSIC_RAW16_FRAME_SIZE)
    {
        return "raw16-classic128";
    }

    return "unknown";
}

static bool AppRadarProbe_shouldPrint(uint32_t frameCounter)
{
    if (frameCounter <= APP_RADAR_PROBE_FIRST_PRINT_COUNT)
    {
        return true;
    }

    return ((frameCounter % APP_RADAR_PROBE_PRINT_INTERVAL) == 0u);
}

void AppRadarProbe_onFrame(const uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    uint64_t dt = 0u;
    uint8_t head[APP_RADAR_PROBE_HEAD_BYTE_COUNT] = {0u};

    m_frameCounter++;

    if (m_hasLastTimestamp)
    {
        dt = timestamp - m_lastTimestamp;
    }
    m_lastTimestamp = timestamp;
    m_hasLastTimestamp = true;

    if (!AppRadarProbe_shouldPrint(m_frameCounter))
    {
        return;
    }

    if (payload != NULL)
    {
        uint32_t index;
        const uint32_t headCount = (count < APP_RADAR_PROBE_HEAD_BYTE_COUNT) ? count : APP_RADAR_PROBE_HEAD_BYTE_COUNT;

        for (index = 0u; index < headCount; index++)
        {
            head[index] = payload[index];
        }
    }

    (void)BoardOutput_printf("probe,f=%lu,cnt=%lu,ch=%u,ts=%08lx%08lx,dt=%lu,fmt=%s,head=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                             (unsigned long)m_frameCounter,
                             (unsigned long)count,
                             (unsigned int)channel,
                             (unsigned long)(timestamp >> 32),
                             (unsigned long)(timestamp & 0xFFFFFFFFu),
                             (unsigned long)(dt & 0xFFFFFFFFu),
                             AppRadarProbe_classifyCount(count),
                             (unsigned int)head[0],
                             (unsigned int)head[1],
                             (unsigned int)head[2],
                             (unsigned int)head[3],
                             (unsigned int)head[4],
                             (unsigned int)head[5],
                             (unsigned int)head[6],
                             (unsigned int)head[7]);
}

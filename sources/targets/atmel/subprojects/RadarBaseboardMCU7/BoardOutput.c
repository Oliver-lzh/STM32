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

#include "BoardOutput.h"
#include "AppRadarDebugAcquisition.h"

#include <platform/impl/SerialPortImpl.h>
#include <protocol/ProtocolHandler.h>
#include <protocol/RequestHandler.h>
#include <protocol/requests/Requests_Macro.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <udi_cdc.h>

#define BOARD_OUTPUT_PRINTF_BUFFER_SIZE  (160u)
#define BOARD_OUTPUT_COMMAND_BUFFER_SIZE (32u)
#define BOARD_OUTPUT_COMMAND_READ_LIMIT  (8u)

static BoardOutput_Mode_t m_mode = BOARD_OUTPUT_MODE_DEBUG_TEXT;

static IGpio *m_gpio                 = NULL;
static ISpi *m_spi                   = NULL;
static IData *m_data                 = NULL;
static II2c *m_i2c                   = NULL;
static uint8_t *m_macroBuffer        = NULL;
static uint32_t m_macroBufferSize    = 0u;
static bool m_isConstructed          = false;
static bool m_debugInitialized       = false;
static bool m_hostInitialized        = false;
static char m_commandBuffer[BOARD_OUTPUT_COMMAND_BUFFER_SIZE];
static uint8_t m_commandBufferLength = 0u;
static bool m_commandOverflow        = false;
static bool m_debugBootPrinted       = false;

static sr_t BoardOutput_debugSendBuffer(const char *buffer, uint16_t length)
{
    if (!udi_cdc_is_tx_ready() || (udi_cdc_get_free_tx_buffer() < length))
    {
        return E_BUSY;
    }

    if (udi_cdc_write_buf(buffer, length) != 0)
    {
        return E_FAILED;
    }

    udi_cdc_flush();
    return E_SUCCESS;
}

static sr_t BoardOutput_debugSendString(const char *text)
{
    return BoardOutput_debugSendBuffer(text, (uint16_t)strlen(text));
}

static void BoardOutput_debugConstructor(void)
{
    SerialPort_Constructor();
    (void)SerialPort_open(0);
    m_debugInitialized = true;
}

static void BoardOutput_hostConstructor(void)
{
    ProtocolHandler_Constructor();
    RequestHandler_register(m_gpio, m_spi, m_data, m_i2c);
    Requests_Macro_register(m_macroBuffer, m_macroBufferSize);
    m_hostInitialized = true;
}

static void BoardOutput_resetCommandBuffer(void)
{
    m_commandBufferLength = 0u;
    m_commandOverflow     = false;
}

static bool BoardOutput_commandEquals(const char *command)
{
    const uint16_t commandLength = (uint16_t)strlen(command);

    if (m_commandBufferLength != commandLength)
    {
        return false;
    }

    return (memcmp(m_commandBuffer, command, commandLength) == 0);
}

sr_t BoardOutput_setMode(BoardOutput_Mode_t mode)
{
    if ((mode != BOARD_OUTPUT_MODE_HOST_PROTOCOL) && (mode != BOARD_OUTPUT_MODE_DEBUG_TEXT))
    {
        return E_INVALID_PARAMETER;
    }

    BoardOutput_resetCommandBuffer();

    if (!m_isConstructed)
    {
        m_mode = mode;
        return E_SUCCESS;
    }

    if (mode == BOARD_OUTPUT_MODE_HOST_PROTOCOL)
    {
        RETURN_ON_ERROR(AppRadarDebugAcquisition_stop());
        SerialPort_clearInputBuffer();

        if (!m_hostInitialized)
        {
            BoardOutput_hostConstructor();
        }
    }
    else if (!m_debugInitialized)
    {
        BoardOutput_debugConstructor();
    }

    m_mode = mode;
    return E_SUCCESS;
}

BoardOutput_Mode_t BoardOutput_getMode(void)
{
    return m_mode;
}

static void BoardOutput_processCommand(void)
{
    if (m_commandBufferLength == 0u)
    {
        return;
    }

    m_commandBuffer[m_commandBufferLength] = '\0';

    if (strcmp(m_commandBuffer, "/mode host") == 0)
    {
        const sr_t ret = BoardOutput_setMode(BOARD_OUTPUT_MODE_HOST_PROTOCOL);
        if (ret == E_SUCCESS)
        {
            (void)BoardOutput_debugSendString("mode,host\r\n");
        }
        else
        {
            (void)BoardOutput_debugSendString("error,mode-host-failed\r\n");
        }
    }
    else if (strcmp(m_commandBuffer, "/mode debug") == 0)
    {
        (void)BoardOutput_debugSendString("mode,debug\r\n");
    }
    else
    {
        (void)BoardOutput_debugSendString("error,unknown-command\r\n");
    }
}

static void BoardOutput_processCommandIfComplete(void)
{
    if (BoardOutput_commandEquals("/mode host") || BoardOutput_commandEquals("/mode debug"))
    {
        BoardOutput_processCommand();
        BoardOutput_resetCommandBuffer();
    }
}

static void BoardOutput_debugRun(void)
{
    uint8_t bytesRead = 0u;

    if (!m_debugBootPrinted)
    {
        if ((BoardOutput_debugSendString("boot,board=RadarBaseboardMCU7,mode=debug\r\n") == E_SUCCESS) &&
            (BoardOutput_debugSendString("probe,expect,classic-packed12=36864\r\n") == E_SUCCESS) &&
            (BoardOutput_debugSendString("probe,expect,classic-raw16=49152\r\n") == E_SUCCESS) &&
            (BoardOutput_debugSendString("probe,expect,packed12=18432\r\n") == E_SUCCESS) &&
            (BoardOutput_debugSendString("probe,expect,raw16=24576\r\n") == E_SUCCESS) &&
            (BoardOutput_debugSendString("probe,waiting-for-frame\r\n") == E_SUCCESS))
        {
            m_debugBootPrinted = true;
        }
    }

    while ((m_mode == BOARD_OUTPUT_MODE_DEBUG_TEXT) && (bytesRead < BOARD_OUTPUT_COMMAND_READ_LIMIT) && udi_cdc_is_rx_ready())
    {
        const int value = udi_cdc_getc();
        const char c   = (char)value;
        bytesRead++;

        if ((c == '\r') || (c == '\n'))
        {
            if (m_commandOverflow)
            {
                BoardOutput_resetCommandBuffer();
                (void)BoardOutput_debugSendString("error,command-too-long\r\n");
                continue;
            }

            BoardOutput_processCommand();
            BoardOutput_resetCommandBuffer();

            if (m_mode != BOARD_OUTPUT_MODE_DEBUG_TEXT)
            {
                break;
            }
            continue;
        }

        if (m_commandOverflow)
        {
            continue;
        }

        if (m_commandBufferLength >= (BOARD_OUTPUT_COMMAND_BUFFER_SIZE - 1u))
        {
            m_commandOverflow     = true;
            m_commandBufferLength = 0u;
            continue;
        }

        m_commandBuffer[m_commandBufferLength] = c;
        m_commandBufferLength++;

        BoardOutput_processCommandIfComplete();
    }
}

void BoardOutput_Constructor(IGpio *gpio,
                             ISpi *spi,
                             IData *data,
                             II2c *i2c,
                             uint8_t *macroBuffer,
                             uint32_t macroBufferSize)
{
    m_gpio            = gpio;
    m_spi             = spi;
    m_data            = data;
    m_i2c             = i2c;
    m_macroBuffer     = macroBuffer;
    m_macroBufferSize = macroBufferSize;
    m_isConstructed   = true;

    if (m_mode == BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        if (!m_debugInitialized)
        {
            BoardOutput_debugConstructor();
        }
        return;
    }

    if (!m_hostInitialized)
    {
        BoardOutput_hostConstructor();
    }
}

void BoardOutput_run(void)
{
    if (m_mode == BOARD_OUTPUT_MODE_HOST_PROTOCOL)
    {
        ProtocolHandler_run();
    }
    else
    {
        BoardOutput_debugRun();
    }
}

sr_t BoardOutput_onFrame(const uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    if (m_mode == BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        (void)payload;
        (void)count;
        (void)channel;
        (void)timestamp;
        return E_SUCCESS;
    }

    return ProtocolHandler_sendDataFrame(payload, count, channel, timestamp);
}

sr_t BoardOutput_printf(const char *format, ...)
{
    char buffer[BOARD_OUTPUT_PRINTF_BUFFER_SIZE];
    int length;
    va_list args;

    if (format == NULL)
    {
        return E_INVALID_PARAMETER;
    }

    if (m_mode != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        return E_NOT_ALLOWED;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length < 0)
    {
        return E_FAILED;
    }

    if ((uint32_t)length >= sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    return BoardOutput_debugSendBuffer(buffer, (uint16_t)length);
}

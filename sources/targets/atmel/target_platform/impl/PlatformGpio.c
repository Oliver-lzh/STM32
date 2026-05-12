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

#include "PlatformGpio.h"

#include <stddef.h>

/******************************************************************************/
/*Macro Definitions ----------------------------------------------------------*/
/******************************************************************************/
#define PIO_MAX_IDX         (31)
#define IOPORT_PIN_INVALID  (0xFFFFFFFF)
#define IOPORT_MODE_DEFAULT (1 << 8) /*!< None of the other defines will be handled */

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/
ioport_pin_t PlatformGpio_getPortPin(uint16_t id)
{
    const uint8_t port = GPIO_PORT(id);
    const uint8_t pin  = GPIO_PIN(id);

    if (pin <= PIO_MAX_IDX)
    {
        switch (port)
        {
            case 'a':
            case 'A':
                return PIO_PA0_IDX + pin;
            case 'b':
            case 'B':
                return PIO_PB0_IDX + pin;
            case 'c':
            case 'C':
                return PIO_PC0_IDX + pin;
            case 'd':
            case 'D':
                return PIO_PD0_IDX + pin;
            case 'e':
            case 'E':
                return PIO_PE0_IDX + pin;
            default:
                break;
        }
    }

    return IOPORT_PIN_INVALID;
}

/******************************************************************************/
/*Interface Methods Definition -----------------------------------------------*/
/******************************************************************************/


sr_t PlatformGpio_initialize(const PlatformGpioDefinition_t *definition, uint8_t count)
{
    return E_SUCCESS;
}

sr_t PlatformGpio_configurePin(uint16_t id, uint8_t flags)
{
    const bool initialState = flags & GPIO_FLAG_OUTPUT_INITIAL_HIGH;

    switch (id)
    {
        case GPIO_NAME_NONE:
            return E_SUCCESS;
            break;
        default:
        {
            const ioport_pin_t portPin = PlatformGpio_getPortPin(id);
            if (portPin == IOPORT_PIN_INVALID)
            {
                return E_INVALID_PARAMETER;
            }

            enum ioport_direction dir;
            ioport_mode_t mode;

            /* getting the selected mode (ignore initial output value) */
            switch (flags & ~((uint8_t)GPIO_FLAG_OUTPUT_INITIAL_HIGH))
            {
                case GPIO_MODE_INPUT:
                    dir  = IOPORT_DIR_INPUT;
                    mode = IOPORT_MODE_DEFAULT;
                    break;
                case GPIO_MODE_INPUT_PULL_DOWN:
                    dir  = IOPORT_DIR_INPUT;
                    mode = IOPORT_MODE_PULLDOWN;
                    break;
                case GPIO_MODE_INPUT_PULL_UP:
                    dir  = IOPORT_DIR_INPUT;
                    mode = IOPORT_MODE_PULLUP;
                    break;
                case GPIO_MODE_OUTPUT_OPEN_DRAIN:
                    dir  = IOPORT_DIR_OUTPUT;
                    mode = IOPORT_MODE_OPEN_DRAIN;
                    break;
                case GPIO_MODE_OUTPUT_PUSH_PULL:
                    dir  = IOPORT_DIR_OUTPUT;
                    mode = IOPORT_MODE_DEFAULT;
                    break;
                default:
                    return E_INVALID_PARAMETER;
            }

            /* if configuring as input, the direction needs to be changed
               first before other settings, for output it is the opposite */
            if (dir == IOPORT_DIR_INPUT)
            {
                ioport_set_pin_dir(portPin, IOPORT_DIR_INPUT);
            }
            ioport_set_pin_mode(portPin, mode);
            ioport_set_pin_level(portPin, initialState);
            if (dir == IOPORT_DIR_OUTPUT)
            {
                ioport_set_pin_dir(portPin, IOPORT_DIR_OUTPUT);
            }
            ioport_enable_pin(portPin);
        }
        break;
    }
    return E_SUCCESS;
}

sr_t PlatformGpio_setPin(uint16_t id, bool state)
{
    switch (id)
    {
        case GPIO_NAME_NONE:
            return E_SUCCESS;
            break;
        default:
        {
            const ioport_pin_t portPin = PlatformGpio_getPortPin(id);
            if (portPin == IOPORT_PIN_INVALID)
            {
                return E_INVALID_PARAMETER;
            }

            ioport_set_pin_level(portPin, state);
        }
        break;
    }

    return E_SUCCESS;
}

sr_t PlatformGpio_getPin(uint16_t id, bool *state)
{
    switch (id)
    {
        case GPIO_NAME_NONE:
            *state = false;
            break;
        default:
        {
            const ioport_pin_t portPin = PlatformGpio_getPortPin(id);
            if (portPin == IOPORT_PIN_INVALID)
            {
                return E_INVALID_PARAMETER;
            }
            *state = ioport_get_pin_level(portPin);
        }
        break;
    }
    return E_SUCCESS;
}

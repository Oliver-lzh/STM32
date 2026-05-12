/**
 * \file 	Commands_IPinsSmartar.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IPinsSmartar               IPinsSmartar Commands
 * \brief           Radar Sensor pins interface Commands.
 *
 * @{
 */
#include "Commands_IPinsSmartar.h"
#include <common/errors.h>
#include <common/serialization.h>
#include <universal/components/subinterfaces/ipins.h>
#include <universal/protocol/protocol_definitions.h>


static uint8_t Commands_IPinsSmartar_setResetPin(IPinsSmartar *pinsSmartar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != sizeof(uint8_t))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    if (payload[0] > 1)  // bool
    {
        return STATUS_COMMAND_PAYLOAD_INVALID;
    }

    const bool state = (payload[0] != 0);
    return pinsSmartar->setResetPin(pinsSmartar, state);
}

static uint8_t Commands_IPinsSmartar_reset(IPinsSmartar *pinsSmartar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != 0)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    return pinsSmartar->reset(pinsSmartar);
}

uint8_t Commands_IPinsSmartar_read(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    if (pinsSmartar == NULL)
    {
        return STATUS_COMMAND_SUBIF_INVALID;
    }

    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IPinsSmartar_write(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    if (pinsSmartar == NULL)
    {
        return STATUS_COMMAND_SUBIF_INVALID;
    }

    switch (bFunction)
    {
        case FN_PINS_SET_RESET_PIN:
            return Commands_IPinsSmartar_setResetPin(pinsSmartar, wLength, payload);
            break;
        case FN_PINS_RESET:
            return Commands_IPinsSmartar_reset(pinsSmartar, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IPinsSmartar_transfer(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    if (pinsSmartar == NULL)
    {
        return STATUS_COMMAND_SUBIF_INVALID;
    }

    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

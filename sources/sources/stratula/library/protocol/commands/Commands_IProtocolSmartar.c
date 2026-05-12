/**
 * \file 	Commands_IProtocolSmartar.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IProtocolSmartar IProtocolSmartar Commands
 * \brief           Radar Command interface Commands.
 *
 * @{
 */
#include "Commands_IProtocolSmartar.h"
#include <common/errors.h>
#include <common/serialization.h>
#include <stddef.h>
#include <universal/components/subinterfaces/iprotocol.h>
#include <universal/protocol/protocol_definitions.h>


static uint8_t Commands_IProtocolSmartar_executeRead(IProtocolSmartar *protocol, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    uint16_t count;
    const uint16_t(*command)[2];
    if (wLengthIn != sizeof(count) + sizeof(*command))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    count            = serialToHost16(payloadIn);
    command          = (const uint16_t(*)[2])(uintptr_t)(payloadIn + sizeof(count));
    uint32_t *values = (uint32_t *)(uintptr_t)(*payloadOut);
    if (*wLengthOut < count * sizeof(*values))
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }
    *wLengthOut = count * sizeof(*values);

    return protocol->executeRead(protocol, *command, count, values);
}

static uint8_t Commands_IProtocolSmartar_executeWrite(IProtocolSmartar *protocol, uint16_t wLength, const uint8_t *payload)
{
    const uint16_t(*command)[2];
    const uint16_t(*values)[2] = (const uint16_t(*)[2])(uintptr_t)(payload);
    const uint16_t length      = wLength - sizeof(*command);
    command                    = (const uint16_t(*)[2])(uintptr_t)(payload + length);
    if ((wLength < sizeof(*command) + sizeof(*values)) || (length % sizeof(*values)))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const uint16_t count = length / sizeof(*values);

    return protocol->executeWrite(protocol, *command, count, values);
}

static uint8_t Commands_IProtocolSmartar_executeWriteBatch(IProtocolSmartar *protocol, uint16_t wLength, const uint8_t *payload)
{
    const uint16_t(*commands)[2][2] = (const uint16_t(*)[2][2])(uintptr_t)(payload);
    if (wLength % sizeof(*commands))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const uint16_t count = wLength / sizeof(*commands);
    return protocol->executeWriteBatch(protocol, commands, count);
}

static uint8_t Commands_IProtocolSmartar_setBits(IProtocolSmartar *protocol, uint16_t wLength, const uint8_t *payload)
{
    uint16_t address;
    uint32_t bitMask;
    if (wLength != sizeof(address) + sizeof(bitMask))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    address = serialToHost16(payload);
    bitMask = serialToHost32(payload + sizeof(address));

    return protocol->setBits(protocol, address, bitMask);
}

uint8_t Commands_IProtocolSmartar_read(IProtocolSmartar *protocol, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IProtocolSmartar_write(IProtocolSmartar *protocol, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    switch (bFunction)
    {
        case FN_PROTOCOL_EXECUTE:
            return Commands_IProtocolSmartar_executeWrite(protocol, wLength, payload);
            break;
        case FN_PROTOCOL_SET_BITS:
            return Commands_IProtocolSmartar_setBits(protocol, wLength, payload);
            break;
        case FN_PROTOCOL_EXECUTE_HELPER:
            return Commands_IProtocolSmartar_executeWriteBatch(protocol, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IProtocolSmartar_transfer(IProtocolSmartar *protocol, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    switch (bFunction)
    {
        case FN_PROTOCOL_EXECUTE:
            return Commands_IProtocolSmartar_executeRead(protocol, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

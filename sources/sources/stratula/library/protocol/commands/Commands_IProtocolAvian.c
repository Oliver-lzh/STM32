/**
 * \file 	Commands_IProtocolAvian.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IProtocolAvian IProtocolAvian Commands
 * \brief           Radar Command interface Commands.
 *
 * @{
 */
#include "Commands_IProtocolAvian.h"
#include <common/errors.h>
#include <common/serialization.h>
#include <stddef.h>
#include <universal/components/subinterfaces/iprotocol.h>
#include <universal/protocol/protocol_definitions.h>


static uint8_t Commands_IProtocolAvian_execute(IProtocolAvian *protocol, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    const uint8_t(*commands)[4] = (const uint8_t(*)[4])(uintptr_t)payloadIn;
    if (wLengthIn % sizeof(*commands))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }
    if (*wLengthOut < wLengthIn)
    {
        return STATUS_PAYLOAD_TOO_LONG;
    }

    const uint8_t count = wLengthIn / sizeof(*commands);
    uint32_t *results   = (uint32_t *)(uintptr_t)(*payloadOut);
    *wLengthOut         = wLengthIn;

    return protocol->execute(protocol, commands, count, results);
}

static uint8_t Commands_IProtocolAvian_executeWrite(IProtocolAvian *protocol, uint16_t wLength, const uint8_t *payload)
{
    const uint8_t(*commands)[4] = (const uint8_t(*)[4])(uintptr_t)payload;
    if (wLength % sizeof(commands))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const uint8_t count = wLength / sizeof(commands);

    return protocol->execute(protocol, commands, count, NULL);
}

static uint8_t Commands_IProtocolAvian_setBits(IProtocolAvian *protocol, uint16_t wLength, const uint8_t *payload)
{
    uint32_t command;
    if (wLength != sizeof(command))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    command                = serialToHost32(payload);
    const uint8_t address  = command >> 24;
    const uint32_t bitMask = command & 0xFFFFFF;

    return protocol->setBits(protocol, address, bitMask);
}

uint8_t Commands_IProtocolAvian_read(IProtocolAvian *protocol, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IProtocolAvian_write(IProtocolAvian *protocol, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    switch (bFunction)
    {
        case FN_PROTOCOL_EXECUTE:
            return Commands_IProtocolAvian_executeWrite(protocol, wLength, payload);
            break;
        case FN_PROTOCOL_SET_BITS:
            return Commands_IProtocolAvian_setBits(protocol, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t Commands_IProtocolAvian_transfer(IProtocolAvian *protocol, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    switch (bFunction)
    {
        case FN_PROTOCOL_EXECUTE:
            return Commands_IProtocolAvian_execute(protocol, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

#include "CommandHandler.h"
#include <common/serialization.h>
#include <universal/protocol/protocol_definitions.h>

#include <stddef.h>


static uint8_t listRegistrations(CommandHandler *self, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    const uint16_t requiredLength = self->m_registrationCount * sizeof(((ICommands *)0)->m_type);
    if (*wLengthOut < requiredLength)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }
    for (uint8_t i = 0; i < self->m_registrationCount; i++)
    {
        ICommands *commands = self->m_registrations[i];
        *payloadOut         = hostToSerial(*payloadOut, commands->m_type);
    }

    *wLengthOut = requiredLength;
    return STATUS_SUCCESS;
}

static ICommands *findRegistration(CommandHandler *self, uint16_t type)
{
    for (uint8_t i = 0; i < self->m_registrationCount; i++)
    {
        ICommands *commands = self->m_registrations[i];
        if (commands->m_type == type)
        {
            return commands;
        }
    }
    return NULL;
}

static uint8_t getCount(CommandHandler *self, uint16_t wType, uint8_t **payload)
{
    ICommands *commands = findRegistration(self, wType);
    if (commands == NULL)
    {
        return STATUS_COMMAND_TYPE_INVALID;
    }

    (*payload)[0] = commands->m_count;
    return STATUS_SUCCESS;
}

bool CommandHandler_registerImplementation(CommandHandler *self, ICommands *commands)
{
    if (self->m_registrationCount >= MAX_COMMAND_REGISTRATIONS)
    {
        return false;
    }
    self->m_registrations[self->m_registrationCount++] = commands;
    return true;
}

uint8_t CommandHandler_write(CommandHandler *self, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    const uint16_t wType = CMD_GET_TYPE(wValue);
    ICommands *commands  = findRegistration(self, wType);
    if (commands == NULL)
    {
        return STATUS_COMMAND_TYPE_INVALID;
    }

    const uint8_t bId           = CMD_GET_ID(wIndex);
    const uint8_t bSubinterface = CMD_GET_SUBIF(wIndex);
    const uint8_t bFunction     = CMD_GET_FUNCTION(wIndex);
    return commands->write(bId, bSubinterface, bFunction, wLength, payload);
}

uint8_t CommandHandler_read(CommandHandler *self, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    const uint16_t wType = CMD_GET_TYPE(wValue);
    if (!wType)
    {
        return getCount(self, CMD_GET_TYPE(wIndex), payload);
    }

    ICommands *commands = findRegistration(self, wType);
    if (commands == NULL)
    {
        return STATUS_COMMAND_TYPE_INVALID;
    }

    const uint8_t bId           = CMD_GET_ID(wIndex);
    const uint8_t bSubinterface = CMD_GET_SUBIF(wIndex);
    const uint8_t bFunction     = CMD_GET_FUNCTION(wIndex);
    return commands->read(bId, bSubinterface, bFunction, wLength, payload);
}

uint8_t CommandHandler_transfer(CommandHandler *self, uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    const uint16_t wType = CMD_GET_TYPE(wValue);
    if (!wType)
    {
        return listRegistrations(self, wLengthOut, payloadOut);
    }

    ICommands *commands = findRegistration(self, wType);
    if (commands == NULL)
    {
        return STATUS_COMMAND_TYPE_INVALID;
    }

    const uint8_t bId           = CMD_GET_ID(wIndex);
    const uint8_t bSubinterface = CMD_GET_SUBIF(wIndex);
    const uint8_t bFunction     = CMD_GET_FUNCTION(wIndex);
    return commands->transfer(bId, bSubinterface, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
}

void CommandHandler_Constructor(CommandHandler *self)
{
    self->m_registrationCount = 0;
}

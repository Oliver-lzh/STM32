/**
 * \file 	Commands_IRadarSmartar.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IRadarSmartar               IRadarSmartar Commands
 * \brief           Radar Chipset interface Commands.
 *
 * @{
 */
#include "Commands_IRadarSmartar.h"
#include "Commands_IPinsSmartar.h"
#include "Commands_IProtocolSmartar.h"
#include "Commands_IRegisters16_32.h"
#include <common/serialization.h>
#include <common/type_serialization.h>
#include <protocol/RequestHandler.h>
#include <universal/components/radar.h>
#include <universal/components/radar/iradarsmartar.h>
#include <universal/components/subinterfaces.h>
#include <universal/protocol/protocol_definitions.h>


#define TYPE COMPONENT_TYPE_RADAR_SMARTAR


static IRadarSmartar *m_instances[MAX_INSTANCE_REGISTRATIONS];  // instance registrations
static bool m_registered = false;

static uint8_t _read(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
static uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
static uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


static ICommands _Commands = {
    .m_type   = TYPE,
    .m_count  = 0,  // number of currently registered instances
    .read     = _read,
    .write    = _write,
    .transfer = _transfer,
};

#define m_instanceCount _Commands.m_count


bool Commands_IRadarSmartar_register(IRadarSmartar *instance)
{
    if (!m_registered)
    {
        if (!RequestHandler_registerComponentImplementation(&_Commands))
        {
            return false;
        }
        m_registered = true;
    }

    if (m_instanceCount < MAX_INSTANCE_REGISTRATIONS)
    {
        m_instances[m_instanceCount++] = instance;
        return true;
    }

    return false;
}

static inline IRadarSmartar *getInstance(uint8_t bId)
{
    if (bId < m_instanceCount)
    {
        return m_instances[bId];
    }

    return NULL;
}

static uint8_t Commands_IRadarSmartar_reset(IRadarSmartar *radar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != sizeof(uint8_t))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    if (payload[0] > 1)
    {
        return STATUS_REQUEST_WINDEX_INVALID;
    }
    const bool softReset = payload[0];
    return radar->reset(radar, softReset);
}


static uint8_t Commands_IRadarSmartar_getDataIndex(IRadarSmartar *radar, uint16_t wLength, uint8_t **payload)
{
    uint8_t dataIndex;
    if (wLength != sizeof(dataIndex))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    RETURN_ON_ERROR(radar->getDataIndex(radar, &dataIndex));
    *payload[0] = dataIndex;

    return E_SUCCESS;
}

uint8_t _read(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    IRadarSmartar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        case COMPONENT_SUBIF_REGISTERS:
        {
            IRegisters16_32 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_32_read(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsSmartar *pins = instance->getIPinsSmartar(instance);
            return Commands_IPinsSmartar_read(pins, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolSmartar *protocol = instance->getIProtocolSmartar(instance);
            return Commands_IProtocolSmartar_read(protocol, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_SMARTAR_GET_DATA_INDEX:
            return Commands_IRadarSmartar_getDataIndex(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    IRadarSmartar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        case COMPONENT_SUBIF_REGISTERS:
        {
            IRegisters16_32 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_32_write(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsSmartar *pins = instance->getIPinsSmartar(instance);
            return Commands_IPinsSmartar_write(pins, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolSmartar *protocol = instance->getIProtocolSmartar(instance);
            return Commands_IProtocolSmartar_write(protocol, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_SMARTAR_RESET:
            return Commands_IRadarSmartar_reset(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    IRadarSmartar *instance = getInstance(bId);
    if (instance == NULL)
    {
        return STATUS_COMMAND_ID_INVALID;
    }

    switch (bSubinterface)
    {
        case COMPONENT_SUBIF_DEFAULT:
            //execute default functions below
            break;
        case COMPONENT_SUBIF_REGISTERS:
        {
            IRegisters16_32 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_32_transfer(registers, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsSmartar *pins = instance->getIPinsSmartar(instance);
            return Commands_IPinsSmartar_transfer(pins, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolSmartar *protocol = instance->getIProtocolSmartar(instance);
            return Commands_IProtocolSmartar_transfer(protocol, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

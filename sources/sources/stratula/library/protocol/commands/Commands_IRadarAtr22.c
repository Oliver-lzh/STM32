/**
 * \file 	Commands_IRadarAtr22.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IRadarAtr22               IRadarAtr22 Commands
 * \brief           Radar Chipset interface Commands.
 *
 * @{
 */
#include "Commands_IRadarAtr22.h"
#include "Commands_IProtocolAtr22.h"
#include "Commands_IRegisters16.h"
#include <common/serialization.h>
#include <common/type_serialization.h>
#include <protocol/RequestHandler.h>
#include <universal/components/radar.h>
#include <universal/components/radar/iradaratr22.h>
#include <universal/components/subinterfaces.h>
#include <universal/protocol/protocol_definitions.h>


#define TYPE COMPONENT_TYPE_RADAR_ATR22


static IRadarAtr22 *m_instances[MAX_INSTANCE_REGISTRATIONS];  // instance registrations
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


bool Commands_IRadarAtr22_register(IRadarAtr22 *instance)
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

static inline IRadarAtr22 *getInstance(uint8_t bId)
{
    if (bId < m_instanceCount)
    {
        return m_instances[bId];
    }

    return NULL;
}


static uint8_t Commands_IRadarAtr22_initialize(IRadarAtr22 *radar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != 0)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    return radar->initialize(radar);
}

static uint8_t Commands_IRadarAtr22_reset(IRadarAtr22 *radar, uint16_t wLength, const uint8_t *payload)
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

static uint8_t Commands_IRadarAtr22_getDataIndex(IRadarAtr22 *radar, uint16_t wLength, uint8_t **payload)
{
    uint8_t dataIndex;
    if (wLength != sizeof(dataIndex))
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    const sr_t ret = radar->getDataIndex(radar, &dataIndex);
    *payload[0]    = dataIndex;

    return ret;
}

uint8_t _read(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, uint8_t **payload)
{
    IRadarAtr22 *instance = getInstance(bId);
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
            IRegisters16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_read(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            return STATUS_COMMAND_SUBIF_INVALID;
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolAtr22 *commands = instance->getIProtocolAtr22(instance);
            return Commands_IProtocolAtr22_read(commands, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_ATR22_GET_DATA_INDEX:
            return Commands_IRadarAtr22_getDataIndex(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    IRadarAtr22 *instance = getInstance(bId);
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
            IRegisters16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_write(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            return STATUS_COMMAND_SUBIF_INVALID;
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolAtr22 *commands = instance->getIProtocolAtr22(instance);
            return Commands_IProtocolAtr22_write(commands, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_ATR22_INITIALIZE:
            return Commands_IRadarAtr22_initialize(instance, wLength, payload);
            break;
        case FN_RADAR_ATR22_RESET:
            return Commands_IRadarAtr22_reset(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    IRadarAtr22 *instance = getInstance(bId);
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
            IRegisters16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters16_transfer(registers, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        case COMPONENT_SUBIF_PINS:
        {
            return STATUS_COMMAND_SUBIF_INVALID;
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolAtr22 *commands = instance->getIProtocolAtr22(instance);
            return Commands_IProtocolAtr22_transfer(commands, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // Atr22 functions
    switch (bFunction)
    {
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

/*  @} */

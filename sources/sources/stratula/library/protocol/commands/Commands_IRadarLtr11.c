/**
 * \file 	Commands_IRadarLtr11.c
 *
 * \addtogroup      Command_Interface   Command Interface
 *
 * \defgroup        Commands_IRadarLtr11               IRadarLtr11 Commands
 * \brief           Radar Chipset interface Commands.
 *
 * @{
 */
#include "Commands_IRadarLtr11.h"
#include "Commands_IPinsLtr11.h"
#include "Commands_IProtocolLtr11.h"
#include "Commands_IRegisters8_16.h"
#include <common/serialization.h>
#include <common/type_serialization.h>
#include <protocol/RequestHandler.h>
#include <universal/components/radar.h>
#include <universal/components/radar/iradarltr11.h>
#include <universal/components/subinterfaces.h>
#include <universal/protocol/protocol_definitions.h>


#define TYPE COMPONENT_TYPE_RADAR_LTR11


static IRadarLtr11 *m_instances[MAX_INSTANCE_REGISTRATIONS];  // instance registrations
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


bool Commands_IRadarLtr11_register(IRadarLtr11 *instance)
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

static inline IRadarLtr11 *getInstance(uint8_t bId)
{
    if (bId < m_instanceCount)
    {
        return m_instances[bId];
    }

    return NULL;
}


static uint8_t Commands_IRadarLtr11_initialize(IRadarLtr11 *radar, uint16_t wLength, const uint8_t *payload)
{
    if (wLength != 0)
    {
        return STATUS_COMMAND_WLENGTH_INVALID;
    }

    return radar->initialize(radar);
}

static uint8_t Commands_IRadarLtr11_reset(IRadarLtr11 *radar, uint16_t wLength, const uint8_t *payload)
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

static uint8_t Commands_IRadarLtr11_getDataIndex(IRadarLtr11 *radar, uint16_t wLength, uint8_t **payload)
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
    IRadarLtr11 *instance = getInstance(bId);
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
            IRegisters8_16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters8_16_read(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsLtr11 *pins = instance->getIPinsLtr11(instance);
            return Commands_IPinsLtr11_read(pins, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolLtr11 *protocol = instance->getIProtocolLtr11(instance);
            return Commands_IProtocolLtr11_read(protocol, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_LTR11_GET_DATA_INDEX:
            return Commands_IRadarLtr11_getDataIndex(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _write(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLength, const uint8_t *payload)
{
    IRadarLtr11 *instance = getInstance(bId);
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
            IRegisters8_16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters8_16_write(registers, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsLtr11 *pins = instance->getIPinsLtr11(instance);
            return Commands_IPinsLtr11_write(pins, bFunction, wLength, payload);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolLtr11 *protocol = instance->getIProtocolLtr11(instance);
            return Commands_IProtocolLtr11_write(protocol, bFunction, wLength, payload);
        }
        default:
            return STATUS_COMMAND_SUBIF_INVALID;
    }

    // default functions
    switch (bFunction)
    {
        case FN_RADAR_LTR11_INITIALIZE:
            return Commands_IRadarLtr11_initialize(instance, wLength, payload);
            break;
        case FN_RADAR_LTR11_RESET:
            return Commands_IRadarLtr11_reset(instance, wLength, payload);
            break;
        default:
            return STATUS_COMMAND_FUNCTION_INVALID;
            break;
    }
}

uint8_t _transfer(uint8_t bId, uint8_t bSubinterface, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    IRadarLtr11 *instance = getInstance(bId);
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
            IRegisters8_16 *registers = instance->getIRegisters(instance);
            return Commands_IRegisters8_16_transfer(registers, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        case COMPONENT_SUBIF_PINS:
        {
            IPinsLtr11 *pins = instance->getIPinsLtr11(instance);
            return Commands_IPinsLtr11_transfer(pins, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
        }
        case COMPONENT_SUBIF_PROTOCOL:
        {
            IProtocolLtr11 *protocol = instance->getIProtocolLtr11(instance);
            return Commands_IProtocolLtr11_transfer(protocol, bFunction, wLengthIn, payloadIn, wLengthOut, payloadOut);
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

#include "Ltr11.h"
#include <impl/thread.h>
#include <stddef.h>


#define SELF        ((Ltr11 *)(uintptr_t)self)
#define REGISTERS   ((IRegisters8_16 *)&SELF->m_registers)
#define PINS_LTR11  ((IPinsLtr11 *)&SELF->m_pins)
#define PROTOCOL    ((IProtocolLtr11 *)&SELF->m_protocol)
#define RADAR_LTR11 (&SELF->b_IRadarLtr11)


// Register definitions
#define MAIN     0xF
#define SW_RESET (1u << 15)
#define CHIP_ID  0x38  // STS_CHIP_VER_REG56_REG


IPinsLtr11 *Ltr11_getIPinsLtr11(IRadarLtr11 *self)
{
    return PINS_LTR11;
}

IRegisters8_16 *Ltr11_getIRegisters(IRadarLtr11 *self)
{
    return REGISTERS;
}

IProtocolLtr11 *Ltr11_getIProtocolLtr11(IRadarLtr11 *self)
{
    return PROTOCOL;
}

sr_t Ltr11_reset(IRadarLtr11 *self, bool softReset)
{
    if (softReset)
    {
        const uint8_t command[3] = LTR11_WRITE(MAIN, SW_RESET);
        RETURN_ON_ERROR(PROTOCOL->executeWrite(PROTOCOL, command));
        /* Mandatory after the soft reset, because the protocol access
         * might have been called while miso arbitration was active.
         */
        PROTOCOL->setMisoArbitration(PROTOCOL, 0);
        this_thread_sleep_for(chrono_microseconds(1200));  //sleep for 1.2 ms , specific to ltr11 (1.17ms measured with logic analyzer)
    }
    else
    {
        /* Mandatory before the hard reset, since we might wait
         * for an edge that never comes after a reset.
         */
        PROTOCOL->setMisoArbitration(PROTOCOL, 0);
        RETURN_ON_ERROR(PINS_LTR11->reset(PINS_LTR11));
    }

    SELF->m_initialized = false;
    return E_SUCCESS;
}

sr_t Ltr11_getDataIndex(IRadarLtr11 *self, uint8_t *index)
{
    *index = SELF->m_dataIndex;
    return E_SUCCESS;
}

sr_t Ltr11_initialize(IRadarLtr11 *self)
{
    if (SELF->m_initialized)
    {
        return E_SUCCESS;
    }

    SELF->m_initialized = true;

    return E_SUCCESS;
}

void Ltr11_Constructor(Ltr11 *self, IData *accessData, IGpio *accessGpio, ISpi *accessSpi, const BoardRadarDefinition_t *boardDefinition, const IPinsLtr11Definition_t *pinsDefinition, const PlatformInterruptDefinition_t *irqConfig)
{
    PinsLtr11_Constructor(&self->m_pins, accessGpio, pinsDefinition);
    ProtocolLtr11_Constructor(&self->m_protocol, accessSpi, boardDefinition->devId, irqConfig);
    RegistersLtr11_Constructor(&self->m_registers, PROTOCOL);

    // IRadarLtr11
    self->b_IRadarLtr11.reset        = Ltr11_reset;
    self->b_IRadarLtr11.getDataIndex = Ltr11_getDataIndex;
    self->b_IRadarLtr11.initialize   = Ltr11_initialize;

    self->b_IRadarLtr11.getIRegisters     = Ltr11_getIRegisters;
    self->b_IRadarLtr11.getIPinsLtr11     = Ltr11_getIPinsLtr11;
    self->b_IRadarLtr11.getIProtocolLtr11 = Ltr11_getIProtocolLtr11;


    self->m_accessData  = accessData;
    self->m_dataIndex   = boardDefinition->dataIndex;
    self->m_initialized = false;

    // perform reset because device is at undefined state after power-up
    const bool softReset = false;
    Ltr11_reset(RADAR_LTR11, softReset);
}

sr_t Ltr11_Detect(ISpi *accessSpi, const BoardRadarDefinition_t *boardDefinition)
{
    ProtocolLtr11 protocol;
    ProtocolLtr11_Constructor(&protocol, accessSpi, boardDefinition->devId, NULL);

    // check the presence of a supported device
    uint16_t chipId;
    RETURN_ON_ERROR(ProtocolLtr11_executeRead(&protocol.b_IProtocolLtr11, LTR11_READ(CHIP_ID), &chipId));

    const uint16_t chipIdMask = 0x0007;
    switch (chipId & chipIdMask)
    {
        case 0x0003:  // Ltr11 (b11)
            break;
        default:
            return E_NOT_SUPPORTED;
            break;
    }

    return E_SUCCESS;
}

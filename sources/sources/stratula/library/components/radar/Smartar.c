#include "Smartar.h"
#include <common/typeutils.h>
#include <impl/thread.h>
#include <stddef.h>


#define SELF          ((Smartar *)(uintptr_t)self)
#define REGISTERS     ((IRegisters16_32 *)&SELF->m_registers)
#define PINS_SMARTAR  ((IPinsSmartar *)&SELF->m_pins)
#define PROTOCOL      ((IProtocolSmartar *)&SELF->m_protocol)
#define RADAR_SMARTAR (&SELF->b_IRadarSmartar)

// Register Definitions
#define CGU_RST 0x0208
#define ID_HW   0x0E08
// Check TECH and DIG register fields
#define ID_HW_RESET_VALUE 0x88000
#define ID_HW_MASK        0xCFF00
#define SW_RESET          (1u << 1)


static inline sr_t _fpgaStartup(IProtocolSmartar *self)
{
    const uint16_t CLK_FD         = 0x0020;
    const uint16_t CLK_FD_TEST    = 0x0024;
    const uint16_t CLK_RCOSC_TRIM = 0x0004;
    const uint16_t CLK_XOSC_CTRL0 = 0x0010;
    const uint16_t CLK_RCOSC_CTRL = 0x0000;
    const uint16_t CLK_WUC        = 0x0030;
    const uint16_t sysClkDelayMs  = 1;  // Wait for system clock to start

    const uint16_t commands[][2][2] = {
        {SMARTAR_WRITE(CLK_FD), SMARTAR_WRITE_VALUE(0x7)},
        {SMARTAR_WRITE(CLK_FD_TEST), SMARTAR_WRITE_VALUE(0)},
        {SMARTAR_WRITE(CLK_RCOSC_TRIM), SMARTAR_WRITE_VALUE(0)},
        {SMARTAR_WRITE(CLK_RCOSC_TRIM), SMARTAR_WRITE_VALUE(0x00000160)},
        {SMARTAR_WRITE(CLK_RCOSC_CTRL), SMARTAR_WRITE_VALUE(0x00000052)},
        {SMARTAR_WRITE(CLK_XOSC_CTRL0), SMARTAR_WRITE_VALUE(0x2)},
        {SMARTAR_WRITE(CLK_WUC), SMARTAR_WRITE_VALUE(0x00044449)},
    };

    RETURN_ON_ERROR(ProtocolSmartar_executeWriteBatch(self, commands, ARRAY_SIZE(commands)));
    this_thread_sleep_for(chrono_milliseconds(sysClkDelayMs));

    return E_SUCCESS;
}

IRegisters16_32 *Smartar_getIRegisters(IRadarSmartar *self)
{
    return REGISTERS;
}

IPinsSmartar *Smartar_getIPinsSmartar(IRadarSmartar *self)
{
    return PINS_SMARTAR;
}

IProtocolSmartar *Smartar_getIProtocolSmartar(IRadarSmartar *self)
{
    return PROTOCOL;
}

/****************************************************************************
 * Public methods
 ****************************************************************************/
sr_t Smartar_reset(IRadarSmartar *self, bool softReset)
{
    if (softReset)
    {
        RETURN_ON_ERROR(PROTOCOL->setBits(PROTOCOL, CGU_RST, SW_RESET));
    }
    else
    {
        /* Reset of the complete digital logic, the XOSC, the clock registers, and the test registers.
         * Hence, the needed startup routine after HW reset.
         */
        RETURN_ON_ERROR(PINS_SMARTAR->reset(PINS_SMARTAR));
        RETURN_ON_ERROR(_fpgaStartup(PROTOCOL));
    }

    return E_SUCCESS;
}

sr_t Smartar_getDataIndex(IRadarSmartar *self, uint8_t *index)
{
    *index = SELF->m_dataIndex;
    return E_SUCCESS;
}

sr_t Smartar_initialize(IRadarSmartar *self)
{
    if (SELF->m_initialized)
    {
        return E_SUCCESS;
    }

    SELF->m_initialized = true;

    return E_SUCCESS;
}

void Smartar_Constructor(Smartar *self, ISpi *accessSpi, IGpio *accessGpio, const IPinsSmartarDefinition_t *pinsDefinition, const BoardRadarDefinition_t *boardDefinition)
{
    PinsSmartar_Constructor(&self->m_pins, accessGpio, pinsDefinition);
    ProtocolSmartar_Constructor(&self->m_protocol, accessSpi, boardDefinition->devId);
    RegistersSmartar_Constructor(&self->m_registers, PROTOCOL);

    // IRadarAtr2 (low level interface)
    self->b_IRadarSmartar.reset               = Smartar_reset;
    self->b_IRadarSmartar.getDataIndex        = Smartar_getDataIndex;
    self->b_IRadarSmartar.initialize          = Smartar_initialize;
    self->b_IRadarSmartar.getIRegisters       = Smartar_getIRegisters;
    self->b_IRadarSmartar.getIProtocolSmartar = Smartar_getIProtocolSmartar;
    self->b_IRadarSmartar.getIPinsSmartar     = Smartar_getIPinsSmartar;

    self->m_initialized = false;
}

sr_t Smartar_Detect(ISpi *accessSpi, IGpio *accessGpio, const BoardRadarDefinition_t *boardDefinition, const IPinsSmartarDefinition_t *pinsDefinition)
{
    PinsSmartar pins;
    PinsSmartar_Constructor(&pins, accessGpio, pinsDefinition);
    RETURN_ON_ERROR(PinsSmartar_reset(&pins.b_IPinsSmartar));

    ProtocolSmartar protocol;
    ProtocolSmartar_Constructor(&protocol, accessSpi, boardDefinition->devId);
    RETURN_ON_ERROR(_fpgaStartup(&protocol.b_IProtocolSmartar));

    // Check the presence of supported hw
    uint32_t id;
    const uint16_t readCommand[2] = SMARTAR_READ(ID_HW);
    RETURN_ON_ERROR(ProtocolSmartar_executeRead(&protocol.b_IProtocolSmartar, readCommand, 1, &id));
    if ((id & ID_HW_MASK) == ID_HW_RESET_VALUE)
    {
        return E_SUCCESS;
    }

    return E_NOT_SUPPORTED;
}

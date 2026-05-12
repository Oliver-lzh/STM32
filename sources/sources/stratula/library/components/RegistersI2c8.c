#include "RegistersI2c8.h"


#define SELF ((RegistersI2c8 *)(uintptr_t)self)


static sr_t RegistersI2c8_read(IRegisters8 *self, uint8_t regAddr, uint8_t *value)
{
    return SELF->m_accessI2c->readWith8BitPrefix(SELF->m_devAddr, regAddr, 1, value);
}

static sr_t RegistersI2c8_write(IRegisters8 *self, uint8_t regAddr, uint8_t value)
{
    return SELF->m_accessI2c->writeWith8BitPrefix(SELF->m_devAddr, regAddr, 1, &value);
}

static sr_t RegistersI2c8_readBurst(IRegistersType *self, uint8_t regAddr, uint8_t count, uint8_t values[])
{
    return SELF->m_accessI2c->readWith8BitPrefix(SELF->m_devAddr, regAddr, count, values);
}

static sr_t RegistersI2c8_writeBurst(IRegistersType *self, uint8_t regAddr, uint8_t count, const uint8_t values[])
{
    return SELF->m_accessI2c->writeWith8BitPrefix(SELF->m_devAddr, regAddr, count, values);
}

void RegistersI2c8_Constructor(RegistersI2c8 *self, II2c *accessI2c, uint16_t devAddr)
{
    Registers8_Constructor(&self->b_Registers, 1);

    self->b_Registers.b_IRegisters.read  = RegistersI2c8_read;
    self->b_Registers.b_IRegisters.write = RegistersI2c8_write;

    self->b_Registers.b_IRegisters.readBurst  = RegistersI2c8_readBurst;
    self->b_Registers.b_IRegisters.writeBurst = RegistersI2c8_writeBurst;

    {
        self->m_accessI2c = accessI2c;
        self->m_devAddr   = devAddr;
    }
}

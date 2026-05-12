#include "RegistersSmartar.h"


#define SELF     ((RegistersSmartar *)(uintptr_t)self)
#define PROTOCOL (SELF->m_protocol)


static sr_t RegistersSmartar_readBurst(IRegisters16_32 *self, AddrType regAddr, AddrType count, RegType values[])
{
    const uint16_t command[2] = SMARTAR_READ(regAddr);

    return PROTOCOL->executeRead(PROTOCOL, command, count, values);
}

static sr_t RegistersSmartar_writeBurst(IRegisters16_32 *self, AddrType regAddr, AddrType count, const RegType values[])
{
    const uint16_t command[2] = SMARTAR_WRITE(regAddr);
    uint16_t buf[count][2];
    for (AddrType i = 0; i < count; i++)
    {
        const uint16_t writeValue[2] = SMARTAR_WRITE_VALUE(values[i]);
        buf[i][0]                    = writeValue[0];
        buf[i][1]                    = writeValue[1];
    }

    return PROTOCOL->executeWrite(PROTOCOL, command, count, (const uint16_t(*)[2])buf);
}

static sr_t RegistersSmartar_read(IRegisters16_32 *self, AddrType regAddr, RegType *value)
{
    return RegistersSmartar_readBurst(self, regAddr, 1, value);
}

static sr_t RegistersSmartar_write(IRegisters16_32 *self, AddrType regAddr, RegType value)
{
    return RegistersSmartar_writeBurst(self, regAddr, 1, &value);
}

void RegistersSmartar_Constructor(RegistersSmartar *self, IProtocolSmartar *protocol)
{
    Registers16_32_Constructor(&self->b_Registers, 1);

    self->b_Registers.b_IRegisters.read  = RegistersSmartar_read;
    self->b_Registers.b_IRegisters.write = RegistersSmartar_write;

    self->b_Registers.b_IRegisters.readBurst  = RegistersSmartar_readBurst;
    self->b_Registers.b_IRegisters.writeBurst = RegistersSmartar_writeBurst;

    {
        self->m_protocol = protocol;
    }
}

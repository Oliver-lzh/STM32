#include "ProtocolSmartar.h"

#include <common/endian_conversion.h>
#include <fatal_error.h>
#include <stddef.h>


#define SELF ((ProtocolSmartar *)(uintptr_t)self)


sr_t ProtocolSmartar_executeRead(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, uint32_t values[])
{
    RETURN_ON_ERROR(SELF->m_accessSpi->write16(SELF->m_devId, 2, command, true));
    RETURN_ON_ERROR(SELF->m_accessSpi->read16(SELF->m_devId, 2 * count, (uint16_t *)values, false));

    while (count--)
    {
        *values = (*values << 16) | (*values >> 16);
        values++;
    }

    return E_SUCCESS;
}

sr_t ProtocolSmartar_executeWrite(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, const uint16_t values[][2])
{
    RETURN_ON_ERROR(SELF->m_accessSpi->write16(SELF->m_devId, 2, command, true));
    RETURN_ON_ERROR(SELF->m_accessSpi->write16(SELF->m_devId, 2 * count, *values, false));

    return E_SUCCESS;
}

sr_t ProtocolSmartar_setBits(IProtocolSmartar *self, uint16_t address, uint32_t bitMask)
{
    uint32_t value;
    const uint16_t readCommand[2] = SMARTAR_READ(address);
    RETURN_ON_ERROR(ProtocolSmartar_executeRead(self, readCommand, 1, &value));

    value |= bitMask;
    const uint16_t writeCommand[2] = SMARTAR_WRITE(address);
    const uint16_t writeValue[2]   = SMARTAR_WRITE_VALUE(value);

    return ProtocolSmartar_executeWrite(self, writeCommand, 1, &writeValue);
}

sr_t ProtocolSmartar_executeWriteBatch(IProtocolSmartar *self, const uint16_t commands[][2][2], uint16_t count)
{
    while (count--)
    {
        RETURN_ON_ERROR(ProtocolSmartar_executeWrite(self, (*commands)[0], 1, &((*commands)[1])));
        commands++;
    }

    return E_SUCCESS;
}

void ProtocolSmartar_Constructor(ProtocolSmartar *self, ISpi *accessSpi, uint8_t devId)
{
    self->b_IProtocolSmartar.executeRead       = ProtocolSmartar_executeRead;
    self->b_IProtocolSmartar.executeWrite      = ProtocolSmartar_executeWrite;
    self->b_IProtocolSmartar.setBits           = ProtocolSmartar_setBits;
    self->b_IProtocolSmartar.executeWriteBatch = ProtocolSmartar_executeWriteBatch;
    {
        self->m_accessSpi = accessSpi;
        self->m_devId     = devId;

        const uint8_t flags = SPI_MODE_0;  ///< SPI mode/configuration flags

        const uint8_t wordSize = 16;  ///< number of bits per transaction

        const uint32_t speed = 50000000;  //spi speed needed for fpga emulating smartar
        if (self->m_accessSpi->configure(self->m_devId, flags, wordSize, speed) != E_SUCCESS)
        {
            fatal_error(FATAL_ERROR_SPI_CONFIG_FAILED);
        }
    }
}

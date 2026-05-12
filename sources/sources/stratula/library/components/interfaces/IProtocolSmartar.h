/**
 * \addtogroup      IProtocolSmartar
 * \brief
 * Radar component command interface
 * @{
 */

#ifndef I_PROTOCOL_SMARTAR_H
#define I_PROTOCOL_SMARTAR_H 1

#include <common/errors.h>
#include <stdint.h>


#define I_PROTOCOL_SMARTAR_WRITE_BIT (1u << 15)


#define SMARTAR_WRITE_VALUE(value)  \
    {                               \
        value >> 16, value & 0xFFFF \
    }

#define SMARTAR_READ(address)                 \
    {                                         \
        (address >> 8), (address & 0xFF) << 8 \
    }

#define SMARTAR_WRITE(address)                                               \
    {                                                                        \
        I_PROTOCOL_SMARTAR_WRITE_BIT | (address >> 8), (address & 0xFF) << 8 \
    }

typedef struct _IProtocolSmartar IProtocolSmartar;
struct _IProtocolSmartar
{
    /**
    * Executes a read command.
    */
    sr_t (*executeRead)(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, uint32_t values[]);

    /**
    * Executes a write command.
    */
    sr_t (*executeWrite)(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, const uint16_t values[][2]);

    /**
    * Sets a mask of bits at a given address
    */
    sr_t (*setBits)(IProtocolSmartar *self, uint16_t address, uint32_t bitMask);

    /**
    * Executes a write batch command.
    */
    sr_t (*executeWriteBatch)(IProtocolSmartar *self, const uint16_t commands[][2][2], uint16_t count);
};

#endif  // I_PROTOCOL_SMARTAR_H

/**
 * \internal
 * \addtogroup      Registers
 * \brief
 * Base implementation of device registers access
 * \endinternal
 * @{
 */
#ifndef REGISTERS_16_32_H
#define REGISTERS_16_32_H 1


#include <components/interfaces/IRegisters16_32.h>


typedef struct _Registers16_32 Registers16_32;
struct _Registers16_32
{
    IRegisters16_32 b_IRegisters;

    uint16_t m_increment;
};


void Registers16_32_Constructor(Registers16_32 *self, RegType increment);

#endif /* REGISTERS_16_32_H */

/** @} */

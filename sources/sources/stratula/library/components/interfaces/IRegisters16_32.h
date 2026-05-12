/**
 * \addtogroup      IRegisters
 * \brief
 * Device registers access interface
 * @{
 */
#ifndef I_REGISTERS_16_32_H
#define I_REGISTERS_16_32_H 1

#include "IRegistersHelper.h"


#define AddrType uint16_t
#define RegType  uint32_t

#define IRegistersType IRegisters16_32
#define BatchType      BatchType16_32


#include "IRegisters.h"


#endif /* I_REGISTERS_16_32_H */

/** @} */

/**
 * \addtogroup      RegistersSmartar
 * \brief
 * Implementation of radar component registers access
 * @{
 */
#ifndef REGISTERS_SMARTAR_H
#define REGISTERS_SMARTAR_H 1

#include "ProtocolSmartar.h"
#include <components/Registers16_32.h>


typedef struct
{
    Registers16_32 b_Registers;

    IProtocolSmartar *m_protocol;
} RegistersSmartar;


void RegistersSmartar_Constructor(RegistersSmartar *self, IProtocolSmartar *commands);


#endif /* REGISTERS_SMARTAR_H */

/** @} */

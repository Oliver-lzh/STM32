/**
 * \addtogroup      ProtocolSmartar
 * \brief
 * Implementation of radar command interface
 * @{
 */
#ifndef PROTOCOL_SMARTAR_H
#define PROTOCOL_SMARTAR_H 1

#include <components/interfaces/IProtocolSmartar.h>
#include <platform/interfaces/ISpi.h>


typedef struct _ProtocolSmartar ProtocolSmartar;
struct _ProtocolSmartar
{
    IProtocolSmartar b_IProtocolSmartar;

    ISpi *m_accessSpi;
    uint8_t m_devId;
};


void ProtocolSmartar_Constructor(ProtocolSmartar *self, ISpi *accessSpi, uint8_t devId);


sr_t ProtocolSmartar_executeRead(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, uint32_t values[]);
sr_t ProtocolSmartar_executeWrite(IProtocolSmartar *self, const uint16_t command[2], uint16_t count, const uint16_t values[][2]);
sr_t ProtocolSmartar_setBits(IProtocolSmartar *self, uint16_t address, uint32_t bitMask);
sr_t ProtocolSmartar_executeWriteBatch(IProtocolSmartar *self, const uint16_t commands[][2][2], uint16_t count);

#endif /* PROTOCOL_SMARTAR_H */

/** @} */

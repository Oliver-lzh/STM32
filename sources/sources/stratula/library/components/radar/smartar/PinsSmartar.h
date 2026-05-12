/**
 * \addtogroup      PinsSmartar
 * \brief
 * Implementation of radar component GPIO access
 * @{
 */
#ifndef PINS_SMARTAR_H
#define PINS_SMARTAR_H 1

#include <components/interfaces/IPinsSmartar.h>
#include <platform/interfaces/IGpio.h>


typedef struct _PinsSmartar PinsSmartar;
struct _PinsSmartar
{
    IPinsSmartar b_IPinsSmartar;

    IGpio *m_accessGpio;
    const IPinsSmartarDefinition_t *m_config;
};


void PinsSmartar_Constructor(PinsSmartar *self, IGpio *accessGpio, const IPinsSmartarDefinition_t *config);


sr_t PinsSmartar_reset(IPinsSmartar *self);

sr_t PinsSmartar_setResetPin(IPinsSmartar *self, bool state);

#endif /* PINS_SMARTAR_H */

/** @} */

/**
 * \addtogroup      Smartar
 * \brief
 * Implementation of Smartar radar component
 * @{
 */


#ifndef SMARTAR_H_
#define SMARTAR_H_


#include <components/radar/smartar/PinsSmartar.h>
#include <components/radar/smartar/ProtocolSmartar.h>
#include <components/radar/smartar/RegistersSmartar.h>

#include <components/interfaces/IRadarSmartar.h>
#include <components/radar/BoardRadarDefinition_t.h>
#include <platform/interfaces/IData.h>
#include <platform/interfaces/IGpio.h>
#include <platform/interfaces/II2c.h>


typedef struct _Smartar Smartar;


void Smartar_Constructor(Smartar *self, ISpi *accessSpi, IGpio *accessGpio, const IPinsSmartarDefinition_t *pinsDefinition, const BoardRadarDefinition_t *boardDefinition);

sr_t Smartar_Detect(ISpi *accessSpi, IGpio *accessGpio, const BoardRadarDefinition_t *boardDefinition, const IPinsSmartarDefinition_t *pinsDefinition);

//IRadar to be implemented
sr_t Smartar_reset(IRadarSmartar *self, bool softReset);
sr_t Smartar_initialize(IRadarSmartar *self);
sr_t Smartar_getDataIndex(IRadarSmartar *self, uint8_t *index);

//ISmartar
IRegisters16_32 *Smartar_getIRegisters(IRadarSmartar *self);
IProtocolSmartar *Smartar_getIProtocolSmartar(IRadarSmartar *self);
IPinsSmartar *Smartar_getIPinsSmartar(IRadarSmartar *self);


struct _Smartar
{
    IRadarSmartar b_IRadarSmartar;

    PinsSmartar m_pins;
    RegistersSmartar m_registers;
    ProtocolSmartar m_protocol;

    IGpio *m_accessGpio;
    uint8_t m_dataIndex;

    bool m_initialized;

    /** \private @{ */
    /** @} */
};


#endif /* SMARTAR_H_ */
/**
 * \addtogroup      Ltr11.h
 *
 * \brief
 * Implementation of Ltr11 radar component
 * @{
 */


#ifndef LTR11_H_
#define LTR11_H_ 1

#include <stdbool.h>
#include <stdint.h>

#include "ltr11/PinsLtr11.h"
#include "ltr11/ProtocolLtr11.h"
#include "ltr11/RegistersLtr11.h"
#include <components/interfaces/IRadarLtr11.h>
#include <platform/interfaces/IData.h>


typedef struct _Ltr11 Ltr11;

void Ltr11_Constructor(Ltr11 *self, IData *accessData, IGpio *accessGpio, ISpi *accessSpi, const BoardRadarDefinition_t *boardDefinition, const IPinsLtr11Definition_t *pinsDefinition, const PlatformInterruptDefinition_t *irqConfig);

sr_t Ltr11_Detect(ISpi *accessSpi, const BoardRadarDefinition_t *boardDefinition);

//IRadarLtr11
sr_t Ltr11_reset(IRadarLtr11 *self, bool softReset);
sr_t Ltr11_initialize(IRadarLtr11 *self);
sr_t Ltr11_getDataIndex(IRadarLtr11 *self, uint8_t *index);

IRegisters8_16 *Ltr11_getIRegisters(IRadarLtr11 *self);
IPinsLtr11 *Ltr11_getIPinsLtr11(IRadarLtr11 *self);
IProtocolLtr11 *Ltr11_getIProtocolLtr11(IRadarLtr11 *self);


struct _Ltr11
{
    IRadarLtr11 b_IRadarLtr11;

    RegistersLtr11 m_registers;
    PinsLtr11 m_pins;
    ProtocolLtr11 m_protocol;

    IData *m_accessData;
    uint8_t m_dataIndex;

    bool m_initialized;

    /** \private @{ */
    /** @} */
};

/** @} */

#endif /* LTR11_H_ */

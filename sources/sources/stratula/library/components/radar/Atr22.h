/**
 * \addtogroup      Atr22
 * \brief
 * Implementation of Atr22 radar component
 * @{
 */


#ifndef ATR22_H_
#define ATR22_H_


#include <stdbool.h>
#include <stdint.h>

#include <components/radar/atr22/ProtocolAtr22.h>
#include <components/radar/atr22/RegistersAtr22.h>

#include <components/interfaces/IRadarAtr22.h>
#include <platform/interfaces/IData.h>
#include <platform/interfaces/IGpio.h>
#include <platform/interfaces/II2c.h>


typedef struct _Atr22 Atr22;

sr_t Atr22_WorkaroundEs(IGpio *gpio, const RadarAtr22PinsDefinition_t *pinsDefinition);

void Atr22_Constructor(Atr22 *self, IData *accessData, II2c *accessSpi, const RadarAtr22Definition_t *boardDefinition);

sr_t Atr22_Detect(II2c *accessI2c, const RadarAtr22Definition_t *boardDefinition);


//IRadar to be implemented
sr_t Atr22_reset(IRadarAtr22 *self, bool softReset);
sr_t Atr22_initialize(IRadarAtr22 *self);
sr_t Atr22_getDataIndex(IRadarAtr22 *self, uint8_t *index);

//IRadarAtr22
IRegisters16 *Atr22_getIRegisters(IRadarAtr22 *self);
IProtocolAtr22 *Atr22_getIProtocolAtr22(IRadarAtr22 *self);


struct _Atr22
{
    IRadarAtr22 b_IRadarAtr22;

    RegistersAtr22 m_registers;
    ProtocolAtr22 m_protocol;

    const RadarAtr22PinsDefinition_t *m_pinsConfig;
    IGpio *m_accessGpio;
    IData *m_accessData;

    bool m_initialized;

    /** \private @{ */
    /** @} */
};


#endif /* ATR22_H_ */
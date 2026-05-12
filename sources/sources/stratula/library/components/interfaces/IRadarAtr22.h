/**
 * \addtogroup      IRadarAtr22
 * \brief
 * Radar component access interface
 * @{
 */

#ifndef IRADARATR22_H_
#define IRADARATR22_H_

#include <components/interfaces/IProtocolAtr22.h>
#include <components/interfaces/IRegisters16.h>


#define ATR22_DEFAULT_I2C_DEV_ADDR (0x33)

typedef struct
{
    uint16_t devAddr;
} RadarAtr22Definition_t;

typedef struct
{
    uint16_t gpioEnableLdo;
    uint16_t gpioReset;
} RadarAtr22PinsDefinition_t;

/**
 * \brief Access interface to a radar front end device of the Atr22 family
 */
typedef struct _IRadarAtr22 IRadarAtr22;
struct _IRadarAtr22
{
    /**
     * Perform reset
     * @param softReset Set to true to do a soft reset, otherwise hard reset will be performed
     * @return Strata error code
     */
    sr_t (*reset)(IRadarAtr22 *self, bool softReset);

    /**
     * Initialize
     * @return Strata error code
     */
    sr_t (*initialize)(IRadarAtr22 *self);

    /**
     * Get index of connected data interface
     * @param index pointer to a variable to write the obtained index to
     * @return Strata error code
     */
    sr_t (*getDataIndex)(IRadarAtr22 *self, uint8_t *index);

    IRegisters16 *(*getIRegisters)(IRadarAtr22 *self);
    IProtocolAtr22 *(*getIProtocolAtr22)(IRadarAtr22 *self);
};


#endif /* IRADARATR22_H_ */

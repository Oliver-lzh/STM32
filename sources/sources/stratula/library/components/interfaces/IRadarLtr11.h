/**
 * \addtogroup      IRadarLtr11.h
 * \brief
 * Radar component access interface
 * @{
 */

#ifndef IRADARLTR11_H_
#define IRADARLTR11_H_

#include <components/interfaces/IPinsLtr11.h>
#include <components/interfaces/IProtocolLtr11.h>
#include <components/interfaces/IRegisters8_16.h>
#include <components/radar/BoardRadarDefinition_t.h>


/**
 * \brief Access interface to a radar front end device of type BGT60LTR11.
 */
typedef struct _IRadarLtr11 IRadarLtr11;
struct _IRadarLtr11
{
    /**
     * Perform reset
     * @param softReset Set to true to do a soft reset, otherwise hard reset will be performed
     * @return Strata error code
     */
    sr_t (*reset)(IRadarLtr11 *self, bool softReset);

    /**
     * Initialize
     * @return Strata error code
     */
    sr_t (*initialize)(IRadarLtr11 *self);

    /**
     * Get index of connected data interface
     * @param index pointer to a variable to write the obtained index to
     * @return Strata error code
     */
    sr_t (*getDataIndex)(IRadarLtr11 *self, uint8_t *index);

    IRegisters8_16 *(*getIRegisters)(IRadarLtr11 *self);
    IPinsLtr11 *(*getIPinsLtr11)(IRadarLtr11 *self);
    IProtocolLtr11 *(*getIProtocolLtr11)(IRadarLtr11 *self);
};


#endif /* IRADARLTR11_H_ */

/** @} */

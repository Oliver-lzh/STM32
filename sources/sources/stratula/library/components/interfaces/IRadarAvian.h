/**
 * \addtogroup      IRadarAvian
 * \brief
 * Radar component access interface
 * @{
 */
#ifndef I_RADAR_AVIAN_H
#define I_RADAR_AVIAN_H 1

#include <components/interfaces/IPinsAvian.h>
#include <components/interfaces/IProtocolAvian.h>
#include <components/interfaces/IRegisters8_32.h>
#include <components/radar/BoardRadarDefinition_t.h>


/**
 * \brief Access interface to a radar front end device of the Avian family,
 * e.g. BGT60TR13C, BGT60ATR24C, BGT60TR13D and BGT60TR12E.
 */
typedef struct _IRadarAvian IRadarAvian;
struct _IRadarAvian
{
    /**
     * Perform reset
     * @param softReset Set to true to do a soft reset, otherwise hard reset will be performed
     * @return Strata error code
     */
    sr_t (*reset)(IRadarAvian *self, bool softReset);

    /**
     * Initialize
     * @return Strata error code
     */
    sr_t (*initialize)(IRadarAvian *self);

    /**
     * Starts the data interface
     * @return Strata error code
     */
    sr_t (*startData)(IRadarAvian *self);

    /**
     * Stops the data interface
     * @return Strata error code
     */
    sr_t (*stopData)(IRadarAvian *self);

    /**
     * Get index of connected data interface
     * @param index pointer to a variable to write the obtained index to
     * @return Strata error code
     */
    sr_t (*getDataIndex)(IRadarAvian *self, uint8_t *index);

    IRegisters8_32 *(*getIRegisters)(IRadarAvian *self);
    IPinsAvian *(*getIPinsAvian)(IRadarAvian *self);
    IProtocolAvian *(*getIProtocolAvian)(IRadarAvian *self);
};


#endif /* I_RADAR_AVIAN_H */

/** @} */

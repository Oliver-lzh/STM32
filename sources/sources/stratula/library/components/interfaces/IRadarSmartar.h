/**
 * \addtogroup      IRadarSmartar
 * \brief
 * Radar component access interface
 * @{
 */

#ifndef IRADARSMARTAR_H_
#define IRADARSMARTAR_H_ 1

#include <components/interfaces/IPinsSmartar.h>
#include <components/interfaces/IProtocolSmartar.h>
#include <components/interfaces/IRegisters16_32.h>


/**
 * \brief Access interface to a radar front end device of the Smartar family
 */
typedef struct _IRadarSmartar IRadarSmartar;
struct _IRadarSmartar
{
    /**
     * Perform reset
     * @param softReset Set to true to do a soft reset, otherwise hard reset will be performed
     * @return Strata error code
     */
    sr_t (*reset)(IRadarSmartar *self, bool softReset);

    /**
     * Initialize
     * @return Strata error code
     */
    sr_t (*initialize)(IRadarSmartar *self);

    /**
     * Get index of connected data interface
     * @param index pointer to a variable to write the obtained index to
     * @return Strata error code
     */
    sr_t (*getDataIndex)(IRadarSmartar *self, uint8_t *index);

    IRegisters16_32 *(*getIRegisters)(IRadarSmartar *self);
    IProtocolSmartar *(*getIProtocolSmartar)(IRadarSmartar *self);
    IPinsSmartar *(*getIPinsSmartar)(IRadarSmartar *self);
};


#endif /* IRADARSMARTAR_H_ */
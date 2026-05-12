/**
 * \addtogroup      IPinsSmartar
 * \brief
 * Radar component GPIO access interface
 * @{
 */
#ifndef I_PINS_SMARTAR_H
#define I_PINS_SMARTAR_H 1

#include <common/errors.h>
#include <platform/interfaces/IGpio.h>


typedef struct
{
    uint16_t gpioReset;
} IPinsSmartarDefinition_t;


typedef struct _IPinsSmartar IPinsSmartar;
struct _IPinsSmartar
{
    /**
    * Sets state of radar reset pin.
    *
    * @param state true for high and false for low
    * @return Strata error code
    */
    sr_t (*setResetPin)(IPinsSmartar *self, bool state);

    sr_t (*reset)(IPinsSmartar *self);
};


#endif /* I_PINS_SMARTAR_H */

/** @} */

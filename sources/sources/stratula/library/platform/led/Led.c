#include "Led.h"

#include <impl/PlatformGpio.h>
#include <stddef.h>


sr_t Led_initialize(const LedDefinition_t *led)
{
    if (led == NULL)
    {
        return E_SUCCESS;
    }

    uint8_t flags = led->activeLow ? (GPIO_MODE_OUTPUT_OPEN_DRAIN | GPIO_FLAG_OUTPUT_INITIAL_HIGH) : GPIO_MODE_OUTPUT_OPEN_SOURCE;
    sr_t result   = PlatformGpio_configurePin(led->gpio, flags);
    if (result == E_INVALID_PARAMETER)
    {
        //Some MCUs don't support all modes. In this case we fall back to push-pull mode.
        flags  = GPIO_MODE_OUTPUT_PUSH_PULL | (led->activeLow ? GPIO_FLAG_OUTPUT_INITIAL_HIGH : 0);
        result = PlatformGpio_configurePin(led->gpio, flags);
    }
    return result;
}

sr_t Led_set(const LedDefinition_t *led, bool state)
{
    if (led == NULL)
    {
        return E_SUCCESS;
    }

    return PlatformGpio_setPin(led->gpio, state ^ led->activeLow);
}

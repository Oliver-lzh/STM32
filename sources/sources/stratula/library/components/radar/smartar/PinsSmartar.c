#include "PinsSmartar.h"

#include <impl/thread.h>


#define SELF ((PinsSmartar *)(uintptr_t)self)


#define RESET_PULSE_WIDTH_US 1 /*  Minimum required is 10ns */


sr_t PinsSmartar_setResetPin(IPinsSmartar *self, bool state)
{
    return SELF->m_accessGpio->setPin(SELF->m_config->gpioReset, state);
}

sr_t PinsSmartar_reset(IPinsSmartar *self)
{
    RETURN_ON_ERROR(PinsSmartar_setResetPin(self, false));
    this_thread_sleep_for(chrono_microseconds(RESET_PULSE_WIDTH_US));  // T_reset
    RETURN_ON_ERROR(PinsSmartar_setResetPin(self, true));
    this_thread_sleep_for(chrono_microseconds(RESET_PULSE_WIDTH_US));  // T_reset

    return E_SUCCESS;
}


void PinsSmartar_Constructor(PinsSmartar *self, IGpio *accessGpio, const IPinsSmartarDefinition_t *config)
{
    self->b_IPinsSmartar.setResetPin = PinsSmartar_setResetPin;
    self->b_IPinsSmartar.reset       = PinsSmartar_reset;

    {
        self->m_config     = config;
        self->m_accessGpio = accessGpio;
    }

    self->m_accessGpio->configurePin(self->m_config->gpioReset, GPIO_MODE_OUTPUT_PUSH_PULL | GPIO_FLAG_OUTPUT_INITIAL_HIGH);
}

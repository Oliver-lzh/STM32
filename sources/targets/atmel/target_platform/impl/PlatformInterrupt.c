/* ===========================================================================
** Copyright (C) 2021 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/

#include "PlatformInterrupt.h"

#include "ConfigurationIsr.h"
#include <common/typeutils.h>

#include <pio.h>
#include <pio_handler.h>

/******************************************************************************/
/*Struct Definitions ----------------------------------------------------------*/
/******************************************************************************/
typedef struct
{
    /** \brief callback function for platform interrupt handler */
    void (*p_handler)(uint32_t, uint32_t);
    /** \brief callback function to invoke from platform interrupt handler */
    void (*fn)(void *);
    /** \brief context to pass to the callback function */
    void *arg;
} InterruptHandler_t;

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/

/* the parameters id and mask are not used, since using
   separate callback functions is easier than a lookup.
 */
static void pio_handler0(const uint32_t id, const uint32_t mask);
static void pio_handler1(const uint32_t id, const uint32_t mask);
static void pio_handler2(const uint32_t id, const uint32_t mask);
static void pio_handler3(const uint32_t id, const uint32_t mask);

#define PLATFORM_INTERRUPT_MAX_COUNT 4u
static InterruptHandler_t m_handlers[PLATFORM_INTERRUPT_MAX_COUNT] = {
    {pio_handler0},
    {pio_handler1},
    {pio_handler2},
    {pio_handler3},
};
static unsigned int m_count = 0;

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

static inline void irq_handler(unsigned int i)
{
    InterruptHandler_t *handler = &m_handlers[i];

    if (handler->fn != NULL)
    {
        handler->fn(handler->arg);
    }
}

static void pio_handler0(const uint32_t id, const uint32_t mask)
{
    irq_handler(0);
}

static void pio_handler1(const uint32_t id, const uint32_t mask)
{
    irq_handler(1);
}

static void pio_handler2(const uint32_t id, const uint32_t mask)
{
    irq_handler(2);
}

static void pio_handler3(const uint32_t id, const uint32_t mask)
{
    irq_handler(3);
}

/******************************************************************************/
/*Public Methods Definition --------------------------------------------------*/
/******************************************************************************/

sr_t PlatformInterrupt_registerCallback(const PlatformInterruptDefinition_t *definition, void(fn)(void *), void *arg)
{
    if (m_count == PLATFORM_INTERRUPT_MAX_COUNT)
    {
        return E_OUT_OF_BOUNDS;
    }

    const uint16_t pin = definition->pin;

    ioport_set_pin_dir(pin, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(pin, 0);
    ioport_enable_pin(pin);

    Pio *const pinGroup         = pio_get_pin_group(pin);
    const uint32_t pinGroupId   = pio_get_pin_group_id(pin);
    const uint32_t pinGroupMask = pio_get_pin_group_mask(pin);
    pio_set_input(pinGroup, pinGroupMask, definition->ul_attribute);
    pio_handler_set(pinGroup, pinGroupId, pinGroupMask, definition->ul_attr, m_handlers[m_count].p_handler);

    m_handlers[m_count].fn  = fn;
    m_handlers[m_count].arg = arg;

    const IRQn_Type IRQn    = (IRQn_Type)pinGroupId;
    const uint32_t priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), ISR_PRIORITY_GPIO_IRQ, m_count);
    NVIC_SetPriority(IRQn, priority);
    NVIC_EnableIRQ(IRQn);

    m_count++;
    return E_SUCCESS;
}

sr_t PlatformInterrupt_enable(const PlatformInterruptDefinition_t *definition, bool enable)
{
    const uint16_t pin          = definition->pin;
    Pio *const pinGroup         = pio_get_pin_group(pin);
    const uint32_t pinGroupMask = pio_get_pin_group_mask(pin);

    if (enable)
    {
        const uint32_t pinGroupId = pio_get_pin_group_id(pin);
        pio_handler_process(pinGroup, pinGroupId);  // workaround to clear pending interrupt
        pio_enable_interrupt(pinGroup, pinGroupMask);
    }
    else
    {
        pio_disable_interrupt(pinGroup, pinGroupMask);
    }

    /* We do not need to enable/disable the NVIC.
       If the PIO does not have any pin enabled, also the NVIC
       interrupt will never occur even though it is enabled.
     */

    return E_SUCCESS;
}

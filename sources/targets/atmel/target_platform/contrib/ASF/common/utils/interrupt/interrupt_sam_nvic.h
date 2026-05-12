/**
 * \file
 *
 * \brief Global interrupt management for SAM D20, SAM3 and SAM4 (NVIC based)
 *
 * Copyright (c) 2012-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef UTILS_INTERRUPT_INTERRUPT_H
#define UTILS_INTERRUPT_INTERRUPT_H

#include <compiler.h>
#include <parts.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \weakgroup interrupt_group
 *
 * @{
 */

/**
 * \name Interrupt Service Routine definition
 *
 * @{
 */

/**
 * \brief Define service routine
 *
 * \note For NVIC devices the interrupt service routines are predefined to
 *       add to vector table in binary generation, so there is no service
 *       register at run time. The routine collections are in exceptions.h.
 *
 * Usage:
 * \code
	ISR(foo_irq_handler)
	{
	     // Function definition
	     ...
	}
\endcode
 *
 * \param func Name for the function.
 */
#  define ISR(func)   \
	void func (void)

/**
 * \brief Initialize interrupt vectors
 *
 * For NVIC the interrupt vectors are put in vector table. So nothing
 * to do to initialize them, except defined the vector function with
 * right name.
 *
 * This must be called prior to \ref irq_register_handler.
 */
#  define irq_initialize_vectors()   \
	do {                             \
	} while(0)

/**
 * \brief Register handler for interrupt
 *
 * For NVIC the interrupt vectors are put in vector table. So nothing
 * to do to register them, except defined the vector function with
 * right name.
 *
 * Usage:
 * \code
	irq_initialize_vectors();
	irq_register_handler(foo_irq_handler);
\endcode
 *
 * \note The function \a func must be defined with the \ref ISR macro.
 * \note The functions prototypes can be found in the device exception header
 *       files (exceptions.h).
 */
#  define irq_register_handler(int_num, int_prio)                      \
	NVIC_ClearPendingIRQ(    (IRQn_Type)int_num);                      \
	NVIC_SetPriority(    (IRQn_Type)int_num, int_prio);                \
	NVIC_EnableIRQ(      (IRQn_Type)int_num);                          \

//@}

#  define cpu_irq_enable()                     \
	do {                                       \
		g_interrupt_enabled = true;            \
		__DMB();                               \
		__enable_irq();                        \
	} while (0)
#  define cpu_irq_disable()                    \
	do {                                       \
		__disable_irq();                       \
		__DMB();                               \
		g_interrupt_enabled = false;           \
	} while (0)

typedef uint32_t irqflags_t;

#if !defined(__DOXYGEN__)
extern volatile bool g_interrupt_enabled;
#endif

#define cpu_irq_is_enabled()    (__get_PRIMASK() == 0)

static volatile uint32_t cpu_irq_critical_section_counter;
static volatile bool     cpu_irq_prev_interrupt_state;

static inline irqflags_t cpu_irq_save(void)
{
	volatile irqflags_t flags = cpu_irq_is_enabled();
	cpu_irq_disable();
	return flags;
}

static inline bool cpu_irq_is_enabled_flags(irqflags_t flags)
{
	return (flags);
}

static inline void cpu_irq_restore(irqflags_t flags)
{
	if (cpu_irq_is_enabled_flags(flags))
		cpu_irq_enable();
}

void cpu_irq_enter_critical(void);
void cpu_irq_leave_critical(void);

/**
 * \weakgroup interrupt_deprecated_group
 * @{
 */

#define Enable_global_interrupt()            cpu_irq_enable()
#define Disable_global_interrupt()           cpu_irq_disable()
#define Is_global_interrupt_enabled()        cpu_irq_is_enabled()

//@}

/**
 * \weakgroup interrupt_deprecated_group
 *
 * The functions in this group where added to allow USB library to control only the USB interrupt
 * in the same way as \ref cpu_irq_save and \ref cpu_irq_restore do.
 * @{
 */

/**
 * \brief This function checks if the specified interrupt is enabled.
 *
 * The NVIC allows to read the enable status of each IRQ through the enable registers ISER. This
 * function does the picky work of selecting the right ISER register and the right bit from that
 * register. This is done in the same way as it is done in \ref NVIC_EnableIRQ.
 */
__STATIC_INLINE uint32_t NVIC_GetEnabledIRQ(IRQn_Type IRQn)
{
	return NVIC->ISER[(uint32_t)((int32_t)IRQn) >> 5] & (uint32_t)(1 << ((uint32_t)((int32_t)IRQn) & (uint32_t)0x1F));
}

/*
 * This function disables the USB interrupt and returns a flag that indicates if the IRQ was
 * enabled before this function was called.
 * The implementation was copied from \ref cpu_irq_save and modified.
 */
static inline irqflags_t usb_irq_save(void)
{
	volatile irqflags_t flags = NVIC_GetEnabledIRQ(USBHS_IRQn);
	NVIC_DisableIRQ(USBHS_IRQn);
	__DMB();
	return flags;
}

/*
 * This function enables the USB interrupt according to the provided parameter flags. The parameter
 * flags should be the value returned by \ref usb_irq_save.
 * The implementation was copied from \ref cpu_irq_restore and modified.
 */
static inline void usb_irq_restore(irqflags_t flags)
{
	if (flags)
	{
		__DMB();
		NVIC_EnableIRQ(USBHS_IRQn);
	}
}
//@}

//@}

#ifdef __cplusplus
}
#endif

#endif /* UTILS_INTERRUPT_INTERRUPT_H */

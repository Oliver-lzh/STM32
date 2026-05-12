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

/**
 * \file SysTimer.h
 * \ingroup Peripherals
 *
 * \brief The Systimer allows to query the elapsed time in milliseconds
 */

#ifndef SYSTIMER_H_
#define SYSTIMER_H_

/*******************************************************************************
 * Includes and Forward Declarations
 *******************************************************************************/

#include <stdint.h>

/** \brief Number of microseconds for one tick.
  *
  * One tick corresponds to 200 microseconds.
  */
#define TICK_TIME_MICROSECS 200

/*******************************************************************************
 * Functions
 *******************************************************************************/

/** \brief Count of 1ms ticks since the SysTimer has been started.
 * \note Do not use this variable directly, instead access it via
 *       SysTimer_getTime().
 */
extern volatile uint32_t SysTimer_s_uSysTickCounter;

/** \brief Initialize the system timer.
  */
void SysTimer_init(void);

/** \brief get number of system ticks from microsecond value.
  */
uint32_t SysTimer_getTicks_per_us(uint16_t microseconds);

/*!
 * \brief This function return the current system tick counter.
 */
static inline uint32_t SysTimer_getTime(void)
{
    return SysTimer_s_uSysTickCounter;
}


#endif /* SYSTIMER_H_ */

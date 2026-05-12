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

/*
 * \file SysTimer.c
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "SysTimer.h"
#include <impl/ConfigurationIsr.h>
#include <sysclk.h>

/*******************************************************************************
 * Defines
 *******************************************************************************/

#define CONST_TICK_FACTOR 1000000
#define TICK_INIT         (CONST_TICK_FACTOR / TICK_TIME_MICROSECS)

/*******************************************************************************
 * Module Data
 *******************************************************************************/
volatile uint32_t SysTimer_s_uSysTickCounter = 0;

/*******************************************************************************
 * Functions
 *******************************************************************************/

void SysTimer_init(void)
{
    // configure sys tick timer
    // -----------------------
    SysTick_Config(SystemCoreClock / TICK_INIT);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), ISR_PRIORITY_SYSTIMER_IRQ, 0));
}

uint32_t SysTimer_getTicks_per_us(uint16_t microseconds)
{
    return (microseconds / TICK_TIME_MICROSECS);
}

// interrupt handlers must be linked in C style, otherwise the concept of weak definition in
// startup files won't work
void SysTick_Handler(void)
{
    SysTimer_s_uSysTickCounter++;
}

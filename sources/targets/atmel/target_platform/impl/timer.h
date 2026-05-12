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

#ifndef TIMER_H_
#define TIMER_H_ 1

#include <common/errors.h>
#include <stddef.h>
#include <stdint.h>


typedef void (*Timer_Callback)(void *arg);
typedef uint32_t TimerId_t;
typedef uint32_t timer_ticks_t;


static inline timer_ticks_t timer_milliseconds(uint32_t interval)
{
    return interval * 1000;
}

static inline timer_ticks_t timer_microseconds(uint32_t interval)
{
    return interval;
}


void Timer_Constructor(void);

TimerId_t Timer_add(void);
sr_t Timer_setCallback(TimerId_t timerId, Timer_Callback callback, void *arg);
sr_t Timer_start(TimerId_t timerId, timer_ticks_t interval);
sr_t Timer_setInterval(TimerId_t timerId, timer_ticks_t interval);
sr_t Timer_stop(TimerId_t timerId);

#endif /* TIMER_H_ */

#include "timer.h"
#include <impl/ConfigurationIsr.h>

#include <pmc.h>
#include <sysclk.h>
#include <tc.h>

#define MAX_TIMERS 2


typedef struct
{
    Timer_Callback m_callback;
    void *m_callbackArg;
} Timer;


static Timer m_timers[MAX_TIMERS];
static TimerId_t m_count = 0;

// currently we only use channel 0 for simplicity
static const uint32_t ch = 0;


/*******************************************************************************
 * Functions
 *******************************************************************************/
static inline Tc *p_tc(TimerId_t timerId)
{
    if (timerId == 1)
    {
        return TC3;
    }
    else
    {
        return TC0;
    }
}

static inline void tc_handler(TimerId_t timerId)
{
    // must read status register to clear interrupt
    volatile uint32_t tc_status = tc_get_status(p_tc(timerId), ch);
    (void)tc_status;

    if (m_timers[timerId].m_callback)
    {
        m_timers[timerId].m_callback(m_timers[timerId].m_callbackArg);
    }
}

void TC0_Handler(void)
{
    tc_handler(0);
}

void TC9_Handler(void)
{
    tc_handler(1);
}

TimerId_t Timer_add(void)
{
    if (m_count == MAX_TIMERS)
    {
        return (TimerId_t)-1;
    }

    m_timers[m_count].m_callback = NULL;

    uint32_t ul_id;
    if (m_count == 1)
    {
        ul_id = ID_TC9;
    }
    else
    {
        ul_id = ID_TC0;
    }

    //Stopping the timer, disabling and clearing the interrupt is only needed for the debugging workaround.
    //#TODO: Change the debugging settings.
    tc_stop(p_tc(m_count), ch);
    /* Configure PMC */
    pmc_enable_periph_clk(ul_id);

    /* Disable and Clear any pending previous timer interrupt */
    NVIC_DisableIRQ((IRQn_Type)ul_id);
    NVIC_ClearPendingIRQ((IRQn_Type)ul_id);

    /* Configure and enable interrupt on RC compare */
    NVIC_SetPriority((IRQn_Type)ul_id, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), ISR_PRIORITY_TIMER_COUNTER_IRQ, 0));
    NVIC_EnableIRQ((IRQn_Type)ul_id);
    return m_count++;
}

sr_t Timer_setCallback(TimerId_t timerId, Timer_Callback callback, void *arg)
{
    if (timerId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }

    m_timers[timerId].m_callback    = callback;
    m_timers[timerId].m_callbackArg = arg;

    return E_SUCCESS;
}

sr_t Timer_start(TimerId_t timerId, timer_ticks_t interval)
{
    if (timerId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }

    Tc *tc = p_tc(timerId);
    tc_stop(tc, ch);

    const uint32_t baseFreq = 1000000;

    //Main board clock
    const uint32_t ul_sysclk = sysclk_get_cpu_hz();
    interval /= 2;
    if (interval == 0)
    {
        interval = 1;
    }
    uint32_t freq = baseFreq / interval;
    if (freq == 0)
    {
        freq = 1;
    }

    uint32_t ul_div;
    uint32_t ul_tcclks;

    if (tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk) == 0)
    {
        return E_INVALID_PARAMETER;
    }
    /* Overwriting the result of tc_find_mck_divisor when the ul_tcclks is 0, as usually the latter gives a good divisor,
    but if ult_tcclks= 0, the corresponding range of the counts value is not adequate.
    In addition, TC_CMR_TCCLKS_TIMER_CLOCK1 is not consistent with the calculation function, which assumes divider == 2, 
    while the actual hardware is running off a different clock (PCK6 or PCK7 for TC0).*/
    if (ul_tcclks == 0)
    {
        ul_tcclks = 1;
        ul_div    = 8;
    }

    const uint32_t counts = ul_sysclk / ul_div / freq;
    //counts is written to RC compare, which range is 0-65535
    if (counts > 65535)
    {
        return E_INVALID_PARAMETER;
    }
    const uint32_t ul_mode = TC_CMR_TCCLKS(ul_tcclks) | TC_CMR_CPCTRG;

    tc_init(tc, ch, ul_mode);
    tc_write_rc(tc, ch, counts);

    // start timer
    tc_enable_interrupt(tc, ch, TC_IER_CPCS);
    tc_start(tc, ch);

    return E_SUCCESS;
}

sr_t Timer_setInterval(TimerId_t timerId, timer_ticks_t interval)
{
    if (timerId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }

    Timer_stop(timerId);

    return Timer_start(timerId, interval);
}

sr_t Timer_stop(TimerId_t timerId)
{
    if (timerId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }

    tc_stop(p_tc(timerId), ch);

    return E_SUCCESS;
}

void Timer_Constructor(void)
{
    pmc_set_writeprotect(0);
}

/// systick.h

// From the clock tree diagram, the core system timer systick is fed by HCLK / 8
// - At 48MHz HCLK: Systick clock = 6MHz
// - At 24Mhz HCLK: Systick clock = 3MHz


#ifndef CH32V003_HAL_SYSTICK_H
#define CH32V003_HAL_SYSTICK_H

#include <stdint.h>
#include "rcc.h"

//                     System counter control register bits

// Bit 0 STE — system counter enable
//   1: Start the system counter STK
//   0: Turn off the system counter STK
#define STK_CTLR_STE_BIT     (0U)
#define STK_CTLR_STE_MASK    ((uint32_t)(1UL << STK_CTLR_STE_BIT))

// Bit 1 STIE — counter interrupt enable
//   1: Enable counter interrupts
//   0: Turn off the counter interrupt
#define STK_CTLR_STIE_BIT    (1U)
#define STK_CTLR_STIE_MASK   ((uint32_t)(1UL << STK_CTLR_STIE_BIT))

// Bit 2 STCLK — clock source selection
//   1: HCLK for time base
//   0: HCLK/8 for time base
#define STK_CTLR_STCLK_BIT   (2U)
#define STK_CTLR_STCLK_MASK  ((uint32_t)(1UL << STK_CTLR_STCLK_BIT))

// Bit 3 STRE — auto-reload count enable
//   1: Count up to compare value then reset to 0
//   0: Continue counting up
#define STK_CTLR_STRE_BIT    (3U)
#define STK_CTLR_STRE_MASK   ((uint32_t)(1UL << STK_CTLR_STRE_BIT))

// Bit 31 SWIE — software interrupt trigger enable
//   1: Trigger software interrupt
//   0: Turn off trigger
//   Note: must write 0 after ISR entry or interrupt fires continuously
#define STK_CTLR_SWIE_BIT    (31U)
#define STK_CTLR_SWIE_MASK   ((uint32_t)(1UL << STK_CTLR_SWIE_BIT))



//                      STK_SR bit definitions

// Bit 0 CNTIF — counting value comparison flag (write 0 to clear)
//   1: Counter has reached the comparison value
//   0: Comparison value not yet reached
#define STK_SR_CNTIF_BIT     (0U)
#define STK_SR_CNTIF_MASK    ((uint32_t)(1UL << STK_SR_CNTIF_BIT))



typedef struct {
    volatile uint32_t ctlr;       // 0x00 control register
    volatile uint32_t sr;         // 0x04 status register
    volatile uint32_t cntr_low;   // 0x08 count low 32 bits
    volatile uint32_t cntr_hi;    // 0x0C count hi 32 bits
    volatile uint32_t cmpr_low;   // 0x10 compare low 32 bits
    volatile uint32_t cmpr_hi;    // 0x14 compare hi 32 bits 
} SysTick_TypeDef;


static inline volatile SysTick_TypeDef *SysTick_GetBase(void) {
    return (volatile SysTick_TypeDef *)0xE000F000U;
}

/* Returns the actual SysTick clock frequency in Hz.
 * Depends on STCLK bit in STK_CTLR:
 *   STCLK = 0 -> SysTick clock = HCLK / 8  (default)
 *   STCLK = 1 -> SysTick clock = HCLK direct */
static uint32_t SysTick_GetClockHz(void)
{
    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();
    uint32_t hclk                          = 0U;
    uint32_t freq                          = 0U;

    hclk = RCC_Get_Hclk_Hz();

    if (hclk == RCC_FREQ_UNKNOWN)
    {
        freq = RCC_FREQ_UNKNOWN;
    }
    else if ((p_stk->ctlr & STK_CTLR_STCLK_MASK) != 0U)
    {
        freq = hclk;           /* STCLK=1: HCLK direct */
    }
    else
    {
        freq = hclk / 8U;      /* STCLK=0: HCLK/8 (default) */
    }

    return freq;
}

static uint64_t SysTick_CountValue(void) {
    uint32_t hi1     = 0U;
    uint32_t lo      = 0U;
    uint32_t hi2     = 0U;
    uint64_t value   = 0U;
    uint32_t settled = 0U;

    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();

    while (settled == 0U) {
        hi1 = p_stk->cntr_hi;
        lo  = p_stk->cntr_low;
        hi2 = p_stk->cntr_hi;

        if (hi1 == hi2) {
            value  = (uint64_t)(((uint64_t)hi1 << 32U) | (uint64_t)lo);
            settled = 1U;
        }
    }
    return value;
}


static uint64_t SysTick_CompareValue(void) {
    uint32_t cmpr_hi  = 0U;
    uint32_t cmpr_lo  = 0U;
    uint64_t value    = 0U;

    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();

    cmpr_hi = p_stk->cmpr_hi;
    cmpr_lo = p_stk->cmpr_low;

    value   = (uint64_t)(((uint64_t)cmpr_hi << 32U) | (uint64_t)cmpr_lo);
    return value;
}


static void SysTick_SetCompareValue(uint64_t compare_value) {
    uint32_t cmpr_lo_val = 0U;
    uint32_t cmpr_hi_val = 0U;
    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();

    cmpr_lo_val = (uint32_t)(compare_value & (uint64_t)0xFFFFFFFFUL);
    cmpr_hi_val = (uint32_t)(compare_value >> 32U);

    p_stk->cmpr_hi  = cmpr_hi_val;
    p_stk->cmpr_low = cmpr_lo_val;
}

static uint64_t SysTicks_Elapsed(uint64_t since) {
    return SysTick_CountValue() - since;
}

static uint64_t SysTick_Ticks_To_Us(uint64_t ticks) {
    uint32_t hclk         = 0U;
    uint32_t ticks_per_us = 0U;
    uint64_t result       = 0U;

    hclk = SysTick_GetClockHz();

    if (hclk >= (uint32_t)1000000U) {
        ticks_per_us = hclk / (uint32_t)1000000U;
        result = ticks / (uint64_t)ticks_per_us;
    }

    return result;
}

static uint64_t SysTick_Ticks_To_Ms(uint64_t ticks) {
    uint32_t hclk         = 0U;
    uint32_t ticks_per_ms = 0U;
    uint64_t result       = 0U;

    hclk = SysTick_GetClockHz();

    if (hclk >= (uint32_t)1000U) {
        ticks_per_ms = hclk / (uint32_t)1000U;
        result = ticks / (uint64_t)ticks_per_ms;
    }

    return result;
}

static void SysTick_Delay_Ticks(uint64_t ticks)
{
    uint64_t start = 0U;

    if (ticks == 0U)
    {
        return;
    }

    start = SysTick_CountValue();

    while ((SysTick_CountValue() - start) < ticks) {

    }
}

static uint64_t SysTick_Ms_To_Ticks(uint32_t ms)
{
    uint32_t hclk         = 0U;
    uint32_t ticks_per_ms = 0U;
    uint64_t result       = 0U;

    hclk = SysTick_GetClockHz();

    if ((hclk != RCC_FREQ_UNKNOWN) && (hclk >= 1000U))
    {
        ticks_per_ms = hclk / 1000U;
        result       = (uint64_t)ms * (uint64_t)ticks_per_ms;
    }

    return result;
}

static uint64_t SysTick_Us_To_Ticks(uint32_t us)
{
    uint32_t hclk         = 0U;
    uint32_t ticks_per_us = 0U;
    uint64_t result       = 0U;

    hclk = SysTick_GetClockHz();

    if ((hclk != RCC_FREQ_UNKNOWN) && (hclk >= 1000000U))
    {
        ticks_per_us = hclk / 1000000U;
        result       = (uint64_t)us * (uint64_t)ticks_per_us;
    }

    return result;
}

static void SysTick_Delay_Ms(uint32_t ms)
{
    SysTick_Delay_Ticks(SysTick_Ms_To_Ticks(ms));
}

static void SysTick_Delay_Us(uint32_t us)
{
    SysTick_Delay_Ticks(SysTick_Us_To_Ticks(us));
}

typedef enum {
    STK_CLKSRC_HCLK_DIV8  = 0U,   // HCLK / 8 — default, lower power
    STK_CLKSRC_HCLK       = 1U    // HCLK direct — higher resolution
} STK_ClkSrc_t;

typedef enum {
    STK_INTR_DISABLE = 0U,
    STK_INTR_ENABLE  = 1U
} STK_Intr_t;

typedef enum {
    STK_RELOAD_DISABLE = 0U,     // count up forever
    STK_RELOAD_ENABLE  = 1U      // reset to 0 on compare match
} STK_Reload_t;

typedef enum {
    STK_SWIE_DISABLE = 0U,
    STK_SWIE_ENABLE  = 1U
} STK_Swie_t;

typedef struct {
    STK_ClkSrc_t  clk_src;      // STCLK — clock source selection
    STK_Intr_t    interrupt;    // STIE  — counter interrupt
    STK_Reload_t  reload;       // STRE  — auto-reload on compare
    STK_Swie_t    swie;         // SWIE  — software interrupt
} SysTick_InitTypeDef;


/* Basic free-running counter on HCLK/8 — no interrupts.
 * Good for: delays, timeouts, elapsed time measurement      */
static const SysTick_InitTypeDef STK_Config_FreeRunning = {
    STK_CLKSRC_HCLK_DIV8,   /* clk_src  */
    STK_INTR_DISABLE,        /* interrupt */
    STK_RELOAD_DISABLE,      /* reload    */
    STK_SWIE_DISABLE         /* swie      */
};

/* Periodic interrupt on HCLK/8 — set compare value first.
 * Good for: RTOS tick, periodic callbacks                   */
static const SysTick_InitTypeDef STK_Config_Periodic = {
    STK_CLKSRC_HCLK_DIV8,   /* clk_src  */
    STK_INTR_ENABLE,         /* interrupt */
    STK_RELOAD_ENABLE,       /* reload    */
    STK_SWIE_DISABLE         /* swie      */
};

/* High resolution free-running counter on HCLK direct.
 * Good for: tight timing measurements, profiling            */
static const SysTick_InitTypeDef STK_Config_HighRes = {
    STK_CLKSRC_HCLK,         /* clk_src  */
    STK_INTR_DISABLE,        /* interrupt */
    STK_RELOAD_DISABLE,      /* reload    */
    STK_SWIE_DISABLE         /* swie      */
};

static void SysTick_Init(const SysTick_InitTypeDef * const p_config)
{
    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();
    uint32_t ctlr                          = 0U;

    p_stk->ctlr = (uint32_t)(p_stk->ctlr & ~STK_CTLR_STE_MASK);

    p_stk->cntr_low = 0U;
    p_stk->cntr_hi  = 0U;

    ctlr = 0U;

    if (p_config->clk_src    == STK_CLKSRC_HCLK)    { ctlr = (uint32_t)(ctlr | STK_CTLR_STCLK_MASK); }
    if (p_config->interrupt  == STK_INTR_ENABLE)    { ctlr = (uint32_t)(ctlr | STK_CTLR_STIE_MASK);  }
    if (p_config->reload     == STK_RELOAD_ENABLE)  { ctlr = (uint32_t)(ctlr | STK_CTLR_STRE_MASK);  }
    if (p_config->swie       == STK_SWIE_ENABLE)    { ctlr = (uint32_t)(ctlr | STK_CTLR_SWIE_MASK);  }

    p_stk->ctlr = ctlr;
    p_stk->ctlr = (uint32_t)(p_stk->ctlr | STK_CTLR_STE_MASK);
}

static void SysTick_Init_FreeRunning(void) {
    SysTick_Init(&STK_Config_FreeRunning);
}

static void SysTick_Init_Periodic(void) {
    SysTick_Init(&STK_Config_Periodic);
}

static void SysTick_Init_HighRes(void) {
    SysTick_Init(&STK_Config_HighRes);
}

typedef struct {
    uint64_t start;
    uint64_t duration_ticks; 
} Timeout_t;

static Timeout_t Timeout_New_Ms(uint32_t ms) {
    Timeout_t t;

    t.start          = SysTick_CountValue();
    t.duration_ticks = SysTick_Ms_To_Ticks(ms);

    return t;
}

static Timeout_t Timeout_New_Us(uint32_t Us) {
    Timeout_t t;

    t.start          = SysTick_CountValue();
    t.duration_ticks = SysTick_Us_To_Ticks(Us);

    return t;
}

static uint32_t Timeout_Is_Expired(const Timeout_t * const p_timeout) {
    return (SysTicks_Elapsed(p_timeout->start) >= p_timeout->duration_ticks) ? 1U : 0U;
}

static uint64_t Timeout_Remaining_Ticks(const Timeout_t * const p_timeout) {
    uint64_t elapsed = 0U;
    uint64_t result  = 0U;

    elapsed = SysTicks_Elapsed(p_timeout->start);

    if (elapsed >= p_timeout->duration_ticks) {
        result = 0U;
    }
    else {
        result = p_timeout->duration_ticks - elapsed;
    }

    return result;
}

static uint64_t Timeout_Remaining_Us(const Timeout_t * const p_timeout) {
    return SysTick_Ticks_To_Us(Timeout_Remaining_Ticks(p_timeout));
}

static uint64_t Timeout_Remaining_Ms(const Timeout_t * const p_timeout) {
    return SysTick_Ticks_To_Ms(Timeout_Remaining_Ticks(p_timeout));
}

static void Timeout_Restart(Timeout_t * const p_timeout)
{
    p_timeout->start = SysTick_CountValue();
}

#endif // CH32V003_HAL_SYSTICK_H
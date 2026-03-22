/// pfic.h
///
/// PFIC (Programmable Fast Interrupt Controller) driver for CH32V003
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 6.5.2

#ifndef CH32V003_HAL_PFIC_H
#define CH32V003_HAL_PFIC_H

#include <stdint.h>

/*====================================================================
 * IRQ numbers — from manual Table 6-1
 * These are the interrupt numbers used with all PFIC functions.
 *====================================================================*/
typedef enum {
    IRQ_NMI        =  2U,
    IRQ_HARDFAULT  =  3U,
    IRQ_SYSTICK    = 12U,
    IRQ_SW         = 14U,
    IRQ_WWDG       = 16U,
    IRQ_PVD        = 17U,
    IRQ_FLASH      = 18U,
    IRQ_RCC        = 19U,
    IRQ_EXTI7_0    = 20U,
    IRQ_AWU        = 21U,
    IRQ_DMA1_CH1   = 22U,
    IRQ_DMA1_CH2   = 23U,
    IRQ_DMA1_CH3   = 24U,
    IRQ_DMA1_CH4   = 25U,
    IRQ_DMA1_CH5   = 26U,
    IRQ_DMA1_CH6   = 27U,
    IRQ_DMA1_CH7   = 28U,
    IRQ_ADC        = 29U,
    IRQ_I2C1_EV    = 30U,
    IRQ_I2C1_ER    = 31U,
    IRQ_USART1     = 32U,
    IRQ_SPI1       = 33U,
    IRQ_TIM1BRK    = 34U,
    IRQ_TIM1UP     = 35U,
    IRQ_TIM1TRG    = 36U,
    IRQ_TIM1CC     = 37U,
    IRQ_TIM2       = 38U
} IRQ_t;

/*====================================================================
 * Priority levels
 * Only bits [7:6] of the priority byte are used.
 * 0 = highest priority, 3 = lowest priority.
 *====================================================================*/
typedef enum {
    IRQ_PRIORITY_0 = 0x00U,   /* highest — preempts everything */
    IRQ_PRIORITY_1 = 0x40U,
    IRQ_PRIORITY_2 = 0x80U,
    IRQ_PRIORITY_3 = 0xC0U    /* lowest                        */
} IRQ_Priority_t;

/*====================================================================
 * PFIC register addresses
 * Large gaps between registers make a struct impractical.
 * Direct address macros are used instead.
 * Base: 0xE000E000
 *====================================================================*/
#define PFIC_BASE           (0xE000E000U)

/*--------------------------------------------------------------------
 * Status registers (read-only)
 * Read these to inspect current state. Cannot change anything.
 *------------------------------------------------------------------*/
/* ISR — interrupt enable status */
#define PFIC_ISR1    (*(volatile uint32_t *)(PFIC_BASE + 0x000U))
#define PFIC_ISR2    (*(volatile uint32_t *)(PFIC_BASE + 0x004U))

/* IPR — interrupt pending status */
#define PFIC_IPR1    (*(volatile uint32_t *)(PFIC_BASE + 0x020U))
#define PFIC_IPR2    (*(volatile uint32_t *)(PFIC_BASE + 0x024U))

/* IACTR — interrupt currently executing */
#define PFIC_IACTR1  (*(volatile uint32_t *)(PFIC_BASE + 0x300U))
#define PFIC_IACTR2  (*(volatile uint32_t *)(PFIC_BASE + 0x304U))

/* GISR — global nesting status */
#define PFIC_GISR    (*(volatile uint32_t *)(PFIC_BASE + 0x04CU))

/*--------------------------------------------------------------------
 * Write-only control registers
 * Write 1 to the bit of the interrupt you want to affect.
 * Writing 0 has NO effect — never read-modify-write these.
 *------------------------------------------------------------------*/
/* IENR — ENABLE an interrupt (write 1 to enable, 0 = no effect) */
#define PFIC_IENR1   (*(volatile uint32_t *)(PFIC_BASE + 0x100U))
#define PFIC_IENR2   (*(volatile uint32_t *)(PFIC_BASE + 0x104U))

/* IRER — DISABLE an interrupt (write 1 to disable, 0 = no effect) */
#define PFIC_IRER1   (*(volatile uint32_t *)(PFIC_BASE + 0x180U))
#define PFIC_IRER2   (*(volatile uint32_t *)(PFIC_BASE + 0x184U))

/* IPSR — SET an interrupt pending (manually trigger for testing) */
#define PFIC_IPSR1   (*(volatile uint32_t *)(PFIC_BASE + 0x200U))
#define PFIC_IPSR2   (*(volatile uint32_t *)(PFIC_BASE + 0x204U))

/* IPRR — CLEAR a pending interrupt without running the ISR */
#define PFIC_IPRR1   (*(volatile uint32_t *)(PFIC_BASE + 0x280U))
#define PFIC_IPRR2   (*(volatile uint32_t *)(PFIC_BASE + 0x284U))

/*--------------------------------------------------------------------
 * Priority and threshold registers (read-write)
 *------------------------------------------------------------------*/
/* IPRIORx — priority byte array, one byte per interrupt number */
#define PFIC_IPRIOR(n)  (*(volatile uint8_t *)(PFIC_BASE + 0x400U + (n)))

/* ITHRESDR — priority threshold (0 = disabled, filter by priority) */
#define PFIC_ITHRESDR   (*(volatile uint32_t *)(PFIC_BASE + 0x040U))

/*--------------------------------------------------------------------
 * System control
 *------------------------------------------------------------------*/
/* CFGR — requires security key in upper 16 bits for RESETSYS */
#define PFIC_CFGR       (*(volatile uint32_t *)(PFIC_BASE + 0x048U))
#define PFIC_CFGR_KEY3  (0xBEEF0000U)
#define PFIC_CFGR_RESETSYS_BIT  (7U)
#define PFIC_CFGR_RESETSYS_MASK ((uint32_t)(1UL << PFIC_CFGR_RESETSYS_BIT))

/* SCTLR — system control (sleep modes, wake-up config) */
#define PFIC_SCTLR      (*(volatile uint32_t *)(0xE000ED10U))

/*====================================================================
 * Internal helper — computes which register and which bit
 * for a given IRQ number.
 *
 * IRQ numbers 0-31  → register 1, bit = irq
 * IRQ numbers 32-38 → register 2, bit = irq - 32
 *====================================================================*/
static inline uint32_t pfic_bit(IRQ_t irq)
{
    uint32_t bit = (uint32_t)irq;
    if (bit >= 32U)
    {
        bit = bit - 32U;
    }
    return (uint32_t)(1UL << bit);
}

/*--------------------------------------------------------------------
 * PFIC_EnableIRQ — enable one interrupt in the PFIC
 *
 * This is layer 2 of the three-layer interrupt system:
 *   Layer 1: peripheral enable bit  (e.g. USART1->CTLR1 RXNEIE)
 *   Layer 2: PFIC enable            ← this function
 *   Layer 3: global enable          (IRQ_Enable() in core.h)
 *
 * Example: enable USART1 interrupt
 *   PFIC_EnableIRQ(IRQ_USART1);
 *------------------------------------------------------------------*/
static void PFIC_EnableIRQ(IRQ_t irq)
{
    if ((uint32_t)irq < 32U)
    {
        PFIC_IENR1 = pfic_bit(irq);
    }
    else
    {
        PFIC_IENR2 = pfic_bit(irq);
    }
}

/*--------------------------------------------------------------------
 * PFIC_DisableIRQ — disable one interrupt in the PFIC
 *
 * Example: disable USART1 interrupt
 *   PFIC_DisableIRQ(IRQ_USART1);
 *------------------------------------------------------------------*/
static void PFIC_DisableIRQ(IRQ_t irq)
{
    if ((uint32_t)irq < 32U)
    {
        PFIC_IRER1 = pfic_bit(irq);
    }
    else
    {
        PFIC_IRER2 = pfic_bit(irq);
    }
}

/*--------------------------------------------------------------------
 * PFIC_SetPriority — set the priority of an interrupt
 *
 * priority: IRQ_PRIORITY_0 (highest) through IRQ_PRIORITY_3 (lowest)
 *
 * Lower priority number = higher urgency = preempts higher numbers.
 * Two interrupts at the same priority level cannot preempt each other.
 *
 * Example: give USART1 higher priority than SysTick
 *   PFIC_SetPriority(IRQ_USART1, IRQ_PRIORITY_0);
 *   PFIC_SetPriority(IRQ_SYSTICK, IRQ_PRIORITY_2);
 *------------------------------------------------------------------*/
static void PFIC_SetPriority(IRQ_t irq, IRQ_Priority_t priority)
{
    PFIC_IPRIOR((uint32_t)irq) = (uint8_t)priority;
}

/*--------------------------------------------------------------------
 * PFIC_GetPriority — read current priority of an interrupt
 *------------------------------------------------------------------*/
static IRQ_Priority_t PFIC_GetPriority(IRQ_t irq)
{
    return (IRQ_Priority_t)PFIC_IPRIOR((uint32_t)irq);
}

/*--------------------------------------------------------------------
 * PFIC_IsEnabled — returns 1U if interrupt is enabled, 0U if not
 *
 * Reads the read-only ISR status registers.
 * Example: check USART1 is enabled before sending
 *   if (PFIC_IsEnabled(IRQ_USART1) != 0U) { ... }
 *------------------------------------------------------------------*/
static uint32_t PFIC_IsEnabled(IRQ_t irq)
{
    uint32_t result = 0U;
    uint32_t bit    = pfic_bit(irq);

    if ((uint32_t)irq < 32U)
    {
        if ((PFIC_ISR1 & bit) != 0U) { result = 1U; }
    }
    else
    {
        if ((PFIC_ISR2 & bit) != 0U) { result = 1U; }
    }

    return result;
}

/*--------------------------------------------------------------------
 * PFIC_IsPending — returns 1U if interrupt is pending (waiting)
 *
 * An interrupt is pending when the hardware has signalled it but
 * the ISR has not run yet.
 *------------------------------------------------------------------*/
static uint32_t PFIC_IsPending(IRQ_t irq)
{
    uint32_t result = 0U;
    uint32_t bit    = pfic_bit(irq);

    if ((uint32_t)irq < 32U)
    {
        if ((PFIC_IPR1 & bit) != 0U) { result = 1U; }
    }
    else
    {
        if ((PFIC_IPR2 & bit) != 0U) { result = 1U; }
    }

    return result;
}

/*--------------------------------------------------------------------
 * PFIC_ClearPending — clear a pending interrupt without running ISR
 *
 * Use when you want to discard an interrupt that fired before you
 * were ready to handle it.
 *------------------------------------------------------------------*/
static void PFIC_ClearPending(IRQ_t irq)
{
    if ((uint32_t)irq < 32U)
    {
        PFIC_IPRR1 = pfic_bit(irq);
    }
    else
    {
        PFIC_IPRR2 = pfic_bit(irq);
    }
}

/*--------------------------------------------------------------------
 * PFIC_SetPending — manually trigger an interrupt (testing only)
 *
 * Sets the pending bit as if the hardware fired the interrupt.
 * Useful for testing ISRs without real hardware events.
 *------------------------------------------------------------------*/
static void PFIC_SetPending(IRQ_t irq)
{
    if ((uint32_t)irq < 32U)
    {
        PFIC_IPSR1 = pfic_bit(irq);
    }
    else
    {
        PFIC_IPSR2 = pfic_bit(irq);
    }
}

/*--------------------------------------------------------------------
 * PFIC_SystemReset — trigger a system reset
 *
 * Requires writing KEY3 (0xBEEF) in the upper 16 bits simultaneously.
 * This function never returns.
 *------------------------------------------------------------------*/
static void PFIC_SystemReset(void)
{
    PFIC_CFGR = (uint32_t)(PFIC_CFGR_KEY3 | PFIC_CFGR_RESETSYS_MASK);

    /* Should never reach here */
    for (;;) {}
}

/*--------------------------------------------------------------------
 * PFIC_IsActive — returns 1U if interrupt ISR is currently executing
 *------------------------------------------------------------------*/
static uint32_t PFIC_IsActive(IRQ_t irq)
{
    uint32_t result = 0U;
    uint32_t bit    = pfic_bit(irq);

    if ((uint32_t)irq < 32U)
    {
        if ((PFIC_IACTR1 & bit) != 0U) { result = 1U; }
    }
    else
    {
        if ((PFIC_IACTR2 & bit) != 0U) { result = 1U; }
    }

    return result;
}
#endif
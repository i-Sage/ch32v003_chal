/// exti.h
///
/// External Interrupt (EXTI) driver for CH32V003
/// Configures GPIO pins as external interrupt sources.
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 6.4 and 6.5.1

#ifndef CH32V003_HAL_EXTI_H
#define CH32V003_HAL_EXTI_H

#include <stdint.h>
#include "gpio.h"
#include "pfic.h"

/*====================================================================
 * Register map — manual Table 6-3
 * Base address: 0x40010400
 *
 * Each register uses bits [9:0] — one bit per EXTI line.
 * Bit N = EXTI line N = GPIO pin N on whichever port AFIO selects.
 * Lines 8 and 9 are PVD and AWU events — not GPIO pins.
 *====================================================================*/
typedef struct {
    volatile uint32_t intenr;   /* 0x00 interrupt enable register          */
    volatile uint32_t evenr;    /* 0x04 event enable register              */
    volatile uint32_t rtenr;    /* 0x08 rising edge trigger enable         */
    volatile uint32_t ftenr;    /* 0x0C falling edge trigger enable        */
    volatile uint32_t swievr;   /* 0x10 software interrupt event register  */
    volatile uint32_t intfr;    /* 0x14 interrupt flag register            */
} EXTI_TypeDef;

/*
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Address 0x40010400 verified against CH32V003 manual
 *            Table 6-3.
 */
static inline volatile EXTI_TypeDef *EXTI_GetBase(void)
{
    return (volatile EXTI_TypeDef *)0x40010400U;
}

/*====================================================================
 * INTENR — Interrupt Enable Register (offset 0x00)
 * Reset value: 0x00000000
 *
 * Bits [9:0] MRx — one bit per EXTI line
 *   1: Interrupt enabled for this line
 *   0: Interrupt masked (disabled) for this line
 *====================================================================*/
#define EXTI_INTENR_MR_MASK(line)  ((uint32_t)(1UL << (line)))

/*====================================================================
 * EVENR — Event Enable Register (offset 0x04)
 * Reset value: 0x00000000
 *
 * Bits [9:0] MRx — one bit per EXTI line
 *   1: Event enabled for this line (used with WFE, not WFI)
 *   0: Event masked for this line
 *
 * Events wake the CPU from WFE without running an ISR.
 * Interrupts (INTENR) wake from WFI and run an ISR.
 * A line can have both enabled simultaneously.
 *====================================================================*/
#define EXTI_EVENR_MR_MASK(line)   ((uint32_t)(1UL << (line)))

/*====================================================================
 * RTENR — Rising Edge Trigger Enable Register (offset 0x08)
 * Reset value: 0x00000000
 *
 * Bits [9:0] TRx — one bit per EXTI line
 *   1: Rising edge (low→high) triggers this line
 *   0: Rising edge does not trigger this line
 *====================================================================*/
#define EXTI_RTENR_TR_MASK(line)   ((uint32_t)(1UL << (line)))

/*====================================================================
 * FTENR — Falling Edge Trigger Enable Register (offset 0x0C)
 * Reset value: 0x00000000
 *
 * Bits [9:0] TRx — one bit per EXTI line
 *   1: Falling edge (high→low) triggers this line
 *   0: Falling edge does not trigger this line
 *
 * Note: RTENR and FTENR can both be set on the same line —
 * this gives you "any edge" (both rising and falling) triggering.
 *====================================================================*/
#define EXTI_FTENR_TR_MASK(line)   ((uint32_t)(1UL << (line)))

/*====================================================================
 * SWIEVR — Software Interrupt Event Register (offset 0x10)
 * Reset value: 0x00000000
 *
 * Bits [9:0] SWIERx — one bit per EXTI line
 *   Writing 1: triggers that line as if the hardware edge occurred.
 *              Sets the corresponding INTFR flag.
 *              If INTENR is set for this line, an interrupt fires.
 *   Must be cleared by software — writing 1 to INTFR clears SWIEVR.
 *====================================================================*/
#define EXTI_SWIEVR_MASK(line)     ((uint32_t)(1UL << (line)))

/*====================================================================
 * INTFR — Interrupt Flag Register (offset 0x14)
 * Reset value: 0x0000XXXX (undefined at reset)
 *
 * Bits [9:0] IFx — one bit per EXTI line
 *   1: Interrupt or event occurred on this line
 *   0: No event on this line
 *
 * IMPORTANT — write 1 to CLEAR a flag (not write 0).
 * This is opposite to most registers.
 * Always clear the flag inside your ISR or it fires again immediately.
 *====================================================================*/
#define EXTI_INTFR_IF_MASK(line)   ((uint32_t)(1UL << (line)))

/*====================================================================
 * Edge trigger selection enum
 *====================================================================*/
typedef enum {
    EXTI_EDGE_RISING  = 0U,   /* trigger on low → high transition   */
    EXTI_EDGE_FALLING = 1U,   /* trigger on high → low transition   */
    EXTI_EDGE_BOTH    = 2U    /* trigger on any edge                 */
} EXTI_Edge_t;

/*====================================================================
 * API
 *====================================================================*/

/*--------------------------------------------------------------------
 * EXTI_ConfigLine — configure one EXTI line
 *
 * Sets the edge trigger and enables the interrupt for one GPIO pin.
 * Call after:
 *   1. GPIO_SetPinMode()   — configure pin as input
 *   2. AFIO_SetExtiPort()  — route this pin number to the correct port
 *
 * line:  GPIO pin number — same as EXTI line number (0–7)
 * edge:  EXTI_EDGE_RISING, EXTI_EDGE_FALLING, or EXTI_EDGE_BOTH
 *
 * Does NOT enable the PFIC interrupt — call PFIC_EnableIRQ separately.
 * All 8 GPIO lines share IRQ_EXTI7_0 in the PFIC.
 *------------------------------------------------------------------*/
static void EXTI_ConfigLine(GPIO_Pin_t pin, EXTI_Edge_t edge)
{
    volatile EXTI_TypeDef * const p_exti = EXTI_GetBase();
    uint32_t line = (uint32_t)pin;
    uint32_t mask = (uint32_t)(1UL << line);

    /* Clear any stale pending flag before enabling */
    p_exti->intfr = mask;

    /* Configure rising edge trigger */
    if ((edge == EXTI_EDGE_RISING) || (edge == EXTI_EDGE_BOTH))
    {
        p_exti->rtenr = (uint32_t)(p_exti->rtenr | mask);
    }
    else
    {
        p_exti->rtenr = (uint32_t)(p_exti->rtenr & ~mask);
    }

    /* Configure falling edge trigger */
    if ((edge == EXTI_EDGE_FALLING) || (edge == EXTI_EDGE_BOTH))
    {
        p_exti->ftenr = (uint32_t)(p_exti->ftenr | mask);
    }
    else
    {
        p_exti->ftenr = (uint32_t)(p_exti->ftenr & ~mask);
    }

    /* Enable interrupt for this line */
    p_exti->intenr = (uint32_t)(p_exti->intenr | mask);
}

/*--------------------------------------------------------------------
 * EXTI_DisableLine — disable interrupt for one EXTI line
 *
 * Stops the line from generating interrupts.
 * Does not affect the PFIC — use PFIC_DisableIRQ to disable the
 * entire EXTI7_0 channel if no lines are needed.
 *------------------------------------------------------------------*/
static void EXTI_DisableLine(GPIO_Pin_t pin)
{
    volatile EXTI_TypeDef * const p_exti = EXTI_GetBase();
    uint32_t mask = (uint32_t)(1UL << (uint32_t)pin);

    p_exti->intenr = (uint32_t)(p_exti->intenr & ~mask);
    p_exti->rtenr  = (uint32_t)(p_exti->rtenr  & ~mask);
    p_exti->ftenr  = (uint32_t)(p_exti->ftenr  & ~mask);
}

/*--------------------------------------------------------------------
 * EXTI_ClearFlag — clear the interrupt flag for one line
 *
 * MUST be called inside your ISR for every line that fired.
 * If you do not clear the flag the ISR fires again immediately
 * after returning.
 *
 * Write 1 to clear — this is opposite to most registers.
 *------------------------------------------------------------------*/
static void EXTI_ClearFlag(GPIO_Pin_t pin)
{
    volatile EXTI_TypeDef * const p_exti = EXTI_GetBase();

    /* Write 1 to clear the flag for this line */
    p_exti->intfr = (uint32_t)(1UL << (uint32_t)pin);
}

/*--------------------------------------------------------------------
 * EXTI_GetFlag — returns 1U if a flag is set, 0U if not
 *
 * Use inside your ISR to identify which pin triggered.
 * All 8 pins share one ISR — you must check each one.
 *------------------------------------------------------------------*/
static uint32_t EXTI_GetFlag(GPIO_Pin_t pin)
{
    volatile EXTI_TypeDef * const p_exti = EXTI_GetBase();
    uint32_t mask   = (uint32_t)(1UL << (uint32_t)pin);
    uint32_t result = 0U;

    if ((p_exti->intfr & mask) != 0U)
    {
        result = 1U;
    }

    return result;
}

/*--------------------------------------------------------------------
 * EXTI_SoftwareTrigger — manually fire an EXTI line from software
 *
 * Behaves exactly as if the hardware edge occurred.
 * Useful for testing your ISR without physical hardware.
 * INTENR must be enabled for this line for an interrupt to fire.
 * The flag is cleared automatically when INTFR is written in the ISR.
 *------------------------------------------------------------------*/
static void EXTI_SoftwareTrigger(GPIO_Pin_t pin)
{
    volatile EXTI_TypeDef * const p_exti = EXTI_GetBase();

    p_exti->swievr = (uint32_t)(p_exti->swievr |
                     (uint32_t)(1UL << (uint32_t)pin));
}

#endif /* CH32V003_HAL_EXTI_H */
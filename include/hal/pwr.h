/// pwr.h
///
/// Power Control (PWR) driver for CH32V003
/// Provides: PVD voltage monitoring, Sleep mode, Standby mode, Auto-wakeup (AWU)
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 2
 
#ifndef CH32V003_HAL_PWR_H
#define CH32V003_HAL_PWR_H
 
#include <stdint.h>
#include "rcc.h"
#include "core.h"
#include "pfic.h"
 
/*====================================================================
 * Register map — CH32V003 manual Table 2-2
 * Base address: 0x40007000
 *====================================================================*/
typedef struct {
    volatile uint32_t ctlr;     /* 0x00 PWR_CTLR   power control register              */
    volatile uint32_t csr;      /* 0x04 PWR_CSR    power control/status register        */
    volatile uint32_t awucsr;   /* 0x08 PWR_AWUCSR auto-wakeup control/status register  */
    volatile uint32_t awuwr;    /* 0x0C PWR_AWUWR  auto-wakeup window compare register  */
    volatile uint32_t awupsc;   /* 0x10 PWR_AWUPSC auto-wakeup prescaler register       */
} PWR_TypeDef;
 
/*
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Address 0x40007000 verified against CH32V003 manual
 *            Table 2-2.
 */
static inline volatile PWR_TypeDef *PWR_GetBase(void)
{
    return (volatile PWR_TypeDef *)0x40007000U;
}
 

/*====================================================================
 * PWR_CTLR — Power Control Register (offset 0x00)
 * CH32V003 manual section 2.4.1
 * Reset value: 0x00000000
 * Note: This register is RESET when waking up from Standby mode.
 *====================================================================*/
 
// Bits [7:5] PLS[2:0] — PVD voltage threshold selection
//   Sets the voltage level compared against VDD by the PVD.
//   The rising and falling thresholds differ by ~200mV hysteresis.
//
//   000: 2.85V rising / 2.70V falling
//   001: 3.05V rising / 2.90V falling
//   010: 3.30V rising / 3.15V falling
//   011: 3.50V rising / 3.30V falling
//   100: 3.70V rising / 3.50V falling
//   101: 3.90V rising / 3.70V falling
//   110: 4.10V rising / 3.90V falling
//   111: 4.40V rising / 4.20V falling
#define PWR_CTLR_PLS_BIT         (5U)
#define PWR_CTLR_PLS_MASK        ((uint32_t)(0x7UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_2V85        ((uint32_t)(0x0UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_3V05        ((uint32_t)(0x1UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_3V30        ((uint32_t)(0x2UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_3V50        ((uint32_t)(0x3UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_3V70        ((uint32_t)(0x4UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_3V90        ((uint32_t)(0x5UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_4V10        ((uint32_t)(0x6UL << PWR_CTLR_PLS_BIT))
#define PWR_CTLR_PLS_4V40        ((uint32_t)(0x7UL << PWR_CTLR_PLS_BIT))

// Bit 4 PVDE — PVD enable
//   1: PVD enabled — hardware monitors VDD against PLS threshold
//   0: PVD disabled
#define PWR_CTLR_PVDE_BIT        (4U)
#define PWR_CTLR_PVDE_MASK       ((uint32_t)(1UL << PWR_CTLR_PVDE_BIT))
 
// Bit 1 PDDS — power-down deep sleep mode selection
//   Selects which low-power mode is entered when SLEEPDEEP=1.
//   1: Standby mode — all clocks off, voltage regulator off
//   0: Sleep mode   — CPU clock off, peripherals still running
//   Note: SLEEPDEEP is in PFIC_SCTLR bit 2, not in this register.
#define PWR_CTLR_PDDS_BIT        (1U)
#define PWR_CTLR_PDDS_MASK       ((uint32_t)(1UL << PWR_CTLR_PDDS_BIT))
 
/*====================================================================
 * PWR_CSR — Power Control/Status Register (offset 0x04)
 * CH32V003 manual section 2.4.2
 * Reset value: 0x00000000
 * Note: This register is NOT reset when waking up from Standby mode.
 *====================================================================*/
 
// Bit 2 PVD0 — PVD output flag (read-only)
//   Only valid when PVDE=1 in PWR_CTLR.
//   1: VDD is BELOW the PVD threshold — power is low (danger)
//   0: VDD is ABOVE the PVD threshold — power is normal
#define PWR_CSR_PVD0_BIT         (2U)
#define PWR_CSR_PVD0_MASK        ((uint32_t)(1UL << PWR_CSR_PVD0_BIT))

/*====================================================================
 * PWR_AWUCSR — Auto-wakeup Control/Status Register (offset 0x08)
 * CH32V003 manual section 2.4.3
 * Reset value: 0x00000000
 *====================================================================*/
 
// Bit 1 AWUEN — auto-wakeup enable
//   1: Auto-wakeup enabled — LSI oscillator drives periodic wakeup
//   0: Auto-wakeup disabled
#define PWR_AWUCSR_AWUEN_BIT     (1U)
#define PWR_AWUCSR_AWUEN_MASK    ((uint32_t)(1UL << PWR_AWUCSR_AWUEN_BIT))
 
/*====================================================================
 * PWR_AWUWR — Auto-wakeup Window Comparison Register (offset 0x0C)
 * CH32V003 manual section 2.4.4
 * Reset value: 0x0000003F (63 — maximum)
 *
 * Bits [5:0] AWUWR[5:0] — 6-bit window value
 *   The internal LSI counter counts up and generates a wakeup
 *   event when it matches this value.
 *   Valid range: 1 to 63.
 *
 * Wake interval formula:
 *   interval_seconds = AWUWR × prescaler_divider / 128000
 *
 * Where 128000 is the LSI frequency in Hz.
 * 
 *====================================================================*/
#define PWR_AWUWR_MASK           ((uint32_t)(0x3FU))
 

/*====================================================================
 * PWR_AWUPSC — Auto-wakeup Prescaler Register (offset 0x10)
 * CH32V003 manual section 2.4.5
 * Reset value: 0x00000000
 *
 * Bits [3:0] AWUPSC[3:0] — LSI clock divider
 *   Divides the 128KHz LSI before the counter.
 *   0000/0001: prescaler off (do not use — no counting occurs)
 *   0010: ÷2       → effective rate 64000 Hz
 *   0011: ÷4       → effective rate 32000 Hz
 *   0100: ÷8       → effective rate 16000 Hz
 *   0101: ÷16      → effective rate  8000 Hz
 *   0110: ÷32      → effective rate  4000 Hz
 *   0111: ÷64      → effective rate  2000 Hz
 *   1000: ÷128     → effective rate  1000 Hz
 *   1001: ÷256     → effective rate   500 Hz
 *   1010: ÷512     → effective rate   250 Hz
 *   1011: ÷1024    → effective rate   125 Hz
 *   1100: ÷2048    → effective rate    62.5 Hz
 *   1101: ÷4096    → effective rate    31.25 Hz
 *   1110: ÷10240   → effective rate    12.5 Hz
 *   1111: ÷61440   → effective rate     2.08 Hz
 * 
 *====================================================================*/
#define PWR_AWUPSC_MASK          ((uint32_t)(0xFU))


typedef enum {
    PWR_AWUPSC_DIV2     = 0x2U,
    PWR_AWUPSC_DIV4     = 0x3U,
    PWR_AWUPSC_DIV8     = 0x4U,
    PWR_AWUPSC_DIV16    = 0x5U,
    PWR_AWUPSC_DIV32    = 0x6U,
    PWR_AWUPSC_DIV64    = 0x7U,
    PWR_AWUPSC_DIV128   = 0x8U,
    PWR_AWUPSC_DIV256   = 0x9U,
    PWR_AWUPSC_DIV512   = 0xAU,
    PWR_AWUPSC_DIV1024  = 0xBU,
    PWR_AWUPSC_DIV2048  = 0xCU,
    PWR_AWUPSC_DIV4096  = 0xDU,
    PWR_AWUPSC_DIV10240 = 0xEU,
    PWR_AWUPSC_DIV61440 = 0xFU
} PWR_AWUPrescaler_t;
 
/*====================================================================
 * PFIC_SCTLR bits used by PWR
 * SLEEPDEEP lives in PFIC_SCTLR bit 2, not in PWR_CTLR.
 * PFIC_SCTLR is already defined in pfic.h — we reference it here.
 * 
 *====================================================================*/
#define PWR_SCTLR_SLEEPDEEP_BIT  (2U)
#define PWR_SCTLR_SLEEPDEEP_MASK ((uint32_t)(1UL << PWR_SCTLR_SLEEPDEEP_BIT))
 
/*====================================================================
 * PVD threshold enum — convenience wrapper over PLS field

 *====================================================================*/
typedef enum {
    PWR_PVD_2V85 = 0U,   /* 2.85V rising / 2.70V falling */
    PWR_PVD_3V05 = 1U,   /* 3.05V rising / 2.90V falling */
    PWR_PVD_3V30 = 2U,   /* 3.30V rising / 3.15V falling */
    PWR_PVD_3V50 = 3U,   /* 3.50V rising / 3.30V falling */
    PWR_PVD_3V70 = 4U,   /* 3.70V rising / 3.50V falling */
    PWR_PVD_3V90 = 5U,   /* 3.90V rising / 3.70V falling */
    PWR_PVD_4V10 = 6U,   /* 4.10V rising / 3.90V falling */
    PWR_PVD_4V40 = 7U    /* 4.40V rising / 4.20V falling */
} PWR_PVDThreshold_t;
 
/*====================================================================
 * API — PVD voltage monitoring
 *====================================================================*/
 
/*--------------------------------------------------------------------
 * PWR_EnablePVD — enable the programmable voltage detector
 *
 * threshold: voltage level to monitor (PWR_PVD_2V85 through _4V40)
 *
 * After enabling, read PWR_ReadPVD() to check current voltage status.
 * For interrupt-driven monitoring, configure EXTI line 8 separately
 * and enable IRQ_PVD in the PFIC — the PVD output is internally
 * connected to EXTI line 8 rising and falling edges.
 *
 * Must call RCC_Enable_Clock(RCC_PERIPH_PWR) before this.
 * 
 *------------------------------------------------------------------*/
static void PWR_EnablePVD(PWR_PVDThreshold_t threshold)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
    uint32_t ctlr_val                  = 0U;
 
    ctlr_val = (uint32_t)(p_pwr->ctlr & ~PWR_CTLR_PLS_MASK);
    ctlr_val = (uint32_t)(ctlr_val | ((uint32_t)threshold << PWR_CTLR_PLS_BIT));
    ctlr_val = (uint32_t)(ctlr_val | PWR_CTLR_PVDE_MASK);
    p_pwr->ctlr = ctlr_val;
}
 
/*--------------------------------------------------------------------
 * PWR_DisablePVD — disable the programmable voltage detector

 *------------------------------------------------------------------*/
static void PWR_DisablePVD(void)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
 
    p_pwr->ctlr = (uint32_t)(p_pwr->ctlr & ~PWR_CTLR_PVDE_MASK);
}
 
/*--------------------------------------------------------------------
 * PWR_ReadPVD — read current PVD output flag
 *
 * Returns 1U if VDD is BELOW the configured threshold (voltage low)
 * Returns 0U if VDD is ABOVE the configured threshold (voltage ok)
 *
 * Only valid after PWR_EnablePVD() has been called.
 * 
 *------------------------------------------------------------------*/
static uint32_t PWR_ReadPVD(void)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
    uint32_t result                    = 0U;
 
    if ((p_pwr->csr & PWR_CSR_PVD0_MASK) != 0U)
    {
        result = 1U;
    }
 
    return result;
}
 

/*--------------------------------------------------------------------
 * PWR_EnablePVD_IRQ — enable PVD interrupt via EXTI line 8
 *
 * The PVD output is internally wired to EXTI line 8. Configuring
 * EXTI line 8 with a rising and/or falling trigger generates an
 * interrupt whenever VDD crosses the PVD threshold.
 *
 * This function configures EXTI line 8 directly since it is not
 * a GPIO pin and AFIO routing is not required for it.
 *
 * EXTI line 8 = PVD event (manual Table 6-2)
 * IRQ slot 17 = PVD_IRQHandler (manual Table 6-1)
 * 
 *------------------------------------------------------------------*/
#define PWR_PVD_EXTI_LINE     (8U)
#define PWR_PVD_EXTI_MASK     ((uint32_t)(1UL << PWR_PVD_EXTI_LINE))
 
/* EXTI base — duplicated here to avoid circular includes with exti.h */
#define PWR_EXTI_BASE         (0x40010400U)
#define PWR_EXTI_INTENR       (*(volatile uint32_t *)(PWR_EXTI_BASE + 0x00U))
#define PWR_EXTI_RTENR        (*(volatile uint32_t *)(PWR_EXTI_BASE + 0x08U))
#define PWR_EXTI_FTENR        (*(volatile uint32_t *)(PWR_EXTI_BASE + 0x0CU))
#define PWR_EXTI_INTFR        (*(volatile uint32_t *)(PWR_EXTI_BASE + 0x14U))
 
/*--------------------------------------------------------------------
 * PWR_PVD interrupt trigger selection
 *
 * PWR_PVD_TRIGGER_LOW_CROSSING:
 *   Fires when VDD drops BELOW the threshold.
 *   This is the danger event — power is failing.
 *   Maps to RISING edge on EXTI line 8 (PVD output 0→1).
 *
 * PWR_PVD_TRIGGER_HIGH_CROSSING:
 *   Fires when VDD recovers ABOVE the threshold.
 *   This is the recovery event — power is restored.
 *   Maps to FALLING edge on EXTI line 8 (PVD output 1→0).
 *
 * PWR_PVD_TRIGGER_BOTH:
 *   Fires on both events — use when you want to track
 *   both the failure and the recovery.
 *   Inside the ISR read PWR_ReadPVD() to determine which
 *   direction the crossing was.
 * 
 *------------------------------------------------------------------*/
typedef enum {
    PWR_PVD_TRIGGER_LOW_CROSSING  = 0U,   /* VDD fell below threshold  */
    PWR_PVD_TRIGGER_HIGH_CROSSING = 1U,   /* VDD rose above threshold  */
    PWR_PVD_TRIGGER_BOTH          = 2U    /* either crossing           */
} PWR_PVDTrigger_t;

static void PWR_EnablePVD_IRQ(PWR_PVDTrigger_t trigger,
                               IRQ_Priority_t   priority)
{
    /* Clear any stale pending flag before configuring */
    PWR_EXTI_INTFR = PWR_PVD_EXTI_MASK;

    /* Clear both edge enables first — apply only what was requested */
    PWR_EXTI_RTENR = (uint32_t)(PWR_EXTI_RTENR & ~PWR_PVD_EXTI_MASK);
    PWR_EXTI_FTENR = (uint32_t)(PWR_EXTI_FTENR & ~PWR_PVD_EXTI_MASK);

    /* Rising edge  = VDD dropped below threshold (low crossing) */
    if ((trigger == PWR_PVD_TRIGGER_LOW_CROSSING) ||
        (trigger == PWR_PVD_TRIGGER_BOTH))
    {
        PWR_EXTI_RTENR = (uint32_t)(PWR_EXTI_RTENR | PWR_PVD_EXTI_MASK);
    }

    /* Falling edge = VDD recovered above threshold (high crossing) */
    if ((trigger == PWR_PVD_TRIGGER_HIGH_CROSSING) ||
        (trigger == PWR_PVD_TRIGGER_BOTH))
    {
        PWR_EXTI_FTENR = (uint32_t)(PWR_EXTI_FTENR | PWR_PVD_EXTI_MASK);
    }

    /* Enable interrupt on line 8 */
    PWR_EXTI_INTENR = (uint32_t)(PWR_EXTI_INTENR | PWR_PVD_EXTI_MASK);

    PFIC_SetPriority(IRQ_PVD, priority);
    PFIC_EnableIRQ(IRQ_PVD);
}

static void PWR_DisablePVD_IRQ(void)
{
    PFIC_DisableIRQ(IRQ_PVD);
    PWR_EXTI_INTENR = (uint32_t)(PWR_EXTI_INTENR & ~PWR_PVD_EXTI_MASK);
    PWR_EXTI_RTENR  = (uint32_t)(PWR_EXTI_RTENR  & ~PWR_PVD_EXTI_MASK);
    PWR_EXTI_FTENR  = (uint32_t)(PWR_EXTI_FTENR  & ~PWR_PVD_EXTI_MASK);
}
 
/* Clear the PVD EXTI flag — call inside PVD_IRQHandler */
static void PWR_ClearPVD_Flag(void)
{
    PWR_EXTI_INTFR = PWR_PVD_EXTI_MASK;   /* write 1 to clear */
}
 
/*====================================================================
 * API — Sleep mode
 *
 * Sleep mode stops the CPU clock while all peripherals keep running.
 * This is the lightest low-power mode — fastest wakeup, lowest impact.
 * Any enabled interrupt wakes the CPU.
 *
 * Power consumption: reduced (CPU halted) but not dramatically low.
 * Wakeup time: ~1 CPU clock cycle.
 *
 * Entry sequence from manual section 2.3.2:
 *   SLEEPDEEP = 0  (in PFIC_SCTLR)
 *   PDDS = 0       (in PWR_CTLR)
 *   execute WFI
 * 
 *====================================================================*/
static void PWR_EnterSleep(void)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
 
    /* PDDS = 0 — sleep mode (not standby) */
    p_pwr->ctlr = (uint32_t)(p_pwr->ctlr & ~PWR_CTLR_PDDS_MASK);
 
    /* SLEEPDEEP = 0 — light sleep (not deep sleep) */
    PFIC_SCTLR = (uint32_t)(PFIC_SCTLR & ~PWR_SCTLR_SLEEPDEEP_MASK);
 
    /* Execute WFI — CPU halts here until any interrupt fires */
    WFI();
 
    /* Execution resumes here after wakeup interrupt runs */
}
 
/*====================================================================
 * API — Standby mode
 *
 * Standby mode stops all clocks and turns off the voltage regulator.
 * Deepest low-power mode — lowest current, longest wakeup time.
 *
 * What is preserved:  SRAM contents, register contents, I/O pin states
 * What is turned off: HSE, HSI, PLL, all peripheral clocks, regulator
 *
 * After wakeup: clock switches to HSI, execution continues after WFI.
 * PWR_CTLR is RESET on wakeup — re-initialise if needed.
 *
 * Wakeup sources:
 *   - Any interrupt configured in EXTI
 *   - AWU event (if configured)
 *
 * IMPORTANT: Disable unused peripheral clocks before entering standby
 * to minimise leakage current. Also ensure USART/SPI are not mid-
 * transmission.
 *
 * Entry sequence from manual section 2.3.3:
 *   SLEEPDEEP = 1  (in PFIC_SCTLR)
 *   PDDS = 1       (in PWR_CTLR)
 *   execute WFI
 * 
 *====================================================================*/
static void PWR_EnterStandby(void)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
 
    /* PDDS = 1 — standby mode */
    p_pwr->ctlr = (uint32_t)(p_pwr->ctlr | PWR_CTLR_PDDS_MASK);
 
    /* SLEEPDEEP = 1 — deep sleep */
    PFIC_SCTLR = (uint32_t)(PFIC_SCTLR | PWR_SCTLR_SLEEPDEEP_MASK);
 
    /* Execute WFI — all clocks stop here */
    WFI();
 
    /* Execution resumes here after wakeup.
     * HSI is now the active clock. Re-call RCC_Init if 48MHz needed. */
 
    /* Clear SLEEPDEEP so a subsequent WFI does not re-enter standby */
    PFIC_SCTLR = (uint32_t)(PFIC_SCTLR & ~PWR_SCTLR_SLEEPDEEP_MASK);
}
 
/*====================================================================
 * API — Auto-wakeup (AWU)
 *
 * Generates a periodic wakeup from Standby mode without external
 * signals. Uses the internal 128KHz LSI oscillator.
 *
 * Wake interval formula:
 *   interval_s = (AWUWR × prescaler_divider) / 128000
 *
 * Prescaler divider values are defined in PWR_AWUPrescaler_t.
 *
 * Example intervals (AWUWR = 63, maximum):
 *   PWR_AWUPSC_DIV4096  → ~2.0 seconds
 *   PWR_AWUPSC_DIV1024  → ~0.5 seconds
 *   PWR_AWUPSC_DIV256   → ~0.12 seconds
 *   PWR_AWUPSC_DIV128   → ~62ms
 *
 * Example intervals (fine-tuned):
 *   PWR_AWUPSC_DIV4096, AWUWR=31  → ~1.0 second
 *   PWR_AWUPSC_DIV4096, AWUWR=16  → ~0.5 seconds
 *
 * Must call PWR_EnterStandby() after configuring AWU.
 * AWU fires IRQ_AWU (slot 21) — define AWU_IRQHandler to handle it.
 * 
 *====================================================================*/
static void PWR_ConfigAWU(PWR_AWUPrescaler_t prescaler, uint8_t window_value)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
    uint8_t clamped_window             = window_value;
 
    /* AWUWR valid range is 1-63 — clamp to prevent writing 0 */
    if (clamped_window == 0U)
    {
        clamped_window = 1U;
    }
    if (clamped_window > 63U)
    {
        clamped_window = 63U;
    }
 
    /* Set prescaler */
    p_pwr->awupsc = (uint32_t)prescaler & PWR_AWUPSC_MASK;
 
    /* Set window comparison value */
    p_pwr->awuwr = (uint32_t)clamped_window & PWR_AWUWR_MASK;
 
    /* Enable AWU */
    p_pwr->awucsr = (uint32_t)(p_pwr->awucsr | PWR_AWUCSR_AWUEN_MASK);
}
 
static void PWR_DisableAWU(void)
{
    volatile PWR_TypeDef * const p_pwr = PWR_GetBase();
 
    p_pwr->awucsr = (uint32_t)(p_pwr->awucsr & ~PWR_AWUCSR_AWUEN_MASK);
}
 

#endif
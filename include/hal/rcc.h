/*

THE CH32V003 CLOCK TREE

We can think of the clock tree as a pipeline with three stages

[CLOCK SOURCES] -> [PLL] -> [SW MUX] -> SYSCLK

CLOCK SOURCES:
    We have 3 (well 2 really) clock sources to choose from:
    - 128kHz  LSI -> This is always on, and is used for the IWDG, PWR and GPIO
    - 24MHz   HSI -> Internal RC oscillator, this is on by default at reset
    - 4-25MHz HSE -> External  crystal oscillator, off by defualt.

PLL:
    The phase locked loop only job is to multiply the input by 2.
    i.e if HSI is the chosen clock source, HS1 * 2 = 48MHz
    i.e if HSE is the chosen clock source at 8MHz, then 2 * 8MHz is 16MHz

    The PLL clock source is selected by PLLSRC in RCC_CFGR0. This MUST BE CONFIGURED
    BEFORE TURNING PLL ON - once the PLL is on and locked, writes are ignored.

SW MUX:
    This selects what becomes SYSCLK
    - SW = 00 -> HSI direct (24 MHz at reset, default)
    - SW = 01 -> HSE direct
    - SW = 10 -> PLL output (48Mhz if HSI*2)

    The SWS bits tell us what clock source is used for SYSCLK, its a read only register
    and its good practice to always poll to confirm that the switch happened, never 
    just trust SW

SYSCLK then goes through the HPRE prescaler before becoming HCLK which feeds all
peripherals. The default value for HPRE is 0010b which means -3, so at reset:
- SYSCLK = 24MHz HSI
- HCLK   = 24MHz / 3 = 8MHz (the peripherals run at 8MHz by default)



[HSI 24MHz] ──┐
              ├──> PLLSRC ──> PLL (×2) ──┐
[HSE 4-25MHz]─┘                          │
                                     SW mux ──> SYSCLK
[HSI 24MHz] ─────────────────────────────┘
                                          │
                                    HPRE prescaler (÷1..÷256)
                                          │
                                        HCLK ──> all peripherals
                                          │
                                       ÷8 divider (or direct if STCLK=1)
                                          │
                                       SysTick clock
*/

#ifndef CH32V003_HAL_RCC_H
#define CH32V003_HAL_RCC_H

#include <stdint.h>

//                          Clock Control Register RCC_CTLR Bits
/* Clock Control Register 0 (RCC_CTLR) — offset 0x00
 * CH32V003 reference manual section 3.4.1
 *--------------------------------------------------------------------*/

// Bit 0 HSION — internal high-speed clock (24MHz) enable
//   1: Enable the HSI oscillator
//   0: Disable the HSI oscillator
//   Note: Set to 1 by hardware when returning from standby or when
//         HSE fails as system clock source
#define RCC_CTLR_HSION_BIT        (0U)
#define RCC_CTLR_HSION_MASK       ((uint32_t)(1UL << RCC_CTLR_HSION_BIT))

// Bit 1 HSIRDY — internal high-speed clock stable ready flag (hardware set)
//   1: HSI clock (24MHz) is stable
//   0: HSI clock (24MHz) is not stable
//   Note: Takes 6 HSI cycles to clear after HSION is cleared to 0
#define RCC_CTLR_HSIRDY_BIT       (1U)
#define RCC_CTLR_HSIRDY_MASK      ((uint32_t)(1UL << RCC_CTLR_HSIRDY_BIT))

// Bit 2 — Reserved, read-only, do not write

// Bits [7:3] HSITRIM[4:0] — internal HSI clock adjustment value
//   User-adjustable trim to superimpose on HSICAL.
//   Default value 16 adjusts HSI to 24MHz ±1%.
//   Each step adjusts HSICAL by approximately 60kHz.
#define RCC_CTLR_HSITRIM_BIT      (3U)
#define RCC_CTLR_HSITRIM_MASK     ((uint32_t)(0x1FUL << RCC_CTLR_HSITRIM_BIT))

// Bits [15:8] HSICAL[7:0] — internal HSI clock calibration (read-only)
//   Automatically initialized at system startup.
//   Combined with HSITRIM to set the final HSI frequency.
#define RCC_CTLR_HSICAL_BIT       (8U)
#define RCC_CTLR_HSICAL_MASK      ((uint32_t)(0xFFUL << RCC_CTLR_HSICAL_BIT))

// Bit 16 HSEON — external high-speed crystal oscillation enable
//   1: Enable the HSE oscillator
//   0: Turn off the HSE oscillator
//   Note: Cleared to 0 by hardware after entering standby low-power mode
#define RCC_CTLR_HSEON_BIT        (16U)
#define RCC_CTLR_HSEON_MASK       ((uint32_t)(1UL << RCC_CTLR_HSEON_BIT))

// Bit 17 HSERDY — external high-speed crystal stabilization ready flag (hardware set)
//   1: External high-speed crystal oscillation is stable
//   0: External high-speed crystal oscillation is not stabilized
//   Note: Takes 6 HSE cycles to clear after HSEON is cleared to 0
#define RCC_CTLR_HSERDY_BIT       (17U)
#define RCC_CTLR_HSERDY_MASK      ((uint32_t)(1UL << RCC_CTLR_HSERDY_BIT))

// Bit 18 HSEBYP — external high-speed crystal bypass control
//   1: Bypass external crystal/ceramic resonators (use external clock source directly)
//   0: No bypass — use external crystal/ceramic resonator normally
//   Note: Must be written with HSEON at 0
#define RCC_CTLR_HSEBYP_BIT       (18U)
#define RCC_CTLR_HSEBYP_MASK      ((uint32_t)(1UL << RCC_CTLR_HSEBYP_BIT))

// Bit 19 CSSON — clock security system enable
//   1: Enable clock security system. When HSE is ready (HSERDY=1),
//      hardware monitors HSE and triggers CSSF flag + NMI interrupt
//      if HSE becomes abnormal. Hardware disables clock monitoring
//      if HSE is not ready.
//   0: Turn off the clock security system
#define RCC_CTLR_CSSON_BIT        (19U)
#define RCC_CTLR_CSSON_MASK       ((uint32_t)(1UL << RCC_CTLR_CSSON_BIT))

// Bits [23:20] — Reserved, read-only, do not write

// Bit 24 PLLON — PLL clock enable
//   1: Enable the PLL clock
//   0: Turn off the PLL clock
//   Note: Cleared by hardware to 0 after entering standby low-power mode
#define RCC_CTLR_PLLON_BIT        (24U)
#define RCC_CTLR_PLLON_MASK       ((uint32_t)(1UL << RCC_CTLR_PLLON_BIT))

// Bit 25 PLLRDY — PLL clock ready lock flag (read-only, hardware set)
//   1: PLL clock is locked and stable
//   0: PLL clock is not locked
#define RCC_CTLR_PLLRDY_BIT       (25U)
#define RCC_CTLR_PLLRDY_MASK      ((uint32_t)(1UL << RCC_CTLR_PLLRDY_BIT))

// Bits [31:26] — Reserved, read-only, do not write



/* Clock Configuration Register 0 (RCC_CFGR0) — offset 0x04
 * CH32V003 reference manual section 3.4.2
 *--------------------------------------------------------------------*/

// Bits [1:0] SW[1:0] — system clock source select (RW)
//   00: HSI as system clock
//   01: HSE as system clock
//   10: PLL output as system clock
//   11: Not available
//   Note: With CSSON=1, hardware forces HSI on return from Standby/Stop
//         or when HSE fails
#define RCC_CFGR0_SW_BIT              (0U)
#define RCC_CFGR0_SW_MASK             ((uint32_t)(0x3UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_HSI              ((uint32_t)(0x0UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_HSE              ((uint32_t)(0x1UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_PLL              ((uint32_t)(0x2UL << RCC_CFGR0_SW_BIT))

// Bits [3:2] SWS[1:0] — system clock status (RO, hardware set)
//   00: System clock source is HSI
//   01: System clock source is HSE
//   10: System clock source is PLL
//   11: Not available
#define RCC_CFGR0_SWS_BIT             (2U)
#define RCC_CFGR0_SWS_MASK            ((uint32_t)(0x3UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_HSI             ((uint32_t)(0x0UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_HSE             ((uint32_t)(0x1UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_PLL             ((uint32_t)(0x2UL << RCC_CFGR0_SWS_BIT))

// Bits [7:4] HPRE[3:0] — AHB (HCLK) prescaler (RW)
//   0000: Prescaler off (SYSCLK not divided)
//   0001: SYSCLK / 2
//   0010: SYSCLK / 3
//   0011: SYSCLK / 4
//   0100: SYSCLK / 5
//   0101: SYSCLK / 6
//   0110: SYSCLK / 7
//   0111: SYSCLK / 8
//   1000: SYSCLK / 2
//   1001: SYSCLK / 4
//   1010: SYSCLK / 8
//   1011: SYSCLK / 16
//   1100: SYSCLK / 32
//   1101: SYSCLK / 64
//   1110: SYSCLK / 128
//   1111: SYSCLK / 256
//   Reset value: 0010b (SYSCLK / 3 = 8MHz at reset)
#define RCC_CFGR0_HPRE_BIT            (4U)
#define RCC_CFGR0_HPRE_MASK           ((uint32_t)(0xFUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV1           ((uint32_t)(0x0UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV2           ((uint32_t)(0x1UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV3           ((uint32_t)(0x2UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV4           ((uint32_t)(0x3UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV5           ((uint32_t)(0x4UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV6           ((uint32_t)(0x5UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV7           ((uint32_t)(0x6UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV8           ((uint32_t)(0x7UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV16          ((uint32_t)(0xBUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV32          ((uint32_t)(0xCUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV64          ((uint32_t)(0xDUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV128         ((uint32_t)(0xEUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV256         ((uint32_t)(0xFUL << RCC_CFGR0_HPRE_BIT))

// Bits [10:8] — Reserved RW, do not modify

// Bits [15:11] ADCPRE[4:0] — ADC clock prescaler {bits 13:11, 15:14} (RW)
//   Note: ADC clock must not exceed 24MHz
//   000xx: HBCLK / 2
//   010xx: HBCLK / 4
//   100xx: HBCLK / 6
//   110xx: HBCLK / 8
//   00100: HBCLK / 4    01100: HBCLK / 16   10100: HBCLK / 24
//   00101: HBCLK / 8    01101: HBCLK / 16   10101: HBCLK / 32
//   (see manual Table for full mapping)
#define RCC_CFGR0_ADCPRE_BIT          (11U)
#define RCC_CFGR0_ADCPRE_MASK         ((uint32_t)(0x1FUL << RCC_CFGR0_ADCPRE_BIT))

// Bit 16 PLLSRC — PLL input clock source (RW, write only when PLL is off)
//   1: HSE fed into PLL without dividing
//   0: HSI fed into PLL without dividing
#define RCC_CFGR0_PLLSRC_BIT          (16U)
#define RCC_CFGR0_PLLSRC_MASK         ((uint32_t)(1UL << RCC_CFGR0_PLLSRC_BIT))
#define RCC_CFGR0_PLLSRC_HSI          ((uint32_t)(0x0UL << RCC_CFGR0_PLLSRC_BIT))
#define RCC_CFGR0_PLLSRC_HSE          ((uint32_t)(1UL   << RCC_CFGR0_PLLSRC_BIT))

// Bits [23:17] — Reserved RO, do not modify

// Bits [26:24] MCO[2:0] — MCO pin clock output control (RW)
//   0xx: No clock output
//   100: SYSCLK output
//   101: Internal 24MHz HSI RC oscillator output
//   110: HSE oscillator output
//   111: PLL clock output
#define RCC_CFGR0_MCO_BIT             (24U)
#define RCC_CFGR0_MCO_MASK            ((uint32_t)(0x7UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_NONE            ((uint32_t)(0x0UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_SYSCLK          ((uint32_t)(0x4UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_HSI             ((uint32_t)(0x5UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_HSE             ((uint32_t)(0x6UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_PLL             ((uint32_t)(0x7UL << RCC_CFGR0_MCO_BIT))

// Bits [31:27] — Reserved RO, do not modify


/* Clock Interrupt Register (RCC_INTR) — offset 0x08
 * CH32V003 reference manual section 3.4.3
 *
 * Three groups of bits per clock source:
 *   xRDYF  — flag    (RO): hardware sets when clock becomes ready
 *   xRDYIE — enable  (RW): allow flag to generate an interrupt
 *   xRDYC  — clear   (WO): write 1 to clear the flag
 *--------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * Bits [7:0] — Interrupt flag bits (read-only, hardware set)
 *-----------------------------------------------------------------*/

// Bit 0 LSIRDYF — LSI clock-ready interrupt flag (RO)
//   1: LSI clock-ready interrupt generated
//   0: No LSI clock-ready interrupt
//   Clear by writing LSIRDYC = 1
#define RCC_INTR_LSIRDYF_BIT         (0U)
#define RCC_INTR_LSIRDYF_MASK        ((uint32_t)(1UL << RCC_INTR_LSIRDYF_BIT))

// Bit 1 — Reserved, read-only, do not write

// Bit 2 HSIRDYF — HSI clock-ready interrupt flag (RO)
//   1: HSI clock-ready interrupt generated
//   0: No HSI clock-ready interrupt
//   Clear by writing HSIRDYC = 1
#define RCC_INTR_HSIRDYF_BIT         (2U)
#define RCC_INTR_HSIRDYF_MASK        ((uint32_t)(1UL << RCC_INTR_HSIRDYF_BIT))

// Bit 3 HSERDYF — HSE clock-ready interrupt flag (RO)
//   1: HSE clock-ready interrupt generated
//   0: No HSE clock-ready interrupt
//   Clear by writing HSERDYC = 1
#define RCC_INTR_HSERDYF_BIT         (3U)
#define RCC_INTR_HSERDYF_MASK        ((uint32_t)(1UL << RCC_INTR_HSERDYF_BIT))

// Bit 4 PLLRDYF — PLL clock-ready lockout interrupt flag (RO)
//   1: PLL clock lock generating interrupt
//   0: No PLL clock lock interrupt
//   Clear by writing PLLRDYC = 1
#define RCC_INTR_PLLRDYF_BIT         (4U)
#define RCC_INTR_PLLRDYF_MASK        ((uint32_t)(1UL << RCC_INTR_PLLRDYF_BIT))

// Bits [6:5] — Reserved, read-only, do not write

// Bit 7 CSSF — clock security system interrupt flag (RO)
//   1: HSE clock failure detected, clock safety interrupt CSSI generated
//   0: No clock security system interrupt
//   Hardware set, clear by writing CSSC = 1
#define RCC_INTR_CSSF_BIT            (7U)
#define RCC_INTR_CSSF_MASK           ((uint32_t)(1UL << RCC_INTR_CSSF_BIT))

/*------------------------------------------------------------------
 * Bits [12:8] — Interrupt enable bits (read-write)
 *-----------------------------------------------------------------*/

// Bit 8 LSIRDYIE — LSI-ready interrupt enable (RW)
//   1: Enable LSI-ready interrupt
//   0: Disable LSI-ready interrupt
#define RCC_INTR_LSIRDYIE_BIT        (8U)
#define RCC_INTR_LSIRDYIE_MASK       ((uint32_t)(1UL << RCC_INTR_LSIRDYIE_BIT))

// Bit 9 — Reserved, read-only, do not write

// Bit 10 HSIRDYIE — HSI-ready interrupt enable (RW)
//   1: Enable HSI-ready interrupt
//   0: Disable HSI-ready interrupt
#define RCC_INTR_HSIRDYIE_BIT        (10U)
#define RCC_INTR_HSIRDYIE_MASK       ((uint32_t)(1UL << RCC_INTR_HSIRDYIE_BIT))

// Bit 11 HSERDYIE — HSE-ready interrupt enable (RW)
//   1: Enable HSE-ready interrupt
//   0: Disable HSE-ready interrupt
#define RCC_INTR_HSERDYIE_BIT        (11U)
#define RCC_INTR_HSERDYIE_MASK       ((uint32_t)(1UL << RCC_INTR_HSERDYIE_BIT))

// Bit 12 PLLRDYIE — PLL-ready interrupt enable (RW)
//   1: Enable PLL-ready interrupt
//   0: Disable PLL-ready interrupt
#define RCC_INTR_PLLRDYIE_BIT        (12U)
#define RCC_INTR_PLLRDYIE_MASK       ((uint32_t)(1UL << RCC_INTR_PLLRDYIE_BIT))

// Bits [15:13] — Reserved, read-only, do not write

/*------------------------------------------------------------------
 * Bits [23:16] — Interrupt clear bits (write-only)
 * Write 1 to clear the corresponding flag. Write 0 has no effect.
 *-----------------------------------------------------------------*/

// Bit 16 LSIRDYC — clear LSI oscillator ready interrupt flag (WO)
//   1: Clear LSIRDYF interrupt flag
//   0: No action
#define RCC_INTR_LSIRDYC_BIT         (16U)
#define RCC_INTR_LSIRDYC_MASK        ((uint32_t)(1UL << RCC_INTR_LSIRDYC_BIT))

// Bit 17 — Reserved, read-only, do not write

// Bit 18 HSIRDYC — clear HSI oscillator ready interrupt flag (WO)
//   1: Clear HSIRDYF interrupt flag
//   0: No action
#define RCC_INTR_HSIRDYC_BIT         (18U)
#define RCC_INTR_HSIRDYC_MASK        ((uint32_t)(1UL << RCC_INTR_HSIRDYC_BIT))

// Bit 19 HSERDYC — clear HSE oscillator ready interrupt flag (WO)
//   1: Clear HSERDYF interrupt flag
//   0: No action
#define RCC_INTR_HSERDYC_BIT         (19U)
#define RCC_INTR_HSERDYC_MASK        ((uint32_t)(1UL << RCC_INTR_HSERDYC_BIT))

// Bit 20 PLLRDYC — clear PLL-ready interrupt flag (WO)
//   1: Clear PLLRDYF interrupt flag
//   0: No action
#define RCC_INTR_PLLRDYC_BIT         (20U)
#define RCC_INTR_PLLRDYC_MASK        ((uint32_t)(1UL << RCC_INTR_PLLRDYC_BIT))

// Bits [22:21] — Reserved, read-only, do not write

// Bit 23 CSSC — clear clock security system interrupt flag (WO)
//   1: Clear CSSF interrupt flag
//   0: No action
#define RCC_INTR_CSSC_BIT            (23U)
#define RCC_INTR_CSSC_MASK           ((uint32_t)(1UL << RCC_INTR_CSSC_BIT))

// Bits [31:24] — Reserved, read-only, do not write

/*====================================================================
 * PB2 Peripheral Reset Register (RCC_APB2PRSTR) — offset 0x0C
 * CH32V003 reference manual section 3.4.4
 * Write 1 to reset the module. Write 0 for no effect.
 *====================================================================*/

// Bit 0 AFIORST — I/O auxiliary function module reset
#define RCC_APB2PRSTR_AFIORST_BIT      (0U)
#define RCC_APB2PRSTR_AFIORST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_AFIORST_BIT))

// Bit 1 — Reserved, read-only, do not write

// Bit 2 IOPARST — PA port module reset for I/O
#define RCC_APB2PRSTR_IOPARST_BIT      (2U)
#define RCC_APB2PRSTR_IOPARST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_IOPARST_BIT))

// Bit 3 — Reserved, read-only, do not write

// Bit 4 IOPCRST — PC port module reset for I/O
#define RCC_APB2PRSTR_IOPCRST_BIT      (4U)
#define RCC_APB2PRSTR_IOPCRST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_IOPCRST_BIT))

// Bit 5 IOPDRST — PD port module reset for I/O
#define RCC_APB2PRSTR_IOPDRST_BIT      (5U)
#define RCC_APB2PRSTR_IOPDRST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_IOPDRST_BIT))

// Bits [8:6] — Reserved, read-only, do not write

// Bit 9 ADC1RST — ADC1 module reset
#define RCC_APB2PRSTR_ADC1RST_BIT      (9U)
#define RCC_APB2PRSTR_ADC1RST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_ADC1RST_BIT))

// Bit 10 — Reserved, read-only, do not write

// Bit 11 TIM1RST — TIM1 module reset
#define RCC_APB2PRSTR_TIM1RST_BIT      (11U)
#define RCC_APB2PRSTR_TIM1RST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_TIM1RST_BIT))

// Bit 12 SPI1RST — SPI1 interface reset
#define RCC_APB2PRSTR_SPI1RST_BIT      (12U)
#define RCC_APB2PRSTR_SPI1RST_MASK     ((uint32_t)(1UL << RCC_APB2PRSTR_SPI1RST_BIT))

// Bit 13 — Reserved, read-only, do not write

// Bit 14 USART1RST — USART1 interface reset
#define RCC_APB2PRSTR_USART1RST_BIT    (14U)
#define RCC_APB2PRSTR_USART1RST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_USART1RST_BIT))

// Bits [31:15] — Reserved, read-only, do not write


/*====================================================================
 * PB1 Peripheral Reset Register (RCC_APB1PRSTR) — offset 0x10
 * CH32V003 reference manual section 3.4.5
 * Write 1 to reset the module. Write 0 for no effect.
 *====================================================================*/

// Bit 0 TIM2RST — Timer 2 module reset
#define RCC_APB1PRSTR_TIM2RST_BIT      (0U)
#define RCC_APB1PRSTR_TIM2RST_MASK     ((uint32_t)(1UL << RCC_APB1PRSTR_TIM2RST_BIT))

// Bits [10:1] — Reserved, read-only, do not write

// Bit 11 WWDGRST — window watchdog reset
#define RCC_APB1PRSTR_WWDGRST_BIT      (11U)
#define RCC_APB1PRSTR_WWDGRST_MASK     ((uint32_t)(1UL << RCC_APB1PRSTR_WWDGRST_BIT))

// Bits [20:12] — Reserved, read-only, do not write

// Bit 21 I2C1RST — I2C1 interface reset
#define RCC_APB1PRSTR_I2C1RST_BIT      (21U)
#define RCC_APB1PRSTR_I2C1RST_MASK     ((uint32_t)(1UL << RCC_APB1PRSTR_I2C1RST_BIT))

// Bits [27:22] — Reserved, read-only, do not write

// Bit 28 PWRRST — power interface module reset
#define RCC_APB1PRSTR_PWRRST_BIT       (28U)
#define RCC_APB1PRSTR_PWRRST_MASK      ((uint32_t)(1UL << RCC_APB1PRSTR_PWRRST_BIT))

// Bits [31:29] — Reserved, read-only, do not write


/*====================================================================
 * HB Peripheral Clock Enable Register (RCC_AHBPCENR) — offset 0x14
 * CH32V003 reference manual section 3.4.6
 * Write 1 to enable module clock. Write 0 to disable.
 *====================================================================*/

// Bit 0 DMA1EN — DMA1 module clock enable
#define RCC_AHBPCENR_DMA1EN_BIT        (0U)
#define RCC_AHBPCENR_DMA1EN_MASK       ((uint32_t)(1UL << RCC_AHBPCENR_DMA1EN_BIT))

// Bit 1 — Reserved, read-only, do not write

// Bit 2 SRAMEN — SRAM interface module clock enable
//   1: SRAM interface module clock on during Sleep mode
//   0: SRAM interface module clock off in Sleep mode
//   Note: Reset value is 1 — SRAM clock is on by default
#define RCC_AHBPCENR_SRAMEN_BIT        (2U)
#define RCC_AHBPCENR_SRAMEN_MASK       ((uint32_t)(1UL << RCC_AHBPCENR_SRAMEN_BIT))

// Bits [31:3] — Reserved, read-only, do not write


/*====================================================================
 * PB2 Peripheral Clock Enable Register (RCC_APB2PCENR) — offset 0x18
 * CH32V003 reference manual section 3.4.7
 * Write 1 to enable module clock. Write 0 to disable.
 * You must enable a peripheral clock here before accessing its registers.
 *====================================================================*/

// Bit 0 AFIOEN — I/O auxiliary function module clock enable
#define RCC_APB2PCENR_AFIOEN_BIT       (0U)
#define RCC_APB2PCENR_AFIOEN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_AFIOEN_BIT))

// Bit 1 — Reserved, read-only, do not write

// Bit 2 IOPAEN — PA port module clock enable
#define RCC_APB2PCENR_IOPAEN_BIT       (2U)
#define RCC_APB2PCENR_IOPAEN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_IOPAEN_BIT))

// Bit 3 — Reserved, read-only, do not write

// Bit 4 IOPCEN — PC port module clock enable
#define RCC_APB2PCENR_IOPCEN_BIT       (4U)
#define RCC_APB2PCENR_IOPCEN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_IOPCEN_BIT))

// Bit 5 IOPDEN — PD port module clock enable
#define RCC_APB2PCENR_IOPDEN_BIT       (5U)
#define RCC_APB2PCENR_IOPDEN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_IOPDEN_BIT))

// Bits [8:6] — Reserved, read-only, do not write

// Bit 9 ADC1EN — ADC1 module clock enable
#define RCC_APB2PCENR_ADC1EN_BIT       (9U)
#define RCC_APB2PCENR_ADC1EN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_ADC1EN_BIT))

// Bit 10 — Reserved, read-only, do not write

// Bit 11 TIM1EN — TIM1 module clock enable
#define RCC_APB2PCENR_TIM1EN_BIT       (11U)
#define RCC_APB2PCENR_TIM1EN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_TIM1EN_BIT))

// Bit 12 SPI1EN — SPI1 interface clock enable
#define RCC_APB2PCENR_SPI1EN_BIT       (12U)
#define RCC_APB2PCENR_SPI1EN_MASK      ((uint32_t)(1UL << RCC_APB2PCENR_SPI1EN_BIT))

// Bit 13 — Reserved, read-only, do not write

// Bit 14 USART1EN — USART1 interface clock enable
#define RCC_APB2PCENR_USART1EN_BIT     (14U)
#define RCC_APB2PCENR_USART1EN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_USART1EN_BIT))

// Bits [31:15] — Reserved, read-only, do not write


/*====================================================================
 * PB1 Peripheral Clock Enable Register (RCC_APB1PCENR) — offset 0x1C
 * CH32V003 reference manual section 3.4.8
 * Write 1 to enable module clock. Write 0 to disable.
 *====================================================================*/

// Bit 0 TIM2EN — Timer 2 module clock enable
#define RCC_APB1PCENR_TIM2EN_BIT       (0U)
#define RCC_APB1PCENR_TIM2EN_MASK      ((uint32_t)(1UL << RCC_APB1PCENR_TIM2EN_BIT))

// Bits [10:1] — Reserved, read-only, do not write

// Bit 11 WWDGEN — window watchdog clock enable
#define RCC_APB1PCENR_WWDGEN_BIT       (11U)
#define RCC_APB1PCENR_WWDGEN_MASK      ((uint32_t)(1UL << RCC_APB1PCENR_WWDGEN_BIT))

// Bits [20:12] — Reserved, read-only, do not write

// Bit 21 I2C1EN — I2C1 interface clock enable
#define RCC_APB1PCENR_I2C1EN_BIT       (21U)
#define RCC_APB1PCENR_I2C1EN_MASK      ((uint32_t)(1UL << RCC_APB1PCENR_I2C1EN_BIT))

// Bits [27:22] — Reserved, read-only, do not write

// Bit 28 PWREN — power interface module clock enable
#define RCC_APB1PCENR_PWREN_BIT        (28U)
#define RCC_APB1PCENR_PWREN_MASK       ((uint32_t)(1UL << RCC_APB1PCENR_PWREN_BIT))

// Bits [31:29] — Reserved, read-only, do not write


/*====================================================================
 * Control/Status Register (RCC_RSTSCKR) — offset 0x24
 * CH32V003 reference manual section 3.4.9
 *
 * Upper bits [31:24]: Reset source flags (RO, hardware set)
 *   All flags are cleared together by writing RMVF = 1.
 *   Note: PORRSTF (bit 27) is cleared by power-on reset only.
 *
 * Lower bits [1:0]: LSI oscillator control
 *====================================================================*/

// Bit 0 LSION — internal low-speed clock (128kHz) enable (RW)
//   1: Enable the LSI (128kHz) oscillator
//   0: Disable the LSI oscillator
#define RCC_RSTSCKR_LSION_BIT          (0U)
#define RCC_RSTSCKR_LSION_MASK         ((uint32_t)(1UL << RCC_RSTSCKR_LSION_BIT))

// Bit 1 LSIRDY — internal low-speed clock stable ready flag (RO, hardware set)
//   1: Internal low-speed clock (128kHz) is stable
//   0: Internal low-speed clock (128kHz) is not stable
//   Note: Takes 3 LSI cycles to clear after LSION is cleared to 0
#define RCC_RSTSCKR_LSIRDY_BIT         (1U)
#define RCC_RSTSCKR_LSIRDY_MASK        ((uint32_t)(1UL << RCC_RSTSCKR_LSIRDY_BIT))

// Bits [23:2] — Reserved, read-only, do not write

// Bit 24 RMVF — clear reset flag control (RW)
//   1: Clear all reset source flags below
//   0: No effect
//   Note: Write 1 then 0 to clear — do not leave at 1
#define RCC_RSTSCKR_RMVF_BIT           (24U)
#define RCC_RSTSCKR_RMVF_MASK          ((uint32_t)(1UL << RCC_RSTSCKR_RMVF_BIT))

// Bit 25 — Reserved, read-only, do not write

// Bit 26 PINRSTF — external NRST pin reset flag (RO)
//   1: NRST pin reset occurred
//   0: No NRST pin reset
#define RCC_RSTSCKR_PINRSTF_BIT        (26U)
#define RCC_RSTSCKR_PINRSTF_MASK       ((uint32_t)(1UL << RCC_RSTSCKR_PINRSTF_BIT))

// Bit 27 PORRSTF — power-up/power-down reset flag (RO)
//   1: Power-up/power-down reset occurred
//   0: No power-up/power-down reset
//   Note: Reset value is 1 — always set after power-on
#define RCC_RSTSCKR_PORRSTF_BIT        (27U)
#define RCC_RSTSCKR_PORRSTF_MASK       ((uint32_t)(1UL << RCC_RSTSCKR_PORRSTF_BIT))

// Bit 28 SFTRSTF — software reset flag (RO)
//   1: Software reset occurred
//   0: No software reset
#define RCC_RSTSCKR_SFTRSTF_BIT        (28U)
#define RCC_RSTSCKR_SFTRSTF_MASK       ((uint32_t)(1UL << RCC_RSTSCKR_SFTRSTF_BIT))

// Bit 29 IWDGRSTF — independent watchdog reset flag (RO)
//   1: Independent watchdog reset occurred
//   0: No independent watchdog reset
#define RCC_RSTSCKR_IWDGRSTF_BIT       (29U)
#define RCC_RSTSCKR_IWDGRSTF_MASK      ((uint32_t)(1UL << RCC_RSTSCKR_IWDGRSTF_BIT))

// Bit 30 WWDGRSTF — window watchdog reset flag (RO)
//   1: Window watchdog reset occurred
//   0: No window watchdog reset
#define RCC_RSTSCKR_WWDGRSTF_BIT       (30U)
#define RCC_RSTSCKR_WWDGRSTF_MASK      ((uint32_t)(1UL << RCC_RSTSCKR_WWDGRSTF_BIT))

// Bit 31 LPWRRSTF — low-power reset flag (RO)
//   1: Low-power management reset occurred
//   0: No low-power reset
#define RCC_RSTSCKR_LPWRRSTF_BIT       (31U)
#define RCC_RSTSCKR_LPWRRSTF_MASK      ((uint32_t)(1UL << RCC_RSTSCKR_LPWRRSTF_BIT))


typedef struct {
    volatile uint32_t ctlr;      // 0x00 R32_RCC_CTLR      clock control register
    volatile uint32_t cfgr0;     // 0x04 R32_RCC_CFGR0     clock configuration register 0 
    volatile uint32_t intr;      // 0x08 R32_RCC_INTR      clock interrupt register
    volatile uint32_t apb2rst;   // 0x0C R32_RCC_APB2PRSTR APB2 peripheral reset register
    volatile uint32_t apb1rst;   // 0x10 R32_RCC_APB1PRSTR APB1 peripheral reset register
    volatile uint32_t ahbpcen;   // 0x14 R32_RCC_AHBPCENR  AHB peripheral clock enable
    volatile uint32_t apb2en;    // 0x18 R32_RCC_APB2PCENR APB2 peripheral clock enable
    volatile uint32_t apb1en;    // 0x1C R32_RCC_APB1PCENR APB1 peripheral clock enable
    volatile uint32_t reserved0; // 0x20                   reserved, do not access
    volatile uint32_t sckr;      // 0x24 R32_RCC_RSTSCKR   control/status register 
} RCC_TypeDef;


static inline volatile RCC_TypeDef *RCC_GetBase(void) {
    return (volatile RCC_TypeDef *)0x40021000U;
}

// Timeout loop iteratoin limit - prevents infinite hang on hardware that never becomes
// ready
#define RCC_INIT_TIMEOUT  (10000U)
#define RCC_FREQ_UNKNOWN  (0XFFFFFFFFU)

// Possible return codes from rcc_init
typedef enum {
    RCC_OK            = 0U,
    RCC_ERR_HSI       = 1U,
    RCC_ERR_PLL_LOCK  = 2U,
    RCC_ERR_SW_SWITCH = 3U
} RCC_Result_t;

// Clock configuration options passed to rcc_init.
// This will be extended later to support HSE
typedef struct {
    uint32_t sysclk_source;   // RCC_CFGR0_SW_HSI / _HSE / _PLL
    uint32_t pll_source;      // RCC_CFGR0_PLLSRC_HSI / _HSE
    uint32_t hpre;            // RCC_CFGR0_HPRE_DIVx
} RCC_InitTypeDef;


// Pre-built config for 48MHz: HSI -> PLL(*2) -> SYSCLK, HCLK=SYSCLK
static const RCC_InitTypeDef RCC_Config_48MHz = {
    RCC_CFGR0_SW_PLL,         // sysclk_source
    RCC_CFGR0_PLLSRC_HSI,     // pll_source
    RCC_CFGR0_HPRE_DIV1,      // hpre
};

static const RCC_InitTypeDef RCC_Config_24MHz = {
    RCC_CFGR0_SW_HSI,         // sysclk_source
    RCC_CFGR0_PLLSRC_HSI,     // pll_source - does not matter here, but must be valid.
    RCC_CFGR0_HPRE_DIV1,      // hpre
};

// TODO: add other default configs for low power modes potentially ?


static RCC_Result_t RCC_Init(const RCC_InitTypeDef * const p_config) {
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    RCC_Result_t result                = RCC_OK;
    uint32_t timeout                   = 0U;
    uint32_t expected_sws              = 0U;

    p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_HSION_MASK);
    // We first need to verify HSI is stable - it should be at reset, but its
    // good practice to confirm before touching anything
    while ((timeout < RCC_INIT_TIMEOUT) 
            && ((p_rcc->ctlr & RCC_CTLR_HSIRDY_MASK) == 0U)) {

        timeout = timeout + 1U;
    }

    if ((p_rcc->ctlr & RCC_CTLR_HSIRDY_MASK) == 0U) {
        result = RCC_ERR_HSI;
    }

    // Configure CFGR0 - PLL source and HPRE prescaler.
    //      The PLL must be off before wrirint PLLSRC
    //      The PLL is off at reset, so this is safe here
    //      We clear SW to HSI first so we are never left 
    //      without a clock during config.
    if (result == RCC_OK) {
        p_rcc->cfgr0 = (uint32_t)(
                            (p_rcc->cfgr0 
                                & ~RCC_CFGR0_SW_MASK
                                & ~RCC_CFGR0_PLLSRC_MASK
                                & ~RCC_CFGR0_HPRE_MASK)
                              | RCC_CFGR0_SW_HSI
                              | p_config->pll_source
                              | p_config->hpre);  

        // if the config struct selected the pll, we enable it and wait for
        // the pll to lock
        if (p_config->sysclk_source == RCC_CFGR0_SW_PLL) {
            p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_PLLON_MASK);

            timeout = 0U;
            while ((timeout < RCC_INIT_TIMEOUT) 
                    && ((p_rcc->ctlr & RCC_CTLR_PLLRDY_MASK) == 0U)) {

                timeout = timeout + 1U;
            }
            
            if ((p_rcc->ctlr & RCC_CTLR_PLLRDY_MASK) == 0U) {
                result = RCC_ERR_PLL_LOCK;
            }
        }
    }

    if (result == RCC_OK) {
        // we can switch to the requested source now
        p_rcc->cfgr0 = (uint32_t)(
                (p_rcc->cfgr0 & ~RCC_CFGR0_SW_MASK) 
                | p_config->sysclk_source );

        // it good practice to confirm that the switch was successful
        // we poll sws
        expected_sws = (uint32_t)(
                        (p_config->sysclk_source 
                         << (RCC_CFGR0_SWS_BIT - RCC_CFGR0_SW_BIT))
                        & RCC_CFGR0_SWS_MASK);
        
        timeout = 0U;
        while ((timeout < RCC_INIT_TIMEOUT) &&
             ((p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK) != expected_sws)) {

                timeout = timeout + 1U;
        }

        if ((p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK) != expected_sws) {
            result = RCC_ERR_SW_SWITCH;
        }
    }
    
    return result;
}

static RCC_Result_t RCC_Init_48mhz(void) {
    return RCC_Init(&RCC_Config_48MHz);
}

static RCC_Result_t RCC_Init_24mhz(void) {
    return RCC_Init(&RCC_Config_24MHz);
}

///Returns raw SYSCLK — before the HPRE prescaler
///
/// Example
/// ```c
/// int main(void) {
///     uint32_t rcc_result = 0U;
///     uint32_t freq       = 0U;
///     
///     rcc_result = rcc_init_48mhz();
///    
///     if (rcc_result != RCC_OK) {
///         // clock init failed
///         for (;;) { /* safe error trap*/ }
///     }
///     
///     freq = rcc_get_sysclk_hz();
/// }
/// ```
static uint32_t RCC_Get_Sysclk_Hz(void) {
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sws                       = 0U;
    uint32_t freq                      = 0U;

    sws = (uint32_t)(p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK);

    if (sws == RCC_CFGR0_SWS_PLL) {
        freq = (uint32_t)48000000UL;
    }
    //TODO: Add support for SHE
    else if (sws == RCC_CFGR0_SWS_HSE) {
        freq = RCC_FREQ_UNKNOWN;
    }
    else {
        freq = (uint32_t)24000000UL;
    }

    return freq;
}
//

// Returns HCLK — what peripherals actually run at
// This reads the current HPRE field and applies the correct divisor
static uint32_t RCC_Get_Hclk_Hz(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sysclk                    = 0U;
    uint32_t hpre                      = 0U;
    uint32_t divisor                   = 1U;
    uint32_t freq                      = 0U;

    sysclk = RCC_Get_Sysclk_Hz();

    /* If SYSCLK is unknown (HSE not configured) we cannot compute HCLK */
    if (sysclk == RCC_FREQ_UNKNOWN)
    {
        freq = RCC_FREQ_UNKNOWN;
    }
    else
    {
        /* Extract the raw HPRE field value */
        hpre = (uint32_t)((p_rcc->cfgr0 & RCC_CFGR0_HPRE_MASK)
                          >> RCC_CFGR0_HPRE_BIT);

        /* Decode the HPRE field into an actual divisor.
         * From the manual table — the encoding is non-linear above 0111 */
        switch (hpre)
        {
            case 0x0U: divisor =   1U; break;  /* 0000 — no division  */
            case 0x1U: divisor =   2U; break;  /* 0001                */
            case 0x2U: divisor =   3U; break;  /* 0010 — reset default */
            case 0x3U: divisor =   4U; break;  /* 0011                */
            case 0x4U: divisor =   5U; break;  /* 0100                */
            case 0x5U: divisor =   6U; break;  /* 0101                */
            case 0x6U: divisor =   7U; break;  /* 0110                */
            case 0x7U: divisor =   8U; break;  /* 0111                */
            case 0x8U: divisor =   2U; break;  /* 1000                */
            case 0x9U: divisor =   4U; break;  /* 1001                */
            case 0xAU: divisor =   8U; break;  /* 1010                */
            case 0xBU: divisor =  16U; break;  /* 1011                */
            case 0xCU: divisor =  32U; break;  /* 1100                */
            case 0xDU: divisor =  64U; break;  /* 1101                */
            case 0xEU: divisor = 128U; break;  /* 1110                */
            case 0xFU: divisor = 256U; break;  /* 1111                */
            default:   divisor =   1U; break;  /* should never happen */
        }

        freq = sysclk / divisor;
    }

    return freq;
}

// Encode a peripheral clock enable entry.
// reg_offset: byte offset of the clock enable register from RCC base.
// bit       : bit position within that register.
#define RCC_PERIPH_ENC(reg_offset, bit) \
    ((uint32_t)(((uint32_t)reg_offset << 5U) | ((uint32_t)(bit))))


typedef enum {
    /* AHB peripherals — RCC_AHBPCENR offset 0x14 */
    RCC_PERIPH_DMA1    = RCC_PERIPH_ENC(0x14U,  0U),
    RCC_PERIPH_SRAM    = RCC_PERIPH_ENC(0x14U,  2U),

    /* APB2 peripherals — RCC_APB2PCENR offset 0x18 */
    RCC_PERIPH_AFIO    = RCC_PERIPH_ENC(0x18U,  0U),
    RCC_PERIPH_GPIOA   = RCC_PERIPH_ENC(0x18U,  2U),
    RCC_PERIPH_GPIOC   = RCC_PERIPH_ENC(0x18U,  4U),
    RCC_PERIPH_GPIOD   = RCC_PERIPH_ENC(0x18U,  5U),
    RCC_PERIPH_ADC1    = RCC_PERIPH_ENC(0x18U,  9U),
    RCC_PERIPH_TIM1    = RCC_PERIPH_ENC(0x18U, 11U),
    RCC_PERIPH_SPI1    = RCC_PERIPH_ENC(0x18U, 12U),
    RCC_PERIPH_USART1  = RCC_PERIPH_ENC(0x18U, 14U),

    /* APB1 peripherals — RCC_APB1PCENR offset 0x1C */
    RCC_PERIPH_TIM2    = RCC_PERIPH_ENC(0x1CU,  0U),
    RCC_PERIPH_WWDG    = RCC_PERIPH_ENC(0x1CU, 11U),
    RCC_PERIPH_I2C1    = RCC_PERIPH_ENC(0x1CU, 21U),
    RCC_PERIPH_PWR     = RCC_PERIPH_ENC(0x1CU, 28U)

} RCC_Periph_t;


static volatile uint32_t *RCC_Get_Clken_Reg(RCC_Periph_t periph) {
    uint32_t offset = (uint32_t)((uint32_t)periph >> 5U);
    return (volatile uint32_t *)(0x40021000U + offset);
}

static uint32_t RCC_Get_Clken_Mask(RCC_Periph_t periph) {
    uint32_t bit = (uint32_t)((uint32_t)periph & 0x1FU);
    return (uint32_t)(1UL << bit);
}

// Enable the clock for a peripheral 
static void RCC_Enable_Clock(RCC_Periph_t periph) {
    volatile uint32_t * const p_reg = RCC_Get_Clken_Reg(periph);
    uint32_t mask                   = RCC_Get_Clken_Mask(periph);

    *p_reg = (uint32_t)(*p_reg | mask);
}

// Disable the clock for a peripheral 
static void RCC_Disable_Clock(RCC_Periph_t periph) {
    volatile uint32_t * const p_reg = RCC_Get_Clken_Reg(periph);
    uint32_t mask                   = RCC_Get_Clken_Mask(periph);

    *p_reg = (uint32_t)(*p_reg & ~mask);
}

// Check if clock is enabled for a peripheral
static uint32_t RCC_Is_Clock_Enabled(RCC_Periph_t periph) {
    volatile uint32_t * const p_reg = RCC_Get_Clken_Reg(periph);
    uint32_t mask                   = RCC_Get_Clken_Mask(periph);
    uint32_t result                 = 0U;

    if ((*p_reg & mask) != 0U) {
        result = 1U;
    }

    return result;
}


/// Add support for peripheral reset, this may be useful if we multiplex a pin
/// for different operations

/// The RSTSCKR register is useful at startup, we can read the reset flags, this tells
/// us exactly why the chip restarted, this can be of interest for safety systems

// static void rcc_log_reset_cause(void)
// {
//     volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
//     uint32_t sckr                      = 0U;

//     sckr = p_rcc->sckr;

//     if ((sckr & RCC_RSTSCKR_PORRSTF_MASK) != 0U)  { /* power-on reset        */ }
//     if ((sckr & RCC_RSTSCKR_PINRSTF_MASK) != 0U)  { /* NRST pin reset        */ }
//     if ((sckr & RCC_RSTSCKR_SFTRSTF_MASK) != 0U)  { /* software reset        */ }
//     if ((sckr & RCC_RSTSCKR_IWDGRSTF_MASK) != 0U) { /* watchdog reset — bad! */ }
//     if ((sckr & RCC_RSTSCKR_WWDGRSTF_MASK) != 0U) { /* window watchdog reset */ }
//     if ((sckr & RCC_RSTSCKR_LPWRRSTF_MASK) != 0U) { /* low-power reset       */ }

//     /* Clear all flags after reading */
//     p_rcc->sckr = (uint32_t)(p_rcc->sckr | RCC_RSTSCKR_RMVF_MASK);
//     p_rcc->sckr = (uint32_t)(p_rcc->sckr & ~RCC_RSTSCKR_RMVF_MASK);
// }


#endif // CH32V003_HAL_RCC_H
 
/// rcc.h
///
/// Reset and Clock Control (RCC) driver for CH32V003
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 3


#ifndef CH32V003_HAL_RCC_H
#define CH32V003_HAL_RCC_H

#include <stdint.h>

/*====================================================================
 * Register struct — CH32V003 manual Table 3-1
 * Base address: 0x40021000
 * 
 *====================================================================*/
typedef struct {
    volatile uint32_t ctlr;      /* 0x00 RCC_CTLR      clock control register         */
    volatile uint32_t cfgr0;     /* 0x04 RCC_CFGR0     clock configuration register 0 */
    volatile uint32_t intr;      /* 0x08 RCC_INTR      clock interrupt register        */
    volatile uint32_t apb2rst;   /* 0x0C RCC_APB2PRSTR APB2 peripheral reset register  */
    volatile uint32_t apb1rst;   /* 0x10 RCC_APB1PRSTR APB1 peripheral reset register  */
    volatile uint32_t ahbpcen;   /* 0x14 RCC_AHBPCENR  AHB  peripheral clock enable    */
    volatile uint32_t apb2en;    /* 0x18 RCC_APB2PCENR APB2 peripheral clock enable    */
    volatile uint32_t apb1en;    /* 0x1C RCC_APB1PCENR APB1 peripheral clock enable    */
    volatile uint32_t reserved0; /* 0x20               reserved                         */
    volatile uint32_t sckr;      /* 0x24 RCC_RSTSCKR   control/status register         */
} RCC_TypeDef;

/*
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Address 0x40021000 verified against CH32V003 manual Table 3-1.
 */
static inline volatile RCC_TypeDef *RCC_GetBase(void)
{
    return (volatile RCC_TypeDef *)0x40021000U;
}

/*====================================================================
 * RCC_CTLR — Clock Control Register (offset 0x00)
 * CH32V003 manual section 3.4.1
 * Reset value: 0x0000xx83 (HSION=1, HSIRDY=1, HSITRIM=10000b)
 *====================================================================*/

// Bit 0 HSION — HSI (24MHz internal RC) enable (RW)
//   1: Enable HSI oscillator
//   0: Disable HSI oscillator
//   Hardware sets this to 1 when returning from standby or when HSE fails.
#define RCC_CTLR_HSION_BIT        (0U)
#define RCC_CTLR_HSION_MASK       ((uint32_t)(1UL << RCC_CTLR_HSION_BIT))

// Bit 1 HSIRDY — HSI stable flag (RO, hardware set)
//   1: HSI is stable and ready
//   0: HSI not yet stable
//   Note: takes 6 HSI cycles to clear after HSION=0
#define RCC_CTLR_HSIRDY_BIT       (1U)
#define RCC_CTLR_HSIRDY_MASK      ((uint32_t)(1UL << RCC_CTLR_HSIRDY_BIT))

// Bits [7:3] HSITRIM[4:0] — HSI frequency trim (RW)
//   User-adjustable value superimposed on HSICAL.
//   Default 16 (10000b) targets 24MHz ±1%. Each step ≈ 60kHz.
#define RCC_CTLR_HSITRIM_BIT      (3U)
#define RCC_CTLR_HSITRIM_MASK     ((uint32_t)(0x1FUL << RCC_CTLR_HSITRIM_BIT))

// Bits [15:8] HSICAL[7:0] — HSI factory calibration (RO)
//   Loaded from factory calibration at reset. Combined with HSITRIM.
#define RCC_CTLR_HSICAL_BIT       (8U)
#define RCC_CTLR_HSICAL_MASK      ((uint32_t)(0xFFUL << RCC_CTLR_HSICAL_BIT))

// Bit 16 HSEON — HSE oscillator enable (RW)
//   1: Enable HSE oscillator
//   0: Disable HSE oscillator
//   Hardware clears to 0 after entering standby.
#define RCC_CTLR_HSEON_BIT        (16U)
#define RCC_CTLR_HSEON_MASK       ((uint32_t)(1UL << RCC_CTLR_HSEON_BIT))

// Bit 17 HSERDY — HSE stable flag (RO, hardware set)
//   1: HSE oscillation is stable
//   0: HSE not yet stable
//   Note: takes 6 HSE cycles to clear after HSEON=0
#define RCC_CTLR_HSERDY_BIT       (17U)
#define RCC_CTLR_HSERDY_MASK      ((uint32_t)(1UL << RCC_CTLR_HSERDY_BIT))

// Bit 18 HSEBYP — HSE bypass (RW)
//   1: External clock source fed directly to OSC_IN, OSC_OUT floating
//   0: Normal crystal/resonator mode
//   Must be written with HSEON=0.
#define RCC_CTLR_HSEBYP_BIT       (18U)
#define RCC_CTLR_HSEBYP_MASK      ((uint32_t)(1UL << RCC_CTLR_HSEBYP_BIT))

// Bit 19 CSSON — Clock Security System enable (RW)
//   1: Monitor HSE — if it fails, switch to HSI and fire NMI (CSSF set)
//   0: Clock security disabled
//   Monitor activates after HSERDY=1, deactivates when HSE turns off.
#define RCC_CTLR_CSSON_BIT        (19U)
#define RCC_CTLR_CSSON_MASK       ((uint32_t)(1UL << RCC_CTLR_CSSON_BIT))

// Bit 24 PLLON — PLL enable (RW)
//   1: Enable PLL
//   0: Disable PLL
//   Hardware clears to 0 after entering standby.
#define RCC_CTLR_PLLON_BIT        (24U)
#define RCC_CTLR_PLLON_MASK       ((uint32_t)(1UL << RCC_CTLR_PLLON_BIT))

// Bit 25 PLLRDY — PLL locked flag (RO, hardware set)
//   1: PLL is locked and stable
//   0: PLL not yet locked
#define RCC_CTLR_PLLRDY_BIT       (25U)
#define RCC_CTLR_PLLRDY_MASK      ((uint32_t)(1UL << RCC_CTLR_PLLRDY_BIT))

/*====================================================================
 * RCC_CFGR0 — Clock Configuration Register 0 (offset 0x04)
 * CH32V003 manual section 3.4.2
 * Reset value: 0x00000020 (HPRE=0010=÷3, all else 0)
 *====================================================================*/

// Bits [1:0] SW[1:0] — system clock source select (RW)
//   00: HSI as SYSCLK
//   01: HSE as SYSCLK
//   10: PLL output as SYSCLK
#define RCC_CFGR0_SW_BIT          (0U)
#define RCC_CFGR0_SW_MASK         ((uint32_t)(0x3UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_HSI          ((uint32_t)(0x0UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_HSE          ((uint32_t)(0x1UL << RCC_CFGR0_SW_BIT))
#define RCC_CFGR0_SW_PLL          ((uint32_t)(0x2UL << RCC_CFGR0_SW_BIT))

// Bits [3:2] SWS[1:0] — system clock status (RO, hardware set)
//   Mirrors SW after the switch completes. Always poll SWS to confirm.
#define RCC_CFGR0_SWS_BIT         (2U)
#define RCC_CFGR0_SWS_MASK        ((uint32_t)(0x3UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_HSI         ((uint32_t)(0x0UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_HSE         ((uint32_t)(0x1UL << RCC_CFGR0_SWS_BIT))
#define RCC_CFGR0_SWS_PLL         ((uint32_t)(0x2UL << RCC_CFGR0_SWS_BIT))

// Bits [7:4] HPRE[3:0] — AHB (HCLK) prescaler (RW)
//   Reset value: 0010 = ÷3, giving 8MHz HCLK from 24MHz HSI at reset
//   0000: ÷1   0001: ÷2   0010: ÷3   0011: ÷4
//   0100: ÷5   0101: ÷6   0110: ÷7   0111: ÷8
//   1000: ÷2   1001: ÷4   1010: ÷8   1011: ÷16
//   1100: ÷32  1101: ÷64  1110: ÷128 1111: ÷256
//   Note: prefetch buffer must be enabled when prescaler > 1
#define RCC_CFGR0_HPRE_BIT        (4U)
#define RCC_CFGR0_HPRE_MASK       ((uint32_t)(0xFUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV1       ((uint32_t)(0x0UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV2       ((uint32_t)(0x1UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV3       ((uint32_t)(0x2UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV4       ((uint32_t)(0x3UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV5       ((uint32_t)(0x4UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV6       ((uint32_t)(0x5UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV7       ((uint32_t)(0x6UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV8       ((uint32_t)(0x7UL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV16      ((uint32_t)(0xBUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV32      ((uint32_t)(0xCUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV64      ((uint32_t)(0xDUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV128     ((uint32_t)(0xEUL << RCC_CFGR0_HPRE_BIT))
#define RCC_CFGR0_HPRE_DIV256     ((uint32_t)(0xFUL << RCC_CFGR0_HPRE_BIT))

// Bits [15:11] ADCPRE[4:0] — ADC clock prescaler (RW)
//   Note: ADC clock must not exceed 24MHz
#define RCC_CFGR0_ADCPRE_BIT      (11U)
#define RCC_CFGR0_ADCPRE_MASK     ((uint32_t)(0x1FUL << RCC_CFGR0_ADCPRE_BIT))

// Bit 16 PLLSRC — PLL input clock source (RW, must write when PLL off)
//   1: HSE fed into PLL
//   0: HSI fed into PLL
#define RCC_CFGR0_PLLSRC_BIT      (16U)
#define RCC_CFGR0_PLLSRC_MASK     ((uint32_t)(1UL << RCC_CFGR0_PLLSRC_BIT))
#define RCC_CFGR0_PLLSRC_HSI      ((uint32_t)(0x0UL << RCC_CFGR0_PLLSRC_BIT))
#define RCC_CFGR0_PLLSRC_HSE      ((uint32_t)(1UL   << RCC_CFGR0_PLLSRC_BIT))

// Bits [26:24] MCO[2:0] — MCO pin clock output (RW)
//   0xx: No output
//   100: SYSCLK
//   101: HSI (24MHz)
//   110: HSE
//   111: PLL output
//   GPIO pin must be configured as AF_PP_50 for MCO output.
#define RCC_CFGR0_MCO_BIT         (24U)
#define RCC_CFGR0_MCO_MASK        ((uint32_t)(0x7UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_NONE        ((uint32_t)(0x0UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_SYSCLK      ((uint32_t)(0x4UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_HSI         ((uint32_t)(0x5UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_HSE         ((uint32_t)(0x6UL << RCC_CFGR0_MCO_BIT))
#define RCC_CFGR0_MCO_PLL         ((uint32_t)(0x7UL << RCC_CFGR0_MCO_BIT))

/*====================================================================
 * RCC_INTR — Clock Interrupt Register (offset 0x08)
 * CH32V003 manual section 3.4.3
 *
 * Three groups per clock source:
 *   xRDYF  (RO)  — flag: hardware sets when clock is ready
 *   xRDYIE (RW)  — enable: allow flag to generate an interrupt
 *   xRDYC  (WO)  — clear: write 1 to clear the flag
 *====================================================================*/

// Interrupt flags (read-only, hardware set)
#define RCC_INTR_LSIRDYF_BIT      (0U)
#define RCC_INTR_LSIRDYF_MASK     ((uint32_t)(1UL << RCC_INTR_LSIRDYF_BIT))
#define RCC_INTR_HSIRDYF_BIT      (2U)
#define RCC_INTR_HSIRDYF_MASK     ((uint32_t)(1UL << RCC_INTR_HSIRDYF_BIT))
#define RCC_INTR_HSERDYF_BIT      (3U)
#define RCC_INTR_HSERDYF_MASK     ((uint32_t)(1UL << RCC_INTR_HSERDYF_BIT))
#define RCC_INTR_PLLRDYF_BIT      (4U)
#define RCC_INTR_PLLRDYF_MASK     ((uint32_t)(1UL << RCC_INTR_PLLRDYF_BIT))

// Bit 7 CSSF — CSS interrupt flag (RO, cleared by writing CSSC=1)
//   1: HSE failure detected — system switched to HSI, NMI fired
#define RCC_INTR_CSSF_BIT         (7U)
#define RCC_INTR_CSSF_MASK        ((uint32_t)(1UL << RCC_INTR_CSSF_BIT))

// Interrupt enable bits (read-write)
#define RCC_INTR_LSIRDYIE_BIT     (8U)
#define RCC_INTR_LSIRDYIE_MASK    ((uint32_t)(1UL << RCC_INTR_LSIRDYIE_BIT))
#define RCC_INTR_HSIRDYIE_BIT     (10U)
#define RCC_INTR_HSIRDYIE_MASK    ((uint32_t)(1UL << RCC_INTR_HSIRDYIE_BIT))
#define RCC_INTR_HSERDYIE_BIT     (11U)
#define RCC_INTR_HSERDYIE_MASK    ((uint32_t)(1UL << RCC_INTR_HSERDYIE_BIT))
#define RCC_INTR_PLLRDYIE_BIT     (12U)
#define RCC_INTR_PLLRDYIE_MASK    ((uint32_t)(1UL << RCC_INTR_PLLRDYIE_BIT))

// Interrupt clear bits (write-only, write 1 to clear flag, 0 = no effect)
#define RCC_INTR_LSIRDYC_BIT      (16U)
#define RCC_INTR_LSIRDYC_MASK     ((uint32_t)(1UL << RCC_INTR_LSIRDYC_BIT))
#define RCC_INTR_HSIRDYC_BIT      (18U)
#define RCC_INTR_HSIRDYC_MASK     ((uint32_t)(1UL << RCC_INTR_HSIRDYC_BIT))
#define RCC_INTR_HSERDYC_BIT      (19U)
#define RCC_INTR_HSERDYC_MASK     ((uint32_t)(1UL << RCC_INTR_HSERDYC_BIT))
#define RCC_INTR_PLLRDYC_BIT      (20U)
#define RCC_INTR_PLLRDYC_MASK     ((uint32_t)(1UL << RCC_INTR_PLLRDYC_BIT))
#define RCC_INTR_CSSC_BIT         (23U)
#define RCC_INTR_CSSC_MASK        ((uint32_t)(1UL << RCC_INTR_CSSC_BIT))

/*====================================================================
 * RCC_APB2PRSTR — APB2 Peripheral Reset Register (offset 0x0C)
 * CH32V003 manual section 3.4.4
 * Write 1 to hold the module in reset. Write 0 to release.
 *====================================================================*/
#define RCC_APB2PRSTR_AFIORST_BIT    (0U)
#define RCC_APB2PRSTR_AFIORST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_AFIORST_BIT))
#define RCC_APB2PRSTR_IOPARST_BIT    (2U)
#define RCC_APB2PRSTR_IOPARST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_IOPARST_BIT))
#define RCC_APB2PRSTR_IOPCRST_BIT    (4U)
#define RCC_APB2PRSTR_IOPCRST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_IOPCRST_BIT))
#define RCC_APB2PRSTR_IOPDRST_BIT    (5U)
#define RCC_APB2PRSTR_IOPDRST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_IOPDRST_BIT))
#define RCC_APB2PRSTR_ADC1RST_BIT    (9U)
#define RCC_APB2PRSTR_ADC1RST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_ADC1RST_BIT))
#define RCC_APB2PRSTR_TIM1RST_BIT    (11U)
#define RCC_APB2PRSTR_TIM1RST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_TIM1RST_BIT))
#define RCC_APB2PRSTR_SPI1RST_BIT    (12U)
#define RCC_APB2PRSTR_SPI1RST_MASK   ((uint32_t)(1UL << RCC_APB2PRSTR_SPI1RST_BIT))
#define RCC_APB2PRSTR_USART1RST_BIT  (14U)
#define RCC_APB2PRSTR_USART1RST_MASK ((uint32_t)(1UL << RCC_APB2PRSTR_USART1RST_BIT))

/*====================================================================
 * RCC_APB1PRSTR — APB1 Peripheral Reset Register (offset 0x10)
 * CH32V003 manual section 3.4.5
 *====================================================================*/
#define RCC_APB1PRSTR_TIM2RST_BIT    (0U)
#define RCC_APB1PRSTR_TIM2RST_MASK   ((uint32_t)(1UL << RCC_APB1PRSTR_TIM2RST_BIT))
#define RCC_APB1PRSTR_WWDGRST_BIT    (11U)
#define RCC_APB1PRSTR_WWDGRST_MASK   ((uint32_t)(1UL << RCC_APB1PRSTR_WWDGRST_BIT))
#define RCC_APB1PRSTR_I2C1RST_BIT    (21U)
#define RCC_APB1PRSTR_I2C1RST_MASK   ((uint32_t)(1UL << RCC_APB1PRSTR_I2C1RST_BIT))
#define RCC_APB1PRSTR_PWRRST_BIT     (28U)
#define RCC_APB1PRSTR_PWRRST_MASK    ((uint32_t)(1UL << RCC_APB1PRSTR_PWRRST_BIT))

/*====================================================================
 * RCC_AHBPCENR — AHB Peripheral Clock Enable (offset 0x14)
 * CH32V003 manual section 3.4.6
 *====================================================================*/
#define RCC_AHBPCENR_DMA1EN_BIT      (0U)
#define RCC_AHBPCENR_DMA1EN_MASK     ((uint32_t)(1UL << RCC_AHBPCENR_DMA1EN_BIT))
#define RCC_AHBPCENR_SRAMEN_BIT      (2U)
#define RCC_AHBPCENR_SRAMEN_MASK     ((uint32_t)(1UL << RCC_AHBPCENR_SRAMEN_BIT))

/*====================================================================
 * RCC_APB2PCENR — APB2 Peripheral Clock Enable (offset 0x18)
 * CH32V003 manual section 3.4.7
 *====================================================================*/
#define RCC_APB2PCENR_AFIOEN_BIT     (0U)
#define RCC_APB2PCENR_AFIOEN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_AFIOEN_BIT))
#define RCC_APB2PCENR_IOPAEN_BIT     (2U)
#define RCC_APB2PCENR_IOPAEN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_IOPAEN_BIT))
#define RCC_APB2PCENR_IOPCEN_BIT     (4U)
#define RCC_APB2PCENR_IOPCEN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_IOPCEN_BIT))
#define RCC_APB2PCENR_IOPDEN_BIT     (5U)
#define RCC_APB2PCENR_IOPDEN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_IOPDEN_BIT))
#define RCC_APB2PCENR_ADC1EN_BIT     (9U)
#define RCC_APB2PCENR_ADC1EN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_ADC1EN_BIT))
#define RCC_APB2PCENR_TIM1EN_BIT     (11U)
#define RCC_APB2PCENR_TIM1EN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_TIM1EN_BIT))
#define RCC_APB2PCENR_SPI1EN_BIT     (12U)
#define RCC_APB2PCENR_SPI1EN_MASK    ((uint32_t)(1UL << RCC_APB2PCENR_SPI1EN_BIT))
#define RCC_APB2PCENR_USART1EN_BIT   (14U)
#define RCC_APB2PCENR_USART1EN_MASK  ((uint32_t)(1UL << RCC_APB2PCENR_USART1EN_BIT))

/*====================================================================
 * RCC_APB1PCENR — APB1 Peripheral Clock Enable (offset 0x1C)
 * CH32V003 manual section 3.4.8
 *====================================================================*/
#define RCC_APB1PCENR_TIM2EN_BIT     (0U)
#define RCC_APB1PCENR_TIM2EN_MASK    ((uint32_t)(1UL << RCC_APB1PCENR_TIM2EN_BIT))
#define RCC_APB1PCENR_WWDGEN_BIT     (11U)
#define RCC_APB1PCENR_WWDGEN_MASK    ((uint32_t)(1UL << RCC_APB1PCENR_WWDGEN_BIT))
#define RCC_APB1PCENR_I2C1EN_BIT     (21U)
#define RCC_APB1PCENR_I2C1EN_MASK    ((uint32_t)(1UL << RCC_APB1PCENR_I2C1EN_BIT))
#define RCC_APB1PCENR_PWREN_BIT      (28U)
#define RCC_APB1PCENR_PWREN_MASK     ((uint32_t)(1UL << RCC_APB1PCENR_PWREN_BIT))

/*====================================================================
 * RCC_RSTSCKR — Control/Status Register (offset 0x24)
 * CH32V003 manual section 3.4.9
 *
 * Upper bits [31:24]: reset source flags (RO, hardware set)
 *   All flags cleared together by writing RMVF=1 then RMVF=0.
 *   PORRSTF (bit 27) reset value is 1 — always set after power-on.
 * Lower bits [1:0]: LSI oscillator control
 *====================================================================*/

// Bit 0 LSION — LSI (128kHz) enable (RW)
//   1: Enable LSI oscillator
//   0: Disable LSI
//   Required before using AWU (auto-wakeup) or IWDG.
#define RCC_RSTSCKR_LSION_BIT      (0U)
#define RCC_RSTSCKR_LSION_MASK     ((uint32_t)(1UL << RCC_RSTSCKR_LSION_BIT))

// Bit 1 LSIRDY — LSI stable flag (RO, hardware set)
//   1: LSI is stable (takes ~3 LSI cycles after LSION=1)
//   0: LSI not yet stable
#define RCC_RSTSCKR_LSIRDY_BIT     (1U)
#define RCC_RSTSCKR_LSIRDY_MASK    ((uint32_t)(1UL << RCC_RSTSCKR_LSIRDY_BIT))

// Bit 24 RMVF — clear all reset flags (RW)
//   Write 1 then 0 to clear all reset source flags below.
#define RCC_RSTSCKR_RMVF_BIT       (24U)
#define RCC_RSTSCKR_RMVF_MASK      ((uint32_t)(1UL << RCC_RSTSCKR_RMVF_BIT))

// Bit 26 PINRSTF — external NRST pin reset (RO)
#define RCC_RSTSCKR_PINRSTF_BIT    (26U)
#define RCC_RSTSCKR_PINRSTF_MASK   ((uint32_t)(1UL << RCC_RSTSCKR_PINRSTF_BIT))

// Bit 27 PORRSTF — power-on/power-down reset (RO, reset value = 1)
#define RCC_RSTSCKR_PORRSTF_BIT    (27U)
#define RCC_RSTSCKR_PORRSTF_MASK   ((uint32_t)(1UL << RCC_RSTSCKR_PORRSTF_BIT))

// Bit 28 SFTRSTF — software reset (RO)
#define RCC_RSTSCKR_SFTRSTF_BIT    (28U)
#define RCC_RSTSCKR_SFTRSTF_MASK   ((uint32_t)(1UL << RCC_RSTSCKR_SFTRSTF_BIT))

// Bit 29 IWDGRSTF — independent watchdog reset (RO)
#define RCC_RSTSCKR_IWDGRSTF_BIT   (29U)
#define RCC_RSTSCKR_IWDGRSTF_MASK  ((uint32_t)(1UL << RCC_RSTSCKR_IWDGRSTF_BIT))

// Bit 30 WWDGRSTF — window watchdog reset (RO)
#define RCC_RSTSCKR_WWDGRSTF_BIT   (30U)
#define RCC_RSTSCKR_WWDGRSTF_MASK  ((uint32_t)(1UL << RCC_RSTSCKR_WWDGRSTF_BIT))

// Bit 31 LPWRRSTF — low-power management reset (RO)
#define RCC_RSTSCKR_LPWRRSTF_BIT   (31U)
#define RCC_RSTSCKR_LPWRRSTF_MASK  ((uint32_t)(1UL << RCC_RSTSCKR_LPWRRSTF_BIT))

/*====================================================================
 * Constants
 *
 *====================================================================*/
#define RCC_INIT_TIMEOUT  (10000U)

// Sentinel returned when HCLK or SYSCLK frequency cannot be determined.
// Chosen as 0xFFFFFFFF (not 0) to prevent divide-by-zero in callers.
#define RCC_FREQ_UNKNOWN  (0xFFFFFFFFU)

/*====================================================================
 * Return codes
 *
 *====================================================================*/
typedef enum {
    RCC_OK            = 0U,   /* success                               */
    RCC_ERR_HSI       = 1U,   /* HSI never became stable               */
    RCC_ERR_HSE       = 2U,   /* HSE never became stable               */
    RCC_ERR_PLL_LOCK  = 3U,   /* PLL never locked                      */
    RCC_ERR_SW_SWITCH = 4U    /* SYSCLK switch never confirmed via SWS  */
} RCC_Result_t;

/*====================================================================
 * Reset cause bitmask
 * Returned by RCC_GetResetCause().
 * Multiple flags can be set simultaneously.
 * 
 *====================================================================*/
typedef enum {
    RCC_RESET_POWER_ON   = (1U << 0U),  /* POR/PDR — normal power-up         */
    RCC_RESET_PIN        = (1U << 1U),  /* NRST pin held low externally       */
    RCC_RESET_SOFTWARE   = (1U << 2U),  /* PFIC_CFGR RESETSYS or SYSRESET    */
    RCC_RESET_IWDG       = (1U << 3U),  /* independent watchdog timeout       */
    RCC_RESET_WWDG       = (1U << 4U),  /* window watchdog timeout            */
    RCC_RESET_LOW_POWER  = (1U << 5U)   /* standby mode low-power reset       */
} RCC_ResetCause_t;

// RCC_CAUSE_IS — test a single reset cause flag in a cause bitmask
//
// Usage:
//   uint32_t cause = RCC_GetResetCause();
//   if (RCC_CAUSE_IS(cause, RCC_RESET_IWDG)) { ... }
//
// Equivalent to: (cause & (uint32_t)flag) != 0U
// The macro just makes the intent clearer at the call site.
#define RCC_CAUSE_IS(cause, flag) \
    (((cause) & (uint32_t)(flag)) != 0U)

/*====================================================================
 * MCO source selection enum
 *
 *====================================================================*/
typedef enum {
    RCC_MCO_NONE   = 0U,   /* MCO pin disabled   */
    RCC_MCO_SYSCLK = 1U,   /* output SYSCLK      */
    RCC_MCO_HSI    = 2U,   /* output HSI 24MHz   */
    RCC_MCO_HSE    = 3U,   /* output HSE         */
    RCC_MCO_PLL    = 4U    /* output PLL clock   */
} RCC_MCOSource_t;

/*====================================================================
 * Clock configuration struct
 *
 *====================================================================*/
typedef struct {
    uint32_t sysclk_source;   /* RCC_CFGR0_SW_HSI / _HSE / _PLL           */
    uint32_t pll_source;      /* RCC_CFGR0_PLLSRC_HSI / _HSE              */
    uint32_t hpre;            /* RCC_CFGR0_HPRE_DIVx                       */
    uint32_t hse_freq_hz;     /* HSE crystal frequency in Hz — 0 if unused */
} RCC_InitTypeDef;

/*====================================================================
 * Pre-built configurations
 *
 *====================================================================*/

// 48MHz: HSI → PLL(×2) → SYSCLK, HCLK = SYSCLK (no prescaler)
static const RCC_InitTypeDef RCC_Config_48MHz = {
    RCC_CFGR0_SW_PLL,
    RCC_CFGR0_PLLSRC_HSI,
    RCC_CFGR0_HPRE_DIV1,
    0U
};

// 24MHz: HSI direct → SYSCLK, HCLK = SYSCLK
static const RCC_InitTypeDef RCC_Config_24MHz = {
    RCC_CFGR0_SW_HSI,
    RCC_CFGR0_PLLSRC_HSI,
    RCC_CFGR0_HPRE_DIV1,
    0U
};

/*====================================================================
 * Internal HSE frequency storage
 * Stored when RCC_Init is called with an HSE configuration.
 * Read back by RCC_GetSysclkHz when SWS indicates HSE is active.
 * 
 *====================================================================*/
static uint32_t g_rcc_hse_freq_hz = 0U;

/*====================================================================
 * Peripheral encode macro and enum
 *
 * Packs the clock enable register offset and bit position into a
 * single uint32_t. The same encoding is used to derive the reset
 * register address:
 *   APB2 enable offset 0x18 → APB2 reset offset 0x0C
 *   APB1 enable offset 0x1C → APB1 reset offset 0x10
 *   AHB  enable offset 0x14 → no reset register
 * 
 *====================================================================*/
#define RCC_PERIPH_ENC(reg_offset, bit) \
    ((uint32_t)(((uint32_t)(reg_offset) << 5U) | ((uint32_t)(bit))))

typedef enum {
    /* AHB peripherals */
    RCC_PERIPH_DMA1   = RCC_PERIPH_ENC(0x14U,  0U),
    RCC_PERIPH_SRAM   = RCC_PERIPH_ENC(0x14U,  2U),

    /* APB2 peripherals */
    RCC_PERIPH_AFIO   = RCC_PERIPH_ENC(0x18U,  0U),
    RCC_PERIPH_GPIOA  = RCC_PERIPH_ENC(0x18U,  2U),
    RCC_PERIPH_GPIOC  = RCC_PERIPH_ENC(0x18U,  4U),
    RCC_PERIPH_GPIOD  = RCC_PERIPH_ENC(0x18U,  5U),
    RCC_PERIPH_ADC1   = RCC_PERIPH_ENC(0x18U,  9U),
    RCC_PERIPH_TIM1   = RCC_PERIPH_ENC(0x18U, 11U),
    RCC_PERIPH_SPI1   = RCC_PERIPH_ENC(0x18U, 12U),
    RCC_PERIPH_USART1 = RCC_PERIPH_ENC(0x18U, 14U),

    /* APB1 peripherals */
    RCC_PERIPH_TIM2   = RCC_PERIPH_ENC(0x1CU,  0U),
    RCC_PERIPH_WWDG   = RCC_PERIPH_ENC(0x1CU, 11U),
    RCC_PERIPH_I2C1   = RCC_PERIPH_ENC(0x1CU, 21U),
    RCC_PERIPH_PWR    = RCC_PERIPH_ENC(0x1CU, 28U)
} RCC_Periph_t;

/*====================================================================
 * Internal helpers — extract register pointer and bit mask
 *
 *====================================================================*/
static inline volatile uint32_t *RCC_GetClkenReg(RCC_Periph_t periph)
{
    uint32_t offset = (uint32_t)((uint32_t)periph >> 5U);
    return (volatile uint32_t *)(0x40021000U + offset);
}

static inline uint32_t RCC_GetClkenMask(RCC_Periph_t periph)
{
    uint32_t bit = (uint32_t)((uint32_t)periph & 0x1FU);
    return (uint32_t)(1UL << bit);
}

// Maps a peripheral's enable register offset to its reset register offset.
// Returns 0 if no reset register exists for this peripheral (AHB).
static inline uint32_t RCC_GetRstOffset(RCC_Periph_t periph)
{
    uint32_t en_offset  = (uint32_t)((uint32_t)periph >> 5U);
    uint32_t rst_offset = 0U;

    if (en_offset == 0x18U)       /* APB2 enable → APB2 reset */
    {
        rst_offset = 0x0CU;
    }
    else if (en_offset == 0x1CU)  /* APB1 enable → APB1 reset */
    {
        rst_offset = 0x10U;
    }
    else
    {
        rst_offset = 0U;          /* AHB — no reset register */
    }

    return rst_offset;
}

/*====================================================================
 * Clock initialisation
 *====================================================================*/

/*--------------------------------------------------------------------
 * RCC_Init — configure the system clock
 *
 * Sequence:
 *   1. Assert and wait for HSI stable
 *   2. Park SYSCLK on HSI while configuring (safe during transition)
 *   3. Configure CFGR0 — PLLSRC and HPRE (PLL must be off)
 *   4. If HSE needed: enable and wait for HSERDY
 *   5. If PLL needed: enable and wait for PLLRDY
 *   6. Switch SYSCLK to requested source
 *   7. Poll SWS to confirm switch
 *------------------------------------------------------------------*/
static RCC_Result_t RCC_Init(const RCC_InitTypeDef * const p_config)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    RCC_Result_t result                = RCC_OK;
    uint32_t     timeout               = 0U;
    uint32_t     expected_sws          = 0U;

    /* Step 1 — ensure HSI is on and stable */
    p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_HSION_MASK);
    timeout     = 0U;
    while ((timeout < RCC_INIT_TIMEOUT) &&
           ((p_rcc->ctlr & RCC_CTLR_HSIRDY_MASK) == 0U))
    {
        timeout = timeout + 1U;
    }
    if ((p_rcc->ctlr & RCC_CTLR_HSIRDY_MASK) == 0U)
    {
        result = RCC_ERR_HSI;
    }

    /* Step 2 — park on HSI and configure CFGR0 */
    if (result == RCC_OK)
    {
        p_rcc->cfgr0 = (uint32_t)((p_rcc->cfgr0
                        & ~RCC_CFGR0_SW_MASK
                        & ~RCC_CFGR0_PLLSRC_MASK
                        & ~RCC_CFGR0_HPRE_MASK)
                        | RCC_CFGR0_SW_HSI
                        | p_config->pll_source
                        | p_config->hpre);

        /* Step 3 — if HSE is the PLL source or direct SYSCLK, enable it */
        if ((p_config->sysclk_source == RCC_CFGR0_SW_HSE) ||
            (p_config->pll_source    == RCC_CFGR0_PLLSRC_HSE))
        {
            p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_HSEON_MASK);
            timeout     = 0U;
            while ((timeout < RCC_INIT_TIMEOUT) &&
                   ((p_rcc->ctlr & RCC_CTLR_HSERDY_MASK) == 0U))
            {
                timeout = timeout + 1U;
            }
            if ((p_rcc->ctlr & RCC_CTLR_HSERDY_MASK) == 0U)
            {
                result = RCC_ERR_HSE;
            }
            else
            {
                /* Store HSE frequency for RCC_GetSysclkHz */
                g_rcc_hse_freq_hz = p_config->hse_freq_hz;
            }
        }
    }

    /* Step 4 — if PLL is the SYSCLK source, enable and wait for lock */
    if ((result == RCC_OK) &&
        (p_config->sysclk_source == RCC_CFGR0_SW_PLL))
    {
        p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_PLLON_MASK);
        timeout     = 0U;
        while ((timeout < RCC_INIT_TIMEOUT) &&
               ((p_rcc->ctlr & RCC_CTLR_PLLRDY_MASK) == 0U))
        {
            timeout = timeout + 1U;
        }
        if ((p_rcc->ctlr & RCC_CTLR_PLLRDY_MASK) == 0U)
        {
            result = RCC_ERR_PLL_LOCK;
        }
    }

    /* Step 5 — switch SYSCLK and confirm via SWS */
    if (result == RCC_OK)
    {
        p_rcc->cfgr0 = (uint32_t)((p_rcc->cfgr0 & ~RCC_CFGR0_SW_MASK)
                        | p_config->sysclk_source);

        expected_sws = (uint32_t)(
                        (p_config->sysclk_source
                         << (RCC_CFGR0_SWS_BIT - RCC_CFGR0_SW_BIT))
                        & RCC_CFGR0_SWS_MASK);

        timeout = 0U;
        while ((timeout < RCC_INIT_TIMEOUT) &&
               ((p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK) != expected_sws))
        {
            timeout = timeout + 1U;
        }
        if ((p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK) != expected_sws)
        {
            result = RCC_ERR_SW_SWITCH;
        }
    }

    return result;
}

static RCC_Result_t RCC_Init48MHz(void)
{
    return RCC_Init(&RCC_Config_48MHz);
}

static RCC_Result_t RCC_Init24MHz(void)
{
    return RCC_Init(&RCC_Config_24MHz);
}

/*====================================================================
 * Clock frequency queries
 *====================================================================*/

// RCC_GetSysclkHz — raw SYSCLK before the HPRE prescaler
static uint32_t RCC_GetSysclkHz(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sws                       = 0U;
    uint32_t freq                      = 0U;

    sws = (uint32_t)(p_rcc->cfgr0 & RCC_CFGR0_SWS_MASK);

    if (sws == RCC_CFGR0_SWS_PLL)
    {
        /* PLL always doubles its input */
        if ((p_rcc->cfgr0 & RCC_CFGR0_PLLSRC_MASK) == RCC_CFGR0_PLLSRC_HSE)
        {
            /* HSE * 2 — use stored frequency */
            if (g_rcc_hse_freq_hz != 0U)
            {
                freq = g_rcc_hse_freq_hz * 2U;
            }
            else
            {
                freq = RCC_FREQ_UNKNOWN;
            }
        }
        else
        {
            /* HSI * 2 = 48MHz */
            freq = 48000000UL;
        }
    }
    else if (sws == RCC_CFGR0_SWS_HSE)
    {
        /* HSE direct — use stored frequency */
        if (g_rcc_hse_freq_hz != 0U)
        {
            freq = g_rcc_hse_freq_hz;
        }
        else
        {
            freq = RCC_FREQ_UNKNOWN;
        }
    }
    else
    {
        /* HSI direct = 24MHz */
        freq = 24000000UL;
    }

    return freq;
}

// RCC_GetHclkHz — HCLK as seen by all peripherals (after HPRE prescaler)
static uint32_t RCC_GetHclkHz(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sysclk                    = 0U;
    uint32_t hpre                      = 0U;
    uint32_t divisor                   = 1U;
    uint32_t freq                      = 0U;

    sysclk = RCC_GetSysclkHz();

    if (sysclk == RCC_FREQ_UNKNOWN)
    {
        freq = RCC_FREQ_UNKNOWN;
    }
    else
    {
        hpre = (uint32_t)((p_rcc->cfgr0 & RCC_CFGR0_HPRE_MASK)
                          >> RCC_CFGR0_HPRE_BIT);

        switch (hpre)
        {
            case 0x0U: divisor =   1U; break;
            case 0x1U: divisor =   2U; break;
            case 0x2U: divisor =   3U; break;  /* reset default */
            case 0x3U: divisor =   4U; break;
            case 0x4U: divisor =   5U; break;
            case 0x5U: divisor =   6U; break;
            case 0x6U: divisor =   7U; break;
            case 0x7U: divisor =   8U; break;
            case 0x8U: divisor =   2U; break;
            case 0x9U: divisor =   4U; break;
            case 0xAU: divisor =   8U; break;
            case 0xBU: divisor =  16U; break;
            case 0xCU: divisor =  32U; break;
            case 0xDU: divisor =  64U; break;
            case 0xEU: divisor = 128U; break;
            case 0xFU: divisor = 256U; break;
            default:   divisor =   1U; break;
        }

        freq = sysclk / divisor;
    }

    return freq;
}

/*====================================================================
 * Peripheral clock enable / disable / query
 *====================================================================*/

// RCC_EnableClock — enable the clock for a peripheral
//   Must be called before accessing any register of the peripheral.
//   If the clock is not enabled, reads always return 0.
static void RCC_EnableClock(RCC_Periph_t periph)
{
    volatile uint32_t * const p_reg = RCC_GetClkenReg(periph);
    uint32_t mask                   = RCC_GetClkenMask(periph);

    *p_reg = (uint32_t)(*p_reg | mask);
}

// RCC_DisableClock — disable the clock for a peripheral
//   Use before entering Sleep or Standby to reduce leakage current.
static void RCC_DisableClock(RCC_Periph_t periph)
{
    volatile uint32_t * const p_reg = RCC_GetClkenReg(periph);
    uint32_t mask                   = RCC_GetClkenMask(periph);

    *p_reg = (uint32_t)(*p_reg & ~mask);
}

// RCC_IsClockEnabled — returns 1U if clock is enabled, 0U if not
static uint32_t RCC_IsClockEnabled(RCC_Periph_t periph)
{
    volatile uint32_t * const p_reg = RCC_GetClkenReg(periph);
    uint32_t mask                   = RCC_GetClkenMask(periph);
    uint32_t result                 = 0U;

    if ((*p_reg & mask) != 0U)
    {
        result = 1U;
    }

    return result;
}

/*====================================================================
 * Peripheral reset
 *
 * Resets a peripheral's registers to their power-on defaults.
 * Useful when:
 *   - Reconfiguring a peripheral for a different mode
 *   - Recovering from a peripheral error state
 *   - Remapping a pin to a different peripheral
 *
 * The peripheral clock must remain enabled during reset.
 * AHB peripherals (DMA1, SRAM) have no reset register — calling
 * this for them is a no-op.
 *====================================================================*/

// RCC_ResetPeripheral — assert reset then release
//   The peripheral's registers are restored to power-on defaults.
//   Blocking: returns after the reset pulse completes.
static void RCC_ResetPeripheral(RCC_Periph_t periph)
{
    uint32_t rst_offset = RCC_GetRstOffset(periph);
    uint32_t mask       = RCC_GetClkenMask(periph);

    /* AHB peripherals have no reset register */
    if (rst_offset == 0U)
    {
        return;
    }

    volatile uint32_t * const p_rst =
        (volatile uint32_t *)(0x40021000U + rst_offset);

    /* Assert reset */
    *p_rst = (uint32_t)(*p_rst | mask);

    /* Release reset
     * A brief delay is implicit — the write itself takes multiple
     * bus cycles. No explicit loop needed for normal peripherals. */
    *p_rst = (uint32_t)(*p_rst & ~mask);
}

/*====================================================================
 * LSI oscillator control
 *
 * LSI is required by:
 *   - Auto-Wakeup Unit (PWR AWU)
 *   - Independent Watchdog (IWDG)
 *
 * Unlike peripheral clocks, LSI is controlled from RSTSCKR, not
 * from a peripheral clock enable register.
 *====================================================================*/

// RCC_EnableLSI — start the 128kHz internal RC oscillator
//   Blocks until LSIRDY = 1 (oscillator stable, typically < 10ms).
//   Must be called before enabling the AWU or IWDG.
static void RCC_EnableLSI(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t timeout                   = 0U;

    p_rcc->sckr = (uint32_t)(p_rcc->sckr | RCC_RSTSCKR_LSION_MASK);

    while ((timeout < RCC_INIT_TIMEOUT) &&
           ((p_rcc->sckr & RCC_RSTSCKR_LSIRDY_MASK) == 0U))
    {
        timeout = timeout + 1U;
    }
    /* If LSI never becomes ready the AWU will not function.
     * Caller can check RCC_IsLSIReady() if confirmation is needed. */
}

// RCC_DisableLSI — stop the LSI oscillator
//   Do not call while IWDG or AWU are running.
static void RCC_DisableLSI(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();

    p_rcc->sckr = (uint32_t)(p_rcc->sckr & ~RCC_RSTSCKR_LSION_MASK);
}

// RCC_IsLSIReady — returns 1U if LSI is stable, 0U if not
static uint32_t RCC_IsLSIReady(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t result                    = 0U;

    if ((p_rcc->sckr & RCC_RSTSCKR_LSIRDY_MASK) != 0U)
    {
        result = 1U;
    }

    return result;
}

/*====================================================================
 * Reset cause bitmask layout
 *
 * RCC_GetResetCause() returns a uint32_t where each bit corresponds
 * to one reset source. Use the RCC_ResetCause_t enum values to test
 * individual bits. Multiple bits can be set simultaneously.
 *
 * Bit  Flag constant            Meaning
 * ───  ─────────────────────    ──────────────────────────────────────
 *  0   RCC_RESET_POWER_ON       POR/PDR — normal power-up or brown-out
 *  1   RCC_RESET_PIN            NRST pin pulled low externally
 *  2   RCC_RESET_SOFTWARE       PFIC_SystemReset() was called
 *  3   RCC_RESET_IWDG           Independent watchdog timed out
 *  4   RCC_RESET_WWDG           Window watchdog timed out
 *  5   RCC_RESET_LOW_POWER      Low-power management reset
 *
 * Common combinations you will see in practice:
 *
 *   0x01  (POR only)
 *         Normal first power-on after programming or battery connect.
 *
 *   0x03  (POR + PIN)
 *         Power-on followed immediately by NRST — someone held the
 *         reset button during power-up, or the reset line was noisy.
 *
 *   0x09  (POR + IWDG)
 *         Watchdog fired and the power then cycled. In the field this
 *         means the firmware hung. Investigate.
 *
 *   0x04  (SOFTWARE only)
 *         Clean software reset — expected after firmware update or
 *         deliberate PFIC_SystemReset() call.
 *
 *   0x08  (IWDG only)
 *         Watchdog fired without a subsequent power cycle.
 *         Most concerning flag in production. Log and investigate.
 *
 * Usage:
 *
 *   uint32_t cause = RCC_GetResetCause();
 *   RCC_ClearResetFlags();
 *
 *   if (RCC_CAUSE_IS(cause, RCC_RESET_IWDG))
 *   {
 *       // watchdog fired — handle it
 *   }
 * 
 *====================================================================*/
static uint32_t RCC_GetResetCause(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sckr                      = p_rcc->sckr;
    uint32_t cause                     = 0U;

    if ((sckr & RCC_RSTSCKR_PORRSTF_MASK)  != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_POWER_ON);
    }
    if ((sckr & RCC_RSTSCKR_PINRSTF_MASK)  != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_PIN);
    }
    if ((sckr & RCC_RSTSCKR_SFTRSTF_MASK)  != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_SOFTWARE);
    }
    if ((sckr & RCC_RSTSCKR_IWDGRSTF_MASK) != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_IWDG);
    }
    if ((sckr & RCC_RSTSCKR_WWDGRSTF_MASK) != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_WWDG);
    }
    if ((sckr & RCC_RSTSCKR_LPWRRSTF_MASK) != 0U)
    {
        cause = (uint32_t)(cause | (uint32_t)RCC_RESET_LOW_POWER);
    }

    return cause;
}

// RCC_ClearResetFlags — clear all reset source flags
//   Write RMVF=1 then RMVF=0 as required by the manual.
//   Call after reading and logging the reset cause.
static void RCC_ClearResetFlags(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();

    p_rcc->sckr = (uint32_t)(p_rcc->sckr |  RCC_RSTSCKR_RMVF_MASK);
    p_rcc->sckr = (uint32_t)(p_rcc->sckr & ~RCC_RSTSCKR_RMVF_MASK);
}

/*====================================================================
 * Clock Security System (CSS)
 *
 * When enabled, the hardware monitors HSE. If HSE fails it:
 *   1. Switches SYSCLK to HSI automatically
 *   2. Disables HSE and PLL
 *   3. Sets CSSF flag in RCC_INTR
 *   4. Fires an NMI (non-maskable interrupt)
 *
 * Inside NMI_Handler:
 *   - Read CSSF to confirm it was a clock failure (not another NMI source)
 *   - Call RCC_ClearCSSFlag() to clear CSSF and dismiss the NMI pending bit
 *   - Re-initialise any clock-dependent peripherals at the new HSI rate
 *====================================================================*/

// RCC_EnableCSS — start monitoring HSE for failures
//   Only meaningful when HSE is active (HSERDY=1).
//   Has no effect when HSE is not ready.
static void RCC_EnableCSS(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();

    p_rcc->ctlr = (uint32_t)(p_rcc->ctlr | RCC_CTLR_CSSON_MASK);
}

// RCC_DisableCSS — stop monitoring HSE
static void RCC_DisableCSS(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();

    p_rcc->ctlr = (uint32_t)(p_rcc->ctlr & ~RCC_CTLR_CSSON_MASK);
}

// RCC_IsCSSFlagSet — returns 1U if a CSS clock failure was detected
static uint32_t RCC_IsCSSFlagSet(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t result                    = 0U;

    if ((p_rcc->intr & RCC_INTR_CSSF_MASK) != 0U)
    {
        result = 1U;
    }

    return result;
}

// RCC_ClearCSSFlag — clear the CSS interrupt flag
//   Call inside NMI_Handler after handling the HSE failure.
//   Write CSSC=1 to clear CSSF.
static void RCC_ClearCSSFlag(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();

    p_rcc->intr = (uint32_t)(p_rcc->intr | RCC_INTR_CSSC_MASK);
}

/*====================================================================
 * MCO — Microcontroller Clock Output
 *
 * Outputs a clock signal on the MCO pin.
 * GPIO pin must be configured as AF_PP_50 before enabling MCO.
 * MCO pin is PA8 (check your package pinout).
 *====================================================================*/

// RCC_SetMCO — select the MCO output source
//   Pass RCC_MCO_NONE to disable the output.
static void RCC_SetMCO(RCC_MCOSource_t source)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t mco_val                   = 0U;
    uint32_t cfgr0_val                 = 0U;

    switch (source)
    {
        case RCC_MCO_SYSCLK: mco_val = RCC_CFGR0_MCO_SYSCLK; break;
        case RCC_MCO_HSI:    mco_val = RCC_CFGR0_MCO_HSI;    break;
        case RCC_MCO_HSE:    mco_val = RCC_CFGR0_MCO_HSE;    break;
        case RCC_MCO_PLL:    mco_val = RCC_CFGR0_MCO_PLL;    break;
        default:             mco_val = RCC_CFGR0_MCO_NONE;   break;
    }

    cfgr0_val  = (uint32_t)(p_rcc->cfgr0 & ~RCC_CFGR0_MCO_MASK);
    cfgr0_val  = (uint32_t)(cfgr0_val | mco_val);
    p_rcc->cfgr0 = cfgr0_val;
}

#endif
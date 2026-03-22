/// gpio.h
///
/// GPIO driver for CH32V003 — ports PA, PC, PD (no PB on this device)
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 7

#ifndef CH32V003_HAL_GPIO_H
#define CH32V003_HAL_GPIO_H

#include <stdint.h>
#include "rcc.h"

/*====================================================================
 * Register map — CH32V003 manual Table 7-8
 * All three ports (A, C, D) share this layout.
 *
 * IMPORTANT: offset 0x04 is reserved — no CFGHR exists on CH32V003
 * (only 8 pins per port). Without reserved0 every field below it
 * maps to the wrong address.
 *====================================================================*/
typedef struct {
    volatile uint32_t cfglr;     /* 0x00 port configuration register low   */
    volatile uint32_t reserved0; /* 0x04 reserved — do not access          */
    volatile uint32_t indr;      /* 0x08 port input data register          */
    volatile uint32_t outdr;     /* 0x0C port output data register         */
    volatile uint32_t bshr;      /* 0x10 port set/reset register           */
    volatile uint32_t bcr;       /* 0x14 port reset register               */
    volatile uint32_t lckr;      /* 0x18 port configuration lock register  */
} GPIO_TypeDef;

#define GPIOA_BASE    (0x40010800U)
#define GPIOC_BASE    (0x40011000U)
#define GPIOD_BASE    (0x40011400U)

/*====================================================================
 * Port accessors
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Addresses verified against CH32V003 manual Table 7-8.
 *====================================================================*/
static inline volatile GPIO_TypeDef *GPIO_GetBase_A(void)
{
    return (volatile GPIO_TypeDef *)GPIOA_BASE;
}
 

static inline volatile GPIO_TypeDef *GPIO_GetBase_C(void)
{
    return (volatile GPIO_TypeDef *)GPIOC_BASE;
}

static inline volatile GPIO_TypeDef *GPIO_GetBase_D(void)
{
    return (volatile GPIO_TypeDef *)GPIOD_BASE;
}

/*====================================================================
 * CFGLR — Port Configuration Register Low (offset 0x00)
 * CH32V003 manual section 7.3.1.1
 *
 * Each pin occupies 4 bits:
 *   bits [4y+1 : 4y  ] = MODEy[1:0]  — direction and speed
 *   bits [4y+3 : 4y+2] = CNFy[1:0]   — type (depends on MODE)
 *
 * Reset value: 0x44444444
 *   Each pin resets to MODE=00 (input), CNF=01 (floating input)
 *====================================================================*/

/* Number of bits per pin in CFGLR */
#define GPIO_CFGLR_PIN_WIDTH    (4U)

/* Mask for one pin's 4-bit field */
#define GPIO_CFGLR_PIN_MASK     (0xFU)

/*--------------------------------------------------------------------
 * MODE field values (bits [1:0] of each pin's 4-bit slot)
 * Applied at position: (pin * GPIO_CFGLR_PIN_WIDTH)
 *------------------------------------------------------------------*/
#define GPIO_MODE_INPUT         (0x0U)  /* input mode (use CNF to select type) */
#define GPIO_MODE_OUT_10MHZ     (0x1U)  /* output, max 10MHz                   */
#define GPIO_MODE_OUT_2MHZ      (0x2U)  /* output, max 2MHz                    */
#define GPIO_MODE_OUT_50MHZ     (0x3U)  /* output, max 50MHz                   */

/*--------------------------------------------------------------------
 * CNF field values (bits [3:2] of each pin's 4-bit slot)
 * Meaning depends on whether MODE is input or output.
 *
 * When MODE = INPUT (00):
 *------------------------------------------------------------------*/
#define GPIO_CNF_IN_ANALOG      (0x0U)  /* analog input                        */
#define GPIO_CNF_IN_FLOATING    (0x1U)  /* floating input (reset default)       */
#define GPIO_CNF_IN_PULLUPDOWN  (0x2U)  /* pull-up/pull-down (set OUTDR bit)   */
/* CNF=11 reserved in input mode */

/*--------------------------------------------------------------------
 * When MODE = OUTPUT (01, 10, or 11):
 *------------------------------------------------------------------*/
#define GPIO_CNF_OUT_PP         (0x0U)  /* push-pull output                    */
#define GPIO_CNF_OUT_OD         (0x1U)  /* open-drain output                   */
#define GPIO_CNF_AF_PP          (0x2U)  /* alternate function push-pull        */
#define GPIO_CNF_AF_OD          (0x3U)  /* alternate function open-drain       */

/*====================================================================
 * BSHR — Port Set/Reset Register (offset 0x10)
 * CH32V003 manual section 7.3.1.4
 *
 * Single write — no read-modify-write required (atomic).
 * bits [7:0]   = BSy — write 1 to SET   pin y in OUTDR
 * bits [15:8]  = Reserved
 * bits [23:16] = BRy — write 1 to CLEAR pin y in OUTDR
 * bits [31:24] = Reserved
 *
 * NOTE: CH32V003 layout differs from STM32 — BR bits are at [23:16]
 * not [31:16] because the port is only 8 pins wide.
 * If both BS and BR are set for the same pin, BS takes priority.
 *====================================================================*/
#define GPIO_BSHR_BS_OFFSET     (0U)    /* set   bits start at bit 0  */
#define GPIO_BSHR_BR_OFFSET     (16U)   /* reset bits start at bit 16 */

/*====================================================================
 * BCR — Port Reset Register (offset 0x14)
 * CH32V003 manual section 7.3.1.5
 *
 * Write-only. bits [7:0] = BRy — write 1 to clear pin y in OUTDR.
 * Writing 0 has no effect.
 * Equivalent to using the BR bits in BSHR — use whichever is clearer.
 *====================================================================*/

/*====================================================================
 * Port and pin types
 *====================================================================*/
typedef enum {
    GPIO_PORT_A = 0U,
    GPIO_PORT_C = 1U,
    GPIO_PORT_D = 2U
} GPIO_Port_t;

typedef enum {
    GPIO_PIN_0 = 0U,
    GPIO_PIN_1 = 1U,
    GPIO_PIN_2 = 2U,
    GPIO_PIN_3 = 3U,
    GPIO_PIN_4 = 4U,
    GPIO_PIN_5 = 5U,
    GPIO_PIN_6 = 6U,
    GPIO_PIN_7 = 7U
} GPIO_Pin_t;

/*====================================================================
 * Pin mode configuration struct
 *====================================================================*/
typedef enum {
    GPIO_MODE_IN_ANALOG    = 0U,  /* analog input                      */
    GPIO_MODE_IN_FLOATING  = 1U,  /* floating input (reset default)     */
    GPIO_MODE_IN_PULLDOWN  = 2U,  /* pull-down input                   */
    GPIO_MODE_IN_PULLUP    = 3U,  /* pull-up input                     */
    GPIO_MODE_OUT_PP_10    = 4U,  /* push-pull output 10MHz            */
    GPIO_MODE_OUT_PP_2     = 5U,  /* push-pull output 2MHz             */
    GPIO_MODE_OUT_PP_50    = 6U,  /* push-pull output 50MHz            */
    GPIO_MODE_OUT_OD_10    = 7U,  /* open-drain output 10MHz           */
    GPIO_MODE_OUT_OD_2     = 8U,  /* open-drain output 2MHz            */
    GPIO_MODE_OUT_OD_50    = 9U,  /* open-drain output 50MHz           */
    GPIO_MODE_AF_PP_10     = 10U, /* alternate function push-pull 10MHz */
    GPIO_MODE_AF_PP_2      = 11U, /* alternate function push-pull 2MHz  */
    GPIO_MODE_AF_PP_50     = 12U, /* alternate function push-pull 50MHz */
    GPIO_MODE_AF_OD_10     = 13U, /* alternate function open-drain 10MHz*/
    GPIO_MODE_AF_OD_2      = 14U, /* alternate function open-drain 2MHz */
    GPIO_MODE_AF_OD_50     = 15U  /* alternate function open-drain 50MHz*/
} GPIO_PinMode_t;

/*====================================================================
 * Internal helpers — not part of the public API
 *====================================================================*/

/*
 * Returns the GPIO base pointer for a given port.
 * Deviation: MISRA C:2012 Rule 15.5 (Advisory) — multiple return points.
 * Justified: switch default is unreachable by enum constraint; single
 * fallthrough return would require an unused local variable.
 */
static volatile GPIO_TypeDef *GPIO_GetBase(GPIO_Port_t port)
{
    volatile GPIO_TypeDef *p_gpio = GPIO_GetBase_A();

    switch (port)
    {
        case GPIO_PORT_A: p_gpio = GPIO_GetBase_A(); break;
        case GPIO_PORT_C: p_gpio = GPIO_GetBase_C(); break;
        case GPIO_PORT_D: p_gpio = GPIO_GetBase_D(); break;
        default:          p_gpio = GPIO_GetBase_A(); break; /* unreachable */
    }

    return p_gpio;
}

/*
 * Encodes a GPIO_PinMode_t into the two-field MODE+CNF value
 * that goes into CFGLR. Returns the 4-bit value, not yet shifted.
 */
static uint32_t GPIO_EncodeCfglr(GPIO_PinMode_t mode)
{
    uint32_t mode_field = 0U;
    uint32_t cnf_field  = 0U;
    uint32_t result     = 0U;

    switch (mode)
    {
        case GPIO_MODE_IN_ANALOG:
            mode_field = GPIO_MODE_INPUT;
            cnf_field  = GPIO_CNF_IN_ANALOG;
            break;
        case GPIO_MODE_IN_FLOATING:
            mode_field = GPIO_MODE_INPUT;
            cnf_field  = GPIO_CNF_IN_FLOATING;
            break;
        case GPIO_MODE_IN_PULLDOWN:   /* fall through — same CNF, OUTDR controls direction */
        case GPIO_MODE_IN_PULLUP:
            mode_field = GPIO_MODE_INPUT;
            cnf_field  = GPIO_CNF_IN_PULLUPDOWN;
            break;
        case GPIO_MODE_OUT_PP_10:
            mode_field = GPIO_MODE_OUT_10MHZ;
            cnf_field  = GPIO_CNF_OUT_PP;
            break;
        case GPIO_MODE_OUT_PP_2:
            mode_field = GPIO_MODE_OUT_2MHZ;
            cnf_field  = GPIO_CNF_OUT_PP;
            break;
        case GPIO_MODE_OUT_PP_50:
            mode_field = GPIO_MODE_OUT_50MHZ;
            cnf_field  = GPIO_CNF_OUT_PP;
            break;
        case GPIO_MODE_OUT_OD_10:
            mode_field = GPIO_MODE_OUT_10MHZ;
            cnf_field  = GPIO_CNF_OUT_OD;
            break;
        case GPIO_MODE_OUT_OD_2:
            mode_field = GPIO_MODE_OUT_2MHZ;
            cnf_field  = GPIO_CNF_OUT_OD;
            break;
        case GPIO_MODE_OUT_OD_50:
            mode_field = GPIO_MODE_OUT_50MHZ;
            cnf_field  = GPIO_CNF_OUT_OD;
            break;
        case GPIO_MODE_AF_PP_10:
            mode_field = GPIO_MODE_OUT_10MHZ;
            cnf_field  = GPIO_CNF_AF_PP;
            break;
        case GPIO_MODE_AF_PP_2:
            mode_field = GPIO_MODE_OUT_2MHZ;
            cnf_field  = GPIO_CNF_AF_PP;
            break;
        case GPIO_MODE_AF_PP_50:
            mode_field = GPIO_MODE_OUT_50MHZ;
            cnf_field  = GPIO_CNF_AF_PP;
            break;
        case GPIO_MODE_AF_OD_10:
            mode_field = GPIO_MODE_OUT_10MHZ;
            cnf_field  = GPIO_CNF_AF_OD;
            break;
        case GPIO_MODE_AF_OD_2:
            mode_field = GPIO_MODE_OUT_2MHZ;
            cnf_field  = GPIO_CNF_AF_OD;
            break;
        case GPIO_MODE_AF_OD_50:
            mode_field = GPIO_MODE_OUT_50MHZ;
            cnf_field  = GPIO_CNF_AF_OD;
            break;
        default:
            mode_field = GPIO_MODE_INPUT;
            cnf_field  = GPIO_CNF_IN_FLOATING;
            break;
    }

    /* Pack MODE into bits [1:0], CNF into bits [3:2] */
    result = (uint32_t)((cnf_field << 2U) | mode_field);
    return result;
}

/*====================================================================
 * Public API
 *====================================================================*/

/*
 * GPIO_SetPinMode — configure a single pin
 *
 * Handles the OUTDR pull direction bit for pull-up/pull-down modes.
 * Must be called after RCC_Enable_Clock for the relevant port.
 *
 * Parameters:
 *   port — GPIO_PORT_A, GPIO_PORT_C, or GPIO_PORT_D
 *   pin  — GPIO_PIN_0 through GPIO_PIN_7
 *   mode — any GPIO_PinMode_t value
 */
static void GPIO_SetPinMode(GPIO_Port_t port, GPIO_Pin_t pin, GPIO_PinMode_t mode)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    uint32_t shift                       = (uint32_t)pin * GPIO_CFGLR_PIN_WIDTH;
    uint32_t cfglr_val                   = 0U;
    uint32_t encoded                     = 0U;

    encoded   = GPIO_EncodeCfglr(mode);

    /* Read-modify-write: clear the 4-bit slot then write new value */
    cfglr_val = (uint32_t)(p_gpio->cfglr & ~(GPIO_CFGLR_PIN_MASK << shift));
    cfglr_val = (uint32_t)(cfglr_val | (encoded << shift));
    p_gpio->cfglr = cfglr_val;

    /* For pull modes, set OUTDR bit to control direction.
     * OUTDR=1 → pull-up, OUTDR=0 → pull-down.
     * Atomic via BSHR/BCR — no read-modify-write on OUTDR needed. */
    if (mode == GPIO_MODE_IN_PULLUP)
    {
        p_gpio->bshr = (uint32_t)(1UL << (uint32_t)pin);   /* set = pull-up   */
    }
    else if (mode == GPIO_MODE_IN_PULLDOWN)
    {
        p_gpio->bcr  = (uint32_t)(1UL << (uint32_t)pin);   /* clear = pull-down */
    }
    else
    {
        /* no pull — nothing to do */
    }
}

/*
 * GPIO_SetPin — drive a pin high (atomic, no read-modify-write)
 * Pin must be configured as an output mode first.
 */
static void GPIO_SetPin(GPIO_Port_t port, GPIO_Pin_t pin)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    p_gpio->bshr = (uint32_t)(1UL << (uint32_t)pin);
}

/*
 * GPIO_ClearPin — drive a pin low (atomic, no read-modify-write)
 * Pin must be configured as an output mode first.
 */
static void GPIO_ClearPin(GPIO_Port_t port, GPIO_Pin_t pin)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    p_gpio->bcr = (uint32_t)(1UL << (uint32_t)pin);
}

/*
 * GPIO_TogglePin — toggle a pin's current output state
 * Reads OUTDR then uses BSHR/BCR to set or clear atomically.
 * The read-modify-write is on OUTDR (not BSHR/BCR), so there is a
 * small window — if atomicity of toggle is required, disable
 * interrupts around this call.
 */
static void GPIO_TogglePin(GPIO_Port_t port, GPIO_Pin_t pin)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    uint32_t pin_mask                    = (uint32_t)(1UL << (uint32_t)pin);

    if ((p_gpio->outdr & pin_mask) != 0U)
    {
        p_gpio->bcr  = pin_mask;   /* currently high — clear */
    }
    else
    {
        p_gpio->bshr = pin_mask;   /* currently low  — set   */
    }
}

/*
 * GPIO_ReadPin — read the current input state of a pin
 * Returns 1U if pin is high, 0U if low.
 * Pin should be configured as input mode, but INDR is readable
 * regardless of mode.
 */
static uint32_t GPIO_ReadPin(GPIO_Port_t port, GPIO_Pin_t pin)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    uint32_t result                      = 0U;

    if ((p_gpio->indr & (uint32_t)(1UL << (uint32_t)pin)) != 0U)
    {
        result = 1U;
    }

    return result;
}

/*
 * GPIO_WritePort — write all 8 pins of a port simultaneously via OUTDR
 * bits [7:0] of value map to pins 0-7.
 * Use for initialising a port to a known state. For runtime pin
 * control use GPIO_SetPin / GPIO_ClearPin.
 */
static void GPIO_WritePort(GPIO_Port_t port, uint8_t value)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    p_gpio->outdr = (uint32_t)value;
}

/*
 * GPIO_ReadPort — read all 8 input pins simultaneously
 * Returns bits [7:0], one bit per pin.
 */
static uint8_t GPIO_ReadPort(GPIO_Port_t port)
{
    volatile GPIO_TypeDef * const p_gpio = GPIO_GetBase(port);
    return (uint8_t)(p_gpio->indr & 0xFFU);
}


/*====================================================================
 * AFIO register map — CH32V003 manual section 7.3.2
 * Base address: 0x40010000
 *====================================================================*/
typedef struct {
    volatile uint32_t reserved0; /* 0x00 reserved — do not access         */
    volatile uint32_t pcfr1;     /* 0x04 AFIO_PCFR1  remap register 1    */
    volatile uint32_t exticr;    /* 0x08 AFIO_EXTICR EXTI config register */
} AFIO_TypeDef;

#define AFIO_BASE    (0x40010000U)

/*
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Address 0x40010000 verified against CH32V003 manual
 *            Table 7-9.
 */
static inline volatile AFIO_TypeDef *AFIO_GetBase(void)
{
    return (volatile AFIO_TypeDef *)AFIO_BASE;
}

/*====================================================================
 * AFIO_PCFR1 — Remap Register 1 (offset 0x04)
 * CH32V003 manual section 7.3.2.1
 *====================================================================*/

// Bit 0 SPI1_RM — SPI1 remapping
//   0: Default  (NSS/PC1, CK/PC5, MISO/PC7, MOSI/PC6)
//   1: Remapped (NSS/PC0, CK/PC5, MISO/PC7, MOSI/PC6)
#define AFIO_PCFR1_SPI1_RM_BIT        (0U)
#define AFIO_PCFR1_SPI1_RM_MASK       ((uint32_t)(1UL << AFIO_PCFR1_SPI1_RM_BIT))
#define AFIO_PCFR1_SPI1_RM_DEFAULT    (0U)
#define AFIO_PCFR1_SPI1_RM_REMAP      (1U)

// Bit 1 I2C1_RM — I2C1 remapping low bit (used with bit 22)
//   00: Default  (SCL/PC2,  SDA/PC1)
//   01: Remapped (SCL/PD1,  SDA/PD0)
//   1x: Remapped (SCL/PC5,  SDA/PC6)
#define AFIO_PCFR1_I2C1_RM_BIT        (1U)
#define AFIO_PCFR1_I2C1_RM_MASK       ((uint32_t)(1UL << AFIO_PCFR1_I2C1_RM_BIT))

// Bit 2 USART1_RM — USART1 remapping low bit (used with bit 21)
//   00: Default  (CK/PD4,  TX/PD5,  RX/PD6,  CTS/PD3, RTS/PC2)
//   01: Remapped (CK/PD7,  TX/PD0,  RX/PD1,  CTS/PC3, RTS/PC2, SW_RX/PD0)
//   10: Remapped (CK/PD7,  TX/PD6,  RX/PD5,  CTS/PC6, RTS/PC7, SW_RX/PD6)
//   11: Remapped (CK/PC5,  TX/PC0,  RX/PC1,  CTS/PC6, RTS/PC7, SW_RX/PC0)
#define AFIO_PCFR1_USART1_RM_BIT      (2U)
#define AFIO_PCFR1_USART1_RM_MASK     ((uint32_t)(1UL << AFIO_PCFR1_USART1_RM_BIT))

// Bits [5:3] — Reserved, do not modify

// Bits [7:6] TIM1_RM[1:0] — TIM1 remapping
//   00: Default  (ETR/PC5, CH1/PD2, CH2/PA1, CH3/PC3, CH4/PC4,
//                 BKIN/PC2, CH1N/PD0, CH2N/PA2, CH3N/PD1)
//   01: Partial  (ETR/PC5, CH1/PC6, CH2/PC7, CH3/PC0, CH4/PD3,
//                 BKIN/PC1, CH1N/PC3, CH2N/PC4, CH3N/PD1)
//   10: Partial  (ETR/PD4, CH1/PD2, CH2/PA1, CH3/PC3, CH4/PC4,
//                 BKIN/PC2, CH1N/PD0, CH2N/PA2, CH3N/PD1)
//   11: Complete (ETR/PC2, CH1/PC4, CH2/PC7, CH3/PC5, CH4/PD4,
//                 BKIN/PC1, CH1N/PC3, CH2N/PD2, CH3N/PC6)
#define AFIO_PCFR1_TIM1_RM_BIT        (6U)
#define AFIO_PCFR1_TIM1_RM_MASK       ((uint32_t)(0x3UL << AFIO_PCFR1_TIM1_RM_BIT))
#define AFIO_PCFR1_TIM1_RM_DEFAULT    ((uint32_t)(0x0UL << AFIO_PCFR1_TIM1_RM_BIT))
#define AFIO_PCFR1_TIM1_RM_PARTIAL1   ((uint32_t)(0x1UL << AFIO_PCFR1_TIM1_RM_BIT))
#define AFIO_PCFR1_TIM1_RM_PARTIAL2   ((uint32_t)(0x2UL << AFIO_PCFR1_TIM1_RM_BIT))
#define AFIO_PCFR1_TIM1_RM_COMPLETE   ((uint32_t)(0x3UL << AFIO_PCFR1_TIM1_RM_BIT))

// Bits [9:8] TIM2_RM[1:0] — TIM2 remapping
//   00: Default  (CH1/ETR/PD4, CH2/PD3, CH3/PC0, CH4/PD7)
//   01: Partial  (CH1/ETR/PC5, CH2/PC2, CH3/PD2, CH4/PC1)
//   10: Partial  (CH1/ETR/PC1, CH2/PD3, CH3/PC0, CH4/PD7)
//   11: Complete (CH1/ETR/PC1, CH2/PC7, CH3/PD6, CH4/PD5)
#define AFIO_PCFR1_TIM2_RM_BIT        (8U)
#define AFIO_PCFR1_TIM2_RM_MASK       ((uint32_t)(0x3UL << AFIO_PCFR1_TIM2_RM_BIT))
#define AFIO_PCFR1_TIM2_RM_DEFAULT    ((uint32_t)(0x0UL << AFIO_PCFR1_TIM2_RM_BIT))
#define AFIO_PCFR1_TIM2_RM_PARTIAL1   ((uint32_t)(0x1UL << AFIO_PCFR1_TIM2_RM_BIT))
#define AFIO_PCFR1_TIM2_RM_PARTIAL2   ((uint32_t)(0x2UL << AFIO_PCFR1_TIM2_RM_BIT))
#define AFIO_PCFR1_TIM2_RM_COMPLETE   ((uint32_t)(0x3UL << AFIO_PCFR1_TIM2_RM_BIT))

// Bits [14:10] — Reserved, do not modify

// Bit 15 PA12_RM — PA1 and PA2 remapping
//   0: PA1/PA2 used as GPIO and multiplexed function
//   1: PA1/PA2 have no functional role (set when using HSE crystal)
#define AFIO_PCFR1_PA12_RM_BIT        (15U)
#define AFIO_PCFR1_PA12_RM_MASK       ((uint32_t)(1UL << AFIO_PCFR1_PA12_RM_BIT))

// Bit 16 — Reserved, do not modify

// Bit 17 ADC_ETRGINJ_RM — ADC external trigger injected conversion remap
//   0: ADC external trigger injected connected to PD3
//   1: ADC external trigger injected connected to PC2
#define AFIO_PCFR1_ADC_ETRGINJ_RM_BIT  (17U)
#define AFIO_PCFR1_ADC_ETRGINJ_RM_MASK ((uint32_t)(1UL << AFIO_PCFR1_ADC_ETRGINJ_RM_BIT))

// Bit 18 ADC_ETRGREG_RM — ADC external trigger regular conversion remap
//   0: ADC external trigger regular connected to PD3
//   1: ADC external trigger regular connected to PC2
#define AFIO_PCFR1_ADC_ETRGREG_RM_BIT  (18U)
#define AFIO_PCFR1_ADC_ETRGREG_RM_MASK ((uint32_t)(1UL << AFIO_PCFR1_ADC_ETRGREG_RM_BIT))

// Bits [20:19] — Reserved, do not modify

// Bit 21 USART1_RM1 — USART1 remap high bit (used with bit 2)
#define AFIO_PCFR1_USART1_RM1_BIT     (21U)
#define AFIO_PCFR1_USART1_RM1_MASK    ((uint32_t)(1UL << AFIO_PCFR1_USART1_RM1_BIT))

// Bit 22 I2C1REMAP1 — I2C1 remap high bit (used with bit 1)
#define AFIO_PCFR1_I2C1_RM1_BIT       (22U)
#define AFIO_PCFR1_I2C1_RM1_MASK      ((uint32_t)(1UL << AFIO_PCFR1_I2C1_RM1_BIT))

// Bit 23 TIM1_IREMAP — TIM1 channel 1 selection
//   0: External pins
//   1: Internal LSI clock
#define AFIO_PCFR1_TIM1_IREMAP_BIT    (23U)
#define AFIO_PCFR1_TIM1_IREMAP_MASK   ((uint32_t)(1UL << AFIO_PCFR1_TIM1_IREMAP_BIT))

// Bits [26:24] SWCFG[2:0] — SWD debug interface configuration
//   0xx: SWD (SDI) enabled — leave at reset value during normal operation
//   100: Turn off SWD, pins available as GPIO
//   Note: disabling SWD removes your debug connection — do not set
//         this unless you have another way to reprogram the chip
#define AFIO_PCFR1_SWCFG_BIT          (24U)
#define AFIO_PCFR1_SWCFG_MASK         ((uint32_t)(0x7UL << AFIO_PCFR1_SWCFG_BIT))
#define AFIO_PCFR1_SWCFG_SWD_ON       ((uint32_t)(0x0UL << AFIO_PCFR1_SWCFG_BIT))
#define AFIO_PCFR1_SWCFG_SWD_OFF      ((uint32_t)(0x4UL << AFIO_PCFR1_SWCFG_BIT))

// Bits [31:27] — Reserved, do not modify

/*====================================================================
 * AFIO_EXTICR — External Interrupt Configuration Register (offset 0x08)
 * CH32V003 manual section 7.3.2.2
 *
 * Each EXTI line x has a 2-bit field EXTIx[1:0] selecting the port:
 *   00: PA pin x
 *   10: PC pin x
 *   11: PD pin x
 *   01: Reserved
 *
 * Bits [31:16] Reserved.
 *====================================================================*/
#define AFIO_EXTICR_EXTI_WIDTH    (2U)
#define AFIO_EXTICR_EXTI_MASK     (0x3U)

#define AFIO_EXTICR_PORT_PA       (0x0U)
#define AFIO_EXTICR_PORT_PC       (0x2U)
#define AFIO_EXTICR_PORT_PD       (0x3U)

/*====================================================================
 * AFIO_SetExtiPort — route an EXTI line to a specific port
 *
 * Parameters:
 *   exti_line — 0-7
 *   port      — GPIO_PORT_A, GPIO_PORT_C, or GPIO_PORT_D
 *
 * Example: route EXTI3 to PD3
 *   AFIO_SetExtiPort(3U, GPIO_PORT_D);
 *====================================================================*/
static void AFIO_SetExtiPort(uint32_t exti_line, GPIO_Port_t port)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t shift                       = 0U;
    uint32_t port_val                    = 0U;
    uint32_t exticr_val                  = 0U;

    if (exti_line <= 7U)
    {
        shift = exti_line * AFIO_EXTICR_EXTI_WIDTH;

        switch (port)
        {
            case GPIO_PORT_A: port_val = AFIO_EXTICR_PORT_PA; break;
            case GPIO_PORT_C: port_val = AFIO_EXTICR_PORT_PC; break;
            case GPIO_PORT_D: port_val = AFIO_EXTICR_PORT_PD; break;
            default:          port_val = AFIO_EXTICR_PORT_PA; break;
        }

        exticr_val  = (uint32_t)(p_afio->exticr
                       & ~((uint32_t)AFIO_EXTICR_EXTI_MASK << shift));
        exticr_val  = (uint32_t)(exticr_val | (port_val << shift));
        p_afio->exticr = exticr_val;
    }
}

/*====================================================================
 * AFIO_SetUsart1Remap — select USART1 pin mapping
 *
 * The two-bit remap field is split across bits 2 and 21 of PCFR1.
 * This helper writes both bits atomically in a single read-modify-write.
 *
 * remap values (pass the 2-bit value, 0-3):
 *   0: Default  TX/PD5,  RX/PD6
 *   1: Remap 1  TX/PD0,  RX/PD1
 *   2: Remap 2  TX/PD6,  RX/PD5
 *   3: Remap 3  TX/PC0,  RX/PC1
 *====================================================================*/
static void AFIO_SetUsart1Remap(uint32_t remap)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t pcfr1_val                   = 0U;
    uint32_t low_bit                     = 0U;
    uint32_t high_bit                    = 0U;

    low_bit  = (uint32_t)((remap >> 0U) & 1U);
    high_bit = (uint32_t)((remap >> 1U) & 1U);

    pcfr1_val  = p_afio->pcfr1;
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_USART1_RM_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_USART1_RM1_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val | (low_bit  << AFIO_PCFR1_USART1_RM_BIT));
    pcfr1_val  = (uint32_t)(pcfr1_val | (high_bit << AFIO_PCFR1_USART1_RM1_BIT));
    p_afio->pcfr1 = pcfr1_val;
}


/*====================================================================
 * Peripheral pin assignment constants — derived from manual
 * sections 7.2.11.1 through 7.2.11.4
 *
 * These constants document which physical pin each peripheral
 * function lands on for each remap setting. Use them alongside
 * GPIO_SetPinMode to configure pins before enabling the peripheral.
 *====================================================================*/

/*--------------------------------------------------------------------
 * USART1 pin assignments — manual Table 7-10
 *
 * GPIO config required (from manual Table 7-3):
 *   TX  full-duplex:  AF_PP (push-pull multiplexed output)
 *   TX  half-duplex:  AF_OD (open-drain multiplexed output)
 *   RX  full-duplex:  IN_FLOATING or IN_PULLUP
 *   CK  sync mode:    AF_PP
 *   RTS flow control: AF_PP
 *   CTS flow control: IN_FLOATING or IN_PULLUP
 *------------------------------------------------------------------*/

/* USART1_RM=00 — default mapping */
#define USART1_RM00_CK_PORT     GPIO_PORT_D
#define USART1_RM00_CK_PIN      GPIO_PIN_4
#define USART1_RM00_TX_PORT     GPIO_PORT_D
#define USART1_RM00_TX_PIN      GPIO_PIN_5
#define USART1_RM00_RX_PORT     GPIO_PORT_D
#define USART1_RM00_RX_PIN      GPIO_PIN_6
#define USART1_RM00_CTS_PORT    GPIO_PORT_D
#define USART1_RM00_CTS_PIN     GPIO_PIN_3
#define USART1_RM00_RTS_PORT    GPIO_PORT_C
#define USART1_RM00_RTS_PIN     GPIO_PIN_2

/* USART1_RM=01 — partial remapping */
#define USART1_RM01_CK_PORT     GPIO_PORT_D
#define USART1_RM01_CK_PIN      GPIO_PIN_7
#define USART1_RM01_TX_PORT     GPIO_PORT_D
#define USART1_RM01_TX_PIN      GPIO_PIN_0
#define USART1_RM01_RX_PORT     GPIO_PORT_D
#define USART1_RM01_RX_PIN      GPIO_PIN_1
#define USART1_RM01_CTS_PORT    GPIO_PORT_C
#define USART1_RM01_CTS_PIN     GPIO_PIN_3
#define USART1_RM01_RTS_PORT    GPIO_PORT_C
#define USART1_RM01_RTS_PIN     GPIO_PIN_2

/* USART1_RM=10 — partial remapping */
#define USART1_RM10_CK_PORT     GPIO_PORT_D
#define USART1_RM10_CK_PIN      GPIO_PIN_7
#define USART1_RM10_TX_PORT     GPIO_PORT_D
#define USART1_RM10_TX_PIN      GPIO_PIN_6
#define USART1_RM10_RX_PORT     GPIO_PORT_D
#define USART1_RM10_RX_PIN      GPIO_PIN_5
#define USART1_RM10_CTS_PORT    GPIO_PORT_C
#define USART1_RM10_CTS_PIN     GPIO_PIN_6
#define USART1_RM10_RTS_PORT    GPIO_PORT_C
#define USART1_RM10_RTS_PIN     GPIO_PIN_7

/* USART1_RM=11 — full remapping */
#define USART1_RM11_CK_PORT     GPIO_PORT_C
#define USART1_RM11_CK_PIN      GPIO_PIN_5
#define USART1_RM11_TX_PORT     GPIO_PORT_C
#define USART1_RM11_TX_PIN      GPIO_PIN_0
#define USART1_RM11_RX_PORT     GPIO_PORT_C
#define USART1_RM11_RX_PIN      GPIO_PIN_1
#define USART1_RM11_CTS_PORT    GPIO_PORT_C
#define USART1_RM11_CTS_PIN     GPIO_PIN_6
#define USART1_RM11_RTS_PORT    GPIO_PORT_C
#define USART1_RM11_RTS_PIN     GPIO_PIN_7


/*--------------------------------------------------------------------
 * SPI1 pin assignments — manual Table 7-11
 *
 * GPIO config required (from manual Table 7-4):
 *   SCK  master:      AF_PP
 *   SCK  slave:       IN_FLOATING
 *   MOSI full master: AF_PP
 *   MOSI full slave:  IN_FLOATING or IN_PULLUP
 *   MISO full master: IN_FLOATING or IN_PULLUP
 *   MISO full slave:  AF_PP
 *   NSS  hw master:   IN_FLOATING or IN_PULLUP
 *   NSS  hw output:   AF_PP
 *   NSS  software:    not used (configure as regular GPIO if needed)
 *------------------------------------------------------------------*/

/* SPI1_RM=0 — default mapping */
#define SPI1_RM0_NSS_PORT       GPIO_PORT_C
#define SPI1_RM0_NSS_PIN        GPIO_PIN_1
#define SPI1_RM0_SCK_PORT       GPIO_PORT_C
#define SPI1_RM0_SCK_PIN        GPIO_PIN_5
#define SPI1_RM0_MISO_PORT      GPIO_PORT_C
#define SPI1_RM0_MISO_PIN       GPIO_PIN_7
#define SPI1_RM0_MOSI_PORT      GPIO_PORT_C
#define SPI1_RM0_MOSI_PIN       GPIO_PIN_6

/* SPI1_RM=1 — remapping */
#define SPI1_RM1_NSS_PORT       GPIO_PORT_C
#define SPI1_RM1_NSS_PIN        GPIO_PIN_0
#define SPI1_RM1_SCK_PORT       GPIO_PORT_C
#define SPI1_RM1_SCK_PIN        GPIO_PIN_5   /* SCK does not move */
#define SPI1_RM1_MISO_PORT      GPIO_PORT_C
#define SPI1_RM1_MISO_PIN       GPIO_PIN_7   /* MISO does not move */
#define SPI1_RM1_MOSI_PORT      GPIO_PORT_C
#define SPI1_RM1_MOSI_PIN       GPIO_PIN_6   /* MOSI does not move */


/*--------------------------------------------------------------------
 * I2C1 pin assignments — manual Table 7-12
 *
 * GPIO config required (from manual Table 7-5):
 *   SCL: AF_OD (open-drain multiplexed output)
 *   SDA: AF_OD (open-drain multiplexed output)
 *
 * Note: I2C lines require external pull-up resistors on the board.
 * Do NOT use GPIO_MODE_IN_PULLUP — use AF_OD and let the external
 * resistors pull the lines high.
 *------------------------------------------------------------------*/

/* I2C1_RM=00 — default mapping */
#define I2C1_RM00_SCL_PORT      GPIO_PORT_C
#define I2C1_RM00_SCL_PIN       GPIO_PIN_2
#define I2C1_RM00_SDA_PORT      GPIO_PORT_C
#define I2C1_RM00_SDA_PIN       GPIO_PIN_1

/* I2C1_RM=01 — remapping */
#define I2C1_RM01_SCL_PORT      GPIO_PORT_D
#define I2C1_RM01_SCL_PIN       GPIO_PIN_1
#define I2C1_RM01_SDA_PORT      GPIO_PORT_D
#define I2C1_RM01_SDA_PIN       GPIO_PIN_0

/* I2C1_RM=1x — remapping (high bit set) */
#define I2C1_RM1X_SCL_PORT      GPIO_PORT_C
#define I2C1_RM1X_SCL_PIN       GPIO_PIN_5
#define I2C1_RM1X_SDA_PORT      GPIO_PORT_C
#define I2C1_RM1X_SDA_PIN       GPIO_PIN_6


/*--------------------------------------------------------------------
 * TIM1 pin assignments — manual Table 7-8
 *
 * GPIO config required (from manual Table 7-1):
 *   CHx input capture:    IN_FLOATING
 *   CHx output compare:   AF_PP
 *   CHxN complementary:   AF_PP
 *   BKIN brake input:     IN_FLOATING
 *   ETR external trigger: IN_FLOATING
 *------------------------------------------------------------------*/

/* TIM1_RM=00 — default mapping */
#define TIM1_RM00_ETR_PORT      GPIO_PORT_C
#define TIM1_RM00_ETR_PIN       GPIO_PIN_5
#define TIM1_RM00_CH1_PORT      GPIO_PORT_D
#define TIM1_RM00_CH1_PIN       GPIO_PIN_2
#define TIM1_RM00_CH2_PORT      GPIO_PORT_A
#define TIM1_RM00_CH2_PIN       GPIO_PIN_1
#define TIM1_RM00_CH3_PORT      GPIO_PORT_C
#define TIM1_RM00_CH3_PIN       GPIO_PIN_3
#define TIM1_RM00_CH4_PORT      GPIO_PORT_C
#define TIM1_RM00_CH4_PIN       GPIO_PIN_4
#define TIM1_RM00_BKIN_PORT     GPIO_PORT_C
#define TIM1_RM00_BKIN_PIN      GPIO_PIN_2
#define TIM1_RM00_CH1N_PORT     GPIO_PORT_D
#define TIM1_RM00_CH1N_PIN      GPIO_PIN_0
#define TIM1_RM00_CH2N_PORT     GPIO_PORT_A
#define TIM1_RM00_CH2N_PIN      GPIO_PIN_2
#define TIM1_RM00_CH3N_PORT     GPIO_PORT_D
#define TIM1_RM00_CH3N_PIN      GPIO_PIN_1

/* TIM1_RM=01 — partial mapping */
#define TIM1_RM01_ETR_PORT      GPIO_PORT_C
#define TIM1_RM01_ETR_PIN       GPIO_PIN_5
#define TIM1_RM01_CH1_PORT      GPIO_PORT_C
#define TIM1_RM01_CH1_PIN       GPIO_PIN_6
#define TIM1_RM01_CH2_PORT      GPIO_PORT_C
#define TIM1_RM01_CH2_PIN       GPIO_PIN_7
#define TIM1_RM01_CH3_PORT      GPIO_PORT_C
#define TIM1_RM01_CH3_PIN       GPIO_PIN_0
#define TIM1_RM01_CH4_PORT      GPIO_PORT_D
#define TIM1_RM01_CH4_PIN       GPIO_PIN_3
#define TIM1_RM01_BKIN_PORT     GPIO_PORT_C
#define TIM1_RM01_BKIN_PIN      GPIO_PIN_1
#define TIM1_RM01_CH1N_PORT     GPIO_PORT_C
#define TIM1_RM01_CH1N_PIN      GPIO_PIN_3
#define TIM1_RM01_CH2N_PORT     GPIO_PORT_C
#define TIM1_RM01_CH2N_PIN      GPIO_PIN_4
#define TIM1_RM01_CH3N_PORT     GPIO_PORT_D
#define TIM1_RM01_CH3N_PIN      GPIO_PIN_1

/* TIM1_RM=10 — partial mapping */
#define TIM1_RM10_ETR_PORT      GPIO_PORT_D
#define TIM1_RM10_ETR_PIN       GPIO_PIN_4
#define TIM1_RM10_CH1_PORT      GPIO_PORT_D
#define TIM1_RM10_CH1_PIN       GPIO_PIN_2
#define TIM1_RM10_CH2_PORT      GPIO_PORT_A
#define TIM1_RM10_CH2_PIN       GPIO_PIN_1
#define TIM1_RM10_CH3_PORT      GPIO_PORT_C
#define TIM1_RM10_CH3_PIN       GPIO_PIN_3
#define TIM1_RM10_CH4_PORT      GPIO_PORT_C
#define TIM1_RM10_CH4_PIN       GPIO_PIN_4
#define TIM1_RM10_BKIN_PORT     GPIO_PORT_C
#define TIM1_RM10_BKIN_PIN      GPIO_PIN_2
#define TIM1_RM10_CH1N_PORT     GPIO_PORT_D
#define TIM1_RM10_CH1N_PIN      GPIO_PIN_0
#define TIM1_RM10_CH2N_PORT     GPIO_PORT_A
#define TIM1_RM10_CH2N_PIN      GPIO_PIN_2
#define TIM1_RM10_CH3N_PORT     GPIO_PORT_D
#define TIM1_RM10_CH3N_PIN      GPIO_PIN_1

/* TIM1_RM=11 — full mapping */
#define TIM1_RM11_ETR_PORT      GPIO_PORT_C
#define TIM1_RM11_ETR_PIN       GPIO_PIN_2
#define TIM1_RM11_CH1_PORT      GPIO_PORT_C
#define TIM1_RM11_CH1_PIN       GPIO_PIN_4
#define TIM1_RM11_CH2_PORT      GPIO_PORT_C
#define TIM1_RM11_CH2_PIN       GPIO_PIN_7
#define TIM1_RM11_CH3_PORT      GPIO_PORT_C
#define TIM1_RM11_CH3_PIN       GPIO_PIN_5
#define TIM1_RM11_CH4_PORT      GPIO_PORT_D
#define TIM1_RM11_CH4_PIN       GPIO_PIN_4
#define TIM1_RM11_BKIN_PORT     GPIO_PORT_C
#define TIM1_RM11_BKIN_PIN      GPIO_PIN_1
#define TIM1_RM11_CH1N_PORT     GPIO_PORT_C
#define TIM1_RM11_CH1N_PIN      GPIO_PIN_3
#define TIM1_RM11_CH2N_PORT     GPIO_PORT_D
#define TIM1_RM11_CH2N_PIN      GPIO_PIN_2
#define TIM1_RM11_CH3N_PORT     GPIO_PORT_C
#define TIM1_RM11_CH3N_PIN      GPIO_PIN_6


/*--------------------------------------------------------------------
 * TIM2 pin assignments — manual Table 7-9
 *
 * GPIO config required (from manual Table 7-2):
 *   CHx input capture:    IN_FLOATING
 *   CHx output compare:   AF_PP
 *   ETR external trigger: IN_FLOATING
 *------------------------------------------------------------------*/

/* TIM2_RM=00 — default mapping */
#define TIM2_RM00_ETR_PORT      GPIO_PORT_D
#define TIM2_RM00_ETR_PIN       GPIO_PIN_4
#define TIM2_RM00_CH1_PORT      GPIO_PORT_D
#define TIM2_RM00_CH1_PIN       GPIO_PIN_4
#define TIM2_RM00_CH2_PORT      GPIO_PORT_D
#define TIM2_RM00_CH2_PIN       GPIO_PIN_3
#define TIM2_RM00_CH3_PORT      GPIO_PORT_C
#define TIM2_RM00_CH3_PIN       GPIO_PIN_0
#define TIM2_RM00_CH4_PORT      GPIO_PORT_D
#define TIM2_RM00_CH4_PIN       GPIO_PIN_7

/* TIM2_RM=01 — partial mapping */
#define TIM2_RM01_ETR_PORT      GPIO_PORT_C
#define TIM2_RM01_ETR_PIN       GPIO_PIN_5
#define TIM2_RM01_CH1_PORT      GPIO_PORT_C
#define TIM2_RM01_CH1_PIN       GPIO_PIN_5
#define TIM2_RM01_CH2_PORT      GPIO_PORT_C
#define TIM2_RM01_CH2_PIN       GPIO_PIN_2
#define TIM2_RM01_CH3_PORT      GPIO_PORT_D
#define TIM2_RM01_CH3_PIN       GPIO_PIN_2
#define TIM2_RM01_CH4_PORT      GPIO_PORT_C
#define TIM2_RM01_CH4_PIN       GPIO_PIN_1

/* TIM2_RM=10 — partial mapping */
#define TIM2_RM10_ETR_PORT      GPIO_PORT_C
#define TIM2_RM10_ETR_PIN       GPIO_PIN_1
#define TIM2_RM10_CH1_PORT      GPIO_PORT_C
#define TIM2_RM10_CH1_PIN       GPIO_PIN_1
#define TIM2_RM10_CH2_PORT      GPIO_PORT_D
#define TIM2_RM10_CH2_PIN       GPIO_PIN_3
#define TIM2_RM10_CH3_PORT      GPIO_PORT_C
#define TIM2_RM10_CH3_PIN       GPIO_PIN_0
#define TIM2_RM10_CH4_PORT      GPIO_PORT_D
#define TIM2_RM10_CH4_PIN       GPIO_PIN_7

/* TIM2_RM=11 — full mapping */
#define TIM2_RM11_ETR_PORT      GPIO_PORT_C
#define TIM2_RM11_ETR_PIN       GPIO_PIN_1
#define TIM2_RM11_CH1_PORT      GPIO_PORT_C
#define TIM2_RM11_CH1_PIN       GPIO_PIN_1
#define TIM2_RM11_CH2_PORT      GPIO_PORT_C
#define TIM2_RM11_CH2_PIN       GPIO_PIN_7
#define TIM2_RM11_CH3_PORT      GPIO_PORT_D
#define TIM2_RM11_CH3_PIN       GPIO_PIN_6
#define TIM2_RM11_CH4_PORT      GPIO_PORT_D
#define TIM2_RM11_CH4_PIN       GPIO_PIN_5


/*--------------------------------------------------------------------
 * Remaining AFIO remap helpers
 *------------------------------------------------------------------*/

static void AFIO_SetTim1Remap(uint32_t remap)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t pcfr1_val                   = 0U;

    pcfr1_val  = p_afio->pcfr1;
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_TIM1_RM_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val | ((remap << AFIO_PCFR1_TIM1_RM_BIT)
                             & AFIO_PCFR1_TIM1_RM_MASK));
    p_afio->pcfr1 = pcfr1_val;
}

static void AFIO_SetTim2Remap(uint32_t remap)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t pcfr1_val                   = 0U;

    pcfr1_val  = p_afio->pcfr1;
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_TIM2_RM_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val | ((remap << AFIO_PCFR1_TIM2_RM_BIT)
                             & AFIO_PCFR1_TIM2_RM_MASK));
    p_afio->pcfr1 = pcfr1_val;
}

static void AFIO_SetSpi1Remap(uint32_t remap)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t pcfr1_val                   = 0U;

    pcfr1_val  = p_afio->pcfr1;
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_SPI1_RM_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val | ((remap << AFIO_PCFR1_SPI1_RM_BIT)
                             & AFIO_PCFR1_SPI1_RM_MASK));
    p_afio->pcfr1 = pcfr1_val;
}

/*
 * I2C1 remap — two-bit field split across bits 1 and 22.
 * Pass the 2-bit remap value (0-2, value 3 is invalid per manual).
 */
static void AFIO_SetI2c1Remap(uint32_t remap)
{
    volatile AFIO_TypeDef * const p_afio = AFIO_GetBase();
    uint32_t pcfr1_val                   = 0U;
    uint32_t low_bit                     = 0U;
    uint32_t high_bit                    = 0U;

    low_bit  = (uint32_t)((remap >> 0U) & 1U);
    high_bit = (uint32_t)((remap >> 1U) & 1U);

    pcfr1_val  = p_afio->pcfr1;
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_I2C1_RM_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val & ~AFIO_PCFR1_I2C1_RM1_MASK);
    pcfr1_val  = (uint32_t)(pcfr1_val | (low_bit  << AFIO_PCFR1_I2C1_RM_BIT));
    pcfr1_val  = (uint32_t)(pcfr1_val | (high_bit << AFIO_PCFR1_I2C1_RM1_BIT));
    p_afio->pcfr1 = pcfr1_val;
}

#endif // CH32V003_HAL_GPIO_H
# CH32V003 HAL — RCC Documentation

> Reference: CH32V003 Reference Manual V1.9, Chapter 3
> Header: `rcc.h`
> No dependencies — include this first

---

## Table of Contents

1. [Overview](#overview)
2. [The clock tree](#the-clock-tree)
3. [Clock sources](#clock-sources)
4. [Initialisation](#initialisation)
5. [Peripheral clock control](#peripheral-clock-control)
6. [Peripheral reset](#peripheral-reset)
7. [Reset cause](#reset-cause)
8. [LSI oscillator](#lsi-oscillator)
9. [Clock Security System (CSS)](#clock-security-system-css)
10. [MCO — clock output pin](#mco--clock-output-pin)
11. [API reference](#api-reference)
12. [Complete examples](#complete-examples)
13. [Personal notes](#personal-notes)

---

## Overview

RCC (Reset and Clock Control) is the first peripheral you configure
in any program. Nothing else works until you give it a clock. RCC
controls:

- Which clock source drives the CPU and peripherals
- How fast each peripheral runs (via prescalers)
- Whether each peripheral is clocked at all (clock gating)
- Resetting peripherals to their power-on defaults
- Why the chip last restarted (reset cause flags)
- Monitoring HSE for failures (CSS)
- Outputting a clock signal on a GPIO pin (MCO)

**RCC has no clock enable of its own — it is always accessible.**

---

## The clock tree

Understanding the clock tree is the most important thing in this
document. Every other frequency in the system derives from it.

```text
[CLOCK SOURCES]          [PLL]          [SW MUX]       SYSCLK
                                                           │
 128kHz  LSI ────────────────────────────────────────────  │ ──> IWDG, AWU
                                                           │
  24MHz  HSI ──┐                                           │
               ├──> PLLSRC ──> PLL(×2) ──────────────> SW mux ──> SYSCLK
 4-25MHz HSE ──┘                │                          │
                                └──> HSI*2=48MHz           │
  24MHz  HSI ──────────────────────────────────────────> SW mux
 4-25MHz HSE ──────────────────────────────────────────> SW mux
                                                           │
                                                    HPRE prescaler
                                                      (÷1..÷256)
                                                           │
                                                         HCLK ──> all peripherals
                                                           │
                                                   ÷8 or direct
                                                           │
                                                       SysTick clock
```

**Three things to know:**

The PLL only multiplies by 2. There is no other multiplier option.

HCLK is what peripherals actually run at. USART baud rates, SPI
clock speeds, timer frequencies — all derived from HCLK.

At reset: HSI is SYSCLK, HPRE=÷3, so HCLK = 24MHz/3 = 8MHz.
This is the speed your code runs at if you never call `RCC_Init`.

---

## Clock sources

### HSI — High Speed Internal (24MHz)

Internal RC oscillator. On by default at reset. No external components
required. Factory calibrated to 24MHz ±1%. Can be user-trimmed via
`HSITRIM` for voltage or temperature compensation.

HSI is the fallback clock — the hardware automatically switches to HSI
if HSE fails (when CSS is enabled) or when waking from standby.

### HSE — High Speed External (4–25MHz)

External crystal or resonator connected to OSC_IN/OSC_OUT pins. More
accurate than HSI. Off by default. Must wait for `HSERDY=1` before use.

HSE can also be bypassed — connect an external clock source directly
to OSC_IN and leave OSC_OUT floating. Maximum 25MHz in bypass mode.

### LSI — Low Speed Internal (128kHz)

Internal RC oscillator. **Off by default.** Must be explicitly enabled
with `RCC_EnableLSI()` before use. Used exclusively by the IWDG
(independent watchdog) and the PWR auto-wakeup unit (AWU).

The IWDG forces LSI on automatically and it cannot be turned off while
IWDG is active.

LSI accuracy is poor — ±30% over temperature and supply voltage. Do
not use it for timing-critical applications.

### PLL — Phase Locked Loop

Not a clock source itself — it multiplies an input (HSI or HSE) by 2.
PLLSRC selects the input. PLL configuration must be done before enabling
the PLL. Once locked, the configuration cannot be changed.

---

## Initialisation

### The mandatory first call

Every program must call `RCC_Init` or one of its convenience wrappers
before accessing any peripheral. The minimal fast startup:

```c
if (RCC_Init48MHz() != RCC_OK) { for (;;) {} }
```

### What `RCC_Init` does internally

```text
1. Enable HSI and wait for HSIRDY
2. Park SYSCLK on HSI (safe during transition)
3. Configure CFGR0 — PLLSRC and HPRE (PLL must be off here)
4. If HSE needed: enable and wait for HSERDY
5. If PLL needed: enable and wait for PLLRDY
6. Switch SYSCLK to the requested source
7. Poll SWS to confirm the switch completed
```

Step 7 is important — never trust SW alone. SWS is the hardware
confirmation that the switch actually happened. The manual explicitly
warns about this.

### Return codes

| Code | Meaning | What to check |
| --- | --- | --- |
| `RCC_OK` | Success | — |
| `RCC_ERR_HSI` | HSI never became stable | Hardware fault — this should not happen |
| `RCC_ERR_HSE` | HSE never became stable | Crystal not connected, wrong load caps, wrong freq |
| `RCC_ERR_PLL_LOCK` | PLL never locked | Usually an HSE issue upstream |
| `RCC_ERR_SW_SWITCH` | SYSCLK switch not confirmed | Should not happen if steps 1-6 succeeded |

### Pre-built configurations

```c
/* 48MHz: HSI → PLL(×2) → SYSCLK, HCLK = 48MHz */
RCC_Init48MHz();

/* 24MHz: HSI direct → SYSCLK, HCLK = 24MHz */
RCC_Init24MHz();
```

### Custom configuration

```c
/* Example: HSE at 8MHz → PLL(×2) = 16MHz SYSCLK */
RCC_InitTypeDef config = {
    RCC_CFGR0_SW_PLL,       /* SYSCLK source: PLL output */
    RCC_CFGR0_PLLSRC_HSE,   /* PLL input: HSE            */
    RCC_CFGR0_HPRE_DIV1,    /* HCLK = SYSCLK (no divide) */
    8000000U                /* HSE crystal = 8MHz        */
};
if (RCC_Init(&config) != RCC_OK) { for (;;) {} }
```

The `hse_freq_hz` field is required when using HSE — it is stored
internally and used by `RCC_GetHclkHz()` to compute correct baud rates.

---

## Peripheral clock control

Every peripheral starts with its clock gated off. Reading a peripheral's
registers while its clock is disabled always returns 0. You must enable
the clock before configuring the peripheral.

```c
/* Enable clock before doing anything with a peripheral */
RCC_EnableClock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_4, GPIO_MODE_OUT_PP_2);
```

### Peripheral list

| Constant | Bus | Peripheral |
| --- | --- | --- |
| `RCC_PERIPH_DMA1` | AHB | DMA controller |
| `RCC_PERIPH_SRAM` | AHB | SRAM interface (on by default) |
| `RCC_PERIPH_AFIO` | APB2 | Alternate function I/O |
| `RCC_PERIPH_GPIOA` | APB2 | GPIO port A |
| `RCC_PERIPH_GPIOC` | APB2 | GPIO port C |
| `RCC_PERIPH_GPIOD` | APB2 | GPIO port D |
| `RCC_PERIPH_ADC1` | APB2 | ADC |
| `RCC_PERIPH_TIM1` | APB2 | Timer 1 |
| `RCC_PERIPH_SPI1` | APB2 | SPI |
| `RCC_PERIPH_USART1` | APB2 | USART |
| `RCC_PERIPH_TIM2` | APB1 | Timer 2 |
| `RCC_PERIPH_WWDG` | APB1 | Window watchdog |
| `RCC_PERIPH_I2C1` | APB1 | I2C |
| `RCC_PERIPH_PWR` | APB1 | Power control |

### Disabling clocks to save power

Before entering Sleep or Standby mode, disable clocks for peripherals
you do not need. Each enabled peripheral with an active clock draws
leakage current even when idle:

```c
/* Disable unused peripherals before low-power entry */
RCC_DisableClock(RCC_PERIPH_ADC1);
RCC_DisableClock(RCC_PERIPH_SPI1);

PWR_EnterSleep();

/* Re-enable after wakeup if needed */
RCC_EnableClock(RCC_PERIPH_ADC1);
```

---

## Peripheral reset

`RCC_ResetPeripheral` resets a peripheral's registers to their
power-on defaults in hardware. More reliable than manually clearing
every register. The peripheral clock must remain enabled during reset.

```c
/* Reset USART1 to defaults before reconfiguring */
RCC_ResetPeripheral(RCC_PERIPH_USART1);
USART1_Init(&new_config);
```

### When to use peripheral reset

**Changing pin mapping (remap)** — when remapping a peripheral from
one set of pins to another, reset the peripheral first to avoid the
old configuration interfering:

```c
/* Was using SPI1 on default pins, now switching to remap 1 */
RCC_ResetPeripheral(RCC_PERIPH_SPI1);
AFIO_SetSpi1Remap(1U);
/* reconfigure SPI1 for new pins */
```

**Recovering from error state** — if USART enters an overrun or frame
error state that polling cannot clear:

```c
RCC_ResetPeripheral(RCC_PERIPH_USART1);
USART1_Init(&config);  /* reinitialise from clean state */
```

**Reusing a peripheral for a different mode** — if you used TIM1 for
PWM output and now want to use it for input capture:

```c
RCC_ResetPeripheral(RCC_PERIPH_TIM1);
/* configure TIM1 for input capture */
```

### AHB peripherals cannot be reset

DMA1 and SRAM have no reset register on this chip.
`RCC_ResetPeripheral` silently does nothing for these.

---

## Reset cause

At every startup the chip records exactly why it restarted. Read this
at the very beginning of `main()` before anything clears it.

### Bit layout

`RCC_GetResetCause()` returns a `uint32_t` bitmask. Each bit maps to
one reset source. Multiple bits can be set simultaneously.

```text
Bit   Flag constant              Meaning
───   ─────────────────────────  ────────────────────────────────────
 0    RCC_RESET_POWER_ON         POR/PDR — normal power-up or brown-out
 1    RCC_RESET_PIN              NRST pin pulled low externally
 2    RCC_RESET_SOFTWARE         PFIC_SystemReset() was called
 3    RCC_RESET_IWDG             Independent watchdog timed out
 4    RCC_RESET_WWDG             Window watchdog timed out
 5    RCC_RESET_LOW_POWER        Low-power management reset from standby
```

### Common combinations

```text
0x01  POR only
      Normal first power-on after programming or battery connect.

0x03  POR + PIN
      Power-on followed by NRST — reset button held during power-up,
      or the reset line was noisy during startup.

0x09  POR + IWDG
      Watchdog fired and the power then cycled. In the field this
      means the firmware hung. Investigate.

0x04  SOFTWARE only
      Clean software reset — expected after firmware update or a
      deliberate PFIC_SystemReset() call.

0x08  IWDG only
      Watchdog fired without a subsequent power cycle.
      Most concerning flag in production. Log and investigate.
```

### `RCC_CAUSE_IS` — testing individual flags

Use the `RCC_CAUSE_IS` macro to test individual flags without writing
the mask and comparison manually:

```c
/* Without the macro */
if ((cause & (uint32_t)RCC_RESET_IWDG) != 0U) { ... }

/* With the macro — reads as plain English */
if (RCC_CAUSE_IS(cause, RCC_RESET_IWDG)) { ... }
```

Both compile to identical code. The macro form is preferred.

### Usage pattern

```c
int main(void)
{
    /* Read before anything else touches RSTSCKR */
    uint32_t cause = RCC_GetResetCause();
    RCC_ClearResetFlags();

    /* Check each flag */
    if (RCC_CAUSE_IS(cause, RCC_RESET_POWER_ON))  { USART1_SendString("POR\r\n");  }
    if (RCC_CAUSE_IS(cause, RCC_RESET_PIN))        { USART1_SendString("NRST\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_SOFTWARE))   { USART1_SendString("SW\r\n");   }
    if (RCC_CAUSE_IS(cause, RCC_RESET_IWDG))       { USART1_SendString("IWDG\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_WWDG))       { USART1_SendString("WWDG\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_LOW_POWER))  { USART1_SendString("LPWR\r\n"); }

    /* Continue with normal init */
    RCC_Init48MHz();
}
```

### Why IWDG reset matters

In production embedded systems, a watchdog reset flag is a diagnostic
event. It means the CPU was not servicing the watchdog — which means
it was stuck in an unexpected state. Logging this to non-volatile
memory and reporting it on the next USART connection is good practice.

`POWER_ON` will almost always be set alongside another flag on the
first boot after programming — that is normal. The flag to watch in
the field is `IWDG` appearing on a device that should never hang.

---

## LSI oscillator

LSI (128kHz) is the low-speed internal oscillator. **Off by default.**

Required before using the PWR auto-wakeup unit (AWU).

```c
/* Before configuring AWU */
RCC_EnableLSI();

/* Optionally confirm it is stable */
if (RCC_IsLSIReady() == 0U) { /* error */ }

/* Now configure AWU */
PWR_ConfigAWU(PWR_AWUPSC_DIV4096, 63U);
```

The IWDG forces LSI on automatically — you do not need to call
`RCC_EnableLSI()` if the IWDG is already running.

LSI is **not accurate** — ±30% over temperature. Do not use AWU timing
values as real-time measurements.

---

## Clock Security System (CSS)

CSS monitors HSE while it is running. If HSE fails mid-operation:

```text
Hardware automatically:
  1. Switches SYSCLK to HSI
  2. Turns off HSE and PLL
  3. Sets CSSF flag in RCC_INTR
  4. Fires NMI (non-maskable interrupt)
```

Your NMI handler must respond:

```c
/* Override the weak NMI_Handler from startup.S */
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void NMI_Handler(void)
{
    if (RCC_IsCSSFlagSet() != 0U)
    {
        RCC_ClearCSSFlag();   /* clears CSSF and dismisses the NMI pending bit */

        /* SYSCLK is now HSI at 24MHz.
         * HCLK depends on what HPRE was set to.
         * Peripherals may be running at wrong speeds — reinitialise: */
        RCC_Init24MHz();
        USART1_Init_115200();   /* re-init baud rate for new clock speed */

        /* Optionally: signal application layer about clock failure */
    }
}

/* In main setup: */
RCC_EnableCSS();   /* call after HSE is stable (after RCC_Init with HSE) */
```

CSS is only useful if you are using HSE. It adds no value on an
HSI-only system since HSI is what CSS falls back to.

---

## MCO — clock output pin

MCO outputs a copy of one of the internal clocks on a GPIO pin.
Useful for:

- Verifying your clock configuration with an oscilloscope
- Providing a clock signal to external devices
- Debugging timing issues

The MCO pin is **PA8**. Configure it as AF_PP_50 before enabling MCO:

```c
RCC_EnableClock(RCC_PERIPH_GPIOA);
GPIO_SetPinMode(GPIO_PORT_A, GPIO_PIN_8, GPIO_MODE_AF_PP_50);

/* Output PLL clock (48MHz) — verify with oscilloscope */
RCC_SetMCO(RCC_MCO_PLL);

/* Disable MCO output */
RCC_SetMCO(RCC_MCO_NONE);
```

| Source | Frequency at 48MHz config |
| --- | --- |
| `RCC_MCO_SYSCLK` | 48MHz |
| `RCC_MCO_HSI` | 24MHz (always) |
| `RCC_MCO_HSE` | Your crystal frequency |
| `RCC_MCO_PLL` | 48MHz (HSI×2) |
| `RCC_MCO_NONE` | Output disabled |

---

## API reference

### `RCC_Init`

```c
static RCC_Result_t RCC_Init(const RCC_InitTypeDef * const p_config);
```

Full clock initialisation. Handles HSI assertion, optional HSE startup,
optional PLL lock, SW mux switch, and SWS confirmation. Returns
`RCC_OK` on success.

---

### `RCC_Init48MHz` / `RCC_Init24MHz`

```c
static RCC_Result_t RCC_Init48MHz(void);
static RCC_Result_t RCC_Init24MHz(void);
```

Convenience wrappers for the two most common configurations.

---

### `RCC_GetSysclkHz`

```c
static uint32_t RCC_GetSysclkHz(void);
```

Returns the raw SYSCLK frequency in Hz before the HPRE prescaler.
Returns `RCC_FREQ_UNKNOWN` if HSE is active but its frequency was not
provided in `RCC_InitTypeDef.hse_freq_hz`.

---

### `RCC_GetHclkHz`

```c
static uint32_t RCC_GetHclkHz(void);
```

Returns HCLK — the frequency that all peripherals actually run at.
This is what you pass to baud rate calculations. Returns
`RCC_FREQ_UNKNOWN` if SYSCLK is unknown.

---

### `RCC_EnableClock` / `RCC_DisableClock` / `RCC_IsClockEnabled`

```c
static void     RCC_EnableClock(RCC_Periph_t periph);
static void     RCC_DisableClock(RCC_Periph_t periph);
static uint32_t RCC_IsClockEnabled(RCC_Periph_t periph);
```

Enable, disable, or check the clock for a peripheral.
`RCC_EnableClock` must be called before accessing any peripheral register.

---

### `RCC_ResetPeripheral`

```c
static void RCC_ResetPeripheral(RCC_Periph_t periph);
```

Asserts then releases the hardware reset for a peripheral. All registers
return to power-on defaults. The peripheral clock must be enabled.
No-op for AHB peripherals (DMA1, SRAM).

---

### `RCC_GetResetCause` / `RCC_ClearResetFlags` / `RCC_CAUSE_IS`

```c
static uint32_t RCC_GetResetCause(void);
static void     RCC_ClearResetFlags(void);
#define         RCC_CAUSE_IS(cause, flag)
```

`RCC_GetResetCause` returns a bitmask of `RCC_ResetCause_t` values.
`RCC_ClearResetFlags` clears all flags. Always read cause before clearing.
`RCC_CAUSE_IS` tests a single flag in a cause bitmask — preferred over
writing the mask and comparison manually. See the Reset cause section
for the full bit layout and common combinations.

---

### `RCC_EnableLSI` / `RCC_DisableLSI` / `RCC_IsLSIReady`

```c
static void     RCC_EnableLSI(void);
static void     RCC_DisableLSI(void);
static uint32_t RCC_IsLSIReady(void);
```

Control the 128kHz internal RC oscillator. Required before using AWU.
`RCC_EnableLSI` blocks until `LSIRDY=1`.

---

### `RCC_EnableCSS` / `RCC_DisableCSS` / `RCC_IsCSSFlagSet` / `RCC_ClearCSSFlag`

```c
static void     RCC_EnableCSS(void);
static void     RCC_DisableCSS(void);
static uint32_t RCC_IsCSSFlagSet(void);
static void     RCC_ClearCSSFlag(void);
```

Clock Security System control. Only relevant when using HSE.
`RCC_ClearCSSFlag` must be called inside `NMI_Handler`.

---

### `RCC_SetMCO`

```c
static void RCC_SetMCO(RCC_MCOSource_t source);
```

Sets the MCO pin output. GPIO PA8 must be configured as `AF_PP_50`
first. Pass `RCC_MCO_NONE` to disable.

---

## Complete examples

### Standard 48MHz startup with reset cause logging

```c
#include "rcc.h"
#include "usart.h"

int main(void)
{
    /* Read reset cause before anything clears it */
    uint32_t cause = RCC_GetResetCause();
    RCC_ClearResetFlags();

    /* Init clock */
    if (RCC_Init48MHz() != RCC_OK) { for (;;) {} }

    /* Init USART for logging */
    RCC_EnableClock(RCC_PERIPH_AFIO);
    RCC_EnableClock(RCC_PERIPH_GPIOD);
    RCC_EnableClock(RCC_PERIPH_USART1);
    GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);
    USART1_Init_115200();

    /* Log reset cause — RCC_CAUSE_IS tests a single flag cleanly */
    if (RCC_CAUSE_IS(cause, RCC_RESET_POWER_ON))  { USART1_SendString("POR\r\n");  }
    if (RCC_CAUSE_IS(cause, RCC_RESET_PIN))        { USART1_SendString("NRST\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_SOFTWARE))   { USART1_SendString("SW\r\n");   }
    if (RCC_CAUSE_IS(cause, RCC_RESET_IWDG))       { USART1_SendString("IWDG\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_WWDG))       { USART1_SendString("WWDG\r\n"); }
    if (RCC_CAUSE_IS(cause, RCC_RESET_LOW_POWER))  { USART1_SendString("LPWR\r\n"); }

    for (;;) { WFI(); }
}
```

---

### Remapping a peripheral at runtime

```c
/* SPI1 was on default pins, remap to alternate pins */

/* 1. Disable SPI1 peripheral */
RCC_DisableClock(RCC_PERIPH_SPI1);

/* 2. Re-enable to get clean state, then reset */
RCC_EnableClock(RCC_PERIPH_SPI1);
RCC_ResetPeripheral(RCC_PERIPH_SPI1);

/* 3. Change AFIO remap */
RCC_EnableClock(RCC_PERIPH_AFIO);
AFIO_SetSpi1Remap(1U);

/* 4. Reconfigure GPIO for new pins */
GPIO_SetPinMode(SPI1_RM1_SCK_PORT,  SPI1_RM1_SCK_PIN,  GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM1_MOSI_PORT, SPI1_RM1_MOSI_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM1_MISO_PORT, SPI1_RM1_MISO_PIN, GPIO_MODE_IN_FLOATING);

/* 5. Reinitialise SPI1 */
```

---

### Periodic standby wakeup with AWU

```c
#include "rcc.h"
#include "pwr.h"

int main(void)
{
    if (RCC_Init48MHz() != RCC_OK) { for (;;) {} }
    RCC_EnableClock(RCC_PERIPH_PWR);

    /* LSI must be running before AWU can work */
    RCC_EnableLSI();

    /* Configure AWU for ~2 second wakeup */
    PWR_ConfigAWU(PWR_AWUPSC_DIV4096, 63U);

    for (;;)
    {
        /* Do work... */

        /* Disable unused clocks before sleep */
        RCC_DisableClock(RCC_PERIPH_USART1);

        PWR_EnterStandby();

        /* Woke up — HSI active, re-init */
        RCC_Init48MHz();
        RCC_EnableClock(RCC_PERIPH_USART1);
    }
}
```

---

### Verifying clock config with MCO

```c
/* Connect oscilloscope to PA8 to verify 48MHz */
RCC_EnableClock(RCC_PERIPH_GPIOA);
GPIO_SetPinMode(GPIO_PORT_A, GPIO_PIN_8, GPIO_MODE_AF_PP_50);
RCC_SetMCO(RCC_MCO_PLL);

/* Scope should show ~48MHz square wave */
/* Once verified, disable to stop wasting power */
RCC_SetMCO(RCC_MCO_NONE);
```

---

## Personal notes

**Always read reset cause before calling `RCC_Init`.** `RCC_Init`
does not clear RSTSCKR, but some versions of startup code might.
The safe pattern is `RCC_GetResetCause()` → `RCC_ClearResetFlags()`
→ `RCC_Init()` at the top of `main()`.

**`PORRSTF` is always set on first boot after programming.** It is
not a sign of a problem. It becomes significant when you see it set
alongside `IWDGRSTF` or `WWDGRSTF` after the device has been running
in the field — that combination means the watchdog fired and then the
device fully power-cycled, which is unusual.

**`RCC_FREQ_UNKNOWN` propagates.** If `RCC_GetHclkHz` returns
`RCC_FREQ_UNKNOWN` and you pass it to `USART1_Init`, the baud rate
calculation returns `USART_ERR_BAUD`. This is intentional — it forces
you to fix the clock setup rather than silently running at the wrong
baud rate.

**Peripheral reset is underused but genuinely useful.** Most HAL
code just reconfigures peripherals register by register without
resetting them first. This leaves stale bits from previous
configurations in place. `RCC_ResetPeripheral` is the correct way to
start fresh. Always use it before reconfiguring a peripheral for a
different mode or remapping its pins.

**LSI is not suitable for real-time applications.** Its ±30%
tolerance means a programmed 1-second AWU interval can fire anywhere
between 700ms and 1300ms at temperature extremes. For accurate
periodic operation at low power, consider an external 32.768kHz
crystal on a larger chip. On the CH32V003, LSI is good enough for
"wake up roughly every N seconds to take a reading."

**MCO is the fastest way to verify clock configuration.** Before
trusting that your 48MHz config actually worked, output it on PA8
and check with a scope or frequency counter. A misconfigured PLL
that appears to work at low baud rates will fail at high speeds
in ways that are hard to debug without this verification step.

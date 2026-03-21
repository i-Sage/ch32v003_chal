# CH32V003 HAL Documentation

- Target: CH32V003 · RISC-V RV32EC
- Headers: `rcc.h`, `systick.h`

---

## Table of Contents

1. [Clock Architecture Overview](#clock-architecture-overview)
2. [rcc.h — Reset and Clock Control](#rcch--reset-and-clock-control)
   - [Types](#rcc-types)
   - [Constants](#rcc-constants)
   - [Pre-built Configurations](#rcc-pre-built-configurations)
   - [Initialisation Functions](#rcc-initialisation-functions)
   - [Clock Query Functions](#rcc-clock-query-functions)
   - [Peripheral Clock Control](#rcc-peripheral-clock-control)
3. [systick.h — System Tick Timer](#systickh--system-tick-timer)
   - [Types](#systick-types)
   - [Pre-built Configurations](#systick-pre-built-configurations)
   - [Initialisation Functions](#systick-initialisation-functions)
   - [Counter Access](#systick-counter-access)
   - [Time Conversion](#systick-time-conversion)
   - [Delay Functions](#systick-delay-functions)
   - [Timeout API](#systick-timeout-api)
4. [Quick Start Examples](#quick-start-examples)
5. [Dependency Map](#dependency-map)

---

## Clock Architecture Overview

```
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
```

**Reset state:** HSI direct, HPRE=÷3, so HCLK=8MHz and SysTick=1MHz.
After `RCC_Init_48mhz()`: SYSCLK=48MHz, HCLK=48MHz, SysTick=6MHz (STCLK=0).

---

## rcc.h — Reset and Clock Control

Include: `#include "rcc.h"`
Manual reference: CH32V003 Reference Manual Chapter 3

---

### RCC Types

#### `RCC_Result_t`

Return code for all RCC init functions.

```c
typedef enum {
    RCC_OK            = 0U,   /* Success                        */
    RCC_ERR_HSI       = 1U,   /* HSI oscillator never stabilised */
    RCC_ERR_PLL_LOCK  = 2U,   /* PLL never achieved lock         */
    RCC_ERR_SW_SWITCH = 3U    /* SYSCLK mux switch not confirmed */
} RCC_Result_t;
```

#### `RCC_InitTypeDef`

Configuration struct passed to `RCC_Init`. Use the pre-built configs instead of
constructing this manually unless you need a custom frequency.

```c
typedef struct {
    uint32_t sysclk_source;   /* RCC_CFGR0_SW_HSI / _HSE / _PLL */
    uint32_t pll_source;      /* RCC_CFGR0_PLLSRC_HSI / _HSE    */
    uint32_t hpre;            /* RCC_CFGR0_HPRE_DIVx             */
} RCC_InitTypeDef;
```

#### `RCC_Periph_t`

Encodes a peripheral's clock enable register and bit position into a single
value. Used exclusively with `RCC_Enable_Clock`, `RCC_Disable_Clock`,
and `RCC_Is_Clock_Enabled`.

```c
typedef enum {
    /* AHB bus — RCC_AHBPCENR */
    RCC_PERIPH_DMA1,
    RCC_PERIPH_SRAM,

    /* APB2 bus — RCC_APB2PCENR */
    RCC_PERIPH_AFIO,
    RCC_PERIPH_GPIOA,
    RCC_PERIPH_GPIOC,
    RCC_PERIPH_GPIOD,
    RCC_PERIPH_ADC1,
    RCC_PERIPH_TIM1,
    RCC_PERIPH_SPI1,
    RCC_PERIPH_USART1,

    /* APB1 bus — RCC_APB1PCENR */
    RCC_PERIPH_TIM2,
    RCC_PERIPH_WWDG,
    RCC_PERIPH_I2C1,
    RCC_PERIPH_PWR
} RCC_Periph_t;
```

---

### RCC Constants

| Constant | Value | Meaning |
|---|---|---|
| `RCC_INIT_TIMEOUT` | `10000U` | Max iterations waiting for hardware ready flags |
| `RCC_FREQ_UNKNOWN` | `0xFFFFFFFFU` | Returned when frequency cannot be determined (e.g. HSE not configured) |

---

### RCC Pre-built Configurations

#### `RCC_Config_48MHz`

HSI → PLL (×2) → SYSCLK. HCLK = SYSCLK = 48MHz. No prescaler.

```c
static const RCC_InitTypeDef RCC_Config_48MHz;
```

#### `RCC_Config_24MHz`

HSI direct → SYSCLK. HCLK = SYSCLK = 24MHz. No prescaler.

```c
static const RCC_InitTypeDef RCC_Config_24MHz;
```

---

### RCC Initialisation Functions

#### `RCC_Init`

```c
static RCC_Result_t RCC_Init(const RCC_InitTypeDef * const p_config);
```

Core clock initialisation. Follows the sequence:
1. Assert and confirm HSI stable (HSIRDY)
2. Configure CFGR0 — park on HSI, write PLLSRC and HPRE
3. If PLL requested: enable PLLON, wait for PLLRDY
4. Switch SW mux to requested source
5. Confirm switch via SWS (read-only hardware mirror of active source)

Returns `RCC_OK` on success, error code on any step failure.
On any error, SYSCLK remains on HSI — the chip is never left without a clock.

**Parameters:**
- `p_config` — pointer to a filled `RCC_InitTypeDef`. Must not be NULL.

---

#### `RCC_Init_48mhz`

```c
static RCC_Result_t RCC_Init_48mhz(void);
```

Convenience wrapper. Calls `RCC_Init(&RCC_Config_48MHz)`.

**Example:**
```c
RCC_Result_t result = RCC_Init_48mhz();
if (result != RCC_OK) {
    for (;;) { /* safe error trap — still running on HSI */ }
}
```

---

#### `RCC_Init_24mhz`

```c
static RCC_Result_t RCC_Init_24mhz(void);
```

Convenience wrapper. Calls `RCC_Init(&RCC_Config_24MHz)`.

---

### RCC Clock Query Functions

#### `RCC_Get_Sysclk_Hz`

```c
static uint32_t RCC_Get_Sysclk_Hz(void);
```

Returns the raw SYSCLK frequency in Hz — **before** the HPRE prescaler.
This is not what peripherals see. Use `RCC_Get_Hclk_Hz` for peripheral timing.

| Active source | Return value |
|---|---|
| HSI | `24000000` |
| PLL (HSI×2) | `48000000` |
| HSE | `RCC_FREQ_UNKNOWN` |

---

#### `RCC_Get_Hclk_Hz`

```c
static uint32_t RCC_Get_Hclk_Hz(void);
```

Returns HCLK in Hz — SYSCLK after the HPRE prescaler. This is what all
peripherals, DMA, and SRAM actually run at.

Returns `RCC_FREQ_UNKNOWN` if SYSCLK source is HSE and HSE frequency is
not yet known.

**Example:**
```c
uint32_t hclk = RCC_Get_Hclk_Hz();
/* After RCC_Init_48mhz(): hclk = 48000000 */
```

---

### RCC Peripheral Clock Control

All peripherals must have their clock enabled before their registers can
be accessed. Writing to a peripheral with its clock disabled produces
no effect or a bus fault.

#### `RCC_Enable_Clock`

```c
static void RCC_Enable_Clock(RCC_Periph_t periph);
```

Enables the clock for the specified peripheral.

**Example:**
```c
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
RCC_Enable_Clock(RCC_PERIPH_USART1);
RCC_Enable_Clock(RCC_PERIPH_TIM1);
```

---

#### `RCC_Disable_Clock`

```c
static void RCC_Disable_Clock(RCC_Periph_t periph);
```

Disables the clock for the specified peripheral. Use for power saving when
a peripheral is not needed.

---

#### `RCC_Is_Clock_Enabled`

```c
static uint32_t RCC_Is_Clock_Enabled(RCC_Periph_t periph);
```

Returns `1U` if the peripheral clock is enabled, `0U` if disabled.

**Example:**
```c
if (RCC_Is_Clock_Enabled(RCC_PERIPH_GPIOD) != 0U) {
    /* safe to configure GPIOD registers */
}
```

---

## systick.h — System Tick Timer

Include: `#include "systick.h"` (which includes `rcc.h` automatically)
Manual reference: CH32V003 Reference Manual Chapter 4

> **Clock note:** SysTick is fed by HCLK÷8 by default (STCLK=0).
> At 48MHz HCLK: SysTick = 6MHz.
> At 24MHz HCLK: SysTick = 3MHz.
> Set STCLK=1 via `STK_Config_HighRes` for HCLK direct.

---

### SysTick Types

#### `SysTick_InitTypeDef`

Configuration struct for `SysTick_Init`.

```c
typedef struct {
    STK_ClkSrc_t clk_src;    /* STCLK — clock source  */
    STK_Intr_t   interrupt;  /* STIE  — IRQ enable    */
    STK_Reload_t reload;     /* STRE  — auto-reload   */
    STK_Swie_t   swie;       /* SWIE  — software IRQ  */
} SysTick_InitTypeDef;
```

**`STK_ClkSrc_t`**

| Value | Meaning |
|---|---|
| `STK_CLKSRC_HCLK_DIV8` | SysTick clock = HCLK ÷ 8 (default) |
| `STK_CLKSRC_HCLK` | SysTick clock = HCLK direct (higher resolution) |

**`STK_Intr_t`**

| Value | Meaning |
|---|---|
| `STK_INTR_DISABLE` | No interrupt on compare match |
| `STK_INTR_ENABLE` | Interrupt fires on compare match |

**`STK_Reload_t`**

| Value | Meaning |
|---|---|
| `STK_RELOAD_DISABLE` | Counter counts up forever (free-running) |
| `STK_RELOAD_ENABLE` | Counter resets to 0 on compare match |

**`STK_Swie_t`**

| Value | Meaning |
|---|---|
| `STK_SWIE_DISABLE` | Software interrupt disabled |
| `STK_SWIE_ENABLE` | Software interrupt enabled |

---

#### `Timeout_t`

Opaque timeout handle. Create with `Timeout_New_Ms` or `Timeout_New_Us`.
Do not access fields directly.

```c
typedef struct {
    uint64_t start;           /* counter snapshot at creation */
    uint64_t duration_ticks;  /* total duration in ticks      */
} Timeout_t;
```

---

### SysTick Pre-built Configurations

#### `STK_Config_FreeRunning`

Free-running counter, HCLK÷8, no interrupts.
**Use for:** delays, timeouts, elapsed time.

#### `STK_Config_Periodic`

Auto-reload with interrupt, HCLK÷8.
**Use for:** RTOS tick, periodic callbacks.
Set compare value with `SysTick_SetCompareValue` before calling init.

#### `STK_Config_HighRes`

Free-running counter, HCLK direct (÷1), no interrupts.
**Use for:** tight timing measurements, profiling.

---

### SysTick Initialisation Functions

#### `SysTick_Init`

```c
static void SysTick_Init(const SysTick_InitTypeDef * const p_config);
```

Initialises SysTick. Sequence:
1. Stop counter (clear STE)
2. Reset counter registers to 0
3. Build config from struct
4. Write config and start counter in single write

Must be called after `RCC_Init_*` — SysTick clock frequency depends on HCLK.

---

#### `SysTick_Init_FreeRunning`

```c
static void SysTick_Init_FreeRunning(void);
```

Convenience wrapper. Calls `SysTick_Init(&STK_Config_FreeRunning)`.
Correct starting point for delays and timeouts.

---

#### `SysTick_Init_Periodic`

```c
static void SysTick_Init_Periodic(void);
```

Convenience wrapper. Calls `SysTick_Init(&STK_Config_Periodic)`.
Set compare value **before** calling this.

**Example — 1kHz periodic tick at 48MHz:**
```c
/* 6MHz SysTick / 6000 ticks = 1kHz */
SysTick_SetCompareValue(6000U);
SysTick_Init_Periodic();
```

---

#### `SysTick_Init_HighRes`

```c
static void SysTick_Init_HighRes(void);
```

Convenience wrapper. Calls `SysTick_Init(&STK_Config_HighRes)`.

---

### SysTick Counter Access

#### `SysTick_GetClockHz`

```c
static uint32_t SysTick_GetClockHz(void);
```

Returns the actual SysTick clock frequency in Hz by reading STCLK from
STK_CTLR and applying either ÷8 or ÷1 to HCLK.

Returns `RCC_FREQ_UNKNOWN` if HCLK is unknown.

| Config | HCLK | SysTick clock |
|---|---|---|
| FreeRunning / Periodic | 48MHz | 6MHz |
| FreeRunning / Periodic | 24MHz | 3MHz |
| HighRes | 48MHz | 48MHz |
| HighRes | 24MHz | 24MHz |

---

#### `SysTick_CountValue`

```c
static uint64_t SysTick_CountValue(void);
```

Returns the current 64-bit counter value. Uses a double-read guard to
protect against rollover of the low 32 bits between the two register reads:

```
read cntr_hi  (first)
read cntr_low
read cntr_hi  (second)
if hi1 == hi2: no rollover — result is valid
else: retry
```

Counter increments continuously once SysTick is started.

---

#### `SysTick_CompareValue`

```c
static uint64_t SysTick_CompareValue(void);
```

Returns the current 64-bit compare register value. No double-read guard
needed — compare registers are written only by software, never by hardware.

---

#### `SysTick_SetCompareValue`

```c
static void SysTick_SetCompareValue(uint64_t compare_value);
```

Writes a new 64-bit compare value. Writes the high word first to prevent
a spurious compare match during the two-register write sequence.

---

### SysTick Time Conversion

All conversion functions return `0` if the clock frequency is unknown or
below the minimum required for the requested unit.

#### `SysTick_GetClockHz` — see Counter Access above.

#### `SysTick_Ticks_To_Us`

```c
static uint64_t SysTick_Ticks_To_Us(uint64_t ticks);
```

Converts a tick count to microseconds. Requires SysTick clock >= 1MHz.

#### `SysTick_Ticks_To_Ms`

```c
static uint64_t SysTick_Ticks_To_Ms(uint64_t ticks);
```

Converts a tick count to milliseconds. Requires SysTick clock >= 1kHz.

#### `SysTick_Us_To_Ticks`

```c
static uint64_t SysTick_Us_To_Ticks(uint32_t us);
```

Converts microseconds to ticks. Returns 0 if clock < 1MHz.

**Supported at 48MHz HCLK (STCLK=0):** 6 ticks per microsecond.

#### `SysTick_Ms_To_Ticks`

```c
static uint64_t SysTick_Ms_To_Ticks(uint32_t ms);
```

Converts milliseconds to ticks. Returns 0 if clock < 1kHz.

**Supported at 48MHz HCLK (STCLK=0):** 6000 ticks per millisecond.

#### `SysTicks_Elapsed`

```c
static uint64_t SysTicks_Elapsed(uint64_t since);
```

Returns the number of ticks elapsed since `since`. Unsigned wraparound is
intentional — correct across counter rollover (which occurs after ~97,000
years at 6MHz).

```c
uint64_t start   = SysTick_CountValue();
do_something();
uint64_t elapsed = SysTicks_Elapsed(start);
```

---

### SysTick Delay Functions

All delay functions are busy-wait. The CPU spins until the requested time
has elapsed. Do not use in interrupt context.

#### `SysTick_Delay_Ticks`

```c
static void SysTick_Delay_Ticks(uint64_t ticks);
```

Busy-waits for exactly `ticks` SysTick clock cycles. Returns immediately
if `ticks == 0`. All other delay functions are built on top of this.

#### `SysTick_Delay_Ms`

```c
static void SysTick_Delay_Ms(uint32_t ms);
```

Busy-waits for `ms` milliseconds.

**Example:**
```c
SysTick_Delay_Ms(500U);   /* 500ms delay */
```

#### `SysTick_Delay_Us`

```c
static void SysTick_Delay_Us(uint32_t us);
```

Busy-waits for `us` microseconds.

**Example:**
```c
SysTick_Delay_Us(100U);   /* 100µs delay */
```

---

### SysTick Timeout API

Non-blocking timeout — check expiry in a loop rather than blocking the CPU.

#### `Timeout_New_Ms`

```c
static Timeout_t Timeout_New_Ms(uint32_t ms);
```

Creates a new timeout that expires `ms` milliseconds from now.
Captures the current counter value at creation time.

#### `Timeout_New_Us`

```c
static Timeout_t Timeout_New_Us(uint32_t us);
```

Creates a new timeout that expires `us` microseconds from now.

#### `Timeout_Is_Expired`

```c
static uint32_t Timeout_Is_Expired(const Timeout_t * const p_timeout);
```

Returns `1U` if the timeout has expired, `0U` if still running.

#### `Timeout_Remaining_Ticks`

```c
static uint64_t Timeout_Remaining_Ticks(const Timeout_t * const p_timeout);
```

Returns remaining ticks. Saturates at `0` — never wraps if already expired.

#### `Timeout_Remaining_Us`

```c
static uint64_t Timeout_Remaining_Us(const Timeout_t * const p_timeout);
```

Returns remaining time in microseconds.

#### `Timeout_Remaining_Ms`

```c
static uint64_t Timeout_Remaining_Ms(const Timeout_t * const p_timeout);
```

Returns remaining time in milliseconds.

#### `Timeout_Restart`

```c
static void Timeout_Restart(Timeout_t * const p_timeout);
```

Resets the start timestamp without changing the duration. The timeout
begins counting again from the moment `Timeout_Restart` is called.

---

## Quick Start Examples

### Minimal startup — 48MHz with delay

```c
#include "rcc.h"
#include "systick.h"

int main(void)
{
    /* 1. Clock — must come first */
    RCC_Result_t result = RCC_Init_48mhz();
    if (result != RCC_OK)
    {
        for (;;) { /* clock failed — safe trap */ }
    }

    /* 2. Enable peripheral clocks before touching their registers */
    RCC_Enable_Clock(RCC_PERIPH_GPIOD);

    /* 3. SysTick — must come after RCC so frequency is known */
    SysTick_Init_FreeRunning();

    /* 4. Application loop */
    for (;;)
    {
        SysTick_Delay_Ms(500U);
        /* toggle GPIO here */
    }
}
```

---

### Non-blocking timeout — sensor poll with expiry

```c
Timeout_t sensor_timeout = Timeout_New_Ms(250U);
uint32_t  sensor_ready   = 0U;

while ((Timeout_Is_Expired(&sensor_timeout) == 0U) && (sensor_ready == 0U))
{
    if (sensor_data_available() != 0U)
    {
        sensor_ready = 1U;
    }
}

if (sensor_ready == 0U)
{
    /* timed out — handle error */
}
```

---

### Elapsed time measurement

```c
uint64_t start   = 0U;
uint64_t elapsed = 0U;
uint64_t us      = 0U;

start   = SysTick_CountValue();
run_algorithm();
elapsed = SysTicks_Elapsed(start);
us      = SysTick_Ticks_To_Us(elapsed);
/* us now holds the algorithm execution time in microseconds */
```

---

### Periodic SysTick interrupt at 1kHz

```c
/* SysTick clock = 6MHz at 48MHz HCLK with STCLK=0
 * 6MHz / 6000 = 1kHz tick rate                      */
SysTick_SetCompareValue(6000U);
SysTick_Init_Periodic();

/* ISR — name depends on your startup/vector table */
void SysTick_Handler(void)
{
    /* clear CNTIF flag — write 0 to STK_SR bit 0 */
    volatile SysTick_TypeDef * const p_stk = SysTick_GetBase();
    p_stk->sr = (uint32_t)(p_stk->sr & ~STK_SR_CNTIF_MASK);

    /* your 1ms tick handler here */
}
```

---

### Check reset cause at startup

```c
void check_reset_cause(void)
{
    volatile RCC_TypeDef * const p_rcc = RCC_GetBase();
    uint32_t sckr                      = p_rcc->sckr;

    if ((sckr & RCC_RSTSCKR_IWDGRSTF_MASK) != 0U) {
        /* watchdog reset — software failed to service IWDG */
        /* log fault, enter safe state */
    }
    if ((sckr & RCC_RSTSCKR_PORRSTF_MASK) != 0U) {
        /* normal power-on reset */
    }

    /* clear all flags */
    p_rcc->sckr = (uint32_t)(p_rcc->sckr |  RCC_RSTSCKR_RMVF_MASK);
    p_rcc->sckr = (uint32_t)(p_rcc->sckr & ~RCC_RSTSCKR_RMVF_MASK);
}
```

---

## Dependency Map

```
systick.h
    └── rcc.h
            └── <stdint.h>
```

`rcc.h` has no internal dependencies.
`systick.h` includes `rcc.h` for `RCC_Get_Hclk_Hz()` and `RCC_FREQ_UNKNOWN`.
Neither header includes any standard library header other than `<stdint.h>`.
No dynamic memory. No floating point. No OS dependency.

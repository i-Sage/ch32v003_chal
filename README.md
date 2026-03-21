# ch32v003_hal

A WORK IN PROGRESS

A bare-metal Hardware Abstraction Layer for the **WCH CH32V003** microcontroller,
written in C11 and targeting compliance with **MISRA C:2012**.

---

## What is this

The CH32V003 is a low-cost RISC-V (RV32EC) microcontroller from WCH with 16KB
flash and 2KB RAM. This HAL provides a clean, auditable, safety-oriented driver
layer for its peripherals — starting from the clock system and working outward.

The goal is not to replicate the vendor SDK. The goal is to write C code that
you can read, understand, audit, and trust.

---

## Hardware

| Property | Value |
|---|---|
| Core | RISC-V RV32EC (QingKeV2) |
| Flash | 16KB |
| RAM | 2KB |
| Max clock | 48MHz (HSI × 2 via PLL) |
| Default clock | 24MHz HSI, HCLK = 8MHz (÷3) |
| SysTick | 64-bit counter, feeds from HCLK or HCLK÷8 |

---

## Design principles

**MISRA C:2012 compliance.** Every deviation from a Required or Advisory rule is
documented in source with the rule number, reason, and any compensating measures.

**No dynamic memory.** No `malloc`, no `free`, no heap. All state is stack or
static allocation.

**No standard library.** The build links only `-lgcc` for soft arithmetic
helpers (`__muldi3` etc.). No libc, no newlib, no `string.h`.

**No magic numbers.** Every register bit has a named `_BIT` position constant
and a `_MASK`. Every multi-bit field has named value constants. No raw literals
at the call site.

**Explicit over implicit.** Integer casts are explicit. Narrowing conversions are
cast and commented. Unsigned wraparound is documented where intentional.

**Configure before enable.** Every hardware block is fully configured before its
enable bit is set — the same discipline as AUTOSAR BSW.

---

## Current status

| Module | File | Status |
|---|---|---|
| Reset and Clock Control | `include/ch32v003_hal/rcc.h` | working progress |
| System Tick Timer | `include/ch32v003_hal/systick.h` | working progress |
| GPIO | — | Planned |
| USART | — | Planned |
| SPI | — | Planned |
| I2C | — | Planned |
| ADC | — | Planned |
| DMA | — | Planned |

---

## Project structure

```
ch32v003_chal/
├── include/
│   └── hal/
│       ├── rcc.h          — clock tree, peripheral clock enable
│       └── systick.h      — 64-bit counter, delays, timeouts
├── docs/
│   └── ch32v003_hal_docs.md  — full API reference
├── CMakeLists.txt
└── README.md
```

All drivers are header-only. There are no `.c` files — every function is
`static` or `static inline`, so each translation unit that includes a header
gets its own private copy. This avoids linker conflicts and keeps each module
self-contained.

---

## Build

The project uses CMake. A typical toolchain file for RISC-V bare-metal:

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER   riscv32-unknown-elf-gcc)
set(CMAKE_ASM_COMPILER riscv32-unknown-elf-gcc)
set(CMAKE_OBJCOPY      riscv32-unknown-elf-objcopy)
set(CMAKE_SIZE         riscv32-unknown-elf-size)
```

Linker flags:

```cmake
target_link_options(${PROJECT_NAME} PRIVATE
    -march=${CH32V_MARCH}
    -mabi=${CH32V_MABI}
    -nostartfiles
    -nostdlib
    -lgcc
    -Wl,--gc-sections
    "-T${CH32V_LINKER_SCRIPT}"
)
```

`-lgcc` is required for 64-bit soft arithmetic (`__muldi3`, `__divdi3`).
`-nostdlib` keeps libc out entirely.

---

## Clock system

The CH32V003 clock tree has three stages:

```
[HSI 24MHz] ──┐
              ├──> PLLSRC ──> PLL (×2) ──┐
[HSE 4-25MHz]─┘                          │
                                     SW mux ──> SYSCLK
[HSI 24MHz] ─────────────────────────────┘
                                          │
                                   HPRE prescaler
                                          │
                                        HCLK ──> peripherals
                                          │
                                       ÷8 (or direct if STCLK=1)
                                          │
                                       SysTick
```

**At reset:** HSI direct, HPRE=÷3, HCLK=8MHz, SysTick=1MHz.

Two pre-built configurations are provided:

```c
RCC_Init_48mhz();   /* HSI × 2 via PLL, HCLK=48MHz, SysTick=6MHz  */
RCC_Init_24mhz();   /* HSI direct,       HCLK=24MHz, SysTick=3MHz  */
```

---

## Quick start

```c
#include "ch32v003_hal/rcc.h"
#include "ch32v003_hal/systick.h"

int main(void)
{
    /* 1 — Clock first, everything depends on it */
    RCC_Result_t result = RCC_Init_48mhz();
    if (result != RCC_OK)
    {
        for (;;) { /* clock failed — still on HSI, safe to trap */ }
    }

    /* 2 — Enable clocks for peripherals you will use */
    RCC_Enable_Clock(RCC_PERIPH_GPIOD);

    /* 3 — SysTick after RCC so frequency query is valid */
    SysTick_Init_FreeRunning();

    /* 4 — Application */
    for (;;)
    {
        SysTick_Delay_Ms(500U);
        /* toggle LED */
    }
}
```

---

## SysTick and timing

SysTick provides a 64-bit free-running counter and a set of timing utilities
built on top of it.

```c
/* Delays */
SysTick_Delay_Ms(250U);
SysTick_Delay_Us(50U);

/* Elapsed time measurement */
uint64_t start   = SysTick_CountValue();
do_work();
uint64_t elapsed = SysTicks_Elapsed(start);
uint64_t us      = SysTick_Ticks_To_Us(elapsed);

/* Non-blocking timeout */
Timeout_t t = Timeout_New_Ms(100U);
while (Timeout_Is_Expired(&t) == 0U)
{
    if (data_ready() != 0U) { break; }
}

/* Periodic SysTick interrupt at 1kHz (48MHz HCLK, STCLK=0, 6MHz tick) */
SysTick_SetCompareValue(6000U);
SysTick_Init_Periodic();
```

---

## Peripheral clock enable

All peripherals must have their clock enabled before their registers are
accessed. Writing to a peripheral with its clock disabled produces no effect
or a bus fault.

```c
/* Single call enables one peripheral */
RCC_Enable_Clock(RCC_PERIPH_USART1);

/* Available peripherals */
RCC_PERIPH_DMA1    RCC_PERIPH_SRAM
RCC_PERIPH_AFIO    RCC_PERIPH_GPIOA    RCC_PERIPH_GPIOC    RCC_PERIPH_GPIOD
RCC_PERIPH_ADC1    RCC_PERIPH_TIM1     RCC_PERIPH_SPI1     RCC_PERIPH_USART1
RCC_PERIPH_TIM2    RCC_PERIPH_WWDG     RCC_PERIPH_I2C1     RCC_PERIPH_PWR
```

---

## Documentation

Full API reference is in `docs/ch32v003_hal_docs.md`. It covers every function,
type, constant, and pre-built configuration in both headers, with examples.

Reference manual: CH32V003 Reference Manual V1.9 (WCH, wch-ic.com)

---

## MISRA compliance notes

Deviations from MISRA C:2012 are documented inline at the point of violation.
The only recurring Advisory deviation is **Rule 11.4** (integer-to-pointer cast)
on the MMIO accessor functions (`RCC_GetBase`, `SysTick_GetBase`). Each instance
carries a justification citing the verified hardware address from the reference
manual.

The build does not yet include a certified static analysis tool. Helix QAC or
Polyspace would be the appropriate tools for a production compliance check.

Documentation for deviations sill a work in progress

---

## Licence

This project is provided as-is for educational and development purposes.
The MISRA C:2012 guidelines themselves are a separate commercial document
published by the MISRA Consortium and are not included here.

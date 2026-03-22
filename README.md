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
| --- | --- |
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
| --- | --- | --- |
| Reset and Clock Control (RCC) | `include/hal/rcc.h` | working progress |
| System Tick Timer | `include/hal/systick.h` | working progress |
| GPIO | `include/hal/gpio.h` | working progress |
| USART | `include/hal/usart.h` | working progress |
| SPI | — | Planned |
| I2C | — | Planned |
| ADC | — | Planned |
| DMA | — | Planned |
| CORE | `include/hal/core.h` | working progress |
| PWR | `include/hal/pwr.h` | working progress |
| PFIC | `include/hal/pfic.h` | working progress |
| EXTI | `include/hal/exti.h` | working progress |

---

## Project structure

```text
ch32v003_chal/
├── docs/
│   ├── 01_core.md         — core register and CPU helper documentation
│   ├── 02_pwr.md          — power control notes and API usage
│   ├── 03_rcc.md          — clock tree and reset/clock control details
│   ├── 06_exti.md         — external interrupt controller documentation
│   ├── 07_gpio_afio.md    — GPIO and alternate-function guide
│   ├── 12_usart.md        — USART driver documentation
│   └── highcode.md        — high-level design and coding notes
├── include/
│   └── hal/
│       ├── core.h         — base types, bit helpers, MMIO utilities
│       ├── exti.h         — EXTI configuration and interrupt routing
│       ├── gpio.h         — GPIO configuration and pin I/O
│       ├── pfic.h         — interrupt controller (PFIC) helpers
│       ├── pwr.h          — low-power and power control helpers
│       ├── rcc.h          — clock tree and peripheral clock enable
│       ├── systick.h      — 64-bit counter, delays, timeouts
│       └── usart.h        — USART configuration and transfer helpers
├── CMakeLists.txt
├── LICENSE.md
└── README.md
```

All drivers are header-only. There are no `.c` files — every function is
`static` or `static inline`, so each translation unit that includes a header
gets its own private copy. This avoids linker conflicts and keeps each module
self-contained.

---

## Versioning policy

This project uses **Semantic Versioning** (`MAJOR.MINOR.PATCH`) with a pre-1.0
policy while the API is still evolving.

- `0.1.x` (patch): bug fixes, documentation updates, and internal cleanups with
no intentional public API break.
- `0.x.0` (minor): new modules/features or behavior changes, including API
adjustments during early development.
- `1.0.0` (major): first stable release where public API compatibility is a
commitment.

Release workflow:

1. Update `VERSION` in `CMakeLists.txt`.
2. Move items from `Unreleased` into a dated section in `CHANGELOG.md`.
3. Commit release metadata.
4. Create an annotated tag, for example `v0.1.0`.

Example tag commands:

```bash
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
```

---

## Documentation

Full API reference is in `docs/`. Each peripheral has its own doc file, it covers every function,
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

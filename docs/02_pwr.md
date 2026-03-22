# CH32V003 HAL — PWR Documentation

> Reference: CH32V003 Reference Manual V1.9, Chapter 2
> Header: `pwr.h`
> Depends on: `rcc.h`, `core.h`, `pfic.h`

---

## Table of Contents

1. [Overview](#overview)
2. [Power supply architecture](#power-supply-architecture)
3. [Feature 1 — PVD voltage monitoring](#feature-1--pvd-voltage-monitoring)
4. [Feature 2 — Sleep mode](#feature-2--sleep-mode)
5. [Feature 3 — Standby mode](#feature-3--standby-mode)
6. [Feature 4 — Auto-wakeup (AWU)](#feature-4--auto-wakeup-awu)
7. [API reference](#api-reference)
8. [Complete examples](#complete-examples)
9. [Choosing the right low-power mode](#choosing-the-right-low-power-mode)
10. [Personal notes](#personal-notes)

---

## Overview

The PWR peripheral manages three distinct concerns:

**Voltage monitoring** — the PVD (Programmable Voltage Detector)
watches VDD against a configurable threshold and can fire an
interrupt when power drops dangerously low or recovers.

**Low-power modes** — two sleep states that trade wakeup speed
against power consumption. Sleep stops the CPU. Standby stops
everything.

**Periodic wakeup** — the AWU (Auto-Wakeup Unit) uses an internal
128KHz oscillator to wake the chip from Standby on a timer without
any external signal.

**Always enable the PWR clock first:**

```c
RCC_Enable_Clock(RCC_PERIPH_PWR);
```

---

## Power supply architecture

The CH32V003 operates from VDD at 2.7V–5.5V. An internal voltage
regulator produces the 1.5V supply used by the CPU core, memory,
and digital peripherals. The I/O pins run directly from VDD.

```text
VDD (2.7–5.5V)
    │
    ├── I/O pins         ← run directly from VDD
    ├── ADC, reset module
    │
    └── Internal regulator → 1.5V
                                │
                                ├── CPU core
                                ├── Memory (SRAM, flash)
                                └── Digital peripherals
```

In **Sleep mode** the regulator stays on. In **Standby mode** the
regulator turns off — this is where the big power saving comes from,
but it also means the CPU clock must restart from scratch on wakeup.

---

## Feature 1 — PVD voltage monitoring

### What it does

The PVD continuously compares VDD against a software-selected
threshold. When VDD crosses the threshold in either direction it
can fire an interrupt, giving your code time to save state before
the voltage drops too low to operate.

### The PVD output signal

```text
VDD stays above threshold:    PVD output = 0  (normal)
VDD drops below threshold:    PVD output = 1  (danger)
```

The ~200mV hysteresis prevents rapid toggling when VDD is near
the boundary — the rising and falling thresholds differ slightly:

```text
Threshold selection PWR_PVD_3V30:
  Rising trigger:  VDD crosses 3.30V going DOWN  → PVD output goes 0→1
  Falling trigger: VDD crosses 3.15V going UP    → PVD output goes 1→0
```

### The PVD interrupt path

PVD does not have its own direct path to the PFIC. Instead the PVD
output is internally wired to **EXTI line 8**. This means the PVD
interrupt goes through the same EXTI mechanism as GPIO interrupts,
but you do not need to configure AFIO — the connection is hardwired.

```text
PVD output signal
       │
       ▼
EXTI line 8 (internal, hardwired — no AFIO needed)
       │
       ▼
PFIC IRQ 17 → PVD_IRQHandler
```

### Edge directions — important

Because EXTI monitors the PVD output (not VDD directly), the edge
direction is counterintuitive:

| What happens to VDD | PVD output | EXTI line 8 edge | Trigger constant |
| --- | --- | --- | --- |
| Drops below threshold | 0 → 1 | Rising | `PWR_PVD_TRIGGER_LOW_CROSSING` |
| Recovers above threshold | 1 → 0 | Falling | `PWR_PVD_TRIGGER_HIGH_CROSSING` |
| Either crossing | both | Both | `PWR_PVD_TRIGGER_BOTH` |

### Threshold levels

| Constant | Rising trigger | Falling trigger | Use case |
| --- | --- | --- | --- |
| `PWR_PVD_2V85` | 2.85V | 2.70V | Minimum operating voltage warning |
| `PWR_PVD_3V05` | 3.05V | 2.90V | Low battery warning |
| `PWR_PVD_3V30` | 3.30V | 3.15V | USB-powered 3.3V system |
| `PWR_PVD_3V50` | 3.50V | 3.30V | General 3.3V with headroom |
| `PWR_PVD_3V70` | 3.70V | 3.50V | Early warning on 3.7V LiPo |
| `PWR_PVD_3V90` | 3.90V | 3.70V | LiPo near-empty detection |
| `PWR_PVD_4V10` | 4.10V | 3.90V | LiPo low detection |
| `PWR_PVD_4V40` | 4.40V | 4.20V | LiPo mid detection |

---

## Feature 2 — Sleep mode

What it does

The CPU clock stops. The CPU halts at the `WFI` instruction.
All peripherals — USART, timers, SysTick, DMA — keep running
at full speed. Any enabled interrupt wakes the CPU and execution
resumes after the `WFI` instruction.

```text
Sleep mode:
  CPU clock:       OFF    ← halted, saves CPU power
  Peripheral clocks: ON   ← everything else still running
  Voltage regulator: ON   ← 1.5V domain still active
  Wakeup time:     ~1 cycle
  Wakeup source:   any enabled interrupt
```

### When to use it

Sleep mode is the right choice when:

- You are waiting for a peripheral to complete (USART RX, timer)
- You want to reduce power between interrupt events
- You need fast wakeup (< 1µs)
- Peripherals must continue operating during the sleep

In most interrupt-driven applications, `WFI()` in the main loop
already puts the chip into a light sleep. `PWR_EnterSleep()` is
simply a more explicit version that also ensures `SLEEPDEEP=0`
before sleeping.

```c
/* These two are equivalent for most purposes */
WFI();               /* simple — just executes WFI */
PWR_EnterSleep();    /* explicit — clears SLEEPDEEP first, then WFI */
```

---

## Feature 3 — Standby mode

What it does

The deepest low-power mode. All high-frequency clocks (HSE, HSI,
PLL) stop. The voltage regulator turns off. Only the standby
circuit and IWDG remain active.

```text
Standby mode:
  CPU:               OFF   ← completely stopped
  HSE/HSI/PLL:       OFF   ← all clocks stopped
  Peripheral clocks: OFF   ← all peripherals stopped
  SRAM contents:     HELD  ← variables survive standby
  Register contents: HELD  ← peripheral config survives
  I/O pin states:    HELD  ← pins keep their last state
  Voltage regulator: OFF   ← maximum power saving
  Wakeup time:       ~HSI startup time (~few µs)
  Wakeup source:     EXTI interrupt or AWU event
```

### What happens on wakeup

After waking from standby:

1. Clock switches to HSI (24MHz by default)
2. `PWR_CTLR` is reset — your PVD and PDDS configuration is gone
3. Execution resumes at the instruction after `WFI()`
4. `PWR_EnterStandby()` clears `SLEEPDEEP` before returning

If your application runs at 48MHz you must call `RCC_Init_48mhz()`
again after waking from standby.

### Important caveats

**Disable unnecessary peripheral clocks before entering standby.**
Any peripheral with its clock enabled continues to draw leakage
current even in standby. Call `RCC_Enable_Clock` with disable
variants for anything you do not need.

**Do not enter standby mid-transmission.** If USART is transmitting
or SPI is clocking data, entering standby aborts the transfer.
Wait for `TC=1` (USART transmission complete) before sleeping.

**Flash writes block standby entry.** The hardware waits for any
ongoing flash operation to complete before entering standby.

---

## Feature 4 — Auto-wakeup (AWU)

What it does

The AWU lets the chip wake periodically from Standby without any
external signal. It uses the internal 128KHz LSI oscillator,
divided by a prescaler, compared against a 6-bit window value.

### Timing formula

```text
interval_seconds = (AWUWR × prescaler_divider) / 128000
```

Where 128000 is the nominal LSI frequency in Hz, AWUWR is the
window value (1–63), and prescaler_divider comes from the
`PWR_AWUPrescaler_t` enum.

### Quick reference timing table

These use AWUWR = 63 (maximum):

| Prescaler | Divider | Interval |
| --- | --- | --- |
| `PWR_AWUPSC_DIV61440` | 61440 | ~30.2 seconds |
| `PWR_AWUPSC_DIV10240` | 10240 | ~5.0 seconds |
| `PWR_AWUPSC_DIV4096` | 4096 | ~2.0 seconds |
| `PWR_AWUPSC_DIV2048` | 2048 | ~1.0 second |
| `PWR_AWUPSC_DIV1024` | 1024 | ~0.5 seconds |
| `PWR_AWUPSC_DIV512` | 512 | ~252ms |
| `PWR_AWUPSC_DIV256` | 256 | ~126ms |
| `PWR_AWUPSC_DIV128` | 128 | ~63ms |

To fine-tune within a prescaler step, reduce AWUWR below 63:

```text
/* Exactly ~1 second using DIV4096 */
AWUWR = 31   →  31 × 4096 / 128000 = 0.992 seconds

/* Exactly ~500ms using DIV2048 */
AWUWR = 31   →  31 × 2048 / 128000 = 0.496 seconds
```

### LSI accuracy warning

The internal LSI oscillator is **not a precision clock**. WCH
specifies it can vary by ±30% over temperature and supply voltage.
An interval programmed as 2 seconds may actually be anywhere from
1.4 to 2.6 seconds depending on conditions. For precision periodic
wakeup use an external crystal and a timer instead.

---

## API reference

### `PWR_EnablePVD`

```c
static void PWR_EnablePVD(PWR_PVDThreshold_t threshold);
```

Enables the PVD and sets the monitoring threshold. Does not
configure interrupts — call `PWR_EnablePVD_IRQ` separately if
interrupt notification is needed.

---

### `PWR_DisablePVD`

```c
static void PWR_DisablePVD(void);
```

Disables the PVD. The `PVD0` status flag in `PWR_CSR` is no
longer meaningful after calling this.

---

### `PWR_ReadPVD`

```c
static uint32_t PWR_ReadPVD(void);
```

Returns `1U` if VDD is currently **below** the configured
threshold. Returns `0U` if VDD is above the threshold. Only valid
after `PWR_EnablePVD()` has been called. Use inside the PVD ISR
when `PWR_PVD_TRIGGER_BOTH` is configured to determine which
direction the crossing occurred.

---

### `PWR_EnablePVD_IRQ`

```c
static void PWR_EnablePVD_IRQ(PWR_PVDTrigger_t trigger,
                               IRQ_Priority_t   priority);
```

Configures EXTI line 8 for PVD interrupt and enables `IRQ_PVD`
in the PFIC. Must call `PWR_EnablePVD()` first.

`trigger` selects which voltage crossing fires the interrupt:

| Value | Fires when |
| --- | --- |
| `PWR_PVD_TRIGGER_LOW_CROSSING` | VDD drops below threshold |
| `PWR_PVD_TRIGGER_HIGH_CROSSING` | VDD recovers above threshold |
| `PWR_PVD_TRIGGER_BOTH` | Either crossing |

---

### `PWR_DisablePVD_IRQ`

```c
static void PWR_DisablePVD_IRQ(void);
```

Disables the PVD interrupt. Clears the EXTI line 8 edge triggers
and the PFIC entry. Does not disable the PVD itself.

---

### `PWR_ClearPVD_Flag`

```c
static void PWR_ClearPVD_Flag(void);
```

Clears the EXTI line 8 pending flag. **Must be called inside
`PVD_IRQHandler`** or the interrupt fires again immediately.

---

### `PWR_EnterSleep`

```c
static void PWR_EnterSleep(void);
```

Sets `SLEEPDEEP=0`, `PDDS=0`, and executes `WFI`. The function
returns after any interrupt wakes the CPU. The waking interrupt's
ISR runs before this function returns.

---

### `PWR_EnterStandby`

```c
static void PWR_EnterStandby(void);
```

Sets `SLEEPDEEP=1`, `PDDS=1`, and executes `WFI`. Execution
resumes after this call when wakeup occurs. Clears `SLEEPDEEP`
before returning so a subsequent `WFI()` does not re-enter
standby unexpectedly.

After wakeup: HSI is the active clock. Call `RCC_Init_48mhz()`
before using any baud-rate-sensitive peripheral.

---

### `PWR_ConfigAWU`

```c
static void PWR_ConfigAWU(PWR_AWUPrescaler_t prescaler,
                           uint8_t           window_value);
```

Configures and enables the auto-wakeup unit. `window_value` is
clamped to 1–63. Call this before `PWR_EnterStandby()`. The AWU
fires `AWU_IRQHandler` (IRQ 21) on each wakeup interval.

---

### `PWR_DisableAWU`

```c
static void PWR_DisableAWU(void);
```

Disables the auto-wakeup unit. The chip will no longer wake
periodically from standby.

---

## Complete examples

### Monitor voltage, take action on low crossing

```c
#include "rcc.h"
#include "pwr.h"

static volatile uint32_t g_power_low = 0U;

void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void PVD_IRQHandler(void)
{
    PWR_ClearPVD_Flag();
    g_power_low = 1U;
}

int main(void)
{
    RCC_Init_48mhz();
    RCC_Enable_Clock(RCC_PERIPH_PWR);

    /* Monitor at 3.30V — fire when VDD drops below it */
    PWR_EnablePVD(PWR_PVD_3V30);
    PWR_EnablePVD_IRQ(PWR_PVD_TRIGGER_LOW_CROSSING, IRQ_PRIORITY_0);

    for (;;)
    {
        WFI();

        if (g_power_low != 0U)
        {
            IRQ_CRITICAL_START();
            g_power_low = 0U;
            IRQ_CRITICAL_END();

            /* Save state, flush USART, disable non-essential systems */
        }
    }
}
```

---

### Track both crossings — log power events

```c
static volatile uint32_t g_power_failures  = 0U;
static volatile uint32_t g_power_recoveries = 0U;

void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void PVD_IRQHandler(void)
{
    PWR_ClearPVD_Flag();

    if (PWR_ReadPVD() != 0U)
    {
        /* PVD=1 → VDD below threshold → power failing */
        g_power_failures = g_power_failures + 1U;
    }
    else
    {
        /* PVD=0 → VDD above threshold → power recovered */
        g_power_recoveries = g_power_recoveries + 1U;
    }
}

/* Setup */
PWR_EnablePVD(PWR_PVD_3V30);
PWR_EnablePVD_IRQ(PWR_PVD_TRIGGER_BOTH, IRQ_PRIORITY_0);
```

---

### Sleep between USART bytes

```c
/* USART interrupt-driven receive already configured */
/* Main loop sleeps until bytes arrive               */

for (;;)
{
    PWR_EnterSleep();   /* explicit: ensures SLEEPDEEP=0 before WFI */

    uint8_t byte = 0U;
    while (USART1_RxRead(&byte) == USART_OK)
    {
        /* process byte */
    }
}
```

---

### Periodic wakeup from standby every 2 seconds

```c
#include "rcc.h"
#include "pwr.h"
#include "core.h"

void AWU_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void AWU_IRQHandler(void)
{
    /* No flag to clear for AWU — wakeup clears it automatically */
    /* Do your periodic work here or set a flag for main */
}

int main(void)
{
    RCC_Init_48mhz();
    RCC_Enable_Clock(RCC_PERIPH_PWR);

    /* Configure AWU for ~2 second interval */
    /* interval = 63 × 4096 / 128000 = 2.016 seconds */
    PWR_ConfigAWU(PWR_AWUPSC_DIV4096, 63U);

    /* Enable AWU interrupt in PFIC */
    PFIC_SetPriority(IRQ_AWU, IRQ_PRIORITY_2);
    PFIC_EnableIRQ(IRQ_AWU);

    for (;;)
    {
        /* Do work here, then sleep */
        /* ... sensor read, transmit result ... */

        /* Enter standby — wakes via AWU every ~2 seconds */
        PWR_EnterStandby();

        /* Woke up — HSI is active, re-initialise clock if needed */
        RCC_Init_48mhz();
    }
}
```

---

### PVD + Standby — save state on power failure

```c
static void save_state_to_flash(void);   /* your implementation */

void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void PVD_IRQHandler(void)
{
    PWR_ClearPVD_Flag();

    if (PWR_ReadPVD() != 0U)
    {
        /* VDD falling — save critical data before power dies */
        save_state_to_flash();

        /* Enter standby to minimise remaining current draw */
        PWR_EnterStandby();
    }
}

/* Setup */
RCC_Enable_Clock(RCC_PERIPH_PWR);
PWR_EnablePVD(PWR_PVD_3V05);
PWR_EnablePVD_IRQ(PWR_PVD_TRIGGER_LOW_CROSSING, IRQ_PRIORITY_0);
```

---

## Choosing the right low-power mode

| | Sleep | Standby |
| --- | --- | --- |
| CPU clock | OFF | OFF |
| Peripheral clocks | ON | OFF |
| Voltage regulator | ON | OFF |
| SRAM contents | Preserved | Preserved |
| PWR_CTLR register | Preserved | **RESET on wakeup** |
| Wakeup time | ~1 cycle | ~HSI startup (~few µs) |
| Wakeup sources | Any interrupt | EXTI interrupt or AWU |
| Clock after wakeup | Unchanged | HSI (must re-init RCC) |
| Power saving | Moderate | Maximum |

**Use Sleep when:**

- Waiting for a peripheral event (USART byte, timer tick)
- Wakeup must be fast and transparent
- Peripherals must keep running (timers counting, USART receiving)
- Most interrupt-driven projects — `WFI()` in the main loop

**Use Standby when:**

- Long idle periods (seconds to hours)
- Minimum battery drain is the priority
- It is acceptable to reinitialise clocks and peripherals on wakeup
- Sensor nodes, data loggers, battery-powered remote devices

---

## Personal notes

**PVD threshold selection depends on your power source.** For a
3.3V regulated supply the useful range is `PWR_PVD_2V85` through
`PWR_PVD_3V05` — anything higher fires before the regulator can
even stabilise. For a LiPo battery (3.0V–4.2V) the `PWR_PVD_3V30`
to `PWR_PVD_3V70` range is more useful.

**PWR_CTLR resets on standby wakeup.** This catches everyone the
first time. If you enable PVD before entering standby and then wake
up and go back to sleep, the PVD is gone. Re-enable it after every
wakeup from standby if you need it.

**Sleep mode is nearly free to use.** In a fully interrupt-driven
application the main loop should always be `WFI()` or
`PWR_EnterSleep()`. There is no reason to spin the CPU doing nothing
between events. Even modest sleep usage cuts average current
consumption significantly.

**AWU timing drift is real.** Do not build anything that requires
accurate timekeeping on AWU alone. It is fine for "wake up roughly
every 2 seconds to take a reading" but not for "log an event at
exactly 500ms intervals." Use SysTick or a hardware timer for
precision timing.

**PVD interrupt priority should be 0 (highest).** When the power
is failing you have limited time and you do not want the PVD handler
waiting behind a lower-priority USART ISR. Give it the highest
priority of all your interrupts.

**Standby + AWU is the standard pattern for battery sensor nodes.**
The device wakes, takes a measurement, transmits the result, and
re-enters standby. With careful peripheral management this keeps
average current in the low microamp range between measurements.

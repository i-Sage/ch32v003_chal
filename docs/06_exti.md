# CH32V003 HAL — EXTI Documentation

> Reference: CH32V003 Reference Manual V1.9, Chapter 6.4 and 6.5.1
> Header: `exti.h`
> Depends on: `gpio.h`, `pfic.h`, `core.h`

---

## Table of Contents

1. [Overview](#overview)
2. [How EXTI works](#how-exti-works)
3. [The three-layer picture](#the-three-layer-picture)
4. [EXTI registers](#exti-registers)
5. [API reference](#api-reference)
6. [Writing an EXTI ISR](#writing-an-exti-isr)
7. [Complete examples](#complete-examples)
8. [Edge selection guide](#edge-selection-guide)
9. [Common mistakes](#common-mistakes)
10. [Personal notes](#personal-notes)

---

## Overview

EXTI (External Interrupt Controller) lets any GPIO pin trigger an
interrupt when its logic level changes. You configure which edge
(rising, falling, or both) causes the trigger, and the hardware
does the rest — your ISR fires the moment the edge is detected,
regardless of what the CPU is doing.

**The mandatory setup sequence for any GPIO interrupt:**

```c
/* 1. Enable clocks */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOx);

/* 2. Configure the GPIO pin as an input */
GPIO_SetPinMode(port, pin, mode);

/* 3. Route the EXTI line to the correct port via AFIO */
AFIO_SetExtiPort(pin_number, port);

/* 4. Configure the edge trigger and enable the line */
EXTI_ConfigLine(pin, edge);

/* 5. Set priority and enable in the PFIC */
PFIC_SetPriority(IRQ_EXTI7_0, priority);
PFIC_EnableIRQ(IRQ_EXTI7_0);
```

---

## How EXTI works

GPIO pins are not connected directly to the PFIC. The EXTI
controller sits between them:

```text
GPIO pin (physical hardware)
        │
        ▼
Edge detect circuit          ← watches for rising/falling edge
        │
        ▼
EXTI controller              ← INTENR gates whether it passes through
        │
        ▼
PFIC IRQ 20 (EXTI7_0)        ← all 8 GPIO lines share this one entry
        │
        ▼
CPU → EXTI7_0_IRQHandler     ← your ISR checks which pin fired
```

**All 8 GPIO pin numbers share one PFIC interrupt.**
Pin 0, pin 1, pin 2 ... pin 7 all feed into IRQ 20. This means
when any GPIO edge fires, the same handler runs. Inside the handler
you read the `INTFR` register to find out which specific pin triggered.

**EXTI lines map to pin numbers, not ports.**
EXTI line 3 corresponds to pin 3 on whichever port AFIO routes it to.
You can have PA3, PC3, or PD3 — but only one of them can be the
source of EXTI line 3 at a time. AFIO selects which.

```text
EXTI line 0  →  pin 0 on the port selected by AFIO for line 0
EXTI line 1  →  pin 1 on the port selected by AFIO for line 1
...
EXTI line 7  →  pin 7 on the port selected by AFIO for line 7
```

You can have multiple lines active simultaneously — for example
PD3 on EXTI line 3 AND PC5 on EXTI line 5, both triggering
the same ISR at IRQ 20.

---

## The three-layer picture

```text
Layer 1 — EXTI controller
          EXTI_ConfigLine() sets the edge and enables INTENR.
          Without this the GPIO edge goes nowhere.

Layer 2 — PFIC
          PFIC_EnableIRQ(IRQ_EXTI7_0) forwards the signal to the CPU.
          Without this the CPU never wakes.

Layer 3 — CPU global enable
          IRQ_Enable() or the startup file's mret.
          Without this the CPU ignores all interrupts.
```

---

## EXTI registers

The EXTI base address is `0x40010400`. All registers use bits [9:0]
— one bit per EXTI line. Bits [31:10] are reserved.

### INTENR — Interrupt Enable Register (0x00)

Controls which lines can generate interrupts. Bit N = 1 means
EXTI line N is enabled. Reset value: all zeros (all masked).

### EVENR — Event Enable Register (0x04)

Controls which lines generate wake events instead of interrupts.
Events wake the CPU from `WFE` (wait for event) without running
an ISR. Interrupts wake from `WFI` and run the ISR. A line can
have both INTENR and EVENR set simultaneously.

For most applications you only use INTENR — `EXTI_ConfigLine`
handles this for you.

### RTENR — Rising Edge Trigger Enable (0x08)

Bit N = 1 means EXTI line N triggers on a rising (low→high) edge.

### FTENR — Falling Edge Trigger Enable (0x0C)

Bit N = 1 means EXTI line N triggers on a falling (high→low) edge.

Setting both RTENR and FTENR for the same line gives "any edge"
triggering — both transitions fire the interrupt.

### SWIEVR — Software Interrupt Event Register (0x10)

Writing 1 to bit N manually fires EXTI line N as if the hardware
edge occurred. Sets the corresponding INTFR flag. If INTENR is
also set, an interrupt fires. Must be cleared by writing 1 to
the corresponding INTFR bit.

### INTFR — Interrupt Flag Register (0x14)

Bit N = 1 means EXTI line N has fired and is waiting to be
acknowledged. **Write 1 to clear** — this is the opposite of most
registers. If you do not clear the flag inside your ISR, the
interrupt fires again immediately after returning.

---

## API reference

### `EXTI_ConfigLine`

```c
static void EXTI_ConfigLine(GPIO_Pin_t pin, EXTI_Edge_t edge);
```

Configures the edge trigger and enables the interrupt for one
GPIO pin number. Clears any stale pending flag before enabling.

`pin` is the GPIO pin number — `GPIO_PIN_0` through `GPIO_PIN_7`.
This is the same as the EXTI line number.

`edge` is one of:

| Value | Triggers on |
| --- | --- |
| `EXTI_EDGE_RISING` | Low → high transition |
| `EXTI_EDGE_FALLING` | High → low transition |
| `EXTI_EDGE_BOTH` | Any transition |

Call after `GPIO_SetPinMode` and `AFIO_SetExtiPort`, and before
`PFIC_EnableIRQ`.

---

### `EXTI_DisableLine`

```c
static void EXTI_DisableLine(GPIO_Pin_t pin);
```

Disables the interrupt for one line. Clears both edge trigger bits
and the interrupt enable bit. Does not affect the PFIC — if all
lines are disabled and no more GPIO interrupts are needed, also
call `PFIC_DisableIRQ(IRQ_EXTI7_0)`.

---

### `EXTI_ClearFlag`

```c
static void EXTI_ClearFlag(GPIO_Pin_t pin);
```

Clears the interrupt pending flag for one line. **Must be called
inside your ISR** for every line that fired. Writing 1 clears the
bit (not 0 — see register description above).

If you forget to call this, the ISR fires again immediately after
returning, creating an infinite loop that starves the main code.

---

### `EXTI_GetFlag`

```c
static uint32_t EXTI_GetFlag(GPIO_Pin_t pin);
```

Returns `1U` if the flag for this line is set, `0U` if not. Use
inside your ISR to identify which pin triggered. Since all 8 pins
share one ISR you must check each pin you care about.

---

### `EXTI_SoftwareTrigger`

```c
static void EXTI_SoftwareTrigger(GPIO_Pin_t pin);
```

Manually fires an EXTI line from software. Behaves exactly as if
the hardware edge occurred. `INTENR` must be enabled for the line
for an interrupt to fire. Useful for testing your ISR without
physical hardware connected.

---

## Writing an EXTI ISR

The ISR name must be `EXTI7_0_IRQHandler` — this matches the weak
alias in `startup.S`. The `interrupt` attribute is required on the
CH32V003 to emit `mret` at the end of the function.

The pattern inside the ISR is always the same:

1. Check which pin fired with `EXTI_GetFlag`
2. Clear the flag immediately with `EXTI_ClearFlag`
3. Do your work

```c
void EXTI7_0_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    /* Check every pin your application uses */
    if (EXTI_GetFlag(GPIO_PIN_3) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_3);
        /* handle pin 3 */
    }

    if (EXTI_GetFlag(GPIO_PIN_5) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_5);
        /* handle pin 5 */
    }
}
```

**Why check all pins, not just one?**
Multiple edges can be pending simultaneously if two pins fired
before the ISR ran. Checking only one and returning leaves the
other flag set, which re-enters the ISR immediately. Check every
pin you have configured.

**Clear the flag before doing the work, not after.**
If you do the work first and the pin fires again during that work,
the clear wipes out the new event. Clear first, then act.

---

## Complete examples

### Button on PD3 — falling edge (button pulls to GND)

```c
#include "rcc.h"
#include "systick.h"
#include "gpio.h"
#include "exti.h"
#include "pfic.h"
#include "core.h"

static volatile uint32_t g_button_pressed = 0U;

void EXTI7_0_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetFlag(GPIO_PIN_3) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_3);
        g_button_pressed = 1U;
    }
}

int main(void)
{
    if (RCC_Init_48mhz() != RCC_OK) { for (;;) {} }

    RCC_Enable_Clock(RCC_PERIPH_AFIO);
    RCC_Enable_Clock(RCC_PERIPH_GPIOD);
    SysTick_Init_FreeRunning();

    /* Pull-up — pin reads 1 normally, 0 when button pressed */
    GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_PULLUP);

    /* Route EXTI line 3 to port D */
    AFIO_SetExtiPort(3U, GPIO_PORT_D);

    /* Trigger on falling edge — button press pulls line low */
    EXTI_ConfigLine(GPIO_PIN_3, EXTI_EDGE_FALLING);

    PFIC_SetPriority(IRQ_EXTI7_0, IRQ_PRIORITY_2);
    PFIC_EnableIRQ(IRQ_EXTI7_0);

    for (;;)
    {
        WFI();

        if (g_button_pressed != 0U)
        {
            IRQ_CRITICAL_START();
            g_button_pressed = 0U;
            IRQ_CRITICAL_END();

            /* do something — toggle LED, send UART message, etc */
        }
    }
}
```

---

### Two buttons on different pins

```c
static volatile uint32_t g_btn_a = 0U;
static volatile uint32_t g_btn_b = 0U;

void EXTI7_0_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetFlag(GPIO_PIN_3) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_3);
        g_btn_a = 1U;
    }

    if (EXTI_GetFlag(GPIO_PIN_4) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_4);
        g_btn_b = 1U;
    }
}

/* In main setup: */
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_PULLUP);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_4, GPIO_MODE_IN_PULLUP);

AFIO_SetExtiPort(3U, GPIO_PORT_D);
AFIO_SetExtiPort(4U, GPIO_PORT_D);

EXTI_ConfigLine(GPIO_PIN_3, EXTI_EDGE_FALLING);
EXTI_ConfigLine(GPIO_PIN_4, EXTI_EDGE_FALLING);

/* Both lines share one PFIC entry */
PFIC_SetPriority(IRQ_EXTI7_0, IRQ_PRIORITY_2);
PFIC_EnableIRQ(IRQ_EXTI7_0);
```

---

### Encoder on PC6 — both edges (count every transition)

```c
static volatile uint32_t g_encoder_ticks = 0U;

void EXTI7_0_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetFlag(GPIO_PIN_6) != 0U)
    {
        EXTI_ClearFlag(GPIO_PIN_6);
        g_encoder_ticks = g_encoder_ticks + 1U;
    }
}

/* In main setup: */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);

GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_6, GPIO_MODE_IN_FLOATING);

AFIO_SetExtiPort(6U, GPIO_PORT_C);

/* Both edges — count every rising and falling transition */
EXTI_ConfigLine(GPIO_PIN_6, EXTI_EDGE_BOTH);

PFIC_SetPriority(IRQ_EXTI7_0, IRQ_PRIORITY_1);
PFIC_EnableIRQ(IRQ_EXTI7_0);
```

---

### EXTI combined with USART interrupts

Both EXTI7_0 and USART1 can be active simultaneously. Give the
more time-critical one the higher priority (lower number):

```c
/* USART RX is more time-critical — give it higher priority */
USART1_EnableRxIRQ(IRQ_PRIORITY_1);

/* Button can tolerate more latency */
EXTI_ConfigLine(GPIO_PIN_3, EXTI_EDGE_FALLING);
PFIC_SetPriority(IRQ_EXTI7_0, IRQ_PRIORITY_2);
PFIC_EnableIRQ(IRQ_EXTI7_0);
```

If a USART byte arrives while the EXTI ISR is running, the
hardware interrupt nesting (enabled by the startup file via
`INTSYSCR`) allows USART1 to preempt EXTI7_0 because it has
higher priority.

---

## Edge selection guide

| Situation | Recommended edge |
| --- | --- |
| Button wired to GND with pull-up | `EXTI_EDGE_FALLING` — detects press |
| Button wired to VCC with pull-down | `EXTI_EDGE_RISING` — detects press |
| Detect both press and release | `EXTI_EDGE_BOTH` |
| Encoder channel | `EXTI_EDGE_BOTH` — count every transition |
| Signal from another MCU / module | Depends on the protocol — check the datasheet |
| Open-drain bus event (I2C ACK etc) | `EXTI_EDGE_FALLING` — line pulled low is the event |

---

## Common mistakes

**Forgetting `EXTI_ClearFlag` inside the ISR.**
The most common bug. The ISR fires, does its work, returns —
but the flag is still set so the CPU immediately jumps back into
the ISR again. Symptom: ISR runs in an infinite loop, main code
never runs. Fix: always call `EXTI_ClearFlag` for every pin that
fired.

**Forgetting `AFIO_SetExtiPort`.**
If you skip this step, EXTI line N defaults to port A. If your
pin is on port C or D the trigger never fires. Symptom: button
does nothing, ISR never runs.

**Only checking one pin when multiple are configured.**
If you have PD3 and PD4 both on EXTI and only check pin 3 in
your ISR, pin 4 flags accumulate and re-trigger the ISR endlessly.
Always check every pin you have enabled.

**Using `GPIO_MODE_IN_FLOATING` with no external pull resistor.**
A floating pin picks up noise and fires randomly. Always use
`GPIO_MODE_IN_PULLUP` or `GPIO_MODE_IN_PULLDOWN` if no external
pull resistor is present on the board.

**Configuring EXTI before enabling the GPIO clock.**
The AFIO and GPIO clocks must be enabled before writing any GPIO
or AFIO registers. Symptom: configuration appears to succeed but
the interrupt never fires.

---

## Personal notes

**EXTI is the right tool for buttons and encoders.** Polling a
button in the main loop wastes CPU cycles and can miss fast events.
EXTI detects the edge in hardware with zero CPU involvement until
the moment the ISR runs.

**Debouncing is your responsibility.** EXTI detects every edge —
including the multiple bounces a mechanical button produces when
pressed. A 10ms software debounce in the ISR (check the pin state
after a short delay and discard if not stable) or a hardware RC
filter (100Ω + 100nF) solves this. Without debouncing, one button
press can register as 5-20 events.

**The shared ISR is not a limitation in practice.** Most projects
use 1-3 GPIO interrupts. Checking 3 flags takes 3 instructions.
The overhead is negligible.

**Priority matters when mixing EXTI with other interrupts.** A
USART RX at 1Mbaud must preempt everything. A button press can
wait a few microseconds. Use `IRQ_PRIORITY_0` or `_1` for
time-critical peripherals and `_2` or `_3` for user inputs.

**`EXTI_EDGE_BOTH` is useful but be careful with bounce.** On a
clean signal like an encoder it is perfect. On a mechanical button
it doubles the bounce events.

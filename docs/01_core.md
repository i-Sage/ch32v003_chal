# CH32V003 HAL — core.h Documentation

> Header: `core.h`
> No dependencies — safe to include from any other header
> Purpose: CPU-level interrupt control, timing utilities, power management

---

## The mental model — three levels of interrupt control

Before using any function in this header it helps to have the full
picture in your head. There are three separate layers that must all
agree before an interrupt reaches your code:

```text
Layer 1 — Peripheral level
          Each peripheral has its own interrupt enable bit.
          e.g. USART1->CTLR1 bit RXNEIE
          "USART1, generate an interrupt signal when a byte arrives"

Layer 2 — PFIC level  (pfic.h handles this)
          The interrupt controller decides which signals reach the CPU.
          e.g. PFIC_EnableIRQ(USART1_IRQ)
          "PFIC, forward USART1's interrupt signal to the CPU"

Layer 3 — CPU level   (core.h handles this)
          The CPU's global master switch.
          e.g. IRQ_Enable()
          "CPU, you are allowed to be interrupted right now"
```

`core.h` only controls layer 3 — the global master switch.
If any layer is off, the interrupt never fires.

---

## What mstatus is

`mstatus` is a special CPU register called a CSR (Control and Status
Register). CSRs are separate from the normal integer registers
(a0-a7, t0-t6 etc) and control how the CPU itself behaves.

You cannot access `mstatus` with normal load/store instructions.
RISC-V has dedicated CSR instructions:

```text
csrr  rd, csr      — read CSR into register rd
csrw  csr, rs      — write register rs into CSR
csrsi csr, imm     — set   specific bits in CSR (immediate mask)
csrci csr, imm     — clear specific bits in CSR (immediate mask)
csrrci rd, csr, imm — read CSR into rd, then clear bits (atomic)
```

Bit 3 of `mstatus` is called **MIE** (Machine Interrupt Enable):

```text
mstatus bit 3 = 0  →  no interrupt can reach the CPU, period
mstatus bit 3 = 1  →  interrupts can reach the CPU (if PFIC allows)
```

`0x08` in binary is `0000 1000` — bit 3. That is why you see
`0x08` in all the CSR instructions in this header.

---

## API Reference

### `IRQ_Enable()`

```c
static inline void IRQ_Enable(void);
```

Sets `mstatus.MIE`. Interrupts can fire after this returns.

The startup file (`handle_reset`) already enables interrupts before
calling `main()` via the `mret` instruction. You do not need to call
`IRQ_Enable()` at the start of `main()` — it is already done.

The primary use case is re-enabling interrupts after a deliberate
`IRQ_Disable()`:

```c
IRQ_Disable();
/* do something that cannot be interrupted */
IRQ_Enable();   /* back to normal */
```

---

### `IRQ_Disable()`

```c
static inline void IRQ_Disable(void);
```

Clears `mstatus.MIE`. No interrupt can fire after this returns.

Use sparingly. Every microsecond interrupts are disabled is a
microsecond the system cannot respond to events. If a USART byte
arrives while interrupts are disabled, the hardware buffers it for
a moment — but if you stay disabled too long, the next byte
overwrites it and you get an overrun error.

```c
IRQ_Disable();
critical_operation();
IRQ_Enable();
```

**Prefer `IRQ_CRITICAL_START/END` over bare disable/enable** — see below.

---

### `IRQ_CRITICAL_START()` and `IRQ_CRITICAL_END()`

```c
IRQ_CRITICAL_START();
/* protected code */
IRQ_CRITICAL_END();
```

These are macros, not functions. `IRQ_CRITICAL_START()` declares a
local variable `_irq_saved_mstatus` in the current scope — which is
why they must be used in the same scope block.

**Why use these instead of `IRQ_Disable` / `IRQ_Enable`:**

If you call `IRQ_Disable()` when interrupts were already disabled
(for example, inside an ISR where they are automatically disabled,
or inside a nested critical section), then calling `IRQ_Enable()` at
the end would incorrectly turn interrupts back on before you intended.

`IRQ_CRITICAL_START` saves the current state atomically. If interrupts
were already off, they stay off at the end. If they were on, they come
back on at the end.

```c
/* Safe nesting example */
IRQ_CRITICAL_START();            /* saves MIE=1, disables */

    IRQ_CRITICAL_START();        /* saves MIE=0, still disabled */
    shared_var = new_value;
    IRQ_CRITICAL_END();          /* restores MIE=0, still disabled */

IRQ_CRITICAL_END();              /* restores MIE=1, re-enabled */
```

**The most important use case — shared variables:**

Any variable read or written by both your main code and an ISR must
be protected. Without protection you can get a half-updated value:

```c
/* Imagine g_counter is uint64_t — takes two 32-bit reads */
/* An ISR could update it between the two reads            */

/* WRONG — not protected */
uint64_t snapshot = g_counter;   /* ISR could fire here */

/* CORRECT */
IRQ_CRITICAL_START();
uint64_t snapshot = g_counter;   /* ISR cannot fire here */
IRQ_CRITICAL_END();
```

**Rules for critical sections:**

- Keep them as short as possible
- Never call blocking functions inside (no delays, no polling loops)
- Never call `SysTick_Delay_Ms` inside (it may depend on an interrupt)
- Always declare variables at the top of the function, not inside the
  critical section macros (MISRA Rule 8.1)

---

### `IRQ_GetMstatus()`

```c
static inline uint32_t IRQ_GetMstatus(void);
```

Reads and returns the full `mstatus` register value as a `uint32_t`.

Mostly useful for debugging. If you just want to know if interrupts
are on, use `IRQ_IsEnabled()` instead.

```c
uint32_t status = IRQ_GetMstatus();
/* bit 3 = MIE, bit 7 = MPIE, bits 12:11 = MPP */
```

---

### `IRQ_IsEnabled()`

```c
static inline uint32_t IRQ_IsEnabled(void);
```

Returns `1U` if global interrupts are currently enabled, `0U` if not.

```c
if (IRQ_IsEnabled() == 0U)
{
    /* interrupts are currently disabled — something is wrong */
}
```

---

### `NOP()`

```c
static inline void NOP(void);
```

Inserts one `nop` instruction — one CPU cycle of delay.

At 48MHz each NOP is approximately **20 nanoseconds**.

```c
/* Rough timing reference at 48MHz */
NOP();                            /*  ~20ns */
NOP(); NOP(); NOP(); NOP(); NOP();/*  ~100ns */
/* 10 NOPs */                     /*  ~200ns */
/* 50 NOPs */                     /* ~1000ns = 1µs */
```

**Important warning about compiler optimisation:**

The compiler is allowed to remove `nop` instructions it considers
useless. For real timing-critical bit-banging:

1. Put the function in `.highcode` (runs from RAM)
2. Use `__asm volatile ("nop")` directly, or call `NOP()` from within
   a `.highcode` function where the volatile qualifier is respected

```c
/* Correct bit-bang timing */
__attribute__((section(".highcode")))
static void send_pulse(void)
{
    GPIO_SetPin(GPIO_PORT_D, GPIO_PIN_4);
    NOP(); NOP(); NOP();    /* ~60ns high time */
    GPIO_ClearPin(GPIO_PORT_D, GPIO_PIN_4);
    NOP(); NOP(); NOP(); NOP(); NOP();   /* ~100ns low time */
}
```

---

### `WFI()`

```c
static inline void WFI(void);
```

Executes the RISC-V `wfi` (Wait For Interrupt) instruction. Puts the
CPU into a low-power sleep state until the next interrupt fires.

**This is the correct way to idle your main loop:**

```c
/* WRONG — burns CPU cycles doing nothing */
for (;;) {}

/* CORRECT — CPU sleeps until something needs attention */
for (;;)
{
    WFI();
}
```

When an interrupt fires:

1. CPU wakes from WFI
2. Jumps to the ISR
3. ISR runs
4. CPU returns to the instruction after WFI
5. Loop continues → WFI again

**Requirement:** interrupts must be enabled (MIE = 1) or the CPU will
never wake from WFI. If you call `WFI()` with interrupts disabled the
chip hangs.

**Power impact:** WFI significantly reduces current consumption during
idle. On a battery-powered device this is the difference between hours
and days of runtime.

---

## Common patterns

### Pattern 1 — Interrupt-driven main loop

```c
#include "core.h"

/* ISR sets this flag when work is ready */
static volatile uint32_t g_work_ready = 0U;

void USART1_IRQHandler(void)
{
    /* process byte, set flag */
    g_work_ready = 1U;
}

int main(void)
{
    /* init peripherals... */

    for (;;)
    {
        WFI();   /* sleep until interrupt */

        if (g_work_ready != 0U)
        {
            IRQ_CRITICAL_START();
            g_work_ready = 0U;   /* clear flag atomically */
            IRQ_CRITICAL_END();

            do_work();
        }
    }
}
```

---

### Pattern 2 — Protecting a multi-byte shared buffer

```c
#define BUF_SIZE (64U)
static volatile uint8_t  g_buffer[BUF_SIZE];
static volatile uint32_t g_count = 0U;

/* ISR writes to buffer */
void USART1_IRQHandler(void)
{
    if (g_count < BUF_SIZE)
    {
        g_buffer[g_count] = (uint8_t)(USART1_GetBase()->datar & 0xFFU);
        g_count = g_count + 1U;
    }
}

/* Main reads from buffer */
void process_buffer(void)
{
    uint32_t count_snapshot = 0U;

    /* Take a consistent snapshot of count */
    IRQ_CRITICAL_START();
    count_snapshot = g_count;
    g_count        = 0U;
    IRQ_CRITICAL_END();

    /* Process count_snapshot bytes — ISR can run freely now */
}
```

---

### Pattern 3 — Checking interrupt state before a risky operation

```c
void safe_operation(void)
{
    uint32_t was_enabled = IRQ_IsEnabled();

    IRQ_Disable();

    /* operation that must not be interrupted */

    if (was_enabled != 0U)
    {
        IRQ_Enable();   /* only re-enable if they were on before */
    }
}
```

This is essentially what `IRQ_CRITICAL_START/END` does internally —
written out manually for clarity.

---

## Personal notes — things to remember

**The startup file already enables interrupts.** The `mret` instruction
at the end of `handle_reset` restores `mstatus` with `MIE=1`. By the
time `main()` runs, interrupts are on. Do not add `IRQ_Enable()` at
the top of `main()` thinking it is required — it is already done.

**`volatile` on shared variables is mandatory.** Any variable that is
written in an ISR and read in main (or vice versa) must be declared
`volatile`. Without it the compiler may cache the value in a register
and never notice the ISR changed it. The `volatile` keyword tells the
compiler "always read this from memory, never assume it is unchanged."

```c
/* Without volatile the compiler may never see ISR updates */
static volatile uint32_t g_flag = 0U;   /* volatile is required */
```

**Critical sections do not protect against hardware atomicity.**
On a 32-bit CPU reading a 64-bit value always takes two loads. A
critical section prevents an ISR from firing between the two loads,
making the read safe. But if both your main code and DMA hardware
write the same variable simultaneously, a critical section does not
help — that is a hardware arbitration problem.

**WFI is not the same as disabling interrupts.** During WFI the CPU
is asleep but still responds to interrupts immediately. During
`IRQ_Disable()` the CPU ignores interrupts entirely. They are
completely different operations used for completely different purposes.

**NOP timing is only approximate.** The count of NOPs needed for a
specific delay depends on the clock speed, the compiler optimisation
level, and whether the code is running from flash (with wait states)
or RAM (no wait states). Always verify bit-bang timing with a logic
analyser or oscilloscope. Do not trust NOP counts from code written
for a different chip or clock speed.

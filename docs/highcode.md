# CH32V003 HAL — `.highcode` RAM Execution

> Applies to: CH32V003 (RISC-V RV32EC, 16KB flash, 2KB RAM)
> Linker section: `.highcode`
> GCC attribute: `__attribute__((section(".highcode")))`

---

## The one-line summary

Functions marked with `.highcode` are copied from flash to RAM at
startup and execute from RAM at full CPU speed with no flash wait
states.

---

## Why this matters

The CH32V003 can run the CPU at 48MHz but flash memory has wait states
above 24MHz. At 48MHz the CPU sometimes stalls waiting for flash to
deliver the next instruction.

RAM has no wait states. Code in RAM always runs at full CPU speed.

```
Flash execution at 48MHz:
CPU: fetch → [wait] → execute → fetch → [wait] → execute
              ↑ stall                    ↑ stall

RAM execution at 48MHz:
CPU: fetch → execute → fetch → execute
             no stalls
```

---

## How to use it

Add `__attribute__((section(".highcode")))` before any function:

```c
__attribute__((section(".highcode")))
static void my_time_critical_function(void)
{
    /* this runs from RAM */
}
```

That is all. The linker script and startup file handle the rest
automatically at boot.

---

## When to use it

### Use it for:

**Bit-banging protocols** — protocols like WS2812 (NeoPixel), DHT22,
or 1-Wire require precise pin timing down to hundreds of nanoseconds.
Flash wait states cause timing jitter that corrupts the signal.

```c
__attribute__((section(".highcode")))
static void ws2812_send_byte(uint8_t byte)
{
    uint32_t i = 0U;
    for (i = 0U; i < 8U; i = i + 1U)
    {
        if ((byte & 0x80U) != 0U)
        {
            GPIO_SetPin(GPIO_PORT_D, GPIO_PIN_4);
            __asm volatile ("nop\nnop\nnop\nnop\nnop\nnop");
            GPIO_ClearPin(GPIO_PORT_D, GPIO_PIN_4);
            __asm volatile ("nop\nnop\nnop\nnop");
        }
        else
        {
            GPIO_SetPin(GPIO_PORT_D, GPIO_PIN_4);
            __asm volatile ("nop\nnop");
            GPIO_ClearPin(GPIO_PORT_D, GPIO_PIN_4);
            __asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop");
        }
        byte = (uint8_t)(byte << 1U);
    }
}
```

---

**Time-critical ISRs** — if an interrupt handler must complete within
a tight deadline (e.g. USART RX at high baud rates, motor control
ISRs), running from RAM gives deterministic cycle counts:

```c
__attribute__((section(".highcode")))
__attribute__((interrupt("WCH-Interrupt-fast")))
void USART1_IRQHandler(void)
{
    volatile USART_TypeDef * const p = USART1_GetBase();

    if ((p->statr & USART_STATR_RXNE_MASK) != 0U)
    {
        g_rx_buf[g_rx_head] = (uint8_t)(p->datar & 0xFFU);
        g_rx_head = (uint8_t)((g_rx_head + 1U) & (RX_BUF_SIZE - 1U));
    }
}
```

---

**Code that runs during flash erase or write** — while the flash
controller is busy writing, it cannot serve instruction fetches. Any
code that executes during a flash write operation must be in RAM or
the CPU will stall indefinitely:

```c
__attribute__((section(".highcode")))
static void flash_wait_busy(void)
{
    /* Must be in RAM — flash is unavailable during write/erase */
    while ((FLASH->STATR & FLASH_STATR_BSY_MASK) != 0U)
    {
        /* busy wait */
    }
}
```

---

**Soft real-time inner loops** — any loop that must run at a
predictable rate and is called frequently enough that flash latency
accumulates into a measurable timing error:

```c
__attribute__((section(".highcode")))
static void pid_update(void)
{
    /* Called every 1ms from SysTick ISR
     * Must complete within 20µs — RAM execution guarantees this   */
    int32_t error    = g_setpoint - g_measured;
    g_integral       = g_integral + error;
    g_output         = (g_kp * error) + (g_ki * g_integral);
}
```

---

### Do NOT use it for:

**Initialisation code** — functions called once at startup have no
timing requirements. Putting `RCC_Init_48mhz()` in `.highcode` wastes
RAM for zero benefit.

**Large utility functions** — string processing, protocol parsers,
anything that is not on the critical timing path. Keep these in flash.

**Everything** — this is the most important rule. RAM is scarce (2KB
total). `.highcode` functions live in RAM permanently from startup
regardless of whether they are currently running.

---

## The RAM cost

Every byte of `.highcode` comes directly out of your 2KB RAM budget.
Your RAM is divided as:

```
RAM (2048 bytes)
├── .highcode    ← permanent cost, resident from startup
├── .data        ← your initialised globals
├── .bss         ← your uninitialised globals
└── stack        ← grows downward from top of RAM (1024 bytes reserved)
```

Check your `.map` file after building to see exactly how much RAM each
section consumes:

```
riscv32-unknown-elf-size firmware.elf
   text    data     bss     dec     hex filename
  4832     128     512    5472    1560 firmware.elf
```

`data` includes `.highcode`. If RAM usage is close to 2048 bytes, move
functions back to flash.

---

## What happens at startup

The linker stores `.highcode` functions in flash (they have to live
somewhere permanent). The startup file copies them to RAM before
`main()` runs. This copy happens automatically — you never see it:

```
Power on
    │
    ▼
handle_reset runs
    │
    ├── copy .highcode from flash → RAM   ← happens here
    ├── copy .data    from flash → RAM
    ├── zero .bss
    └── call main()
    │
    ▼
main() — .highcode functions are already in RAM, ready to call
```

---

## Checking it worked

After building, inspect the map file to confirm your function landed
in the `.highcode` section:

```
riscv32-unknown-elf-objdump -h firmware.elf | grep highcode
```

Or check the full symbol list:

```
riscv32-unknown-elf-nm firmware.elf | grep ws2812
```

The address should be in the RAM range (`0x20000000` onwards), not
the flash range (`0x00000000` onwards).

---

## Personal note — when to reach for this

`.highcode` is a sharp tool. The rule of thumb:

> If you can measure the timing problem with an oscilloscope or logic
> analyser, use `.highcode`. If you are adding it speculatively, do
> not — you cannot afford the RAM.

On the CH32V003 with 2KB RAM the practical limit is one or two small
functions — a bit-bang driver and maybe a hot ISR. Beyond that the
stack runs out of room.

The most common real use case is WS2812/NeoPixel LED control.
The second most common is a USART ISR that must handle 1Mbaud+
without dropping bytes.

Everything else in this HAL runs fine from flash at 48MHz.

# CH32V003 HAL — USART Documentation

> Reference: CH32V003 Reference Manual V1.9, Chapter 12
> Header: `usart.h`
> Depends on: `rcc.h`, `gpio.h`

---

## Table of Contents

1. [Overview](#overview)
2. [How USART works](#how-usart-works)
3. [Baud rate](#baud-rate)
4. [Types reference](#types-reference)
5. [API reference](#api-reference)
6. [Pre-built configurations](#pre-built-configurations)
7. [Pin setup — required before init](#pin-setup--required-before-init)
8. [Complete examples](#complete-examples)
9. [Error handling](#error-handling)
10. [Common baud rates at 48MHz](#common-baud-rates-at-48mhz)

---

## Overview

USART1 is the only USART peripheral on the CH32V003. This driver
provides polling-mode transmit and receive — the CPU waits for each
operation to complete. Interrupt-driven operation (receive into a ring
buffer) is planned for a later version.

**The mandatory startup sequence:**

```
1. RCC_Init_48mhz()                         clock first
2. RCC_Enable_Clock(RCC_PERIPH_AFIO)        AFIO before GPIO
3. RCC_Enable_Clock(RCC_PERIPH_GPIOx)       the port TX/RX live on
4. RCC_Enable_Clock(RCC_PERIPH_USART1)      USART clock
5. GPIO_SetPinMode(TX pin, AF_PP_50)        TX as alternate function
6. GPIO_SetPinMode(RX pin, IN_FLOATING)     RX as floating input
7. USART1_Init(&config)                     configure and enable
```

---

## How USART works

USART is an asynchronous serial protocol. Each byte is framed as:

```
[start bit][data bits][parity bit][stop bit(s)]

Standard 8N1 (8 data, no parity, 1 stop):
[0][d0 d1 d2 d3 d4 d5 d6 d7][1]
 │                            └── stop bit (line returns idle high)
 └── start bit (line goes low — receiver detects this as frame start)
```

Both sides must agree on baud rate, word length, parity, and stop bits
before communication works. 8N1 at 115200 is the de-facto standard for
debug output.

**Transmit flow:**

```
CPU writes byte to DATAR
      │
      ▼
Byte moves to shift register (TXE flag goes back to 1)
      │
      ▼
Hardware clocks out bits one at a time at baud rate
      │
      ▼
TC flag set when last bit is shifted out
```

**Receive flow:**

```
Remote device sends framed byte
      │
      ▼
Hardware samples line at baud rate, assembles byte
      │
      ▼
Byte moves to DATAR (RXNE flag set)
      │
      ▼
CPU reads DATAR (RXNE cleared automatically)
```

---

## Baud rate

The BRR register controls baud rate:

```
BRR = HCLK / Baud   (rounded integer)
```

The hardware interprets BRR as a 16× oversampled divider — the lower
4 bits are the fractional part (sixteenths) and the upper 12 bits are
the integer part. Our formula computes the correct combined value
without floating point:

```c
brr_val = (hclk + (baud / 2U)) / baud;   /* rounded */
```

The `+ baud/2` before dividing gives correct rounding rather than
truncation, minimising the baud rate error.

**BRR must be >= 16** — meaning the maximum baud rate is `HCLK / 16`.
At 48MHz that is 3,000,000 baud. `USART1_Init` returns `USART_ERR_BAUD`
if the computed BRR would be too small.

---

## Types reference

### `USART_WordLen_t`

| Value | Meaning |
|---|---|
| `USART_WORD_8BIT` | 8 data bits — standard |
| `USART_WORD_9BIT` | 9 data bits — multiprocessor or parity with 8 data bits |

---

### `USART_Parity_t`

| Value | Meaning |
|---|---|
| `USART_PARITY_NONE` | No parity bit — standard |
| `USART_PARITY_EVEN` | Even parity |
| `USART_PARITY_ODD` | Odd parity |

> When parity is enabled, the parity bit occupies the MSB of the data
> word. If using 8-bit data with parity, set `USART_WORD_9BIT` so the
> parity bit does not eat into your 8 data bits.

---

### `USART_StopBits_t`

| Value | Meaning |
|---|---|
| `USART_STOP_1` | 1 stop bit — standard async |
| `USART_STOP_2` | 2 stop bits — slower devices or noisy lines |
| `USART_STOP_0_5` | 0.5 stop bits — smartcard mode only |
| `USART_STOP_1_5` | 1.5 stop bits — smartcard mode only |

---

### `USART_FlowCtrl_t`

| Value | Meaning |
|---|---|
| `USART_FLOW_NONE` | No hardware flow control — standard |
| `USART_FLOW_RTS` | RTS output only |
| `USART_FLOW_CTS` | CTS input only |
| `USART_FLOW_BOTH` | RTS and CTS |

> Hardware flow control requires additional GPIO pins to be configured
> as AF_PP (RTS) or IN_FLOATING/IN_PULLUP (CTS). See the GPIO docs
> for the correct pins per remap setting.

---

### `USART_Mode_t`

| Value | Meaning |
|---|---|
| `USART_MODE_TX_ONLY` | Transmitter enabled only |
| `USART_MODE_RX_ONLY` | Receiver enabled only |
| `USART_MODE_TX_RX` | Full duplex — TX and RX both enabled |

---

### `USART_InitTypeDef`

```c
typedef struct {
    uint32_t          baud;       /* baud rate in Hz  e.g. 115200U     */
    USART_WordLen_t   word_len;   /* USART_WORD_8BIT or _9BIT          */
    USART_Parity_t    parity;     /* USART_PARITY_NONE / _EVEN / _ODD  */
    USART_StopBits_t  stop_bits;  /* USART_STOP_1 / _2 / _0_5 / _1_5  */
    USART_FlowCtrl_t  flow_ctrl;  /* USART_FLOW_NONE / _RTS / _CTS     */
    USART_Mode_t      mode;       /* USART_MODE_TX_ONLY / _RX / _TX_RX */
} USART_InitTypeDef;
```

---

### `USART_Result_t`

| Value | Meaning |
|---|---|
| `USART_OK` | Success |
| `USART_ERR_BAUD` | HCLK unknown or baud rate too high (BRR < 16) |
| `USART_ERR_BUSY` | Timeout waiting for TXE, TC, or RXNE flag |
| `USART_ERR_RX` | Receive error — framing (FE), noise (NE), or overrun (ORE) |
| `USART_ERR_PARAM` | NULL pointer or zero length passed to function |

---

## API Reference

### `USART1_Init`

```c
static USART_Result_t USART1_Init(const USART_InitTypeDef * const p_config);
```

Configures and enables USART1. Follows the required register write order:

1. Disable USART (clear UE)
2. Write BRR (baud rate)
3. Write CTLR2 (stop bits)
4. Write CTLR3 (flow control)
5. Write CTLR1 (word length, parity, TX/RX enable)
6. Enable USART (set UE last)

**Must be called after** GPIO pins are configured as AF_PP (TX) and
IN_FLOATING (RX). Writing CTLR1/2/3 before UE is set is required by
the hardware.

Returns `USART_ERR_BAUD` if `RCC_Get_Hclk_Hz()` returns
`RCC_FREQ_UNKNOWN` or if the requested baud rate would produce a BRR
value below 16.

---

### `USART1_Init_115200`

```c
static USART_Result_t USART1_Init_115200(void);
```

Convenience wrapper. Calls `USART1_Init(&USART_Config_115200_8N1)`.
8N1, no parity, no flow control, full duplex.

---

### `USART1_Init_9600`

```c
static USART_Result_t USART1_Init_9600(void);
```

Convenience wrapper. Calls `USART1_Init(&USART_Config_9600_8N1)`.

---

### `USART1_SendByte`

```c
static USART_Result_t USART1_SendByte(uint8_t byte);
```

Transmits a single byte. Polls TXE until the transmit data register is
empty, then writes the byte.

Does **not** wait for TC (transmission complete). This means the byte
may still be shifting out when the function returns. This is correct
for back-to-back byte transmissions — the next call polls TXE again
and the hardware handles the pipeline.

Use `USART1_SendByte_WaitTC` for the final byte of a transmission if
you need to know the line is completely idle.

Returns `USART_ERR_BUSY` if TXE does not go high within `USART_TIMEOUT`
iterations.

---

### `USART1_SendByte_WaitTC`

```c
static USART_Result_t USART1_SendByte_WaitTC(uint8_t byte);
```

Transmits a single byte and waits for TC (transmission complete) —
the entire frame has been shifted out and the line is idle.

Use when:
- Sending the final byte of a message
- Before disabling USART
- Before changing baud rate
- Before switching a half-duplex line from TX to RX

Returns `USART_ERR_BUSY` if TC does not go high within timeout.

---

### `USART1_SendBuffer`

```c
static USART_Result_t USART1_SendBuffer(const uint8_t * const p_data,
                                        uint32_t              len);
```

Transmits `len` bytes from `p_data`. Uses `USART1_SendByte` for all
bytes except the last, which uses `USART1_SendByte_WaitTC`.

Returns `USART_ERR_PARAM` if `p_data` is NULL or `len` is 0.
Returns `USART_ERR_BUSY` if any byte times out.
Stops transmitting immediately on any error.

```c
uint8_t data[] = {0x01U, 0x02U, 0x03U, 0xFFU};
USART_Result_t result = USART1_SendBuffer(data, 4U);
```

---

### `USART1_SendString`

```c
static USART_Result_t USART1_SendString(const char * const p_str);
```

Transmits a null-terminated string. Walks the string until `'\0'`.
Maximum length is `USART_MAX_STRING_LEN` (1024) bytes as a safety
guard against unterminated strings.

Does not append `\r\n` — include them in your string if needed.

Returns `USART_ERR_PARAM` if `p_str` is NULL.

```c
USART1_SendString("Hello\r\n");
USART1_SendString("Status: OK\r\n");
```

---

### `USART1_ReceiveByte`

```c
static USART_Result_t USART1_ReceiveByte(uint8_t * const p_byte);
```

Waits for RXNE (receive data register not empty) then reads one byte
into `*p_byte`. Reading DATAR automatically clears RXNE.

After reading, checks STATR for FE, NE, and ORE error flags. Returns
`USART_ERR_RX` if any error flag was set. The byte is still written to
`*p_byte` even on error — caller decides whether to discard it.

Returns `USART_ERR_BUSY` if RXNE never goes high within timeout.
Returns `USART_ERR_PARAM` if `p_byte` is NULL.

```c
uint8_t byte = 0U;
USART_Result_t result = USART1_ReceiveByte(&byte);
if (result == USART_OK) {
    /* byte is valid */
} else if (result == USART_ERR_RX) {
    /* framing or overrun error — byte may be corrupted */
} else {
    /* timeout — no byte arrived */
}
```

---

### `USART1_DataAvailable`

```c
static uint32_t USART1_DataAvailable(void);
```

Returns `1U` if a received byte is waiting in DATAR, `0U` if not.
Non-blocking — does not poll or wait.

Use in your main loop to check for incoming data without blocking:

```c
for (;;)
{
    if (USART1_DataAvailable() != 0U)
    {
        uint8_t byte = 0U;
        USART1_ReceiveByte(&byte);
        /* process byte */
    }

    /* continue doing other work */
}
```

---

## Pre-built configurations

### `USART_Config_115200_8N1`

```
Baud:      115200
Data bits: 8
Parity:    None
Stop bits: 1
Flow ctrl: None
Mode:      TX + RX
```

Standard configuration for debug output and most host communication.

### `USART_Config_9600_8N1`

```
Baud:      9600
Data bits: 8
Parity:    None
Stop bits: 1
Flow ctrl: None
Mode:      TX + RX
```

Common for GPS modules, GSM modems, and older sensor modules.

---

## Pin setup — required before init

USART1 pins depend on which remap is active. These are the four options.
See the GPIO documentation for full tables.

### Remap 0 — Default (TX=PD5, RX=PD6)

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
RCC_Enable_Clock(RCC_PERIPH_USART1);

GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM00_RX_PORT, USART1_RM00_RX_PIN, GPIO_MODE_IN_FLOATING);
/* No AFIO remap needed — reset default */
```

### Remap 1 (TX=PD0, RX=PD1)

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
RCC_Enable_Clock(RCC_PERIPH_USART1);
AFIO_SetUsart1Remap(1U);

GPIO_SetPinMode(USART1_RM01_TX_PORT, USART1_RM01_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM01_RX_PORT, USART1_RM01_RX_PIN, GPIO_MODE_IN_FLOATING);
```

### Remap 2 (TX=PD6, RX=PD5)

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
RCC_Enable_Clock(RCC_PERIPH_USART1);
AFIO_SetUsart1Remap(2U);

GPIO_SetPinMode(USART1_RM10_TX_PORT, USART1_RM10_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM10_RX_PORT, USART1_RM10_RX_PIN, GPIO_MODE_IN_FLOATING);
```

### Remap 3 (TX=PC0, RX=PC1)

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);
RCC_Enable_Clock(RCC_PERIPH_USART1);
AFIO_SetUsart1Remap(3U);

GPIO_SetPinMode(USART1_RM11_TX_PORT, USART1_RM11_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM11_RX_PORT, USART1_RM11_RX_PIN, GPIO_MODE_IN_FLOATING);
```

---

## Complete examples

### Minimal — TX only debug output

```c
#include "rcc.h"
#include "systick.h"
#include "gpio.h"
#include "usart.h"

int main(void)
{
    /* Clock */
    if (RCC_Init_48mhz() != RCC_OK) { for (;;) {} }

    /* Peripheral clocks */
    RCC_Enable_Clock(RCC_PERIPH_AFIO);
    RCC_Enable_Clock(RCC_PERIPH_GPIOD);
    RCC_Enable_Clock(RCC_PERIPH_USART1);

    /* SysTick */
    SysTick_Init_FreeRunning();

    /* GPIO — default remap, TX only */
    GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);

    /* USART — TX only mode */
    USART_InitTypeDef config = {
        115200U,
        USART_WORD_8BIT,
        USART_PARITY_NONE,
        USART_STOP_1,
        USART_FLOW_NONE,
        USART_MODE_TX_ONLY
    };

    if (USART1_Init(&config) != USART_OK) { for (;;) {} }

    USART1_SendString("Boot OK\r\n");

    for (;;)
    {
        USART1_SendString("Alive\r\n");
        SysTick_Delay_Ms(1000U);
    }
}
```

---

### Full duplex — echo server

```c
int main(void)
{
    if (RCC_Init_48mhz() != RCC_OK) { for (;;) {} }

    RCC_Enable_Clock(RCC_PERIPH_AFIO);
    RCC_Enable_Clock(RCC_PERIPH_GPIOD);
    RCC_Enable_Clock(RCC_PERIPH_USART1);

    SysTick_Init_FreeRunning();

    GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);
    GPIO_SetPinMode(USART1_RM00_RX_PORT, USART1_RM00_RX_PIN, GPIO_MODE_IN_FLOATING);

    if (USART1_Init_115200() != USART_OK) { for (;;) {} }

    USART1_SendString("Echo server ready\r\n");

    for (;;)
    {
        if (USART1_DataAvailable() != 0U)
        {
            uint8_t        byte   = 0U;
            USART_Result_t result = USART1_ReceiveByte(&byte);

            if (result == USART_OK)
            {
                USART1_SendByte(byte);   /* echo */
            }
            else if (result == USART_ERR_RX)
            {
                USART1_SendString("!RX_ERR\r\n");
            }
            else
            {
                /* timeout — nothing received in time */
            }
        }
    }
}
```

---

### Send a binary frame

```c
/* Protocol frame: [0xAA][len][data...][checksum] */
static USART_Result_t send_frame(const uint8_t * const p_data,
                                 uint8_t               len)
{
    uint8_t        header[2U] = {0xAAU, len};
    uint8_t        checksum   = 0U;
    uint32_t       i          = 0U;
    USART_Result_t result     = USART_OK;

    /* Compute checksum */
    for (i = 0U; i < (uint32_t)len; i = i + 1U)
    {
        checksum = (uint8_t)(checksum ^ p_data[i]);
    }

    result = USART1_SendBuffer(header, 2U);

    if (result == USART_OK)
    {
        result = USART1_SendBuffer(p_data, (uint32_t)len);
    }

    if (result == USART_OK)
    {
        result = USART1_SendByte_WaitTC(checksum);
    }

    return result;
}
```

---

### Receive with timeout using Timeout_t

```c
static USART_Result_t receive_with_timeout(uint8_t * const p_byte,
                                           uint32_t        timeout_ms)
{
    Timeout_t      t      = Timeout_New_Ms(timeout_ms);
    USART_Result_t result = USART_ERR_BUSY;

    while (Timeout_Is_Expired(&t) == 0U)
    {
        if (USART1_DataAvailable() != 0U)
        {
            result = USART1_ReceiveByte(p_byte);
            break;
        }
    }

    return result;
}

/* Usage — wait up to 100ms for a response byte */
uint8_t response = 0U;
if (receive_with_timeout(&response, 100U) == USART_OK)
{
    /* got a byte within 100ms */
}
else
{
    /* timed out */
}
```

---

## Error handling

| Error | Cause | What to do |
|---|---|---|
| `USART_ERR_BAUD` | `RCC_FREQ_UNKNOWN` or baud > HCLK/16 | Call `RCC_Init_*` before `USART1_Init` |
| `USART_ERR_BUSY` on TX | Hardware stalled | Check TX GPIO is configured as AF_PP, check USART clock is enabled |
| `USART_ERR_BUSY` on RX | No data arrived within timeout | Sender not transmitting, or baud mismatch |
| `USART_ERR_RX` — FE | Framing error | Baud rate mismatch between sender and receiver |
| `USART_ERR_RX` — NE | Noise error | Line quality issue, long cable, no termination |
| `USART_ERR_RX` — ORE | Overrun error | CPU too slow reading DATAR — byte arrived before previous one was read |
| `USART_ERR_PARAM` | NULL pointer | Check your pointer arguments |

---

## Common baud rates at 48MHz HCLK

| Baud rate | BRR value | Actual baud | Error |
|---|---|---|---|
| 1200 | 40000 | 1200 | 0.00% |
| 2400 | 20000 | 2400 | 0.00% |
| 4800 | 10000 | 4800 | 0.00% |
| 9600 | 5000 | 9600 | 0.00% |
| 19200 | 2500 | 19200 | 0.00% |
| 38400 | 1250 | 38400 | 0.00% |
| 57600 | 833 | 57,624 | 0.04% |
| 115200 | 417 | 115,108 | 0.08% |
| 230400 | 208 | 230,769 | 0.16% |
| 460800 | 104 | 461,538 | 0.16% |
| 921600 | 52 | 923,077 | 0.16% |
| 1000000 | 48 | 1,000,000 | 0.00% |
| 2000000 | 24 | 2,000,000 | 0.00% |
| 3000000 | 16 | 3,000,000 | 0.00% |

All values are within UART's ±2% tolerance.
`BRR = (48,000,000 + baud/2) / baud` computes these automatically.
The maximum baud rate at 48MHz is 3,000,000 (BRR=16).

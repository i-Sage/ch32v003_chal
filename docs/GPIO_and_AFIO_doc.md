# CH32V003 HAL — GPIO & AFIO Documentation

> Reference: CH32V003 Reference Manual V1.9, Chapters 7.2 and 7.3
> Header: `gpio.h`
> Depends on: `rcc.h`

---

## Table of Contents

1. [Overview](#overview)
2. [GPIO Ports](#gpio-ports)
3. [Pin Modes Reference](#pin-modes-reference)
4. [API Reference](#api-reference)
5. [AFIO — Alternate Function I/O](#afio--alternate-function-io)
6. [Peripheral Pin Assignments](#peripheral-pin-assignments)
   - [USART1](#usart1)
   - [SPI1](#spi1)
   - [I2C1](#i2c1)
   - [TIM1](#tim1)
   - [TIM2](#tim2)
7. [Complete Examples](#complete-examples)

---

## Overview

The CH32V003 has three GPIO ports: **PA**, **PC**, and **PD**. There is no PB.
Each port has 8 pins (pin 0 through pin 7).

Every pin is configured through a single 4-bit field in the `CFGLR` register —
2 bits for direction/speed (`MODE`) and 2 bits for type (`CNF`). The
`GPIO_PinMode_t` enum encodes all valid MODE+CNF combinations so you never
have to think about the raw bit values.

**The mandatory startup sequence for any GPIO pin:**

```c
/* 1. Enable the port clock */
RCC_Enable_Clock(RCC_PERIPH_GPIOx);

/* 2. For alternate function pins — enable AFIO clock too */
RCC_Enable_Clock(RCC_PERIPH_AFIO);

/* 3. Configure the pin mode */
GPIO_SetPinMode(GPIO_PORT_x, GPIO_PIN_y, mode);

/* 4. For alternate function — configure remap if not using default */
AFIO_SetXxxRemap(remap_value);
```

---

## GPIO Ports

| Enum | Port | Base address | RCC clock to enable |
|---|---|---|---|
| `GPIO_PORT_A` | PA | `0x40010800` | `RCC_PERIPH_GPIOA` |
| `GPIO_PORT_C` | PC | `0x40011000` | `RCC_PERIPH_GPIOC` |
| `GPIO_PORT_D` | PD | `0x40011400` | `RCC_PERIPH_GPIOD` |

**Pin enum values** — the same set applies to all three ports:

| Enum | Pin number |
|---|---|
| `GPIO_PIN_0` | Pin 0 |
| `GPIO_PIN_1` | Pin 1 |
| `GPIO_PIN_2` | Pin 2 |
| `GPIO_PIN_3` | Pin 3 |
| `GPIO_PIN_4` | Pin 4 |
| `GPIO_PIN_5` | Pin 5 |
| `GPIO_PIN_6` | Pin 6 |
| `GPIO_PIN_7` | Pin 7 |

---

## Pin Modes Reference

### Input modes

| `GPIO_PinMode_t` value | Description | When to use |
|---|---|---|
| `GPIO_MODE_IN_ANALOG` | Analog input — pin disconnected from digital logic | ADC inputs, OPA inputs |
| `GPIO_MODE_IN_FLOATING` | Floating input — no pull resistor | USART RX, SPI MISO master, timer ETR/BKIN/capture, EXTI |
| `GPIO_MODE_IN_PULLDOWN` | Digital input with internal pull-down to GND | Button wired to VCC, idle-high signals |
| `GPIO_MODE_IN_PULLUP` | Digital input with internal pull-up to VCC | Button wired to GND, open-drain bus lines |

```c
/* Button on PD3, wired to GND — needs pull-up */
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_PULLUP);

uint32_t pressed = (GPIO_ReadPin(GPIO_PORT_D, GPIO_PIN_3) == 0U) ? 1U : 0U;
```

```c
/* ADC input on PC4 — must be analog, not floating */
RCC_Enable_Clock(RCC_PERIPH_GPIOC);
GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_4, GPIO_MODE_IN_ANALOG);
```

---

### Output modes — push-pull

Push-pull drives the pin actively to both VCC and GND.
Use for LEDs, logic outputs, and most digital signals.

| `GPIO_PinMode_t` value | Speed | Description |
|---|---|---|
| `GPIO_MODE_OUT_PP_2` | 2 MHz max | Lowest power, slowest edges |
| `GPIO_MODE_OUT_PP_10` | 10 MHz max | General purpose |
| `GPIO_MODE_OUT_PP_50` | 50 MHz max | High speed signals |

```c
/* LED on PD4 — push-pull, 2MHz is plenty for an LED */
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_4, GPIO_MODE_OUT_PP_2);

GPIO_SetPin(GPIO_PORT_D, GPIO_PIN_4);     /* LED on  */
GPIO_ClearPin(GPIO_PORT_D, GPIO_PIN_4);   /* LED off */
GPIO_TogglePin(GPIO_PORT_D, GPIO_PIN_4);  /* toggle  */
```

---

### Output modes — open-drain

Open-drain can only pull the pin to GND. The pin floats high unless an
external pull-up resistor is present. Required for I2C and for
wired-AND bus topologies.

| `GPIO_PinMode_t` value | Speed | Description |
|---|---|---|
| `GPIO_MODE_OUT_OD_2` | 2 MHz max | Open-drain, low power |
| `GPIO_MODE_OUT_OD_10` | 10 MHz max | Open-drain, general |
| `GPIO_MODE_OUT_OD_50` | 50 MHz max | Open-drain, high speed |

```c
/* Single-wire bus line — open-drain with external 4.7kΩ pull-up */
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_6, GPIO_MODE_OUT_OD_10);

GPIO_ClearPin(GPIO_PORT_D, GPIO_PIN_6);  /* pull line low  */
GPIO_SetPin(GPIO_PORT_D, GPIO_PIN_6);    /* release line   */
```

---

### Alternate function modes — push-pull

Used for peripheral signals that drive the line (USART TX, SPI MOSI
master, SPI SCK master, TIM output compare, MCO).

| `GPIO_PinMode_t` value | Speed | Description |
|---|---|---|
| `GPIO_MODE_AF_PP_2` | 2 MHz max | AF push-pull, low power |
| `GPIO_MODE_AF_PP_10` | 10 MHz max | AF push-pull, general |
| `GPIO_MODE_AF_PP_50` | 50 MHz max | AF push-pull, high speed |

```c
/* USART1 TX on PD5 (default remap) — peripheral drives the line */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_5, GPIO_MODE_AF_PP_50);
```

---

### Alternate function modes — open-drain

Used for I2C (which requires open-drain by the I2C standard) and
for USART TX in half-duplex synchronous mode.

| `GPIO_PinMode_t` value | Speed | Description |
|---|---|---|
| `GPIO_MODE_AF_OD_2` | 2 MHz max | AF open-drain, low power |
| `GPIO_MODE_AF_OD_10` | 10 MHz max | AF open-drain, general |
| `GPIO_MODE_AF_OD_50` | 50 MHz max | AF open-drain, high speed |

```c
/* I2C1 SCL on PC2 — must be open-drain, external pull-up required */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);
GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_2, GPIO_MODE_AF_OD_50);
GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_1, GPIO_MODE_AF_OD_50);
```

---

### Speed selection guide

| Speed | Use when |
|---|---|
| `_2` (2MHz) | LEDs, slow control signals, I2C at 100kHz, power saving |
| `_10` (10MHz) | UART up to ~1Mbaud, I2C at 400kHz, general AF |
| `_50` (50MHz) | SPI, high-speed UART, timer PWM, any signal > 4MHz |

Higher speed settings consume more power and produce faster edges which
can cause more EMI. Always use the minimum speed that meets your timing.

---

## API Reference

### `GPIO_SetPinMode`

```c
static void GPIO_SetPinMode(GPIO_Port_t port, GPIO_Pin_t pin, GPIO_PinMode_t mode);
```

Configures a single pin. Handles the `OUTDR` pull-direction bit
automatically for `GPIO_MODE_IN_PULLUP` and `GPIO_MODE_IN_PULLDOWN`.

Must be called after `RCC_Enable_Clock` for the port.

---

### `GPIO_SetPin`

```c
static void GPIO_SetPin(GPIO_Port_t port, GPIO_Pin_t pin);
```

Drives a pin high. Single atomic write to `BSHR` — no read-modify-write.
Safe to call from interrupt context.

---

### `GPIO_ClearPin`

```c
static void GPIO_ClearPin(GPIO_Port_t port, GPIO_Pin_t pin);
```

Drives a pin low. Single atomic write to `BCR` — no read-modify-write.
Safe to call from interrupt context.

---

### `GPIO_TogglePin`

```c
static void GPIO_TogglePin(GPIO_Port_t port, GPIO_Pin_t pin);
```

Reads `OUTDR` then uses `BSHR` or `BCR` to set or clear atomically.
The read introduces a small race window — if strict toggle atomicity is
required, disable interrupts around this call.

---

### `GPIO_ReadPin`

```c
static uint32_t GPIO_ReadPin(GPIO_Port_t port, GPIO_Pin_t pin);
```

Returns `1U` if the pin is high, `0U` if low. Reads `INDR` regardless
of pin mode — works on both input and output configured pins.

---

### `GPIO_WritePort`

```c
static void GPIO_WritePort(GPIO_Port_t port, uint8_t value);
```

Writes all 8 pins simultaneously via `OUTDR`. Bits 0-7 of `value` map
to pins 0-7. Use for initialising a port to a known state. For runtime
single-pin control use `GPIO_SetPin` / `GPIO_ClearPin`.

---

### `GPIO_ReadPort`

```c
static uint8_t GPIO_ReadPort(GPIO_Port_t port);
```

Reads all 8 input pins simultaneously. Returns bits [7:0].

---

## AFIO — Alternate Function I/O

AFIO has two jobs:

**Remapping** — moves peripheral pins from their default locations to
alternate locations. Must be configured before the peripheral is enabled.

**EXTI routing** — selects which port's pin drives each external interrupt
line.

**AFIO requires its own clock:**

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);   /* always before writing AFIO registers */
```

---

### Remap helper functions

#### `AFIO_SetUsart1Remap(remap)`

Sets USART1 pin mapping. `remap` is 0–3. See USART1 table below.

#### `AFIO_SetSpi1Remap(remap)`

Sets SPI1 pin mapping. `remap` is 0–1. See SPI1 table below.

#### `AFIO_SetI2c1Remap(remap)`

Sets I2C1 pin mapping. `remap` is 0–2. See I2C1 table below.

#### `AFIO_SetTim1Remap(remap)`

Sets TIM1 pin mapping. `remap` is 0–3. See TIM1 table below.

#### `AFIO_SetTim2Remap(remap)`

Sets TIM2 pin mapping. `remap` is 0–3. See TIM2 table below.

#### `AFIO_SetExtiPort(exti_line, port)`

Routes an EXTI line (0–7) to a specific port.

```c
/* Route EXTI3 to PD3 */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
AFIO_SetExtiPort(3U, GPIO_PORT_D);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_PULLUP);
```

---

### SWD debug interface — SWCFG warning

`AFIO_PCFR1` bits [26:24] control the SWD (debug) interface.
**Do not modify `SWCFG` unless you have an alternative programming method.**
Setting `SWCFG=100` permanently disables SWD until the next power cycle,
removing your debug/programming connection.

---

## Peripheral Pin Assignments

The tables below show every pin for every remap setting.
Use the named constants (`USART1_RM00_TX_PORT` etc.) in your code
rather than raw port/pin values.

---

### USART1

**GPIO configuration required (from manual Table 7-3):**

| USART pin | Mode | Required GPIO mode |
|---|---|---|
| TX | Full-duplex | `GPIO_MODE_AF_PP_50` |
| TX | Half-duplex synchronous | `GPIO_MODE_AF_OD_50` |
| RX | Full-duplex | `GPIO_MODE_IN_FLOATING` or `GPIO_MODE_IN_PULLUP` |
| CK | Synchronous mode | `GPIO_MODE_AF_PP_50` |
| RTS | Hardware flow control | `GPIO_MODE_AF_PP_50` |
| CTS | Hardware flow control | `GPIO_MODE_IN_FLOATING` or `GPIO_MODE_IN_PULLUP` |

#### Remap 0 — Default mapping

`AFIO_SetUsart1Remap(0U)` — this is the reset default, call is optional.

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| CK | PD4 | `USART1_RM00_CK_PORT` | `USART1_RM00_CK_PIN` |
| TX | PD5 | `USART1_RM00_TX_PORT` | `USART1_RM00_TX_PIN` |
| RX | PD6 | `USART1_RM00_RX_PORT` | `USART1_RM00_RX_PIN` |
| CTS | PD3 | `USART1_RM00_CTS_PORT` | `USART1_RM00_CTS_PIN` |
| RTS | PC2 | `USART1_RM00_RTS_PORT` | `USART1_RM00_RTS_PIN` |

```c
/* USART1 full-duplex, default pins */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);

GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM00_RX_PORT, USART1_RM00_RX_PIN, GPIO_MODE_IN_FLOATING);
```

#### Remap 1 — Partial mapping

`AFIO_SetUsart1Remap(1U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| CK | PD7 | `USART1_RM01_CK_PORT` | `USART1_RM01_CK_PIN` |
| TX | PD0 | `USART1_RM01_TX_PORT` | `USART1_RM01_TX_PIN` |
| RX | PD1 | `USART1_RM01_RX_PORT` | `USART1_RM01_RX_PIN` |
| CTS | PC3 | `USART1_RM01_CTS_PORT` | `USART1_RM01_CTS_PIN` |
| RTS | PC2 | `USART1_RM01_RTS_PORT` | `USART1_RM01_RTS_PIN` |

```c
/* USART1 full-duplex, remap 1 — TX=PD0, RX=PD1 */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
AFIO_SetUsart1Remap(1U);

GPIO_SetPinMode(USART1_RM01_TX_PORT, USART1_RM01_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM01_RX_PORT, USART1_RM01_RX_PIN, GPIO_MODE_IN_FLOATING);
```

#### Remap 2 — Partial mapping

`AFIO_SetUsart1Remap(2U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| CK | PD7 | `USART1_RM10_CK_PORT` | `USART1_RM10_CK_PIN` |
| TX | PD6 | `USART1_RM10_TX_PORT` | `USART1_RM10_TX_PIN` |
| RX | PD5 | `USART1_RM10_RX_PORT` | `USART1_RM10_RX_PIN` |
| CTS | PC6 | `USART1_RM10_CTS_PORT` | `USART1_RM10_CTS_PIN` |
| RTS | PC7 | `USART1_RM10_RTS_PORT` | `USART1_RM10_RTS_PIN` |

```c
/* USART1 full-duplex, remap 2 — TX=PD6, RX=PD5 */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
AFIO_SetUsart1Remap(2U);

GPIO_SetPinMode(USART1_RM10_TX_PORT, USART1_RM10_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM10_RX_PORT, USART1_RM10_RX_PIN, GPIO_MODE_IN_FLOATING);
```

#### Remap 3 — Full mapping

`AFIO_SetUsart1Remap(3U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| CK | PC5 | `USART1_RM11_CK_PORT` | `USART1_RM11_CK_PIN` |
| TX | PC0 | `USART1_RM11_TX_PORT` | `USART1_RM11_TX_PIN` |
| RX | PC1 | `USART1_RM11_RX_PORT` | `USART1_RM11_RX_PIN` |
| CTS | PC6 | `USART1_RM11_CTS_PORT` | `USART1_RM11_CTS_PIN` |
| RTS | PC7 | `USART1_RM11_RTS_PORT` | `USART1_RM11_RTS_PIN` |

```c
/* USART1 full-duplex, remap 3 — TX=PC0, RX=PC1 */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);
AFIO_SetUsart1Remap(3U);

GPIO_SetPinMode(USART1_RM11_TX_PORT, USART1_RM11_TX_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(USART1_RM11_RX_PORT, USART1_RM11_RX_PIN, GPIO_MODE_IN_FLOATING);
```

---

### SPI1

**GPIO configuration required (from manual Table 7-4):**

| SPI pin | Mode | Required GPIO mode |
|---|---|---|
| SCK master | Any | `GPIO_MODE_AF_PP_50` |
| SCK slave | Any | `GPIO_MODE_IN_FLOATING` |
| MOSI master full-duplex | Any | `GPIO_MODE_AF_PP_50` |
| MOSI slave full-duplex | Any | `GPIO_MODE_IN_FLOATING` or `GPIO_MODE_IN_PULLUP` |
| MISO master full-duplex | Any | `GPIO_MODE_IN_FLOATING` or `GPIO_MODE_IN_PULLUP` |
| MISO slave full-duplex | Any | `GPIO_MODE_AF_PP_50` |
| NSS hardware master | Any | `GPIO_MODE_IN_FLOATING` or `GPIO_MODE_IN_PULLUP` |
| NSS hardware output | Any | `GPIO_MODE_AF_PP_50` |
| NSS software | Any | Not used as AF — configure as GPIO if needed |

#### Remap 0 — Default mapping

`AFIO_SetSpi1Remap(0U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| NSS | PC1 | `SPI1_RM0_NSS_PORT` | `SPI1_RM0_NSS_PIN` |
| SCK | PC5 | `SPI1_RM0_SCK_PORT` | `SPI1_RM0_SCK_PIN` |
| MISO | PC7 | `SPI1_RM0_MISO_PORT` | `SPI1_RM0_MISO_PIN` |
| MOSI | PC6 | `SPI1_RM0_MOSI_PORT` | `SPI1_RM0_MOSI_PIN` |

```c
/* SPI1 master, full-duplex, software NSS, default pins */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);

GPIO_SetPinMode(SPI1_RM0_SCK_PORT,  SPI1_RM0_SCK_PIN,  GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM0_MOSI_PORT, SPI1_RM0_MOSI_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM0_MISO_PORT, SPI1_RM0_MISO_PIN, GPIO_MODE_IN_FLOATING);

/* Software NSS — use a regular GPIO pin for chip select */
GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_1, GPIO_MODE_OUT_PP_50);
GPIO_SetPin(GPIO_PORT_C, GPIO_PIN_1);   /* CS idle high */
```

#### Remap 1 — NSS moves to PC0

`AFIO_SetSpi1Remap(1U)` — only NSS changes, SCK/MISO/MOSI stay the same.

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| NSS | PC0 | `SPI1_RM1_NSS_PORT` | `SPI1_RM1_NSS_PIN` |
| SCK | PC5 | `SPI1_RM1_SCK_PORT` | `SPI1_RM1_SCK_PIN` |
| MISO | PC7 | `SPI1_RM1_MISO_PORT` | `SPI1_RM1_MISO_PIN` |
| MOSI | PC6 | `SPI1_RM1_MOSI_PORT` | `SPI1_RM1_MOSI_PIN` |

---

### I2C1

**GPIO configuration required (from manual Table 7-5):**

| I2C pin | Required GPIO mode | Note |
|---|---|---|
| SCL | `GPIO_MODE_AF_OD_50` | External pull-up resistor required on board |
| SDA | `GPIO_MODE_AF_OD_50` | External pull-up resistor required on board |

> I2C lines **must** be open-drain. The I2C standard requires external
> pull-up resistors (typically 4.7kΩ to VCC for 100kHz, 2.2kΩ for 400kHz).
> Do not rely on internal pull-ups — they are too weak for reliable I2C.

#### Remap 0 — Default mapping

`AFIO_SetI2c1Remap(0U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| SCL | PC2 | `I2C1_RM00_SCL_PORT` | `I2C1_RM00_SCL_PIN` |
| SDA | PC1 | `I2C1_RM00_SDA_PORT` | `I2C1_RM00_SDA_PIN` |

```c
/* I2C1 default pins — PC2=SCL, PC1=SDA */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);

GPIO_SetPinMode(I2C1_RM00_SCL_PORT, I2C1_RM00_SCL_PIN, GPIO_MODE_AF_OD_50);
GPIO_SetPinMode(I2C1_RM00_SDA_PORT, I2C1_RM00_SDA_PIN, GPIO_MODE_AF_OD_50);
```

#### Remap 1 — PD1/PD0

`AFIO_SetI2c1Remap(1U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| SCL | PD1 | `I2C1_RM01_SCL_PORT` | `I2C1_RM01_SCL_PIN` |
| SDA | PD0 | `I2C1_RM01_SDA_PORT` | `I2C1_RM01_SDA_PIN` |

```c
/* I2C1 remap 1 — PD1=SCL, PD0=SDA */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
AFIO_SetI2c1Remap(1U);

GPIO_SetPinMode(I2C1_RM01_SCL_PORT, I2C1_RM01_SCL_PIN, GPIO_MODE_AF_OD_50);
GPIO_SetPinMode(I2C1_RM01_SDA_PORT, I2C1_RM01_SDA_PIN, GPIO_MODE_AF_OD_50);
```

#### Remap 2 — PC5/PC6

`AFIO_SetI2c1Remap(2U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| SCL | PC5 | `I2C1_RM1X_SCL_PORT` | `I2C1_RM1X_SCL_PIN` |
| SDA | PC6 | `I2C1_RM1X_SDA_PORT` | `I2C1_RM1X_SDA_PIN` |

---

### TIM1

**GPIO configuration required (from manual Table 7-1):**

| TIM1 pin | Configuration | Required GPIO mode |
|---|---|---|
| CHx | Input capture | `GPIO_MODE_IN_FLOATING` |
| CHx | Output compare | `GPIO_MODE_AF_PP_50` |
| CHxN | Complementary output | `GPIO_MODE_AF_PP_50` |
| BKIN | Brake input | `GPIO_MODE_IN_FLOATING` |
| ETR | External trigger clock | `GPIO_MODE_IN_FLOATING` |

#### Remap 0 — Default mapping

`AFIO_SetTim1Remap(0U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR | PC5 | `TIM1_RM00_ETR_PORT` | `TIM1_RM00_ETR_PIN` |
| CH1 | PD2 | `TIM1_RM00_CH1_PORT` | `TIM1_RM00_CH1_PIN` |
| CH2 | PA1 | `TIM1_RM00_CH2_PORT` | `TIM1_RM00_CH2_PIN` |
| CH3 | PC3 | `TIM1_RM00_CH3_PORT` | `TIM1_RM00_CH3_PIN` |
| CH4 | PC4 | `TIM1_RM00_CH4_PORT` | `TIM1_RM00_CH4_PIN` |
| BKIN | PC2 | `TIM1_RM00_BKIN_PORT` | `TIM1_RM00_BKIN_PIN` |
| CH1N | PD0 | `TIM1_RM00_CH1N_PORT` | `TIM1_RM00_CH1N_PIN` |
| CH2N | PA2 | `TIM1_RM00_CH2N_PORT` | `TIM1_RM00_CH2N_PIN` |
| CH3N | PD1 | `TIM1_RM00_CH3N_PORT` | `TIM1_RM00_CH3N_PIN` |

```c
/* TIM1 CH1 PWM output on PD2, default remap */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);

GPIO_SetPinMode(TIM1_RM00_CH1_PORT, TIM1_RM00_CH1_PIN, GPIO_MODE_AF_PP_50);
```

#### Remap 1 — Partial mapping

`AFIO_SetTim1Remap(1U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR | PC5 | `TIM1_RM01_ETR_PORT` | `TIM1_RM01_ETR_PIN` |
| CH1 | PC6 | `TIM1_RM01_CH1_PORT` | `TIM1_RM01_CH1_PIN` |
| CH2 | PC7 | `TIM1_RM01_CH2_PORT` | `TIM1_RM01_CH2_PIN` |
| CH3 | PC0 | `TIM1_RM01_CH3_PORT` | `TIM1_RM01_CH3_PIN` |
| CH4 | PD3 | `TIM1_RM01_CH4_PORT` | `TIM1_RM01_CH4_PIN` |
| BKIN | PC1 | `TIM1_RM01_BKIN_PORT` | `TIM1_RM01_BKIN_PIN` |
| CH1N | PC3 | `TIM1_RM01_CH1N_PORT` | `TIM1_RM01_CH1N_PIN` |
| CH2N | PC4 | `TIM1_RM01_CH2N_PORT` | `TIM1_RM01_CH2N_PIN` |
| CH3N | PD1 | `TIM1_RM01_CH3N_PORT` | `TIM1_RM01_CH3N_PIN` |

#### Remap 2 — Partial mapping

`AFIO_SetTim1Remap(2U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR | PD4 | `TIM1_RM10_ETR_PORT` | `TIM1_RM10_ETR_PIN` |
| CH1 | PD2 | `TIM1_RM10_CH1_PORT` | `TIM1_RM10_CH1_PIN` |
| CH2 | PA1 | `TIM1_RM10_CH2_PORT` | `TIM1_RM10_CH2_PIN` |
| CH3 | PC3 | `TIM1_RM10_CH3_PORT` | `TIM1_RM10_CH3_PIN` |
| CH4 | PC4 | `TIM1_RM10_CH4_PORT` | `TIM1_RM10_CH4_PIN` |
| BKIN | PC2 | `TIM1_RM10_BKIN_PORT` | `TIM1_RM10_BKIN_PIN` |
| CH1N | PD0 | `TIM1_RM10_CH1N_PORT` | `TIM1_RM10_CH1N_PIN` |
| CH2N | PA2 | `TIM1_RM10_CH2N_PORT` | `TIM1_RM10_CH2N_PIN` |
| CH3N | PD1 | `TIM1_RM10_CH3N_PORT` | `TIM1_RM10_CH3N_PIN` |

#### Remap 3 — Full mapping

`AFIO_SetTim1Remap(3U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR | PC2 | `TIM1_RM11_ETR_PORT` | `TIM1_RM11_ETR_PIN` |
| CH1 | PC4 | `TIM1_RM11_CH1_PORT` | `TIM1_RM11_CH1_PIN` |
| CH2 | PC7 | `TIM1_RM11_CH2_PORT` | `TIM1_RM11_CH2_PIN` |
| CH3 | PC5 | `TIM1_RM11_CH3_PORT` | `TIM1_RM11_CH3_PIN` |
| CH4 | PD4 | `TIM1_RM11_CH4_PORT` | `TIM1_RM11_CH4_PIN` |
| BKIN | PC1 | `TIM1_RM11_BKIN_PORT` | `TIM1_RM11_BKIN_PIN` |
| CH1N | PC3 | `TIM1_RM11_CH1N_PORT` | `TIM1_RM11_CH1N_PIN` |
| CH2N | PD2 | `TIM1_RM11_CH2N_PORT` | `TIM1_RM11_CH2N_PIN` |
| CH3N | PC6 | `TIM1_RM11_CH3N_PORT` | `TIM1_RM11_CH3N_PIN` |

---

### TIM2

**GPIO configuration required (from manual Table 7-2):**

| TIM2 pin | Configuration | Required GPIO mode |
|---|---|---|
| CHx | Input capture | `GPIO_MODE_IN_FLOATING` |
| CHx | Output compare | `GPIO_MODE_AF_PP_50` |
| ETR | External trigger | `GPIO_MODE_IN_FLOATING` |

#### Remap 0 — Default mapping

`AFIO_SetTim2Remap(0U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR/CH1 | PD4 | `TIM2_RM00_ETR_PORT` | `TIM2_RM00_ETR_PIN` |
| CH2 | PD3 | `TIM2_RM00_CH2_PORT` | `TIM2_RM00_CH2_PIN` |
| CH3 | PC0 | `TIM2_RM00_CH3_PORT` | `TIM2_RM00_CH3_PIN` |
| CH4 | PD7 | `TIM2_RM00_CH4_PORT` | `TIM2_RM00_CH4_PIN` |

> Note: TIM2 ETR and CH1 share the same pin in all remap configurations.

```c
/* TIM2 CH2 PWM output on PD3, default remap */
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);

GPIO_SetPinMode(TIM2_RM00_CH2_PORT, TIM2_RM00_CH2_PIN, GPIO_MODE_AF_PP_50);
```

#### Remap 1 — Partial mapping

`AFIO_SetTim2Remap(1U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR/CH1 | PC5 | `TIM2_RM01_ETR_PORT` | `TIM2_RM01_ETR_PIN` |
| CH2 | PC2 | `TIM2_RM01_CH2_PORT` | `TIM2_RM01_CH2_PIN` |
| CH3 | PD2 | `TIM2_RM01_CH3_PORT` | `TIM2_RM01_CH3_PIN` |
| CH4 | PC1 | `TIM2_RM01_CH4_PORT` | `TIM2_RM01_CH4_PIN` |

#### Remap 2 — Partial mapping

`AFIO_SetTim2Remap(2U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR/CH1 | PC1 | `TIM2_RM10_ETR_PORT` | `TIM2_RM10_ETR_PIN` |
| CH2 | PD3 | `TIM2_RM10_CH2_PORT` | `TIM2_RM10_CH2_PIN` |
| CH3 | PC0 | `TIM2_RM10_CH3_PORT` | `TIM2_RM10_CH3_PIN` |
| CH4 | PD7 | `TIM2_RM10_CH4_PORT` | `TIM2_RM10_CH4_PIN` |

#### Remap 3 — Full mapping

`AFIO_SetTim2Remap(3U)`

| Function | Port/Pin | Constant (port) | Constant (pin) |
|---|---|---|---|
| ETR/CH1 | PC1 | `TIM2_RM11_ETR_PORT` | `TIM2_RM11_ETR_PIN` |
| CH2 | PC7 | `TIM2_RM11_CH2_PORT` | `TIM2_RM11_CH2_PIN` |
| CH3 | PD6 | `TIM2_RM11_CH3_PORT` | `TIM2_RM11_CH3_PIN` |
| CH4 | PD5 | `TIM2_RM11_CH4_PORT` | `TIM2_RM11_CH4_PIN` |

---

## Complete Examples

### Blink an LED on PD4

```c
#include "rcc.h"
#include "systick.h"
#include "gpio.h"

int main(void)
{
    RCC_Result_t result = RCC_Init_48mhz();
    if (result != RCC_OK) { for (;;) {} }

    RCC_Enable_Clock(RCC_PERIPH_GPIOD);
    SysTick_Init_FreeRunning();

    GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_4, GPIO_MODE_OUT_PP_2);

    for (;;)
    {
        GPIO_TogglePin(GPIO_PORT_D, GPIO_PIN_4);
        SysTick_Delay_Ms(500U);
    }
}
```

---

### Read a button with debounce on PD3

```c
RCC_Enable_Clock(RCC_PERIPH_GPIOD);
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_PULLUP);

uint32_t button_is_pressed(void)
{
    uint32_t pressed = 0U;

    /* Pin reads 0 when button pulls it to GND */
    if (GPIO_ReadPin(GPIO_PORT_D, GPIO_PIN_3) == 0U)
    {
        SysTick_Delay_Ms(20U);   /* debounce */
        if (GPIO_ReadPin(GPIO_PORT_D, GPIO_PIN_3) == 0U)
        {
            pressed = 1U;
        }
    }
    return pressed;
}
```

---

### USART1 TX only — default pins

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);

/* TX=PD5, no remap needed (reset default) */
GPIO_SetPinMode(USART1_RM00_TX_PORT, USART1_RM00_TX_PIN, GPIO_MODE_AF_PP_50);

/* Now initialise USART1 peripheral */
```

---

### I2C1 — default pins with open-drain

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);

/* SCL=PC2, SDA=PC1 — both must be open-drain */
/* Board must have 4.7kΩ pull-ups to VCC on both lines */
GPIO_SetPinMode(I2C1_RM00_SCL_PORT, I2C1_RM00_SCL_PIN, GPIO_MODE_AF_OD_50);
GPIO_SetPinMode(I2C1_RM00_SDA_PORT, I2C1_RM00_SDA_PIN, GPIO_MODE_AF_OD_50);

/* Now initialise I2C1 peripheral */
```

---

### SPI1 master — software NSS

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOC);

/* SCK=PC5, MOSI=PC6, MISO=PC7 */
GPIO_SetPinMode(SPI1_RM0_SCK_PORT,  SPI1_RM0_SCK_PIN,  GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM0_MOSI_PORT, SPI1_RM0_MOSI_PIN, GPIO_MODE_AF_PP_50);
GPIO_SetPinMode(SPI1_RM0_MISO_PORT, SPI1_RM0_MISO_PIN, GPIO_MODE_IN_FLOATING);

/* Software NSS — use PC1 as a regular GPIO chip-select */
GPIO_SetPinMode(GPIO_PORT_C, GPIO_PIN_1, GPIO_MODE_OUT_PP_50);
GPIO_SetPin(GPIO_PORT_C, GPIO_PIN_1);   /* deassert CS (active low) */

/* Now initialise SPI1 peripheral */
```

---

### EXTI3 interrupt on PD3

```c
RCC_Enable_Clock(RCC_PERIPH_AFIO);
RCC_Enable_Clock(RCC_PERIPH_GPIOD);

/* Route EXTI line 3 to port D */
AFIO_SetExtiPort(3U, GPIO_PORT_D);

/* Configure PD3 as floating input */
GPIO_SetPinMode(GPIO_PORT_D, GPIO_PIN_3, GPIO_MODE_IN_FLOATING);

/* Now configure EXTI3 trigger and enable in EXTI peripheral */
```
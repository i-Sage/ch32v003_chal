/// usart.h
///
/// USART1 driver for CH32V003
/// Supports polling TX and interrupt-driven RX with ring buffer.
/// MISRA C:2012 compliant
/// Reference: CH32V003 Reference Manual V1.9, Chapter 12

#ifndef CH32V003_HAL_USART_H
#define CH32V003_HAL_USART_H

#include <stdint.h>
#include "rcc.h"
#include "gpio.h"
#include "core.h"
#include "pfic.h"

/*====================================================================
 * Register map — CH32V003 manual Table 12-2
 * Base address: 0x40013800
 *
 * DATAR is physically two registers behind one address:
 *   Reading  → receives from RDR, clears RXNE automatically
 *   Writing  → transmits via TDR, clears TXE automatically
 *====================================================================*/
typedef struct {
    volatile uint32_t statr;   /* 0x00 USART_STATR  status register         */
    volatile uint32_t datar;   /* 0x04 USART_DATAR  data register (RDR/TDR) */
    volatile uint32_t brr;     /* 0x08 USART_BRR    baud rate register       */
    volatile uint32_t ctlr1;   /* 0x0C USART_CTLR1  control register 1       */
    volatile uint32_t ctlr2;   /* 0x10 USART_CTLR2  control register 2       */
    volatile uint32_t ctlr3;   /* 0x14 USART_CTLR3  control register 3       */
    volatile uint32_t gpr;     /* 0x18 USART_GPR    guard time and prescaler  */
} USART_TypeDef;

/*
 * Deviation: MISRA C:2012 Rule 11.4 (Advisory)
 * Reason:    Integer-to-pointer cast required for MMIO access.
 *            Address 0x40013800 verified against CH32V003 manual
 *            Table 12-2.
 */
static inline volatile USART_TypeDef *USART1_GetBase(void)
{
    return (volatile USART_TypeDef *)0x40013800U;
}

/*====================================================================
 * STATR — Status Register (offset 0x00)
 * CH32V003 manual section 12.10.1
 * Reset value: 0x000000C0 (TXE=1, TC=1 at reset — ready to transmit)
 *====================================================================*/

// Bit 9 CTS — CTS state change flag (RW, write 0 to clear)
//   1: Change detected on nCTS line
//   0: No change
//   Requires CTSE=1 in CTLR3. If CTSIE=1, generates interrupt.
#define USART_STATR_CTS_BIT      (9U)
#define USART_STATR_CTS_MASK     ((uint32_t)(1UL << USART_STATR_CTS_BIT))

// Bit 8 LBD — LIN break detection flag (RW, write 0 to clear)
//   1: LIN break detected
//   0: No LIN break detected
//   If LBDIE=1, generates interrupt when set.
#define USART_STATR_LBD_BIT      (8U)
#define USART_STATR_LBD_MASK     ((uint32_t)(1UL << USART_STATR_LBD_BIT))

// Bit 7 TXE — transmit data register empty (RO)
//   1: Data transferred to shift register, TDR ready for new byte
//   0: Data not yet transferred to shift register
//   Reset value: 1 (ready at reset)
//   If TXEIE=1 in CTLR1, generates interrupt when set.
//   Cleared automatically when you write to DATAR.
#define USART_STATR_TXE_BIT      (7U)
#define USART_STATR_TXE_MASK     ((uint32_t)(1UL << USART_STATR_TXE_BIT))

// Bit 6 TC — transmission complete (RW, write 0 to clear)
//   1: Frame fully shifted out AND TXE=1 (line is completely idle)
//   0: Transmission still in progress
//   Reset value: 1
//   Clear by: reading STATR then writing DATAR, or writing 0 directly.
//   Use before disabling USART or changing baud rate.
//   If TCIE=1, generates interrupt when set.
#define USART_STATR_TC_BIT       (6U)
#define USART_STATR_TC_MASK      ((uint32_t)(1UL << USART_STATR_TC_BIT))

// Bit 5 RXNE — read data register not empty (RW, write 0 to clear)
//   1: Received byte ready to read in DATAR
//   0: No data received
//   Reading DATAR clears this automatically.
//   If RXNEIE=1 in CTLR1, generates interrupt when set.
#define USART_STATR_RXNE_BIT     (5U)
#define USART_STATR_RXNE_MASK    ((uint32_t)(1UL << USART_STATR_RXNE_BIT))

// Bit 4 IDLE — bus idle detected (RO)
//   1: Idle line detected (bus has been idle for one frame length)
//   0: No idle line detected
//   Cleared by reading STATR then DATAR.
//   Not set again until RXNE is set first.
//   If IDLEIE=1, generates interrupt when set.
#define USART_STATR_IDLE_BIT     (4U)
#define USART_STATR_IDLE_MASK    ((uint32_t)(1UL << USART_STATR_IDLE_BIT))

// Bit 3 ORE — overrun error (RO)
//   1: New byte arrived before previous byte was read from DATAR
//   0: No overrun
//   Note: received data in DATAR not lost, but shift register
//         overwritten by new byte.
//   Cleared by reading STATR then DATAR.
//   If RXNEIE or EIE=1, generates interrupt.
#define USART_STATR_ORE_BIT      (3U)
#define USART_STATR_ORE_MASK     ((uint32_t)(1UL << USART_STATR_ORE_BIT))

// Bit 2 NE — noise error (RO)
//   1: Noise detected on received frame
//   0: No noise detected
//   Cleared by reading STATR then DATAR.
//   Does not generate interrupt on its own.
//   If EIE=1 (in CTLR3) and DMAR=1, generates interrupt.
#define USART_STATR_NE_BIT       (2U)
#define USART_STATR_NE_MASK      ((uint32_t)(1UL << USART_STATR_NE_BIT))

// Bit 1 FE — frame error (RO)
//   1: Synchronisation error, excessive noise, or break character detected
//   0: No frame error
//   Cleared by reading STATR then DATAR.
//   Does not generate interrupt on its own.
//   If EIE=1 and DMAR=1, generates interrupt.
#define USART_STATR_FE_BIT       (1U)
#define USART_STATR_FE_MASK      ((uint32_t)(1UL << USART_STATR_FE_BIT))

// Bit 0 PE — parity error (RO)
//   1: Parity check failed (only meaningful when PCE=1 in CTLR1)
//   0: No parity error
//   Cleared by reading STATR then DATAR.
//   Must wait for RXNE=1 before clearing.
//   If PEIE=1 in CTLR1, generates interrupt.
#define USART_STATR_PE_BIT       (0U)
#define USART_STATR_PE_MASK      ((uint32_t)(1UL << USART_STATR_PE_BIT))

// Combined RX error mask — check all three error flags at once
#define USART_STATR_ERR_MASK     (USART_STATR_ORE_MASK | \
                                  USART_STATR_NE_MASK  | \
                                  USART_STATR_FE_MASK)

/*====================================================================
 * DATAR — Data Register (offset 0x04)
 * CH32V003 manual section 12.10.2
 *
 * bits [8:0] DR[8:0] — 9-bit capable for 9-bit word mode
 * For standard 8-bit mode use bits [7:0] only.
 *====================================================================*/
#define USART_DATAR_DR_MASK      ((uint32_t)(0x1FFU))

/*====================================================================
 * BRR — Baud Rate Register (offset 0x08)
 * CH32V003 manual section 12.10.3
 *
 * Formula: BRR = HCLK / Baud  (rounded integer)
 *
 * The register is split as:
 *   bits [15:4] = DIV_Mantissa[11:0]  — integer part
 *   bits  [3:0] = DIV_Fraction[3:0]   — fractional part (sixteenths)
 *
 * Our formula computes the correct combined value without float:
 *   brr = (hclk + baud/2) / baud    (baud/2 gives correct rounding)
 *
 * BRR must be >= 16. Maximum baud = HCLK / 16.
 * Must be written while UE=0.
 *====================================================================*/
#define USART_BRR_FRACTION_BIT   (0U)
#define USART_BRR_FRACTION_MASK  ((uint32_t)(0xFUL))
#define USART_BRR_MANTISSA_BIT   (4U)
#define USART_BRR_MANTISSA_MASK  ((uint32_t)(0xFFFUL << USART_BRR_MANTISSA_BIT))

/*====================================================================
 * CTLR1 — Control Register 1 (offset 0x0C)
 * CH32V003 manual section 12.10.4
 *====================================================================*/

// Bit 13 UE — USART enable
//   1: USART enabled (divider and output active)
//   0: USART disabled — stops after current byte completes
//   All configuration must be done while UE=0.
#define USART_CTLR1_UE_BIT       (13U)
#define USART_CTLR1_UE_MASK      ((uint32_t)(1UL << USART_CTLR1_UE_BIT))

// Bit 12 M — word length
//   0: 8 data bits (standard)
//   1: 9 data bits
#define USART_CTLR1_M_BIT        (12U)
#define USART_CTLR1_M_MASK       ((uint32_t)(1UL << USART_CTLR1_M_BIT))

// Bit 11 WAKE — wakeup method
//   0: Idle line wakeup from mute mode
//   1: Address mark wakeup from mute mode
#define USART_CTLR1_WAKE_BIT     (11U)
#define USART_CTLR1_WAKE_MASK    ((uint32_t)(1UL << USART_CTLR1_WAKE_BIT))

// Bit 10 PCE — parity control enable
//   1: Parity generation (TX) and checking (RX) enabled
//   0: No parity
//   Takes effect after current byte completes.
#define USART_CTLR1_PCE_BIT      (10U)
#define USART_CTLR1_PCE_MASK     ((uint32_t)(1UL << USART_CTLR1_PCE_BIT))

// Bit 9 PS — parity selection (only valid when PCE=1)
//   0: Even parity
//   1: Odd parity
//   Takes effect after current byte completes.
#define USART_CTLR1_PS_BIT       (9U)
#define USART_CTLR1_PS_MASK      ((uint32_t)(1UL << USART_CTLR1_PS_BIT))

// Bit 8 PEIE — parity error interrupt enable
//   1: Interrupt generated when PE flag is set
//   0: No parity interrupt
#define USART_CTLR1_PEIE_BIT     (8U)
#define USART_CTLR1_PEIE_MASK    ((uint32_t)(1UL << USART_CTLR1_PEIE_BIT))

// Bit 7 TXEIE — TXE interrupt enable
//   1: Interrupt generated when TXE flag is set (TDR empty, ready to write)
//   0: No TXE interrupt
#define USART_CTLR1_TXEIE_BIT    (7U)
#define USART_CTLR1_TXEIE_MASK   ((uint32_t)(1UL << USART_CTLR1_TXEIE_BIT))

// Bit 6 TCIE — transmission complete interrupt enable
//   1: Interrupt generated when TC flag is set (frame fully shifted out)
//   0: No TC interrupt
#define USART_CTLR1_TCIE_BIT     (6U)
#define USART_CTLR1_TCIE_MASK    ((uint32_t)(1UL << USART_CTLR1_TCIE_BIT))

// Bit 5 RXNEIE — RXNE interrupt enable
//   1: Interrupt generated when RXNE=1 (received byte ready to read)
//   0: No RXNE interrupt
//   This is the bit that enables interrupt-driven receive.
#define USART_CTLR1_RXNEIE_BIT   (5U)
#define USART_CTLR1_RXNEIE_MASK  ((uint32_t)(1UL << USART_CTLR1_RXNEIE_BIT))

// Bit 4 IDLEIE — IDLE interrupt enable
//   1: Interrupt generated when IDLE flag is set (bus went idle)
//   0: No idle interrupt
#define USART_CTLR1_IDLEIE_BIT   (4U)
#define USART_CTLR1_IDLEIE_MASK  ((uint32_t)(1UL << USART_CTLR1_IDLEIE_BIT))

// Bit 3 TE — transmitter enable
//   1: Transmitter enabled — sends an idle frame first
//   0: Transmitter disabled
#define USART_CTLR1_TE_BIT       (3U)
#define USART_CTLR1_TE_MASK      ((uint32_t)(1UL << USART_CTLR1_TE_BIT))

// Bit 2 RE — receiver enable
//   1: Receiver enabled — begins searching for start bit on RX pin
//   0: Receiver disabled
#define USART_CTLR1_RE_BIT       (2U)
#define USART_CTLR1_RE_MASK      ((uint32_t)(1UL << USART_CTLR1_RE_BIT))

// Bit 1 RWU — receiver wakeup / mute mode
//   0: Receiver in normal operating mode
//   1: Receiver in silent/mute mode
//   Note: must receive a byte first before setting in idle-line mode.
//   Note: cannot be modified by software when RXNE=1 in address-mark mode.
#define USART_CTLR1_RWU_BIT      (1U)
#define USART_CTLR1_RWU_MASK     ((uint32_t)(1UL << USART_CTLR1_RWU_BIT))

// Bit 0 SBK — send break character
//   1: Send break character (reset by hardware on stop bit of break frame)
//   0: Do not send break
#define USART_CTLR1_SBK_BIT      (0U)
#define USART_CTLR1_SBK_MASK     ((uint32_t)(1UL << USART_CTLR1_SBK_BIT))

/*====================================================================
 * CTLR2 — Control Register 2 (offset 0x10)
 * CH32V003 manual section 12.10.5
 *====================================================================*/

// Bit 14 LINEN — LIN mode enable
//   1: LIN mode (allows LIN sync breaks via SBK, detects LIN breaks)
//   0: LIN mode disabled
#define USART_CTLR2_LINEN_BIT    (14U)
#define USART_CTLR2_LINEN_MASK   ((uint32_t)(1UL << USART_CTLR2_LINEN_BIT))

// Bits [13:12] STOP[1:0] — stop bit count
//   00: 1 stop bit   (standard async — use this)
//   01: 0.5 stop bit (smartcard mode only)
//   10: 2 stop bits  (slower devices, noisy lines)
//   11: 1.5 stop bits (smartcard mode only)
#define USART_CTLR2_STOP_BIT     (12U)
#define USART_CTLR2_STOP_MASK    ((uint32_t)(0x3UL << USART_CTLR2_STOP_BIT))
#define USART_CTLR2_STOP_1       ((uint32_t)(0x0UL << USART_CTLR2_STOP_BIT))
#define USART_CTLR2_STOP_0_5     ((uint32_t)(0x1UL << USART_CTLR2_STOP_BIT))
#define USART_CTLR2_STOP_2       ((uint32_t)(0x2UL << USART_CTLR2_STOP_BIT))
#define USART_CTLR2_STOP_1_5     ((uint32_t)(0x3UL << USART_CTLR2_STOP_BIT))

// Bit 11 CLKEN — synchronous CK pin enable
//   1: CK pin output enabled (synchronous mode)
//   0: CK pin disabled (standard async — leave at 0)
//   Note: CLKEN, CPOL, CPHA cannot be changed after enabling transmit.
#define USART_CTLR2_CLKEN_BIT    (11U)
#define USART_CTLR2_CLKEN_MASK   ((uint32_t)(1UL << USART_CTLR2_CLKEN_BIT))

// Bit 10 CPOL — clock polarity (synchronous mode only)
//   0: CK steady low outside transmission window
//   1: CK steady high outside transmission window
//   Note: cannot be changed after enabling transmit.
#define USART_CTLR2_CPOL_BIT     (10U)
#define USART_CTLR2_CPOL_MASK    ((uint32_t)(1UL << USART_CTLR2_CPOL_BIT))

// Bit 9 CPHA — clock phase (synchronous mode only)
//   0: First clock transition is first data capture edge
//   1: Second clock transition is first data capture edge
//   Note: cannot be changed after enabling transmit.
#define USART_CTLR2_CPHA_BIT     (9U)
#define USART_CTLR2_CPHA_MASK    ((uint32_t)(1UL << USART_CTLR2_CPHA_BIT))

// Bit 8 LBCL — last bit clock pulse (synchronous mode only)
//   0: CK pulse for last data bit NOT output on CK pin
//   1: CK pulse for last data bit (MSB) output on CK pin
//   Note: cannot be changed after enabling transmit.
#define USART_CTLR2_LBCL_BIT     (8U)
#define USART_CTLR2_LBCL_MASK    ((uint32_t)(1UL << USART_CTLR2_LBCL_BIT))

// Bit 7 — Reserved, do not modify

// Bit 6 LBDIE — LIN break detection interrupt enable
//   1: Interrupt generated when LBD flag is set
//   0: No LIN break interrupt
#define USART_CTLR2_LBDIE_BIT    (6U)
#define USART_CTLR2_LBDIE_MASK   ((uint32_t)(1UL << USART_CTLR2_LBDIE_BIT))

// Bit 5 LBDL — LIN break detection length
//   0: 10-bit break character detection
//   1: 11-bit break character detection
#define USART_CTLR2_LBDL_BIT     (5U)
#define USART_CTLR2_LBDL_MASK    ((uint32_t)(1UL << USART_CTLR2_LBDL_BIT))

// Bit 4 — Reserved, do not modify

// Bits [3:0] ADD[3:0] — USART node address
//   Address of this USART in multiprocessor communication.
//   Used in mute mode with address mark wakeup.
#define USART_CTLR2_ADD_BIT      (0U)
#define USART_CTLR2_ADD_MASK     ((uint32_t)(0xFUL << USART_CTLR2_ADD_BIT))

/*====================================================================
 * CTLR3 — Control Register 3 (offset 0x14)
 * CH32V003 manual section 12.10.6
 *====================================================================*/

// Bit 10 CTSIE — CTS interrupt enable
//   1: Interrupt generated when CTS flag is set
//   0: No CTS interrupt
#define USART_CTLR3_CTSIE_BIT    (10U)
#define USART_CTLR3_CTSIE_MASK   ((uint32_t)(1UL << USART_CTLR3_CTSIE_BIT))

// Bit 9 CTSE — CTS hardware flow control enable
//   1: CTS flow control enabled — transmission pauses when nCTS is high
//   0: CTS disabled
#define USART_CTLR3_CTSE_BIT     (9U)
#define USART_CTLR3_CTSE_MASK    ((uint32_t)(1UL << USART_CTLR3_CTSE_BIT))

// Bit 8 RTSE — RTS hardware flow control enable
//   1: RTS flow control enabled — nRTS driven by hardware
//   0: RTS disabled
#define USART_CTLR3_RTSE_BIT     (8U)
#define USART_CTLR3_RTSE_MASK    ((uint32_t)(1UL << USART_CTLR3_RTSE_BIT))

// Bit 7 DMAT — DMA transmit enable
//   1: DMA request generated when TXE=1
//   0: DMA not used for transmission
#define USART_CTLR3_DMAT_BIT     (7U)
#define USART_CTLR3_DMAT_MASK    ((uint32_t)(1UL << USART_CTLR3_DMAT_BIT))

// Bit 6 DMAR — DMA receive enable
//   1: DMA request generated when RXNE=1
//   0: DMA not used for reception
#define USART_CTLR3_DMAR_BIT     (6U)
#define USART_CTLR3_DMAR_MASK    ((uint32_t)(1UL << USART_CTLR3_DMAR_BIT))

// Bit 5 SCEN — smartcard mode enable
//   1: Smartcard mode enabled
//   0: Smartcard mode disabled
#define USART_CTLR3_SCEN_BIT     (5U)
#define USART_CTLR3_SCEN_MASK    ((uint32_t)(1UL << USART_CTLR3_SCEN_BIT))

// Bit 4 NACK — smartcard NACK enable
//   1: NACK sent on parity error in smartcard receive mode
//   0: No NACK
#define USART_CTLR3_NACK_BIT     (4U)
#define USART_CTLR3_NACK_MASK    ((uint32_t)(1UL << USART_CTLR3_NACK_BIT))

// Bit 3 HDSEL — half-duplex selection
//   1: Half-duplex mode — TX pin used for both TX and RX
//   0: Full-duplex mode (standard)
#define USART_CTLR3_HDSEL_BIT    (3U)
#define USART_CTLR3_HDSEL_MASK   ((uint32_t)(1UL << USART_CTLR3_HDSEL_BIT))

// Bit 2 IRLP — IrDA low-power mode
//   1: IrDA low-power mode
//   0: Normal IrDA mode
#define USART_CTLR3_IRLP_BIT     (2U)
#define USART_CTLR3_IRLP_MASK    ((uint32_t)(1UL << USART_CTLR3_IRLP_BIT))

// Bit 1 IREN — IrDA mode enable
//   1: IrDA mode enabled
//   0: IrDA disabled
#define USART_CTLR3_IREN_BIT     (1U)
#define USART_CTLR3_IREN_MASK    ((uint32_t)(1UL << USART_CTLR3_IREN_BIT))

// Bit 0 EIE — error interrupt enable (only active when DMAR=1)
//   1: Interrupt generated when FE, ORE, or NE is set
//   0: No error interrupt
#define USART_CTLR3_EIE_BIT      (0U)
#define USART_CTLR3_EIE_MASK     ((uint32_t)(1UL << USART_CTLR3_EIE_BIT))

/*====================================================================
 * GPR — Guard Time and Prescaler Register (offset 0x18)
 * CH32V003 manual section 12.10.7
 *
 * Only relevant for smartcard mode and IrDA low-power mode.
 * In standard async UART operation this register is not used.
 *====================================================================*/

// Bits [15:8] GT[7:0] — guard time value (smartcard mode)
//   Number of baud clocks inserted between frames in smartcard mode.
//   TC flag is set after the guard time has elapsed.
#define USART_GPR_GT_BIT         (8U)
#define USART_GPR_GT_MASK        ((uint32_t)(0xFFUL << USART_GPR_GT_BIT))

// Bits [7:0] PSC[7:0] — prescaler value
//   IrDA low-power: source clock divided by PSC (0 = clock retained)
//   Normal IrDA:    must be set to 1
//   Smartcard:      source clock divided by 2×PSC (lower 5 bits valid)
#define USART_GPR_PSC_BIT        (0U)
#define USART_GPR_PSC_MASK       ((uint32_t)(0xFFUL << USART_GPR_PSC_BIT))

/*====================================================================
 * Configuration types
 *====================================================================*/

typedef enum {
    USART_WORD_8BIT = 0U,   /* 8 data bits (standard)                    */
    USART_WORD_9BIT = 1U    /* 9 data bits                               */
} USART_WordLen_t;

typedef enum {
    USART_PARITY_NONE = 0U,   /* no parity bit                           */
    USART_PARITY_EVEN = 1U,   /* even parity                             */
    USART_PARITY_ODD  = 2U    /* odd parity                              */
} USART_Parity_t;

typedef enum {
    USART_STOP_1   = 0U,   /* 1   stop bit (standard)                   */
    USART_STOP_2   = 1U,   /* 2   stop bits                             */
    USART_STOP_0_5 = 2U,   /* 0.5 stop bits (smartcard only)            */
    USART_STOP_1_5 = 3U    /* 1.5 stop bits (smartcard only)            */
} USART_StopBits_t;

typedef enum {
    USART_FLOW_NONE = 0U,   /* no hardware flow control                  */
    USART_FLOW_RTS  = 1U,   /* RTS output only                           */
    USART_FLOW_CTS  = 2U,   /* CTS input only                            */
    USART_FLOW_BOTH = 3U    /* RTS and CTS                               */
} USART_FlowCtrl_t;

typedef enum {
    USART_MODE_TX_ONLY = 0U,   /* transmitter only                       */
    USART_MODE_RX_ONLY = 1U,   /* receiver only                          */
    USART_MODE_TX_RX   = 2U    /* full duplex — TX and RX both enabled   */
} USART_Mode_t;

typedef struct {
    uint32_t          baud;       /* baud rate in Hz  e.g. 115200U        */
    USART_WordLen_t   word_len;   /* USART_WORD_8BIT or _9BIT             */
    USART_Parity_t    parity;     /* USART_PARITY_NONE / _EVEN / _ODD    */
    USART_StopBits_t  stop_bits;  /* USART_STOP_1 / _2 / _0_5 / _1_5    */
    USART_FlowCtrl_t  flow_ctrl;  /* USART_FLOW_NONE / _RTS / _CTS       */
    USART_Mode_t      mode;       /* USART_MODE_TX_ONLY / _RX / _TX_RX   */
} USART_InitTypeDef;

/*====================================================================
 * Return codes
 *====================================================================*/
typedef enum {
    USART_OK        = 0U,   /* success                                   */
    USART_ERR_BAUD  = 1U,   /* HCLK unknown or BRR < 16 (baud too high)  */
    USART_ERR_BUSY  = 2U,   /* timeout waiting for TXE, TC, or RXNE      */
    USART_ERR_RX    = 3U,   /* receive error — FE, NE, or ORE detected   */
    USART_ERR_PARAM = 4U,   /* NULL pointer or invalid parameter          */
    USART_ERR_EMPTY = 5U    /* ring buffer empty — no byte available      */
} USART_Result_t;

/* Timeout iteration count for polling functions */
#define USART_TIMEOUT        (100000U)

/* Maximum string length for USART1_SendString — safety guard */
#define USART_MAX_STRING_LEN (1024U)

/*====================================================================
 * Pre-built configurations
 *====================================================================*/

// 115200 8N1 — standard debug and host communication
static const USART_InitTypeDef USART_Config_115200_8N1 = {
    115200U,
    USART_WORD_8BIT,
    USART_PARITY_NONE,
    USART_STOP_1,
    USART_FLOW_NONE,
    USART_MODE_TX_RX
};

// 9600 8N1 — GPS modules, GSM modems, older sensors
static const USART_InitTypeDef USART_Config_9600_8N1 = {
    9600U,
    USART_WORD_8BIT,
    USART_PARITY_NONE,
    USART_STOP_1,
    USART_FLOW_NONE,
    USART_MODE_TX_RX
};

/*====================================================================
 * Ring buffer for interrupt-driven RX
 *
 * Size must be a power of 2 — wrap uses bitmask (&) instead of
 * modulo (%) which is faster on hardware without a divider.
 *
 * 64 bytes is enough for 115200 baud assuming the main loop runs
 * at least every few milliseconds. Increase if you receive long
 * bursts faster than you consume them.
 *
 * The ISR writes via rx_head. Your code reads via rx_tail.
 * Buffer full:  (head + 1) & MASK == tail  (1 slot always wasted)
 * Buffer empty: head == tail
 *====================================================================*/
#define USART1_RX_BUF_SIZE   (64U)
#define USART1_RX_BUF_MASK   (USART1_RX_BUF_SIZE - 1U)

// volatile — ISR modifies these asynchronously, compiler must not
// cache them in registers between accesses.
static volatile uint8_t  g_usart1_rx_buf[USART1_RX_BUF_SIZE];
static volatile uint8_t  g_usart1_rx_head    = 0U;
static volatile uint8_t  g_usart1_rx_tail    = 0U;
static volatile uint32_t g_usart1_rx_overrun = 0U;

/*====================================================================
 * Initialisation
 *
 * Register write order required by hardware:
 *   1. Disable USART (UE=0)
 *   2. Write BRR    (baud rate — must be done while UE=0)
 *   3. Write CTLR2  (stop bits — must be done while UE=0)
 *   4. Write CTLR3  (flow control — must be done while UE=0)
 *   5. Write CTLR1  (word length, parity, TE, RE)
 *   6. Enable USART (UE=1 — last step)
 *====================================================================*/
static USART_Result_t USART1_Init(const USART_InitTypeDef * const p_config)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t       hclk      = 0U;
    uint32_t       brr_val   = 0U;
    uint32_t       ctlr1_val = 0U;
    uint32_t       ctlr2_val = 0U;
    uint32_t       ctlr3_val = 0U;
    USART_Result_t result    = USART_OK;

    if (p_config->baud == 0U)
    {
        result = USART_ERR_PARAM;
    }

    if (result == USART_OK)
    {
        hclk = RCC_Get_Hclk_Hz();
        if (hclk == RCC_FREQ_UNKNOWN) { result = USART_ERR_BAUD; }
    }

    if (result == USART_OK)
    {
        /* Step 1 — disable USART before reconfiguring */
        p_usart->ctlr1 = (uint32_t)(p_usart->ctlr1 & ~USART_CTLR1_UE_MASK);

        /* Step 2 — BRR: rounded integer division, no floating point
         * Adding baud/2 before dividing gives correct rounding.
         * BRR must be >= 16 (USARTDIV >= 1.0) or baud rate is invalid. */
        brr_val = (hclk + (p_config->baud / 2U)) / p_config->baud;

        if (brr_val < 16U)
        {
            result = USART_ERR_BAUD;
        }
    }

    if (result == USART_OK)
    {
        p_usart->brr = brr_val;

        /* Step 3 — CTLR2: stop bits */
        ctlr2_val = (uint32_t)(p_usart->ctlr2 & ~USART_CTLR2_STOP_MASK);
        switch (p_config->stop_bits)
        {
            case USART_STOP_1:   ctlr2_val = (uint32_t)(ctlr2_val | USART_CTLR2_STOP_1);   break;
            case USART_STOP_2:   ctlr2_val = (uint32_t)(ctlr2_val | USART_CTLR2_STOP_2);   break;
            case USART_STOP_0_5: ctlr2_val = (uint32_t)(ctlr2_val | USART_CTLR2_STOP_0_5); break;
            case USART_STOP_1_5: ctlr2_val = (uint32_t)(ctlr2_val | USART_CTLR2_STOP_1_5); break;
            default:             ctlr2_val = (uint32_t)(ctlr2_val | USART_CTLR2_STOP_1);   break;
        }
        p_usart->ctlr2 = ctlr2_val;

        /* Step 4 — CTLR3: hardware flow control */
        ctlr3_val = (uint32_t)(p_usart->ctlr3
                     & ~USART_CTLR3_CTSE_MASK
                     & ~USART_CTLR3_RTSE_MASK);

        if ((p_config->flow_ctrl == USART_FLOW_RTS) ||
            (p_config->flow_ctrl == USART_FLOW_BOTH))
        {
            ctlr3_val = (uint32_t)(ctlr3_val | USART_CTLR3_RTSE_MASK);
        }
        if ((p_config->flow_ctrl == USART_FLOW_CTS) ||
            (p_config->flow_ctrl == USART_FLOW_BOTH))
        {
            ctlr3_val = (uint32_t)(ctlr3_val | USART_CTLR3_CTSE_MASK);
        }
        p_usart->ctlr3 = ctlr3_val;

        /* Step 5 — CTLR1: word length, parity, TX/RX enable */
        ctlr1_val = 0U;

        if (p_config->word_len == USART_WORD_9BIT)
        {
            ctlr1_val = (uint32_t)(ctlr1_val | USART_CTLR1_M_MASK);
        }

        if (p_config->parity == USART_PARITY_EVEN)
        {
            ctlr1_val = (uint32_t)(ctlr1_val | USART_CTLR1_PCE_MASK);
        }
        else if (p_config->parity == USART_PARITY_ODD)
        {
            ctlr1_val = (uint32_t)(ctlr1_val | USART_CTLR1_PCE_MASK
                                              | USART_CTLR1_PS_MASK);
        }
        else { /* PARITY_NONE — PCE stays 0 */ }

        if ((p_config->mode == USART_MODE_TX_ONLY) ||
            (p_config->mode == USART_MODE_TX_RX))
        {
            ctlr1_val = (uint32_t)(ctlr1_val | USART_CTLR1_TE_MASK);
        }
        if ((p_config->mode == USART_MODE_RX_ONLY) ||
            (p_config->mode == USART_MODE_TX_RX))
        {
            ctlr1_val = (uint32_t)(ctlr1_val | USART_CTLR1_RE_MASK);
        }

        /* Step 6 — enable USART last */
        ctlr1_val      = (uint32_t)(ctlr1_val | USART_CTLR1_UE_MASK);
        p_usart->ctlr1 = ctlr1_val;
    }

    return result;
}

static USART_Result_t USART1_Init_115200(void)
{
    return USART1_Init(&USART_Config_115200_8N1);
}

static USART_Result_t USART1_Init_9600(void)
{
    return USART1_Init(&USART_Config_9600_8N1);
}

/*====================================================================
 * Interrupt-driven RX — enable / disable
 *
 * USART1_EnableRxIRQ configures all three interrupt layers:
 *   Layer 1: USART1 peripheral RXNEIE bit
 *   Layer 2: PFIC IRQ_USART1 enable at requested priority
 *   Layer 3: global MIE — already set by startup file via mret
 *
 * Must be called after USART1_Init().
 *====================================================================*/
static void USART1_EnableRxIRQ(IRQ_Priority_t priority)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();

    /* Reset ring buffer to clean state before enabling */
    IRQ_CRITICAL_START();
    g_usart1_rx_head    = 0U;
    g_usart1_rx_tail    = 0U;
    g_usart1_rx_overrun = 0U;
    IRQ_CRITICAL_END();

    /* Layer 1 — enable RXNE interrupt in USART1 peripheral */
    p_usart->ctlr1 = (uint32_t)(p_usart->ctlr1 | USART_CTLR1_RXNEIE_MASK);

    /* Layer 2 — set priority then enable in PFIC */
    PFIC_SetPriority(IRQ_USART1, priority);
    PFIC_EnableIRQ(IRQ_USART1);
}

static void USART1_DisableRxIRQ(void)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();

    /* Disable in PFIC first — stops new ISR entries */
    PFIC_DisableIRQ(IRQ_USART1);

    /* Then disable in peripheral */
    p_usart->ctlr1 = (uint32_t)(p_usart->ctlr1 & ~USART_CTLR1_RXNEIE_MASK);
}

/*====================================================================
 * Ring buffer read functions
 * These are called from your main loop, not from the ISR.
 * All accesses to shared state use critical sections.
 *====================================================================*/

// USART1_RxAvailable — number of bytes waiting in the ring buffer
static uint32_t USART1_RxAvailable(void)
{
    uint8_t head = 0U;
    uint8_t tail = 0U;

    IRQ_CRITICAL_START();
    head = g_usart1_rx_head;
    tail = g_usart1_rx_tail;
    IRQ_CRITICAL_END();

    return (uint32_t)((uint8_t)(head - tail) & (uint8_t)USART1_RX_BUF_MASK);
}

// USART1_RxRead — read one byte from the ring buffer
//   Returns USART_OK if a byte was available.
//   Returns USART_ERR_EMPTY if the buffer was empty.
static USART_Result_t USART1_RxRead(uint8_t * const p_byte)
{
    uint8_t        tail   = 0U;
    USART_Result_t result = USART_ERR_EMPTY;

    if (p_byte == ((void *)0U)) { return USART_ERR_PARAM; }

    IRQ_CRITICAL_START();
    if (g_usart1_rx_tail != g_usart1_rx_head)
    {
        tail             = g_usart1_rx_tail;
        *p_byte          = g_usart1_rx_buf[tail];
        g_usart1_rx_tail = (uint8_t)((tail + 1U) & USART1_RX_BUF_MASK);
        result           = USART_OK;
    }
    IRQ_CRITICAL_END();

    return result;
}

// USART1_RxFlush — discard all bytes currently in the ring buffer
static void USART1_RxFlush(void)
{
    IRQ_CRITICAL_START();
    g_usart1_rx_tail = g_usart1_rx_head;
    IRQ_CRITICAL_END();
}

// USART1_RxOverrunCount — bytes lost because the buffer was full
//   An overrun means the ISR received a byte but had nowhere to put it.
//   Fix by increasing USART1_RX_BUF_SIZE or reading the buffer faster.
static uint32_t USART1_RxOverrunCount(void)
{
    uint32_t count = 0U;

    IRQ_CRITICAL_START();
    count = g_usart1_rx_overrun;
    IRQ_CRITICAL_END();

    return count;
}

/*====================================================================
 * USART1 ISR — interrupt service routine
 *
 * This definition overrides the weak alias in startup.S automatically.
 * The linker prefers non-weak definitions — no startup file changes needed.
 *
 * Placed in .highcode so it executes from RAM:
 *   At 1Mbaud a byte arrives every 10µs. Flash wait states at 48MHz
 *   add unpredictable latency. RAM execution is deterministic.
 *
 * The WCH-Interrupt-fast attribute tells GCC to use mret (machine
 * return from exception) instead of ret at the end of the function,
 * which is required for RISC-V interrupt handlers.
 *====================================================================*/
void USART1_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")))
    __attribute__((section(".highcode")));

void USART1_IRQHandler(void)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t statr    = 0U;
    uint8_t  byte     = 0U;
    uint8_t  next_head = 0U;

    /* Read status once — captures all flags atomically */
    statr = p_usart->statr;

    if ((statr & USART_STATR_RXNE_MASK) != 0U)
    {
        /* Reading DATAR clears RXNE and all error flags */
        byte = (uint8_t)(p_usart->datar & 0xFFU);

        /* Compute next write position */
        next_head = (uint8_t)((g_usart1_rx_head + 1U) & USART1_RX_BUF_MASK);

        if (next_head != g_usart1_rx_tail)
        {
            /* Space available — store byte and advance head */
            g_usart1_rx_buf[g_usart1_rx_head] = byte;
            g_usart1_rx_head                  = next_head;
        }
        else
        {
            /* Buffer full — byte is lost, count the event */
            g_usart1_rx_overrun = g_usart1_rx_overrun + 1U;
        }
    }

    /* Handle overrun error separately — ORE is cleared by reading
     * STATR (done above) then DATAR. If RXNE was also set we already
     * read DATAR above. If only ORE is set, read DATAR to clear it. */
    if (((statr & USART_STATR_ORE_MASK)  != 0U) &&
        ((statr & USART_STATR_RXNE_MASK) == 0U))
    {
        (void)p_usart->datar;   /* clear ORE */
    }
}

/*====================================================================
 * Polling transmit
 *====================================================================*/

// USART1_SendByte — transmit one byte, poll TXE
//   Does NOT wait for TC — use USART1_SendByte_WaitTC for the final
//   byte of a message or before disabling USART.
static USART_Result_t USART1_SendByte(uint8_t byte)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t       timeout = 0U;
    USART_Result_t result  = USART_OK;

    while ((timeout < USART_TIMEOUT) &&
           ((p_usart->statr & USART_STATR_TXE_MASK) == 0U))
    {
        timeout = timeout + 1U;
    }

    if ((p_usart->statr & USART_STATR_TXE_MASK) == 0U)
    {
        result = USART_ERR_BUSY;
    }
    else
    {
        p_usart->datar = (uint32_t)byte;
    }

    return result;
}

// USART1_SendByte_WaitTC — transmit one byte, wait for TC
//   Use for the last byte of a message, before disabling USART,
//   or before changing baud rate.
static USART_Result_t USART1_SendByte_WaitTC(uint8_t byte)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t       timeout = 0U;
    USART_Result_t result  = USART_OK;

    result = USART1_SendByte(byte);

    if (result == USART_OK)
    {
        while ((timeout < USART_TIMEOUT) &&
               ((p_usart->statr & USART_STATR_TC_MASK) == 0U))
        {
            timeout = timeout + 1U;
        }

        if ((p_usart->statr & USART_STATR_TC_MASK) == 0U)
        {
            result = USART_ERR_BUSY;
        }
    }

    return result;
}

// USART1_SendBuffer — transmit a byte array
//   Uses USART1_SendByte for all but the last byte.
//   Last byte uses USART1_SendByte_WaitTC — caller knows when done.
static USART_Result_t USART1_SendBuffer(const uint8_t * const p_data,
                                        uint32_t              len)
{
    uint32_t       i      = 0U;
    USART_Result_t result = USART_OK;

    if ((p_data == ((void *)0U)) || (len == 0U))
    {
        result = USART_ERR_PARAM;
    }

    if (result == USART_OK)
    {
        while ((i < (len - 1U)) && (result == USART_OK))
        {
            result = USART1_SendByte(p_data[i]);
            i      = i + 1U;
        }

        if (result == USART_OK)
        {
            result = USART1_SendByte_WaitTC(p_data[len - 1U]);
        }
    }

    return result;
}

// USART1_SendString — transmit a null-terminated string
//   Does not append \r\n — include in your string if needed.
//   Maximum USART_MAX_STRING_LEN bytes — safety guard against
//   unterminated strings.
static USART_Result_t USART1_SendString(const char * const p_str)
{
    const char    *p      = p_str;
    uint32_t       count  = 0U;
    USART_Result_t result = USART_OK;

    if (p_str == ((void *)0U)) { return USART_ERR_PARAM; }

    while ((result == USART_OK)    &&
           (*p != '\0')            &&
           (count < USART_MAX_STRING_LEN))
    {
        result = USART1_SendByte((uint8_t)*p);
        p      = p + 1;
        count  = count + 1U;
    }

    return result;
}

/*====================================================================
 * Polling receive — use when NOT using interrupt-driven RX
 *====================================================================*/

// USART1_DataAvailable — non-blocking check for received byte
//   Returns 1U if a byte is ready in DATAR, 0U if not.
static uint32_t USART1_DataAvailable(void)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t result = 0U;

    if ((p_usart->statr & USART_STATR_RXNE_MASK) != 0U)
    {
        result = 1U;
    }

    return result;
}

// USART1_ReceiveByte — blocking receive, poll RXNE
//   Waits up to USART_TIMEOUT iterations for a byte.
//   Returns USART_ERR_BUSY on timeout, USART_ERR_RX on receive error.
//   The byte is written to *p_byte even if an error flag was set —
//   caller decides whether to discard it.
static USART_Result_t USART1_ReceiveByte(uint8_t * const p_byte)
{
    volatile USART_TypeDef * const p_usart = USART1_GetBase();
    uint32_t       timeout = 0U;
    uint32_t       statr   = 0U;
    USART_Result_t result  = USART_OK;

    if (p_byte == ((void *)0U)) { return USART_ERR_PARAM; }

    while ((timeout < USART_TIMEOUT) &&
           ((p_usart->statr & USART_STATR_RXNE_MASK) == 0U))
    {
        timeout = timeout + 1U;
    }

    statr = p_usart->statr;

    if ((statr & USART_STATR_RXNE_MASK) == 0U)
    {
        result = USART_ERR_BUSY;
    }
    else
    {
        /* Reading DATAR clears RXNE and error flags */
        *p_byte = (uint8_t)(p_usart->datar & 0xFFU);

        if ((statr & USART_STATR_ERR_MASK) != 0U)
        {
            result = USART_ERR_RX;
        }
    }

    return result;
}

#endif /* CH32V003_HAL_USART_H */
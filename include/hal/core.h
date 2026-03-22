/// core.h
///
/// Core CPU control for CH32V003 — RISC-V RV32EC QingKeV2
/// Global interrupt enable/disable and critical section support.
/// MISRA C:2012 compliant
///
/// This header wraps RISC-V CSR (Control and Status Register)
/// instructions that control the CPU interrupt state.

#ifndef CH32V003_HAL_CORE_H
#define CH32V003_HAL_CORE_H

#include <stdint.h>

/*====================================================================
 * Background — how RISC-V interrupt control works
 *
 * The RISC-V CPU has a special register called mstatus (machine
 * status). Bit 3 of mstatus is called MIE (Machine Interrupt Enable).
 *
 *   mstatus.MIE = 1  →  interrupts can fire
 *   mstatus.MIE = 0  →  no interrupt can fire, regardless of PFIC
 *
 * This is the global master switch. Even if a peripheral has its
 * interrupt enabled and the PFIC has the IRQ line enabled, nothing
 * reaches the CPU while MIE = 0.
 *
 * Two CSR instructions control this bit:
 *   csrsi mstatus, 0x08   set   bit 3 — enable  interrupts
 *   csrci mstatus, 0x08   clear bit 3 — disable interrupts
 *
 * Both are single atomic instructions — they cannot be interrupted
 * mid-execution.
 *====================================================================*/

/*====================================================================
 * Simple enable / disable
 *====================================================================*/

// IRQ_Enable — enable global interrupts
//
// Sets mstatus.MIE. Interrupts can fire after this returns.
// Call once during startup after all peripherals are initialised.
// Note: the startup file already enables interrupts via mret —
// this function is for re-enabling after a deliberate disable.
static inline void IRQ_Enable(void)
{
    __asm volatile ("csrsi mstatus, 0x08");
}

// IRQ_Disable — disable global interrupts
//
// Clears mstatus.MIE. No interrupt can fire after this returns.
// Use sparingly — disabling interrupts increases ISR latency and
// can cause missed events if left disabled too long.
static inline void IRQ_Disable(void)
{
    __asm volatile ("csrci mstatus, 0x08");
}

/*====================================================================
 * Critical sections
 *
 * A critical section protects a block of code from being interrupted.
 * It saves the current interrupt state, disables interrupts, runs
 * the protected code, then restores the previous state.
 *
 * Why save and restore instead of just enable/disable:
 *
 * If interrupts were already disabled when IRQ_CRITICAL_START() is
 * called — for example inside a nested critical section — a plain
 * IRQ_Enable() at the end would incorrectly turn interrupts back on.
 * Saving and restoring mstatus preserves whatever state existed
 * before the critical section, making nesting safe.
 *
 * Usage:
 *
 *   IRQ_CRITICAL_START();
 *   // code here cannot be interrupted
 *   shared_variable = new_value;
 *   IRQ_CRITICAL_END();
 *
 * IMPORTANT: IRQ_CRITICAL_START and IRQ_CRITICAL_END must appear in
 * the same scope — they share a local variable _irq_saved_mstatus.
 * Do not put them in separate functions or separate if/else branches.
 *
 * Nested critical sections are safe:
 *
 *   IRQ_CRITICAL_START();          // saves MIE=1, disables
 *       IRQ_CRITICAL_START();      // saves MIE=0, still disabled
 *       IRQ_CRITICAL_END();        // restores MIE=0, still disabled
 *   IRQ_CRITICAL_END();            // restores MIE=1, re-enabled
 *====================================================================*/

// IRQ_CRITICAL_START — enter a critical section
//
// csrrci: read-and-clear-immediate
//   1. reads current mstatus into _irq_saved_mstatus
//   2. clears bit 3 (MIE) in mstatus
// Both steps happen atomically in one instruction.
#define IRQ_CRITICAL_START()                                         \
    uint32_t _irq_saved_mstatus = 0U;                                \
    __asm volatile (                                                 \
        "csrrci %0, mstatus, 0x08"                                   \
        : "=r" (_irq_saved_mstatus)                                  \
    )

// IRQ_CRITICAL_END — exit a critical section
//
// Restores mstatus to the value saved by IRQ_CRITICAL_START.
// If MIE was 1 before the critical section, it is 1 again now.
// If MIE was 0 before (nested section), it stays 0.
#define IRQ_CRITICAL_END()                                           \
    __asm volatile (                                                 \
        "csrw mstatus, %0"                                           \
        :                                                            \
        : "r" (_irq_saved_mstatus)                                   \
    )

/*====================================================================
 * State query
 *====================================================================*/

// IRQ_GetMstatus — read the current value of mstatus
//
// Useful for debugging or for code that needs to inspect the full
// CPU status register. For interrupt state specifically, prefer
// IRQ_IsEnabled() below.
static inline uint32_t IRQ_GetMstatus(void)
{
    uint32_t mstatus = 0U;
    __asm volatile ("csrr %0, mstatus" : "=r" (mstatus));
    return mstatus;
}

// IRQ_IsEnabled — returns 1U if global interrupts are currently on
//
// Returns 0U if interrupts are disabled (MIE = 0).
// Returns 1U if interrupts are enabled  (MIE = 1).
static inline uint32_t IRQ_IsEnabled(void)
{
    uint32_t result = 0U;

    if ((IRQ_GetMstatus() & 0x08U) != 0U)
    {
        result = 1U;
    }

    return result;
}

/*====================================================================
 * NOP — no-operation delay
 *
 * Inserts a single CPU cycle of delay. Used for tight timing loops
 * in bit-banging code. Combine multiples for longer delays.
 *
 * At 48MHz each NOP is approximately 20ns.
 *
 * Example — 200ns delay:
 *   NOP(); NOP(); NOP(); NOP(); NOP();
 *   NOP(); NOP(); NOP(); NOP(); NOP();
 *
 * Note: the compiler may optimise away consecutive NOPs if the
 * function is not marked __attribute__((section(".highcode"))).
 * For precise timing always use .highcode functions.
 * 
 *====================================================================*/
static inline void NOP(void)
{
    __asm volatile ("nop");
}

/*====================================================================
 * WFI — wait for interrupt
 *
 * Puts the CPU into a low-power idle state until the next interrupt
 * fires. The CPU wakes, services the interrupt, then resumes after
 * the WFI instruction.
 *
 * Use in your main loop when there is nothing to do:
 * 
 *   for (;;)
 *   {
 *       WFI();   // sleep until something happens
 *   }
 * 
 * This reduces power consumption compared to a busy spin loop.
 * Interrupts must be enabled (MIE = 1) or the CPU will never wake.
 * 
 *====================================================================*/
static inline void WFI(void)
{
    __asm volatile ("wfi");
}

#endif /* CH32V003_HAL_CORE_H */
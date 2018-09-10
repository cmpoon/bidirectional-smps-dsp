/*
 * stkov_systemstack.c
 *
 *  Created on: 10 Jun 2018
 *      Author: Chung
 */

// Choose which watchpoint to use (User configurable)
#define WP 0 // Valid values are 0 or 1 (Default is 0)

// Address and value definitions for Emulation Watchpoint Registers
#if WP == 0
#define WP_MASK (volatile unsigned long *)0x00000848 // WP0 MASK register addr
#define WP_REF (volatile unsigned long *)0x0000084A // WP0 REF register addr
#define WP_EVT_CNTL (volatile unsigned int *)0x0000084E // WP0 EVT_CNTL register addr
#define WP_EVT_ID (volatile unsigned int *)0x0000084F // WP0 EVT_ID register addr
#define EVT_CNTL 0x080A // EVT_CNTL value for WP0
#else
#define WP_MASK (volatile unsigned long *)0x00000828 // WP1 MASK register addr
#define WP_REF (volatile unsigned long *)0x0000082A // WP1 REF register addr
#define WP_EVT_CNTL (volatile unsigned int *)0x0000082E // WP1 EVT_CNTL register addr
#define WP_EVT_ID (volatile unsigned int *)0x0000082F // WP1 EVT_ID register addr
#define EVT_CNTL 0x081A // EVT_CNTL value for WP1
#endif

#define STKOV_RANGEMASK 0x0007 // 0x0007 = trigger range is 8 words

// Other Definitions
extern cregister volatile unsigned int IER;
/*********************************************************************
 * Function: STKOV_initSystemStack() *
 * Description: Configures a hardware watchpoint to trigger an *
 * RTOSINT on write access in a specified range at the end of the *
 * system (or C/C++) stack. *
 * DSP: TMS320C28x *
 * Include files: none *
 * Function Prototype: *
 * unsigned int STKOV_initSystemStack( *
 * unsigned long, unsigned long, unsigned int); *
 * Usage: error = STKOV_initSystemStack( *
 * stackStartAddr, stackEndAddr, margin); *
 * Input Parameters: *
 * unsigned long stackStartAddr = Address of first word in stack. *
 * unsigned long stackEndAddr = Address of first word after last *
 * word in stack. For example, if the stack starts at 0x100, and *
 * is of length 0x80, then StackEndAddr = 0x180. *
 * unsigned int margin = The minimum number of words before *
 * stackEndAddr that the watchpoint is to be set at. *
 * Return Value: *
 * unsigned int error: *
 * 0 = no error *
 * 1 = software failed to gain control of the WP *
 * 2 = the watchpoint range falls outside the stack *
 * Notes: *
 * 1) If any value other than 0 is returned, it means that the *
 * overflow detection was not enabled. *
 *********************************************************************/
unsigned int STKOV_initSystemStack(unsigned long stackStartAddr,
                                   unsigned long stackEndAddr,
                                   unsigned int margin)
{
    unsigned long addr; // Address to set the WP at
// Compute starting address of watchpoint range
    addr = (stackEndAddr - margin) & (unsigned long) (~STKOV_RANGEMASK);
    // Check to be sure the watchpoint range falls within the stack.
    if (addr < stackStartAddr) // Check if range underruns the stack start
        return (2); // Return error code
    if (addr > stackEndAddr) // Catch arithmetic underflow
        return (2); // Return error code
    // Enable EALLOW protected register access
    asm(" EALLOW");
    // Attempt to gain control of the watchpoint
    *WP_EVT_CNTL = 0x0001; // Write 0x0001 to EVT_CNTL to claim ownership
    // of the watchpoint
    asm(" RPT #1 || NOP");
    // Wait at least 3 cycles for the write to occur
    // Confirm that the application owns the watchpoint
    if ((*WP_EVT_ID & 0xC000) != 0x4000) // Software failed to gain control of watchpoint
    {
        asm(" EDIS");
        // Disable EALLOW protected register access
        return (1); // Return error code
    }
    // Proceed to configure the watchpoint
    *WP_MASK = (unsigned long) STKOV_RANGEMASK; // Watchpoint reference address mask
    *WP_REF = addr | (unsigned long) STKOV_RANGEMASK; // Watchpoint reference address
    // (write all masked bits as 1's)
    *WP_EVT_CNTL = EVT_CNTL; // Enable the watchpoint
    IER |= 0x8000; // Enable RTOSINT
    // Successful Return
    asm(" EDIS");
    // Disable EALLOW protected register access
    return (0); // Return with no error
} //end of STKOV_initSystemStack()
// end of file stkov_systemstack.c


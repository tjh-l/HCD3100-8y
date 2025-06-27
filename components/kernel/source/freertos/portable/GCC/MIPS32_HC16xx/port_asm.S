#include <mips/cpu.h>
#include <regdef.h>
#include "ISR_Support.h"

	.extern uxSavedTaskStackPointer
	.extern vPortInterruptHandoff
	.extern xISRStackTop
	.extern show_stack
	.extern pRaException
	.extern pSpException

	.set    noreorder
	.set    noat
	.text

# --------------------------------------------------------------------
#   Function_Name: _osGeneralExceptHdl
#   Description: Exception and Interrupt Handler
#   Arguments: None
#   Return Value: None
# --------------------------------------------------------------------
	.globl  _osGeneralExceptHdl
	.ent    _osGeneralExceptHdl

_osGeneralExceptHdl:
	mfc0    k0, C0_CAUSE
	li      k1, 0x7c
	and     k1, k0
	beqz    k1, not_exception
	nop
	.word   0x1000ffff
	nop

not_exception:
	portYIELD_SAVE

	la      sp, xISRStackTop
	lw      sp, (sp)

	jal     vPortInterruptHandoff
	nop

	portYIELD_RESTORE

	.end    _osGeneralExceptHdl

/******************************************************************/

	.set    noat
	.ent    vPortStartFirstTask
	.global vPortStartFirstTask

vPortStartFirstTask:

	/* Simply restore the context of the highest priority task that has been
	created so far. */
	portRESTORE_CONTEXT

	.end vPortStartFirstTask

/*******************************************************************/

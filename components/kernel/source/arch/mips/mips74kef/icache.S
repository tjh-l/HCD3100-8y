#include <machine/regdef.h>
#include <mips/hal.h>

	.text
	.set    noreorder

 #--------------------------------------------------------------------
 #
 #   Function_Name: icache_invalidate_all
 #
 #--------------------------------------------------------------------
	.globl  icache_invalidate_all
	.ent    icache_invalidate_all

icache_invalidate_all:
	li      t0, 0x80000000
	li      t2, 0x80008000 # 32KB cache
FLUSH_ALL_REPEAT:
	.set    mips3
	cache   Index_Invalidate_I, 0x000(t0)
	cache   Index_Invalidate_I, 0x020(t0)
	cache   Index_Invalidate_I, 0x040(t0)
	cache   Index_Invalidate_I, 0x060(t0)
	cache   Index_Invalidate_I, 0x080(t0)
	cache   Index_Invalidate_I, 0x0a0(t0)
	cache   Index_Invalidate_I, 0x0c0(t0)
	cache   Index_Invalidate_I, 0x0e0(t0)
	cache   Index_Invalidate_I, 0x100(t0)
	cache   Index_Invalidate_I, 0x120(t0)
	cache   Index_Invalidate_I, 0x140(t0)
	cache   Index_Invalidate_I, 0x160(t0)
	cache   Index_Invalidate_I, 0x180(t0)
	cache   Index_Invalidate_I, 0x1a0(t0)
	cache   Index_Invalidate_I, 0x1c0(t0)
	cache   Index_Invalidate_I, 0x1e0(t0)
	cache   Index_Invalidate_I, 0x200(t0)
	cache   Index_Invalidate_I, 0x220(t0)
	cache   Index_Invalidate_I, 0x240(t0)
	cache   Index_Invalidate_I, 0x260(t0)
	cache   Index_Invalidate_I, 0x280(t0)
	cache   Index_Invalidate_I, 0x2a0(t0)
	cache   Index_Invalidate_I, 0x2c0(t0)
	cache   Index_Invalidate_I, 0x2e0(t0)
	cache   Index_Invalidate_I, 0x300(t0)
	cache   Index_Invalidate_I, 0x320(t0)
	cache   Index_Invalidate_I, 0x340(t0)
	cache   Index_Invalidate_I, 0x360(t0)
	cache   Index_Invalidate_I, 0x380(t0)
	cache   Index_Invalidate_I, 0x3a0(t0)
	cache   Index_Invalidate_I, 0x3c0(t0)
	cache   Index_Invalidate_I, 0x3e0(t0)
	.set    mips1
	addiu   t0, 0x400
	blt     t0, t2, FLUSH_ALL_REPEAT
	nop

	.set    mips3
	sync     #synchronization barrier
	.set    mips1
	lui     t0, 0xa000
	lw      t1, 0(t0)
	.set    mips3
	sync     #synchronization barrier
	.set    mips1
	nop
	jr      ra
	nop
	.end    icache_invalidate_all

 #--------------------------------------------------------------------
 #
 #   Function_Name: cache_flush
 #
 #--------------------------------------------------------------------
	.globl  icache_invalidate
	.ent    icache_invalidate

icache_invalidate:
	addiu   sp, sp, -24
	sw      ra, 20(sp)
	#jal    arch_local_irq_disable
	nop

	blez    a1, FLUSH_END
	li      t2, 0x8000 # 32KB cache
	blt     a1, t2, FLUSH_RANGE
	nop

	jal     icache_invalidate_all
	nop
	b       FLUSH_END
	nop

FLUSH_RANGE:
	li      t2, 0xffffffe0  # Get start address
	and     t0, a0, t2
	add     t1, a0, a1      # Get end address
	addu    t1, -1
	and     t1, t2

FLUSH_RANGE_REPEAT:
	.set    push
	.set    noreorder
	.set    mips3
	cache   Hit_Invalidate_I, 0x000(t0)
	#   hi_writeback
	.set    pop
	bne     t0, t1, FLUSH_RANGE_REPEAT # if flushed data len. < len
	addiu   t0, 32 # 32 Byte cache line

	nop
	.set    mips3
	sync     #synchronization barrier
	.set    mips1
	nop

	lui     t2, 0xa000
	lw      t1, (t2)
	nop
	.set    push
	.set    noreorder
	.set    mips3
	sync
	.set    pop
	.set    mips1
	nop

FLUSH_END:
	lw      ra, 20(sp)
	addiu   sp, sp, 24
	jr      ra
	nop

	.end    icache_invalidate

	.set    reorder

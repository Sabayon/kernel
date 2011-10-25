/*
 * OMAP44xx sleep code.
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * 	Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 * This program is free software,you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/linkage.h>
#include <asm/system.h>

#include <mach/omap4-common.h>

ENTRY(omap_bus_sync)
	stmfd	sp!, {lr}
	/* SO write to drain of MPU-2-DDR T2ASYNC FIFO */
	bl	omap_get_dram_barrier_base
	ldr	r2, [r0]
	str	r2, [r0]
	/* SO write to drain MPU-2-L3 T2ASYNC FIFO */
	bl	omap_get_sram_barrier_base
	ldr	r2, [r0]
	str	r2, [r0]
	isb
	ldmfd	sp!, {pc}
ENDPROC(omap_bus_sync)

ENTRY(omap_do_wfi)
	stmfd	sp!, {lr}
	/* Drain interconnect write buffers. */
	bl omap_bus_sync

	/*
	 * Execute an ISB instruction to ensure that all of the
	 * CP15 register changes have been committed.
	 */
	isb

	/*
	 * Execute a barrier instruction to ensure that all cache,
	 * TLB and branch predictor maintenance operations issued
	 * by any CPU in the cluster have completed.
	 */
	dsb
	dmb

	/*
	 * Execute a WFI instruction and wait until the
	 * STANDBYWFI output is asserted to indicate that the
	 * CPU is in idle and low power state. CPU can specualatively
	 * prefetch the instructions so add NOPs after WFI. Sixteen
	 * NOPs as per Cortex-A9 pipeline.
	 */
	wfi					@ Wait For Interrupt
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	ldmfd	sp!, {pc}
ENDPROC(omap_do_wfi)

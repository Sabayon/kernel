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

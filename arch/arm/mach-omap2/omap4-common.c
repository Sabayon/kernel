/*
 * OMAP4 specific common source file.
 *
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Author:
 *	Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 *
 * This program is free software,you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/hardware/gic.h>
#include <asm/hardware/cache-l2x0.h>

#include <mach/hardware.h>
#include <mach/omap4-common.h>

#ifdef CONFIG_CACHE_L2X0
#define L2X0_POR_OFFSET_VALUE		0x9
static void __iomem *l2cache_base;
#endif

void __iomem *gic_dist_base_addr;


void __init gic_init_irq(void)
{
	void __iomem *gic_cpu_base;

	/* Static mapping, never released */
	gic_dist_base_addr = ioremap(OMAP44XX_GIC_DIST_BASE, SZ_4K);
	BUG_ON(!gic_dist_base_addr);

	/* Static mapping, never released */
	gic_cpu_base = ioremap(OMAP44XX_GIC_CPU_BASE, SZ_512);
	BUG_ON(!gic_cpu_base);

	gic_init(0, 29, gic_dist_base_addr, gic_cpu_base);
}

#ifdef CONFIG_CACHE_L2X0

static void omap4_l2x0_disable(void)
{
	/* Disable PL310 L2 Cache controller */
	omap_smc1(0x102, 0x0);
}

static void omap4_l2x0_set_debug(unsigned long val)
{
	/* Program PL310 L2 Cache controller debug register */
	omap_smc1(0x100, val);
}

static int __init omap_l2_cache_init(void)
{
	u32 aux_ctrl = 0;
	u32 por_ctrl = 0;
	u32 lockdown = 0;

	/*
	 * To avoid code running on other OMAPs in
	 * multi-omap builds
	 */
	if (!cpu_is_omap44xx())
		return -ENODEV;

	/* Static mapping, never released */
	l2cache_base = ioremap(OMAP44XX_L2CACHE_BASE, SZ_4K);
	BUG_ON(!l2cache_base);

	/*
	 * 16-way associativity, parity disabled
	 * Way size - 32KB (es1.0)
	 * Way size - 64KB (es2.0 +)
	 */
	aux_ctrl = readl_relaxed(l2cache_base + L2X0_AUX_CTRL);

	if (omap_rev() == OMAP4430_REV_ES1_0) {
		aux_ctrl |= 0x2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT;
		goto skip_aux_por_api;
	}

	/*
	 * Drop instruction prefetch hint since it degrades the
	 * the performance.
	 */
	aux_ctrl |= ((0x3 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) |
		(1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT) |
		(1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |
		(1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT));

	omap_smc1(0x109, aux_ctrl);

	/* Setup POR Control register */
	por_ctrl = readl_relaxed(l2cache_base + L2X0_PREFETCH_CTRL);

	/*
	 * Double linefill is available only on OMAP4460 L2X0.
	 * Undocumented bit 25 is set for better performance.
	 */
	if (cpu_is_omap446x())
		por_ctrl |= ((1 << L2X0_PREFETCH_DATA_PREFETCH_SHIFT) |
			(1 << L2X0_PREFETCH_DOUBLE_LINEFILL_SHIFT) |
			(1 << 25));

	if (cpu_is_omap446x() || (omap_rev() >= OMAP4430_REV_ES2_2)) {
		por_ctrl |= L2X0_POR_OFFSET_VALUE;
		omap_smc1(0x113, por_ctrl);
	}

	if (cpu_is_omap446x()) {
		writel_relaxed(0xa5a5, l2cache_base + 0x900);
		writel_relaxed(0xa5a5, l2cache_base + 0x908);
		writel_relaxed(0xa5a5, l2cache_base + 0x904);
		writel_relaxed(0xa5a5, l2cache_base + 0x90C);
	}

	/*
	 * FIXME: Temporary WA for OMAP4460 stability issue.
	 * Lock-down specific L2 cache ways which  makes effective
	 * L2 size as 512 KB instead of 1 MB
	 */
	if (cpu_is_omap446x()) {
		lockdown = 0xa5a5;
		writel_relaxed(lockdown, l2cache_base + L2X0_LOCKDOWN_WAY_D0);
		writel_relaxed(lockdown, l2cache_base + L2X0_LOCKDOWN_WAY_D1);
		writel_relaxed(lockdown, l2cache_base + L2X0_LOCKDOWN_WAY_I0);
		writel_relaxed(lockdown, l2cache_base + L2X0_LOCKDOWN_WAY_I1);
	}

skip_aux_por_api:
	/* Enable PL310 L2 Cache controller */
	omap_smc1(0x102, 0x1);

	l2x0_init(l2cache_base, aux_ctrl, L2X0_AUX_CTRL_MASK);

	/*
	 * Override default outer_cache.disable with a OMAP4
	 * specific one
	*/
	outer_cache.disable = omap4_l2x0_disable;
	outer_cache.set_debug = omap4_l2x0_set_debug;

	return 0;
}
early_initcall(omap_l2_cache_init);
#endif

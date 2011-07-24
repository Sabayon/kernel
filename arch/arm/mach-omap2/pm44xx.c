/*
 * OMAP4 Power Management Routines
 *
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/irqreturn.h>

#include <plat/serial.h>

#include "pm.h"
#include "powerdomain.h"
#include "clock.h"
#include "cminst44xx.h"
#include "prcm44xx.h"
#include "cm1_44xx.h"
#include "cm2_44xx.h"
#include "prm44xx.h"
#include "prminst44xx.h"

#include <mach/omap4-common.h>

#include "prm-regbits-44xx.h"
#include "cm-regbits-44xx.h"

struct power_state {
	struct powerdomain *pwrdm;
	u32 next_state;
#ifdef CONFIG_SUSPEND
	u32 saved_state;
#endif
	struct list_head node;
};

static LIST_HEAD(pwrst_list);

#ifdef CONFIG_SUSPEND
static int omap4_pm_suspend(void)
{
	do_wfi();
	return 0;
}

static int omap4_pm_enter(suspend_state_t suspend_state)
{
	int ret = 0;

	switch (suspend_state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = omap4_pm_suspend();
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int omap4_pm_begin(suspend_state_t state)
{
	disable_hlt();
	return 0;
}

static void omap4_pm_end(void)
{
	enable_hlt();
	return;
}

static const struct platform_suspend_ops omap_pm_ops = {
	.begin		= omap4_pm_begin,
	.end		= omap4_pm_end,
	.enter		= omap4_pm_enter,
	.valid		= suspend_valid_only_mem,
};
#endif /* CONFIG_SUSPEND */

static int __init pwrdms_setup(struct powerdomain *pwrdm, void *unused)
{
	struct power_state *pwrst;

	if (!pwrdm->pwrsts)
		return 0;

	pwrst = kmalloc(sizeof(struct power_state), GFP_ATOMIC);
	if (!pwrst)
		return -ENOMEM;
	pwrst->pwrdm = pwrdm;
	pwrst->next_state = PWRDM_POWER_ON;
	list_add(&pwrst->node, &pwrst_list);

	return omap_set_pwrdm_state(pwrst->pwrdm, pwrst->next_state);
}

static void __init prcm_setup_regs(void)
{
	struct clk *dpll_abe_ck, *dpll_core_ck, *dpll_iva_ck;
	struct clk *dpll_mpu_ck, *dpll_per_ck, *dpll_usb_ck;
	struct clk *dpll_unipro_ck;
	/*Enable all the DPLL autoidle */
	dpll_abe_ck = clk_get(NULL, "dpll_abe_ck");
	omap3_dpll_allow_idle(dpll_abe_ck);
	dpll_core_ck = clk_get(NULL, "dpll_core_ck");
	omap3_dpll_allow_idle(dpll_core_ck);
	dpll_iva_ck = clk_get(NULL, "dpll_iva_ck");
	omap3_dpll_allow_idle(dpll_iva_ck);
	if (cpu_is_omap446x())
		dpll_mpu_ck = clk_get(NULL, "virt_dpll_mpu_ck");
	else
		dpll_mpu_ck = clk_get(NULL, "dpll_mpu_ck");
	omap3_dpll_allow_idle(dpll_mpu_ck);
	dpll_per_ck = clk_get(NULL, "dpll_per_ck");
	omap3_dpll_allow_idle(dpll_per_ck);
	dpll_usb_ck = clk_get(NULL, "dpll_usb_ck");
	omap3_dpll_allow_idle(dpll_usb_ck);
	dpll_unipro_ck = clk_get(NULL, "dpll_unipro_ck");
	omap3_dpll_allow_idle(dpll_unipro_ck);
	/* Enable autogating for all DPLL post dividers */
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_MPU_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT1_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M4_DPLL_IVA_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M5_DPLL_IVA_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTHIF_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M3_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT1_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M4_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M5_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT3_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M6_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT4_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M7_DPLL_CORE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTHIF_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M3_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT1_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M4_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M5_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT3_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M6_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_HSDIVIDER_CLKOUT4_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M7_DPLL_PER_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_ABE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_ABE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTHIF_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM1_PARTITION, OMAP4430_CM1_CKGEN_INST, OMAP4_CM_DIV_M3_DPLL_ABE_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_USB_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKDCOLDO_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_CLKDCOLDO_DPLL_USB_OFFSET);
	omap4_cminst_rmw_inst_reg_bits(OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK, 0x0,
		OMAP4430_CM2_PARTITION, OMAP4430_CM2_CKGEN_INST, OMAP4_CM_DIV_M2_DPLL_UNIPRO_OFFSET);
	/* Enable IO_ST interrupt */
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_IO_ST_MASK, OMAP4430_IO_ST_MASK,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_OCP_SOCKET_INST, OMAP4_PRM_IRQENABLE_MPU_OFFSET);

	/* Enable GLOBAL_WUEN */
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_GLOBAL_WUEN_MASK, OMAP4430_GLOBAL_WUEN_MASK,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_IO_PMCTRL_OFFSET);
	/*
	 * Errata ID: i608 Impacted OMAP4430 ES 1.0,2.0,2.1,2.2
	 * On OMAP4, Retention-Till-Access Memory feature is not working
	 * reliably and hardware recommondation is keep it disabled by
	 * default
	 */
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_DISABLE_RTA_EXPORT_MASK,
		0x1 << OMAP4430_DISABLE_RTA_EXPORT_SHIFT,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_SRAM_WKUP_SETUP_OFFSET);
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_DISABLE_RTA_EXPORT_MASK,
		0x1 << OMAP4430_DISABLE_RTA_EXPORT_SHIFT,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_LDO_SRAM_CORE_SETUP_OFFSET);
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_DISABLE_RTA_EXPORT_MASK,
		0x1 << OMAP4430_DISABLE_RTA_EXPORT_SHIFT,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_LDO_SRAM_MPU_SETUP_OFFSET);
	omap4_prminst_rmw_inst_reg_bits(OMAP4430_DISABLE_RTA_EXPORT_MASK,
		0x1 << OMAP4430_DISABLE_RTA_EXPORT_SHIFT,
		OMAP4430_PRM_PARTITION, OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_LDO_SRAM_IVA_SETUP_OFFSET);
	/* Toggle CLKREQ in RET and OFF states */
	omap4_prminst_write_inst_reg(0x2, OMAP4430_PRM_PARTITION,
		OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_CLKREQCTRL_OFFSET);
	/*
	 * De-assert PWRREQ signal in Device OFF state
	 *	0x3: PWRREQ is de-asserted if all voltage domain are in
	 *	OFF state. Conversely, PWRREQ is asserted upon any
	 *	voltage domain entering or staying in ON or SLEEP or
	 *	RET state.
	 */
	omap4_prminst_write_inst_reg(0x3, OMAP4430_PRM_PARTITION,
		OMAP4430_PRM_DEVICE_INST, OMAP4_PRM_PWRREQCTRL_OFFSET);
}
static irqreturn_t prcm_interrupt_handler (int irq, void *dev_id)
{
	u32 irqenable_mpu, irqstatus_mpu;

	irqenable_mpu = omap4_prm_read_inst_reg(OMAP4430_PRM_OCP_SOCKET_INST,
					 OMAP4_PRM_IRQENABLE_MPU_OFFSET);
	irqstatus_mpu = omap4_prm_read_inst_reg(OMAP4430_PRM_OCP_SOCKET_INST,
					 OMAP4_PRM_IRQSTATUS_MPU_OFFSET);

	/* Check if a IO_ST interrupt */
	if (irqstatus_mpu & OMAP4430_IO_ST_MASK) {
		omap4_trigger_ioctrl();
	}

	/* Clear the interrupt */
	irqstatus_mpu &= irqenable_mpu;
	omap4_prm_write_inst_reg(irqstatus_mpu, OMAP4430_PRM_OCP_SOCKET_INST,
					OMAP4_PRM_IRQSTATUS_MPU_OFFSET);

	return IRQ_HANDLED;
}

int omap4_can_sleep(void)
{
	if (!omap_uart_can_sleep())
		return -1;
	return 0;
}

/**
 * omap4_pm_init - Init routine for OMAP4 PM
 *
 * Initializes all powerdomain and clockdomain target states
 * and all PRCM settings.
 */
static int __init omap4_pm_init(void)
{
	int ret;

	if (!cpu_is_omap44xx())
		return -ENODEV;

	pr_err("Power Management for TI OMAP4.\n");

	ret = pwrdm_for_each(pwrdms_setup, NULL);
	if (ret) {
		pr_err("Failed to setup powerdomains\n");
		goto err2;
	}

#ifdef CONFIG_SUSPEND
	suspend_set_ops(&omap_pm_ops);
#endif /* CONFIG_SUSPEND */

err2:
	return ret;
}
late_initcall(omap4_pm_init);

/*
 * OMAP4 CPU idle Routines
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Santosh Shilimkar <santosh.shilimkar@ti.com>
 * Rajendra Nayak <rnayak@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>

#include <asm/proc-fns.h>

#include <mach/omap4-common.h>

#include "pm.h"
#include "prm.h"

#ifdef CONFIG_CPU_IDLE

/* Machine specific information to be recorded in the C-state driver_data */
struct omap4_idle_statedata {
	u32 cpu_state;
	u32 mpu_logic_state;
	u32 mpu_state;
	u8 valid;
};

static struct cpuidle_params cpuidle_params_table[] = {
	/* C1 - CPU0 ON + CPU1 ON + MPU ON */
	{.exit_latency = 2 + 2 , .target_residency = 5, .valid = 1},
	/* C2- CPU0 OFF + CPU1 OFF + MPU CSWR */
	{.exit_latency = 328 + 440 , .target_residency = 960, .valid = 1},
	/* C3 - CPU0 OFF + CPU1 OFF + MPU OSWR */
	{.exit_latency = 460 + 518 , .target_residency = 1100, .valid = 1},
};

#define OMAP4_NUM_STATES ARRAY_SIZE(cpuidle_params_table)

struct omap4_idle_statedata omap4_idle_data[OMAP4_NUM_STATES];
static struct powerdomain *mpu_pd, *cpu0_pd, *cpu1_pd;

/**
 * omap4_enter_idle - Programs OMAP4 to enter the specified state
 * @dev: cpuidle device
 * @state: The target state to be programmed
 *
 * Called from the CPUidle framework to program the device to the
 * specified low power state selected by the governor.
 * Returns the amount of time spent in the low power state.
 */
static int omap4_enter_idle(struct cpuidle_device *dev,
			struct cpuidle_state *state)
{
	struct omap4_idle_statedata *cx = cpuidle_get_statedata(state);
	struct timespec ts_preidle, ts_postidle, ts_idle;
	u32 cpu1_state;

	/* Used to keep track of the total time in idle */
	getnstimeofday(&ts_preidle);

	local_irq_disable();
	local_fiq_disable();

	/*
	 * Keep demoting CPU0 C-state till CPU1 hits OFF state.
	 * This is necessary to honour hardware recommondation
	 * of triggeing all the possible low power modes once CPU1 is
	 * out of coherency and in OFF mode.
	 * Update dev->last_state so that governor stats reflects right
	 * data.
	 */
	cpu1_state = pwrdm_read_pwrst(cpu1_pd);
	if (cpu1_state != PWRDM_POWER_OFF) {
		dev->last_state = dev->safe_state;
		cx = cpuidle_get_statedata(dev->safe_state);
	}

	/* Call idle CPU PM enter notifier chain */
	if (cx->cpu_state == PWRDM_POWER_OFF)
		cpu_pm_enter();

	pwrdm_set_logic_retst(mpu_pd, cx->mpu_logic_state);
	omap_set_pwrdm_state(mpu_pd, cx->mpu_state);

	/* Call idle CPU cluster PM enter notifier chain */
	if ((cx->mpu_state == PWRDM_POWER_RET) &&
		(cx->mpu_logic_state == PWRDM_POWER_OFF))
			cpu_cluster_pm_enter();

	omap4_enter_lowpower(dev->cpu, cx->cpu_state);

	/* Call idle CPU PM exit notifier chain */
	if (pwrdm_read_prev_pwrst(cpu0_pd) == PWRDM_POWER_OFF)
		cpu_pm_exit();

	/* Call idle CPU cluster PM exit notifier chain */
	if (omap4_mpuss_read_prev_context_state())
		cpu_cluster_pm_exit();

	getnstimeofday(&ts_postidle);
	ts_idle = timespec_sub(ts_postidle, ts_preidle);

	local_irq_enable();
	local_fiq_enable();

	return ts_idle.tv_nsec / NSEC_PER_USEC + ts_idle.tv_sec * USEC_PER_SEC;
}

DEFINE_PER_CPU(struct cpuidle_device, omap4_idle_dev);

struct cpuidle_driver omap4_idle_driver = {
	.name =		"omap4_idle",
	.owner =	THIS_MODULE,
};

/* Fill in the state data from the mach tables and register the driver_data */
static inline struct omap4_idle_statedata *_fill_cstate(
					struct cpuidle_device *dev,
					int idx, const char *descr)
{
	struct omap4_idle_statedata *cx = &omap4_idle_data[idx];
	struct cpuidle_state *state = &dev->states[idx];

	state->exit_latency	= cpuidle_params_table[idx].exit_latency;
	state->target_residency	= cpuidle_params_table[idx].target_residency;
	state->flags		= CPUIDLE_FLAG_TIME_VALID;
	state->enter		= omap4_enter_idle;
	cx->valid		= cpuidle_params_table[idx].valid;
	sprintf(state->name, "C%d", idx + 1);
	strncpy(state->desc, descr, CPUIDLE_DESC_LEN);
	cpuidle_set_statedata(state, cx);

	return cx;
}

/**
 * omap4_idle_init - Init routine for OMAP4 idle
 *
 * Registers the OMAP4 specific cpuidle driver to the cpuidle
 * framework with the valid set of states.
 */
int __init omap4_idle_init(void)
{
	struct omap4_idle_statedata *cx;
	struct cpuidle_device *dev;
	unsigned int cpu_id = 0;

	mpu_pd = pwrdm_lookup("mpu_pwrdm");
	cpu0_pd = pwrdm_lookup("cpu0_pwrdm");
	cpu1_pd = pwrdm_lookup("cpu1_pwrdm");
	if ((!mpu_pd) || (!cpu0_pd) || (!cpu1_pd))
		return -ENODEV;

	cpuidle_register_driver(&omap4_idle_driver);

	dev = &per_cpu(omap4_idle_dev, cpu_id);
	dev->cpu = cpu_id;

	/* C1 - CPU0 ON + CPU1 ON + MPU ON */
	cx = _fill_cstate(dev, 0, "MPUSS ON");
	dev->safe_state = &dev->states[0];
	cx->valid = 1;	/* C1 is always valid */
	cx->cpu_state = PWRDM_POWER_ON;
	cx->mpu_state = PWRDM_POWER_ON;
	cx->mpu_logic_state = PWRDM_POWER_RET;

	/* C2 - CPU0 OFF + CPU1 OFF + MPU CSWR */
	cx = _fill_cstate(dev, 1, "MPUSS CSWR");
	cx->cpu_state = PWRDM_POWER_OFF;
	cx->mpu_state = PWRDM_POWER_RET;
	cx->mpu_logic_state = PWRDM_POWER_RET;

	/* C3 - CPU0 OFF + CPU1 OFF + MPU OSWR */
	cx = _fill_cstate(dev, 2, "MPUSS OSWR");
	cx->cpu_state = PWRDM_POWER_OFF;
	cx->mpu_state = PWRDM_POWER_RET;
	cx->mpu_logic_state = PWRDM_POWER_OFF;

	dev->state_count = OMAP4_NUM_STATES;
	if (cpuidle_register_device(dev)) {
		pr_err("%s: CPUidle register device failed\n", __func__);
			return -EIO;
		}

	return 0;
}
#else
int __init omap4_idle_init(void)
{
	return 0;
}
#endif /* CONFIG_CPU_IDLE */

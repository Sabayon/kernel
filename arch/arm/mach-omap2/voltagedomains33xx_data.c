/*
 * AM33XX voltage domain data
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/regulator/consumer.h>

#include <plat/voltage.h>
#include <plat/omap_device.h>

#include "omap_opp_data.h"

#define TOLERANCE 12500 /* in uV */

int am33x_mpu_voltdm_scale(struct voltagedomain *voltdm,
				unsigned long target_volt)
{
	int ret = -EINVAL;


	if (!voltdm->regulator)
		return ret;


	ret = regulator_set_voltage(voltdm->regulator, target_volt,
		target_volt + TOLERANCE);

	if (ret)
		pr_debug("Voltage change failed, ret = %d\n", ret);
	else
		pr_debug("Voltage scaled to %d\n",
			regulator_get_voltage(voltdm->regulator));

	return ret;
}

struct omap_vdd_dep_info am33xx_vddmpu_dep_info[] = {
	{.name = NULL, .dep_table = NULL, .nr_dep_entries = 0},
};

static struct omap_vdd_info am33xx_vdd1_info;

int am33x_mpu_voltdm_init(struct voltagedomain *voltdm)
{
	struct regulator *mpu_regulator;
	struct device *mpu_dev;

	mpu_dev = omap_device_get_by_hwmod_name("mpu");
	if (!mpu_dev) {
		pr_warning("%s: unable to get the mpu device\n", __func__);
		return -EINVAL;
	}

	mpu_regulator = regulator_get(mpu_dev, voltdm->name);

	if (IS_ERR(mpu_regulator)) {
		pr_err("%s: Could not get regulator for %s\n",
			__func__, voltdm->name);
		return -ENODEV;
	} else {
		voltdm->regulator = mpu_regulator;
		voltdm->scale = &am33x_mpu_voltdm_scale;
	}

	return 0;
}

static struct voltagedomain am33xx_voltdm_mpu = {
	.name = "mpu",
	.scalable = true,
	.use_regulator = 1,
	.regulator_init = &am33x_mpu_voltdm_init,
	.vdd	= &am33xx_vdd1_info,
};

static struct voltagedomain am33xx_voltdm_core = {
	.name = "core",
};

static struct voltagedomain am33xx_voltdm_rtc = {
	.name = "rtc",
};

static struct voltagedomain *voltagedomains_am33xx[] __initdata = {
	&am33xx_voltdm_mpu,
	&am33xx_voltdm_core,
	&am33xx_voltdm_rtc,
	NULL,
};

static const char *sys_clk_name __initdata = "sys_clkin_ck";

void __init am33xx_voltagedomains_init(void)
{
	struct voltagedomain *voltdm;
	int i;

	am33xx_vdd1_info.dep_vdd_info = am33xx_vddmpu_dep_info;

	for (i = 0; voltdm = voltagedomains_am33xx[i], voltdm; i++)
		voltdm->sys_clk.name = sys_clk_name;

	voltdm_init(voltagedomains_am33xx);
}

/* linux/drivers/staging/thermal_exynos4/thermal_interface.c
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/cpu_cooling.h>
#include <linux/platform_data/exynos4_tmu.h>
#include <linux/exynos_thermal.h>

static unsigned int verbose;

struct exynos4_thermal_zone {
	unsigned int idle_interval;
	unsigned int active_interval;
	struct thermal_zone_device *therm_dev;
	struct thermal_cooling_device *cool_dev;
	struct platform_device *exynos4_dev;
	struct exynos4_tmu_platform_data *sensor_data;
};
static struct thermal_sensor_info *exynos4_sensor_info;

static struct exynos4_thermal_zone *th_zone;

static int exynos4_get_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode *mode)
{
	if (exynos4_sensor_info) {
		pr_info("Temperature sensor not initialised\n");
		*mode = THERMAL_DEVICE_DISABLED;
	} else
		*mode = THERMAL_DEVICE_ENABLED;
	return 0;
}

/*
 * set operation mode;
 * enabled: the thermal layer of the kernel takes care about
 *          the temperature.
 * disabled: temperature sensor is not enabled.
 */
static int exynos4_set_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode mode)
{
	if (!th_zone->therm_dev) {
		pr_notice("thermal zone not registered\n");
		return 0;
	}
	if (mode == THERMAL_DEVICE_ENABLED)
		th_zone->therm_dev->polling_delay =
				th_zone->active_interval*1000;
	else
		th_zone->therm_dev->polling_delay =
				th_zone->idle_interval*1000;

	thermal_zone_device_update(th_zone->therm_dev);
	pr_info("thermal polling set for duration=%d sec\n",
				th_zone->therm_dev->polling_delay/1000);
	return 0;
}

/*This may be called from interrupt based temperature sensor*/
void exynos4_report_trigger(void)
{
	unsigned int th_temp = th_zone->sensor_data->threshold;
	unsigned int monitor_temp = th_temp +
			th_zone->sensor_data->trigger_levels[1];

	thermal_zone_device_update(th_zone->therm_dev);

	if (th_zone->therm_dev->last_temperature > monitor_temp)
		th_zone->therm_dev->polling_delay =
					th_zone->active_interval*1000;
	else
		th_zone->therm_dev->polling_delay =
					th_zone->idle_interval*1000;
}

static int exynos4_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
	if (verbose)
		pr_info("%s, trip no=%d\n", __func__, trip);
	if (trip == 0 || trip == 1)
		*type = THERMAL_TRIP_STATE_ACTIVE;
	else if (trip == 2)
		*type = THERMAL_TRIP_CRITICAL;
	else
		return -EINVAL;

	return 0;
}

static int exynos4_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
	unsigned int th_temp = th_zone->sensor_data->threshold;

	/*Monitor zone*/
	if (trip == 0)
		*temp = th_temp + th_zone->sensor_data->trigger_levels[1];
	/*Warn zone*/
	else if (trip == 1)
		*temp = th_temp + th_zone->sensor_data->trigger_levels[2];
	/*Panic zone*/
	else if (trip == 2)
		*temp = th_temp + th_zone->sensor_data->trigger_levels[3];
	else
		return -EINVAL;

	return 0;
}

static int exynos4_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temp)
{
	unsigned int th_temp = th_zone->sensor_data->threshold;
	/*Panic zone*/
	*temp = th_temp + th_zone->sensor_data->trigger_levels[3];
	return 0;
}

static int exynos4_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	/* if the cooling device is the one from exynos4 bind it */
	if (cdev != th_zone->cool_dev)
		return 0;

	if (thermal_zone_bind_cooling_device(thermal, 0, cdev)) {
		pr_err("error binding cooling dev\n");
		return -EINVAL;
	}
	if (thermal_zone_bind_cooling_device(thermal, 1, cdev)) {
		pr_err("error binding cooling dev\n");
		return -EINVAL;
	}

	return 0;
}

static int exynos4_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
	if (cdev != th_zone->cool_dev)
		return 0;
	if (thermal_zone_unbind_cooling_device(thermal, 0, cdev)) {
		pr_err("error unbinding cooling dev\n");
		return -EINVAL;
	}
	return 0;
}

static int exynos4_get_temp(struct thermal_zone_device *thermal,
			       unsigned long *t)
{
	int temp = 0;
	void *data;

	if (!exynos4_sensor_info) {
		pr_info("Temperature sensor not initialised\n");
		return -EINVAL;
	}
	data = exynos4_sensor_info->private_data;
	temp = exynos4_sensor_info->read_temperature(data);

	if (verbose)
		pr_notice("temp %d\n", temp);

	*t = temp;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops exynos4_dev_ops = {
	.bind = exynos4_bind,
	.unbind = exynos4_unbind,
	.get_temp = exynos4_get_temp,
	.get_mode = exynos4_get_mode,
	.set_mode = exynos4_set_mode,
	.get_trip_type = exynos4_get_trip_type,
	.get_trip_temp = exynos4_get_trip_temp,
	.get_crit_temp = exynos4_get_crit_temp,
};

int exynos4_register_temp_sensor(struct thermal_sensor_info *sensor_info)
{
	exynos4_sensor_info = sensor_info;
	return 0;
}


static int __devinit exynos4_probe(struct platform_device *device)
{
	return 0;
}

static int exynos4_remove(struct platform_device *device)
{
	return 0;
}

static struct platform_driver exynos4_driver = {
	.driver = {
		.name  = "exynos4",
		.owner = THIS_MODULE,
	},
	.probe = exynos4_probe,
	.remove = exynos4_remove,
};

static int exynos4_register_platform(void)
{
	int err = 0;

	th_zone = kzalloc(sizeof(struct exynos4_thermal_zone), GFP_KERNEL);
	if (!th_zone)
		return -ENOMEM;

	err = platform_driver_register(&exynos4_driver);
	if (err)
		return err;

	th_zone->exynos4_dev = platform_device_alloc("exynos4", -1);
	if (!th_zone->exynos4_dev) {
		err = -ENOMEM;
		goto err_device_alloc;
	}
	err = platform_device_add(th_zone->exynos4_dev);
	if (err)
		goto err_device_add;

	return 0;

err_device_add:
	platform_device_put(th_zone->exynos4_dev);
err_device_alloc:
	platform_driver_unregister(&exynos4_driver);
	return err;
}

static void exynos4_unregister_platform(void)
{
	platform_device_unregister(th_zone->exynos4_dev);
	platform_driver_unregister(&exynos4_driver);
	kfree(th_zone);
}

static int exynos4_register_thermal(void)
{
	if (!exynos4_sensor_info) {
		pr_info("Temperature sensor not initialised\n");
		return -EINVAL;
	}

	th_zone->sensor_data = exynos4_sensor_info->sensor_data;
	if (!th_zone->sensor_data) {
		pr_info("Temperature sensor data not initialised\n");
		return -EINVAL;
	}

	th_zone->cool_dev = cpufreq_cooling_register(
		(struct freq_pctg_table *)th_zone->sensor_data->freq_tab,
		th_zone->sensor_data->level_count);

	if (IS_ERR(th_zone->cool_dev))
		return -EINVAL;

	th_zone->therm_dev = thermal_zone_device_register("exynos4-therm",
				3, NULL, &exynos4_dev_ops, 0, 0, 0, 1000);
	if (IS_ERR(th_zone->therm_dev))
		return -EINVAL;

	th_zone->active_interval = 1;
	th_zone->idle_interval = 10;
	exynos4_set_mode(th_zone->therm_dev, THERMAL_DEVICE_DISABLED);

	return 0;
}

static void exynos4_unregister_thermal(void)
{
	if (th_zone->cool_dev)
		cpufreq_cooling_unregister();

	if (th_zone->therm_dev)
		thermal_zone_device_unregister(th_zone->therm_dev);
}

static int __init exynos4_thermal_init(void)
{
	int err = 0;

	err = exynos4_register_platform();
	if (err)
		goto out_err;

	err = exynos4_register_thermal();
	if (err)
		goto err_unreg;

	return 0;

err_unreg:
	exynos4_unregister_thermal();
	exynos4_unregister_platform();

out_err:
	return err;
}

static void __exit exynos4_thermal_exit(void)
{
	exynos4_unregister_thermal();
	exynos4_unregister_platform();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Daniel <amit.kachhap@linaro.org>");
MODULE_DESCRIPTION("samsung Exynos4 thermal monitor driver");

module_init(exynos4_thermal_init);
module_exit(exynos4_thermal_exit);

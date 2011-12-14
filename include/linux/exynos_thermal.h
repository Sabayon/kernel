/* linux/include/linux/exynos_thermal.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef THERMAL_INTERFACE_H
#define THERMAL_INTERFACE_H
/* CPU Zone information */

#define SENSOR_NAME_LEN	16

#define PANIC_ZONE      4
#define WARN_ZONE       3
#define MONITOR_ZONE    2
#define SAFE_ZONE       1
#define NO_ACTION       0

struct thermal_sensor_info {
	char	name[SENSOR_NAME_LEN];
	int	(*read_temperature)(void *data);
	void	*private_data;
	void	*sensor_data;
};

extern int exynos4_register_temp_sensor(struct thermal_sensor_info *sensor);
extern void exynos4_report_trigger(void);
#endif

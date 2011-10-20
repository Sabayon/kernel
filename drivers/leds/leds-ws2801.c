/*
 * LEDs driver for WS2801 RGB Controller
 *
 * Copyright (C) 2006 Kristian Kielhofner <kris@krisk.org>
 *
 * Based on leds-net48xx.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>

#define DRVNAME "ws2801-leds"
#define WS2801_LED_CLOCK_GPIO	159
#define WS2801_LED_DATA_GPIO	158

static unsigned long rgb_color;

static struct platform_device *pdev;

static void ws2801_set_rgb(void)
{
	int count;
	int color_bit;

	for (count = 23; count >= 0 ; count--) {
		color_bit = (rgb_color>>count) & (1<<0);
		gpio_set_value(WS2801_LED_DATA_GPIO, color_bit);
		gpio_set_value(WS2801_LED_CLOCK_GPIO, 1);
		gpio_set_value(WS2801_LED_CLOCK_GPIO, 0);
	}

}

static void ws2801_red_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	rgb_color &= ((0x00<<16)|(0xff<<8)|(0xff<<0));
	rgb_color |= (value<<16);
	ws2801_set_rgb();
}

static void ws2801_green_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	rgb_color &= ((0xff<<16)|(0x00<<8)|(0xff<<0));
	rgb_color |= (value<<8);
	ws2801_set_rgb();
}

static void ws2801_blue_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	rgb_color &= ((0xff<<16)|(0xff<<8)|(0x00<<0));
	rgb_color |= (value<<0);
	ws2801_set_rgb();
}

static struct led_classdev ws2801_red_led = {
	.name			= "ws2801-red",
	.brightness_set		= ws2801_red_led_set,
	.flags			= LED_CORE_SUSPENDRESUME,
};

static struct led_classdev ws2801_green_led = {
	.name		= "ws2801-green",
	.brightness_set	= ws2801_green_led_set,
	.flags			= LED_CORE_SUSPENDRESUME,
};

static struct led_classdev ws2801_blue_led = {
	.name           = "ws2801-blue",
	.brightness_set = ws2801_blue_led_set,
	.flags			= LED_CORE_SUSPENDRESUME,
};

static int ws2801_led_probe(struct platform_device *pdev)
{
	int ret;

	ret = led_classdev_register(&pdev->dev, &ws2801_red_led);
	if (ret < 0)
		return ret;

	ret = led_classdev_register(&pdev->dev, &ws2801_green_led);
	if (ret < 0)
		goto err1;

	ret = led_classdev_register(&pdev->dev, &ws2801_blue_led);
	if (ret < 0)
		goto err2;

	gpio_request_one(WS2801_LED_DATA_GPIO,
		GPIOF_OUT_INIT_LOW, "ws2801_data");

	gpio_request_one(WS2801_LED_CLOCK_GPIO,
		GPIOF_OUT_INIT_LOW, "ws2801_clock");

	ws2801_set_rgb();
	return ret;

err2:
	led_classdev_unregister(&ws2801_green_led);
err1:
	led_classdev_unregister(&ws2801_red_led);

	return ret;
}

static int ws2801_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&ws2801_red_led);
	led_classdev_unregister(&ws2801_green_led);
	led_classdev_unregister(&ws2801_blue_led);
	return 0;
}

static struct platform_driver ws2801_led_driver = {
	.probe		= ws2801_led_probe,
	.remove		= ws2801_led_remove,
	.driver		= {
		.name		= DRVNAME,
		.owner		= THIS_MODULE,
	},
};

static int __init ws2801_led_init(void)
{
	int ret;

	ret = platform_driver_register(&ws2801_led_driver);
	if (ret < 0)
		goto out;

out:
	return ret;
}

static void __exit ws2801_led_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&ws2801_led_driver);
}

module_init(ws2801_led_init);
module_exit(ws2801_led_exit);

MODULE_AUTHOR("David Anders <danders@tincantools.com>");
MODULE_DESCRIPTION("WS2801 RGB LED driver");
MODULE_LICENSE("GPL");


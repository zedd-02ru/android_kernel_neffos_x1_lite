/* Copyright (C) 1996-2016, TP-LINK TECHNOLOGIES CO., LTD.
 *
 * File name: usb_mode_switch.c
 *
 * Description: There are two usb mode in some projects ---- Normal Mode
 *              and Power Consumption Test mode. This module provides an
 *              interface to switch the usb mode.
 *
 * Author: Yubin Yang
 *
 * Email: yangyubin@tp-link.com.cn
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#include <../power/mt6755/bq24296.h>
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>

/* 0 for normal mode and 1 for power consumption test mode */
static int mode = 0;

static ssize_t usb_mode_switch_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	return sprintf(buf, "%s\n", mode == 0 ? "Normal Mode" :
			"Power Consumption Test Mode");
}

static ssize_t usb_mode_switch_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t len)
{
	int input;
	int ret;

	int pin_u2202 = 22;
	int pin_u2203 = 16;
	int pin_u2202_dir;
	int pin_u2203_dir;

	ret = sscanf(buf, "%d", &input);
	if (ret != 1) {
		pr_err("USB_MODE_SWITCH : Error input!\n");
		return -EINVAL;
	}

	if (input == 0) {  /* Switch USB mode to Normal Mode */
		bq24296_set_watchdog(0x1);
		bq24296_set_batfet_disable(0x0);

		pin_u2202_dir = 1;
		pin_u2203_dir = 1;

		mt_set_gpio_dir(pin_u2202, pin_u2202_dir);
		mt_set_gpio_out(pin_u2202, 0);
		mt_set_gpio_dir(pin_u2203, pin_u2203_dir);
		mt_set_gpio_out(pin_u2203, 0);

		mode = 0;

	} else {
		pin_u2202_dir = 1;
		pin_u2203_dir = 1;

		mt_set_gpio_dir(pin_u2203, pin_u2203_dir);
		mt_set_gpio_out(pin_u2203, 1);
		mt_set_gpio_mode(pin_u2202, 0);
		mt_set_gpio_dir(pin_u2202, pin_u2202_dir);
		mt_set_gpio_out(pin_u2202, 1);

		bq24296_set_watchdog(0x0);
		bq24296_set_batfet_disable(0x1);
		mode = 1;
	}

	return ret;
}

static DEVICE_ATTR(usb_mode_switch, S_IWUSR|S_IRUGO, usb_mode_switch_show,
		   usb_mode_switch_store);

static int __init usb_mode_switch_init(void)
{
	int err;
	err = sysfs_create_file(kernel_kobj, &dev_attr_usb_mode_switch.attr);
	if (err) {
		pr_err("USB_MODE_SWITCH : Can't create sysfs\n");
	}
	return 0;
}

static void __exit usb_mode_switch_exit(void)
{
	sysfs_remove_file(kernel_kobj, &dev_attr_usb_mode_switch.attr);
}

module_init(usb_mode_switch_init);
module_exit(usb_mode_switch_exit);

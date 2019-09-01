/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include "mach/mt_thermal.h"
#include <mach/mt_clkmgr.h>
#include <mt_ptp.h>
#include <mach/wd_api.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <tscpu_settings.h>

/* [lifeiping start] */
#include <asm-generic/current.h>
#include <linux/signal.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define SYSRST_SIG_CODE 1324

#define TIMER_DELAY  20

static struct task_struct g_task;
static struct task_struct *pg_task = &g_task;

static int sysrst_happened = 0;
/* [lifeiping end] */

/*=============================================================
 *Local variable definition
 *=============================================================*/
static unsigned int cl_dev_sysrst_state;
static unsigned int cl_dev_sysrst_state_buck;
static struct thermal_cooling_device *cl_dev_sysrst;
static struct thermal_cooling_device *cl_dev_sysrst_buck;
/*=============================================================*/

/* [lifeiping start] add send signal func */
extern unsigned int thermald_daemon_pid;

static void sysrst_func(unsigned long val)
{
	pr_err("[sysrst] force sysrst!\n");
	*(unsigned int *)0x0 = 0xdead;
}
static DEFINE_TIMER(sysrst_timer, sysrst_func, 0, 0);

static void sd_func(unsigned long val)
{
	pr_err("[shutdown] force shutdown!\n");
	machine_power_off();
}
static DEFINE_TIMER(sd_timer, sd_func, 0, 0);

/*
 * param:
 *       type [in]  1:system shutdown, others: system reset
 */
int send_sysrst_signal(unsigned int type)
{
	int ret = 0;
	siginfo_t info;

        if (!thermald_daemon_pid) {
		tscpu_printk("[cpu_sysrst] can't get thermald pid\n");
		return -1;
	}

	pg_task = get_pid_task(find_vpid(thermald_daemon_pid), PIDTYPE_PID);

	if (1 == type) {
		if (pg_task) {
			info.si_signo = SIGIO;
			info.si_errno = 0;
			info.si_code = 1; //shutdown signal
			info.si_addr = NULL;
			ret = send_sig_info(SIGIO, &info, pg_task);
		}

		mod_timer(&sd_timer, jiffies + TIMER_DELAY * HZ); //after 20s force shutdown
	} else {
		if (pg_task) {
			info.si_signo = SIGIO;
			info.si_errno = 0;
			info.si_code = SYSRST_SIG_CODE; //sysrst signal
			info.si_addr = NULL;
			ret = send_sig_info(SIGIO, &info, pg_task);
		}

		mod_timer(&sysrst_timer, jiffies + TIMER_DELAY * HZ); //after 20s force sysrst
	}

	if (ret != 0) {
		return -1;
		tscpu_printk("[sysrst]send signal fail\n");
	}

	tscpu_printk("[sysrst] send signal success, pid is: %d\n", thermald_daemon_pid);

	return 0;
}
EXPORT_SYMBOL(send_sysrst_signal);
/* [lifeiping end] */

/*
 * cooling device callback functions (tscpu_cooling_sysrst_ops)
 * 1 : ON and 0 : OFF
 */
static int sysrst_cpu_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("sysrst_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int sysrst_cpu_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("sysrst_cpu_get_cur_state\n"); */
	*state = cl_dev_sysrst_state;
	return 0;
}

static int sysrst_cpu_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	cl_dev_sysrst_state = state;

	if (cl_dev_sysrst_state == 1) {
		tscpu_printk("sysrst_cpu_set_cur_state = 1\n");
		tscpu_printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		tscpu_printk("*****************************************\n");
		tscpu_printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

#ifndef CONFIG_ARM64
		BUG();
#else
		/* [lifeiping start] modify action of sysrst to send signal */
		if (0 == sysrst_happened) {
			send_sysrst_signal(0);
			sysrst_happened = 1;
		}
		//*(unsigned int *)0x0 = 0xdead;    /* To trigger data abort to reset the system for thermal protection. */
		/* [lifeiping end] */
#endif

	}
	return 0;
}

static int sysrst_buck_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("sysrst_buck_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int sysrst_buck_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("sysrst_buck_get_cur_state\n"); */
	*state = cl_dev_sysrst_state_buck;
	return 0;
}

static int sysrst_buck_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	cl_dev_sysrst_state_buck = state;

	if (cl_dev_sysrst_state_buck == 1) {
		tscpu_printk("sysrst_buck_set_cur_state = 1\n");
		tscpu_printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		tscpu_printk("*****************************************\n");
		tscpu_printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

#ifndef CONFIG_ARM64
		BUG();
#else
		/* [lifeiping start] modify action of sysrst to send signal */
		if (0 == sysrst_happened) {
			send_sysrst_signal(0);
			sysrst_happened = 1;
		}
		//*(unsigned int *)0x0 = 0xdead;    /* To trigger data abort to reset the system for thermal protection. */
		/* [lifeiping end] */
#endif

	}
	return 0;
}



static struct thermal_cooling_device_ops mtktscpu_cooling_sysrst_ops = {
	.get_max_state = sysrst_cpu_get_max_state,
	.get_cur_state = sysrst_cpu_get_cur_state,
	.set_cur_state = sysrst_cpu_set_cur_state,
};

static struct thermal_cooling_device_ops mtktsbuck_cooling_sysrst_ops = {
	.get_max_state = sysrst_buck_get_max_state,
	.get_cur_state = sysrst_buck_get_cur_state,
	.set_cur_state = sysrst_buck_set_cur_state,
};

static int __init mtk_cooler_sysrst_init(void)
{
	tscpu_dprintk("mtk_cooler_sysrst_init: Start\n");
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktscpu-sysrst", NULL,
							    &mtktscpu_cooling_sysrst_ops);

	cl_dev_sysrst_buck = mtk_thermal_cooling_device_register("mtktsbuck-sysrst", NULL,
							    &mtktsbuck_cooling_sysrst_ops);


	tscpu_dprintk("mtk_cooler_sysrst_init: End\n");
	return 0;
}

static void __exit mtk_cooler_sysrst_exit(void)
{
	tscpu_dprintk("mtk_cooler_sysrst_exit\n");
	if (cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}
module_init(mtk_cooler_sysrst_init);
module_exit(mtk_cooler_sysrst_exit);

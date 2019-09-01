/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/charging.h>
#include "mtk_pep20_intf.h"

#if !defined(TA_AC_CHARGING_CURRENT)
#include <mach/mt_pe.h>
#endif

#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT
static struct mutex pep20_access_lock;
static struct mutex pep20_pmic_sync_lock;
static struct wake_lock pep20_suspend_lock;
static int pep20_ta_vchr_org = 5000; /* mA */
static int pep20_idx = -1;
static int pep20_vbus = 5000; /* mA */
static bool pep20_to_check_chr_type = true;
static bool pep20_is_cable_out_occur; /* Plug out happend while detect PE+20 */
static bool pep20_is_connect;
static bool pep20_is_enabled = true;
int pe20_vbus_test = 5000; /* mv*/
kal_bool pe20_start_test = KAL_FALSE;
int pep20_bus_current = CHARGE_CURRENT_2000_00_MA;
static bool pep20_reset_5v_for_temp = false;	// [lidebiao] Charging too long for high temp
static bool pep20_is_connect_for_charging_test = false;	// [lidebiao] add for fastcharging test
static bool pep20_connect_is_detected = false;	// [lidebiao] add codes for pep detect error


typedef struct _pep20_profile {
	u32 vbat;
	u32 vchr;
} pep20_profile_t, *p_pep20_profile_t;

typedef struct _pep20_profile_current_struct {
	u32 vbat;
	u32 chrcurent;
} pep20_profile_current_struct,pep20_profile_current_struct_p;;

pep20_profile_t pep20_profile[] = {
	{3400, VBAT3400_VBUS},
	{3500, VBAT3500_VBUS},
	{3600, VBAT3600_VBUS},
	{3700, VBAT3700_VBUS},
	{3800, VBAT3800_VBUS},
	{3900, VBAT3900_VBUS},
	{4000, VBAT4000_VBUS},
	{4100, VBAT4100_VBUS},
	{4200, VBAT4200_VBUS},
	{4300, VBAT4300_VBUS},
	{4400, VBAT4300_VBUS},
};

pep20_profile_current_struct pep20_profile_current[] = {
	{3400, CHARGE_CURRENT_2450_00_MA},
	{3500, CHARGE_CURRENT_2450_00_MA},
	{3600, CHARGE_CURRENT_2450_00_MA},
	{3700, CHARGE_CURRENT_2450_00_MA},
	{3800, CHARGE_CURRENT_2450_00_MA},
	{3900, CHARGE_CURRENT_2450_00_MA},
	{4000, CHARGE_CURRENT_2450_00_MA},
	{4100, CHARGE_CURRENT_2450_00_MA},
	{4200, CHARGE_CURRENT_2450_00_MA},
	{4300, CHARGE_CURRENT_2450_00_MA},
	{4400, CHARGE_CURRENT_2450_00_MA},
};

static int pep20_enable_hw_vbus_ovp(bool enable)
{
	int ret = 0;
	u32 data;

	data = (enable ? 1 : 0); /* Compatible with charging_hw_bq25896.c */
	ret = battery_charging_control(CHARGING_CMD_SET_VBUS_OVP_EN, &data);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);

	return ret;
}

/* Enable/Disable HW & SW VBUS OVP */
static int pep20_enable_vbus_ovp(bool enable)
{
	int ret = 0;
	u32 sw_ovp = (enable ? V_CHARGER_MAX : 15000);

	/* Enable/Disable HW(PMIC) OVP */
	ret = pep20_enable_hw_vbus_ovp(enable);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	/* Enable/Disable SW OVP status */
	batt_cust_data.v_charger_max = sw_ovp;

	return ret;
}


static int pep20_set_mivr(u32 mivr)
{
	int ret = 0;

	ret = battery_charging_control(CHARGING_CMD_SET_VINDPM, &mivr);
	if (ret < 0)
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);
	return ret;
}

static int pep20_leave(void)
{
	int ret = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);
	ret = mtk_pep20_reset_ta_vchr();
	if (ret < 0 || pep20_is_connect) {
		battery_log(BAT_LOG_CRTI,
			"%s: failed, is_connect = %d, ret = %d\n",
			__func__, pep20_is_connect, ret);
		return ret;
	}

	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);
	return ret;
}

static int pep20_check_leave_status(void)
{
	int ret = 0;
	u32 ichg = 0, vchr = 0;
	kal_bool current_sign;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	/* PE+ leaves unexpectedly */
	vchr = battery_meter_get_charger_voltage();
	if (abs(vchr - pep20_ta_vchr_org) < 1000) {
		battery_log(BAT_LOG_CRTI,
			"%s: PE+20 leave unexpectedly, recheck TA\n", __func__);
		pep20_to_check_chr_type = true;
		ret = pep20_leave();
		if (ret < 0 || pep20_is_connect)
			goto _err;

		return ret;
	}

	ichg = battery_meter_get_battery_current(); /* 0.1 mA */
	ichg /= 10; /* mA */
	current_sign = battery_meter_get_battery_current_sign();

	/* Check SOC & Ichg */
	if (BMT_status.SOC > batt_cust_data.ta_stop_battery_soc &&
	    current_sign && ichg < PEP20_ICHG_LEAVE_THRESHOLD) {
		ret = pep20_leave();
		if (ret < 0 || pep20_is_connect)
			goto _err;
		battery_log(BAT_LOG_CRTI,
			"%s: OK, SOC = (%d,%d), Ichg = %dmA, stop PE+20\n",
			__func__, BMT_status.SOC, batt_cust_data.ta_stop_battery_soc,
			ichg);
	}

	return ret;

_err:
	battery_log(BAT_LOG_CRTI, "%s: failed, is_connect = %d, ret = %d\n",
		__func__, pep20_is_connect, ret);
	return ret;
}

static int __pep20_set_ta_vchr(u32 chr_volt)
{
	int ret = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	/* Not to set chr volt if cable is plugged out */
	if (pep20_is_cable_out_occur) {
		battery_log(BAT_LOG_CRTI, "%s: failed, cable out\n", __func__);
		return -EIO;
	}

	ret = battery_charging_control(CHARGING_CMD_SET_TA20_CURRENT_PATTERN,
		&chr_volt);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);

	return ret;
}

static int pep20_set_ta_vchr(u32 chr_volt)
{
	int ret = 0;
	int vchr_before, vchr_after, vchr_delta;
	const u32 sw_retry_cnt_max = 3;
	const u32 retry_cnt_max = 5;
	u32 sw_retry_cnt = 0, retry_cnt = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	do {
		vchr_before = battery_meter_get_charger_voltage();
		ret = __pep20_set_ta_vchr(chr_volt);
		vchr_after = battery_meter_get_charger_voltage();

		vchr_delta = abs(vchr_after - chr_volt);

		/* It is successful
		 * if difference to target is less than 500mv
		 */
		if (vchr_delta < 500 && ret == 0) {
			battery_log(BAT_LOG_CRTI, "%s: OK, vchr = (%d, %d), vchr_target = %d\n",
				__func__, vchr_before, vchr_after, chr_volt);
			return ret;
		}

		if (ret == 0 || sw_retry_cnt >= sw_retry_cnt_max)
			retry_cnt++;
		else {
			msleep(2000);
			sw_retry_cnt++;
		}

		battery_log(BAT_LOG_CRTI,
			"%s: retry_cnt = (%d, %d), vchr = (%d, %d), vchr_target = %d\n",
			__func__, sw_retry_cnt, retry_cnt, vchr_before, vchr_after, chr_volt);

	} while (!pep20_is_cable_out_occur && BMT_status.charger_exist == KAL_TRUE
		&& retry_cnt < retry_cnt_max);

	ret = -EIO;
	battery_log(BAT_LOG_CRTI,
		"%s: failed, vchr_org = %d, vchr_after = %d, target_vchr = %d\n",
		__func__, pep20_ta_vchr_org, vchr_after, chr_volt);

	return ret;
}

static int pep20_detect_ta(void)
{
	int ret;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);
	pep20_ta_vchr_org = battery_meter_get_charger_voltage();

	/* Disable OVP */
	ret = pep20_enable_vbus_ovp(false);
	if (ret < 0)
		goto _err;

	if (abs(pep20_ta_vchr_org - 8500) > 500)
		ret = pep20_set_ta_vchr(8500);
	else
		ret = pep20_set_ta_vchr(6500);

	if (ret < 0) {
		pep20_to_check_chr_type = false;
		goto _err;
	}

	pep20_is_connect = true;
	mt_charger_enable_DP_voltage(1);
	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);

	return ret;
_err:
	pep20_is_connect = false;
	pep20_enable_vbus_ovp(true);
	battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n", __func__, ret);
	return ret;
}

int pep20_plugout_reset(void)
{
	int ret = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	pep20_to_check_chr_type = true;

	ret = mtk_pep20_reset_ta_vchr();
	if (ret < 0)
		goto _err;

	mt_charger_enable_DP_voltage(0);
	mtk_pep20_set_is_cable_out_occur(false);
	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);

	return ret;
_err:
	battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n", __func__, ret);

	return ret;
}

static int pep20_init_ta(void)
{
	int ret = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	ret = battery_charging_control(CHARGING_CMD_INIT, NULL);
	if (ret < 0) {
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);
	return ret;
}

int mtk_pep20_set_charging_current(CHR_CURRENT_ENUM *ichg, CHR_CURRENT_ENUM *aicr)
{
	int ret = 0;

	if (!pep20_is_connect)
		return -ENOTSUPP;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);
	*aicr = pep20_bus_current;
	*ichg = TA_AC_CHARGING_CURRENT;
	battery_log(BAT_LOG_CRTI, "%s: OK, ichg = %dmA, AICR = %dmA\n",
		__func__, *ichg / 100, *aicr / 100);

	return ret;
}

int mtk_pep20_init(void)
{
	wake_lock_init(&pep20_suspend_lock, WAKE_LOCK_SUSPEND,
		"PE+20 TA charger suspend wakelock");
	mutex_init(&pep20_access_lock);
	mutex_init(&pep20_pmic_sync_lock);
	return 0;
}

/* [lidebiao start] Charging too long for high temp */
int mtk_pep20_reset_ta_vchr_for_temp(void)
{
	int ret = 0, chr_volt = 0;
	u32 retry_cnt = 0;

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (pep20_reset_5v_for_temp == true) {
		return ret;
	}
	/* Reset TA's charging voltage */
	do {
		ret = battery_charging_control(CHARGING_CMD_SET_TA20_RESET,
			NULL);

		msleep(250);

		/* Check charger's voltage */
		chr_volt = battery_meter_get_charger_voltage();
		if (abs(chr_volt - pep20_ta_vchr_org) <= 1000) {
			pep20_vbus = chr_volt;
			pep20_idx = -1;
			mutex_lock(&pep20_pmic_sync_lock);
			pep20_reset_5v_for_temp = true;
			mutex_unlock(&pep20_pmic_sync_lock);
			break;
		}

		retry_cnt++;
	} while (retry_cnt < 3);

	if (!pep20_reset_5v_for_temp) {
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);

		/*
		 * SET_TA20_RESET success but chr_volt does not reset to 5V
		 * set ret = -EIO to represent the case
		 */
		if (ret == 0)
			ret = -EIO;
		return ret;
	}

	pep20_enable_vbus_ovp(true);
	pep20_set_mivr(4500);
	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);

	return ret;
}
/* [lidebiao end] */

int mtk_pep20_reset_ta_vchr(void)
{
	int ret = 0, chr_volt = 0;
	u32 retry_cnt = 0;

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	/* Reset TA's charging voltage */
	do {
		ret = battery_charging_control(CHARGING_CMD_SET_TA20_RESET,
			NULL);

		msleep(250);

		/* Check charger's voltage */
		chr_volt = battery_meter_get_charger_voltage();
		if (abs(chr_volt - pep20_ta_vchr_org) <= 1000) {
			pep20_vbus = chr_volt;
			pep20_idx = -1;
			pep20_is_connect = false;
			break;
		}

		retry_cnt++;
	} while (retry_cnt < 3);

	if (pep20_is_connect) {
		battery_log(BAT_LOG_CRTI, "%s: failed, ret = %d\n",
			__func__, ret);

		/*
		 * SET_TA20_RESET success but chr_volt does not reset to 5V
		 * set ret = -EIO to represent the case
		 */
		if (ret == 0)
			ret = -EIO;
		return ret;
	}

	pep20_enable_vbus_ovp(true);
	pep20_set_mivr(4500);
	battery_log(BAT_LOG_CRTI, "%s: OK\n", __func__);

	return ret;
}


int mtk_pep20_check_charger(void)
{
	int ret = 0;

	if (!pep20_is_enabled) {
		battery_log(BAT_LOG_CRTI, "%s: stop, PE+20 is disabled\n",
			__func__);
		return ret;
	}

	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);

	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	if (!BMT_status.charger_exist || pep20_is_cable_out_occur)
		pep20_plugout_reset();

	/*
	 * Not to check charger type or
	 * Not standard charger or
	 * SOC is not in range
	 */
	if (!pep20_to_check_chr_type ||
	    (BMT_status.charger_type != STANDARD_CHARGER && BMT_status.charger_type != NONSTANDARD_CHARGER) || // [houjihai] add pep2.0 check for non standard
	    BMT_status.SOC < batt_cust_data.ta_start_battery_soc ||
	    BMT_status.SOC >= batt_cust_data.ta_stop_battery_soc)
		goto _out;

	ret = pep20_init_ta();
	if (ret < 0)
		goto _out;

	ret = mtk_pep20_reset_ta_vchr();
	if (ret < 0)
		goto _out;

	ret = pep20_detect_ta();
	if (ret < 0)
		goto _out;

	pep20_to_check_chr_type = false;

	battery_log(BAT_LOG_CRTI, "%s: OK, to_check_chr_type = %d\n",
		__func__, pep20_to_check_chr_type);
	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
_out:
	battery_log(BAT_LOG_CRTI,
		"%s: stop, SOC = (%d, %d, %d), to_check_chr_type = %d, chr_type = %d, ret = %d\n",
		__func__, BMT_status.SOC, batt_cust_data.ta_start_battery_soc,
		batt_cust_data.ta_stop_battery_soc, pep20_to_check_chr_type,
		BMT_status.charger_type, ret);

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
}

int mtk_pep20_start_algorithm(void)
{
	int ret = 0;
	int i;
	int vbat, vbus, ichg;
	int pre_vbus, pre_idx;
	int tune = 0, pes = 0; /* For log, to know the state of PE+20 */
	kal_bool current_sign;
	u32 size;

	/* [lidebiao start] Charging too long for high temp */
	mutex_lock(&pep20_pmic_sync_lock);
	pep20_reset_5v_for_temp = false;
	mutex_unlock(&pep20_pmic_sync_lock);
	/* [lidebiao end] */

	if (!pep20_is_enabled) {
		battery_log(BAT_LOG_CRTI, "%s: stop, PE+20 is disabled\n",
			__func__);
		return ret;
	}

	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);
	battery_log(BAT_LOG_FULL, "%s: starts\n", __func__);

	if (!BMT_status.charger_exist || pep20_is_cable_out_occur)
		pep20_plugout_reset();

	if (!pep20_is_connect) {
		ret = -EIO;
		battery_log(BAT_LOG_CRTI, "%s: stop, PE+20 is not connected\n",
			__func__);
		wake_unlock(&pep20_suspend_lock);
		mutex_unlock(&pep20_access_lock);
		return ret;
	}

	vbat = battery_meter_get_battery_voltage(KAL_FALSE);
	vbus = battery_meter_get_charger_voltage();
	ichg = battery_meter_get_battery_current();
	current_sign = battery_meter_get_battery_current_sign();

	pre_vbus = pep20_vbus;
	pre_idx = pep20_idx;

	ret = pep20_check_leave_status();
	if (!pep20_is_connect || ret < 0) {
		pes = 1;
		goto _out;
	}

	size = ARRAY_SIZE(pep20_profile);
	for (i = 0; i < size; i++) {
		tune = 0;

		/* Exceed this level, check next level */
		if (vbat > (pep20_profile[i].vbat + 100))
			continue;

		/* If vbat is still 30mV larger than the lower level
		 * Do not down grade
		 */
		if (i < pep20_idx && vbat > (pep20_profile[i].vbat + 30))
			continue;

		if (pep20_vbus != pep20_profile[i].vchr)
			tune = 1;

		pep20_vbus = pep20_profile[i].vchr;
		pep20_bus_current = pep20_profile_current[i].chrcurent;
		pep20_idx = i;

		if( pe20_start_test == KAL_TRUE ) {
			pep20_vbus = pe20_vbus_test;
		}

		if (abs(vbus - pep20_vbus) >= 1000)
			tune = 2;

		if (tune != 0) {
			ret = pep20_set_ta_vchr(pep20_vbus);
			if (ret < 0)
				pep20_leave();
			else
				pep20_set_mivr(pep20_vbus - 1000);
		}
		break;
	}
	pes = 2;

_out:
	battery_log(BAT_LOG_CRTI,
		"%s: vbus = (%d, %d), idx = (%d, %d), I = (%d, %d)\n",
		__func__, pre_vbus, pep20_vbus, pre_idx, pep20_idx,
		(int)current_sign, (int)ichg / 10);

	battery_log(BAT_LOG_CRTI,
		"%s: SOC = %d, is_connect = %d, tune = %d, pes = %d, vbat = %d, ret = %d\n",
		__func__, BMT_status.SOC, pep20_is_connect, tune, pes, vbat, ret);
	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
}

void mtk_pep20_set_to_check_chr_type(bool check)
{
	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);

	battery_log(BAT_LOG_CRTI, "%s: check = %d\n", __func__, check);
	pep20_to_check_chr_type = check;
	pep20_connect_is_detected = check;	// [lidebiao] Solve PEP2.0 detected failed when high temp

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);
}

void mtk_pep20_set_is_enable(bool enable)
{
	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);

	battery_log(BAT_LOG_CRTI, "%s: enable = %d\n", __func__, enable);
	pep20_is_enabled = enable;

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);
}

void mtk_pep20_set_is_cable_out_occur(bool out)
{
	battery_log(BAT_LOG_CRTI, "%s: out = %d\n", __func__, out);
	mutex_lock(&pep20_pmic_sync_lock);
	pep20_is_cable_out_occur = out;
	pep20_reset_5v_for_temp = false;	// [lidebiao] Charging too long for high temp
	pep20_is_connect_for_charging_test = false;	// [lidebiao] add for fastcharging test
	pep20_connect_is_detected = false;	// [lidebiao] add codes for pep detect error
	mutex_unlock(&pep20_pmic_sync_lock);
}

bool mtk_pep20_get_to_check_chr_type(void)
{
	return pep20_to_check_chr_type;
}

bool mtk_pep20_get_is_connect(void)
{
	/*
	 * Cable out is occurred,
	 * but not execute plugout_reset yet
	 */
	if (pep20_is_cable_out_occur)
		return false;

	return pep20_is_connect;
}

bool mtk_pep20_get_is_enable(void)
{
	return pep20_is_enabled;
}

int battery_pump_express20_algorithm_set_vbus(CHR_VOLTAGE_ENUM chr_vol)
{
	int ret;

	if (pep20_is_connect == KAL_FALSE)
		return -2;

	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);

	pe20_vbus_test = chr_vol;
	pe20_start_test = KAL_TRUE;
	ret = pep20_set_ta_vchr(chr_vol);
	if (ret < 0)
		pep20_leave();
	else
		pep20_set_mivr(pep20_vbus - 1000);
	battery_log(BAT_LOG_CRTI,"[PE+20]start vbus = %d\n", chr_vol);

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
}

/* [lidebiao start] add for fastcharging test */
bool mtk_pep20_get_is_connect_for_charging_test(void)
{
	/*
	 * Cable out is occurred,
	 * but not execute plugout_reset yet
	 */
	if (pep20_is_cable_out_occur)
		return false;

	return pep20_is_connect_for_charging_test;
}

int mtk_pep20_check_charger_for_charging_test(void)
{
	int ret = 0;

	if (pep20_is_connect_for_charging_test) {
		battery_log(BAT_LOG_CRTI, "%s: stop, PE+20 is connect.\n",
			__func__);
		return ret;
	}

	/* [lidebiao start] add codes for pep detect error*/
	if (pep20_connect_is_detected) {
		battery_log(BAT_LOG_CRTI, "%s: stop, PE+20 is not connect and has detected failed.\n",
			__func__);
		return ret;
	}
	/* [lidebiao end] */

	mutex_lock(&pep20_access_lock);
	wake_lock(&pep20_suspend_lock);

	battery_log(BAT_LOG_CRTI, "%s: starts\n", __func__);

	if (!BMT_status.charger_exist || pep20_is_cable_out_occur)
		pep20_plugout_reset();

	/*
	 * Not to check charger type or
	 * Not standard charger or
	 * SOC is not in range
	 */
	/* [lidebiao start] Avoid check PEP2.0 error when reboot phone */
	if (!pep20_to_check_chr_type ||
	    (BMT_status.charger_type != STANDARD_CHARGER && BMT_status.charger_type != NONSTANDARD_CHARGER)) // [houjihai] add pep2.0 check for non standard
	/* [lidebiao end] */
		goto _out;

	ret = pep20_init_ta();
	battery_log(BAT_LOG_CRTI, "11%s: OK, ret= %d\n",__func__, ret);
	if (ret < 0)
		goto _out;

	ret = mtk_pep20_reset_ta_vchr();
	battery_log(BAT_LOG_CRTI, "22%s: OK, ret= %d\n",__func__, ret);
	if (ret < 0)
		goto _out;

	ret = pep20_detect_ta();
	if (ret < 0)
		goto _out;

	pep20_is_connect_for_charging_test = true;
	pep20_connect_is_detected = true;	// [lidebiao] add codes for pep detect error
	ret = mtk_pep20_reset_ta_vchr();

	pep20_is_enabled = true;
	pep20_to_check_chr_type = true;

	battery_log(BAT_LOG_CRTI, "%s: OK, to_check_chr_type = %d\n",
		__func__, pep20_is_connect_for_charging_test);

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
_out:
	pep20_is_enabled = true;
	pep20_to_check_chr_type = true;
	pep20_connect_is_detected = true;	// [lidebiao] add codes for pep detect error
	battery_log(BAT_LOG_CRTI,
		"%s: stop, SOC = (%d, %d, 110), to_check_chr_type = %d, chr_type = %d, ret = %d\n",
		__func__, BMT_status.SOC, batt_cust_data.ta_start_battery_soc, pep20_to_check_chr_type,
		BMT_status.charger_type, ret);

	wake_unlock(&pep20_suspend_lock);
	mutex_unlock(&pep20_access_lock);

	return ret;
}
/* [lidebiao end] */

#endif

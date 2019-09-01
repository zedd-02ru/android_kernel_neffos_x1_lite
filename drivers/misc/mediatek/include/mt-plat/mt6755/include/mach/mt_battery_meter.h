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

#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

/* ============================================================*/
/* define*/
/* ============================================================*/
/*#define SOC_BY_AUXADC*/
#define SOC_BY_HW_FG
#define CONFIG_MTK_EMBEDDED_BATTERY	// [lidebiao] Modify GM2.0 param
/*#define HW_FG_FORCE_USE_SW_OCV*/
/*#define SOC_BY_SW_FG*/

/*#define CONFIG_DIS_CHECK_BATTERY*/
/*#define FIXED_TBAT_25*/

/* ADC resistor  */
#define R_BAT_SENSE	4
#define R_I_SENSE	4
#define R_CHARGER_1	330
#define R_CHARGER_2	39

#define TEMPERATURE_T0	110
#define TEMPERATURE_T1	0
#define TEMPERATURE_T2	25
#define TEMPERATURE_T3	50
#define TEMPERATURE_T	255 /* This should be fixed, never change the value*/

/* [lidebiao start] Modify GM2.0 param */
#define FG_METER_RESISTANCE	5

#if defined(CONFIG_TPLINK_PRODUCT_TP902)
/* Qmax for battery  */
#define Q_MAX_POS_50 2250
#define Q_MAX_POS_25 2150
#define Q_MAX_POS_0 944
#define Q_MAX_NEG_10 699

#define Q_MAX_POS_50_H_CURRENT 2160
#define Q_MAX_POS_25_H_CURRENT 2106
#define Q_MAX_POS_0_H_CURRENT 925
#define Q_MAX_NEG_10_H_CURRENT 685
#endif

#if defined(CONFIG_TPLINK_PRODUCT_TP903)
/* Qmax for battery  */
#define Q_MAX_POS_50 2993
#define Q_MAX_POS_25 2944
#define Q_MAX_POS_0 2445
#define Q_MAX_NEG_10 645

#define Q_MAX_POS_50_H_CURRENT 2933
#define Q_MAX_POS_25_H_CURRENT 2885
#define Q_MAX_POS_0_H_CURRENT 2396
#define Q_MAX_NEG_10_H_CURRENT 632
#endif

#if defined(CONFIG_TPLINK_PRODUCT_TP904) || defined(CONFIG_TPLINK_PRODUCT_TP912)
/* Qmax for battery  */
#define Q_MAX_POS_50 2444
#define Q_MAX_POS_25 2390
#define Q_MAX_POS_0 1252
#define Q_MAX_NEG_10 750

#define Q_MAX_POS_50_H_CURRENT 2395
#define Q_MAX_POS_25_H_CURRENT 2342
#define Q_MAX_POS_0_H_CURRENT 1227
#define Q_MAX_NEG_10_H_CURRENT 735
#endif
/* [lidebiao end] */

/* [zhangdong start] Modify GM2.0 param */
#if defined(CONFIG_TPLINK_PRODUCT_TP910)
/* Qmax for battery  */
#define Q_MAX_POS_50 3016
#define Q_MAX_POS_25 2857
#define Q_MAX_POS_0 1465
#define Q_MAX_NEG_10 941

#define Q_MAX_POS_50_H_CURRENT 2956
#define Q_MAX_POS_25_H_CURRENT 2800
#define Q_MAX_POS_0_H_CURRENT 1436
#define Q_MAX_NEG_10_H_CURRENT 922
#endif
/* [zhangdong end] */

/* Discharge Percentage */
#define OAM_D5	1		/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#ifdef CONFIG_MTK_HAFG_20
#define CUST_TRACKING_POINT	0
#else
#define CUST_TRACKING_POINT	1
#endif
#define CUST_R_SENSE         56
#define CUST_HW_CC	0
#define AGING_TUNING_VALUE	103
#define CUST_R_FG_OFFSET	0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE	1000
#define R_FG_BOARD_SLOPE	1000 /*slope*/
/* [lidebiao start] Modify GM2.0 param */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	/* [zhangdong start] Modify GM2.0 param */
	#if defined(CONFIG_TPLINK_PRODUCT_TP910)
		#define CAR_TUNE_VALUE	102
	#else
		#define CAR_TUNE_VALUE	100 /*1.00 */
	#endif
	/* [zhangdong end] */
#else
#if defined(CONFIG_TPLINK_PRODUCT_TP902)
	#define CAR_TUNE_VALUE	101 /*1.00 */
#endif
#if defined(CONFIG_TPLINK_PRODUCT_TP903)
	#define CAR_TUNE_VALUE	100 /*1.00 */
#endif
#endif
/* [lidebiao end] */

/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10  /*1mA*/
#define MinErrorOffset	1000
#define FG_VBAT_AVERAGE_SIZE	18
#define R_FG_VALUE	10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC		30
#define DIFFERENCE_HWOCV_SWOCV		15	// [lidebiao] Modify GM2.0 param
#define DIFFERENCE_SWOCV_RTC		1       // [houjihai] Modify DIFFERENCE_SWOCV_RTC from 10 to 1
#define DIFFERENCE_HWOCV_VBAT		30
#define DIFFERENCE_VBAT_RTC			30
/* [lidebiao start] Modify GM2.0 param */
#if defined(CONFIG_TPLINK_PRODUCT_TP902) || defined(CONFIG_TPLINK_PRODUCT_TP904) || defined(CONFIG_TPLINK_PRODUCT_TP910) || defined(CONFIG_TPLINK_PRODUCT_TP912)
#define DIFFERENCE_SWOCV_RTC_POS 15
#else
#define DIFFERENCE_SWOCV_RTC_POS 30
#endif
/* [lidebiao end] */

#define MAX_SWOCV	3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC	70
#define AGING1_UPDATE_SOC	30
/* [lidebiao start] Modify GM2.0 param */
#if defined(CONFIG_TPLINK_PRODUCT_TP902) || defined(CONFIG_TPLINK_PRODUCT_TP904) || defined(CONFIG_TPLINK_PRODUCT_TP910) || defined(CONFIG_TPLINK_PRODUCT_TP912)
	#define BATTERYPSEUDO100	98
#else
	#define BATTERYPSEUDO100	96
#endif
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
/* [zhangdong start] */
#if defined(CONFIG_TPLINK_PRODUCT_TP910)
#define BATTERYPSEUDO1 6
#else
#define BATTERYPSEUDO1 4
#endif
/* [zhangdong end] */
#else
#define BATTERYPSEUDO1 4
#endif
/* [lidebiao end] */

/*PCB setting*/
#define CALI_CAR_TUNE_AVG_NUM 60

/* #define Q_MAX_BY_SYS */			/*8. Qmax variant by system drop voltage.*/
#define Q_MAX_SYS_VOLTAGE		3350
#define SHUTDOWN_GAUGE0
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS	60

#define SHUTDOWN_SYSTEM_VOLTAGE	3400
#define CHARGE_TRACKING_TIME	60
#define DISCHARGE_TRACKING_TIME	10

#define RECHARGE_TOLERANCE	10
/* SW Fuel Gauge */
#define MAX_HWOCV	5
#define MAX_VBAT	90

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE	5
#define CUST_POWERON_MAX_VBAT_TOLRANCE	90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE	30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP	3600		/*3.6V*/
#define VBAT_LOW_POWER_WAKEUP	3500		/*3.5v*/
#define NORMAL_WAKEUP_PERIOD	5400		/*90 * 60 = 90 min*/
#define LOW_POWER_WAKEUP_PERIOD	300		/*5 * 60 = 5 min*/
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	/*30 s*/

#define INIT_SOC_BY_SW_SOC

/*3. UI SOC sync to FG SOC immediately*/
/*#define SYNC_UI_SOC_IMM*/

/*6. Q_MAX aging algorithm*/
#define MTK_ENABLE_AGING_ALGORITHM

/*5. Gauge Adjust by OCV 9. MD sleep current check*/
#define MD_SLEEP_CURRENT_CHECK

/*7. Qmax variant by current loading.*/
/* #define Q_MAX_BY_CURRENT */

#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC
/* #define USE_EMBEDDED_BATTERY */

/* Calculate do in Kernel */
/* #define FORCE_D0_IN_KERNEL */

/* Use UI_SOC3 to smooth UI_SOC2 */
/* #define USING_SMOOTH_UI_SOC2 */

/* SOC track to SWOCV */
#define CUST_TRACKING_GAP		15	/* start tracking gap */
#define CUST_TRACKINGOFFSET		0	/* Force offset to shift SOC to 0 */
#define CUST_TRACKINGEN			0	/* 0:disable, 1:enable */

/* Multi battery */
#if defined(CONFIG_TPLINK_PRODUCT_TP903)
#define MTK_MULTI_BAT_PROFILE_SUPPORT
#endif
#endif

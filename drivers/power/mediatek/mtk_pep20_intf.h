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

#ifndef __MTK_PEP20_INTF_H
#define __MTK_PEP20_INTF_H


#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT
extern int mtk_pep20_init(void);
extern int mtk_pep20_reset_ta_vchr(void);
extern int mtk_pep20_check_charger(void);
extern int mtk_pep20_start_algorithm(void);
extern int mtk_pep20_set_charging_current(CHR_CURRENT_ENUM *ichg,
	CHR_CURRENT_ENUM *aicr);

extern void mtk_pep20_set_to_check_chr_type(bool check);
extern void mtk_pep20_set_is_enable(bool enable);
extern void mtk_pep20_set_is_cable_out_occur(bool out);

extern bool mtk_pep20_get_to_check_chr_type(void);
extern bool mtk_pep20_get_is_connect(void);
extern bool mtk_pep20_get_is_enable(void);
extern int mtk_pep20_reset_ta_vchr_for_temp(void);	// [lidebiao] Charging too long for high temp
/* [lidebiao start] add for fastcharging test */
extern bool mtk_pep20_get_is_connect_for_charging_test(void);
extern int mtk_pep20_check_charger_for_charging_test(void);
/* [lidebiao end] */
#else /* NOT CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT */
static inline int mtk_pep20_init(void)
{
	return -ENOTSUPP;
}

static inline int mtk_pep20_reset_ta_vchr(void)
{
	return -ENOTSUPP;
}

static inline int mtk_pep20_check_charger(void)
{
	return -ENOTSUPP;
}

/* [lidebiao start] add for fastcharging test */
static inline int mtk_pep20_check_charger_for_charging_test(void)
{
	return -ENOTSUPP;
}
/* [lidebiao end] */

static inline int mtk_pep20_start_algorithm(void)
{
	return -ENOTSUPP;
}

static inline int mtk_pep20_set_charging_current(CHR_CURRENT_ENUM *ichg,
	CHR_CURRENT_ENUM *aicr)
{
	return -ENOTSUPP;
}

static inline void mtk_pep20_set_to_check_chr_type(bool check)
{
}

static inline void mtk_pep20_set_is_enable(bool enable)
{
}

static inline void mtk_pep20_set_is_cable_out_occur(bool out)
{
}

static inline bool mtk_pep20_get_to_check_chr_type(void)
{
	return false;
}

static inline bool mtk_pep20_get_is_connect(void)
{
	return false;
}

static inline bool mtk_pep20_get_is_enable(void)
{
	return false;
}

/* [lidebiao start] Charging too long for high temp */
static inline int mtk_pep20_reset_ta_vchr_for_temp(void)
{
	return -ENOTSUPP;
}
/* [lidebiao end] */
/* [lidebiao start] add for fastcharging test */
static inline bool mtk_pep20_get_is_connect_for_charging_test(void)
{
	return false;
}
/* [lidebiao end] */
#endif /* CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT */

#endif /* __MTK_PEP20_INTF_H */

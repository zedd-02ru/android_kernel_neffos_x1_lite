/*
 * Richtek RT9460 Charger GM20
 *
 * Copyright (C) 2017, Richtek Technology Corp.
 * Author: CY Huang <cy_huang@richtek.com>
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* #ifdef CONFIG_RT_REGMAP */
#include "mtk_charger_intf.h"

#include "rt9460.h"

static bool dbg_log_en;
module_param(dbg_log_en, bool, S_IRUGO | S_IWUSR);

struct rt9460_info {
	struct device *dev;
	struct i2c_client *i2c;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_props;
#endif /* #ifdef CONFIG_RT_REGMAP */
	struct mtk_charger_info mchg_info;
	struct mutex io_lock;
	int irq;
	u8 irq_mask[RT9460_IRQ_REGNUM];
	u8 chip_rev;
	bool err_state;
};

static const struct rt9460_platform_data rt9460_def_platform_data = {
	.chg_name = "primary_charger",
	.ichg = 1250000, /* unit: uA */
	.aicr = 500000, /* unit: uA */
	.mivr = 4500000, /* unit: uV */
	.ieoc = 200000, /* unit: uA */
	.voreg = 4350000, /* unit : uV */
	.wt_fc = 0x4,
	.wt_prc = 0x0,
	.twdt = 0x3,
	.eoc_timer = 0x0,
	.intr_gpio = -1,
};

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9460_REG_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_DEVID, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_IRQ1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_IRQ2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_IRQ3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_MASK1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_MASK2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_MASK3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRLDPDM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL10, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL11, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL12, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_CTRL13, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_STATIRQ, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_STATMASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9460_REG_HIDDEN1, 1, RT_VOLATILE, {});

static const rt_register_map_t rt9460_register_map[] = {
	RT_REG(RT9460_REG_CTRL1),
	RT_REG(RT9460_REG_CTRL2),
	RT_REG(RT9460_REG_CTRL3),
	RT_REG(RT9460_REG_DEVID),
	RT_REG(RT9460_REG_CTRL4),
	RT_REG(RT9460_REG_CTRL5),
	RT_REG(RT9460_REG_CTRL6),
	RT_REG(RT9460_REG_CTRL7),
	RT_REG(RT9460_REG_IRQ1),
	RT_REG(RT9460_REG_IRQ2),
	RT_REG(RT9460_REG_IRQ3),
	RT_REG(RT9460_REG_MASK1),
	RT_REG(RT9460_REG_MASK2),
	RT_REG(RT9460_REG_MASK3),
	RT_REG(RT9460_REG_CTRLDPDM),
	RT_REG(RT9460_REG_CTRL8),
	RT_REG(RT9460_REG_CTRL9),
	RT_REG(RT9460_REG_CTRL10),
	RT_REG(RT9460_REG_CTRL11),
	RT_REG(RT9460_REG_CTRL12),
	RT_REG(RT9460_REG_CTRL13),
	RT_REG(RT9460_REG_STATIRQ),
	RT_REG(RT9460_REG_STATMASK),
	RT_REG(RT9460_REG_HIDDEN1),
};

#define RT9460_REGISTER_NUM ARRAY_SIZE(rt9460_register_map)

static const struct rt_regmap_properties rt9460_regmap_props = {
	.aliases = "rt9460",
	.register_num = RT9460_REGISTER_NUM,
	.rm = rt9460_register_map,
	.rt_regmap_mode = RT_SINGLE_BYTE,
};

static int rt9460_io_write(void *i2c, u32 addr, int len, const void *src)
{
	return i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
}

static int rt9460_io_read(void *i2c, u32 addr, int len, void *dst)
{
	return i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
}

static struct rt_regmap_fops rt9460_regmap_fops = {
	.read_device = rt9460_io_read,
	.write_device = rt9460_io_write,
};
#else
#define rt9460_io_write(i2c, addr, len, src) \
	i2c_smbus_write_i2c_block_data(i2c, addr, len, src)
#define rt9460_io_read(i2c, addr, len, dst) \
	i2c_smbus_read_i2c_block_data(i2c, addr, len, dst)
#endif /* #ifdef CONFIG_RT_REGMAP */

/* common i2c transfer function ++ */
static int rt9460_reg_write(struct rt9460_info *ri, u8 reg, u8 data)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};

	return rt_regmap_reg_write(ri->regmap_dev, &rrd, reg, data);
#else
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt9460_io_write(ri->i2c, reg, 1, &data);
	mutex_unlock(&ri->io_lock);
	return 0;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

static int rt9460_reg_block_write(struct rt9460_info *ri, u8 reg,
				  int len, const void *src)
{
#ifdef CONFIG_RT_REGMAP
	return rt_regmap_block_write(ri->regmap_dev, reg, len, src);
#else
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt9460_io_write(ri->i2c, reg, len, src);
	mutex_unlock(&ri->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

static int rt9460_reg_read(struct rt9460_info *ri, u8 reg)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};
	int ret = 0;

	ret = rt_regmap_reg_read(ri->regmap_dev, &rrd, reg);
	return (ret < 0 ? ret : rrd.rt_data.data_u32);
#else
	u8 data = 0;
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt9460_io_read(ri->i2c, reg, 1, &data);
	mutex_unlock(&ri->io_lock);
	return (ret < 0) ? ret : data;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

static int rt9460_reg_block_read(struct rt9460_info *ri, u8 reg,
				 int len, void *dst)
{
#ifdef CONFIG_RT_REGMAP
	return rt_regmap_block_read(ri->regmap_dev, reg, len, dst);
#else
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt9460_io_read(ri->i2c, reg, len, dst);
	mutex_unlock(&ri->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

static int rt9460_reg_assign_bits(struct rt9460_info *ri,
				  u8 reg, u8 mask, u8 data)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};

	return rt_regmap_update_bits(ri->regmap_dev, &rrd, reg, mask, data);
#else
	u8 orig_data = 0;
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt9460_io_read(ri->i2c, reg, 1, &orig_data);
	if (ret < 0)
		goto out_unlock;
	orig_data &= (~mask);
	orig_data |= (data & mask);
	ret = rt9460_io_write(ri->i2c, reg, 1, &orig_data);
out_unlock:
	mutex_unlock(&ri->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

#define rt9460_reg_set_bits(ri, reg, mask) \
	rt9460_reg_assign_bits(ri, reg, mask, mask)
#define rt9460_reg_clr_bits(ri, reg, mask) \
	rt9460_reg_assign_bits(ri, reg, mask, 0)
/* common i2c transfer function -- */


/* ================== */
/* Internal Functions */
/* ================== */
static inline int rt9460_set_wt_fc(struct rt9460_info *ri, u32 sel)
{
	u8 reg_data = 0;

	if (sel > RT9460_WTFC_MAX)
		reg_data = RT9460_WTFC_MAX;
	else
		reg_data = sel;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL10, RT9460_WTFC_MASK,
				      reg_data << RT9460_WTFC_SHFT);
}

static inline int rt9460_set_wt_prc(struct rt9460_info *ri, u32 sel)
{
	u8 reg_data = 0;

	if (sel > RT9460_WTPRC_MAX)
		reg_data = RT9460_WTPRC_MAX;
	else
		reg_data = sel;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL10, RT9460_WTPRC_MASK,
				      reg_data << RT9460_WTPRC_SHFT);
}

static inline int rt9460_set_twdt(struct rt9460_info *ri, u32 sel)
{
	u8 reg_data = 0;

	if (sel > RT9460_TWDT_MAX)
		reg_data = RT9460_TWDT_MAX;
	else
		reg_data = sel;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL13, RT9460_TWDT_MASK,
				      reg_data << RT9460_TWDT_SHFT);
}

static inline int rt9460_set_eoc_timer(struct rt9460_info *ri, u32 sel)
{
	u8 reg_data = 0;

	if (sel > RT9460_EOCTIMER_MAX)
		reg_data = RT9460_EOCTIMER_MAX;
	else
		reg_data = sel;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL12,
				      RT9460_EOCTIMER_MASK,
				      reg_data << RT9460_EOCTIMER_SHFT);
}

static inline int rt9460_set_te_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL2, RT9460_TEEN_MASK,
				      enable ? 0xff : 0);
}

static inline int rt9460_set_mivr_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL9, RT9460_MIVRDIS_MASK,
				      enable ? 0 : 0xff);
}

static inline int rt9460_set_ccjeita_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL7,
				      RT9460_CCJEITAEN_MASK, enable ? 0xff : 0);
}

static inline int rt9460_set_batd_enable(struct rt9460_info *ri, u8 enable)
{ return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL7, RT9460_BATDEN_MASK,
				      enable ? 0xff : 0);
}

static inline int rt9460_set_irq_pulse_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL12,
				      RT9460_IRQPULSE_MASK, enable ? 0xff : 0);
}

static inline int rt9460_set_wdt_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL13, RT9460_WDTEN_MASK,
				      enable ? 0xff : 0);
}

static inline int rt9460_set_safetimer_enable(struct rt9460_info *ri, u8 enable)
{
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL10,
				      RT9460_TMRPAUSE_MASK, enable ? 0 : 0xff);
}

static int rt9460_charger_get_cv(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *cv = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL3);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_VOREG_MASK;
	reg_data >>= RT9460_VOREG_SHFT;
	if (reg_data > RT9460_VOREG_MAX)
		reg_data = RT9460_VOREG_MAX;
	*cv = (reg_data * 20000) + 3500000;
	return 0;
}

static int rt9460_charger_get_is_otg_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *enable = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL2);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_OPAMODE_MASK) ? 1 : 0;
	return 0;
}

static int rt9460_charger_get_is_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *enable = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_CHIPEN_MASK) ? 1 : 0;
	return 0;
}

static int rt9460_charger_get_is_hiz_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *enable = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL2);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_CHGHZ_MASK) ? 1 : 0;
	return 0;
}

static int rt9460_charger_get_mivr(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *mivr = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL9);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_MIVRLVL_MASK;
	reg_data >>= RT9460_MIVRLVL_SHFT;
	if (reg_data >= 0x04 && reg_data <= RT9460_MIVRLVL_MAX)
		*mivr = (reg_data * 500) + 7000;
	else if (reg_data < 0x04)
		*mivr = (reg_data * 250) + 4000;
	else {
		dev_err(ri->dev, "unknown mivr value\n");
		return -EINVAL;
	}
	return 0;
}

static int rt9460_charger_get_is_mivr_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *enable = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL9);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_MIVRDIS_MASK) ? 0 : 1;
	return 0;
}

static const char * const rt9460_charger_status_str[RT9460_CHGSTAT_MAX + 1] = {
	"ready", "progress", "done", "fault",
};
#define INVALD_STR "invalid"
static int rt9460_charger_get_status_string(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	const char *stat_str = (const char *)data;
	u8 reg_data = 0;
	int ret = 0;

	stat_str = INVALD_STR;
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL1);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_CHGSTAT_MASK;
	reg_data >>= RT9460_CHGSTAT_SHFT;
	stat_str = rt9460_charger_status_str[reg_data];
	return 0;
}

static int rt9460_charger_set_ieoc(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* unit : 10uA */
	u32 ieoc = *(u32 *)data;
	u8 reg_data = 0;

	if (ieoc >= 10000)
		reg_data = (ieoc - 10000) / 5000;
	if (ieoc > RT9460_IEOC_MAX)
		ieoc = RT9460_IEOC_MAX;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL2, RT9460_IEOC_MASK,
				      reg_data << RT9460_IEOC_SHFT);
}

/* ================== */
/* interface Functions */
/* ================== */
static int rt9460_charger_init(struct mtk_charger_info *mchr_info, void *data)
{
	return 0;
}

static int rt9460_charger_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL7, RT9460_CHIPEN_MASK,
				      enable ? 0xff : 0);
}

static int rt9460_charger_set_cv(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* data unit is uV */
	u32 cv = *(u32 *)data;
	u8 reg_data = 0;

	if (cv >= 3500000)
		reg_data = (cv - 3500000) / 20000;
	if (reg_data > RT9460_VOREG_MAX)
		reg_data = RT9460_VOREG_MAX;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL3, RT9460_VOREG_MASK,
				      reg_data << RT9460_VOREG_SHFT);
}

static int rt9460_charger_set_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* data unit is 10uA */
	u32 ichg = *(u32 *)data;
	u8 reg_data = 0;

	if (ichg >= 125000)
		reg_data = (ichg - 125000) / 12500;
	if (reg_data > RT9460_ICHG_MAX)
		reg_data = RT9460_ICHG_MAX;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL6, RT9460_ICHG_MASK,
				      reg_data << RT9460_ICHG_SHFT);
}

static int rt9460_charger_get_ichg(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* data unit is 10uA */
	u32 *ichg = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL6);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_ICHG_MASK;
	reg_data >>= RT9460_ICHG_SHFT;
	if (reg_data > RT9460_ICHG_MAX)
		reg_data = RT9460_ICHG_MAX;
	*ichg = (reg_data * 12500) + 125000;
	return 0;
}

static int rt9460_charger_set_aicr(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* data unit is 10uA */
	u32 aicr = *(u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	if (aicr > 300000) {
		dev_warn(ri->dev, "aicr over 3A, will set to disable state\n");
		reg_data = RT9460_AICR_MAX;
	} else if (aicr >= 60000 && aicr <= 300000) {
		/* from 600mA start, step is 100mA */
		reg_data = aicr / 10000;
	} else if (aicr >= 50000 && aicr < 60000) {
		/* config to 500mA */
		reg_data = 0x04;
	} else if (aicr >= 15000 && aicr < 50000) {
		/* config to 150mA */
		reg_data = 0x02;
	} else {
		/* config to minimum 100mA */
		reg_data = 0x00;
	}
	/* config to only AICR */
	ret = rt9460_reg_assign_bits(ri, RT9460_REG_CTRLDPDM,
				     RT9460_IINLMTSEL_MASK,
				     0x01 << RT9460_IINLMTSEL_SHFT);
	if (ret < 0)
		dev_err(ri->dev, "%s: config iinlmtsel fail\n", __func__);
	/*  bypass otg pin decide */
	ret = rt9460_reg_assign_bits(ri, RT9460_REG_CTRL2, RT9460_IININT_MASK,
				     0xff);
	if (ret < 0)
		dev_err(ri->dev, "%s: config iinint fail\n", __func__);
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL11, RT9460_AICR_MASK,
				      reg_data << RT9460_AICR_SHFT);
}

static int rt9460_charger_get_aicr(struct  mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* data unit is 10uA */
	u32 *aicr = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL11);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_AICR_MASK;
	reg_data >>= RT9460_AICR_SHFT;
	if (reg_data >= RT9460_AICR_MAX)
		*aicr = UINT_MAX;
	else if (reg_data >= 0x06 && reg_data < RT9460_AICR_MAX)
		*aicr = reg_data * 10000;
	else if (reg_data >= 0x04 && reg_data < 0x06)
		*aicr = 50000;
	else if (reg_data >= 0x02 && reg_data < 0x04)
		*aicr = 15000;
	else
		*aicr = 10000;
	return 0;
}

static int rt9460_charger_get_is_chg_done(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 *full_status = (u32 *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL1);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	reg_data &= RT9460_CHGSTAT_MASK;
	reg_data >>= RT9460_CHGSTAT_SHFT;
	*full_status = (reg_data == 0x02 ? KAL_TRUE : KAL_FALSE);
	return 0;
}

static int rt9460_charger_reset_watchdog(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	struct rt9460_platform_data *pdata = dev_get_platdata(ri->dev);

	if (pdata->enable_wdt) {
		/* disable reset then enable again */
		rt9460_set_wdt_enable(ri, 0);
		rt9460_set_wdt_enable(ri, 1);
	}
	return 0;
}

#if 0
static int rt9460_charger_get_chg_type(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);

	/* To-Do */
	dev_dbg(ri->dev, "%s\n", __func__);
	return 0;
}
#endif

static int rt9460_charger_set_mivr(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* unit : mV */
	u32 mivr = *(u32 *)data;
	u8 reg_data = 0;

	if (mivr > 11000)
		reg_data = 0x0b;
	else if (mivr >= 7000 && mivr <= 11000)
		reg_data = (mivr - 7000) / 500;
	else if (mivr >= 5000 && mivr < 7000)
		reg_data = 0x02;
	else if (mivr >= 4000 && mivr < 5000)
		reg_data = (mivr - 4000) / 250;
	else
		reg_data = 0;
	if (reg_data > RT9460_MIVRLVL_MAX)
		reg_data = RT9460_MIVRLVL_MAX;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL9, RT9460_MIVRLVL_MASK,
				      reg_data << RT9460_MIVRLVL_SHFT);
}

static int rt9460_charger_enable_safety_timer(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL10, RT9460_TMRPAUSE_MASK,
				      enable ? 0x00 : 0xff);
}

static int rt9460_charger_get_is_safety_timer_enable(struct mtk_charger_info *mchr_info,
	void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	bool *enable = (bool *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL10);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_TMRPAUSE_MASK) ? false : true;
	return 0;
}

static int rt9460_charger_set_hiz(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL2, RT9460_CHGHZ_MASK,
				      enable ? 0xff : 0x00);
}

static int rt9460_charger_set_boost_oc(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	/* unit : mA */
	u32 curr = *(u32 *)data;
	u8 high_cap = 0;

	if (curr > 500)
		high_cap = 1;
	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL5, RT9460_OTGOC_MASK,
				      high_cap ? 0xff : 0x00);
}

static int rt9460_charger_enable_otg(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL2, RT9460_OPAMODE_MASK,
				      enable ? 0xff : 0x00);
}

static int rt9460_charger_enable_power_path(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL7, RT9460_CHGEN_MASK,
				      enable ? 0xff : 0x00);
}

static int rt9460_charger_get_is_power_path_enable(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	bool *enable = (bool *)data;
	u8 reg_data = 0;
	int ret = 0;

	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0)
		return ret;
	reg_data = (u8)ret;
	*enable = (reg_data & RT9460_CHGEN_MASK) ? true : false;
	return 0;
}

static int rt9460_charger_set_pwrstat_led_en(struct mtk_charger_info *mchr_info,
	void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 enable = *(u32 *)data;

	return rt9460_reg_assign_bits(ri, RT9460_REG_CTRL1, RT9460_STATEN_MASK,
				      enable ? 0xff : 0x00);
}

static int rt9460_charger_set_error_state(struct mtk_charger_info *mchr_info,
	void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);

	ri->err_state = *(bool *)data;
	return rt9460_charger_set_hiz(mchr_info, data);
}

static int rt9460_charger_dump_register(struct mtk_charger_info *mchr_info, void *data)
{
	struct rt9460_info *ri = container_of(mchr_info, struct rt9460_info, mchg_info);
	u32 cv = 0, ichg = 0, aicr = 0, mivr = 0, chg_en = 0, pp_en = 0;
	u32 otg_en = 0, hiz_en = 0, mivr_en = 0;
	const char *stat = NULL;
	int i = 0, ret = 0;

	/* register dump */
	for (i = 0; i <= RT9460_REG_STATMASK && dbg_log_en; i++) {
		/* bypass irq event register */
		if (i >= RT9460_REG_IRQ1 &&  i <= RT9460_REG_IRQ3)
			continue;
		/* unknow register */
		if (i > RT9460_REG_CTRLDPDM  && i < RT9460_REG_CTRL8)
			continue;
		/* unknown register */
		if (i > RT9460_REG_CTRL8 && i < RT9460_REG_CTRL9)
			continue;
		/* bypass stat irq register */
		if (i == RT9460_REG_STATIRQ)
			continue;
		dev_info(ri->dev, "reg[0x%02x] -> 0x%02x\n", i,
		       rt9460_reg_read(ri, i));
	}
	ret = rt9460_charger_get_cv(mchr_info, &cv);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_cv fail\n", __func__);
	ret = rt9460_charger_get_ichg(mchr_info, &ichg);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_ichg fail\n", __func__);
	ret = rt9460_charger_get_aicr(mchr_info, &aicr);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_aicr fail\n", __func__);
	ret = rt9460_charger_get_mivr(mchr_info, &mivr);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_mivr fail\n", __func__);
	ret = rt9460_charger_get_is_enable(mchr_info, &chg_en);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_chg_en fail\n", __func__);
	ret = rt9460_charger_get_is_power_path_enable(mchr_info, &pp_en);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_power_path_en fail\n", __func__);
	ret = rt9460_charger_get_is_otg_enable(mchr_info, &otg_en);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_otg_en fail\n", __func__);
	ret = rt9460_charger_get_is_hiz_enable(mchr_info, &hiz_en);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_hiz_en fail\n", __func__);
	ret = rt9460_charger_get_is_mivr_enable(mchr_info, &mivr_en);
	if (ret < 0)
		dev_err(ri->dev, "%s: get_mivr_en fail\n", __func__);
	ret = rt9460_charger_get_status_string(mchr_info, &stat);
	if (ret < 0)
		dev_err(ri->dev, "%s: get charger status string fail\n", __func__);
	dev_info(ri->dev, "cv(uV) %u, ichg(10uA) %u, aicr(10uA) %u, mivr(mV) %u\n",
		 cv, ichg, aicr, mivr);
	dev_info(ri->dev, "chg_en %u, pp_en %u, otg_en %u, hiz_en %u\n", chg_en,
		 pp_en, otg_en, hiz_en);
	dev_info(ri->dev, "mivr_en %u, status %s, err_state %d\n", mivr_en, stat,
		 ri->err_state);
	return 0;
}

static const mtk_charger_intf rt9460_mchr_intf[CHARGING_CMD_NUMBER] = {
	[CHARGING_CMD_INIT] = rt9460_charger_init,
	[CHARGING_CMD_DUMP_REGISTER] = rt9460_charger_dump_register,
	[CHARGING_CMD_ENABLE] = rt9460_charger_enable,
	[CHARGING_CMD_SET_CV_VOLTAGE] = rt9460_charger_set_cv,
	[CHARGING_CMD_SET_CURRENT] = rt9460_charger_set_ichg,
	[CHARGING_CMD_GET_CURRENT] = rt9460_charger_get_ichg,
	[CHARGING_CMD_SET_INPUT_CURRENT] = rt9460_charger_set_aicr,
	[CHARGING_CMD_GET_INPUT_CURRENT] = rt9460_charger_get_aicr,
	[CHARGING_CMD_GET_CHARGING_STATUS] = rt9460_charger_get_is_chg_done,
	[CHARGING_CMD_RESET_WATCH_DOG_TIMER] = rt9460_charger_reset_watchdog,
	 /* [CHARGING_CMD_GET_CHARGER_TYPE] = rt9460_charger_get_chg_type, */
	[CHARGING_CMD_SET_VINDPM] = rt9460_charger_set_mivr,
	[CHARGING_CMD_ENABLE_SAFETY_TIMER] = rt9460_charger_enable_safety_timer,
	[CHARGING_CMD_GET_IS_SAFETY_TIMER_ENABLE] = rt9460_charger_get_is_safety_timer_enable,
	[CHARGING_CMD_SET_HIZ_SWCHR] = rt9460_charger_set_hiz,
	[CHARGING_CMD_SET_BOOST_CURRENT_LIMIT] = rt9460_charger_set_boost_oc,
	[CHARGING_CMD_ENABLE_OTG] = rt9460_charger_enable_otg,
	[CHARGING_CMD_ENABLE_POWER_PATH] = rt9460_charger_enable_power_path,
	[CHARGING_CMD_GET_IS_POWER_PATH_ENABLE] = rt9460_charger_get_is_power_path_enable,
	[CHARGING_CMD_SET_PWRSTAT_LED_EN] = rt9460_charger_set_pwrstat_led_en,
	[CHARGING_CMD_SET_ERROR_STATE] = rt9460_charger_set_error_state,

	/* The following interfaces are not related to charger
	 * Define in mtk_charger_intf.c
	 */
	[CHARGING_CMD_SW_INIT] = mtk_charger_sw_init,
	[CHARGING_CMD_SET_HV_THRESHOLD] = mtk_charger_set_hv_threshold,
	[CHARGING_CMD_GET_HV_STATUS] = mtk_charger_get_hv_status,
	[CHARGING_CMD_GET_BATTERY_STATUS] = mtk_charger_get_battery_status,
	[CHARGING_CMD_GET_CHARGER_DET_STATUS] = mtk_charger_get_charger_det_status,
	[CHARGING_CMD_GET_CHARGER_TYPE] = mtk_charger_get_charger_type,
	[CHARGING_CMD_GET_IS_PCM_TIMER_TRIGGER] = mtk_charger_get_is_pcm_timer_trigger,
	[CHARGING_CMD_SET_PLATFORM_RESET] = mtk_charger_set_platform_reset,
	[CHARGING_CMD_GET_PLATFORM_BOOT_MODE] = mtk_charger_get_platform_boot_mode,
	[CHARGING_CMD_SET_POWER_OFF] = mtk_charger_set_power_off,
	[CHARGING_CMD_GET_POWER_SOURCE] = mtk_charger_get_power_source,
	[CHARGING_CMD_GET_CSDAC_FALL_FLAG] = mtk_charger_get_csdac_full_flag,
	[CHARGING_CMD_DISO_INIT] = mtk_charger_diso_init,
	[CHARGING_CMD_GET_DISO_STATE] = mtk_charger_get_diso_state,
	[CHARGING_CMD_SET_VBUS_OVP_EN] = mtk_charger_set_vbus_ovp_en,
	[CHARGING_CMD_GET_BIF_VBAT] = mtk_charger_get_bif_vbat,
	[CHARGING_CMD_SET_CHRIND_CK_PDN] = mtk_charger_set_chrind_ck_pdn,
	[CHARGING_CMD_GET_BIF_TBAT] = mtk_charger_get_bif_tbat,
	[CHARGING_CMD_SET_DP] = mtk_charger_set_dp,
};

static struct mtk_charger_info rt9460_charger_info = {
	.mchr_intf = rt9460_mchr_intf,
	.device_id = 0x40,
};

static irqreturn_t rt9460_irq_BATAB_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_SYSUVP_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHTERM_TMRI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_WATCHDOGI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	rt9460_charger_reset_watchdog(&ri->mchg_info, NULL);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_WAKEUPI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_VINOVPI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_TSDI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_SYSWAKEUPI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHTREGI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHTMRI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHRCHGI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHTERMI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_info(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHBATOVI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHBADI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_CHRVPI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_BSTLOWVI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_BSTOLI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_BSTVINOVI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_MIVRI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;

	dev_warn(ri->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_PWR_RDYI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	int ret = 0;

	dev_info(ri->dev, "%s\n", __func__);
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL1);
	if (ret < 0) {
		dev_err(ri->dev, "read reg ctrl1 fail\n");
		goto out_pwrrdy;
	}
	dev_info(ri->dev, "pwr_rdy [%d]\n", (ret & RT9460_PWRRDY_MASK) ? 1 : 0);
out_pwrrdy:
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_TSCOLDI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	int ret = 0;

	dev_warn(ri->dev, "%s\n", __func__);
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0) {
		dev_err(ri->dev, "read reg ctrl7 fail\n");
		goto out_tscold;
	}
	dev_info(ri->dev, "cold [%d]\n", (ret & RT9460_TSCOLD_MASK) ? 1 : 0);
out_tscold:
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_TSCOOLI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	int ret = 0;

	dev_info(ri->dev, "%s\n", __func__);
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0) {
		dev_err(ri->dev, "read reg ctrl7 fail\n");
		goto out_tscool;
	}
	dev_info(ri->dev, "cool [%d]\n", (ret & RT9460_TSCOOL_MASK) ? 1 : 0);
out_tscool:
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_TSWARMI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	int ret = 0;

	dev_info(ri->dev, "%s\n", __func__);
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0) {
		dev_err(ri->dev, "read reg ctrl7 fail\n");
		goto out_tswarm;
	}
	dev_info(ri->dev, "warm [%d]\n", (ret & RT9460_TSWARM_MASK) ? 1 : 0);
out_tswarm:
	return IRQ_HANDLED;
}

static irqreturn_t rt9460_irq_TSHOTI_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	int ret = 0;

	dev_warn(ri->dev, "%s\n", __func__);
	ret = rt9460_reg_read(ri, RT9460_REG_CTRL7);
	if (ret < 0) {
		dev_err(ri->dev, "read reg ctrl7 fail\n");
		goto out_tshot;
	}
	dev_info(ri->dev, "hot [%d]\n", (ret & RT9460_TSHOT_MASK) ? 1 : 0);
out_tshot:
	return IRQ_HANDLED;
}

static const irq_handler_t const rt9460_irq_desc[RT9460_IRQ_MAXNUM] = {
	[RT9460_IRQ_BATAB]	= rt9460_irq_BATAB_handler,
	[RT9460_IRQ_SYSUVP]	= rt9460_irq_SYSUVP_handler,
	[RT9460_IRQ_CHTERM_TMRI]	= rt9460_irq_CHTERM_TMRI_handler,
	[RT9460_IRQ_WATCHDOGI]	= rt9460_irq_WATCHDOGI_handler,
	[RT9460_IRQ_WAKEUPI]	= rt9460_irq_WAKEUPI_handler,
	[RT9460_IRQ_VINOVPI]	= rt9460_irq_VINOVPI_handler,
	[RT9460_IRQ_TSDI]	= rt9460_irq_TSDI_handler,
	[RT9460_IRQ_SYSWAKEUPI]	= rt9460_irq_SYSWAKEUPI_handler,
	[RT9460_IRQ_CHTREGI]	= rt9460_irq_CHTREGI_handler,
	[RT9460_IRQ_CHTMRI]	= rt9460_irq_CHTMRI_handler,
	[RT9460_IRQ_CHRCHGI]	= rt9460_irq_CHRCHGI_handler,
	[RT9460_IRQ_CHTERMI]	= rt9460_irq_CHTERMI_handler,
	[RT9460_IRQ_CHBATOVI]	= rt9460_irq_CHBATOVI_handler,
	[RT9460_IRQ_CHBADI]	= rt9460_irq_CHBADI_handler,
	[RT9460_IRQ_CHRVPI]	= rt9460_irq_CHRVPI_handler,
	[RT9460_IRQ_BSTLOWVI]	= rt9460_irq_BSTLOWVI_handler,
	[RT9460_IRQ_BSTOLI]	= rt9460_irq_BSTOLI_handler,
	[RT9460_IRQ_BSTVINOVI]	= rt9460_irq_BSTVINOVI_handler,
	[RT9460_IRQ_MIVRI]	= rt9460_irq_MIVRI_handler,
	[RT9460_IRQ_PWR_RDYI]	= rt9460_irq_PWR_RDYI_handler,
	[RT9460_IRQ_TSCOLDI]	= rt9460_irq_TSCOLDI_handler,
	[RT9460_IRQ_TSCOOLI]	= rt9460_irq_TSCOOLI_handler,
	[RT9460_IRQ_TSWARMI]	= rt9460_irq_TSWARMI_handler,
	[RT9460_IRQ_TSHOTI]	= rt9460_irq_TSHOTI_handler,
};

static irqreturn_t rt9460_intr_handler(int irq, void *dev_id)
{
	struct rt9460_info *ri = (struct rt9460_info *)dev_id;
	u8 event[RT9460_IRQ_REGNUM] = {0};
	int i, j, id, ret = 0;

	dev_dbg(ri->dev, "%s triggered\n", __func__);
	ret = rt9460_reg_block_read(ri, RT9460_REG_IRQ1, 3, event);
	if (ret < 0) {
		dev_info(ri->dev, "read irq event fail\n");
		goto out_intr_handler;
	}
	ret = rt9460_reg_read(ri, RT9460_REG_STATIRQ);
	if (ret < 0) {
		dev_info(ri->dev, "read stat irq event fail\n");
		goto out_intr_handler;
	}
	event[3] = (u8)ret;
	for (i = 0; i < RT9460_IRQ_REGNUM; i++) {
		event[i] &=  ~(ri->irq_mask[i]);
		if (!event[i])
			continue;
		for (j = 0; j < 8; j++) {
			if (!(event[i] & (1 << j)))
				continue;
			id = i * 8 + j;
			if (!rt9460_irq_desc[id]) {
				dev_warn(ri->dev, "no %d irq_handler", id);
				continue;
			}
			rt9460_irq_desc[id](id, ri);
		}
	}
out_intr_handler:
	/* irq rez workaround ++ */
	ret = rt9460_reg_set_bits(ri, RT9460_REG_CTRL12, RT9460_WKTMREN_MASK);
	if (ret < 0)
		dev_err(ri->dev, "irq: set wktimer fail\n");
	/* wait internal osc to be enabled */
	mdelay(10);
	ret = rt9460_reg_set_bits(ri, RT9460_REG_CTRL12, RT9460_IRQREZ_MASK);
	if (ret < 0)
		dev_err(ri->dev, "irq: set irqrez fail\n");
	ret = rt9460_reg_clr_bits(ri, RT9460_REG_CTRL12, RT9460_WKTMREN_MASK);
	if (ret < 0)
		dev_err(ri->dev, "irq: clr wktimer fail\n");
	/* irq rez workaround -- */
	return IRQ_HANDLED;
}

static int rt9460_chip_irq_init(struct rt9460_info *ri)
{
	struct rt9460_platform_data *pdata = dev_get_platdata(ri->dev);
	int ret = 0;

	ret = devm_gpio_request_one(ri->dev, pdata->intr_gpio, GPIOF_IN,
				    "rt9460_intr_gpio");
	if (ret < 0) {
		dev_err(ri->dev, "request gpio fail\n");
		return ret;
	}
	ri->irq = gpio_to_irq(pdata->intr_gpio);
	ret = devm_request_threaded_irq(ri->dev, ri->irq, NULL,
					rt9460_intr_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					dev_name(ri->dev), ri);
	if (ret < 0) {
		dev_err(ri->dev, "request interrupt fail\n");
		return ret;
	}
	device_init_wakeup(ri->dev, true);
	ri->irq_mask[0] = 0x00;
	ri->irq_mask[1] = 0x00;
	ri->irq_mask[2] = 0x00;
	if (pdata->enable_hwjeita)
		ri->irq_mask[3] = 0x04;
	else
		ri->irq_mask[3] = 0xf4;
	ret = rt9460_reg_block_write(ri, RT9460_REG_MASK1, 3, ri->irq_mask);
	if (ret < 0)
		dev_err(ri->dev, "set irq masks fail\n");
	return rt9460_reg_write(ri, RT9460_REG_STATMASK, ri->irq_mask[3]);
}

static int rt9460_register_mtk_charger(struct rt9460_info *ri)
{
	struct rt9460_platform_data *pdata = dev_get_platdata(ri->dev);

	memcpy(&ri->mchg_info, &rt9460_charger_info,
	       sizeof(struct mtk_charger_info));
	ri->mchg_info.name = pdata->chg_name;
	mtk_charger_set_info(&ri->mchg_info);
	return 0;
}

static int rt9460_register_rt_regmap(struct rt9460_info *ri)
{
#ifdef CONFIG_RT_REGMAP
	ri->regmap_props = devm_kzalloc(ri->dev, sizeof(*ri->regmap_props),
					GFP_KERNEL);
	if (!ri->regmap_props)
		return -ENOMEM;
	memcpy(ri->regmap_props, &rt9460_regmap_props,
	       sizeof(*ri->regmap_props));
	ri->regmap_props->name = kasprintf(GFP_KERNEL, "rt9460.%s",
					  dev_name(ri->dev));
	ri->regmap_dev = rt_regmap_device_register(ri->regmap_props,
				&rt9460_regmap_fops, ri->dev, ri->i2c, ri);
	if (!ri->regmap_dev)
		return -EINVAL;
	return 0;
#else
	return 0;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

static int rt9460_chip_pdata_init(struct rt9460_info *ri)
{
	struct rt9460_platform_data *pdata = dev_get_platdata(ri->dev);
	struct mtk_charger_info *mchr_info = &ri->mchg_info;
	u32 enable = 0;
	int ret = 0;

	/* unit from uV to mV */
	pdata->mivr /= 1000;
	ret = rt9460_charger_set_mivr(mchr_info, &pdata->mivr);
	if (ret < 0) {
		dev_err(ri->dev, "set mivr fail\n");
		return ret;
	}
	/* unit from uA to 10uA */
	pdata->aicr /= 10;
	ret = rt9460_charger_set_aicr(mchr_info, &pdata->aicr);
	if (ret < 0) {
		dev_err(ri->dev, "set aicr fail\n");
		return ret;
	}
	/* unit uV */
	ret = rt9460_charger_set_cv(mchr_info, &pdata->voreg);
	if (ret < 0) {
		dev_err(ri->dev, "set voreg fail\n");
		return ret;
	}
	/* unit from uA to 10uA */
	pdata->ichg /= 10;
	ret = rt9460_charger_set_ichg(mchr_info, &pdata->ichg);
	if (ret < 0) {
		dev_err(ri->dev, "set ichg fail\n");
		return ret;
	}
	/* unit from uA to 10uA */
	pdata->ieoc /= 10;
	ret = rt9460_charger_set_ieoc(mchr_info, &pdata->ieoc);
	if (ret < 0) {
		dev_err(ri->dev, "set ieoc fail\n");
		return ret;
	}
	ret = rt9460_set_wt_fc(ri, pdata->wt_fc);
	if (ret < 0) {
		dev_err(ri->dev, "set wt_fc fail\n");
		return ret;
	}
	ret = rt9460_set_wt_prc(ri, pdata->wt_prc);
	if (ret < 0) {
		dev_err(ri->dev, "set wt_prc fail\n");
		return ret;
	}
	ret = rt9460_set_twdt(ri, pdata->twdt);
	if (ret < 0) {
		dev_err(ri->dev, "set twdt fail\n");
		return ret;
	}
	ret = rt9460_set_eoc_timer(ri, pdata->eoc_timer);
	if (ret < 0) {
		dev_err(ri->dev, "set eoc_timer fail\n");
		return ret;
	}
	ret = rt9460_set_te_enable(ri, pdata->enable_te);
	if (ret < 0) {
		dev_err(ri->dev, "set te enable fail\n");
		return ret;
	}
	ret = rt9460_set_mivr_enable(ri, (pdata->disable_mivr ? 0 : 1));
	if (ret < 0) {
		dev_err(ri->dev, "set mivr enable fail\n");
		return ret;
	}
	ret = rt9460_set_ccjeita_enable(ri, pdata->enable_ccjeita);
	if (ret < 0) {
		dev_err(ri->dev, "set ccjeita enable fail\n");
		return ret;
	}
	ret = rt9460_set_batd_enable(ri, pdata->enable_batd);
	if (ret < 0) {
		dev_err(ri->dev, "set batd enable fail\n");
		return ret;
	}
	ret = rt9460_set_irq_pulse_enable(ri, pdata->enable_irq_pulse);
	if (ret < 0) {
		dev_err(ri->dev, "set irq_pulse enable fail\n");
		return ret;
	}
	ret = rt9460_set_wdt_enable(ri, pdata->enable_wdt);
	if (ret < 0) {
		dev_err(ri->dev, "set wdt enable fail\n");
		return ret;
	}
	ret = rt9460_set_safetimer_enable(ri, pdata->enable_safetimer);
	if (ret < 0) {
		dev_err(ri->dev, "set safetimer enable fail\n");
		return ret;
	}
	enable = 0;
	if (!strcmp(pdata->chg_name, "secondary_charger"))
		rt9460_charger_enable(mchr_info, &enable);
	return ret;
}

static int rt9460_chip_reset(struct i2c_client *i2c)
{
	u8 tmp[RT9460_IRQ_REGNUM] = {0};
	u8 reg_data = 0;
	int ret = 0;

	dev_dbg(&i2c->dev, "%s\n", __func__);
	/* [zhangdong] bypass the return value when writing chip reset */
	i2c_smbus_write_byte_data(i2c, RT9460_REG_CTRL4, 0x80);
	mdelay(20);
	/* fix ppsense icc acuracy ++ */
	ret = i2c_smbus_read_byte_data(i2c, RT9460_REG_CTRL8);
	if (ret < 0) {
		dev_err(&i2c->dev, "read ctrl8 fail\n");
		return ret;
	}
	reg_data = (u8)ret;
	reg_data |= 0x7;
	ret = i2c_smbus_write_byte_data(i2c, RT9460_REG_CTRL8, reg_data);
	if (ret < 0) {
		dev_err(&i2c->dev, "write ctrl8 fail\n");
		return ret;
	}
	/* fix ppsense icc acuracy -- */

	/* [zhangdong start] config vprec to 3.0v */
	ret = i2c_smbus_read_byte_data(i2c, RT9460_REG_CTRL6);
	if (ret < 0) {
		dev_err(&i2c->dev, "read ctrl6 fail\n");
		return ret;
	}
	reg_data = (u8)ret;
	reg_data |= 0x5;
	ret = i2c_smbus_write_byte_data(i2c, RT9460_REG_CTRL6, reg_data);
	if (ret < 0) {
		dev_err(&i2c->dev, "write ctrl6 fail\n");
		return ret;
	}
	/* [zhangdong end] */

	/* config internal vdda to 4.4v workaround ++ */
	ret = i2c_smbus_read_byte_data(i2c, RT9460_REG_HIDDEN1);
	if (ret < 0) {
		dev_err(&i2c->dev, "read hidden1 fail\n");
		return ret;
	}
	reg_data = (u8)ret;
	reg_data |= 0x80;
	ret = i2c_smbus_write_byte_data(i2c, RT9460_REG_HIDDEN1, reg_data);
	if (ret < 0) {
		dev_err(&i2c->dev, "write hidden1 fail\n");
		return ret;
	}
	/* config internal vdda to 4.4v workaround -- */
	/* default disable safety timer */
	ret = i2c_smbus_write_byte_data(i2c, RT9460_REG_CTRL10, 0x01);
	if (ret < 0) {
		dev_err(&i2c->dev, "default disable timer fail\n");
		return ret;
	}
	/* mask all irq events and read status to be cleared */
	memset(tmp, 0xff, RT9460_IRQ_REGNUM);
	ret = i2c_smbus_write_i2c_block_data(i2c, RT9460_REG_MASK1, 3, tmp);
	if (ret < 0) {
		dev_err(&i2c->dev, "set all irq masked fail\n");
		return ret;
	}
	ret = i2c_smbus_write_byte_data(i2c, RT9460_REG_STATMASK, tmp[3]);
	if (ret < 0) {
		dev_err(&i2c->dev, "set stat irq masked fail\n");
		return ret;
	}
	ret = i2c_smbus_read_i2c_block_data(i2c, RT9460_REG_IRQ1, 3, tmp);
	if (ret < 0) {
		dev_err(&i2c->dev, "read all irqevents fail\n");
		return ret;
	}
	ret = i2c_smbus_read_byte_data(i2c, RT9460_REG_STATIRQ);
	if (ret < 0) {
		dev_err(&i2c->dev, "read stat irqevent fail\n");
		return ret;
	}
	tmp[3] = (u8)ret;
	return 0;
}

static inline int rt9460_i2c_detect_devid(struct i2c_client *i2c)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(i2c, RT9460_REG_DEVID);
	if (ret < 0) {
		dev_info(&i2c->dev, "%s: chip io may bail\n", __func__);
		return ret;
	}
	dev_dbg(&i2c->dev, "%s: dev_id 0x%02x\n", __func__, ret);
	if ((ret & 0xf0) != 0x40) {
		dev_err(&i2c->dev, "%s: vendor id not correct\n", __func__);
		return -ENODEV;
	}
	/* finally return the rev id */
	return (ret & 0x0f);
}

static void rt_parse_dt(struct device *dev, struct rt9460_platform_data *pdata)
{
	/* just used to prevent the null parameter */
	if (!dev || !pdata)
		return;
	if (of_property_read_string(dev->of_node, "charger_name",
				    &pdata->chg_name) < 0)
		dev_warn(dev, "not specified chg_name\n");
	if (of_property_read_u32(dev->of_node, "ichg", &pdata->ichg) < 0)
		dev_warn(dev, "not specified ichg value\n");
	if (of_property_read_u32(dev->of_node, "aicr", &pdata->aicr) < 0)
		dev_warn(dev, "not specified aicr value\n");
	if (of_property_read_u32(dev->of_node, "mivr", &pdata->mivr) < 0)
		dev_warn(dev, "not specified mivr value\n");
	if (of_property_read_u32(dev->of_node, "ieoc", &pdata->ieoc) < 0)
		dev_warn(dev, "not specified ieoc_value\n");
	if (of_property_read_u32(dev->of_node, "cv", &pdata->voreg) < 0)
		dev_warn(dev, "not specified cv value\n");
	if (of_property_read_u32(dev->of_node, "wt_fc", &pdata->wt_fc) < 0)
		dev_warn(dev, "not specified wt_fc value\n");
	if (of_property_read_u32(dev->of_node, "wt_prc", &pdata->wt_prc) < 0)
		dev_warn(dev, "not specified wt_prc value\n");
	if (of_property_read_u32(dev->of_node, "twdt", &pdata->twdt) < 0)
		dev_dbg(dev, "not specified twdt value\n");
	if (of_property_read_u32(dev->of_node,
				 "eoc_timer", &pdata->eoc_timer) < 0)
		dev_dbg(dev, "not specified eoc_timer value\n");
	pdata->enable_te = of_property_read_bool(dev->of_node, "enable_te");
	pdata->disable_mivr = of_property_read_bool(dev->of_node, "disable_mivr");
	pdata->enable_ccjeita = of_property_read_bool(dev->of_node,
						       "enable_ccjeita");
	pdata->enable_hwjeita = of_property_read_bool(dev->of_node,
						      "enable_hwjeita");
	pdata->enable_batd = of_property_read_bool(dev->of_node, "enable_batd");
	pdata->enable_irq_pulse = of_property_read_bool(dev->of_node,
							"enable_irq_pulse");
	pdata->enable_wdt = of_property_read_bool(dev->of_node, "enable_wdt");
	pdata->enable_safetimer = of_property_read_bool(dev->of_node,
							"enable_safetimer");
#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	pdata->intr_gpio = of_get_named_gpio(dev->of_node, "rt,intr_gpio", 0);
#else
	if (of_property_read_u32(dev->of_node, "rt,intr_gpio_num", &pdata->intr_gpio) < 0)
		dev_warn(dev, "not specified irq gpio number\n");
#endif /* #if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND) */
}

static int rt9460_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct rt9460_platform_data *pdata = dev_get_platdata(&i2c->dev);
	struct rt9460_info *ri = NULL;
	u8 rev_id = 0;
	bool use_dt = i2c->dev.of_node;
	int ret = 0;

	dev_info(&i2c->dev, "%s start\n", __func__);
	ret = rt9460_i2c_detect_devid(i2c);
	if (ret < 0)
		return ret;
	/* if success, return value is revision id */
	rev_id = (u8)ret;
	/* driver data */
	ri = devm_kzalloc(&i2c->dev, sizeof(*ri), GFP_KERNEL);
	if (!ri)
		return -ENOMEM;
	/* platform data */
	if (use_dt) {
		pdata = devm_kzalloc(&i2c->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;
		memcpy(pdata, &rt9460_def_platform_data, sizeof(*pdata));
		i2c->dev.platform_data = pdata;
		rt_parse_dt(&i2c->dev, pdata);
	} else {
		if (!pdata) {
			dev_info(&i2c->dev, "no pdata specify\n");
			return -EINVAL;
		}
	}
	ri->dev = &i2c->dev;
	ri->i2c = i2c;
	ri->i2c->timing = 400;        /* [zhangdong] Config i2c speed */
	mutex_init(&ri->io_lock);
	ri->chip_rev = rev_id;
	memset(ri->irq_mask, 0xff, RT9460_IRQ_REGNUM);
	i2c_set_clientdata(i2c, ri);
	/* do whole chip reset */
	ret = rt9460_chip_reset(i2c);
	if (ret < 0)
		return ret;
	/* rt-regmap register */
	ret = rt9460_register_rt_regmap(ri);
	if (ret < 0)
		return ret;
	ret = rt9460_chip_pdata_init(ri);
	if (ret < 0)
		return ret;
	ret = rt9460_register_mtk_charger(ri);
	if (ret < 0)
		return ret;
	ret = rt9460_chip_irq_init(ri);
	if (ret < 0)
		return ret;
	dev_info(ri->dev, "%s end\n", __func__);
	return 0;
}

static int rt9460_i2c_remove(struct i2c_client *i2c)
{
	struct rt9460_info *ri = i2c_get_clientdata(i2c);

	dev_info(ri->dev, "%s start\n", __func__);
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(ri->regmap_dev);
#endif /* #ifdef CONFIG_RT_REGMAP */
	dev_info(ri->dev, "%s end\n", __func__);
	return 0;
}

static void rt9460_i2c_shutdown(struct i2c_client *i2c)
{
	struct rt9460_info *ri = i2c_get_clientdata(i2c);

	disable_irq(ri->irq);
	rt9460_chip_reset(i2c);
}

static int rt9460_i2c_suspend(struct device *dev)
{
	struct rt9460_info *ri = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(ri->irq);
	return 0;
}

static int rt9460_i2c_resume(struct device *dev)
{
	struct rt9460_info *ri = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(ri->irq);
	return 0;
}

static SIMPLE_DEV_PM_OPS(rt9460_pm_ops, rt9460_i2c_suspend, rt9460_i2c_resume);

static const struct of_device_id of_id_table[] = {
	{ .compatible = "richtek,rt9460"},
	{},
};
MODULE_DEVICE_TABLE(of, of_id_table);

static const struct i2c_device_id i2c_id_table[] = {
	{ "rt9460", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, i2c_id_table);

static struct i2c_driver rt9460_i2c_driver = {
	.driver = {
		.name = "rt9460",
		.owner = THIS_MODULE,
		.pm = &rt9460_pm_ops,
		.of_match_table = of_match_ptr(of_id_table),
	},
	.probe = rt9460_i2c_probe,
	.remove = rt9460_i2c_remove,
	.shutdown = rt9460_i2c_shutdown,
	.id_table = i2c_id_table,
};
module_i2c_driver(rt9460_i2c_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Richtek RT9460 Charger driver");
MODULE_AUTHOR("CY Huang <cy_huang@richtek.com>");
MODULE_VERSION("1.0.0");

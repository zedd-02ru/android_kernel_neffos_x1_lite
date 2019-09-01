/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>

#include "inc/rt5081a_pmu.h"

static bool dbg_log_en; /* module param to enable/disable debug log */
module_param(dbg_log_en, bool, S_IRUGO | S_IWUSR);

static int rt5081a_pmu_read_device(void *i2c, u32 addr, int len, void *dst)
{
#ifdef CONFIG_MTK_I2C_EXTENSION
	struct i2c_client *client = (struct i2c_client *)i2c;
	struct rt5081a_pmu_chip *chip = i2c_get_clientdata(client);
	struct i2c_adapter *adap = client->adapter;
	u8 command = (u8)addr;
	struct i2c_msg msg[2];
	int ret = 0;

	memset(msg, 0, sizeof(struct i2c_msg) * 2);
	/* first message will be the register address */
	msg[0].addr = (client->addr & I2C_MASK_FLAG);
	msg[0].flags = 0;
	msg[0].ext_flag = client->ext_flag;
	msg[0].len = 1;
	msg[0].timing = client->timing;
	msg[0].buf = &command;
	msg[1].addr = (client->addr & I2C_MASK_FLAG);
	msg[1].ext_flag = client->ext_flag;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].timing = client->timing;
	if (len > 1) {
		msg[1].ext_flag |= I2C_DMA_FLAG;
		msg[1].buf = (u8 *)(uintptr_t)chip->rt_DMABuf_pa;
		ret = i2c_transfer(adap, msg, ARRAY_SIZE(msg));
	} else
		ret = i2c_smbus_read_byte_data(i2c, addr);
	if (ret < 0)
		return ret;
	if (len > 1)
		memcpy(dst, chip->rt_DMABuf_va, len);
	else
		*(u8 *)dst = (u8)ret;

	return 0;
#else
	return i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
#endif /* #ifdef CONFIG_MTK_I2C_EXTENSION */
}

static int rt5081a_pmu_write_device(void *i2c, u32 addr, int len,
				   const void *src)
{
#ifdef CONFIG_MTK_I2C_EXTENSION
	struct i2c_client *client = (struct i2c_client *)i2c;
	struct rt5081a_pmu_chip *chip = i2c_get_clientdata(client);
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg = {0};
	int ret = 0;

	msg.addr = (client->addr & I2C_MASK_FLAG);
	msg.flags = 0;
	msg.ext_flag = client->ext_flag;
	msg.len = len + 1;
	msg.timing = client->timing;
	if (len > 1) {
		chip->rt_DMABuf_va[0] = (u8)addr;
		memcpy(chip->rt_DMABuf_va + 1, src, len);
		msg.addr |= I2C_DMA_FLAG;
		msg.buf = (u8 *)(uintptr_t)chip->rt_DMABuf_pa;
		ret = i2c_transfer(adap, &msg, 1);
	} else
		ret = i2c_smbus_write_byte_data(i2c, addr, *(u8 *)src);
	if (ret < 0)
		pr_err("%s fail, ret = %d\n", __func__, ret);
	return ret;
#else
	return i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
#endif /* #ifdef CONFIG_MTK_I2C_EXTENSION */
}

static struct rt_regmap_fops rt5081a_regmap_fops = {
	.read_device = rt5081a_pmu_read_device,
	.write_device = rt5081a_pmu_write_device,
};

int rt5081a_pmu_reg_read(struct rt5081a_pmu_chip *chip, u8 addr)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x\n", __func__, addr);
	rt_mutex_lock(&chip->io_lock);
	ret = rt_regmap_reg_read(chip->rd, &rrd, addr);
	rt_mutex_unlock(&chip->io_lock);
	return (ret < 0 ? ret : rrd.rt_data.data_u32);
#else
	u8 data = 0;
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x\n", __func__, addr);
	rt_mutex_lock(&chip->io_lock);
	ret = rt5081a_pmu_read_device(chip->i2c, addr, 1, &data);
	rt_mutex_unlock(&chip->io_lock);
	return (ret < 0 ? ret : data);
#endif /* #ifdef CONFIG_RT_REGMAP */
}
EXPORT_SYMBOL(rt5081a_pmu_reg_read);

int rt5081a_pmu_reg_write(struct rt5081a_pmu_chip *chip, u8 addr, u8 data)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x data %02x\n", __func__,
		addr, data);
	rt_mutex_lock(&chip->io_lock);
	ret = rt_regmap_reg_write(chip->rd, &rrd, addr, data);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#else
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x data %02x\n", __func__,
		addr, data);
	rt_mutex_lock(&chip->io_lock);
	ret = rt5081a_pmu_write_device(chip->i2c, addr, 1, &data);
	rt_mutex_unlock(&chip->io_lock);
	return (ret < 0 ? ret : data);
#endif /* #ifdef CONFIG_RT_REGMAP */
}
EXPORT_SYMBOL(rt5081a_pmu_reg_write);

int rt5081a_pmu_reg_update_bits(struct rt5081a_pmu_chip *chip, u8 addr,
			       u8 mask, u8 data)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x data %02x\n", __func__,
		addr, data);
	rt_dbg(chip->dev, "%s: mask %02x\n", __func__, mask);
	rt_mutex_lock(&chip->io_lock);
	ret = rt_regmap_update_bits(chip->rd, &rrd, addr, mask, data);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#else
	u8 orig = 0;
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x data %02x\n", __func__,
		addr, data);
	rt_dbg(chip->dev, "%s: mask %02x\n", __func__, mask);
	rt_mutex_lock(&chip->io_lock);
	ret = rt5081a_pmu_read_device(chip->i2c, addr, 1, &orig);
	if (ret < 0)
		goto out_update_bits;
	orig &= ~mask;
	orig |= (data & mask);
	ret = rt5081a_pmu_write_device(chip->i2c, addr, 1, &orig);
out_update_bits:
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}
EXPORT_SYMBOL(rt5081a_pmu_reg_update_bits);

int rt5081a_pmu_reg_block_read(struct rt5081a_pmu_chip *chip, u8 addr,
			      int len, u8 *dest)
{
#ifdef CONFIG_RT_REGMAP
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x size %d\n", __func__,
		addr, len);
	rt_mutex_lock(&chip->io_lock);
	ret = rt_regmap_block_read(chip->rd, addr, len, dest);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#else
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x size %d\n", __func__,
		addr, len);
	rt_mutex_lock(&chip->io_lock);
	ret = rt5081a_pmu_read_device(chip->i2c, addr, len, dest);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}
EXPORT_SYMBOL(rt5081a_pmu_reg_block_read);

int rt5081a_pmu_reg_block_write(struct rt5081a_pmu_chip *chip, u8 addr,
			       int len, const u8 *src)
{
#ifdef CONFIG_RT_REGMAP
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x size %d\n", __func__, addr,
		len);
	rt_mutex_lock(&chip->io_lock);
	ret = rt_regmap_block_write(chip->rd, addr, len, src);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#else
	int ret = 0;

	rt_dbg(chip->dev, "%s: reg %02x size %d\n", __func__, addr,
		len);
	rt_mutex_lock(&chip->io_lock);
	ret = rt5081a_pmu_write_device(chip->i2c, addr, len, src);
	rt_mutex_unlock(&chip->io_lock);
	return ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}
EXPORT_SYMBOL(rt5081a_pmu_reg_block_write);

static int rt_parse_dt(struct device *dev,
		       struct rt5081a_pmu_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "rt,intr_gpio", 0);
	if (ret < 0)
		goto out_parse_dt;
	pdata->intr_gpio = ret;
#else
	ret =  of_property_read_u32(np, "rt,intr_gpio_num", &pdata->intr_gpio);
	if (ret < 0)
		goto out_parse_dt;
#endif
	return 0;
out_parse_dt:
	return ret;
}

static inline void rt_config_of_node(struct device *dev)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "rt5081a_pmu_dts");
	if (np) {
		dev_err(dev, "find rt5081a_pmu_dts node\n");
		dev->of_node = np;
	}
}

static inline int rt5081a_pmu_chip_id_check(struct i2c_client *i2c)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(i2c, RT5081A_PMU_REG_DEVINFO);
	if (ret < 0)
		return ret;
	if ((ret & 0xF0) == 0xA0)
		return (ret & 0x0F);
	return -ENODEV;
}

static int rt5081a_pmu_suspend(struct device *dev)
{
	struct rt5081a_pmu_chip *chip = dev_get_drvdata(dev);

	rt_dbg(chip->dev, "%s\n", __func__);
	rt5081a_pmu_irq_suspend(chip);
	return 0;
}

static int rt5081a_pmu_resume(struct device *dev)
{
	struct rt5081a_pmu_chip *chip = dev_get_drvdata(dev);

	rt_dbg(dev, "%s\n", __func__);
	rt5081a_pmu_irq_resume(chip);
	return 0;
}

static SIMPLE_DEV_PM_OPS(rt5081a_pmu_pm_ops, rt5081a_pmu_suspend,
	rt5081a_pmu_resume);

static int rt5081a_pmu_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct rt5081a_pmu_chip *chip;
	struct rt5081a_pmu_platform_data *pdata = dev_get_platdata(&i2c->dev);
	bool use_dt = i2c->dev.of_node;
	uint8_t chip_rev = 0;
	int ret = 0;

	ret = rt5081a_pmu_chip_id_check(i2c);
	if (ret < 0)
		return ret;
	chip_rev = ret;
	if (use_dt) {
		rt_config_of_node(&i2c->dev);
		pdata = devm_kzalloc(&i2c->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;
		ret = rt_parse_dt(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "error parse platform data\n");
			devm_kfree(&i2c->dev, pdata);
			return ret;
		}
		i2c->dev.platform_data = pdata;
	} else {
		if (!pdata)
			return -EINVAL;
	}
	chip = devm_kzalloc(&i2c->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	chip->i2c = i2c;
	chip->dev = &i2c->dev;
	chip->chip_rev = chip_rev;
	rt_mutex_init(&chip->io_lock);
	i2c_set_clientdata(i2c, chip);

	pm_runtime_set_active(&i2c->dev);
	ret = rt5081a_pmu_regmap_register(chip, &rt5081a_regmap_fops);
	if (ret < 0)
		goto out_regmap;
	ret = rt5081a_pmu_irq_register(chip);
	if (ret < 0)
		goto out_irq;
	ret = rt5081a_pmu_subdevs_register(chip);
	if (ret < 0)
		goto out_subdevs;
	pm_runtime_enable(&i2c->dev);
	dev_info(&i2c->dev, "%s successfully\n", __func__);
	return 0;
out_subdevs:
	rt5081a_pmu_irq_unregister(chip);
out_irq:
	rt5081a_pmu_regmap_unregister(chip);
out_regmap:
	pm_runtime_set_suspended(&i2c->dev);
	devm_kfree(&i2c->dev, chip);
	if (use_dt)
		devm_kfree(&i2c->dev, pdata);
	return ret;
}

static int rt5081a_pmu_remove(struct i2c_client *i2c)
{
	struct rt5081a_pmu_chip *chip = i2c_get_clientdata(i2c);

	pm_runtime_disable(&i2c->dev);
	rt5081a_pmu_subdevs_unregister(chip);
	rt5081a_pmu_irq_unregister(chip);
	rt5081a_pmu_regmap_unregister(chip);
	pm_runtime_set_suspended(&i2c->dev);
	dev_info(chip->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct i2c_device_id rt5081a_pmu_id_table[] = {
	{"rt5081a_pmu", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt5081a_pmu_id_table);

static const struct of_device_id rt5081a_pmu_ofid_table[] = {
	{.compatible = "mediatek,rt5081a_pmu",},
	{},
};
MODULE_DEVICE_TABLE(of, rt5081a_pmu_ofid_table);

static struct i2c_driver rt5081a_pmu = {
	.driver = {
		.name = "rt5081a_pmu",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt5081a_pmu_ofid_table),
		.pm = &rt5081a_pmu_pm_ops,
	},
	.probe = rt5081a_pmu_probe,
	.remove = rt5081a_pmu_remove,
	.id_table = rt5081a_pmu_id_table,
};

module_i2c_driver(rt5081a_pmu);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("cy_huang <cy_huang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081A PMU");
MODULE_VERSION("1.0.2_G");

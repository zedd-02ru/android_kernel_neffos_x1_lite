/*
 *
 * Copyright (C) 2016 TPLink.
 *
 * Author: wuzehui<wuzehui_w8802@tplink.com.cn>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/pinctrl/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/i2c-dev.h>

#define I2C_MASTER_CLOCK  400
static struct pinctrl *tfa_pinctrl;
static struct pinctrl_state *tfa_reset;
static struct pinctrl_state *tfa_enable;

static struct i2c_client *new_client;

static u8 *TfaI2CDMABuf_va = NULL;
static dma_addr_t TfaI2CDMABuf_pa;

static int tfa_nxpspk_open(struct inode *inode, struct file *fp);
static long tfa_nxpspk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
static ssize_t tfa_nxpspk_read(struct file *fp,	char __user *data, size_t count, loff_t *offset);
static ssize_t tfa_nxpspk_write(struct file *fp, const char __user *data, size_t count, loff_t *offset);


/*****************************************************************************
 * FILE OPERATION FUNCTION
 *  AudDrv_nxpspk_ioctl
 *
 * DESCRIPTION
 *  IOCTL Msg handle
 *
 *****************************************************************************
 */
static long tfa_nxpspk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	switch (cmd)
	{
        case I2C_SLAVE:
        case I2C_SLAVE_FORCE:
            new_client->addr = arg;
            return 0;
        default:
        {
            printk("AudDrv_nxpspk_ioctl Fail command: 0x%4x.\n", cmd);
            ret = -1;
            break;
        }
	}
	return ret;
}


static int tfa_nxpspk_open(struct inode *inode, struct file *fp)
{
	return 0;
}

static int nxp_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.timing = I2C_MASTER_CLOCK;

	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.ext_flag = client->ext_flag | I2C_DMA_FLAG; //USE DMA Transfer
	msg.flags = 0;
	msg.buf = (char *)buf;
	msg.len = count;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
    if(ret != 1){
	  pr_err("nxp_i2c_master_send err = %d\n", ret);
    }
	return (ret == 1) ? count : ret;
}

static int nxp_i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.timing = I2C_MASTER_CLOCK;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.ext_flag = client->ext_flag | I2C_DMA_FLAG; //USE DMA Transfer
	msg.flags = I2C_M_RD;
	msg.buf = (char *)buf;
	msg.len = count;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg received), return #bytes received,
	 * else error code.
	 */
	if(ret != 1){
	  pr_err("nxp_i2c_master_recv err = %d\n", ret);
    }
	return (ret == 1) ? count : ret;
}


static ssize_t tfa_nxpspk_write(struct file *fp, const char __user *data, size_t count, loff_t *offset)
{
	int i = 0;
	int ret;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	if (copy_from_user(tmp, data, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	for (i = 0;  i < count; i++){
		TfaI2CDMABuf[i] = tmp[i];
    }
	ret = nxp_i2c_master_send(new_client, (char *)TfaI2CDMABuf_pa, count);
	kfree(tmp);
	return ret;
}

static ssize_t tfa_nxpspk_read(struct file *fp,  char __user *data, size_t count, loff_t *offset)
{
	int i = 0;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;
	int ret = 0;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		  return -ENOMEM;

	ret = nxp_i2c_master_recv(new_client, (char *)TfaI2CDMABuf_pa, count);
	for (i = 0; i < count; i++)
		  tmp[i] = TfaI2CDMABuf[i];

	if (ret >= 0){
		ret = copy_to_user(data, tmp, count) ? (-EFAULT) : ret;
    }
	kfree(tmp);
	return ret;
}

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open    = tfa_nxpspk_open,
	.unlocked_ioctl   = tfa_nxpspk_ioctl,
	.write   = tfa_nxpspk_write,
	.read    = tfa_nxpspk_read,
};

#define MODULE_NAME	"i2c_smartpa"
static struct miscdevice tfa_misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODULE_NAME,
	.fops = &fops,
};


#define GTP_DMA_MAX_TRANSACTION_LENGTH 1024

static int tfa_i2c_probe(struct i2c_client *pClient,
	const struct i2c_device_id *pID)
{
	int ret;
	pr_warn("+tfa_i2c_probe\n");
	tfa_pinctrl = devm_pinctrl_get(&pClient->dev);

	if(IS_ERR(tfa_pinctrl)) {
		printk("Cannot find pinctrl....");
	} else {
		tfa_reset= pinctrl_lookup_state(tfa_pinctrl,"smartpa_reset");
		tfa_enable= pinctrl_lookup_state(tfa_pinctrl,"smartpa_enable");

		if(IS_ERR(tfa_enable)) {
			printk("%s:lookup error .......",__func__);
		} else {
			printk("%s:enable smart pa ...",__func__);
			pinctrl_select_state(tfa_pinctrl,tfa_enable);
			msleep(10);
		}
	}

	TfaI2CDMABuf_va = (u8*)dma_alloc_coherent(&pClient->dev, 4096, &TfaI2CDMABuf_pa, GFP_KERNEL);
	if (!TfaI2CDMABuf_va) {
		pr_err("dma_alloc_coherent error aa\n");
		return -1;
	}
	memset(TfaI2CDMABuf_va, 0, GTP_DMA_MAX_TRANSACTION_LENGTH);

	new_client = pClient;

	ret = misc_register(&tfa_misc);
	if (ret) {
		printk("%s:register faild %d.......",__func__,ret);
		return ret;
	}
	pr_warn("-tfa_i2c_probe\n");
	return 0;
}


static int tfa_i2c_remove(struct i2c_client *pClient)
{
	pr_warn("+tfa_i2c_remove\n");
	misc_deregister(&tfa_misc);
	pr_warn("-tfa_i2c_remove\n");
	return 0;
}

static const struct i2c_device_id tfa_i2c_id[] = {
	{"tfa9890", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, tfa_i2c_id);

static const struct of_device_id tfa_of_match[] = {
	{.compatible = "mediatek,tfasmartpa"},
	{},
};

static struct i2c_driver tfa_i2c_driver = {
	.driver = {
			.name = "tfa9890",
			.owner = THIS_MODULE,
			.of_match_table = tfa_of_match,
		},
	.probe = tfa_i2c_probe,
	.remove = tfa_i2c_remove,
	.id_table = tfa_i2c_id,
};

static int tfa_mod_init(void)
{
	/* [houjihai start ] Modify for device and driver match failed start */
#if 0
	if (i2c_add_driver(&tfa_i2c_driver)) {
		pr_warn("fail to add device into i2c");
		return -1;
	}
	return 0;
#endif
	int ret = 0;
	pr_warn("+tfa_mod_init\n");
	ret = i2c_add_driver(&tfa_i2c_driver);
	pr_warn("-tfa_mod_init\n");
	return ret;
	/* [houjihai end] */
}

static void tfa_mod_exit(void)
{
	pr_warn("tfa_mod_exit.");
}

module_init(tfa_mod_init);
module_exit(tfa_mod_exit);

MODULE_AUTHOR("wuzehui<wuzehui_w8802@tplink.com.cn>");
MODULE_DESCRIPTION("Smart PA TFA9890 driver");
MODULE_LICENSE("GPLv2");

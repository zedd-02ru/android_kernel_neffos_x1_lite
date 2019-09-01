/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include<linux/wakelock.h>
#include "include/tpd_ft5x0x_common.h"
#include <linux/proc_fs.h>
#include <mach/gpio_const.h>


#include "focaltech_core.h"
/* #include "ft5x06_ex_fun.h" */

#include "tpd.h"
#include <linux/input/mt.h>
/* #define TIMER_DEBUG */


#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif

#ifdef FTS_MCAP_TEST
#include "mcap_test_lib.h"
#endif
#include <linux/wakelock.h>

#include "focaltech_drv_printlog.h"

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
enum DOZE_T
{
    DOZE_DISABLED = 0,
    DOZE_ENABLED = 1,
    DOZE_WAKEUP = 2,
};
static DOZE_T doze_status = DOZE_DISABLED;
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client);

enum TOUCH_IPI_CMD_T
{
    /* SCP->AP */
    IPI_COMMAND_SA_GESTURE_TYPE,
    /* AP->SCP */
    IPI_COMMAND_AS_CUST_PARAMETER,
    IPI_COMMAND_AS_ENTER_DOZEMODE,
    IPI_COMMAND_AS_ENABLE_GESTURE,
    IPI_COMMAND_AS_GESTURE_SWITCH,
};

struct Touch_Cust_Setting
{
    u32 i2c_num;
    u32 int_num;
    u32 io_int;
    u32 io_rst;
};

struct Touch_IPI_Packet
{
    u32 cmd;
    union
    {
        u32 data;
        Touch_Cust_Setting tcs;
    } param;
};

/* static bool tpd_scp_doze_en = FALSE; */
static bool tpd_scp_doze_en = TRUE;
DEFINE_MUTEX(i2c_access);
#endif

#define TPD_SUPPORT_POINTS  10


struct i2c_client *i2c_client = NULL;
struct task_struct *thread_tpd = NULL;
/*******************************************************************************
* 4.Static variables
*******************************************************************************/
struct i2c_client *fts_i2c_client               = NULL;
struct input_dev *fts_input_dev             =NULL;
#ifdef TPD_AUTO_UPGRADE
static bool is_update = false;
#endif
#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
u8 *tpd_i2c_dma_va = NULL;
dma_addr_t tpd_i2c_dma_pa = 0;
#endif

struct wake_lock ps_lock;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);
static int tpd_flag;
/*static int point_num = 0;
static int p_point_num = 0;*/

#ifdef USB_CHARGE_DETECT
u8 b_usb_plugin = 0;
int ctp_is_probe = 0;
#endif

#ifdef  TPD_AUTO_UPGRADE
struct workqueue_struct *touch_wq;
struct work_struct fw_update_work;
#endif

//unsigned int tpd_rst_gpio_number = 10;
static unsigned int tpd_int_gpio_number = 1;
static unsigned int touch_irq = 0;
#define TPD_OK 0

/* Register define */
#define DEVICE_MODE 0x00
#define GEST_ID     0x01
#define TD_STATUS   0x02

#define TOUCH1_XH   0x03
#define TOUCH1_XL   0x04
#define TOUCH1_YH   0x05
#define TOUCH1_YL   0x06

#define TOUCH2_XH   0x09
#define TOUCH2_XL   0x0A
#define TOUCH2_YH   0x0B
#define TOUCH2_YL   0x0C

#define TOUCH3_XH   0x0F
#define TOUCH3_XL   0x10
#define TOUCH3_YH   0x11
#define TOUCH3_YL   0x12

#define TPD_RESET_ISSUE_WORKAROUND
#define TPD_MAX_RESET_COUNT 3


/*********************************************************************/
//lichenggang add for tp-link for create node to gesture and glove mode
static struct proc_dir_entry *prEntry_tp = NULL;
static struct proc_dir_entry *prEntry_dtap = NULL;
static atomic_t glove_enable;
atomic_t double_enable;
static int init_fts_proc(void);
DEFINE_MUTEX(ft_suspend_lock);
static int tp_suspend = 0;
static ssize_t tp_double_write_func(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static ssize_t tp_double_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);

static ssize_t glove_enable_write_func(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static ssize_t glove_enable_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
//char manufacture_info[12];
static u8 finger_num = 0;
static int usb_check_state =0;

#if FT_ESD_PROTECT
int apk_debug_flag = 0;
#endif
void esd_switch(s32 on);
extern ssize_t tp_info_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
//#define TPD_ERR(a, arg...)  pr_err(TPD_DEVICE ": " a, ##arg);


/*********************************************************************/


#ifdef FTS_MCAP_TEST
struct i2c_client *g_focalclient = NULL;
extern int focal_i2c_Read(unsigned char *writebuf, int writelen, unsigned char *readbuf, int readlen);
extern int focal_i2c_Write(unsigned char *writebuf, int writelen);
#endif


//#define LCT_ADD_TP_LOCKDOWN_INFO
#ifdef LCT_ADD_TP_LOCKDOWN_INFO
#define CTP_PROC_LOCKDOWN_FILE "tp_lockdown_info"
extern unsigned char ft5x46_ctpm_LockDownInfo_get_from_boot(  struct i2c_client *client,char *pProjectCode );
static struct proc_dir_entry *ctp_lockdown_status_proc = NULL;
char tp_lockdown_info[128];

static int ctp_lockdown_proc_show(struct seq_file *file, void* data)
{
    char temp[40] = {0};

    sprintf(temp, "%s\n", tp_lockdown_info);
    seq_printf(file, "%s\n", temp);
    return 0;
}

static int ctp_lockdown_proc_open (struct inode* inode, struct file* file)
{
    return single_open(file, ctp_lockdown_proc_show, inode->i_private);
}

static const struct file_operations ctp_lockdown_proc_fops =
{
    .open = ctp_lockdown_proc_open,
    .read = seq_read,
};
#endif

static ssize_t tp_double_write_func(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	int ret = 0;
	int state = 0;
	char buf[10] = {0};

	if(count > 10)
		return count;
	if(copy_from_user(buf,buffer,count)){
		printk("%s,read proc input error.\n",__func__);
		return count;
	}
	sscanf(buf,"%d",&ret);
	printk("tp_double_read_func:buf=%d,ret=%d\n",*buf,ret);
	mutex_lock(&ft_suspend_lock);
	if((ret == 0)||(ret == 1))
	{
		atomic_set(&double_enable,ret);
	}
	if(tp_suspend== 1)
	{
		if(ret == 1)
		{
			printk(".....gesture will enable.....\n");
			fts_read_reg(i2c_client,0xa5,(u8 *)&state);
			fts_write_reg(i2c_client,0xa5,0x00);
			fts_write_reg(i2c_client,0xd0,0x01);
			fts_write_reg(i2c_client,0xd1,0x3f);
			fts_write_reg(i2c_client,0xd2,0x07);
			fts_write_reg(i2c_client,0xd6,0xff);
			enable_irq(touch_irq);
		}
		if(ret == 0)
		{
			printk(".....gesture will disable.....\n");
			fts_read_reg(i2c_client,0xa5,(u8 *)&state);
			msleep(50);
			fts_write_reg(i2c_client,0xa5,0x02);
		}
	}
	mutex_unlock(&ft_suspend_lock);
	return count;

}

static ssize_t tp_double_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int len = 0;
	int ret = 0;
	char page[512];
	printk("double_tap enable is:%d\n",atomic_read(&double_enable));
	len = sprintf(page,"%d\n",atomic_read(&double_enable));
	ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));
	return ret;
}

static ssize_t glove_enable_write_func(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
 {
	int ret = 0;
	//int state = 0 ;
	char buf[10]={0};

	if(count>10)
		return count;
	if(copy_from_user(buf,buffer,count))
	{
		printk("%s:read from proc error!\n",__func__);
		return count;
	}
	sscanf(buf,"%d\n",&ret);
	printk("%s:buf=%d,ret=%d\n",__func__,*buf,ret);
	mutex_lock(&ft_suspend_lock);
	if((ret==1)||(ret==0))
	{
		atomic_set(&glove_enable,ret);
		//atomic_set(&glove_enable,0);
		if(ret == 1)
		{
			printk("glove mode will enable\n");
			fts_write_reg(i2c_client,0xC0,1);
		}else{
			printk("glove mode will disable\n");
			fts_write_reg(i2c_client,0xC0,0);
		}
	}
	mutex_unlock(&ft_suspend_lock);
	return count;
}

static ssize_t glove_enable_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int len = 0;
	int ret = 0 ;
	char page[512];
	printk("glove_enable is: %d\n", atomic_read(&glove_enable));
	len = sprintf(page, "%d\n", atomic_read(&glove_enable));
	ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));
	return ret;
}

//static ssize_t tp_info_read_func(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
//{
	//int len = 0;
	//int ret = 0 ;
	//char page[512];
	//printk("tp_info is: %s\n", manufacture_info);
	//len = sprintf(page, "%s\n", manufacture_info);
	//ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));
	//return ret;
//}

static const struct file_operations double_tap_enable_fops = {

	.write = tp_double_write_func,
	.read = tp_double_read_func,
	.open = simple_open,
	.owner = THIS_MODULE,

};

static const struct file_operations glove_enable_fops = {
	.write = glove_enable_write_func,
	.read = glove_enable_read_func,
	.open = simple_open,
	.owner = THIS_MODULE,
};

static const struct file_operations tp_info_fops = {
	//.write = glove_enable_write_func,
	.read = tp_info_read_func,
	.open = simple_open,
	.owner = THIS_MODULE,
};



#ifdef TIMER_DEBUG
static struct timer_list test_timer;
static void timer_func(unsigned long data)
{
    tpd_flag = 1;
    wake_up_interruptible(&waiter);

    mod_timer(&test_timer, jiffies + 100*(1000/HZ));
}

static int init_test_timer(void)
{
    memset((void *)&test_timer, 0, sizeof(test_timer));
    test_timer.expires  = jiffies + 100*(1000/HZ);
    test_timer.function = timer_func;
    test_timer.data     = 0;
    init_timer(&test_timer);
    add_timer(&test_timer);
    return 0;
}
#endif


#if defined(CONFIG_TPD_ROTATE_90) || defined(CONFIG_TPD_ROTATE_270) || defined(CONFIG_TPD_ROTATE_180)

static void tpd_rotate_180(int *x, int *y)
{
    *y = TPD_RES_Y + 1 - *y;
    *x = TPD_RES_X + 1 - *x;
}

#endif
struct touch_info
{
    int y[TPD_SUPPORT_POINTS];
    int x[TPD_SUPPORT_POINTS];
    int p[TPD_SUPPORT_POINTS];
    int id[TPD_SUPPORT_POINTS];
    int count;
};

/*dma declare, allocate and release*/
//#define __MSG_DMA_MODE__
#ifdef CONFIG_MTK_I2C_EXTENSION
u8 *g_dma_buff_va = NULL;
dma_addr_t g_dma_buff_pa = 0;
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION

static void msg_dma_alloct(void)
{
    if (NULL == g_dma_buff_va)
    {
        tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        g_dma_buff_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 128, &g_dma_buff_pa, GFP_KERNEL);
    }

    if (!g_dma_buff_va)
    {
        FTS_DEBUG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
    }
}
static void msg_dma_release(void)
{
    if (g_dma_buff_va)
    {
        dma_free_coherent(NULL, 128, g_dma_buff_va, g_dma_buff_pa);
        g_dma_buff_va = NULL;
        g_dma_buff_pa = 0;
        FTS_DEBUG("[DMA][release] Allocate DMA I2C Buffer release!\n");
    }
}
#endif

static DEFINE_MUTEX(i2c_access);
static DEFINE_MUTEX(i2c_rw_access);

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
/* static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX; */
/* static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX; */
static int tpd_def_calmat_local_normal[8]  = TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] = TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

static const struct i2c_device_id ft5x0x_tpd_id[] = {{"ft5x0x", 0}, {} };
static const struct of_device_id ft5x0x_dt_match[] =
{
    {.compatible = "mediatek,cap_touch"},
    {},
};
MODULE_DEVICE_TABLE(of, ft5x0x_dt_match);

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .of_match_table = of_match_ptr(ft5x0x_dt_match),
        .name = "ft5x0x",
    },
    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = ft5x0x_tpd_id,
    .detect = tpd_i2c_detect,
};

static int of_get_ft5x0x_platform_data(struct device *dev)
{
    /*int ret, num;*/

    if (dev->of_node)
    {
        const struct of_device_id *match;

        match = of_match_device(of_match_ptr(ft5x0x_dt_match), dev);
        if (!match)
        {
            FTS_DEBUG("Error: No device match found\n");
            return -ENODEV;
        }
    }
//  tpd_rst_gpio_number = of_get_named_gpio(dev->of_node, "rst-gpio", 0);
//  tpd_int_gpio_number = of_get_named_gpio(dev->of_node, "int-gpio", 0);
    /*ret = of_property_read_u32(dev->of_node, "rst-gpio", &num);
    if (!ret)
        tpd_rst_gpio_number = num;
    ret = of_property_read_u32(dev->of_node, "int-gpio", &num);
    if (!ret)
        tpd_int_gpio_number = num;
    */
    //FTS_DEBUG("g_vproc_en_gpio_number %d\n", tpd_rst_gpio_number);
    FTS_DEBUG("g_vproc_vsel_gpio_number %d\n", tpd_int_gpio_number);
    return 0;
}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static ssize_t show_scp_ctrl(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}
static ssize_t store_scp_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    u32 cmd;
    Touch_IPI_Packet ipi_pkt;

    if (kstrtoul(buf, 10, &cmd))
    {
        FTS_DEBUG("[SCP_CTRL]: Invalid values\n");
        return -EINVAL;
    }

    FTS_DEBUG("SCP_CTRL: Command=%d", cmd);
    switch (cmd)
    {
        case 1:
            /* make touch in doze mode */
            tpd_scp_wakeup_enable(TRUE);
            tpd_suspend(NULL);
            break;
        case 2:
            tpd_resume(NULL);
            break;
        /*case 3:
        // emulate in-pocket on
        ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
        ipi_pkt.param.data = 1;
        md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
        break;
        case 4:
         // emulate in-pocket off
         ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
         ipi_pkt.param.data = 0;
        md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
         break;*/
        case 5:
        {
            Touch_IPI_Packet ipi_pkt;

            ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
            ipi_pkt.param.tcs.i2c_num = TPD_I2C_NUMBER;
            ipi_pkt.param.tcs.int_num = CUST_EINT_TOUCH_PANEL_NUM;
            ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
            //ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;
            if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0) < 0)
                FTS_DEBUG("[TOUCH] IPI cmd failed (%d)\n", ipi_pkt.cmd);

            break;
        }
        default:
            FTS_DEBUG("[SCP_CTRL] Unknown command");
            break;
    }

    return size;
}
static DEVICE_ATTR(tpd_scp_ctrl, 0664, show_scp_ctrl, store_scp_ctrl);
#endif

static struct device_attribute *ft5x0x_attrs[] =
{
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    &dev_attr_tpd_scp_ctrl,
#endif
};



#ifdef USB_CHARGE_DETECT
void tpd_usb_plugin(u8 plugin)
{
    int ret = -1;
    b_usb_plugin = plugin;

    if (!ctp_is_probe)
    {
        return;
    }
    FTS_DEBUG("Fts usb detect: %s %d.\n",__func__,b_usb_plugin);

    ret = i2c_smbus_write_i2c_block_data(fts_i2c_client, 0x8B, 1, &b_usb_plugin);
    if ( ret < 0 )
    {
        FTS_DEBUG("Fts usb detect write err: %s %d.\n",__func__,b_usb_plugin);
    }
}
EXPORT_SYMBOL(tpd_usb_plugin);
#endif


#ifdef CONFIG_MTK_I2C_EXTENSION

int fts_i2c_read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
    int ret=0;

    // for DMA I2c transfer

    mutex_lock(&i2c_rw_access);

    if ((NULL!=client) && (writelen>0) && (writelen<=128))
    {
        // DMA Write
        memcpy(g_dma_buff_va, writebuf, writelen);
        client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
        if ((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
            //dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
            FTS_DEBUG("i2c write failed\n");
        client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
    }

    // DMA Read

    if ((NULL!=client) && (readlen>0) && (readlen<=128))

    {
        client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;

        ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

        memcpy(readbuf, g_dma_buff_va, readlen);

        client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
    }

    mutex_unlock(&i2c_rw_access);

    return ret;
}
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
    int i = 0;
    int ret = 0;

    if (writelen <= 8)
    {
        client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
        return i2c_master_send(client, writebuf, writelen);
    }
    else if ((writelen > 8)&&(NULL != tpd_i2c_dma_va))
    {
        for (i = 0; i < writelen; i++)
            tpd_i2c_dma_va[i] = writebuf[i];

        client->addr = (client->addr & I2C_MASK_FLAG )| I2C_DMA_FLAG;
        //client->ext_flag = client->ext_flag | I2C_DMA_FLAG;
        ret = i2c_master_send(client, (unsigned char *)tpd_i2c_dma_pa, writelen);
        client->addr = client->addr & I2C_MASK_FLAG & ~I2C_DMA_FLAG;
        //ret = i2c_master_send(client, (u8 *)(uintptr_t)tpd_i2c_dma_pa, writelen);
        //client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
        return ret;
    }
    return 1;
}

#else

int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
    int ret = -1;

    mutex_lock(&i2c_rw_access);

    if (readlen > 0)
    {
        if (writelen > 0)
        {
            struct i2c_msg msgs[] =
            {
                {
                    .addr = client->addr,
                    .flags = 0,
                    .len = writelen,
                    .buf = writebuf,
                },
                {
                    .addr = client->addr,
                    .flags = I2C_M_RD,
                    .len = readlen,
                    .buf = readbuf,
                },
            };
            ret = i2c_transfer(client->adapter, msgs, 2);
            if (ret < 0)
                dev_err(&client->dev, "%s: i2c read error.\n", __func__);
        }
        else
        {
            struct i2c_msg msgs[] =
            {
                {
                    .addr = client->addr,
                    .flags = I2C_M_RD,
                    .len = readlen,
                    .buf = readbuf,
                },
            };
            ret = i2c_transfer(client->adapter, msgs, 1);
            if (ret < 0)
                dev_err(&client->dev, "%s:i2c read error.\n", __func__);
        }
    }

    mutex_unlock(&i2c_rw_access);

    return ret;
}

int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret = -1;

    struct i2c_msg msgs[] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = writelen,
            .buf = writebuf,
        },
    };
    mutex_lock(&i2c_rw_access);
    if (writelen > 0)
    {
        ret = i2c_transfer(client->adapter, msgs, 1);
        if (ret < 0)
            dev_err(&client->dev, "%s: i2c write error.\n", __func__);
    }

    mutex_unlock(&i2c_rw_access);

    return ret;
}

#endif
/************************************************************************
* Name: fts_write_reg
* Brief: write register
* Input: i2c info, reg address, reg value
* Output: no
* Return: fail <0
***********************************************************************/
int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
    unsigned char buf[2] = {0};

    buf[0] = regaddr;
    buf[1] = regvalue;

    return fts_i2c_write(client, buf, sizeof(buf));
}
/************************************************************************
* Name: fts_read_reg
* Brief: read register
* Input: i2c info, reg address, reg value
* Output: get reg value
* Return: fail <0
***********************************************************************/
int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{

    return fts_i2c_read(client, &regaddr, 1, regvalue, 1);

}

#ifdef  TPD_AUTO_UPGRADE
static void fts_fw_update_work_func(struct work_struct *work)
{
    TPD_ERR("********************FTS Enter CTP Auto Upgrade ********************\n");
    disable_irq(touch_irq);
    is_update = true ;
#if (FTS_UPGRADE_PINGPANG_TEST || FTS_UPGRADE_ERROR_TEST)
    fts_ctpm_auto_upgrade_test(fts_i2c_client);
#else
    fts_ctpm_auto_upgrade(fts_i2c_client);
#endif
    is_update = false;
    enable_irq(touch_irq);
}

static int fts_workqueue_init(void)
{
    /*
     *TPD_FUN ();
     */

    touch_wq = create_singlethread_workqueue("touch_wq");
    if (touch_wq)
    {
        INIT_WORK(&fw_update_work, fts_fw_update_work_func);
    }
    else
    {
        goto err_workqueue_init;
    }
    return 0;

err_workqueue_init:
    printk("create_singlethread_workqueue failed\n");
    return -1;
}
#endif

#ifdef MT_PROTOCOL_B
static int fts_read_Touchdata(struct ts_event *data)
{
       u8 buf[POINT_READ_BUF] = { 0 };//0xFF
	int ret = -1;
	int i = 0;
	u8 pointid = FTS_MAX_ID;
	//mutex_lock(&i2c_access);
	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0)
	{
		dev_err(&fts_i2c_client->dev, "%s read touchdata failed.\n",__func__);
		//mutex_unlock(&i2c_access);
		return ret;
	}

	//mutex_unlock(&i2c_access);
	memset(data, 0, sizeof(struct ts_event));
	data->touch_point = 0;
	data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
	finger_num = data->touch_point_num;

	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)
	{
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else
			data->touch_point++;
		data->au16_x[i] =
		    (s16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
		data->au16_y[i] =
		    (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
		data->au8_touch_event[i] =
		    buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
		data->au8_finger_id[i] =
		    (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;

		data->pressure[i] =
			(buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);	//cannot constant value
		data->area[i] =
			(buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;

		if((data->au8_touch_event[i]==0 || data->au8_touch_event[i]==2)&&((data->touch_point_num==0)/*||(data->pressure[i]==0 && data->area[i]==0  )*/))
			return 1;

		if(data->pressure[i]<=0)
		{
			data->pressure[i]=0x3f;
		}
		if(data->area[i]<=0)
		{
			data->area[i]=0x05;
		}

		//if ( pinfo->au16_x[i]==0 && pinfo->au16_y[i] ==0)
		//	pt00f++;
	}

	return 0;
}
#endif

static int report_count=0;
#ifdef MT_PROTOCOL_B
static int fts_report_value(struct ts_event *data)
 {
	int i = 0,j=0;
	int up_point = 0;
 	int touchs = 0;
	static int touch_major_report_num  = 5;

	for (i = 0; i < data->touch_point; i++)
	{
		input_mt_slot(tpd->dev, data->au8_finger_id[i]);
		if (data->au8_touch_event[i]== 0 || data->au8_touch_event[i] == 2)
		{
			 report_count++;
			 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,1);
			 input_report_key(tpd->dev, BTN_TOOL_FINGER, 1);
			 //input_report_abs(tpd->dev, ABS_MT_PRESSURE,data->pressure[i]);
			 //input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR,data->area[i]);
			 input_report_abs(tpd->dev, ABS_MT_POSITION_X,data->au16_x[i]);
			 input_report_abs(tpd->dev, ABS_MT_POSITION_Y,data->au16_y[i]);
			 input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR,touch_major_report_num);
			 if((report_count%50)==0){
					touch_major_report_num++;
					input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR,touch_major_report_num);
				}
			 if(report_count==150){
			 	TPD_ERR("tpd->dev::tpd_down_B:: x[%d],y[%d]=[%d   %d]\n",i,i,data->au16_x[i],data->au16_y[i]);
				report_count=0;
			}
			touchs |= BIT(data->au8_finger_id[i]);
   			data->touchs |= BIT(data->au8_finger_id[i]);
		}
		else
		{
			 up_point++;
			 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,0);
			 data->touchs &= ~BIT(data->au8_finger_id[i]);
		}

	}

	for(i = 0; i < 10; i++){
		if(BIT(i) & (data->touchs ^ touchs)){
			printk("\n Up by manual  id=%d \n", i);
			data->touchs &= ~BIT(i);
			input_mt_slot(tpd->dev, i);
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
		}
	}


	data->touchs = touchs;
	if((data->touch_point_num==0))    // release all touches in final
	{
		for(j = 0; j <10; j++)
		{
			input_mt_slot( tpd->dev, j);
			input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, false);
		}
		//last_touchpoint=0;
		data->touchs=0;
		input_report_key(tpd->dev, BTN_TOUCH, 0);
		input_report_key(tpd->dev,BTN_TOOL_FINGER,0);
		input_sync(tpd->dev);
		return 0;
    }

	if(data->touch_point == up_point){
		touch_major_report_num=5;
		input_report_key(tpd->dev, BTN_TOUCH, 0);
	}else{
		input_report_key(tpd->dev, BTN_TOUCH, 1);
	}
	input_sync(tpd->dev);
	return 0;

}
#endif


static int key_back_button = 0;
static int key_menu_button = 0;
static int key_home_button = 0;
extern int button_reverse;
static int fts_report_button(struct ts_event *data)
{
	if (data->au8_touch_event[0]== 0 || data->au8_touch_event[0] == 2)
	{
		if(button_reverse == 1)
		{
			if((data->au16_x[0] == 180)&&(key_back_button == 0)){
				input_report_key(tpd->dev, KEY_BACK, 1);
				key_back_button = 1;
				TPD_ERR("tpd_down::KEY_BACK\n");
				input_sync(tpd->dev);
			}else if((data->au16_x[0] == 360)&&(key_home_button == 0)){
				input_report_key(tpd->dev, KEY_HOMEPAGE, 1);
				key_home_button = 1;
				TPD_ERR("tpd_down::KEY_HOMEPAGE\n");
				input_sync(tpd->dev);	
			}else if((data->au16_x[0] == 540)&&(key_menu_button == 0)){
				input_report_key(tpd->dev, KEY_MENU, 1);
				key_menu_button = 1;
				TPD_ERR("tpd_down::KEY_MENU\n");
				input_sync(tpd->dev);
			}
		}else{
			if(data->au16_x[0] == 180&&(key_menu_button == 0)){
				input_report_key(tpd->dev, KEY_MENU, 1);
				key_menu_button = 1;
				TPD_ERR("tpd_down::KEY_MENU\n");
				input_sync(tpd->dev);
			}else if(data->au16_x[0] == 360&&(key_home_button == 0)){
				input_report_key(tpd->dev, KEY_HOMEPAGE, 1);
				key_home_button = 1;
				TPD_ERR("tpd_down::KEY_HOMEPAGE\n");
				input_sync(tpd->dev);
			}else if(data->au16_x[0] == 540&&(key_back_button == 0)){
				input_report_key(tpd->dev, KEY_BACK, 1);
				key_back_button = 1;
				TPD_ERR("tpd_down::KEY_BACK\n");
				input_sync(tpd->dev);
			}
		}
	}else{
		if(key_back_button == 1){
			input_report_key(tpd->dev, KEY_BACK, 0);
			input_sync(tpd->dev);
			key_back_button = 0;
		}else if(key_home_button == 1){
			input_report_key(tpd->dev, KEY_HOMEPAGE, 0);
			input_sync(tpd->dev);
			key_home_button = 0;
		}else if(key_menu_button == 1){
			input_report_key(tpd->dev, KEY_MENU, 0);
			input_sync(tpd->dev);
			key_menu_button = 0;
		}
	}
	return 0;
}

static int touch_event_handler(void *unused)
{
    int i = 0;
#if FTS_GESTRUE_EN
    int ret = 0;
    u8 state = 0;
#endif
	struct ts_event pevent={};
   	struct touch_info finfo;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };

    if (tpd_dts_data.use_tpd_button)
    {
        memset(&finfo, 0, sizeof(struct touch_info));
        for (i = 0; i < TPD_SUPPORT_POINTS; i++)
            finfo.p[i] = 1;
    }

    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        /*enable_irq(touch_irq);*/
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag != 0);

        tpd_flag = 0;

        set_current_state(TASK_RUNNING);

#if FTS_GESTRUE_EN
        ret = fts_read_reg(fts_i2c_client, 0xd0,&state);
        if (ret<0)
        {
            FTS_DEBUG("[Focal][Touch] read value fail");
            //return ret;
        }
        if (state ==1)
        {
            fts_read_Gestruedata();
	        enable_irq(touch_irq);
            continue;
        }
#endif

#ifdef MT_PROTOCOL_B
		{

#if FT_ESD_PROTECT
	apk_debug_flag = 1;
#endif
			ret = fts_read_Touchdata(&pevent);
			if (ret == 0) {
				TPD_DEBUG("%s: read touch data success, start report event \n", __func__);
				#ifdef CONFIG_TPLINK_PRODUCT_TP902
					if(pevent.au16_y[0] != 1350){
						fts_report_value(&pevent);
					}else{
						fts_report_button(&pevent);
					}
				#endif

				#ifdef CONFIG_TPLINK_PRODUCT_TP903
					if(pevent.au16_y[0] != 1960){
						fts_report_value(&pevent);
					}else{
						fts_report_button(&pevent);
					}
				#endif

			} else {
				TPD_DMESG("%s: read touch data faild \n", __func__);
			}
#if FT_ESD_PROTECT
	apk_debug_flag = 0;
#endif
		}
#else
		{
/* #if CTP_ESD_PROTECT
			apk_debug_flag = 1;
#endif */
		if (tpd_touchinfo(&cinfo, &pinfo)) {
			if (tpd_dts_data.use_tpd_button) {
				if (cinfo.p[0] == 0)
					memcpy(&finfo, &cinfo, sizeof(struct touch_info));
			}

			if ((cinfo.y[0] >= TPD_RES_Y) && (pinfo.y[0] < TPD_RES_Y)
			     && ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
				TPD_DEBUG("Dummy release --->\n");
				tpd_up(pinfo.x[0], pinfo.y[0]);
				input_sync(tpd->dev);
				continue;
			}

			if (tpd_dts_data.use_tpd_button) {
				if ((cinfo.y[0] <= TPD_RES_Y && cinfo.y[0] != 0) && (pinfo.y[0] > TPD_RES_Y)
				&& ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
					TPD_DEBUG("Dummy key release --->\n");
					tpd_button(pinfo.x[0], pinfo.y[0], 0);
					input_sync(tpd->dev);
					continue;
				}

			if ((cinfo.y[0] > TPD_RES_Y) || (pinfo.y[0] > TPD_RES_Y)) {
				if (finfo.y[0] > TPD_RES_Y) {
					if ((cinfo.p[0] == 0) || (cinfo.p[0] == 2)) {
							TPD_DEBUG("Key press --->\n");
							tpd_button(pinfo.x[0], pinfo.y[0], 1);
					} else if ((cinfo.p[0] == 1) &&
						((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
							TPD_DEBUG("Key release --->\n");
							tpd_button(pinfo.x[0], pinfo.y[0], 0);
					}
					input_sync(tpd->dev);
				}
				continue;
			}
			}

			if (cinfo.count > 0) {
				for (i = 0; i < cinfo.count; i++)
					tpd_down(cinfo.x[i], cinfo.y[i], i + 1, cinfo.id[i]);
			} else {
#ifdef TPD_SOLVE_CHARGING_ISSUE
				tpd_up(1, 48);
#else
				tpd_up(cinfo.x[0], cinfo.y[0]);
#endif

			}
			input_sync(tpd->dev);

/* #if CTP_ESD_PROTECT
			apk_debug_flag = 0;
#endif */
		}
	}
#endif

    }while (!kthread_should_stop());

    FTS_DEBUG("touch_event_handler exit\n");

    return 0;
}

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);

    return 0;
}

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
   // FTS_DEBUG("TPD interrupt has been triggered\n");
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
    return IRQ_HANDLED;
}
static int tpd_irq_registration(void)
{
    struct device_node *node = NULL;
    int ret = 0;
    u32 ints[2] = {0,0};

    node = of_find_matching_node(node, touch_of_match);
    if (node)
    {
        /*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
        of_property_read_u32_array(node,"debounce", ints, ARRAY_SIZE(ints));
        FTS_DEBUG("debounce:ints[0]=%d,ints[1]=%d", ints[0], ints[1]);
        gpio_set_debounce(ints[0], ints[1]);
        touch_irq = irq_of_parse_and_map(node, 0);
        FTS_DEBUG("touch_irq=%d", touch_irq);
        ret = request_irq(touch_irq, tpd_eint_interrupt_handler,
                          IRQF_TRIGGER_FALLING, "TOUCH_PANEL-eint", NULL);
        if (ret > 0)
            FTS_DEBUG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
    }
    else
    {
        FTS_DEBUG("[%s] tpd request_irq can not find touch eint device node!.", __func__);
    }
    return 0;
}

extern unsigned char ft5x46_ctpm_InkId_get_from_boot(  struct i2c_client *client );
static int init_fts_proc(void)
{
	int ret = 0;
	prEntry_tp = proc_mkdir("touchpanel",NULL);
	if(prEntry_tp == NULL)
	{
		ret = -ENOMEM;
		printk("touchpanle:can not create proc entry!\n");
	}

	prEntry_dtap = proc_create("double_tap_enable",0666,prEntry_tp,&double_tap_enable_fops);
	if(prEntry_dtap == NULL)
	{
		ret = -ENOMEM;
		printk("double_tap can not create\n");
	}
	prEntry_dtap = proc_create("glove_mode_enable",0666,prEntry_tp,&glove_enable_fops);
	if(prEntry_dtap == NULL)
	{
		ret=-ENOMEM;
		printk("glove_enable can not create\n");
	}
	prEntry_dtap = proc_create("tp_info",0666,prEntry_tp,&tp_info_fops);
	if(prEntry_dtap == NULL)
	{
		ret=-ENOMEM;
		printk("tp_info can not create\n");
	}

	return 0;
}


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int retval = TPD_OK;
    //u8 report_rate = 0;
    int reset_count = 0;
    char data;
    int err = 0;

    i2c_client = client;
    fts_i2c_client = client;
#ifdef FTS_MCAP_TEST
    g_focalclient = client;
#endif
    fts_input_dev=tpd->dev;

    TPD_ERR("tp902::FT_probe_statr\n");

    if (i2c_client->addr != 0x38)
    {
        i2c_client->addr = 0x38;
        FTS_DEBUG("tp902:i2c_client_FT->addr=%d\n",i2c_client->addr);
    }

    of_get_ft5x0x_platform_data(&client->dev);
    /* configure the gpio pins */
    TPD_ERR("mtk_tpd: tpd_probe ft5x0x\n");
    /* set INT mode */
    tpd_gpio_as_int(tpd_int_gpio_number);
    msleep(100);

#ifdef CONFIG_MTK_I2C_EXTENSION
    msg_dma_alloct();
#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
    if (NULL == tpd_i2c_dma_va)
    {
        tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        tpd_i2c_dma_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 250, &tpd_i2c_dma_pa, GFP_KERNEL);
    }
    if (!tpd_i2c_dma_va)
        TPD_ERR("TPD dma_alloc_coherent error!\n");    
#endif

#if FTS_GESTRUE_EN
    fts_Gesture_init(tpd->dev);
#endif

#ifdef MT_PROTOCOL_B
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
		input_mt_init_slots(tpd->dev, MT_MAX_TOUCH_POINTS);
	#else
		input_mt_init_slots(tpd->dev, MT_MAX_TOUCH_POINTS, 2);
	#endif
	//input_set_abs_params(tpd->dev, ABS_MT_TOUCH_MAJOR,0, 255, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, TPD_RES_X, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, TPD_RES_Y, 0, 0);
	//input_set_abs_params(tpd->dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#else
    input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, MT_MAX_TOUCH_POINTS, 0, 0);
#endif

reset_proc:

    err = fts_read_reg(i2c_client, 0x00, &data);
    if (err< 0 || data!=0) // reg0 data running state is 0; other state is not 0
    {
        TPD_ERR("I2C transfer error, line: %d\n", __LINE__);
#ifdef TPD_RESET_ISSUE_WORKAROUND
        if ( ++reset_count < TPD_MAX_RESET_COUNT )
        {
            goto reset_proc;
        }
#endif


#ifdef CONFIG_MTK_I2C_EXTENSION
        msg_dma_release();
#endif
        //gpio_free(tpd_rst_gpio_number);
        gpio_free(tpd_int_gpio_number);
        return -1;
    }
    tpd_load_status = 1;
    tpd_irq_registration();

#ifdef SYSFS_DEBUG
    fts_create_sysfs(fts_i2c_client);
#endif
    //hidi2c_to_stdi2c(fts_i2c_client);

    fts_get_upgrade_array();
#ifdef FTS_CTL_IIC
    if (fts_rw_iic_drv_init(fts_i2c_client) < 0)
        dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
#endif

#ifdef FTS_APK_DEBUG
    fts_create_apk_debug_channel(fts_i2c_client);//include creat proc//*****ITO proc
#endif

    thread_tpd = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if (IS_ERR(thread_tpd))
    {
        retval = PTR_ERR(thread_tpd);
        FTS_DEBUG(TPD_DEVICE " failed to create kernel thread_tpd: %d\n", retval);
    }

    TPD_ERR("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
	
#ifdef TIMER_DEBUG
    init_test_timer();
#endif

#ifdef LCT_ADD_TP_LOCKDOWN_INFO
    ft5x46_ctpm_LockDownInfo_get_from_boot(i2c_client, tp_lockdown_info);
    FTS_DEBUG("tpd_probe, ft5x46_ctpm_LockDownInfo_get_from_boot, tp_lockdown_info=%s\n", tp_lockdown_info);
    ctp_lockdown_status_proc = proc_create(CTP_PROC_LOCKDOWN_FILE, 0644, NULL, &ctp_lockdown_proc_fops);
    if (ctp_lockdown_status_proc == NULL)
    {
        FTS_DEBUG("tpd, create_proc_entry ctp_lockdown_status_proc failed\n");
    }
#endif

    {
        u8 ver;
        fts_read_reg(client, 0xA6, &ver);
        TPD_ERR(TPD_DEVICE " fts_read_reg version : %d\n", ver);
    }

#ifdef TPD_AUTO_UPGRADE
    wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "tp_wakelock");
    {
        err = fts_workqueue_init ();
        if ( err != 0 )
        {
            printk( "fts_workqueue_init failed\n" );
        }
        else
            queue_work ( touch_wq, &fw_update_work );
    }
#endif

	//lichenggang add for tp-link for create proc node
    init_fts_proc();
	//lichenggang add for tp-link fot  fts tp 8B protocol
	fts_write_reg(i2c_client,0x8B,usb_check_state);

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    int ret;

    ret = get_md32_semaphore(SEMAPHORE_TOUCH);
    if (ret < 0)
        pr_err("[TOUCH] HW semaphore reqiure timeout\n");
#endif

#ifdef USB_CHARGE_DETECT  //add by wangyang
    if (ctp_is_probe == 0)
    {
        i2c_smbus_write_i2c_block_data(fts_i2c_client, 0x8B, 1, &b_usb_plugin);
        ctp_is_probe =1;
    }
#endif

    return 0;
}

static int tpd_remove(struct i2c_client *client)
{
    FTS_DEBUG("TPD removed\n");
#ifdef CONFIG_CUST_FTS_APK_DEBUG
    //ft_rw_iic_drv_exit();
#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
    if (tpd_i2c_dma_va)
    {
        dma_free_coherent(NULL, 4096, tpd_i2c_dma_va, tpd_i2c_dma_pa);
        tpd_i2c_dma_va = NULL;
        tpd_i2c_dma_pa = 0;
    }
#endif

#ifdef TPD_AUTO_UPGRADE
    cancel_work_sync(&fw_update_work);
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
    msg_dma_release();
#endif

    //gpio_free(tpd_rst_gpio_number);
    gpio_free(tpd_int_gpio_number);
    return 0;
}

#if FT_ESD_PROTECT
void esd_switch(s32 on)
{
	return;
}
#endif


static int tpd_local_init(void)
{
    int retval = -1;
    FTS_DEBUG("Focaltech FT5x0x I2C Touchscreen Driver...\n");

//wwm start//
#if 1
    tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
    retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
    if (retval != 0)
    {
        FTS_DEBUG("Failed to set reg-vgp6 voltage: %d\n", retval);
        return -1;
    }
#endif
//wwm end//

    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        FTS_DEBUG("unable to add i2c driver.\n");
        return -1;
    }
    /* tpd_load_status = 1; */
    if (tpd_dts_data.use_tpd_button)
    {
        tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
                           tpd_dts_data.tpd_key_dim_local);
    }

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))

    memcpy(tpd_calmat, tpd_def_calmat_local_factory, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_factory, 8 * 4);

    memcpy(tpd_calmat, tpd_def_calmat_local_normal, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_normal, 8 * 4);

#endif

    FTS_DEBUG("end %s, %d\n", __func__, __LINE__);
    tpd_type_cap = 1;

    return 0;
}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client)
{
    s8 ret = -1;
    s8 retry = 0;
    char gestrue_on = 0x01;
    char gestrue_data;
    int i;

    /* FTS_DEBUG("Entering doze mode..."); */
    pr_alert("Entering doze mode...");

    /* Enter gestrue recognition mode */
    ret = fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);
    if (ret < 0)
    {
        /* FTS_DEBUG("Failed to enter Doze %d", retry); */
        pr_alert("Failed to enter Doze %d", retry);
        return ret;
    }
    msleep(30);

    for (i = 0; i < 10; i++)
    {
        fts_read_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, &gestrue_data);
        if (gestrue_data == 0x01)
        {
            doze_status = DOZE_ENABLED;
            /* FTS_DEBUG("FTP has been working in doze mode!"); */
            pr_alert("FTP has been working in doze mode!");
            break;
        }
        msleep(20);
        fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);

    }

    return ret;
}
#endif

static void tpd_resume(struct device *h)
{

	int j =0;
	printk("...fts resume start...\n");
	mutex_lock(&ft_suspend_lock);
	tp_suspend = 0;
	if(1 == atomic_read(&double_enable))
		fts_write_reg(i2c_client, 0xD0,0X00);

    //tpd_gpio_output(tpd_rst_gpio_number, 0);
    // msleep(10);
    //tpd_gpio_output(tpd_rst_gpio_number, 1);
    // msleep(200);
	tpd_gpio_as_int(tpd_int_gpio_number);
	for(j = 0; j < 10; j++)
	{
		input_mt_slot( tpd->dev, j);
		input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, 0);
	}
	input_sync(tpd->dev);
	input_report_key(tpd->dev,BTN_TOUCH,0);
	input_report_key(tpd->dev,BTN_TOOL_FINGER, 0);
	input_sync(tpd->dev);
	enable_irq(touch_irq);
	msleep(50);
	fts_write_reg(i2c_client,0xA5,0x00);
	//lichenggang add gor tp-link fot  fts tp 8B protocol
	fts_write_reg(i2c_client,0x8B,usb_check_state);
	//glove mode
	if(1 == atomic_read(&glove_enable))
	{
		fts_write_reg(i2c_client,0xC0,1);
	}else{
		fts_write_reg(i2c_client,0xC0,0);
	}
	mutex_unlock(&ft_suspend_lock);
#ifdef USB_CHARGE_DETECT
    msleep(120);
    tpd_usb_plugin(b_usb_plugin);
#endif
	printk("....fts resume end...\n");

}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
void tpd_scp_wakeup_enable(bool en)
{
    tpd_scp_doze_en = en;
}

void tpd_enter_doze(void)
{

}
#endif

static void tpd_suspend(struct device *h)
{


    static char data = 0x3;
    u8 read_data = 0;
	int i=0;
    printk("...fts enter suspend\n");
	//#if FTS_GESTRUE_EN
	mutex_lock(&ft_suspend_lock);
	tp_suspend = 1;
	for(i = 0; i < 10; i++)
	{
		input_mt_slot( tpd->dev, i);
		input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, 0);
	}
	input_sync(tpd->dev);
	input_report_key(tpd->dev,BTN_TOUCH,0);
	input_report_key(tpd->dev,BTN_TOOL_FINGER,0);
	input_sync(tpd->dev);
	input_report_key(tpd->dev, KEY_BACK, 0);
	input_sync(tpd->dev);
	input_report_key(tpd->dev, KEY_HOMEPAGE, 0);
	input_sync(tpd->dev);
	input_report_key(tpd->dev, KEY_MENU, 0);
	input_sync(tpd->dev);

	if(1 == atomic_read(&double_enable))
	{
		fts_write_reg(i2c_client, 0xd0, 0x01);
        fts_write_reg(i2c_client, 0xd1, 0xff);
        fts_write_reg(i2c_client, 0xd2, 0xff);
        fts_write_reg(i2c_client, 0xd5, 0xff);
        fts_write_reg(i2c_client, 0xd6, 0xff);
        fts_write_reg(i2c_client, 0xd7, 0xff);
        fts_write_reg(i2c_client, 0xd8, 0xff);
		msleep(20);
		for(i=0;i<10;i++){
			printk("read_data::time=%d",i);
			fts_read_reg(i2c_client,0xd0,&read_data);
			if(read_data == 0x01){
				printk("fts::gesture write 0x01 successful!\n");
				mutex_unlock(&ft_suspend_lock);
				return;
			}else{
				fts_write_reg(i2c_client, 0xd0, 0x01);
                fts_write_reg(i2c_client, 0xd1, 0xff);
                fts_write_reg(i2c_client, 0xd2, 0xff);
                fts_write_reg(i2c_client, 0xd5, 0xff);
                fts_write_reg(i2c_client, 0xd6, 0xff);
                fts_write_reg(i2c_client, 0xd7, 0xff);
                fts_write_reg(i2c_client, 0xd8, 0xff);
                msleep(10);
			}
		}
		if (i >= 9)
        {
            printk("fts gesture write 0x01 to d0 fail \n");
			mutex_unlock(&ft_suspend_lock);
			return;
        }
	}
    disable_irq(touch_irq);
	fts_write_reg(i2c_client,0xA5,data);
	mutex_unlock(&ft_suspend_lock);
	printk("...fts suspend end!\n");
	return;
}

int usb_check(int usb_state)
{
	printk("usb_check_usb_state =%d",usb_state);
	if(usb_check_state != usb_state )		
	{			
		if(usb_state ==1)			
		{				
			fts_write_reg(i2c_client,0x8B,0x01);
			printk("usb enable\n");				
			usb_check_state = usb_state;				
			return 1;			
		}else if(usb_state == 0){				
			fts_write_reg(i2c_client,0x8B,0x00);				
			printk("usb disable\n");				
			usb_check_state = usb_state;				
			return 0;			
		}else{			
			return 0;
		}
	}else{ 		
		return 0;
	}
}

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "FT5x0x",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
    .attrs = {
        .attr = ft5x0x_attrs,
        .num  = ARRAY_SIZE(ft5x0x_attrs),
    },
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
    FTS_DEBUG("MediaTek FT5x0x touch panel driver init\n");
    tpd_get_dts_info();
    if (tpd_driver_add(&tpd_device_driver) < 0)
        FTS_DEBUG("add FT5x0x driver failed\n");

    return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
    FTS_DEBUG("MediaTek FT5x0x touch panel driver exit\n");
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);//subsys_initcall
module_exit(tpd_driver_exit);


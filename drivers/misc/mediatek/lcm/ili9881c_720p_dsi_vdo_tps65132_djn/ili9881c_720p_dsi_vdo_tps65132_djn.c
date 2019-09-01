#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
//#include <platform/mtk_auxadc_sw.h>
//#include <platform/mtk_auxadc_hw.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "mt-plat/upmu_common.h"
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>

#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif

#endif
#endif

/* [liliwen start] open ili9881c log */
//#undef CONFIG_ILI9881_DEBUG
#define CONFIG_ILI9881_DEBUG
/* [liliwen end] */

#ifdef CONFIG_ILI9881_DEBUG
#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(ALWAYS, "[LK/"LOG_TAG"]"" ILI988C: "string, ##args)
#define LCM_LOGD(string, args...)  dprintf(ALWAYS, "[LK/"LOG_TAG"]"" ILI988C: "string, ##args)
#else
#define LCM_LOGI(fmt, args...)  printk("[KERNEL/"LOG_TAG"]"" ILI988C: "fmt, ##args)
#define LCM_LOGD(fmt, args...)  printk("[KERNEL/"LOG_TAG"]"" ILI988C: "fmt, ##args)
#endif
#else
#ifdef BUILD_LK
#define LCM_LOGI(string, args...)
#define LCM_LOGD(string, args...)
#else
#define LCM_LOGI(fmt, args...)
#define LCM_LOGD(fmt, args...)
#endif
#endif

/* TPS65132 */
#ifdef BUILD_LK
#define GPIO_65132_VSN_EN GPIO_LCD_BIAS_ENN_PIN
#define GPIO_65132_VSP_EN GPIO_LCD_BIAS_ENP_PIN
#endif

/* LCDs */
/* [liliwen start] AUXADC */
#define TXD_VOL      (0)
#define UNKNOWN_VOL     (999)
#define TXD_VOL_THRESHOLD_MIN (700)          //1st supply
#define TXD_VOL_THRESHOLD_MAX (1100)
/* [liliwen start] TP904 DVT1 vol */
#define TP904_DVT1_VOLTAGE_MIN (450)
#define TP904_DVT1_VOLTAGE_MAX (750)
/* [liliwen end] */

#define AUX_IN2_NTC  (12) /* to choose different LCDs */
/* [liliwen end] */

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

/* [liliwen] extern get_pcb_version_voltage */
extern int get_pcb_version_voltage(void);

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define set_gpio_lcd_enp(cmd) \
		lcm_util.set_gpio_lcd_enp_bias(cmd)
#define set_gpio_lcd_enn(cmd) \
		lcm_util.set_gpio_lcd_enn_bias(cmd)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

/*****************************************************************************
 * Define
 *****************************************************************************/

extern int mt_set_gpio_out(unsigned long pin, unsigned long output);
extern int mt_set_gpio_mode(unsigned long pin, unsigned long mode);
extern int mt_set_gpio_dir(unsigned long pin, unsigned long dir);

/* [liliwen start] remove tps65132 driver */
#if 0
#define I2C_I2C_LCD_BIAS_CHANNEL 0
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL	/* for I2C channel 0 */
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E
/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_client *tps65132_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/
struct tps65132_dev {
	struct i2c_client *client;
};

static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	LCM_LOGI("tps65132_iic_probe\n");
	LCM_LOGI("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	tps65132_i2c_client = client;
	return 0;
}

static int tps65132_remove(struct i2c_client *client)
{
	LCM_LOGI("tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

/* for LCDs, FAQ 12444 */
int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	char write_data[2] = { 0 };
	struct i2c_client *client = tps65132_i2c_client;
	if (tps65132_i2c_client == NULL) {
		printk("client is null %s \n",__func__);
		return -1;
	}

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		LCM_LOGI("tps65132 write data fail !!\n");
	return ret;
}
EXPORT_SYMBOL_GPL(tps65132_write_bytes);

static const struct i2c_device_id tps65132_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

static const struct of_device_id lcm_of_match[] = {
	{ .compatible = "mediatek,i2c_lcd_bias" },
	{},
};

static struct i2c_driver tps65132_iic_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "tps65132",
		   .of_match_table = lcm_of_match,
		   },
	.id_table = tps65132_id,
	.probe = tps65132_probe,
	.remove = tps65132_remove,
};

static int __init tps65132_iic_init(void)
{
	LCM_LOGI("tps65132_iic_init\n");
	i2c_add_driver(&tps65132_iic_driver);
	LCM_LOGI("tps65132_iic_init success\n");
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	LCM_LOGI("tps65132_iic_exit\n");
	i2c_del_driver(&tps65132_iic_driver);
}

module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");
#endif

extern int tps65132_write_bytes(unsigned char addr, unsigned char value);
/* [liliwen end] */
#endif

#define LCM_DSI_CMD_MODE	(0)
#define FRAME_WIDTH		(720)
#define FRAME_HEIGHT		(1280)

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY		0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting[] = {
	{0xFF, 3, {0x98, 0x81, 0x03}},
	{0x01, 1, {0x00}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x73}},
	{0x04, 1, {0x73}},
	{0x05, 1, {0x00}},
	{0x06, 1, {0x06}},
	{0x07, 1, {0x02}},
	{0x08, 1, {0x00}},
	{0x09, 1, {0x01}},
	{0x0a, 1, {0x01}},
	{0x0b, 1, {0x01}},
	{0x0c, 1, {0x01}},
	{0x0d, 1, {0x01}},
	{0x0e, 1, {0x01}},
	{0x0f, 1, {0x2F}},
	{0x10, 1, {0x2F}},
	{0x11, 1, {0x00}},
	{0x12, 1, {0x00}},
	{0x13, 1, {0x01}},
	{0x14, 1, {0x00}},
	{0x15, 1, {0x00}},
	{0x16, 1, {0x00}},
	{0x17, 1, {0x00}},
	{0x18, 1, {0x00}},
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x00}},
	{0x1b, 1, {0x00}},
	{0x1c, 1, {0x00}},
	{0x1d, 1, {0x00}},
	{0x1e, 1, {0xC0}},
	{0x1f, 1, {0x80}},
	{0x20, 1, {0x03}},
	{0x21, 1, {0x03}},
	{0x22, 1, {0x00}},
	{0x23, 1, {0x00}},
	{0x24, 1, {0x00}},
	{0x25, 1, {0x00}},
	{0x26, 1, {0x00}},
	{0x27, 1, {0x00}},
	{0x28, 1, {0x33}},
	{0x29, 1, {0x02}},
	{0x2a, 1, {0x00}},
	{0x2b, 1, {0x00}},
	{0x2c, 1, {0x00}},
	{0x2d, 1, {0x00}},
	{0x2e, 1, {0x00}},
	{0x2f, 1, {0x00}},
	{0x30, 1, {0x00}},
	{0x31, 1, {0x00}},
	{0x32, 1, {0x00}},
	{0x33, 1, {0x00}},
	{0x34, 1, {0x03}},
	{0x35, 1, {0x00}},
	{0x36, 1, {0x03}},
	{0x37, 1, {0x00}},
	{0x38, 1, {0x00}},
	{0x39, 1, {0x00}},
	{0x3a, 1, {0x00}},
	{0x3b, 1, {0x00}},
	{0x3c, 1, {0x00}},
	{0x3d, 1, {0x00}},
	{0x3e, 1, {0x00}},
	{0x3f, 1, {0x00}},
	{0x40, 1, {0x00}},
	{0x41, 1, {0x00}},
	{0x42, 1, {0x00}},
	{0x43, 1, {0x01}},
	{0x44, 1, {0x00}},
	{0x50, 1, {0x01}},
	{0x51, 1, {0x23}},
	{0x52, 1, {0x45}},
	{0x53, 1, {0x67}},
	{0x54, 1, {0x89}},
	{0x55, 1, {0xab}},
	{0x56, 1, {0x01}},
	{0x57, 1, {0x23}},
	{0x58, 1, {0x45}},
	{0x59, 1, {0x67}},
	{0x5a, 1, {0x89}},
	{0x5b, 1, {0xab}},
	{0x5c, 1, {0xcd}},
	{0x5d, 1, {0xef}},
	{0x5e, 1, {0x10}},
	{0x5f, 1, {0x09}},
	{0x60, 1, {0x08}},
	{0x61, 1, {0x0F}},
	{0x62, 1, {0x0E}},
	{0x63, 1, {0x0D}},
	{0x64, 1, {0x0C}},
	{0x65, 1, {0x02}},
	{0x66, 1, {0x02}},
	{0x67, 1, {0x02}},
	{0x68, 1, {0x02}},
	{0x69, 1, {0x02}},
	{0x6a, 1, {0x02}},
	{0x6b, 1, {0x02}},
	{0x6c, 1, {0x02}},
	{0x6d, 1, {0x02}},
	{0x6e, 1, {0x02}},
	{0x6f, 1, {0x02}},
	{0x70, 1, {0x02}},
	{0x71, 1, {0x06}},
	{0x72, 1, {0x07}},
	{0x73, 1, {0x02}},
	{0x74, 1, {0x02}},
	{0x75, 1, {0x06}},
	{0x76, 1, {0x07}},
	{0x77, 1, {0x0E}},
	{0x78, 1, {0x0F}},
	{0x79, 1, {0x0C}},
	{0x7a, 1, {0x0D}},
	{0x7b, 1, {0x02}},
	{0x7c, 1, {0x02}},
	{0x7d, 1, {0x02}},
	{0x7e, 1, {0x02}},
	{0x7f, 1, {0x02}},
	{0x80, 1, {0x02}},
	{0x81, 1, {0x02}},
	{0x82, 1, {0x02}},
	{0x83, 1, {0x02}},
	{0x84, 1, {0x02}},
	{0x85, 1, {0x02}},
	{0x86, 1, {0x02}},
	{0x87, 1, {0x09}},
	{0x88, 1, {0x08}},
	{0x89, 1, {0x02}},
	{0x8A, 1, {0x02}},
	{0xFF, 3, {0x98, 0x81, 0x04}},
	{0x6C, 1, {0x15}},
	{0x6E, 1, {0x2A}},
	{0x6F, 1, {0x57}},
	{0x3A, 1, {0xA4}},
	{0x8D, 1, {0x1A}},
	{0x87, 1, {0xBA}},
	{0x26, 1, {0x76}},
	{0xB2, 1, {0xD1}},
	{0x88, 1, {0x0B}},
	{0x3C, 1, {0x81}},
	{0xFF, 3, {0x98, 0x81, 0x01}},
	{0x22, 1, {0x0A}},
	{0x31, 1, {0x00}},
	{0x53, 1, {0x43}},
	{0x55, 1, {0x50}},
	{0x50, 1, {0xA8}},
	{0x51, 1, {0xA8}},
	{0x60, 1, {0x06}},
	{0x62, 1, {0x20}},
	{0xA0, 1, {0x00}},
	{0xA1, 1, {0x27}},
	{0xA2, 1, {0x3B}},
	{0xA3, 1, {0x1A}},
	{0xA4, 1, {0x1F}},
	{0xA5, 1, {0x34}},
	{0xA6, 1, {0x27}},
	{0xA7, 1, {0x26}},
	{0xA8, 1, {0xAD}},
	{0xA9, 1, {0x1C}},
	{0xAA, 1, {0x27}},
	{0xAB, 1, {0x83}},
	{0xAC, 1, {0x19}},
	{0xAD, 1, {0x18}},
	{0xAE, 1, {0x4D}},
	{0xAF, 1, {0x24}},
	{0xB0, 1, {0x29}},
	{0xB1, 1, {0x4B}},
	{0xB2, 1, {0x56}},
	{0xB3, 1, {0x23}},
	{0xC0, 1, {0x00}},
	{0xC1, 1, {0x27}},
	{0xC2, 1, {0x3B}},
	{0xC3, 1, {0x1A}},
	{0xC4, 1, {0x1F}},
	{0xC5, 1, {0x34}},
	{0xC6, 1, {0x26}},
	{0xC7, 1, {0x27}},
	{0xC8, 1, {0xAD}},
	{0xC9, 1, {0x1C}},
	{0xCA, 1, {0x27}},
	{0xCB, 1, {0x83}},
	{0xCC, 1, {0x19}},
	{0xCD, 1, {0x18}},
	{0xCE, 1, {0x4D}},
	{0xCF, 1, {0x24}},
	{0xD0, 1, {0x29}},
	{0xD1, 1, {0x4B}},
	{0xD2, 1, {0x56}},
	{0xD3, 1, {0x14}},
	{0xFF, 3, {0x98, 0x81, 0x00}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {} },
	{0x29, 1, {0x00}},
	{0x35, 1, {0x00}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif

#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 350;	/* this value must be in MTK suggested table */
#else
	/* [liliwen edit] Change PLL_CLOCK */
	params->dsi.PLL_CLOCK = 175;	/* this value must be in MTK suggested table */
#endif
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif

	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 5;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 10;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 5;
	params->dsi.horizontal_backporch = 35;
	params->dsi.horizontal_frontporch = 35;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;


	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

#ifdef BUILD_LK

/* [liliwen start] extern TPS65132_write_byte */
#if 0
#define TPS65132_SLAVE_ADDR_WRITE  0x7C
#define I2C_I2C_LCD_BIAS_CHANNEL 0
#define I2C_ID_NAME "tps65132"

static struct mt_i2c_t TPS65132_i2c;

int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;//I2C1
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
	TPS65132_i2c.mode = ST_MODE;
	TPS65132_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65132_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	return ret_code;
}
EXPORT_SYMBOL_GPL(TPS65132_write_byte);
#endif

extern int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value);
/* [liliwen end] */
#endif

static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;

	cmd = 0x00;
	data = 0x0F;

#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO_65132_VSP_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_VSP_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_VSP_EN, GPIO_OUT_ONE);
	MDELAY(2);
	mt_set_gpio_mode(GPIO_65132_VSN_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_VSN_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_VSN_EN, GPIO_OUT_ONE);
#else
	set_gpio_lcd_enp(1);
	set_gpio_lcd_enn(1);
#endif
#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
#else
	ret = tps65132_write_bytes(cmd, data);
#endif

	if (ret < 0)
		LCM_LOGI("ili9881c----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("ili9881c----tps6132----cmd=%0x--i2c write success----\n", cmd);

	cmd = 0x01;
	data = 0x0F;

#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
#else
	ret = tps65132_write_bytes(cmd, data);
#endif

	if (ret < 0)
		LCM_LOGI("ili9881c----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		LCM_LOGI("ili9881c----tps6132----cmd=%0x--i2c write success----\n", cmd);

	MDELAY(5);
	SET_RESET_PIN(1);
	UDELAY(100);
	SET_RESET_PIN(0);
	UDELAY(100);

	SET_RESET_PIN(1);
	MDELAY(10);

	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(0);
	MDELAY(1);

#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO_65132_VSN_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_VSN_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_VSN_EN, GPIO_OUT_ZERO);
	mt_set_gpio_mode(GPIO_65132_VSP_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_VSP_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_VSP_EN, GPIO_OUT_ZERO);
#else
	set_gpio_lcd_enp(0);
	set_gpio_lcd_enn(0);
#endif
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static int get_lcd_id(void)
{
	int ret = 0, data[4], ret_value = 0;
	/* [liliwen start] Change channel */
	int Channel = AUX_IN2_NTC;
	/* [liliwen end] */
	static int valid_id = -1;
	/* [liliwen] pcb_version_voltage */
	int pcb_version_voltage = 0;

	ret_value = IMM_GetOneChannelValue(Channel, data, &ret);
	if (ret_value == -1) {/* AUXADC is busy */
		ret = valid_id;
	} else {
		valid_id = ret;
	}

	LCM_LOGI("[lcd_auxadc_get_data(AUX_IN2_NTC)]: ret=%d\n", ret);

	/* Mt_auxadc_hal.c */
	/* #define VOLTAGE_FULL_RANGE  1500 // VA voltage */
	/* #define AUXADC_PRECISE      4096 // 12 bits */
	if (ret != -1) {
		ret = ret * 1500 / 4096;
	}
	/* ret = ret*1800/4096;//82's ADC power */
	LCM_LOGI("APtery output mV = %d\n", ret);

	/* [liliwen] AUXADC compare */
	if (ret > TXD_VOL_THRESHOLD_MIN && ret < TXD_VOL_THRESHOLD_MAX) {
		/* [liliwen start] get_pcb_version_voltage */
		pcb_version_voltage = get_pcb_version_voltage();
		if (pcb_version_voltage > TP904_DVT1_VOLTAGE_MIN && pcb_version_voltage < TP904_DVT1_VOLTAGE_MAX) {
			ret = TXD_VOL;
		} else {
			ret = UNKNOWN_VOL;
		}
		/* [liliwen end] */
	} else {
		ret = UNKNOWN_VOL;
	}

	return ret;
}

static unsigned int lcm_compare_id(void)
{
	int ret = 0;
	ret = get_lcd_id();

	/* [liliwen] txd lcm compare */
	if (ret == TXD_VOL) {
		return 1;  /* txd LCD */
	} else{
		return 0;
	}
}

/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		LCM_LOGI("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;	/* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	    && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

LCM_DRIVER ili9881c_720p_dsi_vdo_tps65132_djn_lcm_drv = {
	.name = "ili9881c_720p_dsi_vdo_tps65132_djn_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
};

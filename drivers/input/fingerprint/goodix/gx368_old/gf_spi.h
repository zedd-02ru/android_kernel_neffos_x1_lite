#ifndef __GF_SPI_H
#define __GF_SPI_H

#include <linux/types.h>
#include <linux/notifier.h>
#include "mt_spi.h"
/**********************************************************/
#define  USE_SPI_BUS	1
#define  GF_FASYNC 		1//If support fasync mechanism.
//[jiangtingyu start] improve finger lightup speed
#define GF_FAST_LIGHTUP      0//If support fast lightup screen mechanism.
//[jiangtingyu end]

/****************Chip Specific***********************/
#define GF_W          	0xF0
#define GF_R          	0xF1
#define GF_WDATA_OFFSET	(0x3)
#define GF_RDATA_OFFSET	(0x5)

#define MTK_SPI_ALIGN_MASK_NUM  10
#define MTK_SPI_ALIGN_MASK  ((0x1 << MTK_SPI_ALIGN_MASK_NUM) - 1)

enum gf_spi_transfer_speed {
	GF_SPI_LOW_SPEED = 0,
	GF_SPI_HIGH_SPEED,
	GF_SPI_KEEP_SPEED,
};

struct gf_configs {
    unsigned short addr;
    unsigned short value;
};

struct gf_mode_config {
    struct gf_configs *p_cfg;
    unsigned int cfg_len;
};

#define  GF_IOC_MAGIC    'g'
#define  GF_IOC_RESET	     _IO(GF_IOC_MAGIC, 0)
#define  GF_IOC_RW	         _IOWR(GF_IOC_MAGIC, 1, struct gf_ioc_transfer)
#define  GF_IOC_CMD          _IOW(GF_IOC_MAGIC,2,unsigned char)
#define  GF_IOC_CONFIG       _IOW(GF_IOC_MAGIC,3,void*)
#define  GF_IOC_ENABLE_IRQ   _IO(GF_IOC_MAGIC,4)
#define  GF_IOC_DISABLE_IRQ  _IO(GF_IOC_MAGIC,5)
#define  GF_IOC_SENDKEY      _IOW(GF_IOC_MAGIC,6,struct gf_key)
#define  GF_IOC_SETSPEED	_IOW(GF_IOC_MAGIC,7,enum gf_spi_transfer_speed)
#define  GF_IOC_MAXNR        8

struct gf_ioc_transfer {
    unsigned char cmd;
    unsigned char reserve;
    unsigned short addr;
    unsigned int len;
    unsigned char* buf;
};

struct gf_key {
    unsigned int key;
    int value;
};

struct gf_key_map
{
    char *name;
    unsigned short val;
    unsigned short val_oem; // [taoyancheng] Add for oem keys
};

struct gf_dev {
    dev_t devt;
    spinlock_t   spi_lock;
    struct list_head device_entry;
#if defined(USE_SPI_BUS)
    struct spi_device *spi;
#elif defined(USE_PLATFORM_BUS)
    struct platform_device *spi;
#endif
    struct clk *core_clk;
    struct clk *iface_clk;

    struct mt_chip_conf spi_mcc;

    struct input_dev *input;
    /* buffer is NULL unless this device is open (users > 0) */
    unsigned users;
    signed irq_gpio;
    signed reset_gpio;
    signed pwr_gpio;
    int irq;
    int irq_num;
    int irq_enabled;
    int clk_enabled;
#ifdef GF_FASYNC
    struct fasync_struct *async;
#endif
    struct notifier_block notifier;
    char device_available;
    char fb_black;
    unsigned char *gBuffer;
    struct mutex buf_lock;
    struct mutex frame_lock;
#ifdef CONFIG_OF
    struct pinctrl *pinctrl_gpios;
/* [taoyancheng] Add oem gpio config */
    struct pinctrl_state *pins_irq, *pins_irq_pull_down;
//    struct pinctrl_state *pins_miso_spi, *pins_miso_pullhigh, *pins_miso_pulllow;
    struct pinctrl_state *pins_reset_high, *pins_reset_low;
    struct pinctrl_state *pins_power_on, *pins_power_off;
    struct pinctrl_state *pins_spi_default;
#endif
};

// [taoyancheng] Use old gpio config for EVT/DVT1 devices of 902A/903A
int  gf_parse_dts(struct gf_dev* gf_dev, bool old_gpio_config);
void gf_cleanup(struct gf_dev *gf_dev);

int  gf_power_on(struct gf_dev *gf_dev, bool old_gpio_config);
int  gf_power_off(struct gf_dev *gf_dev, bool old_gpio_config);

int  gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms);
int  gf_irq_num(struct gf_dev *gf_dev);

#endif /*__GF_SPI_H*/

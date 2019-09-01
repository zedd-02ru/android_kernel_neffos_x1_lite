#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

/* [taoyancheng] Modified to avoid a build error: 
 * spi.h/spidev.h should be included before
 * mt_spi.h/mt_spi_hal.h, which is included in
 * gf_spi.h. But USE_SPI_BUS is also defined in
 * gf_spi.h, So just move it forward.
*/
//#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
//#elif defined(USE_PLATFORM_BUS)
//#include <linux/platform_device.h>
//#endif

#include "gf_spi.h"

/*GPIO pins reference.*/
int gf_parse_dts(struct gf_dev* gf_dev, bool old_gpio_config)
{
    struct device_node *node;
    struct platform_device *pdev = NULL;
    int ret = 0;

    node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
    if (node) {
        pdev = of_find_device_by_node(node);
        if (pdev) {
            gf_dev->pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
            if (IS_ERR(gf_dev->pinctrl_gpios)) {
                ret = PTR_ERR(gf_dev->pinctrl_gpios);
                pr_info("%s can't find fingerprint pinctrl\n", __func__);
                return ret;
            }
        } else {
            pr_info("%s platform device is null\n", __func__);
        }
    } else {
        pr_info("%s device node is null\n", __func__);
    }
/* [taoyancheng start] Add oem gpio config */
    gf_dev->pins_irq = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "irq");
    if (IS_ERR(gf_dev->pins_irq)) {
        ret = PTR_ERR(gf_dev->pins_irq);
        pr_info("%s can't find fingerprint pinctrl irq\n", __func__);
        return ret;
    }
    gf_dev->pins_irq_pull_down = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "irq_pull_down");
    if (IS_ERR(gf_dev->pins_irq_pull_down)) {
        ret = PTR_ERR(gf_dev->pins_irq_pull_down);
        pr_info("%s can't find fingerprint pinctrl irq_pull_down\n", __func__);
        return ret;
    }
    gf_dev->pins_spi_default = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "spi_default");
    if (IS_ERR(gf_dev->pins_spi_default)) {
        ret = PTR_ERR(gf_dev->pins_spi_default);
        pr_info("%s can't find fingerprint pinctrl spi_default\n", __func__);
        return ret;
    }
// [taoyancheng] Use old gpio config for EVT/DVT1 devices of 902A/903A
    if (old_gpio_config) {
        gf_dev->pins_reset_high = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_high_old");
        if (IS_ERR(gf_dev->pins_reset_high)) {
            ret = PTR_ERR(gf_dev->pins_reset_high);
            pr_info("%s can't find fingerprint pinctrl reset_high\n", __func__);
            return ret;
        }
        gf_dev->pins_reset_low = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_low_old");
        if (IS_ERR(gf_dev->pins_reset_low)) {
            ret = PTR_ERR(gf_dev->pins_reset_low);
            pr_info("%s can't find fingerprint pinctrl reset_low\n", __func__);
            return ret;
        }
    } else {
        gf_dev->pins_reset_high = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_high");
        if (IS_ERR(gf_dev->pins_reset_high)) {
            ret = PTR_ERR(gf_dev->pins_reset_high);
            pr_info("%s can't find fingerprint pinctrl reset_high\n", __func__);
            return ret;
        }
        gf_dev->pins_reset_low = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "reset_low");
        if (IS_ERR(gf_dev->pins_reset_low)) {
            ret = PTR_ERR(gf_dev->pins_reset_low);
            pr_info("%s can't find fingerprint pinctrl reset_low\n", __func__);
            return ret;
        }
        gf_dev->pins_power_on = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "power_on");
        if (IS_ERR(gf_dev->pins_power_on)) {
            ret = PTR_ERR(gf_dev->pins_power_on);
            pr_info("%s can't find fingerprint pinctrl power_on\n", __func__);
            return ret;
        }
        gf_dev->pins_power_off = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "power_off");
        if (IS_ERR(gf_dev->pins_power_off)) {
            ret = PTR_ERR(gf_dev->pins_power_off);
            pr_info("%s can't find fingerprint pinctrl power_off\n", __func__);
            return ret;
        }
    }
/* [taoyancheng end] */
    pr_info("%s, get pinctrl success!\n", __func__);
    return 0;
}

void gf_cleanup(struct gf_dev* gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }
}


/*power management*/
int gf_power_on(struct gf_dev* gf_dev, bool old_gpio_config)
{
    int rc = 0;
//	hwPowerOn(MT6331_POWER_LDO_VIBR, VOL_2800, "fingerprint");
/* [taoyancheng start] Refer to GX368 spec., do some preparations before power on */
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_irq_pull_down);
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_low);
    msleep(5);
// [taoyancheng] Use old gpio config for EVT/DVT1 devices of 902A/903A
    if (!old_gpio_config) {
        pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_power_on);
    }
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_irq);
    msleep(10);
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);
/* [taoyancheng end] */
    return rc;
}

int gf_power_off(struct gf_dev* gf_dev, bool old_gpio_config)
{
    int rc = 0;
//	hwPowerDown(MT6331_POWER_LDO_VIBR, "fingerprint");
// [taoyancheng] Use old gpio config for EVT/DVT1 devices of 902A/903A
    if (!old_gpio_config) {
        pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_power_off); // [taoyancheng] Cut off power supply
    }
    return rc;
}


/********************************************************************
*CPU output low level in RST pin to reset GF. This is the MUST action for GF.
*Take care of this function. IO Pin driver strength / glitch and so on.
********************************************************************/
int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_low);
    mdelay((delay_ms > 10)?delay_ms:10); // [taoyancheng] Increase to 10 ms according to spec.
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
    struct device_node *node;

    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_irq);

    node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
    if (node) {
        gf_dev->irq_num = irq_of_parse_and_map(node, 0);
        pr_info("%s, gf_irq = %d\n", __func__, gf_dev->irq_num);
        gf_dev->irq = gf_dev->irq_num;
    } else
        pr_info("%s can't find compatible node\n", __func__);
    return gf_dev->irq;
}


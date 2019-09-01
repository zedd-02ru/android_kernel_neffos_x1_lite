/*
 * Richtek RT9460 Charger Header File
 *
 * Copyright (C) 2017, Richtek Technology Corp.
 * Author: CY Hunag <cy_huang@richtek.com>
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

#ifndef __I2C_CHARGER_RT9460_H
#define __I2C_CHARGER_RT9460_H

struct rt9460_platform_data {
	const char *chg_name;
	u32 ichg;
	u32 aicr;
	u32 mivr;
	u32 ieoc;
	u32 voreg;
	u32 wt_fc;
	u32 wt_prc;
	u32 twdt;
	u32 eoc_timer;
	int intr_gpio;
	u8 enable_te:1;
	u8 disable_mivr:1;
	u8 enable_ccjeita:1;
	u8 enable_hwjeita:1;
	u8 enable_batd:1;
	u8 enable_irq_pulse:1;
	u8 enable_wdt:1;
	u8 enable_safetimer:1;
};

#define RT9460_REG_CTRL1	(0x00)
#define RT9460_REG_CTRL2	(0x01)
#define RT9460_REG_CTRL3	(0x02)
#define RT9460_REG_DEVID	(0x03)
#define RT9460_REG_CTRL4	(0x04)
#define RT9460_REG_CTRL5	(0x05)
#define RT9460_REG_CTRL6	(0x06)
#define RT9460_REG_CTRL7	(0x07)
#define RT9460_REG_IRQ1		(0x08)
#define RT9460_REG_IRQ2		(0x09)
#define RT9460_REG_IRQ3		(0x0A)
#define RT9460_REG_MASK1	(0x0B)
#define RT9460_REG_MASK2	(0x0C)
#define RT9460_REG_MASK3	(0x0D)
#define RT9460_REG_CTRLDPDM	(0x0E)
#define RT9460_REG_CTRL8	(0x1C)
#define RT9460_REG_CTRL9	(0x21)
#define RT9460_REG_CTRL10	(0x22)
#define RT9460_REG_CTRL11	(0x23)
#define RT9460_REG_CTRL12	(0x24)
#define RT9460_REG_CTRL13	(0x25)
#define RT9460_REG_STATIRQ	(0x26)
#define RT9460_REG_STATMASK	(0x27)
#define RT9460_REG_HIDDEN1	(0x31)

/* RT9460_REG_IRQ1 ~ IRQ3 + RT9460_REG_STATIRQ */
#define RT9460_IRQ_REGNUM (4)
enum {
	RT9460_IRQ_BATAB = 0,
	RT9460_IRQ_SYSUVP,
	RT9460_IRQ_CHTERM_TMRI,
	RT9460_IRQ_WATCHDOGI = 4,
	RT9460_IRQ_WAKEUPI,
	RT9460_IRQ_VINOVPI = 6,
	RT9460_IRQ_TSDI,
	RT9460_IRQ_SYSWAKEUPI = 8,
	RT9460_IRQ_CHTREGI,
	RT9460_IRQ_CHTMRI,
	RT9460_IRQ_CHRCHGI,
	RT9460_IRQ_CHTERMI,
	RT9460_IRQ_CHBATOVI,
	RT9460_IRQ_CHBADI,
	RT9460_IRQ_CHRVPI,
	RT9460_IRQ_BSTLOWVI = 21,
	RT9460_IRQ_BSTOLI,
	RT9460_IRQ_BSTVINOVI,
	RT9460_IRQ_MIVRI = 26,
	RT9460_IRQ_PWR_RDYI,
	RT9460_IRQ_TSCOLDI,
	RT9460_IRQ_TSCOOLI,
	RT9460_IRQ_TSWARMI,
	RT9460_IRQ_TSHOTI,
	RT9460_IRQ_MAXNUM,
};

/* RT9460_REG_CTRL1 : 0x00 */
#define RT9460_PWRRDY_MASK	(0x04)
#define RT9460_STATEN_MASK	(0x40)
#define RT9460_CHGSTAT_MASK	(0x30)
#define RT9460_CHGSTAT_SHFT	(4)
#define RT9460_CHGSTAT_MAX	(3)

/* RT9460_REG_CTRL2 : 0x01 */
#define RT9460_IEOC_MASK	(0xe0)
#define RT9460_IEOC_SHFT	(5)
#define RT9460_IEOC_MAX		(0x7)
#define RT9460_TEEN_MASK	(0x08)
#define RT9460_IININT_MASK	(0x04)
#define RT9460_CHGHZ_MASK	(0x02)
#define RT9460_OPAMODE_MASK	(0x01)

/* RT9460_REG_CTRL3 : 0x02 */
#define RT9460_VOREG_MASK	(0xfc)
#define RT9460_VOREG_SHFT	(2)
#define RT9460_VOREG_MAX	(0x38)

/* RT9460_REG_CTRL5 : 0x05 */
#define RT9460_OTGOC_MASK	(0x40)

/* RT9460_REG_CTRL6 : 0x06 */
#define RT9460_ICHG_MASK	(0xf0)
#define RT9460_ICHG_SHFT	(4)
#define RT9460_ICHG_MAX		(0xf)

/* RT9460_REG_CTRL7 : 0x07 */
#define RT9460_TSCOLD_MASK	(0x01)
#define RT9460_TSCOOL_MASK	(0x02)
#define RT9460_TSWARM_MASK	(0x04)
#define RT9460_TSHOT_MASK	(0x08)
/* MTK proprietary ++ */
/* MTK power patch mean rt chipen, charger enable mean rt power path_en */
#define RT9460_CHGEN_MASK	(0x20)
#define RT9460_CHIPEN_MASK	(0x10)
/* MTK proprietary -- */
#define RT9460_BATDEN_MASK	(0x40)
#define RT9460_CCJEITAEN_MASK	(0x80)

/* RT9460_REG_CTRLDPDM : 0x0E */
#define RT9460_IINLMTSEL_MASK	(0x18)
#define RT9460_IINLMTSEL_SHFT	(3)

/* RT9460_REG_CTRL9 : 0x21 */
#define RT9460_MIVRLVL_MASK	(0x0f)
#define RT9460_MIVRLVL_SHFT	(0)
#define RT9460_MIVRLVL_MAX	(0x0b)
#define RT9460_MIVRDIS_MASK	(0x10)

/* RT9460_REG_CTRL10 : 0x22 */
#define RT9460_WTFC_MASK	(0x38)
#define RT9460_WTFC_SHFT	(3)
#define RT9460_WTFC_MAX		(0x7)
#define RT9460_WTPRC_MASK	(0x06)
#define RT9460_WTPRC_SHFT	(1)
#define RT9460_WTPRC_MAX	(0x3)
#define RT9460_TMRPAUSE_MASK	(0x01)

/* RT9460_REG_CTRL11 : 0x23 */
#define RT9460_AICR_MASK	(0xf8)
#define RT9460_AICR_SHFT	(3)
#define RT9460_AICR_MAX		(0x1f)

/* RT9460_REG_CTRL12 : 0x24 */
#define RT9460_EOCTIMER_MASK	(0xc0)
#define RT9460_EOCTIMER_SHFT	(6)
#define RT9460_EOCTIMER_MAX	(0x3)
#define RT9460_WKTMREN_MASK	(0x04)
#define RT9460_IRQPULSE_MASK	(0x02)
#define RT9460_IRQREZ_MASK	(0x01)

/* RT9460_REG_CTRL13 : 0x25 */
#define RT9460_TWDT_MASK	(0x03)
#define RT9460_TWDT_SHFT	(0)
#define RT9460_TWDT_MAX		(0x3)
#define RT9460_WDTEN_MASK	(0x80)

#endif /* #ifndef __I2C_CHARGER_RT9460_H */

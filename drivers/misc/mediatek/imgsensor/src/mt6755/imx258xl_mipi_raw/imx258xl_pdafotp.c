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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>


#ifdef IMX258XL_PDAFOTP_DEBUG
#define PFX "IMX258XL_pdafotp"
#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#else
#define LOG_INF(format, args...)
#endif

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define IMX258XL_EEPROM_READ_ID  0xA0
#define IMX258XL_EEPROM_WRITE_ID   0xA1
#define IMX258XL_I2C_SPEED        400
#define IMX258XL_MAX_OFFSET		0xFFFF

#define DATA_SIZE 2048
BYTE imx258xl_eeprom_data[DATA_SIZE]= {0};
BYTE imx258xl_eeprom_pdaf_data[1500] = {0};
BYTE imx258xl_eeprom_spc_data[128] = {0};
BYTE imx258xl_eeprom_awb_data[16] = {0};

static bool get_done = false;
static bool pdaf_get_done = false;
static bool spc_get_done = false;
static bool awb_get_done = false;
static int last_size = 0;
static int last_offset = 0;


static bool selective_read_eeprom(kal_uint16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    if(addr > IMX258XL_MAX_OFFSET)
        return false;

	kdSetI2CSpeed(IMX258XL_I2C_SPEED);

	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, IMX258XL_EEPROM_READ_ID)<0)
		return false;
    return true;
}

static bool _read_imx258xl_eeprom(kal_uint16 addr, BYTE* data, kal_uint32 size ){
	int i = 0;
	int offset = addr;
	for(i = 0; i < size; i++) {
		if(!selective_read_eeprom(offset, &data[i])){
			return false;
		}
		LOG_INF("read_eeprom 0x%0x %d\n",offset, data[i]);
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
    return true;
}

bool read_imx258xl_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size){
	BYTE flag;

	if(!pdaf_get_done) {
		if (!selective_read_eeprom(0x0800, &flag)) {
			pr_err("Read pdaf flag error\n");
			return false;
		}

		if (flag != 0x03) {
			pr_err("Flag of PDAF Proc  is wrong\n");
			return false;
		}

		if(!_read_imx258xl_eeprom(0x0801, imx258xl_eeprom_data, 1404)){
			pr_err("Read pdaf eeprom error\n");
			return false;
		}

		pr_err("PDAF data read ok\n");
		memcpy(imx258xl_eeprom_pdaf_data, imx258xl_eeprom_data, 1404);
		pdaf_get_done = 1;
	}

	memcpy(data, imx258xl_eeprom_pdaf_data, 1404);
	return true;
}

bool read_imx258xl_eeprom_SPC( kal_uint16 addr, BYTE* data, kal_uint32 size){
	BYTE flag;
	int i, checksum=0, sum=0;

	if (!spc_get_done) {
		if (!selective_read_eeprom(0x1B7E, &flag)){
			pr_err("Read spc flag error\n");
			return false;
		}

		if(flag != 0x01) {
			pr_err("Flag of SPC is wrong");
			return false;
		}

		if(!_read_imx258xl_eeprom(0x1B00, imx258xl_eeprom_data, 128)){
			pr_err("Read spc eeprom error\n");
			return false;
		}

		for(i=0; i<126; i++) {
			sum += (int)(imx258xl_eeprom_data[i]);
		}

		checksum = imx258xl_eeprom_data[127];

		if (checksum != (sum%0xFF)) {
			pr_err("checksum of SPC is wrong\n");
			return false;
		}

		pr_err("SPC data read ok\n");

		memcpy(imx258xl_eeprom_spc_data, imx258xl_eeprom_data+1, 126);
		spc_get_done = 1;
	}
	memcpy(data, imx258xl_eeprom_spc_data, 126);
	return true;
}

bool read_imx258xl_eeprom_AWB(kal_uint16 addr, BYTE* data, kal_uint32 size){
	BYTE flag;
	int i, checksum=0, sum=0;
	if (!awb_get_done) {
		if (!selective_read_eeprom(0x002B, &flag)){
			pr_err("Read awb flag error\n");
			return false;
		}

		if (flag != 0x11) {
			pr_err("Flag of AWB is wrong");
			return false;
		}


		if(!_read_imx258xl_eeprom(0x002C, imx258xl_eeprom_data, 19)){
			pr_err("Read spc awb error\n");
			return false;
		}

		for(i=0; i<17; i++) {
			sum += (int)(imx258xl_eeprom_data[i]);
		}

		checksum = (imx258xl_eeprom_data[18] << 8) + imx258xl_eeprom_data[17];

		if (checksum != (sum%0xFFFF + 1)) {
			pr_err("checksum of AWB is wrong\n");
			return false;
		}

		pr_err("AWB data read ok\n");

		memcpy(imx258xl_eeprom_awb_data, imx258xl_eeprom_data, 12);
		awb_get_done = 1;
	}
	memcpy(data, imx258xl_eeprom_awb_data, 12);
	return true;
}

int read_imx258xl_eeprom_MID(void)
{
	BYTE mid;
	BYTE data[128];

	/* should check SPC firstly */
	spc_get_done = 0;
	read_imx258xl_eeprom_SPC(0x1B00, data, 126);

	if (!spc_get_done) {
		return 0;
	} else {
		if (!selective_read_eeprom(0x0004, &mid)) {
			pr_err("Read mid error\n");
			return 0;
		}
	}

	return (int)mid;
}
	

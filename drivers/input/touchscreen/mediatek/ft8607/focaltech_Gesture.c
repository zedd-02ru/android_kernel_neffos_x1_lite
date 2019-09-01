/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*******************************************************************************
*
* File Name: Focaltech_Gestrue.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*
* Modify by mshl on 2015-03-20
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
#if FTS_GESTRUE_EN
#include "ft_gesture_lib.h"
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include<linux/wakelock.h>
#define  TPD_PACKET_LENGTH                     128
#define PROC_READ_DRAWDATA          10
extern struct i2c_client *i2c_client;
#include "focaltech_drv_printlog.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define GESTURE_LEFT                                0x20
#define GESTURE_RIGHT                               0x21
#define GESTURE_UP                                  0x22
#define GESTURE_DOWN                                0x23
#define GESTURE_DOUBLECLICK                         0x24
#define GESTURE_O                                   0x30
#define GESTURE_W                                   0x31
#define GESTURE_M                                   0x32
#define GESTURE_E                                   0x33
#define GESTURE_L                                   0x44
#define GESTURE_S                                   0x46
#define GESTURE_V                                   0x54
#define GESTURE_Z                                   0x65
#define GESTURE_C                                   0x34
#define FTS_GESTRUE_POINTS                          255
#define FTS_GESTRUE_POINTS_ONETIME                  62
#define FTS_GESTRUE_POINTS_HEADER                   8
#define FTS_GESTURE_OUTPUT_ADRESS                   0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH              4
#define PROC_UPGRADE                                0
#define TS_WAKE_LOCK_TIMEOUT                        (2 * HZ)
unsigned short gesture_id;

unsigned short Drawdata[15] = {0};
//static unsigned char proc_operate_mode = PROC_UPGRADE;

unsigned char buf[FTS_GESTRUE_POINTS * 2] ;
//static unsigned char gesture_point_readbuf[TPD_PACKET_LENGTH];
//static struct i2c_client *client01;
/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
short pointnum                      = 0;
unsigned short coordinate_x[150]    = {0};
unsigned short coordinate_y[150]    = {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

/*******************************************************************************
* Static function prototypes
*******************************************************************************/


/*******************************************************************************
*   Name: fts_Gesture_init
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_Gesture_init(struct input_dev *input_dev)
{
    //init_para(480,854,60,0,0);
    input_set_capability(input_dev, EV_KEY, KEY_POWER);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
    //input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);

    __set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
    __set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
    __set_bit(KEY_GESTURE_UP, input_dev->keybit);
    __set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
    __set_bit(KEY_GESTURE_U, input_dev->keybit);
    __set_bit(KEY_GESTURE_O, input_dev->keybit);
    __set_bit(KEY_GESTURE_E, input_dev->keybit);
    __set_bit(KEY_GESTURE_M, input_dev->keybit);
    __set_bit(KEY_GESTURE_W, input_dev->keybit);
    __set_bit(KEY_GESTURE_L, input_dev->keybit);
    __set_bit(KEY_GESTURE_S, input_dev->keybit);
    __set_bit(KEY_GESTURE_V, input_dev->keybit);
    __set_bit(KEY_GESTURE_Z, input_dev->keybit);
    //__set_bit(KEY_GESTURE_C, input_dev->keybit);
    return 0;
}

#if 0
/*******************************************************************************
*   Name: fts_check_gesture
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_check_gesture(struct input_dev *input_dev,int gesture_id)
{
	u8 read_data;
	int i;
	FTS_DEBUG("fts gesture_id==0x%x\n ",gesture_id);
    switch (gesture_id)
    {
        case GESTURE_LEFT:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_RIGHT:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_UP:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_DOWN:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_DOUBLECLICK:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_O:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_W:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_M:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_E:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_L:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_S:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_V:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_Z:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        case GESTURE_C:
            input_report_key(input_dev, KEY_POWER, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_POWER, 0);
            input_sync(input_dev);
            break;
        default:
			{
				printk("fts_check_gesture 0x01 to 0x00D0!\n");
				fts_write_reg(i2c_client, 0xd0, 0x01);
				msleep(20);
				for(i=0;i<10;i++){
					printk("read_data::time=%d",i);
					fts_read_reg(i2c_client,0xd0,&read_data);
					if(read_data == 0x01){
						printk("fts::gesture write 0x01 successful!\n");
						return;
					}else{
						fts_write_reg(i2c_client, 0xd0, 0x01);
						msleep(10);
					}
				}
				if (i >= 9)
				{
					printk("fts gesture write 0x01 to d0 fail \n");
					return;
				}
			}
	 }

}
#endif


/************************************************************************
*   Name: fts_read_Gestruedata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
void fts_read_Gestruedata(void)        //read data from fw.Drawdata[i] sent to mobile************************************************
{
    //unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
    int ret = -1,i = 0;
    int gesture_id = 0;
	u8 read_data;

    buf[0] = 0xd3;
    pointnum = 0;

    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
    if (ret < 0)
    {
        FTS_DEBUG( "%s read Gestruedata failed.\n", __func__);
        return ;
    }

    // other IC's gestrue in driver
    if (0x24 == buf[0])
    {
        gesture_id = 0x24;
		//FTS_DEBUG( "0x%x check_gesture gesture_id.\n", gesture_id);
		input_report_key(fts_input_dev, KEY_POWER, 1);
        input_sync(fts_input_dev);
        input_report_key(fts_input_dev, KEY_POWER, 0);
        input_sync(fts_input_dev);
        //fts_check_gesture(fts_input_dev,gesture_id);
        //return -1;
    }else{
		  printk("fts_check_gesture 0x01 to 0x00D0!\n");
		  fts_write_reg(i2c_client, 0xd0, 0x01);
		  msleep(20);
		  for(i=0;i<10;i++){
		  	printk("read_data::time=%d",i);
		  	fts_read_reg(i2c_client,0xd0,&read_data);
		  	if(read_data == 0x01){
		  		printk("fts::gesture write 0x01 successful!\n");
		  		return;
		  	}else{
		  		fts_write_reg(i2c_client, 0xd0, 0x01);
		  		msleep(10);
		  	}
		  }
		  if (i >= 9)
		  {
		  	printk("fts gesture write 0x01 to d0 fail \n");
		  	return;
		}
	}
/*  gesture_id = buf[0];
    pointnum = 7;
    buf[0] = 0xd3;

    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 2 +6));
    if (ret < 0)
    {
        FTS_DEBUG( "%s read touchdata failed.\n", __func__);
        return ret;
    }
    fts_check_gesture(fts_input_dev,gesture_id);
    return -1; */
	return;
}
#endif

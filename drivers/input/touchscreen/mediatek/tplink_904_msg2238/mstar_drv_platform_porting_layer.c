////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_platform_porting_layer.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */
 
/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_platform_interface.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_main.h"
#include "mstar_drv_printlog.h"

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#include "mstar_drv_jni_interface.h"
#endif //CONFIG_ENABLE_JNI_INTERFACE

/*=============================================================*/
// EXTREN VARIABLE DECLARATION
/*=============================================================*/

extern struct i2c_client *g_I2cClient;

extern struct kset *g_TouchKSet;
extern struct kobject *g_TouchKObj;



#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
extern struct tpd_device *tpd;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
extern struct regulator *g_ReguVdd;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

extern struct of_device_id touch_dt_match_table[];
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
extern struct regulator *g_ReguVdd;
extern struct regulator *g_ReguVcc_i2c;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM


#ifdef CONFIG_ENABLE_ITO_MP_TEST
extern u32 g_IsInMpTest;
#endif //CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_ESD_PROTECTION
extern u32 SLAVE_I2C_ID_DWI2C;
extern u8 g_IsUpdateFirmware;
extern u8 g_IsHwResetByDriver;
#endif //CONFIG_ENABLE_ESD_PROTECTION

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;

extern u8 g_ChipType;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static spinlock_t _gIrqLock;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
static struct work_struct _gFingerTouchWork;
static int _gIrq = -1;
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
static struct work_struct _gFingerTouchWork;
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
static int _gIrq = -1;

static int MS_TS_MSG_IC_GPIO_RST = 0; // Must set a value other than 1
static int MS_TS_MSG_IC_GPIO_INT = 1; // Must set value as 1
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

static int _gInterruptFlag = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
static int _gGpioRst = 0;
static int _gGpioIrq = 0;
static int MS_TS_MSG_IC_GPIO_RST = 0;
static int MS_TS_MSG_IC_GPIO_INT = 0;

static struct pinctrl *_gTsPinCtrl = NULL;
static struct pinctrl_state *_gPinCtrlStateActive = NULL;
static struct pinctrl_state *_gPinCtrlStateSuspend = NULL;
static struct pinctrl_state *_gPinCtrlStateRelease = NULL;
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_NOTIFIER_FB
static struct notifier_block _gFbNotifier;
#else
static struct early_suspend _gEarlySuspend;
#endif //CONFIG_ENABLE_NOTIFIER_FB
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifndef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
static DECLARE_WAIT_QUEUE_HEAD(_gWaiter);
static struct task_struct *_gThread = NULL;
static int _gTpdFlag = 0;
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
int g_TpVirtualKey[] = {TOUCH_KEY_MENU, TOUCH_KEY_HOME, TOUCH_KEY_BACK, TOUCH_KEY_SEARCH};

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
struct kobject *g_PropertiesKObj = NULL;
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#define BUTTON_W (100)
#define BUTTON_H (100)

int g_TpVirtualKeyDimLocal[MAX_KEY_NUM][4] = {{BUTTON_W/2*1,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {BUTTON_W/2*3,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {BUTTON_W/2*5,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {BUTTON_W/2*7,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H}};
#endif 
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

#ifdef CONFIG_ENABLE_ESD_PROTECTION
int g_IsEnableEsdCheck = 1;
struct delayed_work g_EsdCheckWork;
struct workqueue_struct *g_EsdCheckWorkqueue = NULL;
void DrvPlatformLyrEsdCheck(struct work_struct *pWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

struct input_dev *g_InputDevice = NULL;
struct mutex g_Mutex;

/*=============================================================*/
// GLOBAL FUNCTION DECLARATION
/*=============================================================*/


/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/


#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
static irqreturn_t _DrvPlatformLyrFingerTouchInterruptHandler(s32 nIrq, void *pDeviceId)
{
    unsigned long nIrqFlag;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);
#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    if (_gInterruptFlag == 1
	#ifdef CONFIG_ENABLE_ITO_MP_TEST
			&& g_IsInMpTest == 0
	#endif 
    ) 
    {
        disable_irq_nosync(_gIrq);
        _gInterruptFlag = 0;
        schedule_work(&_gFingerTouchWork);
    }
#else    
    if (_gInterruptFlag == 1
	#ifdef CONFIG_ENABLE_ITO_MP_TEST
			&& g_IsInMpTest == 0
	#endif 
    )
    {    
		disable_irq_nosync(_gIrq);
		_gInterruptFlag = 0;
		_gTpdFlag = 1;
		wake_up_interruptible(&_gWaiter);
    }        
#endif 
    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    return IRQ_HANDLED;
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
}

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
/* read data through I2C then report data to input sub-system when interrupt occurred */
static void _DrvPlatformLyrFingerTouchDoWork(struct work_struct *pWork)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvIcFwLyrHandleFingerTouch(NULL, 0);

    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

    if (_gInterruptFlag == 0
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    ) 
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        enable_irq(_gIrq);
#else
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

        _gInterruptFlag = 1;
    }

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

#else

static int _DrvPlatformLyrFingerTouchHandler(void *pUnUsed)
{
    unsigned long nIrqFlag;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);
	
    do
    {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(_gWaiter, _gTpdFlag != 0);
        _gTpdFlag = 0;
        set_current_state(TASK_RUNNING);
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        if (g_IsInMpTest == 0)
        {
#endif 
       DrvIcFwLyrHandleFingerTouch(NULL, 0);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
        }
#endif 

        spin_lock_irqsave(&_gIrqLock, nIrqFlag);
        if (_gInterruptFlag == 0       
#ifdef CONFIG_ENABLE_ITO_MP_TEST
            && g_IsInMpTest == 0
#endif 
        )
        
        {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            enable_irq(_gIrq);
#else
            mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif 
            _gInterruptFlag = 1;
        } 
        spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);	
    } while (!kthread_should_stop());	
    return 0;
}
#endif 
#endif

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
void DrvPlatformLyrTouchDeviceRegulatorPowerOn(bool nFlag)
{
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    s32 nRetVal = 0;
    //DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    TPD_ERR("*** tp905::%s() ***\n", __func__);
    if (nFlag == true)
    {
        nRetVal = regulator_enable(g_ReguVdd);
        if (nRetVal)
        {
			TPD_ERR("regulator_enable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal);
        }
        mdelay(20);
    }
    else
    {
        nRetVal = regulator_disable(g_ReguVdd);
        if (nRetVal)
        {
			TPD_ERR("regulator_disable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal);
        }
        mdelay(20);
    }
#else
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    hwPowerOn(PMIC_APP_CAP_TOUCH_VDD, VOL_2800, "TP"); 
#endif 
#endif
}
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

void DrvPlatformLyrTouchDevicePowerOn(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
    udelay(100);
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
    mdelay(100);
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
    mdelay(25); 
#endif 
#endif

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    g_IsHwResetByDriver = 1; // Indicate HwReset is triggered by Device Driver instead of Firmware or Touch IC
#endif 
}

void DrvPlatformLyrTouchDevicePowerOff(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
    DrvIcFwLyrOptimizeCurrentConsumption();

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
	#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
		tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
	#else
		mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
		mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
		mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
	#endif
#endif    

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 0;
#endif //CONFIG_ENABLE_ESD_PROTECTION
}

void DrvPlatformLyrTouchDeviceResetHw(void)
{
    TPD_ERR("*** tp905::%s() ***\n", __func__); 
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
    udelay(100);
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
    mdelay(100);
    tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
    mdelay(25); 
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    g_IsHwResetByDriver = 1; // Indicate HwReset is triggered by Device Driver instead of Firmware or Touch IC
#endif //CONFIG_ENABLE_ESD_PROTECTION
}

void DrvPlatformLyrDisableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

   // DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
   // DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

    {
        if (_gInterruptFlag == 1) 
        {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            disable_irq(_gIrq);
#else
            mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif          
            _gInterruptFlag = 0;
        }
    }
#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

void DrvPlatformLyrEnableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

    TPD_ERR("*** %s() ***\n", __func__); 
    //DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        enable_irq(_gIrq);
#else
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif 
        _gInterruptFlag = 1;        
    }

#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

void DrvPlatformLyrFingerTouchPressed(s32 nX, s32 nY, s32 nPressure, s32 nId)
{
    TPD_ERR("tpd_down::*** tpd905:%s() ***\n", __func__); 
    //DBG(&g_I2cClient->dev, "point touch pressed\n"); 

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
    input_mt_slot(g_InputDevice, nId);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, true);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);
	

    DBG(&g_I2cClient->dev, "nId=%d, nX=%d, nY=%d\n", nId, nX, nY); // TODO : add for debug
#else // TYPE A PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 1);
    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        input_report_abs(g_InputDevice, ABS_MT_TRACKING_ID, nId); // ABS_MT_TRACKING_ID is used for MSG26xxM/MSG28xx only
    }
    input_report_abs(g_InputDevice, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_WIDTH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);
	
    input_mt_sync(g_InputDevice);
#endif 

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
	#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
		#ifdef CONFIG_MTK_BOOT
			if (tpd_dts_data.use_tpd_button)
			{
				if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
				{   
					tpd_button(nX, nY, 1);  
				}
			}
		#endif //CONFIG_MTK_BOOT
	#else
		#ifdef CONFIG_TP_HAVE_KEY    
			if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
			{   
				tpd_button(nX, nY, 1);  
			}
		#endif //CONFIG_TP_HAVE_KEY
	#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    TPD_EM_PRINT(nX, nY, nX, nY, nId, 1);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

void DrvPlatformLyrFingerTouchReleased(s32 nX, s32 nY, s32 nId)
{
    //DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    //DBG(&g_I2cClient->dev, "point touch released\n"); 

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL 
    input_mt_slot(g_InputDevice, nId);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, false);
    //DBG(&g_I2cClient->dev, "nId=%d\n", nId); // TODO : add for debug
#else 
    input_report_key(g_InputDevice, BTN_TOUCH, 0);
    input_mt_sync(g_InputDevice);
#endif 


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_MTK_BOOT
    if (tpd_dts_data.use_tpd_button)
    {
        if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
        {   
            tpd_button(nX, nY, 0); 
        }            
    }
#endif //CONFIG_MTK_BOOT  
#else
#ifdef CONFIG_TP_HAVE_KEY 
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {   
       tpd_button(nX, nY, 0); 
    }            
#endif //CONFIG_TP_HAVE_KEY    
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_EM_PRINT(nX, nY, nX, nY, 0, 0);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

void DrvPlatformLyrVariableInitialize(void)
{
    mutex_init(&g_Mutex);
    spin_lock_init(&_gIrqLock);
}

s32 DrvPlatformLyrInputDeviceInitialize(struct i2c_client *pClient)
{
    s32 nRetVal = 0;
    TPD_ERR("*** tp905::%s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    g_InputDevice = tpd->dev;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_dts_data.use_tpd_button)
    {
        u32 i;

        for (i = 0; i < tpd_dts_data.tpd_key_num; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, tpd_dts_data.tpd_key_local[i]);
        }
    }
#else
	#ifdef CONFIG_TP_HAVE_KEY
		{
			u32 i;
			
			for (i = 0; i < MAX_KEY_NUM; i ++)
			{
				input_set_capability(g_InputDevice, EV_KEY, g_TpVirtualKey[i]);
			}
		}
	#endif //CONFIG_TP_HAVE_KEY
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        input_set_abs_params(g_InputDevice, ABS_MT_TRACKING_ID, 0, (MUTUAL_MAX_TOUCH_NUM-1), 0, 0); // ABS_MT_TRACKING_ID is used for MSG26xxM/MSG28xx only
    }

/*
#ifndef CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_X, TOUCH_SCREEN_X_MIN, TOUCH_SCREEN_X_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_Y, TOUCH_SCREEN_Y_MIN, TOUCH_SCREEN_Y_MAX, 0, 0);
*/

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, SELF_MAX_TOUCH_NUM, 0); // for MSG21xxA/MSG22xx
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, MUTUAL_MAX_TOUCH_NUM, 0); // for MSG26xxM/MSG28xx
    }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif

    return nRetVal;    
}



s32 DrvPlatformLyrTouchDeviceRegisterFingerTouchInterruptHandler(void)
{
    s32 nRetVal = 0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    if (DrvIcFwLyrIsRegisterFingerTouchInterruptHandler())
    {    	
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        {
            struct device_node *pDeviceNode = NULL;
            u32 ints[2] = {0,0};
            tpd_gpio_as_int(MS_TS_MSG_IC_GPIO_INT);
		    //pDeviceNode = of_find_compatible_node(NULL, NULL, "mediatek,touch_panel-eint"); //"mediatek,cap_touch"
            pDeviceNode = of_find_matching_node(pDeviceNode, touch_of_match);
            if (pDeviceNode)
            {
                of_property_read_u32_array(pDeviceNode, "debounce", ints, ARRAY_SIZE(ints));
                gpio_set_debounce(ints[0], ints[1]);
                _gIrq = irq_of_parse_and_map(pDeviceNode, 0);
                if (_gIrq == 0)
                {
                    printk("*** Unable to irq_of_parse_and_map() ***\n");
                }

                /* request an irq and register the isr */
                nRetVal = request_threaded_irq(_gIrq, NULL, _DrvPlatformLyrFingerTouchInterruptHandler,
                      IRQF_TRIGGER_RISING | IRQF_ONESHOT,
                      "touch_panel-eint", NULL); 
                if (nRetVal != 0)
                {
                    DBG(&g_I2cClient->dev, "*** gpio_pin=%d, debounce=%d ***\n", ints[0], ints[1]);
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "*** request_irq() can not find touch eint device node! ***\n");
            }

		  //enable_irq(_gIrq);
        }
        _gInterruptFlag = 1;

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
        /* initialize the finger touch work queue */ 
        INIT_WORK(&_gFingerTouchWork, _DrvPlatformLyrFingerTouchDoWork);
#else
        _gThread = kthread_run(_DrvPlatformLyrFingerTouchHandler, 0, TPD_DEVICE);
        if (IS_ERR(_gThread))
        { 
            nRetVal = PTR_ERR(_gThread);
            DBG(&g_I2cClient->dev, "Failed to create kernel thread: %d\n", nRetVal); 
        }
#endif 
#endif
    }
    
    return nRetVal;    
}	

void DrvPlatformLyrTouchDeviceRegisterEarlySuspend(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_NOTIFIER_FB
    _gFbNotifier.notifier_call = MsDrvInterfaceTouchDeviceFbNotifierCallback;
    fb_register_client(&_gFbNotifier);
#else
    _gEarlySuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    _gEarlySuspend.suspend = MsDrvInterfaceTouchDeviceSuspend;
    _gEarlySuspend.resume = MsDrvInterfaceTouchDeviceResume;
    register_early_suspend(&_gEarlySuspend);
#endif //CONFIG_ENABLE_NOTIFIER_FB   
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 DrvPlatformLyrTouchDeviceRemove(struct i2c_client *pClient)
{
    TPD_ERR("***tp905::%s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    gpio_free(MS_TS_MSG_IC_GPIO_INT);
    gpio_free(MS_TS_MSG_IC_GPIO_RST);
    
    if (g_InputDevice)
    {
        free_irq(_gIrq, g_InputDevice);

        input_unregister_device(g_InputDevice);
        g_InputDevice = NULL;
    }

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
    _DrvPlatformLyrTouchPinCtrlUnInit();
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
    _DrvPlatformLyrVirtualKeysUnInit();
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY


#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
//    gpio_free(MS_TS_MSG_IC_GPIO_INT);
//    gpio_free(MS_TS_MSG_IC_GPIO_RST);
/*
#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
    _DrvPlatformLyrTouchPinCtrlUnInit();
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL
*/
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif    

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {    	
        if (g_TouchKSet)
        {
            kset_unregister(g_TouchKSet);
            g_TouchKSet = NULL;
        }
    
        if (g_TouchKObj)
        {
            kobject_put(g_TouchKObj);
            g_TouchKObj = NULL;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED


    DrvMainRemoveProcfsDirEntry();


#ifdef CONFIG_ENABLE_JNI_INTERFACE
    DeleteMsgToolMem();
#endif //CONFIG_ENABLE_JNI_INTERFACE

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    if (g_EsdCheckWorkqueue)
    {
        destroy_workqueue(g_EsdCheckWorkqueue);
        g_EsdCheckWorkqueue = NULL;
    }
#endif //CONFIG_ENABLE_ESD_PROTECTION

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    return 0;
}

void DrvPlatformLyrSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    //DBG(&g_I2cClient->dev, "*** tp905::%s() nIicDataRate = %d ***\n", __func__, nIicDataRate); 
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    pClient->timing = nIicDataRate/1000;
#endif
}

#ifdef CONFIG_ENABLE_ESD_PROTECTION
void DrvPlatformLyrEsdCheck(struct work_struct *pWork)
{
#ifdef CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE
    static u8 szEsdCheckValue[8];
    u8 szTxData[3] = {0};
    u8 szRxData[8] = {0};
    u32 i = 0;
    s32 retW = -1;
    s32 retR = -1;
#else
    u8 szData[MUTUAL_DEMO_MODE_PACKET_LENGTH] = {0xFF};
    u32 i = 0;
    s32 rc = 0;
#endif //CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

    DBG(&g_I2cClient->dev, "*** %s() g_IsEnableEsdCheck = %d ***\n", __func__, g_IsEnableEsdCheck); 

    if (g_IsEnableEsdCheck == 0)
    {
        return;
    }

    if (_gInterruptFlag == 0) // Skip ESD check while finger touch
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while finger touch.\n");
        goto EsdCheckEnd;
    }
    	
    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while update firmware is proceeding.\n");
        goto EsdCheckEnd;
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    if (g_IsInMpTest == 1) // Check whether mp test is proceeding
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while mp test is proceeding.\n");
        goto EsdCheckEnd;
    }
#endif //CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE 
    szTxData[0] = 0x55;
    szTxData[1] = 0xAA;
    szTxData[2] = 0x55;

    mutex_lock(&g_Mutex);

    retW = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    retR = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 8);

    mutex_unlock(&g_Mutex);

    DBG(&g_I2cClient->dev, "szRxData[] : 0x%x  0x%x  0x%x  0x%x  0x%x  0x%x  0x%x  0x%x\n", \
            szRxData[0], szRxData[1], szRxData[2], szRxData[3],szRxData[4], szRxData[5], szRxData[6], szRxData[7]);

    DBG(&g_I2cClient->dev, "retW = %d, retR = %d\n", retW, retR);

    if (retW > 0 && retR > 0)
    {
        while (i < 8)
        {
            if (szEsdCheckValue[i] != szRxData[i])
            {
                break;
            }
            i ++;
        }
        
        if (i == 8)
        {
            if (szRxData[0] == 0 && szRxData[1] == 0 && szRxData[2] == 0 && szRxData[3] == 0 && szRxData[4] == 0 && szRxData[5] == 0 && szRxData[6] == 0 && szRxData[7] == 0)
            {
                DBG(&g_I2cClient->dev, "Firmware not support ESD check command.\n");
            }
            else
            {
                DBG(&g_I2cClient->dev, "ESD check failed case1.\n");
                
                DrvPlatformLyrTouchDeviceResetHw();
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "ESD check success.\n");
        } 

        for (i = 0; i < 8; i ++)
        {
            szEsdCheckValue[i] = szRxData[i];
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "ESD check failed case2.\n");

        DrvPlatformLyrTouchDeviceResetHw();
    }
#else /* Method 2. Use read finger touch data for checking whether I2C connection is still available under ESD testing. */
    mutex_lock(&g_Mutex);
    
    while (i < 3)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
        {
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szData[0], MUTUAL_DEMO_MODE_PACKET_LENGTH); // for MSG26xxM/MSG28xx
            DBG(&g_I2cClient->dev, "szData[0] = 0x%x\n", szData[0]);
        }
        else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
        {
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szData[0], SELF_DEMO_MODE_PACKET_LENGTH); // for MSG21xxA/MSG22xx
            DBG(&g_I2cClient->dev, "szData[0] = 0x%x\n", szData[0]);
        }
        else
        {
			DBG(&g_I2cClient->dev, "Un-recognized chip type = 0x%x\n", g_ChipType);
            break;
        }
		if (rc > 0)
			{
				DBG(&g_I2cClient->dev, "ESD check success\n");
				break;
			}
		 
			i++;
		}
		if (i == 3)
		{
			DBG(&g_I2cClient->dev, "ESD check failed, rc = %d\n", rc);
		}

		mutex_unlock(&g_Mutex);

		if (i >= 3)
		{
			DrvPlatformLyrTouchDeviceResetHw();
		}
	#endif //CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

	EsdCheckEnd :

		if (g_IsEnableEsdCheck == 1)
		{
			queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
		}
}
#endif //CONFIG_ENABLE_ESD_PROTECTION
//------------------------------------------------------------------------------//

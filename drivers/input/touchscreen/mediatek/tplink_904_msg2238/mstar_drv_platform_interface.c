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
 * @file    mstar_drv_platform_interface.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_platform_interface.h"
#include "mstar_drv_main.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_printlog.h"

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/



extern u8 g_IsUpdateFirmware;
extern u8 g_IsDeviceWorking;

extern struct input_dev *g_InputDevice;
extern struct i2c_client *g_I2cClient;


#ifdef CONFIG_ENABLE_CHARGER_DETECTION
extern u8 g_ForceUpdate;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
extern int g_IsEnableEsdCheck;
extern struct delayed_work g_EsdCheckWork;
extern struct workqueue_struct *g_EsdCheckWorkqueue;
#endif //CONFIG_ENABLE_ESD_PROTECTION

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;


/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/


/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/


/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_NOTIFIER_FB
int MsDrvInterfaceTouchDeviceFbNotifierCallback(struct notifier_block *pSelf, unsigned long nEvent, void *pData)
{
    struct fb_event *pEventData = pData;
    int *pBlank;

    if (pEventData && pEventData->data && nEvent == FB_EVENT_BLANK)
    {
        pBlank = pEventData->data;

        if (*pBlank == FB_BLANK_UNBLANK)
        {
            DBG(&g_I2cClient->dev, "*** %s() TP Resume ***\n", __func__);

            if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
            {
                DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
                return 0;
            }

            
    
            {
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
                DrvPlatformLyrTouchDevicePowerOn(); 
            }   
    
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
            {
                u8 szChargerStatus[20] = {0};
     
                DrvCommonReadFile("/sys/class/power_supply/battery/status", szChargerStatus, 20);
            
                DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
            
                g_ForceUpdate = 1; // Set flag to force update charger status
                
                if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
                {
                    DrvFwCtrlChargerDetection(1); // charger plug-in
                }
                else // Not charging
                {
                    DrvFwCtrlChargerDetection(0); // charger plug-out
                }

                g_ForceUpdate = 0; // Clear flag after force update charger status
            }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

            if (IS_FIRMWARE_DATA_LOG_ENABLED)    
            {
                DrvIcFwLyrRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
            } //IS_FIRMWARE_DATA_LOG_ENABLED

            DrvPlatformLyrEnableFingerTouchReport(); 

#ifdef CONFIG_ENABLE_ESD_PROTECTION
            g_IsEnableEsdCheck = 1;
            queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION
        }
        else if (*pBlank == FB_BLANK_POWERDOWN)
        {
            DBG(&g_I2cClient->dev, "*** %s() TP Suspend ***\n", __func__);
            
#ifdef CONFIG_ENABLE_ESD_PROTECTION
            g_IsEnableEsdCheck = 0;
            cancel_delayed_work_sync(&g_EsdCheckWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

            if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
            {
                DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
                return 0;
            }




            DrvPlatformLyrFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
            input_sync(g_InputDevice);

            DrvPlatformLyrDisableFingerTouchReport();

            {
                DrvPlatformLyrTouchDevicePowerOff(); 
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvPlatformLyrTouchDeviceRegulatorPowerOn(false);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
            }    
        }
    }

    return 0;
}

#else

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
void MsDrvInterfaceTouchDeviceSuspend(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_ERR("tp905::*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 0;
    cancel_delayed_work_sync(&g_EsdCheckWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        TPD_ERR("Not allow to power on/off touch ic while update firmware.\n");
        return;
    }
    DrvPlatformLyrFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
    input_sync(g_InputDevice);
    DrvPlatformLyrDisableFingerTouchReport();
    {
        DrvPlatformLyrTouchDevicePowerOff(); 
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
	#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
        DrvPlatformLyrTouchDeviceRegulatorPowerOn(false);
        g_IsDeviceWorking = 0;
	#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    }    
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
void MsDrvInterfaceTouchDeviceResume(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_ERR("tp905::*** %s() ***\n", __func__);

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        TPD_ERR("Not allow to power on/off touch ic while update firmware.\n");
        return;
    }
   
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
	#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
			DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
	#endif             
#endif 
        DrvPlatformLyrTouchDevicePowerOn();
        g_IsDeviceWorking = 1;
    }   
    
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
    {
        u8 szChargerStatus[20] = {0};
 
        DrvCommonReadFile("/sys/class/power_supply/battery/status", szChargerStatus, 20);
        TPD_ERR("*** Battery Status : %s ***\n", szChargerStatus);        
        g_ForceUpdate = 1; // Set flag to force update charger status
        if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
        {
            DrvFwCtrlChargerDetection(1); // charger plug-in
        }
        else // Not charging
        {
            DrvFwCtrlChargerDetection(0); // charger plug-out
        }

        g_ForceUpdate = 0; // Clear flag after force update charger status
    }           
#endif 

    if (IS_FIRMWARE_DATA_LOG_ENABLED)    
    {
        DrvIcFwLyrRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
    } //IS_FIRMWARE_DATA_LOG_ENABLED

    DrvPlatformLyrEnableFingerTouchReport(); 

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif 
}

#endif //CONFIG_ENABLE_NOTIFIER_FB

/* probe function is used for matching and initializing input device */
s32 /*__devinit*/ MsDrvInterfaceTouchDeviceProbe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId)
{
    s32 nRetVal = 0;
    TPD_ERR("*** tp905::TouchDeviceProbe %s() ***\n", __func__);
    DrvPlatformLyrVariableInitialize();
    //DrvPlatformLyrTouchDeviceRequestGPIO(pClient);
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
#endif 

    DrvPlatformLyrTouchDevicePowerOn();
    nRetVal = DrvMainTouchDeviceInitialize();
    if (nRetVal == -ENODEV)
    {
        DrvPlatformLyrTouchDeviceRemove(pClient);
        return nRetVal;
    }

    DrvPlatformLyrInputDeviceInitialize(pClient); 
    
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
	#ifdef CONFIG_ENABLE_DMA_IIC
    	DmaAlloc(); 
	#endif 
#endif 

    DrvPlatformLyrTouchDeviceRegisterFingerTouchInterruptHandler();
    DrvPlatformLyrTouchDeviceRegisterEarlySuspend();
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
    DrvIcFwLyrCheckFirmwareUpdateBySwId();
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    INIT_DELAYED_WORK(&g_EsdCheckWork, DrvPlatformLyrEsdCheck);
    g_EsdCheckWorkqueue = create_workqueue("esd_check");
    queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION

    DBG(&g_I2cClient->dev, "*** MStar touch driver registered ***\n");
    
    return nRetVal;
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvPlatformLyrTouchDeviceRemove(pClient);
}

void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    DBG(&g_I2cClient->dev, "*** tp905::%s() ***\n", __func__);

    DrvPlatformLyrSetIicDataRate(pClient, nIicDataRate);
}    

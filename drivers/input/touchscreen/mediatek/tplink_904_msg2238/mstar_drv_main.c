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
 * @file    mstar_drv_main.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/
//#include <linux/platform_device.h>
#include "mstar_drv_main.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_printlog.h"

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#include "mstar_drv_jni_interface.h"
#endif //CONFIG_ENABLE_JNI_INTERFACE

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/


/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern MutualFirmwareInfo_t g_MutualFirmwareInfo;
extern SelfFirmwareInfo_t g_SelfFirmwareInfo;
extern u8 g_DemoModePacket[MUTUAL_DEMO_MODE_PACKET_LENGTH]; // for MSG21xxA/MSG22xx : DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH, for MSG26xxM/MSG28xx : DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH
extern u8 g_LogModePacket[MUTUAL_DEBUG_MODE_PACKET_LENGTH]; // for MSG21xxA/MSG22xx : DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH, for MSG26xxM/MSG28xx : DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH
extern u16 g_FirmwareMode;


extern struct i2c_client *g_I2cClient;

extern u8 g_ChipType;
extern u8 g_Msg22xxChipRevision;

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
extern TestScopeInfo_t g_TestScopeInfo;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static u16 _gDebugReg[MAX_DEBUG_REGISTER_NUM] = {0};
static u16 _gDebugRegValue[MAX_DEBUG_REGISTER_NUM] = {0};
static u32 _gDebugRegCount = 0;

static u8 _gDebugCmdArgu[MAX_DEBUG_COMMAND_ARGUMENT_NUM] = {0};
static u16 _gDebugCmdArguCount = 0;
static u32 _gDebugReadDataSize = 0;

static char _gDebugBuf[1024] = {0};

static u8 *_gPlatformFwVersion = NULL; // internal use firmware version for MStar

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ItoTestMode_e _gItoTestMode = 0;
#endif //CONFIG_ENABLE_ITO_MP_TEST


static u32 _gIsUpdateComplete = 0;

static u8 *_gFwVersion = NULL; // customer firmware version

static u32 _gFeatureSupportStatus = 0;

static struct proc_dir_entry *_gProcClassEntry = NULL;
static struct proc_dir_entry *_gProcMsTouchScreenMsg20xxEntry = NULL;
static struct proc_dir_entry *_gProcDeviceEntry = NULL;
static struct proc_dir_entry *_gProcChipTypeEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareDataEntry = NULL;
static struct proc_dir_entry *_gProcApkFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcCustomerFirmwareVersionEntry = NULL;
static struct proc_dir_entry *_gProcPlatformFirmwareVersionEntry = NULL;
static struct proc_dir_entry *_gProcDeviceDriverVersionEntry = NULL;
static struct proc_dir_entry *_gProcSdCardFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareDebugEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSetDebugValueEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSmBusDebugEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSetDQMemValueEntry = NULL;
#ifdef CONFIG_ENABLE_ITO_MP_TEST
static struct proc_dir_entry *_gProcMpTestEntry = NULL;
static struct proc_dir_entry *_gProcMpTestLogEntry = NULL;
static struct proc_dir_entry *_gProcMpTestFailChannelEntry = NULL;
static struct proc_dir_entry *_gProcMpTestScopeEntry = NULL;
#endif //CONFIG_ENABLE_ITO_MP_TEST
static struct proc_dir_entry *_gProcFirmwareModeEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSensorEntry = NULL;
static struct proc_dir_entry *_gProcFirmwarePacketHeaderEntry = NULL;
static struct proc_dir_entry *_gProcQueryFeatureSupportStatusEntry = NULL;
static struct proc_dir_entry *_gProcChangeFeatureSupportStatusEntry = NULL;
#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static struct proc_dir_entry *_gProcReportRateEntry = NULL;
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *_gProcJniMethodEntry = NULL;
#endif //CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *_gProcSeLinuxLimitFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcForceFirmwareUpdateEntry = NULL;

static const struct file_operations _gProcChipType = { 
    .read = DrvMainProcfsChipTypeRead,
    .write = DrvMainProcfsChipTypeWrite,
};

static const struct file_operations _gProcFirmwareData = { 
    .read = DrvMainProcfsFirmwareDataRead,
    .write = DrvMainProcfsFirmwareDataWrite,
};

static const struct file_operations _gProcApkFirmwareUpdate = { 
    .read = DrvMainProcfsFirmwareUpdateRead,
    .write = DrvMainProcfsFirmwareUpdateWrite,
};

static const struct file_operations _gProcCustomerFirmwareVersion = { 
    .read = DrvMainProcfsCustomerFirmwareVersionRead,
    .write = DrvMainProcfsCustomerFirmwareVersionWrite,
};

static const struct file_operations _gProcPlatformFirmwareVersion = { 
    .read = DrvMainProcfsPlatformFirmwareVersionRead,
    .write = DrvMainProcfsPlatformFirmwareVersionWrite,
};

static const struct file_operations _gProcDeviceDriverVersion = { 
    .read = DrvMainProcfsDeviceDriverVersionRead,
    .write = DrvMainProcfsDeviceDriverVersionWrite,
};

static const struct file_operations _gProcSdCardFirmwareUpdate = { 
    .read = DrvMainProcfsSdCardFirmwareUpdateRead,
    .write = DrvMainProcfsSdCardFirmwareUpdateWrite,
};

static const struct file_operations _gProcFirmwareDebug = { 
    .read = DrvMainProcfsFirmwareDebugRead,
    .write = DrvMainProcfsFirmwareDebugWrite,
};

static const struct file_operations _gProcFirmwareSetDebugValue = { 
    .read = DrvMainProcfsFirmwareSetDebugValueRead,
    .write = DrvMainProcfsFirmwareSetDebugValueWrite,
};

static const struct file_operations _gProcFirmwareSmBusDebug = { 
    .read = DrvMainProcfsFirmwareSmBusDebugRead,
    .write = DrvMainProcfsFirmwareSmBusDebugWrite,
};

static const struct file_operations _gProcFirmwareSetDQMemValue = {
    .read = DrvMainProcfsFirmwareSetDQMemValueRead,
    .write = DrvMainProcfsFirmwareSetDQMemValueWrite,
};

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static const struct file_operations _gProcMpTest = { 
    .read = DrvMainProcfsMpTestRead,
    .write = DrvMainProcfsMpTestWrite,
};

static const struct file_operations _gProcMpTestLog = { 
    .read = DrvMainProcfsMpTestLogRead,
    .write = DrvMainProcfsMpTestLogWrite,
};

static const struct file_operations _gProcMpTestFailChannel = { 
    .read = DrvMainProcfsMpTestFailChannelRead,
    .write = DrvMainProcfsMpTestFailChannelWrite,
};

static const struct file_operations _gProcMpTestScope = { 
    .read = DrvMainProcfsMpTestScopeRead,
    .write = DrvMainProcfsMpTestScopeWrite,
};
#endif //CONFIG_ENABLE_ITO_MP_TEST


static const struct file_operations _gProcFirmwareMode = { 
    .read = DrvMainProcfsFirmwareModeRead,
    .write = DrvMainProcfsFirmwareModeWrite,
};

static const struct file_operations _gProcFirmwareSensor = { 
    .read = DrvMainProcfsFirmwareSensorRead,
    .write = DrvMainProcfsFirmwareSensorWrite,
};

static const struct file_operations _gProcFirmwarePacketHeader = { 
    .read = DrvMainProcfsFirmwarePacketHeaderRead,
    .write = DrvMainProcfsFirmwarePacketHeaderWrite,
};


static const struct file_operations _gProcQueryFeatureSupportStatus = { 
    .read = DrvMainProcfsQueryFeatureSupportStatusRead,
    .write = DrvMainProcfsQueryFeatureSupportStatusWrite,
};

static const struct file_operations _gProcChangeFeatureSupportStatus = { 
    .read = DrvMainProcfsChangeFeatureSupportStatusRead,
    .write = DrvMainProcfsChangeFeatureSupportStatusWrite,
};


#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static const struct file_operations _gProcReportRate = { 
    .read = DrvMainProcfsReportRateRead,
    .write = DrvMainProcfsReportRateWrite,
};
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static const struct file_operations _gProcJniMethod = {
    .read = MsgToolRead,
    .write = MsgToolWrite,
    .unlocked_ioctl = MsgToolIoctl,    
    .compat_ioctl = MsgToolIoctl,    
};
#endif //CONFIG_ENABLE_JNI_INTERFACE

static const struct file_operations _gProcSeLinuxLimitFirmwareUpdate = { 
    .read = DrvMainProcfsSeLinuxLimitFirmwareUpdateRead,
};

static const struct file_operations _gProcForceFirmwareUpdate = { 
    .read = DrvMainProcfsForceFirmwareUpdateRead,
};



/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

//u32 SLAVE_I2C_ID_DBBUS = (0xC4>>1); //0x62 // for MSG21XX/MSG21XXA/MSG26XXM/MSG28XX
u32 SLAVE_I2C_ID_DBBUS = (0xB2>>1); //0x59 // for MSG22XX
u32 SLAVE_I2C_ID_DWI2C = (0x4C>>1); //0x26 


u16 FIRMWARE_MODE_UNKNOWN_MODE = 0xFFFF;
u16 FIRMWARE_MODE_DEMO_MODE = 0xFFFF;
u16 FIRMWARE_MODE_DEBUG_MODE = 0xFFFF;
u16 FIRMWARE_MODE_RAW_DATA_MODE = 0xFFFF;

u16 DEMO_MODE_PACKET_LENGTH = 0; // If project use MSG26xxM/MSG28xx, set MUTUAL_DEMO_MODE_PACKET_LENGTH as default. If project use MSG21xxA/MSG22xx, set SELF_DEMO_MODE_PACKET_LENGTH as default. 
u16 DEBUG_MODE_PACKET_LENGTH = 0; // If project use MSG26xxM/MSG28xx, set MUTUAL_DEBUG_MODE_PACKET_LENGTH as default. If project use MSG21xxA/MSG22xx, set SELF_DEBUG_MODE_PACKET_LENGTH as default. 
u16 MAX_TOUCH_NUM = 0; // If project use MSG26xxM/MSG28xx, set MUTUAL_MAX_TOUCH_NUM as default. If project use MSG21xxA/MSG22xx, set SELF_MAX_TOUCH_NUM as default. 


struct kset *g_TouchKSet = NULL;
struct kobject *g_TouchKObj = NULL;
u8 g_IsSwitchModeByAPK = 0;

/* [linchao] Is the device still power on */
u8 g_IsDeviceWorking = 1;
u8 IS_GESTURE_WAKEUP_ENABLED = 0;
u8 IS_GESTURE_DEBUG_MODE_ENABLED = 0;
u8 IS_GESTURE_INFORMATION_MODE_ENABLED = 0;
u8 IS_GESTURE_WAKEUP_MODE_SUPPORT_64_TYPES_ENABLED = 0;

u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL = CONFIG_TOUCH_DRIVER_DEBUG_LOG_LEVEL;
u8 IS_FIRMWARE_DATA_LOG_ENABLED = CONFIG_ENABLE_FIRMWARE_DATA_LOG;
u8 IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = 0;



#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
u16 g_FwPacketDataAddress = 0;
u16 g_FwPacketFlagAddress = 0;
u8 g_FwSupportSegment = 0;
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
u32 g_IsEnableReportRate = 0;
u32 g_InterruptCount = 0;
u32 g_ValidTouchCount = 0;
u32 g_InterruptReportRate = 0;
u32 g_ValidTouchReportRate = 0;

struct timeval g_StartTime;
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

u8 g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
u32 g_FwDataCount = 0;

u8 g_IsHotknotEnabled = 0;
u8 g_IsBypassHotknot = 0;



/*=============================================================*/
// EXTERN FUNCTION DECLARATION
/*=============================================================*/

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_CHARGER_DETECTION
extern kal_bool upmu_is_chr_det(void);
#endif //CONFIG_ENABLE_CHARGER_DETECTION
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static s32 _DrvMainCreateProcfsDirEntry(void);

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

//------------------------------------------------------------------------------//
/* [linchao start] Read hw_info */
typedef enum __VENDOR_SUPPORT__ {
    DJN = 0,
    BOOYI,
    VENDOR_UNDEFINED,
    VENDOR_SUPPORT_MAX,
} VENDOR_SUPPORT;

static const char *panel_ic[VENDOR_SUPPORT_MAX] = {
    "djn",
    "booyi",
    "undefined"
};

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern u8 msg22xx_booyi_update_bin[];
extern u8 msg22xx_djn_update_bin[];
#endif

//ssize_t DrvMainProcfsHwInfoRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
ssize_t DrvMainProcfsHwInfoRead(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 nLength = 0;
    int vendor = 0;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg22xxSwId_e eSwId = MSG22XX_SW_ID_UNDEFINED;
    unsigned char panel_tpye[] = "msg2238";

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    if (0 == g_IsDeviceWorking)
    {
        nLength = sprintf(buf, "Don't support to read when power off\n");
    }
    else
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
        if (eSwId == MSG22XX_SW_ID_BOOYI)
        {
            nUpdateBinMajor = msg22xx_booyi_update_bin[0xBFF5]<<8 | msg22xx_booyi_update_bin[0xBFF4];
            nUpdateBinMinor = msg22xx_booyi_update_bin[0xBFF7]<<8 | msg22xx_booyi_update_bin[0xBFF6];
        }
        else if (eSwId == MSG22XX_SW_ID_DJN)
        {
            nUpdateBinMajor = msg22xx_djn_update_bin[0xBFF5]<<8 | msg22xx_djn_update_bin[0xBFF4];
            nUpdateBinMinor = msg22xx_djn_update_bin[0xBFF7]<<8 | msg22xx_djn_update_bin[0xBFF6];
        }
        else //eSwId == MSG22XX_SW_ID_UNDEFINED
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;
        }
#endif
        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);
        if (eSwId == MSG22XX_SW_ID_BOOYI)
        {
             vendor = BOOYI;
        }
        else if (eSwId == MSG22XX_SW_ID_DJN)
        {
             vendor = DJN;
        }
        else
        {
            vendor = VENDOR_UNDEFINED;
        }

        nLength = sprintf(buf,
            "type:\t%s\n"
            "vendor:\t%s\n"
            "firmware_version:\t%d.%d\n"
            "update_version:\t%d.%d\n"
            "driver_version:\t%s\n",
            panel_tpye,
            panel_ic[vendor],
            nMajor, nMinor,
            nUpdateBinMajor, nUpdateBinMinor,
            DEVICE_DRIVER_RELEASE_VERSION);
    }
    return nLength;
}
/* [linchao end] */

static DEVICE_ATTR(tp_info, S_IRUGO, DrvMainProcfsHwInfoRead, NULL);

ssize_t DrvMainProcfsChipTypeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%d", g_ChipType);

    if (g_ChipType == CHIP_TYPE_MSG22XX)  // (0x7A)
    {
        DBG(&g_I2cClient->dev, "g_Msg22xxChipRevision = 0x%x\n", g_Msg22xxChipRevision); // TODO : add for debug
    }

    *pPos += nLength;

    return nLength;
}			  
			  
ssize_t DrvMainProcfsChipTypeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

//    g_ChipType = DrvIcFwLyrGetChipType();

    return nCount;
}

ssize_t DrvMainProcfsFirmwareDataRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    DBG(&g_I2cClient->dev, "*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    *pPos += g_FwDataCount;

    return g_FwDataCount;
}			  
			  
ssize_t DrvMainProcfsFirmwareDataWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    u32 nNum = nCount / 1024;
    u32 nRemainder = nCount % 1024;
    u32 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nNum > 0) // nCount >= 1024
    {
        for (i = 0; i < nNum; i ++)
        {
            memcpy(g_FwData[g_FwDataCount], pBuffer+(i*1024), 1024);

            g_FwDataCount ++;
        }

        if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
        {
            DBG(&g_I2cClient->dev, "nRemainder = %d\n", nRemainder);

            memcpy(g_FwData[g_FwDataCount], pBuffer+(i*1024), nRemainder);

            g_FwDataCount ++;
        }
    }
    else // nCount < 1024
    {
        if (nCount > 0)
        {
            memcpy(g_FwData[g_FwDataCount], pBuffer, nCount);

            g_FwDataCount ++;
        }
    }

    DBG(&g_I2cClient->dev, "*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "*** buf[0] = %c ***\n", pBuffer[0]);
    }

    return nCount;
}

ssize_t DrvMainProcfsFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    
    nLength = sprintf(pBuffer, "%d", _gIsUpdateComplete);

    DBG(&g_I2cClient->dev, "*** _gIsUpdateComplete = %d ***\n", _gIsUpdateComplete);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DrvPlatformLyrDisableFingerTouchReport();

    DBG(&g_I2cClient->dev, "*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);

    if (0 != DrvIcFwLyrUpdateFirmware(g_FwData, EMEM_ALL))
    {
        _gIsUpdateComplete = 0;
        DBG(&g_I2cClient->dev, "Update FAILED\n");
    }
    else
    {
        _gIsUpdateComplete = 1;
        DBG(&g_I2cClient->dev, "Update SUCCESS\n");
    }

    DrvPlatformLyrEnableFingerTouchReport();

    return nCount;
}

ssize_t DrvMainProcfsCustomerFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%s\n", _gFwVersion);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsCustomerFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u16 nMajor = 0, nMinor = 0;

    DrvIcFwLyrGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    return nCount;
}

ssize_t DrvMainProcfsPlatformFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%s\n", _gPlatformFwVersion);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsPlatformFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DrvIcFwLyrGetPlatformFirmwareVersion(&_gPlatformFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    return nCount;
}

ssize_t DrvMainProcfsDeviceDriverVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%s", DEVICE_DRIVER_RELEASE_VERSION);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsDeviceDriverVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

ssize_t DrvMainProcfsSdCardFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u16 nMajor = 0, nMinor = 0;
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvIcFwLyrGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    nLength = sprintf(pBuffer, "%s\n", _gFwVersion);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsSdCardFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    char *pValid = NULL;
    char *pTmpFilePath = NULL;
    char szFilePath[100] = {0};
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "pBuffer = %s\n", pBuffer);

    if (pBuffer != NULL)
    {
        pValid = strstr(pBuffer, ".bin");
        
        if (pValid)
        {
            pTmpFilePath = strsep((char **)&pBuffer, ".");
            
            DBG(&g_I2cClient->dev, "pTmpFilePath = %s\n", pTmpFilePath);

            strcat(szFilePath, pTmpFilePath);
            strcat(szFilePath, ".bin");

            DBG(&g_I2cClient->dev, "szFilePath = %s\n", szFilePath);
            
            if (0 != DrvIcFwLyrUpdateFirmwareBySdCard(szFilePath))
            {
                DBG(&g_I2cClient->dev, "Update FAILED\n");
            }
            else
            {
                DBG(&g_I2cClient->dev, "Update SUCCESS\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The file type of the update firmware bin file is not a .bin file.\n");
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "The file path of the update firmware bin file is NULL.\n");
    }

    return nCount;
}

ssize_t DrvMainProcfsSeLinuxLimitFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DBG(&g_I2cClient->dev, "FIRMWARE_FILE_PATH_ON_SD_CARD = %s\n", FIRMWARE_FILE_PATH_ON_SD_CARD);
    
    if (0 != DrvIcFwLyrUpdateFirmwareBySdCard(FIRMWARE_FILE_PATH_ON_SD_CARD))
    {
        _gIsUpdateComplete = 0;
        DBG(&g_I2cClient->dev, "Update FAILED\n");
    }
    else
    {
        _gIsUpdateComplete = 1;
        DBG(&g_I2cClient->dev, "Update SUCCESS\n");
    }

    nLength = sprintf(pBuffer, "%d", _gIsUpdateComplete);

    DBG(&g_I2cClient->dev, "*** _gIsUpdateComplete = %d ***\n", _gIsUpdateComplete);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsForceFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DBG(&g_I2cClient->dev, "*** IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d ***\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);

    IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = 1; // Enable force firmware update
    _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;

    nLength = sprintf(pBuffer, "%d", _gFeatureSupportStatus);

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwareDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
                
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = sprintf(pBuffer, "%s\n", szOut);

    *pPos += nLength;
    
    return nLength;
}

ssize_t DrvMainProcfsFirmwareDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i;
    char *pCh = NULL;
    char *pStr = NULL;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", pBuffer[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", pBuffer[1]);
        DBG(&g_I2cClient->dev, "*** pBuffer[2] = %c ***\n", pBuffer[2]);
        DBG(&g_I2cClient->dev, "*** pBuffer[3] = %c ***\n", pBuffer[3]);
        DBG(&g_I2cClient->dev, "*** pBuffer[4] = %c ***\n", pBuffer[4]);
        DBG(&g_I2cClient->dev, "*** pBuffer[5] = %c ***\n", pBuffer[5]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);
       
        memset(_gDebugBuf, 0, 1024);

        if (copy_from_user(_gDebugBuf, pBuffer, nCount))
        {
            DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

            return -EFAULT;
        }

        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;
        
        i = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < MAX_DEBUG_REGISTER_NUM))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);
            
            _gDebugReg[i] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));

            DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", i, _gDebugReg[i]);
            i ++;
        }
        _gDebugRegCount = i;
        
        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);
    }

    return nCount;
}

ssize_t DrvMainProcfsFirmwareSetDebugValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = sprintf(pBuffer, "%s\n", szOut);

    *pPos += nLength;
    
    return nLength;
}

ssize_t DrvMainProcfsFirmwareSetDebugValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", pBuffer[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", pBuffer[1]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);

        memset(_gDebugBuf, 0, 1024);

        if (copy_from_user(_gDebugBuf, pBuffer, nCount))
        {
            DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

            return -EFAULT;
        }
       
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;
        k = 0;
        
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if ((i%2) == 0)
            {
                _gDebugReg[j] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", j, _gDebugReg[j]);
                j ++;
            }
            else // (i%2) == 1
            {	
                _gDebugRegValue[k] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugRegValue[%d] = 0x%04X\n", k, _gDebugRegValue[k]);
                k ++;
            }

            i ++;
        }
        _gDebugRegCount = j;
        
        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
    
        for (i = 0; i < _gDebugRegCount; i ++)
        {
            RegSet16BitValue(_gDebugReg[i], _gDebugRegValue[i]);
            DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X, _gDebugRegValue[%d] = 0x%04X\n", i, _gDebugReg[i], i , _gDebugRegValue[i]); // add for debug
        }

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    
    return nCount;
}

ssize_t DrvMainProcfsFirmwareSmBusDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 szSmBusRxData[MAX_I2C_TRANSACTION_LENGTH_LIMIT] = {0};
    u8 szOut[MAX_I2C_TRANSACTION_LENGTH_LIMIT*2] = {0};
    u8 szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if (_gDebugCmdArguCount > 0) // Send write command
    {
        DBG(&g_I2cClient->dev, "Execute I2C SMBUS write command\n"); 

        IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDebugCmdArgu[0], _gDebugCmdArguCount);
    }

    if (_gDebugReadDataSize > 0) // Send read command
    {
        DBG(&g_I2cClient->dev, "Execute I2C SMBUS read command\n"); 

        IicReadData(SLAVE_I2C_ID_DWI2C, &szSmBusRxData[0], _gDebugReadDataSize);
    }

    for (i = 0; i < _gDebugReadDataSize; i ++)
    {
        DBG(&g_I2cClient->dev, "szSmBusRxData[%d] = 0x%x\n", i, szSmBusRxData[i]); 

        sprintf(szValue, "0x%02X", szSmBusRxData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = sprintf(pBuffer, "%s\n", szOut);

    *pPos += nLength;
    
    return nLength;
}

ssize_t DrvMainProcfsFirmwareSmBusDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i, j;
    char szCmdType[5] = {0};
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", pBuffer[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", pBuffer[1]);
        DBG(&g_I2cClient->dev, "*** pBuffer[2] = %c ***\n", pBuffer[2]);
        DBG(&g_I2cClient->dev, "*** pBuffer[3] = %c ***\n", pBuffer[3]);
        DBG(&g_I2cClient->dev, "*** pBuffer[4] = %c ***\n", pBuffer[4]);
        DBG(&g_I2cClient->dev, "*** pBuffer[5] = %c ***\n", pBuffer[5]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);
       
        memset(_gDebugBuf, 0, 1024);

        if (copy_from_user(_gDebugBuf, pBuffer, nCount))
        {
            DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

            return -EFAULT;
        }

        // Reset to 0 before parsing the adb command
        _gDebugCmdArguCount = 0;
        _gDebugReadDataSize = 0;
        
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (j < MAX_DEBUG_COMMAND_ARGUMENT_NUM))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);
            
            if (strcmp(pCh, "w") == 0 || strcmp(pCh, "r") == 0)
            {
                memcpy(szCmdType, pCh, strlen(pCh));
            }
            else if (strcmp(szCmdType, "w") == 0)
            {
                _gDebugCmdArgu[j] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugCmdArgu[%d] = 0x%02X\n", j, _gDebugCmdArgu[j]);
                
                j ++;
       
                _gDebugCmdArguCount = j;
                DBG(&g_I2cClient->dev, "_gDebugCmdArguCount = %d\n", _gDebugCmdArguCount);
            }
            else if (strcmp(szCmdType, "r") == 0)
            {
                sscanf(pCh, "%d", &_gDebugReadDataSize);   
                DBG(&g_I2cClient->dev, "_gDebugReadDataSize = %d\n", _gDebugReadDataSize);
            }
            else
            {
                DBG(&g_I2cClient->dev, "Un-supported adb command format!\n");
            }

            i ++;
        }
    }

    return nCount;
}

ssize_t DrvMainProcfsFirmwareSetDQMemValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u32 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = DrvIcFwLyrReadDQMemValue(_gDebugReg[i]);
    }

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;

        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%08X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = sprintf(pBuffer, "%s\n", szOut);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwareSetDQMemValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;
    u16 nRealDQMemAddr = 0;
    u32 nRealDQMemValue = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", pBuffer[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", pBuffer[1]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);

        memset(_gDebugBuf, 0, 1024);

        if (copy_from_user(_gDebugBuf, pBuffer, nCount))
        {
            DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

            return -EFAULT;
        }

        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;
        k = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if ((i%2) == 0)
            {
                _gDebugReg[j] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", j, _gDebugReg[j]);
                j ++;
            }
            else // (i%2) == 1
            {
                _gDebugRegValue[k] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugRegValue[%d] = 0x%04X\n", k, _gDebugRegValue[k]);
                k ++;
            }

            i ++;
        }
        _gDebugRegCount = j;

        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);

        if ((_gDebugReg[0] % 4) == 0)
        {
            nRealDQMemAddr = _gDebugReg[0];
            nRealDQMemValue = DrvIcFwLyrReadDQMemValue(nRealDQMemAddr);
            _gDebugReg[0] = nRealDQMemAddr;
            DBG(&g_I2cClient->dev, "nRealDQMemValue Raw = %X\n", nRealDQMemValue);
            nRealDQMemValue &= 0xFFFF0000;
            nRealDQMemValue |= _gDebugRegValue[0];
            DBG(&g_I2cClient->dev, "nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            DrvIcFwLyrWriteDQMemValue(nRealDQMemAddr, nRealDQMemValue);
        }
        else if ((_gDebugReg[0] % 4) == 2)
        {
            nRealDQMemAddr = _gDebugReg[0] - 2;
            nRealDQMemValue = DrvIcFwLyrReadDQMemValue(nRealDQMemAddr);
            _gDebugReg[0] = nRealDQMemAddr;
            DBG(&g_I2cClient->dev, "nRealDQMemValue Raw = %X\n", nRealDQMemValue);

            nRealDQMemValue &= 0x0000FFFF;
            nRealDQMemValue |= (_gDebugRegValue[0] << 16);
            DBG(&g_I2cClient->dev, "nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            DrvIcFwLyrWriteDQMemValue(nRealDQMemAddr, nRealDQMemValue);
        }
    }

    return nCount;
}

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_ITO_MP_TEST

ssize_t DrvMainProcfsMpTestRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    char page[512];
	int ret = -1;
    TPD_ERR( "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    TPD_ERR("*** ctp mp test status = %d ***\n", DrvIcFwLyrGetMpTestResult());
    nLength = sprintf(page, "%d\n", DrvIcFwLyrGetMpTestResult());
	//*pPos += nLength;
	ret = simple_read_from_buffer(pBuffer, nCount, pPos, page, strlen(page));
    return ret;
}

ssize_t DrvMainProcfsMpTestWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nMode = 0;

    TPD_ERR("*** %s() ***\n", __func__);
    
    if (pBuffer != NULL)
    {
        sscanf(pBuffer, "%x", &nMode);   

        DBG(&g_I2cClient->dev, "Mp Test Mode = 0x%x\n", nMode);

        if (nMode == ITO_TEST_MODE_OPEN_TEST) //open test
        {
            _gItoTestMode = ITO_TEST_MODE_OPEN_TEST;
            DrvIcFwLyrScheduleMpTestWork(ITO_TEST_MODE_OPEN_TEST);
        }
        else if (nMode == ITO_TEST_MODE_SHORT_TEST) //short test
        {
            _gItoTestMode = ITO_TEST_MODE_SHORT_TEST;
            DrvIcFwLyrScheduleMpTestWork(ITO_TEST_MODE_SHORT_TEST);
        }
        else if (nMode == ITO_TEST_MODE_WATERPROOF_TEST) //waterproof test
        {
            _gItoTestMode = ITO_TEST_MODE_WATERPROOF_TEST;
            DrvIcFwLyrScheduleMpTestWork(ITO_TEST_MODE_WATERPROOF_TEST);
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined MP Test Mode ***\n");
        }
    }
    
    return nCount;
}

ssize_t DrvMainProcfsMpTestLogRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvIcFwLyrGetMpTestDataLog(_gItoTestMode, pBuffer, &nLength);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsMpTestLogWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

ssize_t DrvMainProcfsMpTestFailChannelRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvIcFwLyrGetMpTestFailChannel(_gItoTestMode, pBuffer, &nLength);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsMpTestFailChannelWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

ssize_t DrvMainProcfsMpTestScopeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        DrvIcFwLyrGetMpTestScope(&g_TestScopeInfo);

        nLength = sprintf(pBuffer, "%d,%d,%d", g_TestScopeInfo.nMx, g_TestScopeInfo.nMy, g_TestScopeInfo.nKeyNum);
    }  
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    
    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsMpTestScopeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

#endif //CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/

ssize_t DrvMainProcfsFirmwareModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        g_FirmwareMode = DrvIcFwLyrGetFirmwareMode();

        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x\n", __func__, g_FirmwareMode);

        nLength = sprintf(pBuffer, "%x", g_FirmwareMode);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        DrvIcFwLyrMutualGetFirmwareInfo(&g_MutualFirmwareInfo);
        g_FirmwareMode = g_MutualFirmwareInfo.nFirmwareMode;

        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x\n", __func__, g_MutualFirmwareInfo.nFirmwareMode);

        nLength = sprintf(pBuffer, "%x", g_MutualFirmwareInfo.nFirmwareMode);
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        DrvIcFwLyrSelfGetFirmwareInfo(&g_SelfFirmwareInfo);
        g_FirmwareMode = g_SelfFirmwareInfo.nFirmwareMode;

        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x, can change firmware mode = %d\n", __func__, g_SelfFirmwareInfo.nFirmwareMode, g_SelfFirmwareInfo.nIsCanChangeFirmwareMode);

        nLength = sprintf(pBuffer, "%x,%d", g_SelfFirmwareInfo.nFirmwareMode, g_SelfFirmwareInfo.nIsCanChangeFirmwareMode);
    }

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwareModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nMode;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (pBuffer != NULL)
    {
        sscanf(pBuffer, "%x", &nMode);   
        DBG(&g_I2cClient->dev, "firmware mode = 0x%x\n", nMode);

        g_IsSwitchModeByAPK = 0;

        if (nMode == FIRMWARE_MODE_DEMO_MODE) //demo mode
        {
            g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_DEMO_MODE);
        }
        else if (nMode == FIRMWARE_MODE_DEBUG_MODE) //debug mode
        {
            g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            g_IsSwitchModeByAPK = 1;
        }
        else if (nMode == FIRMWARE_MODE_RAW_DATA_MODE) //raw data mode
        {
            if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
            {
                g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
                g_IsSwitchModeByAPK = 1;
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Firmware Mode ***\n");
        }
    }

    DBG(&g_I2cClient->dev, "*** g_FirmwareMode = 0x%x ***\n", g_FirmwareMode);
    
    return nCount;
}

ssize_t DrvMainProcfsFirmwareSensorRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        if (g_MutualFirmwareInfo.nLogModePacketHeader == 0xA5 || g_MutualFirmwareInfo.nLogModePacketHeader == 0xAB)
        {
            nLength = sprintf(pBuffer, "%d,%d", g_MutualFirmwareInfo.nMx, g_MutualFirmwareInfo.nMy);
        }
        else if (g_MutualFirmwareInfo.nLogModePacketHeader == 0xA7)
        {
            nLength = sprintf(pBuffer, "%d,%d,%d,%d", g_MutualFirmwareInfo.nMx, g_MutualFirmwareInfo.nMy, g_MutualFirmwareInfo.nSs, g_MutualFirmwareInfo.nSd);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Undefined debug mode packet format : 0x%x\n", g_MutualFirmwareInfo.nLogModePacketHeader);
            nLength = 0;
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nLength = sprintf(pBuffer, "%d", g_SelfFirmwareInfo.nLogModePacketLength);
    }
    
    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwareSensorWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

ssize_t DrvMainProcfsFirmwarePacketHeaderRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        nLength = sprintf(pBuffer, "%d", g_MutualFirmwareInfo.nLogModePacketHeader);
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nLength = sprintf(pBuffer, "%d", g_SelfFirmwareInfo.nLogModePacketHeader);
    }    

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsFirmwarePacketHeaderWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

ssize_t DrvMainKObjectPacketShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 i = 0;
    u32 nLength = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (strcmp(pAttr->attr.name, "packet") == 0)
    {
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            if (g_DemoModePacket != NULL)
            {
                DBG(&g_I2cClient->dev, "g_FirmwareMode=%x, g_DemoModePacket[0]=%x, g_DemoModePacket[1]=%x\n", g_FirmwareMode, g_DemoModePacket[0], g_DemoModePacket[1]);
                DBG(&g_I2cClient->dev, "g_DemoModePacket[2]=%x, g_DemoModePacket[3]=%x\n", g_DemoModePacket[2], g_DemoModePacket[3]);
                DBG(&g_I2cClient->dev, "g_DemoModePacket[4]=%x, g_DemoModePacket[5]=%x\n", g_DemoModePacket[4], g_DemoModePacket[5]);
                
                for (i = 0; i < DEMO_MODE_PACKET_LENGTH; i ++)
                {
                    pBuf[i] = g_DemoModePacket[i];
                }

                nLength = DEMO_MODE_PACKET_LENGTH;
                DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
            }
            else
            {
                DBG(&g_I2cClient->dev, "g_DemoModePacket is NULL\n");
            }
        }
        else //g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE
        {
            if (g_LogModePacket != NULL)
            {
                DBG(&g_I2cClient->dev, "g_FirmwareMode=%x, g_LogModePacket[0]=%x, g_LogModePacket[1]=%x\n", g_FirmwareMode, g_LogModePacket[0], g_LogModePacket[1]);
                DBG(&g_I2cClient->dev, "g_LogModePacket[2]=%x, g_LogModePacket[3]=%x\n", g_LogModePacket[2], g_LogModePacket[3]);
                DBG(&g_I2cClient->dev, "g_LogModePacket[4]=%x, g_LogModePacket[5]=%x\n", g_LogModePacket[4], g_LogModePacket[5]);

                if ((g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) && (g_LogModePacket[0] == 0xA5 || g_LogModePacket[0] == 0xAB || g_LogModePacket[0] == 0xA7))
                {
                    for (i = 0; i < g_MutualFirmwareInfo.nLogModePacketLength; i ++)
                    {
                        pBuf[i] = g_LogModePacket[i];
                    }

                    nLength = g_MutualFirmwareInfo.nLogModePacketLength;
                    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
                }
                else if ((g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX) && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE) && (g_LogModePacket[0] == 0x62))
                {
                    for (i = 0; i < g_SelfFirmwareInfo.nLogModePacketLength; i ++)
                    {
                        pBuf[i] = g_LogModePacket[i];
                    }

                    nLength = g_SelfFirmwareInfo.nLogModePacketLength;
                    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
                }
                else
                {
                    DBG(&g_I2cClient->dev, "CURRENT MODE IS NOT DEBUG MODE/WRONG DEBUG MODE HEADER\n");
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "g_LogModePacket is NULL\n");
            }
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "pAttr->attr.name = %s \n", pAttr->attr.name);
    }

    return nLength;
}

ssize_t DrvMainKObjectPacketStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
/*
    if (strcmp(pAttr->attr.name, "packet") == 0)
    {

    }
*/    	
    return nCount;
}

static struct kobj_attribute packet_attr = __ATTR(packet, 0664, DrvMainKObjectPacketShow, DrvMainKObjectPacketStore);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *attrs[] = {
    &packet_attr.attr,
    NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
};


//------------------------------------------------------------------------------//

ssize_t DrvMainProcfsQueryFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%d", _gFeatureSupportStatus);

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsQueryFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nFeature;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (pBuffer != NULL)
    {
        sscanf(pBuffer, "%x", &nFeature);   
        DBG(&g_I2cClient->dev, "nFeature = 0x%x\n", nFeature);

        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_WAKEUP_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_DEBUG_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_INFORMATION_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG)
        {
            _gFeatureSupportStatus = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        }
        else if (nFeature == FEATURE_FIRMWARE_DATA_LOG)
        {
            _gFeatureSupportStatus = IS_FIRMWARE_DATA_LOG_ENABLED;
            
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
            {
                if (_gFeatureSupportStatus == 1) // If the debug mode data log function is supported, then get packet address and flag address for segment read finger touch data.
                {
                    DrvIcFwLyrGetTouchPacketAddress(&g_FwPacketDataAddress, &g_FwPacketFlagAddress);
                }
            }
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
        }
        else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE)
        {
            _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Feature ***\n");
        }
    }

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);
    
    return nCount;
}

ssize_t DrvMainProcfsChangeFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(pBuffer, "%d", _gFeatureSupportStatus);

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsChangeFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i;
    u32 nFeature = 0, nNewValue = 0;
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (pBuffer != NULL)
    {
        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);

        memset(_gDebugBuf, 0, 1024);

        if (copy_from_user(_gDebugBuf, pBuffer, nCount))
        {
            DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

            return -EFAULT;
        }
       
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;
        
        i = 0;
        
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 3))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if (i == 0)
            {
                nFeature = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "nFeature = 0x%04X\n", nFeature);
            }
            else if (i == 1)
            {	
                nNewValue = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "nNewValue = %d\n", nNewValue);
            }
            else
            {
                DBG(&g_I2cClient->dev, "End of parsing adb command.\n");
            }

            i ++;
        }

        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) 
        {
            IS_GESTURE_WAKEUP_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_WAKEUP_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) 
        {
            IS_GESTURE_DEBUG_MODE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_DEBUG_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) 
        {
            IS_GESTURE_INFORMATION_MODE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_INFORMATION_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG)
        {
            TOUCH_DRIVER_DEBUG_LOG_LEVEL = nNewValue;
            _gFeatureSupportStatus = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        }
        else if (nFeature == FEATURE_FIRMWARE_DATA_LOG)
        {
            IS_FIRMWARE_DATA_LOG_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_FIRMWARE_DATA_LOG_ENABLED;
        }
        else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE)
        {
            IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Feature ***\n");
        }

        DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);
    }
    
    return nCount;
}

//------------------------------------------------------------------------------//


/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
ssize_t DrvMainProcfsReportRateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    struct timeval tEndTime;
    suseconds_t nStartTime, nEndTime, nElapsedTime;
    u32 nLength = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "g_InterruptCount = %d, g_ValidTouchCount = %d\n", g_InterruptCount, g_ValidTouchCount);
    
    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    // Get end time
    do_gettimeofday(&tEndTime);
    
    nStartTime = g_StartTime.tv_sec + g_StartTime.tv_usec/1000000;
    nEndTime = tEndTime.tv_sec + tEndTime.tv_usec/1000000;

    nElapsedTime = nEndTime - nStartTime;

    DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
    DBG(&g_I2cClient->dev, "End time : %lu sec, %lu msec\n", tEndTime.tv_sec, tEndTime.tv_usec); 

    DBG(&g_I2cClient->dev, "Elapsed time : %lu sec\n", nElapsedTime); 
    
    // Calculate report rate
    if (nElapsedTime != 0)
    {
        g_InterruptReportRate = g_InterruptCount / nElapsedTime;
        g_ValidTouchReportRate = g_ValidTouchCount / nElapsedTime;
    }
    else
    {
        g_InterruptReportRate = 0;		
        g_ValidTouchReportRate = 0;		
    }

    DBG(&g_I2cClient->dev, "g_InterruptReportRate = %d, g_ValidTouchReportRate = %d\n", g_InterruptReportRate, g_ValidTouchReportRate);

    g_InterruptCount = 0; // Reset count
    g_ValidTouchCount = 0;

    nLength = sprintf(pBuffer, "%d,%d", g_InterruptReportRate, g_ValidTouchReportRate);

    *pPos += nLength;

    return nLength;
}

ssize_t DrvMainProcfsReportRateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (pBuffer != NULL)
    {
        sscanf(pBuffer, "%d", &g_IsEnableReportRate);   

        DBG(&g_I2cClient->dev, "g_IsEnableReportRate = %d\n", g_IsEnableReportRate); // 1 : enable report rate calculation, 0 : disable report rate calculation, 2 : reset count

        g_InterruptCount = 0; // Reset count
        g_ValidTouchCount = 0;
    }
    
    return nCount;
}
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

/*--------------------------------------------------------------------------*/


//------------------------------------------------------------------------------//

s32 DrvMainTouchDeviceInitialize(void)
{
    s32 nRetVal = 0;

    TPD_ERR("*** tp905::%s() ***\n", __func__);
    _DrvMainCreateProcfsDirEntry();
	
	if (sysfs_create_file(kernel_kobj, &dev_attr_tp_info.attr)) {		
		TPD_ERR("[camerahw2_probe] sysfs_create_file front camera fail\n");		
		return -ENODEV; 
		}

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    CreateMsgToolMem();
#endif 

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    DrvIcFwLyrCreateMpTestWorkQueue();
#endif 
    
    g_ChipType = DrvIcFwLyrGetChipType(); // Try to get chip type by SLAVE_I2C_ID_DBBUS(0x62) firstly.
    if (g_ChipType == 0) // If failed, try to get chip type by SLAVE_I2C_ID_DBBUS(0x59) again.
    {
        SLAVE_I2C_ID_DBBUS = (0xB2>>1); //0x59
        g_ChipType = DrvIcFwLyrGetChipType(); 
    }
    DrvPlatformLyrTouchDeviceResetHw();

    if (g_ChipType != 0) // To make sure TP is attached on cell phone.
    {
        if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
        {
            memset(&g_MutualFirmwareInfo, 0x0, sizeof(MutualFirmwareInfo_t));
        }
        else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
        {
            memset(&g_SelfFirmwareInfo, 0x0, sizeof(SelfFirmwareInfo_t));
        }

        DrvIcFwLyrVariableInitialize();

#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
        {
			//#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
            u32 nChargerStatus = 0; 
            nChargerStatus = upmu_is_chr_det();
            DBG(&g_I2cClient->dev, "*** Battery Status : %d ***\n", nChargerStatus);
            if (nChargerStatus) // Charging
            {
                DrvFwCtrlChargerDetection(1); // charger plug-in
            }
            else // Not charging
            {
                DrvFwCtrlChargerDetection(0); // charger plug-out
            }
        }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION
    }
    else
    {
        nRetVal = -ENODEV;
    }

    return nRetVal;
}

void DrvMainRemoveProcfsDirEntry(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);


#ifdef CONFIG_ENABLE_JNI_INTERFACE
    if (_gProcJniMethodEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_JNI_NODE, _gProcDeviceEntry);    		
        _gProcJniMethodEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_JNI_NODE);
    }
#endif //CONFIG_ENABLE_JNI_INTERFACE	

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (_gProcReportRateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_REPORT_RATE, _gProcDeviceEntry);    	
        _gProcReportRateEntry = NULL;	
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_REPORT_RATE);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE


    if (_gProcFirmwareModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_MODE, _gProcDeviceEntry);    
        _gProcFirmwareModeEntry = NULL;		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_MODE);
    }

    if (_gProcFirmwareSensorEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SENSOR, _gProcDeviceEntry);  
        _gProcFirmwareSensorEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SENSOR);
    }

    if (_gProcFirmwarePacketHeaderEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_PACKET_HEADER, _gProcDeviceEntry);    		
        _gProcFirmwarePacketHeaderEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_PACKET_HEADER);
    }

    if (_gProcQueryFeatureSupportStatusEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS, _gProcDeviceEntry);   
        _gProcQueryFeatureSupportStatusEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS);
    }

    if (_gProcChangeFeatureSupportStatusEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS, _gProcDeviceEntry);  
        _gProcChangeFeatureSupportStatusEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS);
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    if (_gProcMpTestEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST, _gProcDeviceEntry);    		
        _gProcMpTestEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST);
    }

    if (_gProcMpTestLogEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_LOG, _gProcDeviceEntry);  
        _gProcMpTestLogEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_LOG);
    }

    if (_gProcMpTestFailChannelEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_FAIL_CHANNEL, _gProcDeviceEntry);    		
        _gProcMpTestFailChannelEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_FAIL_CHANNEL);
    }

    if (_gProcMpTestScopeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_SCOPE, _gProcDeviceEntry); 
        _gProcMpTestScopeEntry = NULL;   		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_SCOPE);
    }
#endif //CONFIG_ENABLE_ITO_MP_TEST

    if (_gProcFirmwareSetDQMemValueEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SET_DQMEM_VALUE, _gProcDeviceEntry);    		
        _gProcFirmwareSetDQMemValueEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DQMEM_VALUE);
    }

    if (_gProcFirmwareSmBusDebugEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SMBUS_DEBUG, _gProcDeviceEntry);    
        _gProcFirmwareSmBusDebugEntry = NULL;		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SMBUS_DEBUG);
    }

    if (_gProcFirmwareSetDebugValueEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SET_DEBUG_VALUE, _gProcDeviceEntry);  
        _gProcFirmwareSetDebugValueEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DEBUG_VALUE);
    }

    if (_gProcFirmwareDebugEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_DEBUG, _gProcDeviceEntry);    	
        _gProcFirmwareDebugEntry = NULL;	
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DEBUG);
    }

    if (_gProcSdCardFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_SD_CARD_FIRMWARE_UPDATE, _gProcDeviceEntry);  
        _gProcSdCardFirmwareUpdateEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_SD_CARD_FIRMWARE_UPDATE);
    }

    if (_gProcSeLinuxLimitFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcSeLinuxLimitFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE);
    }

    if (_gProcForceFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FORCE_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcForceFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FORCE_FIRMWARE_UPDATE);
    }

    if (_gProcDeviceDriverVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_DEVICE_DRIVER_VERSION, _gProcDeviceEntry);
        _gProcDeviceDriverVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_DEVICE_DRIVER_VERSION);
    }

    if (_gProcPlatformFirmwareVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_PLATFORM_FIRMWARE_VERSION, _gProcDeviceEntry);
        _gProcPlatformFirmwareVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_PLATFORM_FIRMWARE_VERSION);
    }

    if (_gProcCustomerFirmwareVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CUSTOMER_FIRMWARE_VERSION, _gProcDeviceEntry);
        _gProcCustomerFirmwareVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CUSTOMER_FIRMWARE_VERSION);
    }

    if (_gProcApkFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcApkFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_UPDATE);
    }

    if (_gProcFirmwareDataEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_DATA, _gProcDeviceEntry);
        _gProcFirmwareDataEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DATA);
    }

    if (_gProcChipTypeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CHIP_TYPE, _gProcDeviceEntry);
        _gProcChipTypeEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CHIP_TYPE);
    }

    if (_gProcDeviceEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_DEVICE, _gProcMsTouchScreenMsg20xxEntry);
        _gProcDeviceEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_DEVICE);
    }

    if (_gProcMsTouchScreenMsg20xxEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, _gProcClassEntry);
        _gProcMsTouchScreenMsg20xxEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MS_TOUCHSCREEN_MSG20XX);
    }

    if (_gProcClassEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CLASS, NULL);
        _gProcClassEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CLASS);
    }
}

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static s32 _DrvMainCreateProcfsDirEntry(void)
{
    s32 nRetVal = 0;
    u8 *pDevicePath = NULL;
   
	TPD_ERR("*** %s() ***\n", __func__);
    _gProcClassEntry = proc_mkdir(PROC_NODE_CLASS, NULL);
    _gProcMsTouchScreenMsg20xxEntry = proc_mkdir(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, _gProcClassEntry);
    _gProcDeviceEntry = proc_mkdir(PROC_NODE_DEVICE, _gProcMsTouchScreenMsg20xxEntry);
    _gProcChipTypeEntry = proc_create(PROC_NODE_CHIP_TYPE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcChipType);
    if (NULL == _gProcChipTypeEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_CHIP_TYPE);
	
    _gProcFirmwareDataEntry = proc_create(PROC_NODE_FIRMWARE_DATA, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareData);
    if (NULL == _gProcFirmwareDataEntry)
		TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_DATA);
     
    _gProcApkFirmwareUpdateEntry = proc_create(PROC_NODE_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcApkFirmwareUpdate);
    if (NULL == _gProcApkFirmwareUpdateEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_UPDATE);
   
    _gProcCustomerFirmwareVersionEntry = proc_create(PROC_NODE_CUSTOMER_FIRMWARE_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcCustomerFirmwareVersion);
    if (NULL == _gProcCustomerFirmwareVersionEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_CUSTOMER_FIRMWARE_VERSION);
      
    _gProcPlatformFirmwareVersionEntry = proc_create(PROC_NODE_PLATFORM_FIRMWARE_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcPlatformFirmwareVersion);
    if (NULL == _gProcPlatformFirmwareVersionEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_PLATFORM_FIRMWARE_VERSION);
   
    _gProcDeviceDriverVersionEntry = proc_create(PROC_NODE_DEVICE_DRIVER_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcDeviceDriverVersion);
    if (NULL == _gProcDeviceDriverVersionEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_DEVICE_DRIVER_VERSION);
    
    _gProcSdCardFirmwareUpdateEntry = proc_create(PROC_NODE_SD_CARD_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcSdCardFirmwareUpdate);
    if (NULL == _gProcSdCardFirmwareUpdateEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_SD_CARD_FIRMWARE_UPDATE);
     
    
    _gProcFirmwareDebugEntry = proc_create(PROC_NODE_FIRMWARE_DEBUG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareDebug);
    if (NULL == _gProcFirmwareDebugEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_DEBUG);
 
    _gProcFirmwareSetDebugValueEntry = proc_create(PROC_NODE_FIRMWARE_SET_DEBUG_VALUE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSetDebugValue);
    if (NULL == _gProcFirmwareSetDebugValueEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SET_DEBUG_VALUE);
   
    _gProcFirmwareSmBusDebugEntry = proc_create(PROC_NODE_FIRMWARE_SMBUS_DEBUG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSmBusDebug);
    if (NULL == _gProcFirmwareSmBusDebugEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SMBUS_DEBUG);
   
    _gProcFirmwareSetDQMemValueEntry = proc_create(PROC_NODE_FIRMWARE_SET_DQMEM_VALUE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSetDQMemValue);
    if (NULL == _gProcFirmwareSetDQMemValueEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SET_DQMEM_VALUE);
   
#ifdef CONFIG_ENABLE_ITO_MP_TEST
    _gProcMpTestEntry = proc_create(PROC_NODE_MP_TEST, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTest);
    if (NULL == _gProcMpTestEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST);
  
    _gProcMpTestLogEntry = proc_create(PROC_NODE_MP_TEST_LOG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestLog);
    if (NULL == _gProcMpTestLogEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_LOG);
  
    _gProcMpTestFailChannelEntry = proc_create(PROC_NODE_MP_TEST_FAIL_CHANNEL, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestFailChannel);
    if (NULL == _gProcMpTestFailChannelEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_FAIL_CHANNEL);
  
    _gProcMpTestScopeEntry = proc_create(PROC_NODE_MP_TEST_SCOPE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestScope);
    if (NULL == _gProcMpTestScopeEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_SCOPE);
#endif //CONFIG_ENABLE_ITO_MP_TEST

    _gProcFirmwareModeEntry = proc_create(PROC_NODE_FIRMWARE_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareMode);
    if (NULL == _gProcFirmwareModeEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_MODE);

    _gProcFirmwareSensorEntry = proc_create(PROC_NODE_FIRMWARE_SENSOR, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSensor);
    if (NULL == _gProcFirmwareSensorEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SENSOR);
   

    _gProcFirmwarePacketHeaderEntry = proc_create(PROC_NODE_FIRMWARE_PACKET_HEADER, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwarePacketHeader);
    if (NULL == _gProcFirmwarePacketHeaderEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_PACKET_HEADER);
 
    /* create a kset with the name of "kset_example" which is located under /sys/kernel/ */
    g_TouchKSet = kset_create_and_add("kset_example", NULL, kernel_kobj);
    if (!g_TouchKSet)
    {
        TPD_ERR("*** kset_create_and_add() failed, nRetVal = %d ***\n", nRetVal);
        nRetVal = -ENOMEM;
    }
	
    g_TouchKObj = kobject_create();
    if (!g_TouchKObj)
    {
        TPD_ERR("*** kobject_create() failed, nRetVal = %d ***\n", nRetVal);
        nRetVal = -ENOMEM;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }

    g_TouchKObj->kset = g_TouchKSet;
    nRetVal = kobject_add(g_TouchKObj, NULL, "%s", "kobject_example");
    if (nRetVal != 0)
    {
        TPD_ERR("*** kobject_add() failed, nRetVal = %d ***\n", nRetVal);
        kobject_put(g_TouchKObj);
        g_TouchKObj = NULL;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }
    
    /* create the files associated with this kobject */
    nRetVal = sysfs_create_group(g_TouchKObj, &attr_group);
    if (nRetVal != 0)
    {
        TPD_ERR("*** sysfs_create_file() failed, nRetVal = %d ***\n", nRetVal);
        kobject_put(g_TouchKObj);
        g_TouchKObj = NULL;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }
    
    pDevicePath = kobject_get_path(g_TouchKObj, GFP_KERNEL);
    TPD_ERR("DEVPATH = %s\n", pDevicePath);
    _gProcQueryFeatureSupportStatusEntry = proc_create(PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcQueryFeatureSupportStatus);
    if (NULL == _gProcQueryFeatureSupportStatusEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS);
    
    _gProcChangeFeatureSupportStatusEntry = proc_create(PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcChangeFeatureSupportStatus);
    if (NULL == _gProcChangeFeatureSupportStatusEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS);
   


#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    _gProcReportRateEntry = proc_create(PROC_NODE_REPORT_RATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcReportRate);
    if (NULL == _gProcReportRateEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_REPORT_RATE);
#endif 


#ifdef CONFIG_ENABLE_JNI_INTERFACE
    _gProcJniMethodEntry = proc_create(PROC_NODE_JNI_NODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcJniMethod);
    if (NULL == _gProcJniMethodEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_JNI_NODE);
#endif 

    _gProcSeLinuxLimitFirmwareUpdateEntry = proc_create(PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcSeLinuxLimitFirmwareUpdate);
    if (NULL == _gProcSeLinuxLimitFirmwareUpdateEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE);
  
    _gProcForceFirmwareUpdateEntry = proc_create(PROC_NODE_FORCE_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcForceFirmwareUpdate);
    if (NULL == _gProcForceFirmwareUpdateEntry)
        TPD_ERR("Failed to create procfs file node(%s)!\n", PROC_NODE_FORCE_FIRMWARE_UPDATE);
    return nRetVal;
}


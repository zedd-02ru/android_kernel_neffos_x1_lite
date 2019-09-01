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
 * @file    mstar_drv_ic_fw_porting_layer.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */
 
/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_printlog.h"

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern struct i2c_client *g_I2cClient;


/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

void DrvIcFwLyrVariableInitialize(void)
{
    DrvFwCtrlVariableInitialize();
}

void DrvIcFwLyrOptimizeCurrentConsumption(void)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlOptimizeCurrentConsumption();
}

u8 DrvIcFwLyrGetChipType(void)
{
    return DrvFwCtrlGetChipType();
}

void DrvIcFwLyrGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlGetCustomerFirmwareVersion(pMajor, pMinor, ppVersion);
}

void DrvIcFwLyrGetPlatformFirmwareVersion(u8 **ppVersion)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlGetPlatformFirmwareVersion(ppVersion);
}

s32 DrvIcFwLyrUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlUpdateFirmware(szFwData, eEmemType);
}	

s32 DrvIcFwLyrUpdateFirmwareBySdCard(const char *pFilePath)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlUpdateFirmwareBySdCard(pFilePath);
}

u32 DrvIcFwLyrIsRegisterFingerTouchInterruptHandler(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return 1; // Indicate that it is necessary to register interrupt handler with GPIO INT pin when firmware is running on IC
}

void DrvIcFwLyrHandleFingerTouch(u8 *pPacket, u16 nLength)
{
    DrvFwCtrlHandleFingerTouch();
}

//------------------------------------------------------------------------------//


u32 DrvIcFwLyrReadDQMemValue(u16 nAddr)
{
//	  DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlReadDQMemValue(nAddr);
}

void DrvIcFwLyrWriteDQMemValue(u16 nAddr, u32 nData)
{
//	  DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlWriteDQMemValue(nAddr, nData);
}

//------------------------------------------------------------------------------//

u16 DrvIcFwLyrGetFirmwareMode(void) // used for MSG26xxM only
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlGetFirmwareMode();
}

u16 DrvIcFwLyrChangeFirmwareMode(u16 nMode)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlChangeFirmwareMode(nMode); 
}

void DrvIcFwLyrSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlSelfGetFirmwareInfo(pInfo);
}

void DrvIcFwLyrMutualGetFirmwareInfo(MutualFirmwareInfo_t *pInfo)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlMutualGetFirmwareInfo(pInfo);
}

void DrvIcFwLyrRestoreFirmwareModeToLogDataMode(void)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvFwCtrlRestoreFirmwareModeToLogDataMode();
}	

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
void DrvIcFwLyrCheckFirmwareUpdateBySwId(void)
{
    DrvFwCtrlCheckFirmwareUpdateBySwId();
}
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
void DrvIcFwLyrGetMpTestScope(TestScopeInfo_t *pInfo) // for MSG26xxM/MSG28xx
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvMpTestGetTestScope(pInfo);
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

void DrvIcFwLyrCreateMpTestWorkQueue(void)
{
    DrvMpTestCreateMpTestWorkQueue();
}

void DrvIcFwLyrScheduleMpTestWork(ItoTestMode_e eItoTestMode)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	
    DrvMpTestScheduleMpTestWork(eItoTestMode);
}

s32 DrvIcFwLyrGetMpTestResult(void)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	
    return DrvMpTestGetTestResult();
}

void DrvIcFwLyrGetMpTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	
    return DrvMpTestGetTestFailChannel(eItoTestMode, pFailChannel, pFailChannelCount);
}

void DrvIcFwLyrGetMpTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvMpTestGetTestDataLog(eItoTestMode, pDataLog, pLength);
}
#endif //CONFIG_ENABLE_ITO_MP_TEST		

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
void DrvIcFwLyrGetTouchPacketAddress(u16 *pDataAddress, u16 *pFlagAddress)
{
//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvFwCtrlGetTouchPacketAddress(pDataAddress, pFlagAddress);
}
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

//------------------------------------------------------------------------------//





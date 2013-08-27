/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_vo.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/12/17
  Description   :
  History       :
  1.Date        : 2009/12/17
    Author      : w58735
    Modification: Created file

*******************************************************************************/
#include "hi_drv_win.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif

HI_S32 HI_MPI_WIN_Init(HI_VOID);

HI_S32 HI_MPI_WIN_DeInit(HI_VOID);

HI_S32 HI_MPI_WIN_Create(const HI_DRV_WIN_ATTR_S *pWinAttr, HI_HANDLE *phWindow);

HI_S32 HI_MPI_WIN_Destroy(HI_HANDLE hWindow);

HI_S32 HI_MPI_WIN_SetEnable(HI_HANDLE hWindow, HI_BOOL bEnable);

HI_S32 HI_MPI_WIN_GetEnable(HI_HANDLE hWindow, HI_BOOL *pbEnable);

HI_S32 HI_MPI_WIN_SetAttr(HI_HANDLE hWindow, const HI_DRV_WIN_ATTR_S *pWinAttr);

HI_S32 HI_MPI_WIN_GetAttr(HI_HANDLE hWindow, HI_DRV_WIN_ATTR_S *pWinAttr);

HI_S32 HI_MPI_WIN_SendFrame(HI_HANDLE hWindow, HI_DRV_VIDEO_FRAME_S *pFrame);

HI_S32 HI_MPI_WIN_DequeueFrame(HI_HANDLE hWindow, HI_DRV_VIDEO_FRAME_S *pFrame);

HI_S32 HI_MPI_WIN_QueueFrame(HI_HANDLE hWindow, HI_DRV_VIDEO_FRAME_S *pFrame);

HI_S32 HI_MPI_WIN_QueueUselessFrame(HI_HANDLE hWindow, HI_DRV_VIDEO_FRAME_S *pFrame);

HI_S32 HI_MPI_WIN_Reset(HI_HANDLE hWindow, HI_DRV_WIN_SWITCH_E enWinFreezeMode);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


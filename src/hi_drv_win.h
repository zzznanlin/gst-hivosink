/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_drv_win.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/12/17
  Description   :
  History       :
  1.Date        : 2009/12/17
    Author      : w58735
    Modification: Created file

******************************************************************************/

#ifndef __HI_DRV_WIN_H__
#define __HI_DRV_WIN_H__

#include "hi_drv_disp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


#define HI_FATAL_WIN 	printf
#define HI_ERR_WIN		printf
#define HI_WARN_WIN		printf
#define HI_INFO_WIN		printf

#define DEF_MAX_WIN_NUM_ON_SINGLE_DISP 16

/* window type */
typedef enum hiDRV_WIN_TYPE_E
{
    HI_DRV_WIN_ACTIVE_SINGLE = 0,
    HI_DRV_WIN_VITUAL_SINGLE,
    HI_DRV_WIN_ACTIVE_MAIN_AND_SLAVE,
    HI_DRV_WIN_ACTIVE_SLAVE,
    HI_DRV_WIN_BUTT
}HI_DRV_WIN_TYPE_E;


/* window swtich mode, in reset mode */
typedef enum hiDRV_WIN_SWITCH_E
{
    HI_DRV_WIN_SWITCH_LAST = 0,
    HI_DRV_WIN_SWITCH_BLACK = 1,
    HI_DRV_WIN_SWITCH_BUTT
} HI_DRV_WIN_SWITCH_E;

/* window attribute */
typedef struct hiDRV_WIN_ATTR_S
{
    HI_BOOL bVirtual;

    /* not change when window lives */
    HI_DRV_DISPLAY_E  enDisp;

    /* may change when window lives */
    HI_DRV_ASPECT_RATIO_S stCustmAR;
    HI_DRV_ASP_RAT_MODE_E enARCvrs;

    HI_BOOL bUseCropRect;
    HI_RECT_S stInRect;
    HI_DRV_CROP_RECT_S stCropRect;

    HI_RECT_S stOutRect;

    /* only for virtual window */
    HI_BOOL             bUserAllocBuffer;
    HI_U32              u32BufNumber; /* [1,16] */
    HI_DRV_PIX_FORMAT_E enDataFormat;
}HI_DRV_WIN_ATTR_S;

/* window information */
typedef struct hiDRV_WIN_INFO_S
{
    HI_DRV_WIN_TYPE_E eType;

    HI_HANDLE hPrim;
    HI_HANDLE hSec;
}HI_DRV_WIN_INFO_S;

typedef HI_S32 (*PFN_GET_FRAME_CALLBACK)(HI_HANDLE hHd, HI_DRV_VIDEO_FRAME_S *pstFrm);
typedef HI_S32 (*PFN_PUT_FRAME_CALLBACK)(HI_HANDLE hHd, HI_DRV_VIDEO_FRAME_S *pstFrm);
typedef HI_S32 (*PFN_GET_WIN_INFO_CALLBACK)(HI_HANDLE hHd, HI_DRV_WIN_PRIV_INFO_S *pstWin);

/* source information.
   window will get / release frame or send private info to sourec
   by function pointer */
typedef struct hiDRV_WIN_SRC_INFO_S
{
    HI_HANDLE hSrc;

    PFN_GET_FRAME_CALLBACK pfAcqFrame;
    PFN_PUT_FRAME_CALLBACK pfRlsFrame;
    PFN_GET_WIN_INFO_CALLBACK pfSendWinInfo;

    HI_U32    u32Resrve0;
    HI_U32    u32Resrve1;
}HI_DRV_WIN_SRC_INFO_S;

/* window current play information, player gets it and adjust Audio and Video
   play rate */
typedef struct hiDRV_WIN_PLAY_INFO_S
{
    HI_U32    u32DelayTime; /* in ms */
    HI_U32    u32DispRate;  /* in 1/100 Hz */
    HI_U32    u32FrameNumInBufQn;

    HI_BOOL   bTBMatch;  /* for interlace frame display on interlace timing */
}HI_DRV_WIN_PLAY_INFO_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_DRV_WIN_H__ */



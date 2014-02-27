/* GStreamer
 * Copyright (C) 2013 LiuDongyang <dongyang.liu@huawei.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gsthivosink
 *
 * The hivosink element rendering video frames in VO of SDK.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! hivosink ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include "gsthivosink.h"

/* sdk header files */
#include "hi_unf_vo.h"

#define GST_RANK_HISI  256


GST_DEBUG_CATEGORY_STATIC (gst_hivosink_debug_category);
#define GST_CAT_DEFAULT gst_hivosink_debug_category

#define DEFAULT_WINDOW_RECT_X       0
#define DEFAULT_WINDOW_RECT_Y       0
#define DEFAULT_WINDOW_RECT_WIDTH   0
#define DEFAULT_WINDOW_RECT_HEIGHT  0
#define DEFAULT_WINDOW_FREEZE       0
#define DEFAULT_STOP_KEEP_FRAME     0
#define DEFAULT_CURRENT_TIMESTAMP   0

/* prototypes */
enum
{
    PROP_0,
    PROP_WINDOW_RECT,
    PROP_WINDOW_FREEZE,
    PROP_STOP_KEEP_FRAME,
    PROP_CURRENT_TIMESTAMP,
};


static void gst_hivosink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_hivosink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_hivosink_dispose (GObject * object);
static void gst_hivosink_finalize (GObject * object);


static GstFlowReturn
gst_hivosink_show_frame (GstVideoSink * video_sink, GstBuffer * buf);



/* pad templates */
static GstStaticPadTemplate gst_hivosink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-rgb, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], "
        "height = (int) [ 1, MAX ]; "
        "video/x-raw-yuv, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]")
    );


/* class initialization */

#define DEBUG_INIT(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_hivosink_debug_category, "hivosink", 0, \
        "debug category for hivosink element");

GST_BOILERPLATE_FULL (GstHivosink, gst_hivosink, GstVideoSink,
    GST_TYPE_VIDEO_SINK, DEBUG_INIT);

static void gst_hivosink_base_init (gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

    gst_element_class_add_pad_template (element_class,
        gst_static_pad_template_get (&gst_hivosink_sink_template));

    gst_element_class_set_details_simple (element_class, "hisi video sink",
        "Sink/Video", "a HISI VO based videosink", "Yan Haifeng <haifeng.yan@linaro.org>");
}

static void gst_hivosink_class_init (GstHivosinkClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS (klass);

    gobject_class->set_property = gst_hivosink_set_property;
    gobject_class->get_property = gst_hivosink_get_property;
    gobject_class->dispose = gst_hivosink_dispose;
    gobject_class->finalize = gst_hivosink_finalize;
    video_sink_class->show_frame = GST_DEBUG_FUNCPTR (gst_hivosink_show_frame);

    g_object_class_install_property (gobject_class, PROP_WINDOW_RECT,
        g_param_spec_string ("window-rect", "Window Rect",
            "The overylay window rect (x,y,width,height)",
            NULL,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_WINDOW_FREEZE,
        g_param_spec_boolean ("freeze", "Window Freeze",
            "freeze/unfreeze video",
            DEFAULT_WINDOW_FREEZE,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_STOP_KEEP_FRAME,
        g_param_spec_boolean ("stop-keep-frame", "stop-keep-frame",
            "Keep displaying the last frame when stop",
            DEFAULT_STOP_KEEP_FRAME,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CURRENT_TIMESTAMP,
        g_param_spec_uint64("current-timestamp", "current-timestamp",
            "Video decoder handle in use",
            0, G_MAXUINT64, DEFAULT_CURRENT_TIMESTAMP,
            G_PARAM_READABLE));

}

static gint hi_vo_init (GstHivosink * hivosink)
{
  HI_UNF_WINDOW_ATTR_S WinAttr;
  guint hWin;
  gint ret;

  ret = HI_UNF_VO_Init (HI_UNF_VO_DEV_MODE_NORMAL);
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_UNF_VO_Init failed. ret:0x%x");
    return HI_FAILURE;
  }

  WinAttr.enDisp = HI_UNF_DISPLAY1;
  WinAttr.bVirtual = HI_FALSE;
  WinAttr.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;
  WinAttr.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
  WinAttr.stWinAspectAttr.u32UserAspectWidth  = 0;
  WinAttr.stWinAspectAttr.u32UserAspectHeight = 0;
  WinAttr.bUseCropRect = HI_FALSE;
  WinAttr.stInputRect.s32X = 0;
  WinAttr.stInputRect.s32Y = 0;
  WinAttr.stInputRect.s32Width = 0;
  WinAttr.stInputRect.s32Height = 0;
  WinAttr.stOutputRect.s32X = hivosink->x;
  WinAttr.stOutputRect.s32Y = hivosink->y;
  WinAttr.stOutputRect.s32Width = hivosink->width;
  WinAttr.stOutputRect.s32Height = hivosink->height;

  ret = HI_UNF_VO_CreateWindow (&WinAttr, &hWin);
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_UNF_VO_CreateWindow failed. ret:0x%x", ret);
    goto vo_deinit;
  }

  ret = HI_UNF_VO_SetWindowEnable (hWin, TRUE);
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_UNF_VO_SetWindowEnable failed. ret:0x%x", ret);
    goto win_destroy;
  }

  hivosink->vo_hdl = hWin;
  GST_INFO ("vo window created, vo_hdl:%p", hWin);

  return HI_SUCCESS;

win_destroy:
  if (HI_SUCCESS != HI_UNF_VO_DestroyWindow (hWin))
    GST_ERROR ("call HI_UNF_VO_DestroyWindow failed.");

vo_deinit:
  if (HI_SUCCESS != HI_UNF_VO_DeInit ())
    GST_ERROR ("call HI_UNF_VO_DeInit failed.");

  return HI_FAILURE;
}


static void gst_hivosink_init (GstHivosink * hivosink, GstHivosinkClass * hivosink_class)
{
    gint ret;

    hivosink->sinkpad =
        gst_pad_new_from_static_template (&gst_hivosink_sink_template, "sink");

    hivosink->vo_hdl = 0;
    hivosink->frame_width = 0;
    hivosink->frame_height = 0;
    hivosink->x = DEFAULT_WINDOW_RECT_X;
    hivosink->y = DEFAULT_WINDOW_RECT_Y;
    hivosink->width = DEFAULT_WINDOW_RECT_WIDTH;
    hivosink->height = DEFAULT_WINDOW_RECT_HEIGHT;
    hivosink->freeze = DEFAULT_WINDOW_FREEZE;
    hivosink->stop_keep_frame = DEFAULT_STOP_KEEP_FRAME;
    hivosink->current_timestamp = DEFAULT_CURRENT_TIMESTAMP;

    ret = hi_vo_init (hivosink);
    if (HI_SUCCESS != ret)
        GST_ERROR ("hi_vo_init failed!");
}


void gst_hivosink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
    GstHivosink *hivosink;

    g_return_if_fail (GST_IS_HIVOSINK (object));
    hivosink = GST_HIVOSINK (object);

    switch (property_id)
    {
        case PROP_WINDOW_RECT:
            {
                gint ret;
                gint tempx,tempy,tempw,temph;
                HI_UNF_WINDOW_ATTR_S WinAttr;

                ret = sscanf(g_value_get_string (value), "%d,%d,%d,%d",
                    &tempx, &tempy, &tempw, &temph);
                if( ret == 4 )
                {
                    hivosink->x = tempx;
                    hivosink->y = tempy;
                    hivosink->width = tempw;
                    hivosink->height = temph;

                    if(hivosink->vo_hdl)
                    {
                        HI_UNF_VO_GetWindowAttr(hivosink->vo_hdl, &WinAttr);
                        WinAttr.stOutputRect.s32X = hivosink->x;
                        WinAttr.stOutputRect.s32Y = hivosink->y;
                        WinAttr.stOutputRect.s32Width = hivosink->width;
                        WinAttr.stOutputRect.s32Height = hivosink->height;
                        HI_UNF_VO_SetWindowAttr(hivosink->vo_hdl, &WinAttr);
                    }
                }
            }
            break;
        case PROP_WINDOW_FREEZE:
            hivosink->freeze = g_value_get_boolean(value);
            HI_UNF_VO_FreezeWindow(hivosink->vo_hdl, hivosink->freeze, HI_UNF_WINDOW_FREEZE_MODE_LAST);
            break;
        case PROP_STOP_KEEP_FRAME:
            hivosink->stop_keep_frame = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void gst_hivosink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
    GstHivosink *hivosink;

    g_return_if_fail (GST_IS_HIVOSINK (object));
    hivosink = GST_HIVOSINK (object);

    switch (property_id)
    {
        case PROP_WINDOW_RECT:
            {
                char rect_str[64];
                sprintf(rect_str, "%d,%d,%d,%d",
                    hivosink->x, hivosink->y, hivosink->width, hivosink->height);
                g_value_set_string (value, rect_str);
            }
            break;
        case PROP_WINDOW_FREEZE:
            g_value_set_boolean(value, hivosink->freeze);
            break;
        case PROP_STOP_KEEP_FRAME:
            g_value_set_boolean(value, hivosink->stop_keep_frame);
            break;
        case PROP_CURRENT_TIMESTAMP:
            g_value_set_uint64(value, hivosink->current_timestamp);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void gst_hivosink_dispose (GObject * object)
{
    GstHivosink *hivosink;

    g_return_if_fail (GST_IS_HIVOSINK (object));
    hivosink = GST_HIVOSINK (object);

    /* clean up as possible.  may be called multiple times */
    if( hivosink->stop_keep_frame )
        HI_UNF_VO_ResetWindow(hivosink->vo_hdl, HI_UNF_WINDOW_FREEZE_MODE_LAST);
    else
        HI_UNF_VO_ResetWindow(hivosink->vo_hdl, HI_UNF_WINDOW_FREEZE_MODE_BLACK);

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

void gst_hivosink_finalize (GObject * object)
{
    GstHivosink *hivosink;

    g_return_if_fail (GST_IS_HIVOSINK (object));
    hivosink = GST_HIVOSINK (object);

    /* clean up object here */
    hivosink = hivosink;
    if( hivosink->vo_hdl)
        HI_UNF_VO_DestroyWindow(hivosink->vo_hdl);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


static HI_VOID SetVideoFrameInfoDefaultValue(HI_UNF_VIDEO_FRAME_INFO_S *pstFrame,
                                HI_U32 u32W, HI_U32 u32H, HI_U32 u32PhyAddr)
{
    HI_U32 u32StrW;

    if (!pstFrame)
    {
        GST_ERROR("Input null pointer!\n");
        return;
    }

    memset(pstFrame, 0, sizeof(HI_UNF_VIDEO_FRAME_INFO_S));

    u32StrW = (u32W + 0xf) & 0xFFFFFFF0ul;

    pstFrame->u32FrameIndex = 0;
    pstFrame->stVideoFrameAddr[0].u32YAddr = u32PhyAddr;
    pstFrame->stVideoFrameAddr[0].u32CAddr = u32PhyAddr + (u32StrW * u32H);
    pstFrame->stVideoFrameAddr[0].u32YStride = u32StrW;
    pstFrame->stVideoFrameAddr[0].u32CStride = u32StrW;

    pstFrame->u32Width  = u32W;
    pstFrame->u32Height = u32H;

    pstFrame->u32SrcPts = 0xffffffff;  /* 0xffffffff means unknown */
    pstFrame->u32Pts    = 0xffffffff;  /* 0xffffffff means unknown */

    pstFrame->u32AspectWidth  = 0;
    pstFrame->u32AspectHeight = 0;

    memset(&(pstFrame->stFrameRate), 0, sizeof(HI_UNF_VCODEC_FRMRATE_S));

    pstFrame->enVideoFormat = HI_UNF_FORMAT_YUV_SEMIPLANAR_420;
    pstFrame->bProgressive = HI_TRUE;
    pstFrame->enFieldMode  = HI_UNF_VIDEO_FIELD_ALL;
    pstFrame->bTopFieldFirst = HI_TRUE;

    pstFrame->enFramePackingType = HI_UNF_FRAME_PACKING_TYPE_NONE;
    pstFrame->u32Circumrotate = 0;
    pstFrame->bVerticalMirror = HI_FALSE;
    pstFrame->bHorizontalMirror = HI_FALSE;

    pstFrame->u32DisplayCenterX = pstFrame->u32Width/2;
    pstFrame->u32DisplayCenterY = pstFrame->u32Height/2;
    pstFrame->u32DisplayWidth  = pstFrame->u32Width;
    pstFrame->u32DisplayHeight = pstFrame->u32Height;

    pstFrame->u32ErrorLevel = 0;

    return;
}

static GstFlowReturn gst_hivosink_show_frame (GstVideoSink * video_sink, GstBuffer * buf)
{
    GstHivosink *hivosink;
    HI_UNF_VIDEO_FRAME_INFO_S FrameInfo;
    gint ret;
    GstStructure *structure;
    gint width,height;

    g_return_val_if_fail (GST_IS_BUFFER (buf), GST_FLOW_ERROR);

    hivosink = GST_HIVOSINK (video_sink);

    structure = gst_caps_get_structure (GST_BUFFER_CAPS(buf), 0);
    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);

    SetVideoFrameInfoDefaultValue(&FrameInfo, width, height, (HI_U32)GST_BUFFER_MALLOCDATA(buf));

    ret = HI_UNF_VO_QueueFrame(hivosink->vo_hdl, &FrameInfo);
    if (HI_SUCCESS != ret)
        GST_ERROR ("put frame to VO failed 0x%x!", ret);

    hivosink->current_timestamp = GST_BUFFER_TIMESTAMP(buf);

    return GST_FLOW_OK;
}


static gboolean plugin_init (GstPlugin * plugin)
{
    gst_element_register (plugin, "hivosink", GST_RANK_HISI,
        gst_hivosink_get_type ());

    return TRUE;
}

#ifndef VERSION
#define VERSION "0.10.32"
#endif
#ifndef PACKAGE
#define PACKAGE "gsthisivosink"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gsthisivosink"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN ""
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "hivosink",
    "HISI SDK vdec decoded video output using hivosink",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)


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

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include "gsthivosink.h"

/* changed: sdk header files */
#include "mpi_win.h"

#include "hi_drv_win.h"

#define GST_RANK_HISI  256


GST_DEBUG_CATEGORY_STATIC (gst_hivosink_debug_category);
#define GST_CAT_DEFAULT gst_hivosink_debug_category

/* prototypes */


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

static void
gst_hivosink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_hivosink_sink_template));

  gst_element_class_set_details_simple (element_class, "hisi video sink",
      "Sink/Video", "a HISI VO based videosink", "Yan Haifeng <haifeng.yan@linaro.org>");
}

static void
gst_hivosink_class_init (GstHivosinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS (klass);

  gobject_class->set_property = gst_hivosink_set_property;
  gobject_class->get_property = gst_hivosink_get_property;
  gobject_class->dispose = gst_hivosink_dispose;
  gobject_class->finalize = gst_hivosink_finalize;
  video_sink_class->show_frame = GST_DEBUG_FUNCPTR (gst_hivosink_show_frame);

}

static gint
hi_vo_init (GstHivosink * hivosink)
{
  HI_DRV_WIN_ATTR_S WinAttr;
  gint hWin;
  gint ret;

  ret = HI_MPI_WIN_Init ();
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_UNF_VO_Init failed. ret:0x%x");
    return HI_FAILURE;
  }

  WinAttr.bVirtual = HI_FALSE;
  WinAttr.enDisp = HI_DRV_DISPLAY_1;
  WinAttr.enARCvrs = HI_DRV_ASP_RAT_MODE_FULL;
  WinAttr.bUseCropRect = HI_FALSE;
  WinAttr.stInRect.s32X = 0;
  WinAttr.stInRect.s32Y = 0;
  WinAttr.stInRect.s32Width = 0;
  WinAttr.stInRect.s32Height = 0;
  WinAttr.stOutRect.s32X = 0;
  WinAttr.stOutRect.s32Y = 0;
  WinAttr.stOutRect.s32Width = 1920;
  WinAttr.stOutRect.s32Height = 1080;

  ret = HI_MPI_WIN_Create (&WinAttr, &hWin);
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_MPI_WIN_Create failed. ret:0x%x", ret);
    goto vo_deinit;
  }

  ret = HI_MPI_WIN_SetEnable (hWin, TRUE);
  if (HI_SUCCESS != ret) {
    GST_ERROR ("call HI_MPI_WIN_SetEnable failed. ret:0x%x", ret);
    goto win_destroy;
  }

  hivosink->vo_hdl = hWin;
  GST_INFO ("vo window created, vo_hdl:%p", hWin);

  return HI_SUCCESS;

win_destroy:
  if (HI_SUCCESS != HI_MPI_WIN_Destroy (hWin))
    GST_ERROR ("call HI_UNF_VO_DestroyWindow failed.");

vo_deinit:
  if (HI_SUCCESS != HI_MPI_WIN_DeInit ())
    GST_ERROR ("call HI_UNF_VO_DeInit failed.");

  return HI_FAILURE;
}

static void
gst_hivosink_init (GstHivosink * hivosink, GstHivosinkClass * hivosink_class)
{
  gint ret;

  hivosink->sinkpad =
      gst_pad_new_from_static_template (&gst_hivosink_sink_template, "sink");

  hivosink->vo_hdl = -1;
  hivosink->frame_width = 0;
  hivosink->frame_height = 0;

  ret = hi_vo_init (hivosink);
  if (HI_SUCCESS != ret)
    GST_ERROR ("hi_vo_init failed!");
}

void
gst_hivosink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstHivosink *hivosink;

  g_return_if_fail (GST_IS_HIVOSINK (object));
  hivosink = GST_HIVOSINK (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_hivosink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstHivosink *hivosink;

  g_return_if_fail (GST_IS_HIVOSINK (object));
  hivosink = GST_HIVOSINK (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_hivosink_dispose (GObject * object)
{
  GstHivosink *hivosink;

  g_return_if_fail (GST_IS_HIVOSINK (object));
  hivosink = GST_HIVOSINK (object);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_hivosink_finalize (GObject * object)
{
  GstHivosink *hivosink;

  g_return_if_fail (GST_IS_HIVOSINK (object));
  hivosink = GST_HIVOSINK (object);

  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#if 1
static gboolean
string_append_field (GQuark field, const GValue * value, gpointer ptr)
{
  GString *str = (GString *) ptr;
  gchar *value_str = gst_value_serialize (value);
  gchar *esc_value_str;

  /* some enums can become really long */
  if (strlen (value_str) > 25) {
    gint pos = 24;

    /* truncate */
    value_str[25] = '\0';

    /* mirror any brackets and quotes */
    if (value_str[0] == '<')
      value_str[pos--] = '>';
    if (value_str[0] == '[')
      value_str[pos--] = ']';
    if (value_str[0] == '(')
      value_str[pos--] = ')';
    if (value_str[0] == '{')
      value_str[pos--] = '}';
    if (value_str[0] == '"')
      value_str[pos--] = '"';
    if (pos != 24)
      value_str[pos--] = ' ';
    /* elippsize */
    value_str[pos--] = '.';
    value_str[pos--] = '.';
    value_str[pos--] = '.';
  }
  esc_value_str = g_strescape (value_str, NULL);

  g_string_append_printf (str, "  %18s: %s\\l", g_quark_to_string (field),
      esc_value_str);

  g_free (value_str);
  g_free (esc_value_str);
  return TRUE;
}


#define STRUCTURE_ESTIMATED_STRING_LEN(s) (16 + (s)->fields->len * 22)


static gchar *
try_dump_describe_caps (GstCaps * caps)
{
  gchar *media = NULL;
 
    if (gst_caps_is_any (caps) || gst_caps_is_empty (caps)) {
      media = gst_caps_to_string (caps);
 
    } else {
      GString *str = NULL;
      guint i;
      guint slen = 0;
 
      for (i = 0; i < gst_caps_get_size (caps); i++) {
        slen += 25 +
            STRUCTURE_ESTIMATED_STRING_LEN (gst_caps_get_structure (caps, i));
      }
 
      str = g_string_sized_new (slen);
      for (i = 0; i < gst_caps_get_size (caps); i++) {
        GstStructure *structure = gst_caps_get_structure (caps, i);
 
        g_string_append (str, gst_structure_get_name (structure));
        g_string_append (str, "\\l");
 
        gst_structure_foreach (structure, string_append_field, (gpointer) str);
      }
 
      media = g_string_free (str, FALSE);
    }
 
  return media;
}
#endif

static HI_VOID DrvSetVideoFrameInfoDefaultValue(HI_DRV_VIDEO_FRAME_S *pstFrame,
                                                     HI_U32 u32W, HI_U32 u32H, HI_U32 u32PhyAddr)
{
    HI_DRV_VIDEO_PRIVATE_S *pstPrivInfo;
    HI_U32 u32StrW, u32StrH;
 
    if (!pstFrame)
    {
        printf("Input null pointer!\n");
        return;
    }
 
    memset(pstFrame, 0, sizeof(HI_DRV_VIDEO_FRAME_S));
 
    u32StrW = (u32W + 0xf) & 0xFFFFFFF0ul;
 
    pstFrame->u32FrameIndex = 0;
    pstFrame->stBufAddr[0].u32PhyAddr_Y = u32PhyAddr; 
    pstFrame->stBufAddr[0].u32PhyAddr_C = u32PhyAddr + (u32StrW * u32H);
    pstFrame->stBufAddr[0].u32Stride_Y = u32StrW;
    pstFrame->stBufAddr[0].u32Stride_C = u32StrW;
 
    pstFrame->u32Width  = u32W;
    pstFrame->u32Height = u32H;
 
    pstFrame->u32SrcPts = 0xffffffff;  /* 0xffffffff means unknown */
    pstFrame->u32Pts    = 0xffffffff;  /* 0xffffffff means unknown */
 
    pstFrame->u32AspectWidth  = 0;
    pstFrame->u32AspectHeight = 0;
    pstFrame->u32FrameRate    = 0;     /* in 1/100 Hz, 0 means unknown */
 
    pstFrame->ePixFormat = HI_DRV_PIX_FMT_NV21;
    pstFrame->bProgressive = HI_TRUE;
    pstFrame->enFieldMode  = HI_DRV_FIELD_ALL;
    pstFrame->bTopFieldFirst = HI_TRUE;
 
    //display region in rectangle (x,y,w,h)
    pstFrame->stDispRect.s32X = 0;
    pstFrame->stDispRect.s32Y = 0;
    pstFrame->stDispRect.s32Width  = pstFrame->u32Width;
    pstFrame->stDispRect.s32Height = pstFrame->u32Height;
 
    pstFrame->eFrmType = HI_DRV_FT_NOT_STEREO;
    pstFrame->u32Circumrotate = 0;
    pstFrame->bToFlip_H = HI_FALSE;
    pstFrame->bToFlip_V = HI_FALSE;
    pstFrame->u32ErrorLevel = 0;
 
    pstPrivInfo = (HI_DRV_VIDEO_PRIVATE_S *)&(pstFrame->u32Priv[0]);
 
    pstPrivInfo->bValid = HI_TRUE;
    pstPrivInfo->u32LastFlag = HI_FALSE;
    pstPrivInfo->eColorSpace = HI_DRV_CS_BT709_YUV_LIMITED;
    pstPrivInfo->eOriginField = HI_DRV_FIELD_ALL;
    pstPrivInfo->stOriginImageRect = pstFrame->stDispRect;
 
    return;
}

static GstFlowReturn
gst_hivosink_show_frame (GstVideoSink * video_sink, GstBuffer * buf)
{
  GstHivosink *hivosink;
  HI_DRV_VIDEO_FRAME_S FrameInfo;
  gint ret;
  GstStructure *structure;
  gint width,height;

  g_return_val_if_fail (GST_IS_BUFFER (buf), GST_FLOW_ERROR);

  hivosink = GST_HIVOSINK (video_sink);

  structure = gst_caps_get_structure (GST_BUFFER_CAPS(buf), 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);

  DrvSetVideoFrameInfoDefaultValue(&FrameInfo, width, height, GST_BUFFER_MALLOCDATA(buf));

  ret = HI_MPI_WIN_QueueFrame (hivosink->vo_hdl, &FrameInfo);
  if (HI_SUCCESS != ret)
    GST_ERROR ("put frame to VO failed 0x%x!", ret);

  return GST_FLOW_OK;
}


static gboolean
plugin_init (GstPlugin * plugin)
{

  GST_ERROR ("plugin_init\n");
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


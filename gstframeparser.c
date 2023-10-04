//
// Created by Marat
//


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstcaps.h>
#include <gst/gstpad.h>
#include <gst/base/gstbaseparse.h>
#include "gstframeparser.h"
#include <inttypes.h>

GST_DEBUG_CATEGORY_STATIC (gst_frameparser_debug_category);
#define GST_CAT_DEFAULT gst_frameparser_debug_category

#define METADATA_SIZE 12

/* prototypes */

static void gst_frameparser_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_frameparser_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_frameparser_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_frameparser_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    //GST_STATIC_CAPS ("application/unknown")
    GST_STATIC_CAPS_ANY
    );

static GstStaticPadTemplate gst_frameparser_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    //GST_STATIC_CAPS ("application/unknown")
    GST_STATIC_CAPS_ANY
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstFrameparser, gst_frameparser, GST_TYPE_BASE_PARSE,
  GST_DEBUG_CATEGORY_INIT (gst_frameparser_debug_category, "frameparser", 0,
  "debug category for frameparser element"));

static void
gst_frameparser_class_init (GstFrameparserClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseParseClass *base_parse_class = GST_BASE_PARSE_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_frameparser_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_frameparser_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_frameparser_set_property;
  gobject_class->get_property = gst_frameparser_get_property;
  base_parse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_frameparser_handle_frame);
}

static void
gst_frameparser_init (GstFrameparser *frameparser)
{

  frameparser->frame_size = 0;
  frameparser->flags = GST_VIDEO_FRAME_FLAG_NONE;
  frameparser->format = GST_VIDEO_FORMAT_RGB;
  frameparser->height = 0;
  frameparser->width = 0;

}

void
gst_frameparser_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFrameparser *frameparser = GST_FRAMEPARSER (object);

  GST_DEBUG_OBJECT (frameparser, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_frameparser_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstFrameparser *frameparser = GST_FRAMEPARSER (object);

  GST_DEBUG_OBJECT (frameparser, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_frameparser_handle_frame (GstBaseParse * parse, GstBaseParseFrame * frame,
    gint * skipsize)
{
  GstFrameparser *frameparser = GST_FRAMEPARSER (parse);

  *skipsize = 0;

  GST_DEBUG_OBJECT (frameparser, "handle_frame");

  // Размер входного буффера
  gsize in_size = gst_buffer_get_size(frame->buffer);

  // Получем данные из входного буффера
  GstMapInfo map;
  if (!gst_buffer_map(frame->buffer, &map, GST_MAP_READ)) {
    GST_ERROR("Failed to map buffer for reading");
    return GST_FLOW_ERROR;
  }

  // Не хватет байт во входном буффере чтобы считать заголовок, ждем еще буфферы.
  if (in_size < METADATA_SIZE) {
    return GST_FLOW_OK;
  }

  // Если первая часть в новом rgb буффере
  if (frameparser->frame_size == 0){

    // Считываем параметры изображения 
    frameparser->frame_size = (*(guint32 *)(map.data)); // первые 4 байта длина
    frameparser->width = (*(guint32 *)(map.data + 4)); // вторые 4 байта ширина
    frameparser->height  = (*(guint32 *)(map.data + 8)); // третьи 4 байта высота

    if (frameparser->frame_size == 0 || frameparser->width == 0 || frameparser->height == 0) {
      GST_ELEMENT_ERROR(frameparser, STREAM, FORMAT, (NULL), ("Invalid data format: Invalid frame size or dimensions"));
      return GST_FLOW_ERROR;
    }

    // Формируем Caps
    GstCaps *caps;
    gchar *caps_string;
    caps_string = g_strdup_printf("video/x-raw,format=RGB,width=%d,height=%d", frameparser->width, frameparser->height);
    caps = gst_caps_from_string(caps_string);

    // Устанавливаем Caps в выходной Pad
    gst_pad_set_caps(parse->srcpad, caps);
    g_free(caps_string);
    gst_caps_unref(caps);

  }

  // Если rgb буффер собран
  if (frameparser->frame_size <= in_size-METADATA_SIZE){
    
    // Формируем rbg буффер
    GstBuffer * processed_data = gst_buffer_new_allocate(NULL, frameparser->frame_size, NULL);
    gst_buffer_fill (processed_data, 0, map.data+METADATA_SIZE, frameparser->frame_size);
    gst_buffer_unmap (frame->buffer, &map);

    // Устанавливаем мета данные на буффер
    gst_buffer_add_video_meta (processed_data,
        frameparser->flags,
        frameparser->format,
        frameparser->width,
        frameparser->height);

    // Отпарвляем буффер дальше по пиплайну
    frame->out_buffer = processed_data;
    GstFlowReturn res = gst_base_parse_finish_frame (parse, frame, frameparser->frame_size+METADATA_SIZE);
    frameparser->frame_size = 0;

    return res;
  }

  gst_buffer_unmap (frame->buffer, &map);

  return GST_FLOW_OK;

}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "frameparser", GST_RANK_NONE,
      GST_TYPE_FRAMEPARSER);
}

#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    frameparser,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)


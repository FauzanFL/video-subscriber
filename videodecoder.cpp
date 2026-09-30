#include "videodecoder.h"
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <QMetaObject>
#include <iostream>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject{parent},
    m_sink(nullptr),
    pipeline_(nullptr),
    appsrc_(nullptr)
{
    GError* error = nullptr;

    // Pipeline GStreamer: AppSrc (H.265) -> Parse -> Decode -> Format BGRA -> AppSink
    pipeline_ = gst_parse_launch(
        "appsrc name=mysrc format=time is-live=true "
        "! h265parse ! avdec_h265 ! videoconvert "
        "! video/x-raw,format=BGRA,width=640,height=480 "
        "! appsink name=mysink emit-signals=true sync=false", &error);

    if (error) {
        std::cerr << "[VideoDecoder] Pipeline GStreamer Error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysrc");
    GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");

    if (appsink) {
        g_signal_connect_data(appsink, "new-sample", G_CALLBACK(on_decoded_sample), this, nullptr, static_cast<GConnectFlags>(0));
        gst_object_unref(appsink);
    }

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

VideoDecoder::~VideoDecoder() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    if (appsrc_) {
        gst_object_unref(appsrc_);
        appsrc_ = nullptr;
    }
}

QVideoSink* VideoDecoder::videoSink() const {
    return m_sink;
}

void VideoDecoder::setVideoSink(QVideoSink* sink) {
    if (m_sink != sink) {
        m_sink = sink;
        emit videoSinkChanged();
    }
}

void VideoDecoder::pushH265Data(const std::vector<uint8_t>& data) {
    if (!appsrc_ || data.empty()) {
        return;
    }

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, data.size(), nullptr);
    if (buffer) {
        gst_buffer_fill(buffer, 0, data.data(), data.size());
        gst_app_src_push_buffer(GST_APP_SRC(appsrc_), buffer);
    }
}

GstFlowReturn VideoDecoder::on_decoded_sample(GstElement* sink, gpointer user_data) {
    auto* self = static_cast<VideoDecoder*>(user_data);
    if (!self) return GST_FLOW_ERROR;

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) return GST_FLOW_ERROR;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // Konversi buffer decoded ke Format BGRA8888 QtMultimedia
        QVideoFrameFormat format(QSize(640, 480), QVideoFrameFormat::Format_BGRA8888);
        QVideoFrame qframe(format);

        if (qframe.map(QVideoFrame::WriteOnly)) {
            memcpy(qframe.bits(0), map.data, map.size);
            qframe.unmap();

            // Pindahkan pendelegasian setVideoFrame ke GUI Thread milik Qt
            QMetaObject::invokeMethod(self, [self, qframe]() {
                if (self->m_sink) {
                    self->m_sink->setVideoFrame(qframe);
                }
            }, Qt::QueuedConnection);
        }
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}
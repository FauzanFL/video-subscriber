#include "videodecoder.h"
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <QMetaObject>
#include <iostream>
#include <QDebug>

VideoDecoder::VideoDecoder(StreamMode mode, QObject *parent)
    : QObject{parent},
    m_sink(nullptr),
    m_mode(mode),
    m_hasVideo(false),
    pipeline_(nullptr),
    appsrc_(nullptr)
{
    // Timer timeout: Jika 2 detik tidak ada frame baru, atur status video terputus
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(3000);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_hasVideo) {
            qDebug() << "[VideoDecoder] Koneksi terputus! Membersihkan pipeline...";
            setHasVideo(false);
        }
        // Reset statistik layar
        if (m_fps != 0) { m_fps = 0; emit fpsChanged(); }
        if (m_bandwidth != 0.0) { m_bandwidth = 0.0; emit bandwidthChanged(); }

        // Hancurkan pipeline agar GStreamer tidak stuck menunggu data
        stopPipeline();
    });

    // 2. Timer Statistik (FPS & Bandwidth)
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000);
    connect(m_statsTimer, &QTimer::timeout, this, [this]() {
        if (!m_hasVideo) return;

        m_fps = m_frameCount.exchange(0);
        size_t currentBytes = m_byteCount.exchange(0);
        m_bandwidth = (currentBytes * 8.0) / 1000000.0;

        emit fpsChanged();
        emit bandwidthChanged();
    });
    m_statsTimer->start();

    if (m_mode == StreamMode::Compressed_H264) {
        qDebug() << "[VideoDecoder] Mode: Terkompresi H.264 (Menggunakan GStreamer)";

        startPipeline();

    } else {
        qDebug() << "[VideoDecoder] Mode: Raw Frame BGRA (Bypass GStreamer)";
    }
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

void VideoDecoder::setHasVideo(bool hasVideo) {
    if (m_hasVideo != hasVideo) {
        m_hasVideo = hasVideo;
        emit hasVideoChanged();
    }
}

void VideoDecoder::pushData(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    // Catat bandwidth dari ukuran asli payload DDS
    m_byteCount += data.size();

    // JIKA MODE RAW: Render langsung ke Qt tanpa GStreamer
    if (m_mode == StreamMode::Raw_BGRA) {
        m_frameCount++; // Hitung FPS
        QVideoFrameFormat format(QSize(640, 480), QVideoFrameFormat::Format_BGRA8888);
        QVideoFrame qframe(format);

        if (qframe.map(QVideoFrame::WriteOnly)) {
            // Ukuran byte data mentah 640x480x4 (BGRA) = 1.228.800 bytes
            size_t copySize = std::min(data.size(), static_cast<size_t>(qframe.mappedBytes(0)));
            memcpy(qframe.bits(0), data.data(), copySize);
            qframe.unmap();

            QMetaObject::invokeMethod(this, [this, qframe]() {
                if (m_sink) {
                    m_sink->setVideoFrame(qframe);
                }
                setHasVideo(true);
                m_timeoutTimer->start();
            }, Qt::QueuedConnection);
        }
        return; // Selesai, jangan masuk ke logika GStreamer
    }

    // JIKA MODE H.264
    if (m_mode == StreamMode::Compressed_H264) {
        // Auto-Reconnect: Jika pipeline hancur akibat timeout, bangun ulang!
        if (!pipeline_) {
            startPipeline();
        }

        if (appsrc_) {
            GstBuffer* buffer = gst_buffer_new_allocate(nullptr, data.size(), nullptr);
            if (buffer) {
                gst_buffer_fill(buffer, 0, data.data(), data.size());
                gst_app_src_push_buffer(GST_APP_SRC(appsrc_), buffer);
            }
        }
    }
}

GstFlowReturn VideoDecoder::on_decoded_sample(GstElement* sink, gpointer user_data) {
    auto* self = static_cast<VideoDecoder*>(user_data);
    if (!self) return GST_FLOW_ERROR;

    // Hitung FPS dari frame yang sukses di-decode
    self->m_frameCount++;

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
                // Tandai video sedang aktif & reset timer timeout
                self->setHasVideo(true);
                self->m_timeoutTimer->start();
            }, Qt::QueuedConnection);
        }
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void VideoDecoder::startPipeline() {
    std::lock_guard<std::mutex> lock(m_pipelineMutex);
    if (pipeline_) return; // Cegah pembuatan ganda

    qDebug() << "[VideoDecoder] Membangun pipeline GStreamer...";
    GError* error = nullptr;

    pipeline_ = gst_parse_launch(
        "appsrc name=mysrc format=time is-live=true "
        "! h264parse ! avdec_h264 ! videoconvert ! videoscale "
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

void VideoDecoder::stopPipeline() {
    std::lock_guard<std::mutex> lock(m_pipelineMutex);
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
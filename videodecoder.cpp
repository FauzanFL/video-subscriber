#include "videodecoder.h"
#include <gst/app/gstappsink.h>
#include <QMetaObject>
#include <iostream>
#include <QDebug>

VideoDecoder::VideoDecoder(const QString& rtspUrl, QObject *parent)
    : QObject{parent},
    m_sink(nullptr),
    m_hasVideo(false),
    pipeline_(nullptr),
    m_rtspUrl(rtspUrl)
{
    qDebug() << "[VideoDecoder] Mode: RTSP Direct Streaming (" << rtspUrl << ")";

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(3000);

    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_hasVideo) {
            qDebug() << "[VideoDecoder] Koneksi RTSP terputus! Mencoba menghubungkan ulang...";
            setHasVideo(false);
            m_currentLatency.store(0);
        } else {
            qDebug() << "[VideoDecoder] Mencari stream RTSP...";
        }

        // 1. Hancurkan pipeline lama sepenuhnya untuk membuang cache & error state
        stopPipeline();

        // 2. Buat pipeline baru dari nol setelah port jaringan dilepas
        QTimer::singleShot(1000, this, [this]() {
            startPipeline();
            m_timeoutTimer->start();
        });
    });

    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000);
    connect(m_statsTimer, &QTimer::timeout, this, [this]() {
        // Simpan hitungan ke variabel yang dibaca QML
        m_currentFps = m_frameCount.exchange(0); // Ambil nilai lalu reset ke 0

        size_t currentBytes = m_byteCount.exchange(0);
        // Kalkulasi ke Mbps (Megabits per second)
        m_currentBandwidth = (currentBytes * 8.0) / 1000000.0;

        // Beritahu QML bahwa data statistik diperbarui
        emit statsChanged();
    });

    startPipeline();
    m_timeoutTimer->start();
    m_statsTimer->start();
}

VideoDecoder::~VideoDecoder() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
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

GstFlowReturn VideoDecoder::on_decoded_sample(GstElement* sink, gpointer user_data) {
    auto* self = static_cast<VideoDecoder*>(user_data);
    if (!self) return GST_FLOW_ERROR;

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) return GST_FLOW_ERROR;

    // Increment FPS Counter (karena 1 sample = 1 frame yang berhasil ditarik)
    self->m_frameCount++;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstClockTime pts = GST_BUFFER_PTS(buffer);
    GstObject* parent_obj = gst_element_get_parent(sink);

    if (parent_obj) {
        GstElement* pipeline = GST_ELEMENT(parent_obj);
        GstClock* clock = gst_element_get_clock(pipeline);

        if (clock) {
            if (GST_CLOCK_TIME_IS_VALID(pts)) {
                GstClockTime current_time = gst_clock_get_time(clock);
                GstClockTime base_time = gst_element_get_base_time(pipeline);

                if (current_time >= base_time) {
                    GstClockTime running_time = current_time - base_time;

                    if (running_time >= pts) {
                        // Kalkulasi selisih dalam milidetik
                        guint64 latency_ms = (running_time - pts) / GST_MSECOND;
                        self->m_currentLatency.store(static_cast<int>(latency_ms));
                    }
                }
            }
            gst_object_unref(clock);
        }
        gst_object_unref(parent_obj);
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        self->m_byteCount += map.size;

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
    if (pipeline_) return; // Cegah duplikasi jika pipeline sudah ada

    GError* error = nullptr;
    // Pipeline GStreamer: RTSP (TCP) -> Depay -> Parse -> Decode -> Format BGRA -> AppSink
    QString pipelineStr = QString(
                              "rtspsrc location=%1 protocols=tcp latency=0 drop-on-latency=true " // tambahkan protocols=tcp jika ingin menggunakan tcp
                              "! queue ! rtph264depay ! queue ! h264parse ! avdec_h264 ! videoconvert ! videoscale "
                              "! video/x-raw,format=BGRA,width=640,height=480 "
                              "! appsink name=mysink emit-signals=true sync=false"
                              ).arg(m_rtspUrl);

    pipeline_ = gst_parse_launch(pipelineStr.toStdString().c_str(), &error);

    if (error) {
        std::cerr << "[VideoDecoder] Pipeline GStreamer Error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (appsink) {
        g_signal_connect_data(appsink, "new-sample", G_CALLBACK(on_decoded_sample), this, nullptr, static_cast<GConnectFlags>(0));
        gst_object_unref(appsink);
    }

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    qDebug() << "[VideoDecoder] Pipeline berhasil dibuat dan dijalankan.";
}

void VideoDecoder::stopPipeline() {
    if (pipeline_) {
        // Hentikan state
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        // Hapus referensi dari memori (Ini kunci utama agar socket jaringan direset!)
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        qDebug() << "[VideoDecoder] Pipeline telah dihancurkan secara aman.";
    }
}
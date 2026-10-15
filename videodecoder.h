#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <gst/gst.h>
#include <vector>
#include <QTimer>
#include <atomic>
#include <mutex>

class VideoDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(double bandwidth READ bandwidth NOTIFY bandwidthChanged)
public:
    // Pilihan mode output GStreamer
    enum class StreamMode {
        Compressed_H264, // Ekstrak byte-stream H.264 (Bandwidth kecil)
        Raw_BGRA         // Decode menjadi frame mentah BGRA (Bandwidth besar)
    };

    explicit VideoDecoder(StreamMode mode, QObject *parent = nullptr);
    ~VideoDecoder() override;

    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink* sink);

    bool hasVideo() const { return m_hasVideo; }
    int fps() const { return m_fps; }
    double bandwidth() const { return m_bandwidth; }

public slots:
    // Slot Qt yang dipanggil saat sinyal DdsSubscriber::frameReceived dipancarkan
    void pushData(const std::vector<uint8_t>& data);

signals:
    void videoSinkChanged();
    void hasVideoChanged();
    void fpsChanged();
    void bandwidthChanged();

private:
    void setHasVideo(bool hasVideo);
    void startPipeline();
    void stopPipeline();

    QVideoSink* m_sink = nullptr;
    StreamMode m_mode;

    // GStreamer components (hanya digunakan untuk mode H.264)
    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;

    bool m_hasVideo = false;
    QTimer* m_timeoutTimer;
    QTimer* m_statsTimer;

    // Variabel statistik (Menggunakan atomic agar aman beda thread)
    std::atomic<int> m_frameCount{0};
    std::atomic<size_t> m_byteCount{0};
    std::mutex m_pipelineMutex;

    int m_fps = 0;
    double m_bandwidth = 0.0;

    static GstFlowReturn on_decoded_sample(GstElement* sink, gpointer user_data);
};

#endif // VIDEODECODER_H

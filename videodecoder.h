#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <gst/gst.h>
#include <vector>
#include <QTimer>

class VideoDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int fps READ fps NOTIFY statsChanged)
    Q_PROPERTY(double bandwidth READ bandwidth NOTIFY statsChanged)
    Q_PROPERTY(int latency READ latency NOTIFY statsChanged)
public:

    explicit VideoDecoder(const QString& rtspUrl, QObject *parent = nullptr);
    ~VideoDecoder() override;

    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink* sink);

    bool hasVideo() const { return m_hasVideo; }
    int fps() const { return m_currentFps; }
    double bandwidth() const { return m_currentBandwidth; }
    int latency() const { return m_currentLatency.load(); }

signals:
    void videoSinkChanged();
    void hasVideoChanged();
    void statsChanged();

private:
    void setHasVideo(bool hasVideo);
    void startPipeline();
    void stopPipeline();

    QString m_rtspUrl;

    QVideoSink* m_sink = nullptr;

    GstElement* pipeline_ = nullptr;

    bool m_hasVideo = false;
    QTimer* m_timeoutTimer;
    QTimer* m_statsTimer;

    // Variabel statistik (Menggunakan atomic agar aman beda thread)
    std::atomic<int> m_frameCount{0};
    std::atomic<size_t> m_byteCount{0};
    std::atomic<int> m_currentLatency{0};

    int m_currentFps = 0;
    double m_currentBandwidth = 0.0;

    static GstFlowReturn on_decoded_sample(GstElement* sink, gpointer user_data);
};

#endif // VIDEODECODER_H

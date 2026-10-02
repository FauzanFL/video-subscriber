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
public:
    // Pilihan mode output GStreamer
    enum class StreamMode {
        Compressed_H265, // Ekstrak byte-stream H.265 (Bandwidth kecil)
        Raw_BGRA         // Decode menjadi frame mentah BGRA (Bandwidth besar)
    };

    explicit VideoDecoder(StreamMode mode, QObject *parent = nullptr);
    ~VideoDecoder() override;

    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink* sink);

    bool hasVideo() const { return m_hasVideo; }

public slots:
    // Slot Qt yang dipanggil saat sinyal DdsSubscriber::frameReceived dipancarkan
    void pushData(const std::vector<uint8_t>& data);

signals:
    void videoSinkChanged();
    void hasVideoChanged();

private:
    void setHasVideo(bool hasVideo);

    QVideoSink* m_sink = nullptr;
    StreamMode m_mode;

    // GStreamer components (hanya digunakan untuk mode H.265)
    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;

    bool m_hasVideo = false;
    QTimer* m_timeoutTimer = nullptr;

    static GstFlowReturn on_decoded_sample(GstElement* sink, gpointer user_data);
};

#endif // VIDEODECODER_H

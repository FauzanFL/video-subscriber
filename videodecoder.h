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
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink* sink);

    bool hasVideo() const { return m_hasVideo; }

public slots:
    // Slot Qt yang dipanggil saat sinyal DdsSubscriber::frameReceived dipancarkan
    void pushH265Data(const std::vector<uint8_t>& data);

signals:
    void videoSinkChanged();
    void hasVideoChanged();

private:
    void setHasVideo(bool hasVideo);

    QVideoSink* m_sink = nullptr;
    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;

    bool m_hasVideo = false;
    QTimer* m_timeoutTimer = nullptr;

    static GstFlowReturn on_decoded_sample(GstElement* sink, gpointer user_data);
};

#endif // VIDEODECODER_H

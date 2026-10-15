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

    explicit VideoDecoder(const QString& rtspUrl, QObject *parent = nullptr);
    ~VideoDecoder() override;

    QVideoSink* videoSink() const;
    void setVideoSink(QVideoSink* sink);

    bool hasVideo() const { return m_hasVideo; }

signals:
    void videoSinkChanged();
    void hasVideoChanged();

private:
    void setHasVideo(bool hasVideo);
    void startPipeline();
    void stopPipeline();

    QString m_rtspUrl;

    QVideoSink* m_sink = nullptr;

    GstElement* pipeline_ = nullptr;

    bool m_hasVideo = false;
    QTimer* m_timeoutTimer = nullptr;

    static GstFlowReturn on_decoded_sample(GstElement* sink, gpointer user_data);
};

#endif // VIDEODECODER_H

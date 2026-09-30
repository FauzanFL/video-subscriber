#ifndef DDSSUBSCRIBER_H
#define DDSSUBSCRIBER_H

#include <QObject>
#include <vector>
#include <dds/dds.hpp>
#include "VideoMessage.hpp"

class DdsSubscriber;

// Listener khusus untuk menangkap event data masuk
class VideoFrameListener : public dds::sub::NoOpDataReaderListener<VideoMsg::Frame> {
public:
    explicit VideoFrameListener(DdsSubscriber* parent) : parent_(parent) {}
    void on_data_available(dds::sub::DataReader<VideoMsg::Frame>& reader) override;

private:
    DdsSubscriber* parent_;
};

class DdsSubscriber : public QObject {
    Q_OBJECT
public:
    // Konstruktor menerima DomainParticipant
    explicit DdsSubscriber(dds::domain::DomainParticipant participant, QObject* parent = nullptr);
    ~DdsSubscriber() override;

    void startListening();
    void stopListening();
    void handleFrameReceived(const VideoMsg::Frame& frame);

signals:
    void frameReceived(const std::vector<uint8_t>& data);

private:
    dds::sub::Subscriber subscriber_;
    VideoFrameListener frame_listener_;
    dds::topic::Topic<VideoMsg::Frame> frame_topic_;
    dds::sub::DataReader<VideoMsg::Frame> frame_reader_;
};

#endif // DDSSUBSCRIBER_H
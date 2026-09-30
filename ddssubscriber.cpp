#include "ddssubscriber.h"
#include <iostream>
#include "qos_profiles.h"

// Implementation Callback Listener DDS
void VideoFrameListener::on_data_available(dds::sub::DataReader<VideoMsg::Frame>& reader) {
    auto samples = reader.take();
    for (const auto& sample : samples) {
        if (sample.info().valid()) {
            parent_->handleFrameReceived(sample.data());
        }
    }
}

DdsSubscriber::DdsSubscriber(dds::domain::DomainParticipant participant, QObject* parent)
    : QObject(parent),
    subscriber_(participant),
    frame_listener_(this),
    frame_topic_(participant, "VideoTopic"),
    frame_reader_(subscriber_, frame_topic_, QoSProfile::VideoSub(subscriber_), &frame_listener_, dds::core::status::StatusMask::data_available())
{}

DdsSubscriber::~DdsSubscriber() {
    stopListening();
}

void DdsSubscriber::startListening() {
    // Mendaftarkan listener untuk memantau event DATA_AVAILABLE
    frame_reader_.listener(&frame_listener_, dds::core::status::StatusMask::data_available());
}

void DdsSubscriber::stopListening() {
    // Melepas listener
    frame_reader_.listener(nullptr, dds::core::status::StatusMask::none());
}

void DdsSubscriber::handleFrameReceived(const VideoMsg::Frame& frame) {
    // Memancarkan sinyal Qt membawa data vektor byte H.265
    emit frameReceived(frame.data());
}
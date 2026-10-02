#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <gst/gst.h>
#include <memory>
#include <dds/dds.hpp>

#include "videodecoder.h"
#include "ddssubscriber.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Inisialisasi GStreamer
    gst_init(&argc, &argv);

    VideoDecoder* decoder = new VideoDecoder(VideoDecoder::StreamMode::Compressed_H265, &app);

    qmlRegisterSingletonInstance("StreamSubscriber", 1, 0, "BackendDecoder", decoder);

    // Inisialisasi DDS Participant (Domain 99)
    dds::domain::DomainParticipant participant(99);

    // Buat Subscriber dan jadikan QApplication sebagai parent agar memori dikelola Qt
    auto subscriber = std::make_unique<DdsSubscriber>(participant, &app);

    // Hubungkan Sinyal Qt dari DdsSubscriber ke Slot VideoDecoder
    QObject::connect(subscriber.get(), &DdsSubscriber::frameReceived,
                     decoder, &VideoDecoder::pushData);

    // Mulai mendengarkan stream data DDS
    subscriber->startListening();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("StreamSubscriber", "Main");

    return QGuiApplication::exec();
}

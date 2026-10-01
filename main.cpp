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

    // Registrasi komponen C++ agar dapat diakses oleh QML
    qmlRegisterType<VideoDecoder>("StreamSubscriber", 1, 0, "VideoDecoder");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("StreamSubscriber", "Main");

    // Mencari objek VideoDecoder yang dibuat oleh QML (berdasarkan objectName)
    QObject* rootObject = engine.rootObjects().first();
    auto* decoder = rootObject->findChild<VideoDecoder*>("videoDecoder");

    // Inisialisasi DDS Participant (Domain 99)
    dds::domain::DomainParticipant participant(99);

    // Buat Subscriber dan jadikan QApplication sebagai parent agar memori dikelola Qt
    auto subscriber = std::make_unique<DdsSubscriber>(participant, &app);

    if (decoder) {
        // Hubungkan Sinyal Qt dari DdsSubscriber ke Slot VideoDecoder
        QObject::connect(subscriber.get(), &DdsSubscriber::frameReceived,
                         decoder, &VideoDecoder::pushH265Data);

        // Mulai mendengarkan stream data DDS
        subscriber->startListening();
    }

    return QGuiApplication::exec();
}

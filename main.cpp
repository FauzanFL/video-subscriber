#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <gst/gst.h>
#include <memory>
#include <dds/dds.hpp>

#include "videodecoder.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Inisialisasi GStreamer
    gst_init(&argc, &argv);

    VideoDecoder* decoder = new VideoDecoder("rtsp://172.20.3.20:8554/stream", &app);

    qmlRegisterSingletonInstance("StreamSubscriber", 1, 0, "BackendDecoder", decoder);

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

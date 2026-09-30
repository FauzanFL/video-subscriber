import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia
import StreamSubscriber

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: "DDS Video Subscriber"

    VideoDecoder {
        id: decoder
        objectName: "videoDecoder"
    }

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            visible: decoder.hasVideo
        }

        Column {
            anchors.centerIn: parent
            spacing: 15
            visible: !decoder.hasVideo

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: !decoder.hasVideo
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Menunggu Video Stream dari DDS..."
                color: "#ffffff"
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Pastikan Publisher sedang mengirimkan data"
                color: "#888888"
                font.pixelSize: 12
            }
        }
    }


    Component.onCompleted: {
        decoder.videoSink = videoOutput.videoSink
    }
}

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

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            visible: BackendDecoder.hasVideo
        }

        Column {
            anchors.centerIn: parent
            spacing: 15
            visible: !BackendDecoder.hasVideo

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: !BackendDecoder.hasVideo
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

        // Overlay Statistik Jaringan & Performa (Muncul saat video jalan)
        Rectangle {
            id: statsOverlay
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 15
            width: 170
            height: 60
            color: "#99000000" // Latar belakang hitam transparan agar tulisan jelas
            radius: 8
            visible: BackendDecoder.hasVideo // Hanya tampil jika video ada

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    Layout.fillWidth: true
                    text: "FPS: " + BackendDecoder.fps
                    color: "#00ff00" // Hijau ala telemetri
                    font.pixelSize: 14
                    font.bold: true
                    font.family: "Monospace"
                }

                Text {
                    Layout.fillWidth: true
                    text: "BW : " + BackendDecoder.bandwidth.toFixed(2) + " Mbps"
                    color: "#00aa88"
                    font.pixelSize: 14
                    font.bold: true
                    font.family: "Monospace"
                }
            }
        }
    }


    Component.onCompleted: {
        BackendDecoder.videoSink = videoOutput.videoSink
    }
}

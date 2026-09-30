import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: "DDS Video Subscriber"

    VideoOutput {
        anchors.fill: parent
    }

}

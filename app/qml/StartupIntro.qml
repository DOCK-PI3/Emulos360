import QtQuick
import QtQuick.Controls
import QtMultimedia

Item {
    id: intro
    required property url videoSource
    required property url posterSource
    required property bool loading
    property bool elapsed: false
    property bool videoFailed: false
    signal finished()

    function finishIfReady(): void {
        if (elapsed && !loading) finished()
    }

    onLoadingChanged: finishIfReady()
    Component.onCompleted: player.play()

    Rectangle { anchors.fill: parent; color: "#101211" }
    Image {
        anchors.fill: parent
        source: intro.posterSource
        sourceSize: Qt.size(1280, 720)
        fillMode: Image.PreserveAspectCrop
        visible: intro.videoFailed
        Accessible.ignored: true
    }
    VideoOutput {
        id: picture
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: !intro.videoFailed
    }
    MediaPlayer {
        id: player
        source: intro.videoSource
        videoOutput: picture
        audioOutput: AudioOutput { volume: 0.48 }
        onErrorOccurred: intro.videoFailed = true
    }
    Timer {
        interval: 6000
        running: true
        onTriggered: {
            intro.elapsed = true
            intro.finishIfReady()
        }
    }
    Rectangle {
        anchors.fill: parent
        visible: intro.elapsed && intro.loading
        color: "#101211"
        Column {
            anchors.centerIn: parent
            spacing: 20
            BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; running: intro.elapsed && intro.loading }
            Label { text: qsTr("Cargando biblioteca…"); color: "#f3f5ef"; font.pixelSize: 20 }
        }
    }
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }
}

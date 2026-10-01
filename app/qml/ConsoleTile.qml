import QtQuick
import QtQuick.Controls.Basic
import Los
Button {
    id: tile
    property string subtitle: ""
    property string symbol: ""
    property url artwork: ""
    property bool featured: false
    property bool reducedMotion: false
    implicitHeight: 160
    padding: 20
    hoverEnabled: true
    Accessible.name: text
    scale: activeFocus || hovered ? 1.015 : 1
    Behavior on scale { enabled: !tile.reducedMotion; NumberAnimation { duration: 120 } }
    background: Rectangle {
        color: tile.featured ? Theme.consoleGreen : Theme.consoleTile
        border.width: tile.activeFocus ? 4 : tile.hovered ? 2 : 0
        border.color: Theme.consoleFocus
        Image { anchors.fill: parent; source: tile.artwork; visible: source.toString().length > 0; fillMode: Image.PreserveAspectCrop; asynchronous: true; sourceSize: Qt.size(700, 500) }
        Rectangle { anchors.fill: parent; visible: tile.artwork.toString().length > 0; color: "#a018281b" }
    }
    contentItem: Item {
        Label { anchors.right: parent.right; anchors.top: parent.top; text: tile.symbol; color: tile.featured ? "#b7e0ac" : Theme.consoleGreen; font.pointSize: 34; font.weight: Font.Light }
        Column {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; spacing: 7
            Label { width: parent.width; text: tile.text; color: tile.featured ? "white" : Theme.consoleText; font.pointSize: tile.featured ? 25 : 18; font.weight: Font.Light; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
            Label { width: parent.width; text: tile.subtitle; color: tile.featured ? "#e0efdb" : Theme.consoleMuted; font.pointSize: 12; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
        }
    }
}

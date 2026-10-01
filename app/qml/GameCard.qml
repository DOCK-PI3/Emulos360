import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ItemDelegate {
    id: card
    objectName: "gameCard"
    required property var game
    property url artwork: ""
    property bool selected: false
    property GameCompatibility compatibility: null
    signal removeRequested()
    padding: 5
    Accessible.name: game.title
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: card.removeRequested() }
    background: Rectangle {
        radius: 11
        color: card.hovered || card.selected ? Theme.raised : "transparent"
        border.width: card.selected || card.visualFocus ? 2 : 0
        border.color: Theme.accent
    }
    contentItem: ColumnLayout {
        spacing: 10
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.panel
            radius: 7
            clip: true
            Rectangle {
                anchors.fill: parent
                visible: art.status !== Image.Ready
                gradient: Gradient { GradientStop { position: 0; color: "#354a3a" } GradientStop { position: 1; color: "#19201c" } }
                Label { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 14; text: "360"; color: "#a7c6ae"; font.pixelSize: 16; font.letterSpacing: 4 }
                Label { anchors.centerIn: parent; text: card.game.title.slice(0, 1).toUpperCase(); color: "#73927b"; font.pixelSize: 106; font.weight: Font.Light }
                Label { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 17; text: qsTr("ELIGE TU CARÁTULA"); color: Theme.muted; font.pixelSize: 9; font.letterSpacing: 1.3 }
            }
            Image { id: art; anchors.fill: parent; source: card.artwork; asynchronous: true; sourceSize: Qt.size(500, 650); fillMode: Image.PreserveAspectFit }
        }
        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 37
            text: card.game.title
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: Theme.text
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Label {
                Layout.fillWidth: true
                text: card.game.format === "XEX" ? qsTr("Xbox 360 · XEX") : qsTr("%1 GiB  ·  %2%3").arg(Number(card.game.sizeGiB).toFixed(1)).arg(card.game.format || qsTr("Xbox 360")).arg(card.game.complete ? "" : qsTr(" · Incompleto"))
                color: Theme.muted
                font.pixelSize: 11
                elide: Text.ElideRight
            }
            CompatibilityBadge { database: card.compatibility; titleId: card.game.titleId; titleName: card.game.title; showLabel: false }
        }
    }
}

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

Item {
    id: badge
    objectName: "compatibilityBadge"
    property GameCompatibility database: null
    property string titleId: ""
    property string titleName: ""
    property bool showLabel: true
    property var report: database && database.revision > 0 ? database.lookup(titleId, titleName) : ({})
    readonly property string statusCode: report.state || "unknown"
    readonly property color statusColor: report.color || "#a0aaa2"
    readonly property string statusLabel: report.label || qsTr("Sin datos")
    implicitWidth: contents.implicitWidth + (showLabel ? 14 : 0)
    implicitHeight: 26
    Accessible.role: Accessible.StaticText
    Accessible.name: statusLabel
    Accessible.description: report.tooltip || qsTr("Sin informe de compatibilidad para esta edición.")
    Rectangle {
        anchors.fill: parent
        visible: badge.showLabel
        radius: 6
        color: Qt.rgba(badge.statusColor.r, badge.statusColor.g, badge.statusColor.b, 0.08)
        border.color: Qt.rgba(badge.statusColor.r, badge.statusColor.g, badge.statusColor.b, 0.25)
    }
    RowLayout {
        id: contents
        anchors.centerIn: parent
        spacing: 6
        Image {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            source: "qrc:/assets/compatibility/" + badge.statusCode + ".svg"
            sourceSize: Qt.size(36, 36)
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }
        Label {
            visible: badge.showLabel
            text: badge.statusLabel
            color: badge.statusColor
            font.pixelSize: 12
            textFormat: Text.PlainText
            Accessible.ignored: true
        }
    }
    HoverHandler { id: hover }
    ToolTip {
        id: tip
        visible: hover.hovered
        delay: 400
        timeout: 15000
        text: badge.Accessible.description
        contentItem: Label {
            text: tip.text
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: Theme.text
        }
        width: 440
        background: Rectangle { color: Theme.raised; radius: 8; border.color: Theme.outline }
    }
}

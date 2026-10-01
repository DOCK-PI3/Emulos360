import QtQuick
import QtQuick.Controls.Basic
Button {
    id: control
    property bool primary: false
    property bool active: false
    implicitHeight: 42
    leftPadding: 17
    rightPadding: 17
    opacity: enabled ? 1 : 0.45
    background: Rectangle {
        radius: 9
        color: control.primary ? Theme.accent : control.active || control.hovered ? Theme.raised : "transparent"
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : control.primary ? Theme.accent : Theme.outline
    }
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.primary ? Theme.accentText : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}

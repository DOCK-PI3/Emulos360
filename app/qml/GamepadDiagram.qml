pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import Los

Item {
    id: diagram
    required property GamepadMapper mapper
    property string family: mapper.family
    readonly property bool playstation: family.startsWith("ps")
    readonly property bool dualSense: family === "ps5"
    readonly property var input: mapper.values
    signal controlSelected(string key)
    implicitWidth: 600
    implicitHeight: 385
    function value(key: string): real { return Number(input[key] || 0) }
    readonly property real leftX: value("+leftx") - value("-leftx")
    readonly property real leftY: value("+lefty") - value("-lefty")
    readonly property real rightX: value("+rightx") - value("-rightx")
    readonly property real rightY: value("+righty") - value("-righty")
    readonly property real stickLeftX: playstation ? 220 : 164
    readonly property real stickLeftY: playstation ? 244 : 174
    readonly property real crossX: playstation ? 160 : 237
    readonly property real crossY: playstation ? 174 : 244
    readonly property var marks: [
        {key:"lefttrigger", label:playstation ? "L2" : "LT", x:145, y:44, w:92, h:40},
        {key:"righttrigger", label:playstation ? "R2" : "RT", x:455, y:44, w:92, h:40},
        {key:"leftshoulder", label:playstation ? "L1" : "LB", x:145, y:92, w:104, h:34},
        {key:"rightshoulder", label:playstation ? "R1" : "RB", x:455, y:92, w:104, h:34},
        {key:"y", label:playstation ? "△" : "Y", x:448, y:133, color:Theme.controllerY},
        {key:"x", label:playstation ? "□" : "X", x:405, y:176, color:Theme.controllerX},
        {key:"b", label:playstation ? "○" : "B", x:491, y:176, color:Theme.controllerB},
        {key:"a", label:playstation ? "×" : "A", x:448, y:219, color:Theme.controllerA},
        {key:"back", label:playstation ? (family === "ps3" ? "SELECT" : "SHARE") : "BACK", x:255, y:151, w:52, h:30},
        {key:"start", label:playstation ? (family === "ps3" ? "START" : "OPTIONS") : "START", x:345, y:151, w:60, h:30},
        {key:"guide", label:playstation ? "PS" : "X", x:300, y:playstation ? 201 : family === "xboxone" ? 112 : 151, w:42, h:42},
        {key:"dpup", label:"↑", x:crossX, y:crossY-28, w:34, h:34},
        {key:"dpdown", label:"↓", x:crossX, y:crossY+28, w:34, h:34},
        {key:"dpleft", label:"←", x:crossX-28, y:crossY, w:34, h:34},
        {key:"dpright", label:"→", x:crossX+28, y:crossY, w:34, h:34}
    ]
    Item {
        id: drawing
        width: 600; height: 385
        anchors.centerIn: parent
        scale: Math.min(diagram.width / width, diagram.height / height)
        Shape {
            anchors.fill: parent
            Accessible.ignored: true
            ShapePath {
                strokeColor: Theme.outline; strokeWidth: 3
                fillColor: diagram.dualSense ? Theme.controllerLight : Theme.controllerShell
                startX: 152; startY: 92
                PathCubic { x: 448; y: 92; control1X: 240; control1Y: 119; control2X: 360; control2Y: 119 }
                PathCubic { x: 527; y: 169; control1X: 488; control1Y: 88; control2X: 516; control2Y: 112 }
                PathCubic { x: 535; y: 346; control1X: 549; control1Y: 237; control2X: 567; control2Y: 324 }
                PathCubic { x: 441; y: 322; control1X: 508; control1Y: 373; control2X: 468; control2Y: 363 }
                PathCubic { x: 300; y: 279; control1X: 410; control1Y: 281; control2X: 367; control2Y: 281 }
                PathCubic { x: 159; y: 322; control1X: 233; control1Y: 281; control2X: 190; control2Y: 281 }
                PathCubic { x: 65; y: 346; control1X: 132; control1Y: 363; control2X: 92; control2Y: 373 }
                PathCubic { x: 73; y: 169; control1X: 33; control1Y: 324; control2X: 51; control2Y: 237 }
                PathCubic { x: 152; y: 92; control1X: 84; control1Y: 112; control2X: 112; control2Y: 88 }
            }
        }
        Rectangle {
            visible: diagram.playstation && diagram.family !== "ps3"
            x: 260; y: 103; width: 80; height: 76; radius: 10
            color: Theme.controllerInset; border.color: Theme.outline
            Accessible.ignored: true
        }
        Repeater {
            model: diagram.marks
            delegate: Button {
                id: mark
                required property var modelData
                readonly property bool pressedNow: diagram.value(modelData.key) > 0.15
                readonly property bool waiting: diagram.mapper.target === modelData.key
                x: modelData.x - width / 2; y: modelData.y - height / 2
                width: modelData.w || 38; height: modelData.h || 38
                hoverEnabled: true
                Accessible.name: modelData.label
                Accessible.description: pressedNow ? qsTr("Pulsado") : qsTr("Pulsa para asignar este control")
                onClicked: diagram.controlSelected(modelData.key)
                background: Rectangle {
                    radius: Math.min(mark.width, mark.height) / 2
                    color: mark.pressedNow ? Theme.accent : Theme.controllerInset
                    border.width: mark.waiting || mark.visualFocus || mark.pressedNow ? 3 : 1
                    border.color: mark.waiting || mark.visualFocus ? Theme.accent : mark.modelData.color || Theme.outline
                }
                contentItem: Text {
                    text: mark.modelData.label
                    color: mark.pressedNow ? Theme.accentText : mark.modelData.color || Theme.text
                    font.pixelSize: mark.width > 50 ? 11 : 21; font.bold: true
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
            }
        }
        Repeater {
            model: ["left", "right"]
            delegate: Item {
                id: stick
                required property string modelData
                readonly property bool leftStick: modelData === "left"
                readonly property real moveX: leftStick ? diagram.leftX : diagram.rightX
                readonly property real moveY: leftStick ? diagram.leftY : diagram.rightY
                readonly property real centerX: leftStick ? diagram.stickLeftX : (diagram.playstation ? 380 : 355)
                readonly property real centerY: leftStick ? diagram.stickLeftY : 244
                x: centerX - 43; y: centerY - 43; width: 86; height: 86
                Rectangle { anchors.fill: parent; radius: 43; color: Theme.controllerInset; border.color: Theme.outline }
                Button {
                    id: stickClick
                    x: 16 + stick.moveX * 12; y: 16 + stick.moveY * 12; width: 54; height: 54
                    readonly property string controlKey: stick.leftStick ? "leftstick" : "rightstick"
                    readonly property bool pressedNow: diagram.value(controlKey) > 0.15
                    Accessible.name: stick.leftStick ? qsTr("Clic stick izquierdo") : qsTr("Clic stick derecho")
                    onClicked: diagram.controlSelected(controlKey)
                    background: Rectangle {
                        radius: 27; color: stickClick.pressedNow ? Theme.accent : Theme.controllerShell
                        border.width: 2
                        border.color: stickClick.visualFocus || diagram.mapper.target === stickClick.controlKey ||
                            diagram.mapper.target.endsWith(stick.modelData + "x") || diagram.mapper.target.endsWith(stick.modelData + "y") ||
                            Math.abs(stick.moveX) + Math.abs(stick.moveY) > 0.3 ? Theme.accent : Theme.outline
                    }
                    contentItem: Text {
                        text: stick.leftStick ? "L" : "R"; color: stickClick.pressedNow ? Theme.accentText : Theme.text
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.bold: true
                    }
                }
            }
        }
    }
}

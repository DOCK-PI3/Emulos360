pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

Dialog {
    id: dialog
    required property LibraryController controller
    readonly property GamepadMapper mapper: controller.gamepadMapper
    property string previewFamily: "auto"
    readonly property var families: [
        {value:"auto",label:qsTr("Detectar automáticamente")},
        {value:"xbox360",label:"Xbox 360"}, {value:"xboxone",label:"Xbox One"},
        {value:"ps3",label:"PlayStation 3"}, {value:"ps4",label:"PlayStation 4"},
        {value:"ps5",label:"PlayStation 5"}, {value:"generic",label:qsTr("Mando genérico")}
    ]
    readonly property string shownFamily: previewFamily === "auto" ? mapper.family : previewFamily
    readonly property var activeControls: mapper.controls.filter(row => Number(mapper.values[row.key] || 0) > 0.15)
    objectName: "gamepadMappingDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(1120, parent ? parent.width - 32 : 1120)
    height: Math.min(800, parent ? parent.height - 32 : 800)
    padding: 22
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    title: qsTr("Mapear mando")
    font.pointSize: Theme.body
    palette.window: Theme.panel
    palette.windowText: Theme.text
    palette.text: Theme.text
    palette.base: Theme.raised
    palette.highlight: Theme.accent
    palette.dark: Theme.accent
    palette.midlight: Theme.raised
    onOpened: mapper.begin()
    onClosed: mapper.end()
    Component.onDestruction: if (mapper.active) mapper.end()
    Connections {
        target: dialog.controller.players
        function onChanged(): void { if (dialog.controller.players.sessionActive) dialog.close() }
    }
    background: Rectangle { color: Theme.panel; radius: 18; border.color: Theme.outline }
    contentItem: ScrollView {
        id: scroller
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
        width: scroller.availableWidth
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            AppComboBox {
                objectName: "mappingDeviceCombo"
                Layout.fillWidth: true
                model: dialog.mapper.devices; textRole: "name"
                currentIndex: dialog.mapper.selected
                enabled: count > 0 && !dialog.mapper.target.length && !dialog.mapper.dirty
                displayText: count ? currentText : qsTr("Esperando un mando…")
                Accessible.name: qsTr("Mando conectado")
                onActivated: index => dialog.mapper.selectDevice(index)
            }
            AppComboBox {
                Layout.preferredWidth: 250
                model: dialog.families; textRole: "label"; valueRole: "value"
                Accessible.name: qsTr("Imagen del mando")
                onActivated: dialog.previewFamily = currentValue
            }
        }
        Label {
            Layout.fillWidth: true; color: Theme.muted; wrapMode: Text.Wrap
            text: dialog.mapper.dirty ? qsTr("Hay cambios pendientes. Guarda o descarta el mapa antes de cambiar de mando.") : qsTr("Elige un control en el dibujo o en la lista y pulsa el botón físico. El asistente te pedirá uno a uno todos los controles de Xbox 360.")
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: dialog.width >= 850 ? Math.max(320, dialog.height - 430) : 640
            columns: dialog.width >= 850 ? 2 : 1
            columnSpacing: 20; rowSpacing: 12
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                Layout.preferredWidth: 620
                Layout.minimumHeight: dialog.width >= 850 ? 320 : 360
                color: Theme.background; radius: 14; border.color: Theme.outline
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: dialog.families.find(row => row.value === dialog.shownFamily).label; color: Theme.text; font.pointSize: Theme.heading; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Label { text: dialog.mapper.selected >= 0 ? qsTr("● Conectado") : qsTr("○ Sin mando"); color: dialog.mapper.selected >= 0 ? Theme.accent : Theme.muted }
                    }
                    GamepadDiagram {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        mapper: dialog.mapper; family: dialog.shownFamily
                        enabled: dialog.mapper.selected >= 0
                        onControlSelected: key => { dialog.mapper.stopCapture(); dialog.mapper.capture(key) }
                    }
                    Label {
                        Layout.fillWidth: true; Layout.minimumHeight: 40
                        text: dialog.activeControls.length ? qsTr("En directo: %1").arg(dialog.activeControls.map(row => row.label).join(" · ")) : qsTr("Mueve los sticks o pulsa cualquier botón para verlo aquí.")
                        color: dialog.activeControls.length ? Theme.accent : Theme.muted; wrapMode: Text.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("LT / L2"); color: Theme.muted }
                        ProgressBar { Layout.fillWidth: true; value: dialog.mapper.values.lefttrigger || 0; Accessible.name: qsTr("Gatillo izquierdo") }
                        Label { text: qsTr("RT / R2"); color: Theme.muted }
                        ProgressBar { Layout.fillWidth: true; value: dialog.mapper.values.righttrigger || 0; Accessible.name: qsTr("Gatillo derecho") }
                    }
                }
            }
            ListView {
                id: assignments
                objectName: "mappingAssignments"
                Layout.preferredWidth: 345; Layout.fillWidth: dialog.width < 850
                Layout.fillHeight: true; Layout.minimumHeight: 160
                clip: true; spacing: 5; model: dialog.mapper.controls
                ScrollBar.vertical: ScrollBar {}
                delegate: ActionButton {
                    required property var modelData
                    width: assignments.width - 14
                    text: modelData.label + "  ·  " + (modelData.assigned ? modelData.binding : qsTr("Sin asignar"))
                    active: dialog.mapper.target === modelData.key
                    primary: active
                    enabled: dialog.mapper.selected >= 0
                    Accessible.name: text
                    onClicked: { dialog.mapper.stopCapture(); dialog.mapper.capture(modelData.key) }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: prompt.implicitHeight + 24
            color: Theme.raised; radius: 10
            Label {
                id: prompt; anchors.fill: parent; anchors.margins: 12
                text: dialog.mapper.status; color: dialog.mapper.target.length ? Theme.accent : Theme.text
                wrapMode: Text.Wrap
                Accessible.name: text
            }
        }
        ProgressBar { Layout.fillWidth: true; visible: dialog.mapper.guided; from: 0; to: dialog.mapper.controls.length; value: dialog.mapper.progress }
        RowLayout {
            Layout.fillWidth: true
            ActionButton { text: qsTr("Asistente paso a paso"); enabled: dialog.mapper.selected >= 0 && !dialog.mapper.target.length; onClicked: dialog.mapper.startGuided() }
            ActionButton { text: qsTr("Omitir"); visible: dialog.mapper.guided; onClicked: dialog.mapper.skip() }
            ActionButton { text: qsTr("Detener"); visible: dialog.mapper.target.length > 0; onClicked: dialog.mapper.stopCapture() }
            Item { Layout.fillWidth: true }
            Label { visible: dialog.mapper.guided; text: qsTr("%1 / %2").arg(dialog.mapper.progress + 1).arg(dialog.mapper.controls.length); color: Theme.muted }
        }
        Label {
            Layout.fillWidth: true; color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12
            text: qsTr("Guía / PS puede estar reservado por el sistema: puedes omitirlo. Touchpad, Share y botones extra pueden asignarse a una función de Xbox 360. Si usas un adaptador que oculta el modelo, elige la imagen manualmente. Los cambios se aplican en la próxima sesión.")
        }
        }
    }
    footer: RowLayout {
        spacing: 8
        ActionButton { text: qsTr("Restaurar mapa"); enabled: dialog.mapper.selected >= 0; onClicked: dialog.mapper.restoreDefaults() }
        ActionButton { text: qsTr("Descartar cambios"); visible: dialog.mapper.dirty; onClicked: dialog.mapper.discard() }
        Item { Layout.fillWidth: true }
        ActionButton { text: dialog.mapper.dirty ? qsTr("Descartar y cerrar") : qsTr("Cerrar"); onClicked: dialog.close() }
        ActionButton {
            objectName: "saveGamepadMapping"
            text: qsTr("Guardar mapa"); primary: true
            enabled: dialog.mapper.selected >= 0 && !dialog.mapper.target.length
            onClicked: dialog.mapper.save()
        }
    }
}

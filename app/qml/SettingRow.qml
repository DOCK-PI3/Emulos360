pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
Pane {
    id: row
    required property var entry
    required property EngineSettings settings
    property bool details: false
    property var profileValues: ({})
    readonly property bool profileManaged: profileValues[entry.key] !== undefined
    readonly property bool editable: !settings.locked && !entry.readonly && !profileManaged
    readonly property var current: profileManaged ? profileValues[entry.key] : settings.values[entry.key]
    readonly property bool isBoolean: entry.type === "bool"
    readonly property var options: {
        const list = (entry.choices || []).slice()
        if (list.length && !list.some(x => String(x.value) === String(current)))
            list.push({label: qsTr("Valor actual: %1").arg(current), value: current})
        return list
    }
    padding: 16
    background: Rectangle { radius: 10; color: Theme.panel; border.color: row.settings.errors[row.entry.key] ? "#e49e87" : Theme.outline }
    contentItem: ColumnLayout {
        spacing: 9
        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: row.entry.label; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
            Label { visible: !row.profileManaged && !!row.settings.modified[row.entry.key]; text: qsTr("Modificado"); color: Theme.accent; font.pixelSize: 11 }
        }
        Label { visible: row.profileManaged; Layout.fillWidth: true; text: qsTr("Valor del perfil gráfico. Selecciona Personalizado para editarlo."); color: Theme.accent; wrapMode: Text.Wrap; font.pixelSize: 12 }
        Label { visible: text.length > 0; Layout.fillWidth: true; text: row.entry.help || ""; color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 13 }
        RowLayout {
            Layout.fillWidth: true
            Loader {
                Layout.fillWidth: true
                sourceComponent: row.isBoolean ? booleanControl : row.options.length ? choiceControl : textControl
            }
            ToolButton { text: qsTr("Detalles"); onClicked: row.details = !row.details; Accessible.name: qsTr("Detalles de %1").arg(row.entry.label) }
            ToolButton { text: qsTr("Restaurar"); enabled: row.editable; onClicked: row.settings.resetValue(row.entry.key); Accessible.name: qsTr("Restaurar %1").arg(row.entry.label) }
        }
        Label { visible: text.length > 0; Layout.fillWidth: true; text: row.settings.errors[row.entry.key] || ""; color: "#efb5a4"; wrapMode: Text.Wrap }
        ColumnLayout {
            visible: row.details
            Layout.fillWidth: true
            Label { text: row.entry.key; color: Theme.accent; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
            Label { Layout.fillWidth: true; text: qsTr("Predeterminado: %1").arg(String(row.entry.default)); color: Theme.muted; wrapMode: Text.WrapAnywhere }
            Label { Layout.fillWidth: true; visible: text.length > 0; text: row.entry.description || ""; color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
        }
    }
    Component {
        id: booleanControl
        Switch { checked: !!row.current; text: checked ? qsTr("Activado") : qsTr("Desactivado"); enabled: row.editable; onToggled: row.settings.setValue(row.entry.key,checked); Accessible.name: row.entry.label }
    }
    Component {
        id: choiceControl
        AppComboBox { model: row.options; textRole: "label"; valueRole: "value"; currentIndex: row.options.findIndex(x => String(x.value) === String(row.current)); enabled: row.editable; onActivated: row.settings.setValue(row.entry.key,currentValue); Accessible.name: row.entry.label }
    }
    Component {
        id: textControl
        TextField {
            text: String(row.current === undefined ? "" : row.current)
            readOnly: !row.editable
            color: Theme.text
            selectByMouse: true
            onTextEdited: row.settings.setValue(row.entry.key,text)
            Accessible.name: row.entry.label
            placeholderText: row.entry.type === "path" ? qsTr("Ruta o vacío para usar la predeterminada") : ""
        }
    }
}

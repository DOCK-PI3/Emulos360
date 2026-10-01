pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los
ColumnLayout {
    id: page
    objectName: "settingsScreen"
    required property LibraryController controller
    readonly property EngineSettings settings: controller.engineSettings
    signal chooseFolder()
    property string category: "graphics"
    property string query: ""
    property bool onlyModified: false
    readonly property var groups: [
        {key:"app",label:qsTr("Emulos360")}, {key:"general",label:qsTr("General")},
        {key:"graphics",label:qsTr("Gráficos")}, {key:"display",label:qsTr("Pantalla")},
        {key:"audio",label:qsTr("Audio")}, {key:"input",label:qsTr("Mandos")},
        {key:"keyboard",label:qsTr("Teclado")}, {key:"console",label:qsTr("Consola")},
        {key:"profiles",label:qsTr("Perfiles")}, {key:"network",label:qsTr("Red")},
        {key:"storage",label:qsTr("Almacenamiento")}, {key:"system",label:qsTr("Sistema")},
        {key:"cpu",label:qsTr("CPU y memoria")}, {key:"overlay",label:qsTr("Menús del motor")},
        {key:"logging",label:qsTr("Registros")}, {key:"advanced",label:qsTr("Avanzados")}
    ]
    readonly property var filtered: settings.entries.filter(item =>
        (query.length ? (item.label + " " + item.key + " " + item.description).toLowerCase().includes(query.toLowerCase()) : item.group === category)
        && (!onlyModified || settings.modified[item.key]))
    spacing: 12
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Ajustes"); color: Theme.text; font.pointSize: Theme.display; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Importar TOML"); enabled: !page.settings.locked; onClicked: importer.open() }
        ActionButton { text: qsTr("Exportar"); onClicked: exporter.open() }
    }
    TextField {
        id: settingsSearch
        Layout.fillWidth: true
        placeholderText: qsTr("Buscar en todos los ajustes…")
        placeholderTextColor: Theme.muted
        onTextChanged: page.query = text
        Accessible.name: qsTr("Buscar ajustes")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 18
        ListView {
            id: navigation
            Layout.preferredWidth: 154
            Layout.fillHeight: true
            clip: true
            model: page.groups
            spacing: 4
            delegate: ActionButton {
                required property var modelData
                width: navigation.width
                text: modelData.label
                active: page.category === modelData.key && !page.query.length
                primary: active
                onClicked: { page.category = modelData.key; settingsSearch.clear(); settingsList.positionViewAtBeginning() }
            }
            ScrollBar.vertical: ScrollBar {}
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            RowLayout {
                visible: page.category !== "app" || page.query.length
                Layout.fillWidth: true
                Label { text: qsTr("%1 ajustes").arg(page.filtered.length); color: Theme.muted }
                Item { Layout.fillWidth: true }
                CheckBox { text: qsTr("Solo modificados"); checked: page.onlyModified; onToggled: page.onlyModified = checked }
            }
            Loader {
                visible: page.category === "app" && !page.query.length
                active: visible
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: Component { AppPreferences { controller: page.controller; onChooseFolder: page.chooseFolder() } }
            }
            ListView {
                id: settingsList
                visible: page.category !== "app" || page.query.length
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: page.filtered
                spacing: 10
                clip: true
                delegate: SettingRow {
                    required property var modelData
                    width: settingsList.width - 14
                    entry: modelData
                    settings: page.settings
                }
                ScrollBar.vertical: ScrollBar {}
                Label { anchors.centerIn: parent; visible: settingsList.count === 0; text: qsTr("No hay ajustes que coincidan."); color: Theme.muted }
            }
        }
    }
    Label { Layout.fillWidth: true; text: page.settings.status; color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
    RowLayout {
        Layout.fillWidth: true
        ActionButton { text: qsTr("Abrir carpeta"); onClicked: page.settings.openFolder() }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Restaurar sección"); enabled: !page.settings.locked && page.category !== "app"; onClicked: page.settings.resetGroup(page.category) }
        ActionButton { text: qsTr("Descartar"); enabled: !page.settings.locked && page.settings.dirty; onClicked: page.settings.reload() }
        ActionButton { text: qsTr("Guardar"); primary: true; enabled: !page.settings.locked && page.settings.dirty; onClicked: page.settings.save() }
    }
    FileDialog { id: importer; title: qsTr("Importar configuración de Xenia"); nameFilters: ["TOML (*.toml)"]; onAccepted: page.settings.importConfig(selectedFile) }
    FileDialog { id: exporter; title: qsTr("Exportar configuración"); nameFilters: ["TOML (*.toml)"]; fileMode: FileDialog.SaveFile; defaultSuffix: "toml"; onAccepted: page.settings.exportConfig(selectedFile) }
}

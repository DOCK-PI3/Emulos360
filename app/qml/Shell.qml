pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los
Pane {
    id: shell
    objectName: "mainShell"
    required property LibraryController controller
    property string page: "library"
    function focusLibrary(): void {
        if (page === "library" && contentLoader.item)
            contentLoader.item.focusLibrary()
    }
    padding: 0
    Shortcut { sequence: "Escape"; enabled: shell.visible && shell.page !== "library"; onActivated: shell.page = "library" }
    FolderDialog {
        id: folder
        title: qsTr("Selecciona tu biblioteca Xbox 360")
        onAccepted: shell.controller.chooseFolder(selectedFolder)
    }
    background: Rectangle { color: Theme.background }
    RowLayout {
        visible: true
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 218
            color: "#151916"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12
                Image { source: "qrc:/assets/emulos360.png"; sourceSize: Qt.size(48, 48); Layout.topMargin: 12; Layout.bottomMargin: 3 }
                Label { text: "Emulos360"; color: Theme.text; font.pixelSize: 23; font.weight: Font.DemiBold }
                Label { text: qsTr("TU ESPACIO DE JUEGO"); color: Theme.muted; font.pixelSize: 9; font.letterSpacing: 1.1; Layout.bottomMargin: 34 }
                Repeater {
                    model: [{key:"library",label:qsTr("Biblioteca")},{key:"store",label:qsTr("Tienda")},{key:"import",label:qsTr("Importar juegos")},{key:"achievements",label:qsTr("Logros")},{key:"profiles",label:qsTr("Perfiles y avatares")},{key:"saves",label:qsTr("Partidas")},{key:"multiplayer",label:qsTr("Multijugador")},{key:"community",label:qsTr("Comunidad")},{key:"settings",label:qsTr("Ajustes")}]
                    delegate: ActionButton {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.label
                        active: shell.page === modelData.key
                        primary: active
                        onClicked: shell.page = modelData.key
                    }
                }
                Item { Layout.fillHeight: true }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.outline }
                Label { text: qsTr("HAZLO TUYO"); color: Theme.accent; font.pixelSize: 10; font.letterSpacing: 1.5; Layout.topMargin: 10 }
                Label { text: qsTr("Tus juegos.\nTus carátulas."); color: Theme.muted; lineHeight: 1.4 }
                ActionButton {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    text: qsTr("Modo consola")
                    onClicked: shell.controller.metro.start()
                }
                Label { text: "F11"; color: Theme.muted; font.pixelSize: 10; Layout.alignment: Qt.AlignHCenter }
            }
        }
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 26
            spacing: 14
            Loader {
                id: contentLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: shell.page === "settings" ? settings : shell.page === "store" ? store : shell.page === "import" ? importer : shell.page === "achievements" ? achievements : shell.page === "profiles" ? profiles : shell.page === "saves" ? saves : shell.page === "community" ? community : shell.page === "multiplayer" ? multiplayer : library
            }
            Label { text: shell.controller.status; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
            Label { text: shell.controller.players.status; color: Theme.accent; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Label { text: shell.controller.metro.status; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap; visible: text.length > 0 }
        }
    }
    Component { id: library; LibraryScreen { controller: shell.controller; onChooseFolder: folder.open() } }
    Component { id: importer; ImportScreen { controller: shell.controller } }
    Component { id: store; StoreScreen { controller: shell.controller } }
    Component { id: achievements; AchievementsScreen { players: shell.controller.players; coverImages: shell.controller.coverImages } }
    Component { id: settings; SettingsScreen { controller: shell.controller; onChooseFolder: folder.open() } }
    Component { id: profiles; ProfilesScreen { players: shell.controller.players } }
    Component { id: saves; SavesScreen { players: shell.controller.players; games: shell.controller.games } }
    Component { id: community; CommunityScreen { players: shell.controller.players; settings: shell.controller.engineSettings; controller: shell.controller } }
    Component { id: multiplayer; PrivateNetplayScreen { controller: shell.controller } }
}

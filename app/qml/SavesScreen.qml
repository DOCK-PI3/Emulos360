pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los
ColumnLayout {
    id: page
    required property PlayerServices players
    required property var games
    property int restoring: -1
    function title(id) { const game = games.find(g => g.titleId === id); return game ? game.title : id }
    Component.onCompleted: players.refreshSaves()
    spacing: 14
    RowLayout {
        Label { text: qsTr("Tus partidas"); color: Theme.text; font.pixelSize: 28; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Actualizar"); enabled: !page.players.busy; onClicked: page.players.refreshSaves() }
    }
    Label { Layout.fillWidth: true; text: qsTr("Guarda y continúa desde el menú de cada juego. Aquí puedes crear copias y recuperar una partida anterior con el mismo perfil. No son estados instantáneos del emulador."); wrapMode: Text.Wrap; color: Theme.muted }
    RowLayout {
        Layout.fillWidth: true
        ActionButton { Layout.fillWidth: true; text: qsTr("Disco interno"); primary: page.players.saveDevice === 0; enabled: !page.players.busy; onClicked: page.players.saveDevice = 0 }
        ActionButton { Layout.fillWidth: true; text: qsTr("Disco externo"); primary: page.players.saveDevice === 1; enabled: !page.players.busy; onClicked: page.players.saveDevice = 1 }
        ActionButton { text: qsTr("Configurar externo…"); enabled: !page.players.busy; onClicked: externalFolder.open() }
    }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: (page.players.saveDevice === 0 ? page.players.internalStorage : page.players.externalStorage) }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.accent; text: qsTr("El juego permite elegir unidad cuando abre su selector de almacenamiento. El disco externo usa una carpeta virtual hasta que elijas una de tu unidad USB o HDD.") }
    FolderDialog { id: externalFolder; title: qsTr("Carpeta de partidas del disco externo"); onAccepted: page.players.chooseExternalStorage(selectedFolder) }
    RowLayout {
        id: tabs
        property int currentIndex: 0
        Layout.fillWidth: true
        ActionButton { Layout.fillWidth: true; text: qsTr("Partidas guardadas"); primary: tabs.currentIndex === 0; onClicked: tabs.currentIndex = 0 }
        ActionButton { Layout.fillWidth: true; text: qsTr("Copias de seguridad"); primary: tabs.currentIndex === 1; onClicked: tabs.currentIndex = 1 }
    }
    ListView {
        id: items
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 10
        model: tabs.currentIndex === 0 ? page.players.saves : page.players.backups
        delegate: Rectangle {
            required property var modelData
            required property int index
            width: items.width - 14; height: 100; radius: 12; color: Theme.panel; border.color: Theme.outline
            RowLayout {
                anchors.fill: parent; anchors.margins: 16
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: page.title(modelData.titleId); elide: Text.ElideRight; font.pixelSize: 18; color: Theme.text }
                    Label { text: qsTr("Perfil %1").arg(modelData.xuid); font.pixelSize: 11; color: Theme.muted }
                    Label { visible: tabs.currentIndex === 1; text: modelData.created || ""; color: Theme.muted; font.pixelSize: 11 }
                }
                ActionButton {
                    text: tabs.currentIndex === 0 ? qsTr("Crear copia") : qsTr("Restaurar")
                    enabled: !page.players.busy
                    onClicked: { if (tabs.currentIndex === 0) page.players.backupSave(index); else { page.restoring = index; confirmation.open() } }
                }
            }
        }
        ScrollBar.vertical: ScrollBar {}
        Label { anchors.centerIn: parent; width: parent.width * 0.8; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; visible: items.count === 0; text: tabs.currentIndex === 0 ? qsTr("Las partidas aparecerán después de guardar dentro de un juego.") : qsTr("Todavía no has creado copias de seguridad."); color: Theme.muted }
    }
    RowLayout {
        ActionButton { text: qsTr("Carpeta de partidas"); onClicked: page.players.openSaves() }
        ActionButton { text: qsTr("Carpeta de copias"); onClicked: page.players.openBackups() }
    }
    Dialog {
        id: confirmation; anchors.centerIn: Overlay.overlay; width: 420; modal: true; title: qsTr("Restaurar partida")
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
        Label { width: 360; wrapMode: Text.Wrap; text: qsTr("Se recuperará esta copia. La partida actual se conservará en una carpeta de recuperación junto a la original."); color: Theme.text }
        onAccepted: page.players.restoreSave(page.restoring)
    }
}

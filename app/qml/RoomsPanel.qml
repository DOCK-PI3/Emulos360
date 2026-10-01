pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ColumnLayout {
    id: panel
    required property LibraryController controller
    readonly property NetplayRooms service: controller.netplayRooms
    property bool requestedOnOpen: false
    function queryOnOpen() {
        if (visible && !requestedOnOpen) { requestedOnOpen = true; service.refresh() }
    }
    function gameFor(titleId) {
        for (let game of controller.games) if (game.titleId.toUpperCase() === titleId) return game
        return null
    }
    spacing: 12
    Component.onCompleted: queryOnOpen()
    onVisibleChanged: queryOnOpen()
    Label { text: qsTr("Salas Netplay"); color: Theme.text; font.pixelSize: 23; font.weight: Font.DemiBold }
    Label {
        Layout.fillWidth: true; color: Theme.muted; wrapMode: Text.Wrap; textFormat: Text.PlainText
        text: panel.controller.privateNetplay.connected ? qsTr("Salas del grupo privado. Abre el juego y entra desde su menú multijugador.") : qsTr("Sesiones públicas del servidor configurado. Abre el juego y entra desde su menú multijugador.")
    }
    RowLayout {
        Layout.fillWidth: true
        TextField {
            id: filter; Layout.fillWidth: true; implicitHeight: 42
            placeholderText: qsTr("Filtrar por juego o anfitrión"); Accessible.name: placeholderText
            color: Theme.text; placeholderTextColor: Theme.muted
            background: Rectangle { color: Theme.panel; radius: 8; border.color: filter.activeFocus ? Theme.accent : Theme.outline }
        }
        CheckBox { id: installed; text: qsTr("Mis juegos"); palette.windowText: Theme.text }
        ActionButton { text: panel.service.busy ? qsTr("Cancelar") : qsTr("Actualizar"); primary: true; onClicked: panel.service.busy ? panel.service.cancel() : panel.service.refresh() }
    }
    Label { Layout.fillWidth: true; text: panel.service.status; textFormat: Text.PlainText; color: Theme.accent; wrapMode: Text.Wrap }
    Label { Layout.fillWidth: true; text: panel.controller.privateNetplay.running ? panel.controller.privateNetplay.endpoint : panel.service.server; textFormat: Text.PlainText; color: Theme.muted; elide: Text.ElideMiddle; font.pixelSize: 11 }
    ListView {
        id: rooms
        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 260
        clip: true; spacing: 10
        model: panel.service.rooms
        ScrollBar.vertical: ScrollBar {}
        delegate: Pane {
            id: room
            required property var modelData
            readonly property var game: panel.gameFor(modelData.titleId)
            readonly property bool matches: (!installed.checked || !!game) &&
                (modelData.title + " " + modelData.host + " " + modelData.titleId).toLowerCase().includes(filter.text.toLowerCase())
            width: ListView.view.width
            visible: matches
            height: matches ? implicitHeight : 0
            padding: 16
            background: Rectangle { color: Theme.raised; radius: 12; border.color: Theme.outline }
            contentItem: RowLayout {
                spacing: 16
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { Layout.fillWidth: true; text: room.modelData.title; textFormat: Text.PlainText; color: Theme.text; font.pixelSize: 19; elide: Text.ElideRight }
                    Label { Layout.fillWidth: true; text: qsTr("Anfitrión: %1 · %2 / %3 jugadores").arg(room.modelData.host).arg(room.modelData.players).arg(room.modelData.total); textFormat: Text.PlainText; color: Theme.accent; wrapMode: Text.Wrap }
                    Label { Layout.fillWidth: true; text: room.modelData.presence; textFormat: Text.PlainText; color: Theme.muted; wrapMode: Text.Wrap }
                    Label { Layout.fillWidth: true; text: qsTr("Título %1 · Media %2 · Versión %3").arg(room.modelData.titleId).arg(room.modelData.mediaId || "—").arg(room.modelData.version || "—"); textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: 11; wrapMode: Text.Wrap }
                }
                ActionButton {
                    text: room.game ? qsTr("Abrir juego") : qsTr("No instalado")
                    enabled: !!room.game && room.game.complete && !panel.controller.players.busy && !panel.controller.engineSettings.dirty && Number(panel.controller.engineSettings.values["Live.network_mode"]) === 2
                    onClicked: panel.controller.players.launchGame(room.game.path, room.game.titleId)
                }
            }
        }
        Label {
            anchors.centerIn: parent; width: parent.width - 32
            visible: !panel.service.busy && panel.service.rooms.length === 0
            text: qsTr("No hay sesiones para mostrar. Actualiza para consultar el servidor.")
            color: Theme.muted; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter
        }
    }
    Label {
        Layout.fillWidth: true; color: Theme.muted; wrapMode: Text.Wrap
        text: qsTr("Para abrir un juego desde aquí, guarda el modo Netplay y selecciona un perfil. Usa la misma versión del juego que el anfitrión. Para crear una sala, abre tu juego y elige crear partida en su menú.")
    }
}

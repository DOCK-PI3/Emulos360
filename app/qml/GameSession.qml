pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ColumnLayout {
    id: session
    objectName: "gameSession"
    required property LibraryController controller
    property bool suspended: false
    property bool showRooms: false
    property bool fullScreen: false
    property bool chromeVisible: true
    signal toggleFullScreen()
    signal browseLibrary()
    spacing: fullScreen && !chromeVisible ? 0 : 8
    function confirmClose() { closeDialog.open() }
    RowLayout {
        objectName: "gameSessionToolbar"
        Layout.fillWidth: true
        Layout.margins: 8
        visible: !session.fullScreen || session.chromeVisible
        ActionButton { text: qsTr("Biblioteca"); onClicked: session.browseLibrary() }
        ActionButton { text: session.showRooms ? qsTr("Volver al juego") : qsTr("Salas Netplay"); onClicked: session.showRooms = !session.showRooms }
        Item { Layout.fillWidth: true }
        ActionButton {
            text: qsTr("Controlar juego")
            enabled: !!session.controller.players.coreWindow.window
            onClicked: {
                session.showRooms = false
                Qt.callLater(function() { session.controller.players.coreWindow.focus() })
            }
        }
        ActionButton { text: session.fullScreen ? qsTr("Ventana") : qsTr("Pantalla completa"); onClicked: session.toggleFullScreen() }
        ActionButton { text: qsTr("Cerrar sesión"); enabled: !!session.controller.players.coreWindow.window; onClicked: session.confirmClose() }
    }
    Label {
        objectName: "gameSessionStatus"
        Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
        visible: !session.fullScreen || session.chromeVisible
        text: session.showRooms ? qsTr("El juego sigue ejecutándose mientras consultas las salas.") : session.controller.players.coreWindow.status
        color: Theme.muted; wrapMode: Text.Wrap
    }
    Item {
        objectName: "gameSessionContent"
        Layout.fillWidth: true; Layout.fillHeight: true
        WindowContainer {
            anchors.fill: parent
            window: session.controller.players.coreWindow.window
            visible: session.visible && !session.showRooms && !closeDialog.visible && !session.suspended
        }
        RoomsPanel { anchors.fill: parent; anchors.margins: 16; controller: session.controller; visible: session.showRooms }
    }
    Dialog {
        id: closeDialog
        objectName: "closeSessionDialog"
        title: qsTr("Cerrar la sesión del motor")
        width: Math.min(560, Overlay.overlay.width - 48)
        implicitHeight: closeMessage.implicitHeight + topPadding + bottomPadding + header.implicitHeight + footer.implicitHeight
        anchors.centerIn: Overlay.overlay
        modal: true
        contentWidth: width - leftPadding - rightPadding
        contentHeight: closeMessage.implicitHeight
        contentItem: Label {
            id: closeMessage
            text: qsTr("Guarda la partida desde el juego antes de cerrar. Los avances que no hayas guardado se perderán.\n\n¿Quieres cerrar el juego y volver a la biblioteca?")
            color: Theme.text
            wrapMode: Text.Wrap
        }
        background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
        footer: DialogButtonBox {
            implicitHeight: 66
            ActionButton { text: qsTr("Seguir jugando"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            ActionButton { text: qsTr("Cerrar juego"); DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: session.controller.players.coreWindow.requestClose()
        onRejected: Qt.callLater(function() { session.controller.players.coreWindow.focus() })
    }
}

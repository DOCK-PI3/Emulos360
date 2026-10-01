import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los

ColumnLayout {
    id: screen
    required property LibraryController controller
    spacing: 18
    Component.onCompleted: Qt.callLater(function() { if (screen.visible) chooseFile.forceActiveFocus() })

    FileDialog {
        id: gameFile
        title: qsTr("Selecciona una ISO, un 7z o un paquete Xbox 360")
        nameFilters: [qsTr("Juegos Xbox 360 (*.iso *.7z)"), qsTr("Todos los archivos (*)")]
        onAccepted: screen.controller.importer.importFile(selectedFile, screen.controller.libraryPath)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Importar juegos")
        color: Theme.text
        font.pixelSize: 23
        font.weight: Font.DemiBold
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 230
        color: Theme.panel
        radius: 14
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: qsTr("De tu archivo a la biblioteca")
                color: Theme.text
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Abre una ISO o un archivo 7z. Las ISO se convierten a GOD; los paquetes XBLA se instalan directamente.")
                color: Theme.muted
                wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Biblioteca: %1").arg(screen.controller.libraryPath || qsTr("sin configurar"))
                color: Theme.muted
                wrapMode: Text.Wrap
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                ActionButton {
                    id: chooseFile
                    text: qsTr("Elegir archivo")
                    primary: true
                    activeFocusOnTab: true
                    enabled: !screen.controller.importer.busy && screen.controller.libraryPath.length > 0
                    onClicked: gameFile.open()
                }
                ActionButton {
                    text: qsTr("Cancelar")
                    visible: screen.controller.importer.busy
                    onClicked: screen.controller.importer.cancel()
                }
            }
        }
    }
    ProgressBar {
        Layout.fillWidth: true
        from: 0
        to: 1
        value: screen.controller.importer.progress
        visible: screen.controller.importer.busy
    }
    Label {
        Layout.fillWidth: true
        text: screen.controller.importer.status || qsTr("El archivo original se conserva. Los temporales se eliminan al terminar.")
        color: Theme.text
        wrapMode: Text.Wrap
    }
    Item { Layout.fillHeight: true }
}

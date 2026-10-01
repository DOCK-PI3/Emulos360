import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los

ColumnLayout {
    id: screen
    required property LibraryController controller
    property bool digital: false
    property string letter: "A"
    spacing: 14
    function openLetter(value) {
        letter = value
        searchField.clear()
        resultList.positionViewAtBeginning()
        controller.store.browseLetter(value, digital)
    }
    function runSearch() {
        letter = ""
        resultList.positionViewAtBeginning()
        controller.store.search(searchField.text, digital)
    }
    Component.onCompleted: {
        controller.store.browseLetter(letter, digital)
        Qt.callLater(function() { if (screen.visible) searchField.forceActiveFocus() })
    }

    FileDialog {
        id: downloadedFile
        title: qsTr("Elige el juego descargado")
        nameFilters: [qsTr("Archivos de juego (*.7z *.iso)"), qsTr("Todos los archivos (*)")]
        onAccepted: screen.controller.store.importDownload(selectedFile, screen.controller.libraryPath)
    }

    Label {
        text: qsTr("Tienda")
        color: Theme.text
        font.pixelSize: 23
        font.weight: Font.DemiBold
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: intro.implicitHeight + 30
        color: Theme.dropdownBackground
        border.color: Theme.outline
        radius: 12
        Label {
            id: intro
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 15
            color: Theme.dropdownText
            wrapMode: Text.Wrap
            text: qsTr("La ficha se abre en tu navegador predeterminado para la comprobación automática de Vimm. Pulsa Descargar allí. Cuando el 7z termine en Descargas, Emulos360 lo importará; si usas otra carpeta, pulsa «Elegir descarga».")
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        ActionButton {
            text: qsTr("Xbox 360")
            primary: !screen.digital
            onClicked: {
                if (screen.digital) {
                    screen.digital = false
                    screen.openLetter("A")
                }
            }
        }
        ActionButton {
            text: qsTr("XBLA / digital")
            primary: screen.digital
            onClicked: {
                if (!screen.digital) {
                    screen.digital = true
                    screen.openLetter("A")
                }
            }
        }
        Item { Layout.fillWidth: true }
        Label {
            text: qsTr("Catálogo: Vimm's Lair")
            color: Theme.muted
        }
    }
    Label {
        text: qsTr("Explorar por letra")
        color: Theme.muted
        font.pixelSize: 12
    }
    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 5
        Repeater {
            model: "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("")
            delegate: ActionButton {
                required property string modelData
                width: 34
                height: 36
                leftPadding: 0
                rightPadding: 0
                text: modelData
                primary: screen.letter === modelData
                onClicked: screen.openLetter(modelData)
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 9
        TextField {
            id: searchField
            objectName: "storeSearch"
            Layout.fillWidth: true
            placeholderText: qsTr("Buscar juego…")
            color: Theme.text
            activeFocusOnTab: true
            onAccepted: screen.runSearch()
        }
        ActionButton {
            text: qsTr("Buscar")
            primary: true
            enabled: !screen.controller.store.busy
            onClicked: screen.runSearch()
        }
        ActionButton {
            text: qsTr("Abrir web")
            onClicked: {
                if (searchField.text.trim().length > 0)
                    screen.controller.store.openSearchInBrowser(searchField.text, screen.digital)
                else if (screen.letter.length > 0)
                    screen.controller.store.openLetterInBrowser(screen.letter, screen.digital)
                else
                    screen.controller.store.openSearchInBrowser("", screen.digital)
            }
        }
    }
    Label {
        Layout.fillWidth: true
        text: screen.controller.store.status || qsTr("Selecciona una letra o busca un título.")
        color: Theme.text
        wrapMode: Text.Wrap
    }
    BusyIndicator { running: screen.controller.store.busy; visible: running; Layout.alignment: Qt.AlignHCenter }
    ListView {
        id: resultList
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 7
        model: screen.controller.store.results
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        delegate: Button {
            id: result
            required property var modelData
            required property int index
            width: resultList.width - 12
            implicitHeight: 62
            activeFocusOnTab: true
            onClicked: screen.controller.store.openGame(index, screen.controller.libraryPath)
            background: Rectangle {
                radius: 9
                color: result.hovered || result.visualFocus ? Theme.raised : Theme.panel
                border.color: result.visualFocus ? Theme.accent : Theme.outline
                border.width: result.visualFocus ? 2 : 1
            }
            contentItem: RowLayout {
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        text: result.modelData.title
                        color: Theme.text
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: result.modelData.kind + (result.modelData.region ? " · " + result.modelData.region : "")
                        color: Theme.muted
                        elide: Text.ElideRight
                    }
                }
                CompatibilityBadge { database: screen.controller.compatibility; titleName: result.modelData.title; showLabel: result.width >= 450 }
            }
        }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: footer.implicitHeight + 28
        color: Theme.panel
        radius: 10
        ColumnLayout {
            id: footer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 14
            spacing: 8
            Label {
                Layout.fillWidth: true
                text: screen.controller.store.awaitingDownload
                    ? qsTr("Esperando un 7z nuevo en %1").arg(screen.controller.store.downloadFolder)
                    : qsTr("Descarga en otra carpeta o ya descargada: selecciónala aquí.")
                color: Theme.muted
                wrapMode: Text.Wrap
            }
            RowLayout {
                ActionButton {
                    text: qsTr("Elegir descarga")
                    primary: true
                    enabled: !screen.controller.importer.busy && screen.controller.libraryPath.length > 0
                    onClicked: downloadedFile.open()
                }
                ActionButton {
                    text: qsTr("Dejar de esperar")
                    visible: screen.controller.store.awaitingDownload
                    onClicked: screen.controller.store.stopWaiting()
                }
                Item { Layout.fillWidth: true }
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
                visible: screen.controller.importer.status.length > 0
                text: screen.controller.importer.status
                color: Theme.text
                wrapMode: Text.Wrap
            }
        }
    }
}

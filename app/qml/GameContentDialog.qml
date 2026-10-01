pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Los

Dialog {
    id: dialog
    required property LibraryController controller
    property int removalIndex: -1
    readonly property GameContent content: controller.content
    function showFor(game: var): void {
        if (content.busy || controller.players.busy) return
        content.selectGame(game)
        open()
    }
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(850, parent.width - 48)
    height: Math.min(700, parent.height - 48)
    modal: true
    focus: true
    padding: 22
    closePolicy: Popup.CloseOnEscape
    onClosed: content.clearCandidates()
    background: Rectangle { color: Theme.background; radius: 14; border.color: Theme.outline }
    contentItem: ColumnLayout {
        spacing: 13
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Contenido del juego"); color: Theme.text; font.pixelSize: 25; font.weight: Font.DemiBold }
                Label { text: dialog.content.title + " · " + dialog.content.titleId; color: Theme.muted; elide: Text.ElideRight; Layout.fillWidth: true }
            }
            ActionButton { text: qsTr("Cerrar"); onClicked: dialog.close() }
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Instala DLC y actualizaciones desde un paquete Xbox 360, una ISO de contenido o un 7z. Se comprobará el ID del juego; las actualizaciones también se compararán con el Media ID cuando esté disponible.")
            color: Theme.muted; wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            ActionButton {
                objectName: "chooseGameContentButton"
                text: qsTr("Elegir paquete del PC")
                enabled: !dialog.content.busy && !dialog.controller.players.busy
                onClicked: packageDialog.open()
            }
            BusyIndicator { running: dialog.content.busy; visible: running; Layout.preferredHeight: 28; Layout.preferredWidth: 28 }
            Item { Layout.fillWidth: true }
            ActionButton { text: qsTr("Actualizar lista"); enabled: !dialog.content.busy; onClicked: dialog.content.refresh() }
        }
        Label { Layout.fillWidth: true; text: dialog.content.status; color: Theme.accent; wrapMode: Text.Wrap }
        Label { text: qsTr("Paquetes encontrados"); color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
        ListView {
            id: candidates
            objectName: "gameContentCandidates"
            Layout.fillWidth: true
            Layout.preferredHeight: 175
            clip: true
            spacing: 5
            model: dialog.content.candidates
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: candidateItem
                required property var modelData
                required property int index
                readonly property bool alreadyInstalled: dialog.content.installed.length > 0 &&
                                                         dialog.content.isInstalled(candidateItem.modelData)
                width: candidates.width
                height: candidateRow.implicitHeight + 16
                radius: 8
                color: Theme.panel
                border.color: Theme.outline
                RowLayout {
                    id: candidateRow
                    anchors.fill: parent
                    anchors.margins: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { Layout.fillWidth: true; text: candidateItem.modelData.name; color: Theme.text; elide: Text.ElideRight }
                        Label {
                            Layout.fillWidth: true
                            text: candidateItem.modelData.type + " · " + candidateItem.modelData.size + " · " +
                                  (candidateItem.modelData.match ? qsTr("Compatible con este juego") : candidateItem.modelData.reason)
                            color: candidateItem.modelData.match ? Theme.muted : "#ff9595"
                            elide: Text.ElideRight
                        }
                    }
                    ActionButton {
                        objectName: "installGameContentButton"
                        text: candidateItem.alreadyInstalled ? qsTr("Instalado") : qsTr("Instalar")
                        enabled: candidateItem.modelData.match && !candidateItem.alreadyInstalled &&
                                 !dialog.content.busy && !dialog.controller.players.busy
                        onClicked: dialog.content.install(candidateItem.index)
                    }
                }
            }
        }
        Label { text: qsTr("Contenido instalado"); color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
        ListView {
            id: installedList
            objectName: "installedGameContent"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5
            model: dialog.content.installed
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: installedItem
                required property var modelData
                required property int index
                width: installedList.width
                height: installedRow.implicitHeight + 16
                radius: 8
                color: Theme.panel
                border.color: Theme.outline
                RowLayout {
                    id: installedRow
                    anchors.fill: parent
                    anchors.margins: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { Layout.fillWidth: true; text: installedItem.modelData.name; color: Theme.text; elide: Text.ElideRight }
                        Label { text: installedItem.modelData.type + " · " + installedItem.modelData.profile; color: Theme.muted }
                    }
                    ActionButton {
                        text: qsTr("Eliminar")
                        enabled: !dialog.content.busy && !dialog.controller.players.busy
                        onClicked: { dialog.removalIndex = installedItem.index; confirmDelete.open() }
                    }
                }
            }
        }
        Label {
            Layout.fillWidth: true
            text: dialog.content.installed.length === 0 ? qsTr("No hay DLC ni actualizaciones instalados para este juego.") : ""
            color: Theme.muted
        }
    }
    FileDialog {
        id: packageDialog
        title: qsTr("Elegir contenido Xbox 360")
        // Xbox content files commonly have a hexadecimal name without an extension.
        nameFilters: [qsTr("Todos los archivos (*)")]
        onAccepted: dialog.content.inspect(selectedFile)
    }
    Dialog {
        id: confirmDelete
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(470, parent.width - 48)
        height: 180
        padding: 18
        modal: true
        focus: true
        title: qsTr("Eliminar contenido")
        background: Rectangle { color: Theme.panel; radius: 10; border.color: Theme.outline }
        contentItem: Label {
            text: qsTr("¿Eliminar este DLC o actualización? El juego base y las partidas no se tocarán.")
            color: Theme.text
            wrapMode: Text.Wrap
        }
        footer: DialogButtonBox {
            ActionButton { text: qsTr("Cancelar"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            ActionButton {
                text: qsTr("Eliminar")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: { dialog.content.remove(dialog.removalIndex); confirmDelete.close() }
            }
        }
    }
}

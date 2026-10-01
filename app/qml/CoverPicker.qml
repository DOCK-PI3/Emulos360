pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Los
Dialog {
    id: picker
    required property LibraryController controller
    property var game: null
    property int selectedOption: -1
    property string region: "Todas"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(1060, parent.width - 48)
    height: Math.min(690, parent.height - 48)
    modal: true
    focus: true
    padding: 24
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 16; color: Theme.background; border.color: Theme.outline }
    function showFor(value: var): void {
        game = value
        selectedOption = -1
        regions.currentIndex = 0
        region = "Todas"
        open()
        controller.findCovers(value.titleId)
    }
    onOpened: {
        options.currentIndex = 0
        options.forceActiveFocus()
    }
    onClosed: controller.cancelCovers()
    contentItem: ColumnLayout {
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Dale tu estilo."); color: Theme.text; font.pixelSize: 29; font.weight: Font.DemiBold }
                Label { text: picker.game ? picker.game.title : ""; color: Theme.muted; Layout.fillWidth: true; elide: Text.ElideRight }
            }
            ActionButton { text: qsTr("Cerrar"); onClicked: picker.close() }
        }
        RowLayout {
            Label { text: qsTr("Región de la carátula"); color: Theme.muted }
            AppComboBox {
                id: regions
                model: [qsTr("Todas"), qsTr("Europa"), qsTr("América"), qsTr("Asia"), qsTr("Sin especificar")]
                Layout.preferredWidth: 190
                onActivated: picker.region = currentText
                Accessible.name: qsTr("Filtrar carátulas por región")
            }
            Item { Layout.fillWidth: true }
            ActionButton {
                objectName: "chooseLocalCoverButton"
                text: qsTr("Elegir imagen del PC")
                onClicked: localCoverDialog.open()
            }
            ActionButton { text: qsTr("Reintentar"); enabled: !picker.controller.coverBusy; onClicked: { picker.selectedOption = -1; picker.controller.findCovers(picker.game.titleId) } }
        }
        Label { Layout.fillWidth: true; text: qsTr("Vista previa por mercado e idioma. Algunos países comparten la misma imagen."); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
        GridView {
            id: options
            objectName: "coverOptionsGrid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            readonly property int columns: Math.max(2, Math.floor(width / 215))
            readonly property var visibleOptions: picker.controller.coverOptions.map((option, index) => Object.assign({}, option, {optionIndex: index})).filter(option => picker.region === "Todas" || option.region === picker.region)
            cellWidth: width / columns
            cellHeight: 314
            keyNavigationEnabled: true
            activeFocusOnTab: true
            Keys.priority: Keys.BeforeItem
            model: visibleOptions
            function syncSelection(): void {
                const option = visibleOptions[currentIndex]
                picker.selectedOption = option && option.state === "ready" ? option.optionIndex : -1
            }
            function move(delta: int): void {
                if (count === 0) return
                currentIndex = Math.max(0, Math.min(count - 1, currentIndex + delta))
                positionViewAtIndex(currentIndex, GridView.Contain)
                syncSelection()
            }
            function applyCurrent(): void {
                syncSelection()
                if (picker.selectedOption >= 0 && picker.controller.useCover(picker.selectedOption)) picker.close()
            }
            onCurrentIndexChanged: syncSelection()
            onVisibleOptionsChanged: syncSelection()
            Keys.onLeftPressed: function(event) { options.move(-1); event.accepted = true }
            Keys.onRightPressed: function(event) { options.move(1); event.accepted = true }
            Keys.onUpPressed: function(event) { options.move(-options.columns); event.accepted = true }
            Keys.onDownPressed: function(event) { options.move(options.columns); event.accepted = true }
            Keys.onReturnPressed: options.applyCurrent()
            Keys.onEnterPressed: options.applyCurrent()
            Keys.onEscapePressed: picker.close()
            delegate: ItemDelegate {
                id: candidate
                required property var modelData
                required property int index
                width: options.cellWidth - 12
                height: options.cellHeight - 12
                padding: 10
                focusPolicy: Qt.NoFocus
                Accessible.name: modelData.name + ", " + modelData.provider
                onClicked: { options.currentIndex = index; options.forceActiveFocus(); options.syncSelection() }
                background: Rectangle { radius: 10; color: Theme.panel; border.color: options.currentIndex === candidate.index && picker.opened ? Theme.accent : Theme.outline; border.width: options.currentIndex === candidate.index && picker.opened ? 2 : 1 }
                contentItem: ColumnLayout {
                    spacing: 6
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Image { anchors.fill: parent; source: candidate.modelData.image || ""; sourceSize: Qt.size(360, 450); asynchronous: true; fillMode: Image.PreserveAspectFit }
                        BusyIndicator { anchors.centerIn: parent; running: candidate.modelData.state === "pending"; visible: running }
                        Label { anchors.centerIn: parent; width: parent.width; text: candidate.modelData.error || ""; visible: candidate.modelData.state === "unavailable"; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.muted; font.pixelSize: 12 }
                    }
                    Label { Layout.fillWidth: true; text: candidate.modelData.name; color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold; elide: Text.ElideRight }
                    Label { text: candidate.modelData.provider; color: Theme.muted; font.pixelSize: 10 }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: picker.controller.coverStatus; color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
            ActionButton { text: qsTr("Usar esta carátula"); primary: true; enabled: picker.selectedOption >= 0; onClicked: { if (picker.controller.useCover(picker.selectedOption)) picker.close() } }
        }
    }
    FileDialog {
        id: localCoverDialog
        objectName: "localCoverFileDialog"
        title: qsTr("Elegir carátula para %1").arg(picker.game ? picker.game.title : "")
        fileMode: FileDialog.OpenFile
        parentWindow: picker.parent ? picker.parent.Window.window : null
        nameFilters: [qsTr("Imágenes (*.png *.jpg *.jpeg *.bmp)")]
        onAccepted: {
            const selected = selectedFile
            Qt.callLater(function() {
                if (picker.game && picker.controller.useLocalCover(picker.game.titleId, selected))
                    picker.close()
            })
        }
    }
}

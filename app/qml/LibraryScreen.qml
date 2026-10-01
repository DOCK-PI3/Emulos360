pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ColumnLayout {
    id: screen
    required property LibraryController controller
    signal chooseFolder()
    property string query: ""
    readonly property bool carouselMode: controller.libraryView === "carousel"
    readonly property var filteredGames: controller.games.filter(game =>
        (game.title + " " + game.titleId).toLowerCase().includes(query))
    readonly property var selectedGame: carouselMode
        ? (carouselLoader.item ? carouselLoader.item.selectedGame : null)
        : (grid.currentIndex >= 0 && grid.currentIndex < grid.count ? filteredGames[grid.currentIndex] : null)
    function focusLibrary(): void {
        if (carouselMode) {
            if (carouselLoader.item) carouselLoader.item.focusLibrary()
        } else {
            grid.forceActiveFocus()
        }
    }
    function playSelected(): void {
        if (selectedGame && selectedGame.complete && !controller.players.busy)
            controller.players.launchGame(selectedGame.path, selectedGame.titleId)
    }
    function editSelectedCover(): void {
        if (selectedGame) picker.showFor(selectedGame)
    }
    function confirmRemoval(value: var): void {
        if (!value || controller.busy || controller.players.busy) return
        removeDialog.game = value
        removeDialog.errorText = ""
        removeDialog.open()
    }
    spacing: 18
    onVisibleChanged: if (visible) Qt.callLater(function() {
        if (screen.visible) screen.focusLibrary()
    })
    Component.onCompleted: if (screen.visible) screen.focusLibrary()
    Shortcut { sequence: "F6"; enabled: screen.visible && !picker.visible; onActivated: screen.editSelectedCover() }
    Shortcut { sequence: "F8"; enabled: screen.visible && !picker.visible && !removeDialog.visible; onActivated: screen.confirmRemoval(screen.selectedGame) }
    Shortcut { sequence: "Escape"; enabled: screen.visible && !picker.visible && !removeDialog.visible && screen.query.length > 0; onActivated: { search.clear(); screen.focusLibrary() } }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Biblioteca"); color: Theme.text; font.pixelSize: 23; font.weight: Font.DemiBold }
        Label { text: screen.controller.games.length.toString().padStart(2, "0"); color: Theme.accent; font.pixelSize: 14 }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Añadir carpeta"); enabled: !screen.controller.busy; onClicked: screen.chooseFolder() }
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: screen.height < 620 ? 116 : 150
        radius: 14
        clip: true
        gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: "#283b2b" } GradientStop { position: 1; color: "#1a241c" } }
        Rectangle { width: 240; height: 240; radius: 120; x: parent.width - 275; y: -50; color: "transparent"; border.width: 26; border.color: "#334e32"; rotation: -25 }
        Rectangle { width: 160; height: 160; radius: 80; x: parent.width - 160; y: 35; color: "transparent"; border.width: 2; border.color: "#5b7649" }
        Column {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 25
            spacing: 7
            Label { text: qsTr("UNA GENERACIÓN. TU COLECCIÓN."); color: Theme.accent; font.pixelSize: 10; font.letterSpacing: 1.8 }
            Label { text: qsTr("Vuelve al frente."); color: Theme.text; font.pixelSize: 35; font.weight: Font.DemiBold }
            Label { text: qsTr("Redescubre tu biblioteca de Xbox 360."); color: Theme.muted; font.pixelSize: 13 }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        TextField {
            id: search
            objectName: "librarySearch"
            Layout.fillWidth: true
            implicitHeight: 42
            leftPadding: 15
            color: Theme.text
            placeholderText: qsTr("Buscar en tu colección…")
            placeholderTextColor: Theme.muted
            onTextChanged: screen.query = text.toLowerCase()
            Accessible.name: qsTr("Buscar juegos")
            background: Rectangle { color: Theme.panel; radius: 9; border.width: 1; border.color: search.activeFocus ? Theme.accent : Theme.outline }
        }
        ActionButton { text: qsTr("Actualizar"); enabled: !screen.controller.busy; onClicked: screen.controller.scan() }
    }
    GridView {
        id: grid
        objectName: "gameGrid"
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !screen.carouselMode
        clip: true
        cellWidth: width / Math.max(2, Math.floor(width / (screen.controller.consoleMode ? 240 : 190)))
        cellHeight: screen.controller.consoleMode ? 385 : 320
        keyNavigationEnabled: true
        activeFocusOnTab: true
        Keys.priority: Keys.BeforeItem
        model: screen.filteredGames
        delegate: GameCard {
            required property var modelData
            required property int index
            width: grid.cellWidth - 14
            height: grid.cellHeight - 18
            game: modelData
            compatibility: screen.controller.compatibility
            artwork: screen.controller.coverImages[modelData.titleId] || ""
            selected: GridView.isCurrentItem
            onClicked: { grid.currentIndex = index; grid.forceActiveFocus() }
            onDoubleClicked: picker.showFor(modelData)
            onRemoveRequested: { grid.currentIndex = index; screen.confirmRemoval(modelData) }
        }
        Keys.onReturnPressed: screen.playSelected()
        Keys.onEnterPressed: screen.playSelected()
        ScrollBar.vertical: ScrollBar {}
        Label { anchors.centerIn: parent; visible: grid.count === 0 && !screen.controller.busy; text: screen.controller.games.length === 0 ? qsTr("Añade tu carpeta de juegos para comenzar.") : qsTr("No hay juegos que coincidan."); color: Theme.muted }
        BusyIndicator { anchors.centerIn: parent; running: screen.controller.busy; visible: running }
    }
    Loader {
        id: carouselLoader
        Layout.fillWidth: true
        Layout.fillHeight: true
        active: screen.carouselMode
        visible: screen.carouselMode
        sourceComponent: LibraryCarousel {
            games: screen.filteredGames
            coverImages: screen.controller.coverImages
            compatibility: screen.controller.compatibility
            reducedMotion: screen.controller.reducedMotion
            busy: screen.controller.busy
            totalGames: screen.controller.games.length
            onPlayRequested: screen.playSelected()
            onCoverRequested: (game) => picker.showFor(game)
            onRemoveRequested: (game) => screen.confirmRemoval(game)
        }
        onLoaded: if (screen.visible && screen.carouselMode) item.focusLibrary()
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 76
        color: Theme.panel
        radius: 12
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label { Layout.fillWidth: true; text: screen.selectedGame ? screen.selectedGame.title : qsTr("Tu próxima partida empieza aquí"); color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold; elide: Text.ElideRight }
                Label { text: screen.selectedGame ? qsTr("Xbox 360  ·  %1").arg(screen.selectedGame.titleId) : qsTr("Selecciona un juego para personalizarlo."); color: Theme.muted; font.pixelSize: 11 }
            }
            ActionButton { text: qsTr("Carátula · Y"); enabled: screen.selectedGame !== null; onClicked: screen.editSelectedCover() }
            ActionButton { text: qsTr("Contenido"); enabled: screen.selectedGame !== null && !screen.controller.content.busy && !screen.controller.players.busy; onClicked: contentDialog.showFor(screen.selectedGame) }
            ActionButton { text: qsTr("Eliminar · X"); enabled: screen.selectedGame !== null && !screen.controller.busy && !screen.controller.players.busy; onClicked: screen.confirmRemoval(screen.selectedGame) }
            ActionButton { text: qsTr("Jugar · A"); primary: true; enabled: screen.selectedGame !== null && screen.selectedGame.complete && !screen.controller.players.busy; onClicked: screen.playSelected() }
        }
    }
    CoverPicker { id: picker; objectName: "libraryCoverPicker"; controller: screen.controller; onClosed: screen.focusLibrary() }
    GameContentDialog { id: contentDialog; objectName: "gameContentDialog"; controller: screen.controller; onClosed: screen.focusLibrary() }
    Dialog {
        id: removeDialog
        objectName: "removeGameDialog"
        property var game: null
        property string errorText: ""
        function deleteGame(): void {
            if (!game) return
            if (screen.controller.removeGame(game.path)) close()
            else errorText = screen.controller.status
        }
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 48)
        implicitHeight: removeBody.implicitHeight + topPadding + bottomPadding + header.implicitHeight + footer.implicitHeight
        contentWidth: width - leftPadding - rightPadding
        contentHeight: removeBody.implicitHeight
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        title: qsTr("Eliminar juego")
        background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
        contentItem: ColumnLayout {
            id: removeBody
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: !removeDialog.game ? "" : removeDialog.game.format === "XEX"
                    ? qsTr("¿Eliminar «%1» y toda su carpeta?").arg(removeDialog.game.title)
                    : removeDialog.game.format === "GOD"
                    ? qsTr("¿Eliminar «%1», su paquete y sus fragmentos?").arg(removeDialog.game.title)
                    : qsTr("¿Eliminar «%1»?").arg(removeDialog.game.title)
                color: Theme.text
                wrapMode: Text.Wrap
            }
            Label { Layout.fillWidth: true; text: qsTr("Se intentará usar la Papelera. Si no está disponible, los archivos se borrarán definitivamente."); color: Theme.muted; wrapMode: Text.Wrap }
            Label { Layout.fillWidth: true; text: qsTr("Mando: izquierda/derecha para elegir, A para aceptar y B para cancelar."); color: Theme.muted; wrapMode: Text.Wrap }
            Label { Layout.fillWidth: true; text: removeDialog.errorText; visible: text.length > 0; color: "#ff9595"; wrapMode: Text.Wrap }
        }
        footer: DialogButtonBox {
            ActionButton {
                id: cancelRemoval
                objectName: "cancelGameRemovalButton"
                text: qsTr("Cancelar")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                Keys.priority: Keys.BeforeItem
                Keys.onRightPressed: confirmRemoval.forceActiveFocus()
                Keys.onReturnPressed: removeDialog.close()
                Keys.onEnterPressed: removeDialog.close()
            }
            ActionButton {
                id: confirmRemoval
                objectName: "confirmGameRemovalButton"
                text: qsTr("Eliminar juego")
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                Keys.priority: Keys.BeforeItem
                Keys.onLeftPressed: cancelRemoval.forceActiveFocus()
                Keys.onReturnPressed: removeDialog.deleteGame()
                Keys.onEnterPressed: removeDialog.deleteGame()
                onClicked: removeDialog.deleteGame()
            }
        }
        onOpened: cancelRemoval.forceActiveFocus()
        onClosed: if (screen.visible) screen.focusLibrary()
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Los
ApplicationWindow {
    id: window
    required property LibraryController controller
    property string initialPage: "library"
    property bool introEnabled: false
    property bool introFinished: !introEnabled
    property bool showSession: true
    readonly property bool sessionActive: controller.players.sessionActive
    property bool gameFullScreen: false
    property bool gameChromeVisible: true
    property bool editingMetroCover: false
    property bool maximizeAfterMetro: false
    property bool metroWasActive: false
    property bool temporaryFullScreen: false
    property int visibilityBeforeFullScreen: Window.Windowed
    function updateWindowVisibility(): void {
        const needsFullScreen = editingMetroCover || (sessionActive && showSession && gameFullScreen)
        if (needsFullScreen && !temporaryFullScreen) {
            visibilityBeforeFullScreen = visibility
            temporaryFullScreen = true
            visibility = Window.FullScreen
        } else if (!needsFullScreen && temporaryFullScreen) {
            temporaryFullScreen = false
            visibility = visibilityBeforeFullScreen
        }
    }
    function toggleGameChrome(): void {
        if (!sessionActive || !showSession || !gameFullScreen || gameSession.showRooms || powerDialog.visible) return
        gameChromeVisible = !gameChromeVisible
        if (!gameChromeVisible) Qt.callLater(function() {
            if (!window.gameChromeVisible) window.controller.players.coreWindow.focus()
        })
    }
    onSessionActiveChanged: {
        gameFullScreen = controller.metro.active
        gameChromeVisible = true
        showSession = true
        updateWindowVisibility()
        if (!sessionActive && !controller.metro.active) Qt.callLater(function() {
            if (!window.sessionActive && !window.controller.metro.active) {
                window.initialPage = "library"
                libraryShell.page = "library"
                window.requestActivate()
                libraryShell.focusLibrary()
            }
        })
    }
    onShowSessionChanged: {
        if (!showSession) gameChromeVisible = true
        updateWindowVisibility()
    }
    onGameFullScreenChanged: {
        if (!gameFullScreen) gameChromeVisible = true
        updateWindowVisibility()
    }
    onEditingMetroCoverChanged: updateWindowVisibility()
    property bool avatarPreview: false
    property bool showAvatar: false
    width: 1320
    height: 820
    minimumWidth: 960
    minimumHeight: 640
    visible: true
    visibility: Window.Maximized
    title: "Emulos360"
    color: Theme.background
    font.pointSize: Theme.body
    palette.windowText: Theme.text
    palette.window: Theme.panel
    palette.text: Theme.text
    palette.buttonText: Theme.text
    palette.base: Theme.panel
    palette.button: Theme.raised
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        ActionButton {
            Layout.fillWidth: true
            visible: window.controller.players.sessionActive && !window.showSession
            text: qsTr("Volver a la sesión · el motor sigue ejecutándose")
            primary: true
            onClicked: window.showSession = true
        }
        Shell { id: libraryShell; Layout.fillWidth: true; Layout.fillHeight: true; visible: !window.editingMetroCover && (!window.controller.players.sessionActive || !window.showSession); controller: window.controller; page: window.initialPage }
        GameSession {
            id: gameSession
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: !window.editingMetroCover && window.controller.players.sessionActive && window.showSession
            controller: window.controller
            suspended: window.controller.metro.powerOpen
            fullScreen: window.gameFullScreen
            chromeVisible: window.gameChromeVisible
            onToggleFullScreen: window.gameFullScreen = !window.gameFullScreen
            onBrowseLibrary: window.showSession = false
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: window.editingMetroCover
            color: Theme.background
        }
    }
    Connections {
        target: window.controller
        function onToggleGameChromeRequested() { window.toggleGameChrome() }
    }
    Connections {
        target: window.controller.players
        function onChanged() { if (!window.controller.players.sessionActive) window.showSession = true }
        function onOperationFailed(reason: string) { operationError.reason = reason; operationError.open() }
    }
    Connections {
        target: window.controller.players.avatarStudio
        function onOpened() { window.showAvatar = true }
    }
    Loader {
        anchors.fill: parent; active: window.showAvatar
        sourceComponent: AvatarEditor {
            players: window.controller.players
            onCloseRequested: window.showAvatar = false
        }
    }
    Loader {
        parent: Overlay.overlay
        anchors.fill: parent
        z: 1000
        active: window.introEnabled && !window.introFinished
        sourceComponent: StartupIntro {
            videoSource: window.controller.introVideo
            posterSource: window.controller.introPoster
            loading: window.controller.busy
            onFinished: window.introFinished = true
        }
    }
    Connections {
        target: window.controller.metro
        function onShowPage(page: string) {
            window.initialPage = page
            libraryShell.page = page
            window.showSession = false
            window.maximizeAfterMetro = true
            window.updateWindowVisibility()
            if (!window.temporaryFullScreen) window.visibility = Window.Maximized
            window.requestActivate()
            if (page === "library") Qt.callLater(function() { libraryShell.focusLibrary() })
        }
        function onEditCover(game: var) {
            window.editingMetroCover = true
            window.showSession = false
            consoleCoverPicker.showFor(game)
            window.requestActivate()
        }
        function onChanged() {
            if (window.controller.metro.active) window.metroWasActive = true
            else if (window.metroWasActive && !window.sessionActive) {
                window.metroWasActive = false
                window.maximizeAfterMetro = true
                window.updateWindowVisibility()
                if (!window.temporaryFullScreen) window.visibility = Window.Maximized
                window.requestActivate()
                Qt.callLater(function() { libraryShell.focusLibrary() })
            }
            if (window.controller.metro.powerOpen) {
                powerDialog.confirmShutdown = false
                powerDialog.open()
                window.requestActivate()
            } else powerDialog.close()
        }
    }
    CoverPicker {
        id: consoleCoverPicker
        objectName: "consoleCoverPicker"
        controller: window.controller
        onClosed: {
            window.editingMetroCover = false
            window.controller.metro.finishCoverEdit()
        }
    }
    Dialog {
        id: powerDialog
        property bool confirmShutdown: false
        anchors.centerIn: Overlay.overlay
        width: Math.min(650, window.width - 48)
        modal: true
        closePolicy: Popup.NoAutoClose
        title: confirmShutdown ? qsTr("Apagar el PC") : qsTr("Sistema")
        background: Rectangle { color: Theme.panel; border.color: Theme.outline; radius: 12 }
        contentItem: ColumnLayout {
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: powerDialog.confirmShutdown ? qsTr("Se cerrará el juego y se apagará Windows. Guarda la partida y los documentos de otras aplicaciones antes de continuar.") : qsTr("Guarda tu partida antes de salir. El juego continúa ejecutándose mientras este menú está abierto.")
                color: Theme.text; wrapMode: Text.Wrap
            }
            ActionButton {
                Layout.fillWidth: true
                text: qsTr("Continuar")
                onClicked: window.controller.metro.cancelPower()
            }
            ActionButton {
                Layout.fillWidth: true
                visible: !powerDialog.confirmShutdown
                text: qsTr("Volver al dashboard / reiniciarlo")
                onClicked: window.controller.metro.confirmPower(5)
            }
            ActionButton {
                Layout.fillWidth: true
                visible: !powerDialog.confirmShutdown
                text: qsTr("Cerrar dashboard · volver a Emulos360")
                onClicked: window.controller.metro.confirmPower(4)
            }
            ActionButton {
                Layout.fillWidth: true
                text: powerDialog.confirmShutdown ? qsTr("Confirmar: apagar Windows") : qsTr("Apagar el PC…")
                onClicked: {
                    if (powerDialog.confirmShutdown) window.controller.metro.confirmPower(6)
                    else powerDialog.confirmShutdown = true
                }
            }
        }
        onOpened: contentItem.children[1].forceActiveFocus()
    }
    Shortcut { sequence: "Home"; enabled: window.controller.metro.active; onActivated: window.controller.metro.openPower() }
    Shortcut { sequence: "F8"; enabled: window.sessionActive && window.showSession && window.gameFullScreen && !gameSession.showRooms; onActivated: window.toggleGameChrome() }
    Shortcut { sequence: "Escape"; enabled: powerDialog.visible; onActivated: window.controller.metro.cancelPower() }
    Component.onCompleted: { if (avatarPreview) controller.players.avatarStudio.preview() }
    Dialog {
        id: operationError
        objectName: "operationErrorDialog"
        property string reason: ""
        anchors.centerIn: Overlay.overlay
        width: Math.min(620, window.width - 48)
        title: qsTr("No se pudo completar la operación")
        modal: true
        standardButtons: Dialog.Ok
        contentItem: Label { text: operationError.reason; textFormat: Text.PlainText; wrapMode: Text.Wrap }
    }
    onClosing: function(close) {
        if (controller.players.busy) {
            close.accepted = false
            if (controller.players.sessionActive) { window.showSession = true; gameSession.confirmClose() }
        }
    }
    Shortcut {
        sequence: "F11"
        enabled: window.introFinished
        onActivated: {
            if (window.sessionActive && window.showSession) window.gameFullScreen = !window.gameFullScreen
            else window.controller.metro.start()
        }
    }
}

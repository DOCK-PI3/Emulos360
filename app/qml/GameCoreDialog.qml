pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

Dialog {
    id: dialog
    required property LibraryController controller
    property var game: null
    property var savedOptions: ({renderer: "", graphicsProfile: ""})
    readonly property CoreOptions cores: controller.players.cores
    readonly property var rendererChoices: [{value: "", label: qsTr("Usar selección global")}].concat(cores.rendererChoices.filter(row => row.value !== ""))
    readonly property var profileChoices: [{value: "", label: qsTr("Usar perfil global")}].concat(cores.graphicsProfileChoices)
    readonly property string selectedProfile: profileCombo.currentValue || cores.defaultGraphicsProfile
    function showFor(value: var): void {
        game = value
        savedOptions = cores.gameOptions(value.titleId)
        rendererCombo.currentIndex = Math.max(0, rendererChoices.findIndex(row => row.value === savedOptions.renderer))
        profileCombo.currentIndex = Math.max(0, profileChoices.findIndex(row => row.value === savedOptions.graphicsProfile))
        open()
    }
    objectName: "gameCoreDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(570, parent ? parent.width - 48 : 570)
    title: game ? qsTr("Gráficos · %1").arg(game.title) : qsTr("Gráficos del juego")
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    contentWidth: width - leftPadding - rightPadding
    contentHeight: body.implicitHeight
    contentItem: ColumnLayout {
        id: body
        spacing: 12
        Label { text: qsTr("Perfil gráfico"); color: Theme.text }
        AppComboBox { id: profileCombo; objectName: "gameGraphicsProfileCombo"; Layout.fillWidth: true; model: dialog.profileChoices; textRole: "label"; valueRole: "value"; enabled: !dialog.cores.locked; Accessible.name: qsTr("Perfil gráfico para este juego") }
        Label { Layout.fillWidth: true; text: dialog.cores.graphicsProfileDescription(dialog.selectedProfile); color: Theme.muted; wrapMode: Text.Wrap }
        Label { Layout.fillWidth: true; text: qsTr("Resoluciones orientativas desde 720p. La salida se adapta a la ventana o monitor. Si aparecen problemas al escalar, vuelve a Predeterminado."); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
        Label { text: qsTr("Motor gráfico"); color: Theme.text }
        AppComboBox { id: rendererCombo; objectName: "gameRendererCombo"; Layout.fillWidth: true; model: dialog.rendererChoices; textRole: "label"; valueRole: "value"; enabled: !dialog.cores.locked; Accessible.name: qsTr("Motor gráfico para este juego") }
        Label { Layout.fillWidth: true; text: qsTr("La selección gráfica se guarda para este juego y se aplica al iniciar Xenia Canary."); color: Theme.muted; wrapMode: Text.Wrap }
        Label { Layout.fillWidth: true; text: dialog.cores.status; color: Theme.muted; wrapMode: Text.Wrap }
    }
    background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
    footer: RowLayout {
        spacing: 8
        ActionButton {
            objectName: "gameUseGlobalGraphicsButton"
            text: qsTr("Usar globales")
            enabled: !dialog.cores.locked
            onClicked: if (dialog.game && dialog.cores.saveGameOptions(dialog.game.titleId, "", "")) dialog.close()
        }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Cancelar"); onClicked: dialog.close() }
        ActionButton {
            objectName: "gameSaveGraphicsButton"
            text: qsTr("Guardar")
            primary: true
            enabled: !dialog.cores.locked
            onClicked: if (dialog.game && dialog.cores.saveGameOptions(dialog.game.titleId, rendererCombo.currentValue || "", profileCombo.currentValue || "")) dialog.close()
        }
    }
}

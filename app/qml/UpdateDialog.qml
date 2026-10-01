pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

Dialog {
    id: dialog
    required property AppUpdater updater
    required property bool canInstall
    property string view: "offer"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(650, parent.width - 48)
    modal: true
    focus: true
    padding: 22
    closePolicy: updater.phase === "downloading" || updater.phase === "installing"
                 ? Popup.NoAutoClose : Popup.CloseOnEscape
    title: view === "news" ? qsTr("Novedades de Emulos360 %1").arg(updater.startupVersion)
         : view === "error" ? qsTr("Actualización")
         : qsTr("Nueva versión de Emulos360: %1").arg(updater.availableVersion)
    background: Rectangle { radius: 12; color: Theme.background; border.color: Theme.outline }

    function showOffer(): void { view = "offer"; open() }
    function showNews(): void { view = "news"; open() }
    function showError(): void { view = "error"; open() }

    onClosed: {
        if (view === "news") updater.clearStartupNews()
        if (view === "error") updater.clearStartupError()
    }
    Connections {
        target: dialog.updater
        function onChanged() {
            if (dialog.opened && dialog.view === "offer" && dialog.updater.phase === "error")
                dialog.view = "error"
        }
    }
    contentItem: ColumnLayout {
        spacing: 14
        Label {
            Layout.fillWidth: true
            text: dialog.view === "news" ? qsTr("La aplicación se actualizó y se reinició correctamente.")
                : dialog.view === "error" ? (dialog.updater.startupError || dialog.updater.status)
                : qsTr("Versión instalada: %1. ¿Quieres descargar e instalar la nueva versión?").arg(dialog.updater.currentVersion)
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: dialog.view === "error" ? Theme.accent : Theme.text
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, Math.max(90, notes.implicitHeight + 12))
            visible: dialog.view !== "error"
            clip: true
            TextArea {
                id: notes
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.PlainText
                text: dialog.view === "news" ? (dialog.updater.startupNotes || qsTr("Sin notas de esta versión."))
                      : (dialog.updater.releaseNotes || qsTr("Sin notas de esta versión."))
                color: Theme.muted
                background: Rectangle { color: Theme.panel; radius: 8 }
            }
        }
        ProgressBar {
            Layout.fillWidth: true
            visible: dialog.view === "offer" && dialog.updater.phase === "downloading"
            value: dialog.updater.progress / 100
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.view === "offer" &&
                     (dialog.updater.phase === "downloading" || dialog.updater.phase === "installing" || !dialog.canInstall)
            text: !dialog.canInstall ? qsTr("Cierra el juego antes de actualizar.") : dialog.updater.status
            wrapMode: Text.Wrap
            color: Theme.muted
        }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            ActionButton {
                text: dialog.view === "offer" ? qsTr("Más tarde") : qsTr("Cerrar")
                enabled: dialog.updater.phase !== "downloading" && dialog.updater.phase !== "installing"
                onClicked: dialog.close()
            }
            ActionButton {
                visible: dialog.view === "offer"
                text: qsTr("Actualizar y reiniciar")
                primary: true
                enabled: dialog.canInstall && dialog.updater.phase === "available"
                onClicked: dialog.updater.startUpdate()
            }
        }
    }
}

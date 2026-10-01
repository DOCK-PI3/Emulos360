import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Los
ColumnLayout {
    id: page
    objectName: "appAppearance"
    required property LibraryController controller
    signal chooseFolder()
    spacing: 12
    Label { text: qsTr("Biblioteca y apariencia"); color: Theme.text; font.pointSize: Theme.heading }
    Label { Layout.fillWidth: true; text: page.controller.libraryPath || qsTr("Sin carpeta seleccionada"); wrapMode: Text.WrapAnywhere; color: Theme.muted }
    ActionButton { text: qsTr("Cambiar carpeta de juegos"); enabled: !page.controller.busy; onClicked: page.chooseFolder() }
    ActionButton { text: qsTr("Abrir Dashboard Metro en el motor"); enabled: !page.controller.players.busy; onClicked: page.controller.metro.start() }
    Switch { text: qsTr("Reducir movimiento"); checked: page.controller.reducedMotion; onToggled: page.controller.reducedMotion = checked }
    Label { text: qsTr("Vista de la biblioteca"); color: Theme.text; font.pointSize: Theme.heading }
    RowLayout {
        spacing: 10
        ActionButton {
            objectName: "libraryGridOption"
            text: qsTr("Cuadrícula")
            primary: page.controller.libraryView === "grid"
            onClicked: page.controller.libraryView = "grid"
        }
        ActionButton {
            objectName: "libraryCarouselOption"
            text: qsTr("Carrusel")
            primary: page.controller.libraryView === "carousel"
            onClicked: page.controller.libraryView = "carousel"
        }
    }
    Label { Layout.fillWidth: true; text: qsTr("El carrusel muestra carátulas grandes y recorre la colección de forma circular con izquierda y derecha."); color: Theme.muted; wrapMode: Text.Wrap }
    Label { text: qsTr("Intro al iniciar"); color: Theme.text; font.pointSize: Theme.heading }
    Label { Layout.fillWidth: true; text: qsTr("Elige una de las dos intros de 6 segundos. La biblioteca se carga mientras se reproduce."); color: Theme.muted; wrapMode: Text.Wrap }
    RowLayout {
        Layout.fillWidth: true
        spacing: 14
        Repeater {
            model: [{key: "aurora", label: qsTr("Aurora · azul glacial")}, {key: "nova", label: qsTr("Nova · violeta y coral")}]
            delegate: ColumnLayout {
                id: option
                required property var modelData
                Layout.fillWidth: true
                spacing: 5
                Image {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 320
                    Layout.preferredHeight: width * 9 / 16
                    Layout.alignment: Qt.AlignHCenter
                    source: page.controller.introMediaUrl(option.modelData.key, true)
                    sourceSize: Qt.size(640, 360)
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }
                ActionButton {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 320
                    Layout.alignment: Qt.AlignHCenter
                    text: option.modelData.label
                    primary: page.controller.introStyle === option.modelData.key
                    onClicked: page.controller.introStyle = option.modelData.key
                }
            }
        }
    }
    ActionButton { text: qsTr("Ver intro seleccionada"); onClicked: preview.open() }
    Label { text: "Emulos360 · Orbit 360"; color: Theme.accent }
    Label { Layout.fillWidth: true; text: qsTr("Interfaz nativa Qt Quick. Motor basado en Xenia Canary. Los ajustes de este apartado se guardan automáticamente."); wrapMode: Text.Wrap; color: Theme.muted }
    Item { Layout.fillHeight: true }
    MediaPlayer {
        id: previewPlayer
        source: page.controller.introVideo
        videoOutput: previewPicture
        audioOutput: AudioOutput { volume: 0.48 }
    }
    Dialog {
        id: preview
        title: qsTr("Vista previa de la intro")
        anchors.centerIn: Overlay.overlay
        width: Math.min(750, Math.max(320, (Overlay.overlay ? Overlay.overlay.width : page.width) - 48))
        height: width * 9 / 16 + 100
        modal: true
        standardButtons: Dialog.Close
        onOpened: previewPlayer.play()
        onClosed: previewPlayer.stop()
        contentItem: VideoOutput { id: previewPicture; fillMode: VideoOutput.PreserveAspectFit }
    }
}

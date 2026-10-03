pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

ScrollView {
    id: page
    required property LibraryController controller
    readonly property CoreOptions cores: controller.players.cores
    clip: true
    ColumnLayout {
        width: page.availableWidth
        spacing: 14
        Label { text: qsTr("Core de emulación"); color: Theme.text; font.pointSize: Theme.heading }
        Label { Layout.fillWidth: true; text: qsTr("Elige el motor para los próximos juegos. También puedes cambiarlo para un título desde Biblioteca → Motor."); color: Theme.muted; wrapMode: Text.Wrap }
        Label { text: qsTr("Core predeterminado"); color: Theme.text }
        AppComboBox {
            objectName: "defaultCoreCombo"
            Layout.fillWidth: true
            model: page.cores.choices
            textRole: "label"
            valueRole: "value"
            currentIndex: page.cores.choices.findIndex(row => row.value === page.cores.defaultCore)
            enabled: !page.cores.locked
            Accessible.name: qsTr("Core predeterminado")
            onActivated: page.cores.defaultCore = currentValue
        }
        Label { text: qsTr("Motor gráfico de este core"); color: Theme.text }
        AppComboBox {
            objectName: "defaultRendererCombo"
            Layout.fillWidth: true
            model: page.cores.rendererChoices
            textRole: "label"
            valueRole: "value"
            currentIndex: page.cores.rendererChoices.findIndex(row => row.value === page.cores.defaultRenderer)
            enabled: !page.cores.locked
            Accessible.name: qsTr("Motor gráfico predeterminado")
            onActivated: page.cores.defaultRenderer = currentValue
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Canary Edge es experimental y se abre en su propia ventana. Al iniciar un título por primera vez copia el perfil, sus partidas y el contenido instalado a su almacenamiento independiente. Sus avances y logros quedan en Edge; no se sincronizan con el core actual.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Netplay, MultiP, avatares y el dashboard de Emulos360 utilizan el core actual. Los ajustes de las demás secciones corresponden a ese core; Edge tiene sus propios ajustes.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Flow {
            Layout.fillWidth: true
            spacing: 8
            ActionButton { objectName: "openEdgeSettings"; text: qsTr("Abrir ajustes de Edge"); enabled: !page.controller.players.busy; onClicked: page.controller.players.openEdgeSettings() }
            ActionButton { text: qsTr("Carpeta de Edge"); onClicked: page.cores.openEdgeFolder() }
            ActionButton { text: qsTr("Partidas de Edge"); onClicked: page.cores.openEdgeContent() }
        }
        Label { Layout.fillWidth: true; text: page.cores.status; color: Theme.muted; wrapMode: Text.Wrap }
    }
}

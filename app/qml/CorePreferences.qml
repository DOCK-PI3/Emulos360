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
        Label { text: qsTr("Motor gráfico"); color: Theme.text; font.pointSize: Theme.heading }
        Label { Layout.fillWidth: true; text: qsTr("Xenia Canary es el motor de emulación. Puedes elegir una GPU global o configurarla por juego desde Biblioteca → Gráficos."); color: Theme.muted; wrapMode: Text.Wrap }
        Label { text: qsTr("Motor gráfico predeterminado"); color: Theme.text }
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
        Label { Layout.fillWidth: true; text: page.cores.status; color: Theme.muted; wrapMode: Text.Wrap }
    }
}

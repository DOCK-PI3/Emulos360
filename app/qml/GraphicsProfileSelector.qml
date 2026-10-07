pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

ColumnLayout {
    id: selector
    required property CoreOptions cores
    spacing: 8
    Label { text: qsTr("Perfil gráfico global"); color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
    AppComboBox {
        objectName: "globalGraphicsProfileCombo"
        Layout.fillWidth: true
        model: selector.cores.graphicsProfileChoices
        textRole: "label"
        valueRole: "value"
        currentIndex: selector.cores.graphicsProfileChoices.findIndex(row => row.value === selector.cores.defaultGraphicsProfile)
        enabled: !selector.cores.locked
        Accessible.name: qsTr("Perfil gráfico global")
        onActivated: selector.cores.defaultGraphicsProfile = currentValue
    }
    Label { Layout.fillWidth: true; text: selector.cores.graphicsProfileDescription(selector.cores.defaultGraphicsProfile); color: Theme.muted; wrapMode: Text.Wrap }
    Label { Layout.fillWidth: true; text: qsTr("Se guarda al seleccionar y se aplica al iniciar. Cada juego puede elegir otro perfil en Biblioteca → Gráficos. Personalizado permite usar los ajustes manuales."); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
    Label { visible: text.length > 0; Layout.fillWidth: true; text: selector.cores.status; color: Theme.accent; wrapMode: Text.Wrap; font.pixelSize: 12 }
    Label { Layout.fillWidth: true; text: qsTr("Resoluciones orientativas para juegos de 720p. La salida se adapta a la ventana o monitor; FSR funciona también con NVIDIA."); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 12 }
}

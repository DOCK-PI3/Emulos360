import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

ScrollView {
    id: page
    objectName: "compatibilityLegend"
    required property LibraryController controller
    contentWidth: availableWidth
    clip: true
    ColumnLayout {
        width: page.availableWidth
        spacing: 14
        Label { text: qsTr("Compatibilidad de los juegos"); color: Theme.text; font.pixelSize: 23 }
        Label {
            Layout.fillWidth: true
            text: qsTr("Estos iconos aparecen junto a cada título de la tienda y al pie de las carátulas. Pasa el ratón sobre el icono para ver el estado y los fallos reportados.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Repeater {
            model: page.controller.compatibility.legend
            delegate: RowLayout {
                id: legendRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 16
                CompatibilityBadge { report: legendRow.modelData; Layout.preferredWidth: 145 }
                Label { Layout.fillWidth: true; text: legendRow.modelData.description; color: Theme.text; wrapMode: Text.Wrap }
            }
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Fuente: informes comunitarios de Xenia Canary recopilados por Xenia Manager. No son pruebas específicas de Emulos360: el resultado puede variar con Windows/Linux, tu GPU, los ajustes y la versión del motor. «En juego» no confirma que el juego pueda terminarse.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("En la biblioteca se usa el identificador del juego. En la tienda se compara el nombre; las ediciones ambiguas muestran «Sin datos». El catálogo incluido permite consultar los estados sin conexión.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Label { Layout.fillWidth: true; text: page.controller.compatibility.status; color: Theme.accent; wrapMode: Text.Wrap }
        RowLayout {
            ActionButton {
                text: qsTr("Actualizar informes")
                enabled: !page.controller.compatibility.busy
                onClicked: page.controller.compatibility.refresh()
            }
            ActionButton {
                text: qsTr("Abrir catálogo de compatibilidad")
                onClicked: Qt.openUrlExternally("https://xenia-manager.github.io/compatibility")
            }
        }
    }
}

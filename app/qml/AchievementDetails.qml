pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
FocusScope {
    id: detail
    property var game: null
    readonly property var entries: game ? game.achievements.filter(a => filter.currentIndex === 0 || (filter.currentIndex === 1 ? a.unlocked : !a.unlocked)) : []
    onGameChanged: { filter.currentIndex = 0; list.positionViewAtBeginning() }
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        Label {
            Layout.fillWidth: true
            text: detail.game ? detail.game.title : qsTr("Tu progreso, juego a juego")
            textFormat: Text.PlainText; color: Theme.text; font.pointSize: Theme.heading; wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: detail.game ? qsTr("%1 de %2 logros · %3 / %4 G").arg(detail.game.unlocked).arg(detail.game.total).arg(detail.game.points).arg(detail.game.totalPoints) : qsTr("Selecciona un juego para ver sus logros.")
            color: Theme.accent; wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true; visible: !!detail.game
            AppComboBox {
                id: filter
                objectName: "achievementFilter"
                Layout.fillWidth: true
                model: [qsTr("Todos los logros"), qsTr("Desbloqueados"), qsTr("Pendientes")]
                Accessible.name: qsTr("Filtrar logros")
            }
            CheckBox { id: secrets; text: qsTr("Mostrar secretos"); checked: false }
        }
        ListView {
            id: list
            objectName: "achievementEntries"
            Layout.fillWidth: true; Layout.fillHeight: true
            model: detail.entries; clip: true; spacing: 10
            activeFocusOnTab: true; keyNavigationEnabled: true; focus: true
            delegate: ItemDelegate {
                id: achievement
                required property var modelData
                required property int index
                readonly property bool concealed: modelData.secret && !modelData.unlocked && !secrets.checked
                width: list.width; implicitHeight: Math.max(106, body.implicitHeight + 28)
                onClicked: { list.currentIndex = index; list.forceActiveFocus() }
                Accessible.name: (concealed ? qsTr("Logro secreto") : modelData.name) + ", " + (modelData.unlocked ? qsTr("Desbloqueado") : qsTr("Pendiente"))
                background: Rectangle { radius: 8; color: Theme.panel; border.color: achievement.activeFocus || (list.activeFocus && list.currentIndex === achievement.index) ? Theme.accent : Theme.outline }
                contentItem: RowLayout {
                    id: body
                    spacing: 14
                    Rectangle {
                        Layout.preferredWidth: 56; Layout.preferredHeight: 56; radius: 8; color: Theme.raised
                        Label { anchors.centerIn: parent; text: achievement.concealed ? "?" : "G"; color: achievement.modelData.unlocked ? Theme.accent : Theme.muted; font.pointSize: Theme.heading }
                        Image { anchors.fill: parent; anchors.margins: 3; source: achievement.concealed ? "" : achievement.modelData.icon; visible: status === Image.Ready; sourceSize: Qt.size(64, 64); fillMode: Image.PreserveAspectFit; asynchronous: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 0; spacing: 6
                        Label { Layout.fillWidth: true; text: achievement.concealed ? qsTr("Logro secreto") : achievement.modelData.name; textFormat: Text.PlainText; color: Theme.text; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                        Label { Layout.fillWidth: true; text: achievement.concealed ? qsTr("Sigue jugando para descubrir este logro.") : achievement.modelData.description; textFormat: Text.PlainText; color: Theme.muted; wrapMode: Text.Wrap }
                        Label { Layout.fillWidth: true; text: achievement.modelData.unlocked ? achievement.modelData.date.length ? qsTr("Desbloqueado · %1").arg(achievement.modelData.date) : qsTr("Desbloqueado") : qsTr("Pendiente"); color: achievement.modelData.unlocked ? Theme.accent : Theme.muted; wrapMode: Text.Wrap }
                    }
                    Label { text: qsTr("%1 G").arg(achievement.modelData.points); color: Theme.text; font.pointSize: Theme.heading }
                }
            }
            ScrollBar.vertical: ScrollBar {}
            Label {
                anchors.centerIn: parent; width: parent.width * 0.85
                visible: !!detail.game && list.count === 0
                text: detail.game && !detail.game.detailsAvailable ? qsTr("El perfil conserva el progreso, pero no se pudo leer el detalle de este juego.") : qsTr("No hay logros en este filtro.")
                color: Theme.muted; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}

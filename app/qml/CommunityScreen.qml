pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los
ColumnLayout {
    id: page
    required property PlayerServices players
    required property EngineSettings settings
    required property LibraryController controller
    spacing: 16
    Label { text: qsTr("Comunidad"); color: Theme.text; font.pixelSize: 28; font.weight: Font.DemiBold }
    Label { Layout.fillWidth: true; text: qsTr("Paquetes de servicios y aplicaciones de Xbox 360."); color: Theme.muted; wrapMode: Text.Wrap }
    ScrollView {
        id: scroll
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 18
            PartyPanel { Layout.fillWidth: true; controller: page.controller }
            RoomsPanel { Layout.fillWidth: true; Layout.preferredHeight: 540; controller: page.controller }
            Pane {
                Layout.fillWidth: true
                padding: 20
                background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.accent }
                contentItem: ColumnLayout {
                    spacing: 14
                    Label { text: "Xenia Canary Netplay"; color: Theme.text; font.pixelSize: 24; font.weight: Font.DemiBold }
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("Crea o busca partidas desde el menú multijugador del juego. El gestor del motor permite configurar el servidor, la interfaz de red, los perfiles y los amigos.") }
                    AppComboBox {
                        id: networkMode
                        Layout.fillWidth: true
                        model: [qsTr("Sin conexión"), qsTr("System Link · LAN / VPN"), qsTr("Netplay · servicio comunitario")]
                        currentIndex: Number(page.settings.values["Live.network_mode"] || 0)
                        enabled: !page.players.busy
                        Accessible.name: qsTr("Modo de red")
                        onActivated: page.settings.setValue("Live.network_mode", String(currentIndex))
                    }
                    RowLayout {
                        ActionButton { text: qsTr("Guardar modo"); enabled: page.settings.dirty && !page.players.busy; onClicked: { if (page.settings.save()) page.players.refreshProfiles() } }
                        ActionButton { text: qsTr("Abrir gestor Netplay"); primary: true; enabled: !page.players.busy && !page.settings.dirty; onClicked: page.players.launchNetplay() }
                    }
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.accent; text: qsTr("Netplay → Manager → Friends permite gestionar amigos. Para entrar en una partida se usa el menú del juego. Los participantes deben usar versiones compatibles del título.") }
                }
            }
            Pane {
                Layout.fillWidth: true
                padding: 20
                background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.outline }
                contentItem: ColumnLayout {
                    spacing: 14
                    Label { text: qsTr("Aplicaciones XEX independientes"); color: Theme.text; font.pixelSize: 18 }
                    Label { Layout.fillWidth: true; text: page.players.communityXex || qsTr("Selecciona una aplicación; los módulos DLL necesitan su propio cargador."); color: Theme.muted; wrapMode: Text.WrapAnywhere }
                    RowLayout {
                        ActionButton { text: qsTr("Elegir .xex"); onClicked: client.open() }
                        ActionButton { text: qsTr("Abrir aplicación"); enabled: !page.players.busy && page.players.communityXex.length > 0; onClicked: page.players.launchCommunity() }
                    }
                }
            }
        }
    }
    FileDialog { id: client; title: qsTr("Aplicación Xbox 360"); nameFilters: ["Xbox 360 (*.xex)"]; onAccepted: page.players.setCommunityXex(selectedFile) }
}

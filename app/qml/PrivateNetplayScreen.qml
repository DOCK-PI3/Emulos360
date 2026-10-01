pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ColumnLayout {
    id: page
    required property LibraryController controller
    readonly property PrivateNetplay service: controller.privateNetplay
    readonly property bool editable: !service.running && !controller.players.busy && !controller.engineSettings.dirty
    function prepare(): bool {
        if (!editable) return false
        if (!controller.engineSettings.setValue("Live.network_mode", "2") || !controller.engineSettings.save()) return false
        controller.players.refreshProfiles()
        return true
    }
    spacing: 12
    Label { text: qsTr("Multijugador privado"); color: Theme.text; font.pixelSize: 28; font.weight: Font.DemiBold }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("Tu grupo de amigos, sus salas y un servidor gestionado desde Emulos360.") }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: page.service.connected ? Theme.accent : Theme.muted; text: page.service.status; textFormat: Text.PlainText }
    ScrollView {
        id: scroll
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 16
            Pane {
                Layout.fillWidth: true
                visible: !page.service.running
                padding: 18
                background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
                contentItem: ColumnLayout {
                    spacing: 12
                    TabBar {
                        id: mode
                        Layout.fillWidth: true
                        TabButton { text: qsTr("Crear servidor · anfitrión") }
                        TabButton { text: qsTr("Unirse · participante") }
                    }
                    Label { text: mode.currentIndex === 0 ? qsTr("Nombre del grupo") : qsTr("Tu nombre"); color: Theme.text }
                    TextField { id: name; Layout.fillWidth: true; text: qsTr("Amigos de Emulos360"); maximumLength: 64; Accessible.name: qsTr("Nombre") }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: mode.currentIndex === 0
                        Label { text: qsTr("IP o dominio que usarán tus amigos"); color: Theme.text }
                        TextField { id: hostAddress; Layout.fillWidth: true; text: page.service.localAddress; placeholderText: qsTr("IP pública, dominio o IP de la VPN"); maximumLength: 253; Accessible.name: qsTr("Dirección del anfitrión") }
                        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("La IP local sirve dentro de tu red. Para Internet introduce tu IP pública o un dominio; para una VPN, su dirección IPv4.") }
                        RowLayout {
                            Label { text: qsTr("Puerto TCP"); color: Theme.text }
                            SpinBox { id: hostPort; from: 1024; to: 65535; value: 36000; editable: true; Accessible.name: qsTr("Puerto del servidor") }
                            CheckBox { id: upnp; text: qsTr("Abrir este puerto con UPnP"); checked: false }
                        }
                        ActionButton { text: qsTr("Iniciar servidor privado"); primary: true; enabled: page.editable; onClicked: { if (page.prepare()) page.service.host(name.text, hostPort.value, hostAddress.text.trim(), upnp.checked) } }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: mode.currentIndex === 1
                        Label { text: qsTr("Invitación recibida del anfitrión"); color: Theme.text }
                        TextArea { id: invitation; Layout.fillWidth: true; Layout.preferredHeight: 85; placeholderText: qsTr("Pega aquí emulos360://join/…"); wrapMode: TextEdit.WrapAnywhere; Accessible.name: qsTr("Invitación privada") }
                        Label { text: qsTr("Cambiar dirección de conexión · opcional"); color: Theme.muted }
                        RowLayout {
                            TextField { id: clientAddress; Layout.fillWidth: true; placeholderText: qsTr("IP o dominio; vacío usa la invitación"); maximumLength: 253; Accessible.name: qsTr("IP del servidor") }
                            SpinBox { id: clientPort; from: 1; to: 65535; value: 36000; editable: true; Accessible.name: qsTr("Puerto de conexión") }
                        }
                        ActionButton { text: qsTr("Unirse al grupo"); primary: true; enabled: page.editable && invitation.text.trim().length > 0; onClicked: { if (page.prepare()) page.service.join(invitation.text.trim(), name.text, clientAddress.text.trim(), clientPort.value) } }
                    }
                    Label { Layout.fillWidth: true; visible: !page.editable; text: qsTr("Cierra el motor y guarda o descarta los ajustes pendientes antes de conectar."); color: Theme.muted; wrapMode: Text.Wrap }
                }
            }
            Pane {
                Layout.fillWidth: true
                visible: page.service.running
                padding: 18
                background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.accent }
                contentItem: ColumnLayout {
                    spacing: 12
                    Label { text: page.service.hosting ? qsTr("Panel del anfitrión") : qsTr("Panel del participante"); color: Theme.text; font.pixelSize: 22 }
                    Label { Layout.fillWidth: true; text: page.service.endpoint; textFormat: Text.PlainText; color: Theme.accent; wrapMode: Text.WrapAnywhere }
                    Label { Layout.fillWidth: true; text: qsTr("Salas: %1 · jugadores en salas: %2 · tiempo activo: %3 s").arg(page.service.metrics.rooms ?? 0).arg(page.service.metrics.players ?? qsTr("privado")).arg(page.service.metrics.uptime ?? 0); color: Theme.text; wrapMode: Text.Wrap }
                    Label { Layout.fillWidth: true; text: qsTr("Respuesta del servicio: %1 ms · variación: %2 ms. Esta medición corresponde al servidor de salas.").arg(page.service.metrics.latencyMs ?? 0).arg(page.service.metrics.jitterMs ?? 0); color: Theme.muted; wrapMode: Text.Wrap }
                    Label { Layout.fillWidth: true; visible: page.service.hosting; text: qsTr("Peticiones: %1 · errores: %2 · memoria del servicio: %3 MB · espera del proceso: %4 ms").arg(page.service.metrics.requests ?? 0).arg(page.service.metrics.errors ?? 0).arg(page.service.metrics.memoryMB ?? 0).arg(page.service.metrics.eventLoopMs ?? 0); color: Theme.muted; wrapMode: Text.Wrap }
                    Label { Layout.fillWidth: true; visible: page.service.hosting; text: page.service.metrics.upnp ?? ""; textFormat: Text.PlainText; color: Theme.muted; wrapMode: Text.Wrap }
                    RowLayout {
                        ActionButton { text: qsTr("Actualizar"); enabled: !page.service.busy; onClicked: page.service.refresh() }
                        ActionButton { text: qsTr("Panel web"); enabled: page.service.connected; onClicked: page.service.openPanel() }
                        ActionButton { text: page.service.hosting ? qsTr("Detener servidor") : qsTr("Desconectar"); enabled: !page.controller.players.busy; onClicked: stopDialog.open() }
                    }
                    Label { Layout.fillWidth: true; visible: page.service.hosting; text: qsTr("El panel web usa el certificado incluido en las invitaciones. El navegador puede pedir que aceptes ese certificado local."); color: Theme.muted; wrapMode: Text.Wrap }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: page.service.hosting && page.service.connected
                        Label { text: qsTr("Invitar a un amigo"); color: Theme.text; font.pixelSize: 18 }
                        RowLayout {
                            TextField { id: inviteName; Layout.fillWidth: true; placeholderText: qsTr("Nombre de la persona"); maximumLength: 64; Accessible.name: qsTr("Persona invitada") }
                            SpinBox { id: duration; from: 1; to: 720; value: 168; editable: true; Accessible.name: qsTr("Duración en horas") }
                            ActionButton { text: qsTr("Crear invitación"); onClicked: page.service.createInvitation(inviteName.text, duration.value) }
                        }
                        Label { text: qsTr("Duración en horas. Cada invitación se puede revocar individualmente."); color: Theme.muted }
                        ActionButton { text: qsTr("Copiar invitación"); primary: true; enabled: page.service.invitation.length > 0; onClicked: page.service.copyInvitation() }
                        CheckBox { text: qsTr("Compartir estadísticas y lista de participantes con el grupo"); checked: page.service.metrics.shareStats ?? true; onClicked: page.service.shareStats(checked) }
                        Repeater {
                            model: page.service.metrics.invitations ?? []
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                Label { Layout.fillWidth: true; text: modelData.label; textFormat: Text.PlainText; color: Theme.text; elide: Text.ElideRight }
                                Label { text: modelData.revoked ? qsTr("Revocada") : qsTr("Caduca: %1").arg(new Date(modelData.expires).toLocaleString()); color: Theme.muted }
                                ActionButton { text: qsTr("Revocar"); enabled: !modelData.revoked; onClicked: page.service.revoke(modelData.id) }
                            }
                        }
                        RowLayout {
                            ActionButton { text: qsTr("Limpiar salas caducadas"); onClicked: page.service.cleanup() }
                            ActionButton { text: qsTr("Detectar IP pública"); onClicked: page.service.findPublicAddress() }
                        }
                    }
                    Repeater {
                        model: page.service.metrics.participants ?? []
                        delegate: Label {
                            required property var modelData
                            Layout.fillWidth: true
                            text: qsTr("Conectado: %1").arg(modelData.name); textFormat: Text.PlainText; color: Theme.text
                        }
                    }
                    Label { visible: (page.service.metrics.leaderboards ?? []).length > 0; text: qsTr("Estadísticas enviadas por los juegos"); color: Theme.text; font.pixelSize: 18 }
                    Repeater {
                        model: page.service.metrics.leaderboards ?? []
                        delegate: Label {
                            required property var modelData
                            Layout.fillWidth: true
                            text: qsTr("%1 · juego %2 · clasificación %3\n%4").arg(modelData.player).arg(modelData.titleId).arg(modelData.board).arg(modelData.values)
                            textFormat: Text.PlainText; color: Theme.muted; wrapMode: Text.WrapAnywhere
                        }
                    }
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("Revocar una invitación corta su acceso al servicio. Las conexiones directas de una partida en curso dependen del propio juego.") }
                }
            }
            RoomsPanel { Layout.fillWidth: true; Layout.preferredHeight: 480; visible: page.service.connected; controller: page.controller }
            Pane {
                Layout.fillWidth: true
                padding: 18
                background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.outline }
                contentItem: ColumnLayout {
                    Label { text: qsTr("Preparar la partida"); color: Theme.text; font.pixelSize: 18 }
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("1. Selecciona o crea un perfil Netplay en Perfiles y avatares.\n2. Todos deben conectar al mismo servidor privado.\n3. Abre el juego y crea o busca la partida en su menú multijugador.\n4. Usa la misma versión, actualización y contenido del juego.") }
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted; text: qsTr("Para Internet, el anfitrión abre el puerto TCP del servicio. Los juegos pueden necesitar otros puertos UDP/TCP; activa UPnP en los ajustes de Xenia o configura los puertos del título. Si hay CGNAT, usa una VPN privada con tus amigos y su dirección IPv4. El servidor de salas conserva el transporte multijugador de cada juego.") }
                }
            }
        }
    }
    Dialog {
        id: stopDialog
        title: page.service.hosting ? qsTr("Detener servidor privado") : qsTr("Salir del grupo")
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 480
        standardButtons: Dialog.Yes | Dialog.No
        onOpened: standardButton(Dialog.No).forceActiveFocus()
        onAccepted: page.service.stop()
        contentItem: Label { text: qsTr("Se cerrará la conexión al servidor privado. ¿Continuar?"); color: Theme.text; wrapMode: Text.Wrap }
    }
}

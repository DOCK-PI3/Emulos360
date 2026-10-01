pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

Pane {
    id: panel
    required property LibraryController controller
    readonly property VoiceParty party: controller.voiceParty
    function selectedName(): string {
        for (const profile of controller.players.profiles)
            if (profile.xuid === controller.players.activeXuid) return profile.gamertag
        return qsTr("Jugador")
    }
    padding: 20
    background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.accent }
    contentItem: ColumnLayout {
        spacing: 12
        Label {
            Layout.fillWidth: true
            text: qsTr("Party de voz")
            font.pixelSize: 24
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Voz Opus y chat cifrados por WebRTC. Un jugador aloja la sala desde Emulos360; no se usa un servidor externo.")
            wrapMode: Text.Wrap
            color: Theme.muted
        }
        Label { Layout.fillWidth: true; text: panel.party.status; wrapMode: Text.Wrap; color: Theme.accent }

        RowLayout {
            Layout.fillWidth: true
            visible: !panel.party.active
            Label { text: qsTr("Tu nombre"); color: Theme.text }
            TextField {
                id: nameField
                Layout.fillWidth: true
                text: panel.selectedName()
                maximumLength: 24
                color: Theme.text
                background: Rectangle { color: Theme.raised; radius: 8; border.color: nameField.activeFocus ? Theme.accent : Theme.outline }
            }
            Label { text: qsTr("Puerto"); color: Theme.text }
            TextField {
                id: portField
                Layout.preferredWidth: 88
                text: "46580"
                inputMethodHints: Qt.ImhDigitsOnly
                color: Theme.text
                background: Rectangle { color: Theme.raised; radius: 8; border.color: portField.activeFocus ? Theme.accent : Theme.outline }
            }
            ActionButton { text: qsTr("Crear sala"); primary: true; onClicked: panel.party.host(nameField.text, Number(portField.text)) }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: !panel.party.active
            TextField {
                id: addressField
                Layout.fillWidth: true
                placeholderText: qsTr("IP local o pública del anfitrión")
                placeholderTextColor: Theme.muted
                color: Theme.text
                background: Rectangle { color: Theme.raised; radius: 8; border.color: addressField.activeFocus ? Theme.accent : Theme.outline }
            }
            TextField {
                id: codeField
                Layout.preferredWidth: 120
                placeholderText: qsTr("Código de 8 cifras")
                placeholderTextColor: Theme.muted
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 8
                color: Theme.text
                background: Rectangle { color: Theme.raised; radius: 8; border.color: codeField.activeFocus ? Theme.accent : Theme.outline }
            }
            ActionButton { text: qsTr("Unirse"); onClicked: panel.party.join(addressField.text, Number(portField.text), codeField.text, nameField.text) }
        }
        Label {
            Layout.fillWidth: true
            visible: !panel.party.active
            text: qsTr("Salas detectadas en tu red local:")
            color: Theme.text
        }
        Repeater {
            model: panel.party.active ? [] : panel.party.nearby
            delegate: ActionButton {
                required property var modelData
                Layout.fillWidth: true
                text: qsTr("%1 · %2:%3 · %4 jugadores").arg(modelData.name).arg(modelData.address).arg(modelData.port).arg(modelData.players)
                onClicked: { addressField.text = modelData.address; portField.text = String(modelData.port); codeField.forceActiveFocus() }
            }
        }
        Label {
            Layout.fillWidth: true
            visible: panel.party.active && panel.party.hosting
            text: qsTr("Tu sala: %1 · puerto TCP %2 · código %3").arg(panel.party.localAddresses).arg(panel.party.port).arg(panel.party.roomCode)
            wrapMode: Text.Wrap
            color: Theme.accent
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            visible: panel.party.active
            text: qsTr("Participantes: %1").arg(panel.party.participants.join(", "))
            wrapMode: Text.Wrap
            color: Theme.text
        }
        RowLayout {
            Layout.fillWidth: true
            visible: panel.party.active
            Switch { text: qsTr("Silenciar micrófono"); checked: panel.party.muted; onToggled: panel.party.muted = checked }
            Item { Layout.fillWidth: true }
            ActionButton { text: qsTr("Salir de la party"); onClicked: panel.party.leave() }
        }
        ListView {
            id: chatList
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            visible: panel.party.active
            clip: true
            model: panel.party.messages
            spacing: 4
            onCountChanged: positionViewAtEnd()
            delegate: Label {
                required property var modelData
                width: chatList.width
                text: modelData.name + ": " + modelData.text
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.text
            }
            ScrollBar.vertical: ScrollBar {}
        }
        RowLayout {
            Layout.fillWidth: true
            visible: panel.party.active
            TextField {
                id: messageField
                Layout.fillWidth: true
                placeholderText: qsTr("Mensaje a la party")
                placeholderTextColor: Theme.muted
                maximumLength: 400
                color: Theme.text
                background: Rectangle { color: Theme.raised; radius: 8; border.color: messageField.activeFocus ? Theme.accent : Theme.outline }
                onAccepted: { panel.party.sendText(text); clear() }
            }
            ActionButton { text: qsTr("Enviar"); onClicked: { panel.party.sendText(messageField.text); messageField.clear() } }
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Por Internet, el anfitrión debe redirigir el puerto TCP elegido y los UDP 46581–46600 hacia su PC. Con CGNAT o NAT restrictivo puede no haber conexión directa.")
            wrapMode: Text.Wrap
            color: Theme.muted
            font.pixelSize: 11
        }
    }
}

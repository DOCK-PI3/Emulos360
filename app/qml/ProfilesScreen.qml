pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Los
ColumnLayout {
    id: page
    required property PlayerServices players
    property string editingXuid: ""
    spacing: 16
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Perfiles y avatares"); font.pixelSize: 28; font.weight: Font.DemiBold; color: Theme.text }
        Item { Layout.fillWidth: true }
        ActionButton { text: qsTr("Actualizar perfiles"); enabled: !page.players.busy; onClicked: page.players.refreshProfiles() }
    }
    Label { Layout.fillWidth: true; text: qsTr("Una identidad para cada jugador. El perfil seleccionado iniciará sesión al abrir un juego."); color: Theme.muted; wrapMode: Text.Wrap }
    RowLayout {
        Layout.fillWidth: true
        TextField { id: gamertag; Layout.fillWidth: true; maximumLength: 15; placeholderText: qsTr("Nuevo gamertag (máximo 15 caracteres)"); placeholderTextColor: Theme.muted; color: Theme.text; onAccepted: page.players.createProfile(text) }
        ActionButton { text: qsTr("Crear perfil"); primary: true; enabled: !page.players.busy && gamertag.text.length > 0; onClicked: page.players.createProfile(gamertag.text) }
    }
    ListView {
        id: profiles
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 12
        clip: true
        model: page.players.profiles
        delegate: Rectangle {
            required property var modelData
            width: profiles.width - 14
            height: 108
            radius: 12
            color: Theme.panel
            border.color: modelData.xuid === page.players.activeXuid ? Theme.accent : Theme.outline
            RowLayout {
                anchors.fill: parent; anchors.margins: 16; spacing: 18
                Rectangle {
                    implicitWidth: 72; implicitHeight: 72; radius: 12; color: Theme.raised; clip: true
                    Label { anchors.centerIn: parent; text: modelData.gamertag.substring(0,1).toUpperCase(); font.pixelSize: 32; color: Theme.accent }
                    Image { anchors.fill: parent; source: modelData.avatar || ""; visible: status === Image.Ready; fillMode: Image.PreserveAspectCrop }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.gamertag; color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold }
                    Label { text: modelData.xuid === page.players.activeXuid ? qsTr("JUGADOR 1 · SELECCIONADO") : modelData.xuid; color: Theme.muted; font.pixelSize: 10 }
                    Label { text: modelData.netplay ? qsTr("Netplay habilitado") : qsTr("Perfil local · Netplay sin habilitar"); color: Theme.muted; font.pixelSize: 11 }
                }
                ActionButton { text: qsTr("Habilitar Netplay"); visible: !modelData.netplay; enabled: !page.players.busy; onClicked: page.players.enableNetplayProfile(modelData.xuid) }
                ActionButton { text: qsTr("Imagen de perfil"); onClicked: { page.editingXuid = modelData.xuid; avatar.open() } }
                ActionButton { text: qsTr("Avatar 3D"); enabled: !page.players.busy; onClicked: page.players.editAvatar(modelData.xuid) }
                ActionButton { text: modelData.xuid === page.players.activeXuid ? qsTr("Seleccionado") : qsTr("Seleccionar"); enabled: !page.players.busy; primary: modelData.xuid === page.players.activeXuid; onClicked: page.players.selectProfile(modelData.xuid) }
            }
        }
        ScrollBar.vertical: ScrollBar {}
        Label { anchors.centerIn: parent; visible: profiles.count === 0; text: qsTr("Crea un perfil o pulsa Actualizar para leer los existentes."); color: Theme.muted; wrapMode: Text.Wrap; width: parent.width * 0.8; horizontalAlignment: Text.AlignHCenter }
    }
    Label { Layout.fillWidth: true; text: qsTr("Avatar 3D abre el editor nativo con los modelos y animaciones originales y guarda el personaje en tu perfil. El renderizado de avatares dentro de los juegos todavía no está implementado. Las imágenes de perfil solo personalizan esta interfaz."); color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap }
    Label { Layout.fillWidth: true; text: qsTr("Habilitar Netplay conserva tus partidas y crea una copia del perfil. Es una identidad para el servicio comunitario; no requiere una cuenta Microsoft."); color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap }
    RowLayout {
        ActionButton { text: qsTr("Elegir editor 3D (.xex)"); onClicked: editor.open() }
        ActionButton { text: qsTr("Abrir XEX del editor (experimental)"); enabled: !page.players.busy && page.players.avatarEditorXex.length > 0; onClicked: page.players.launchAvatarEditor() }
    }
    Dialog {
        id: avatar
        anchors.centerIn: Overlay.overlay
        width: Math.min(480,page.width)
        title: qsTr("Imagen de perfil de Emulos360")
        modal: true
        standardButtons: Dialog.Close
        background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.outline }
        ColumnLayout {
            width: parent.width; spacing: 12
            Label { text: qsTr("Tono de piel"); color: Theme.text }
            AppComboBox { id: skin; Layout.fillWidth: true; model: [qsTr("Claro"),qsTr("Medio"),qsTr("Oscuro")]; property var colors: ["#f3c49d","#b77d54","#65422f"] }
            Label { text: qsTr("Color de ropa"); color: Theme.text }
            AppComboBox { id: shirt; Layout.fillWidth: true; model: [qsTr("Verde"),qsTr("Azul"),qsTr("Rojo"),qsTr("Violeta")]; property var colors: ["#a4d94e","#4a8bde","#d95151","#a07be8"] }
            AppComboBox { id: style; Layout.fillWidth: true; model: [qsTr("Pelo corto"),qsTr("Gorra"),qsTr("Sin pelo")] }
            ActionButton { Layout.fillWidth: true; text: qsTr("Crear y guardar"); primary: true; onClicked: { page.players.createAvatar(page.editingXuid,skin.colors[skin.currentIndex],shirt.colors[shirt.currentIndex],style.currentIndex); avatar.close() } }
            ActionButton { Layout.fillWidth: true; text: qsTr("Usar una imagen propia…"); onClicked: image.open() }
        }
    }
    FileDialog { id: image; title: qsTr("Imagen del perfil"); nameFilters: ["Imágenes (*.png *.jpg *.jpeg *.webp)"]; onAccepted: { page.players.importAvatar(page.editingXuid,selectedFile); avatar.close() } }
    FileDialog { id: editor; title: qsTr("Editor de avatares Xbox 360"); nameFilters: ["Xbox 360 (*.xex)"]; onAccepted: page.players.setAvatarEditorXex(selectedFile) }
}

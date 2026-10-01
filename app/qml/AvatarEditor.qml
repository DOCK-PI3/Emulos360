pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick3D
import Los
Rectangle {
    id: editor
    required property PlayerServices players
    readonly property AvatarStudio studio: players.avatarStudio
    signal closeRequested()
    Component.onCompleted: studio.setPreviewActive(true)
    Component.onDestruction: studio.setPreviewActive(false)
    color: Theme.background
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        RowLayout {
            Layout.fillWidth: true
            ActionButton { text: qsTr("‹ Perfiles"); onClicked: { if (editor.studio.dirty) discard.open(); else editor.closeRequested() } }
            ColumnLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Tu avatar Xbox 360"); color: Theme.text; font.pixelSize: 26; font.weight: Font.DemiBold }
                Label { text: editor.studio.status; color: Theme.muted; font.pixelSize: 12 }
            }
            Item { Layout.fillWidth: true }
            ActionButton { text: qsTr("Guardar en el perfil"); primary: true; enabled: editor.studio.ready && editor.studio.profile.length > 0 && !editor.players.busy; onClicked: editor.studio.save() }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 24
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 400; radius: 18; color: "#18251f"
                View3D {
                    id: view
                    anchors.fill: parent
                    environment: SceneEnvironment { backgroundMode: SceneEnvironment.Transparent; antialiasingMode: SceneEnvironment.MSAA; antialiasingQuality: SceneEnvironment.High }
                    PerspectiveCamera { id: camera; position: Qt.vector3d(0, 78, 225); clipNear: 1; clipFar: 1200 }
                    camera: camera
                    DirectionalLight { eulerRotation: Qt.vector3d(-25,-30,0); brightness: 1.6; ambientColor: "#777777" }
                    DirectionalLight { eulerRotation: Qt.vector3d(-10,150,0); brightness: 0.7 }
                    Node {
                        id: figure
                        eulerRotation.y: turn.value
                        Repeater3D {
                            model: editor.studio.parts
                            delegate: Model {
                                id: partModel
                                required property var modelData
                                geometry: modelData.geometry
                                materials: AvatarMaterial { part: partModel.modelData; avatarColors: editor.studio.colors }
                            }
                        }
                    }
                }
                ColumnLayout {
                    anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20
                    Label { text: qsTr("Girar avatar"); color: Theme.muted; Layout.alignment: Qt.AlignHCenter }
                    Slider { id: turn; Layout.fillWidth: true; from: -180; to: 180; value: 0; Accessible.name: qsTr("Girar avatar") }
                }
                Label { anchors.centerIn: parent; visible: !editor.studio.ready; text: editor.studio.status; width: parent.width - 40; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.text }
            }
            ColumnLayout {
                Layout.minimumWidth: 320; Layout.maximumWidth: 320; Layout.preferredWidth: 320; Layout.fillHeight: true; spacing: 12
                Label { text: qsTr("PERSONALIZAR"); color: Theme.accent; font.pixelSize: 12; font.letterSpacing: 2 }
                AppComboBox { Layout.fillWidth: true; model: [qsTr("Cuerpo masculino"),qsTr("Cuerpo femenino")]; currentIndex: Math.max(0,editor.studio.body-1); onActivated: editor.studio.setBody(currentIndex+1) }
                AppComboBox {
                    Layout.fillWidth: true; visible: editor.studio.animated
                    model: editor.studio.animations; textRole: "name"; valueRole: "id"
                    currentIndex: {
                        const entries = editor.studio.animations
                        const selected = editor.studio.animationId
                        for (let i = 0; i < entries.length; ++i)
                            if (entries[i].id === selected) return i
                        return -1
                    }
                    onActivated: editor.studio.setAnimation(currentValue)
                    Accessible.name: qsTr("Animación original")
                }
                AppComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("Pelo"),qsTr("Camisetas y conjuntos"),qsTr("Pantalones"),qsTr("Calzado"),qsTr("Sombreros"),qsTr("Guantes"),qsTr("Gafas"),qsTr("Muñequeras"),qsTr("Pendientes"),qsTr("Anillos"),qsTr("Mentón"),qsTr("Nariz"),qsTr("Orejas"),qsTr("Boca"),qsTr("Ojos"),qsTr("Cejas"),qsTr("Barba"),qsTr("Maquillaje"),qsTr("Rasgos de piel")]
                    currentIndex: editor.studio.category; onActivated: editor.studio.category = currentIndex
                }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 4
                    model: editor.studio.choices
                    delegate: ItemDelegate {
                        required property var modelData
                        width: ListView.view.width - 12; text: modelData.name; highlighted: modelData.selected
                        onClicked: editor.studio.choose(modelData.id)
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
                Label { text: qsTr("Colores"); color: Theme.text }
                GridLayout {
                    columns: 3; Layout.fillWidth: true
                    Repeater {
                        model: [qsTr("Piel"),qsTr("Pelo"),qsTr("Labios"),qsTr("Iris"),qsTr("Cejas"),qsTr("Sombra de ojos"),qsTr("Barba"),qsTr("Rasgo 1"),qsTr("Rasgo 2")]
                        delegate: Button {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true; text: modelData; font.pixelSize: 10
                            background: Rectangle { color: editor.studio.colors[index] || "#888888"; radius: 8; border.color: Theme.outline }
                            onClicked: { colors.slot=index; colors.selectedColor=editor.studio.colors[index]; colors.open() }
                        }
                    }
                }
                Label { Layout.fillWidth: true; text: qsTr("La vista 3D usa los recursos originales. La compatibilidad de renderizado dentro de los juegos sigue en desarrollo."); color: Theme.muted; wrapMode: Text.Wrap; font.pixelSize: 11 }
            }
        }
    }
    ColorDialog { id: colors; property int slot: 0; onAccepted: editor.studio.setColor(slot,selectedColor) }
    Dialog { id: discard; anchors.centerIn: Overlay.overlay; title: qsTr("¿Salir sin guardar el avatar?"); modal: true; standardButtons: Dialog.Discard|Dialog.Cancel; onDiscarded: editor.closeRequested() }
}

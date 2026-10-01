import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

ColumnLayout {
    id: page
    objectName: "appPreferences"
    required property LibraryController controller
    signal chooseFolder()
    property string section: "preferences"
    spacing: 12

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        ActionButton {
            objectName: "appPreferencesTab"
            text: qsTr("Preferencias")
            primary: page.section === "preferences"
            onClicked: page.section = "preferences"
        }
        ActionButton {
            objectName: "appCompatibilityTab"
            text: qsTr("Compatibilidad")
            primary: page.section === "compatibility"
            onClicked: page.section = "compatibility"
        }
        ActionButton {
            objectName: "appAboutTab"
            text: qsTr("Acerca de")
            primary: page.section === "about"
            onClicked: page.section = "about"
        }
        Item { Layout.fillWidth: true }
    }

    Loader {
        Layout.fillWidth: true
        Layout.fillHeight: true
        sourceComponent: page.section === "about" ? aboutComponent : page.section === "compatibility" ? compatibilityComponent : appearanceComponent
    }

    Component {
        id: appearanceComponent
        ScrollView {
            clip: true
            AppAppearance {
                width: parent.width
                controller: page.controller
                onChooseFolder: page.chooseFolder()
            }
        }
    }
    Component { id: aboutComponent; AboutScreen { controller: page.controller } }
    Component { id: compatibilityComponent; CompatibilityLegend { controller: page.controller } }
}

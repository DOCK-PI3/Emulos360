pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
FocusScope {
    id: dashboard
    required property LibraryController controller
    signal chooseFolder()
    property string page: "home"
    readonly property var pages: [
        {key:"home", label:qsTr("inicio")}, {key:"library", label:qsTr("juegos")},
        {key:"achievements", label:qsTr("logros")},
        {key:"profiles", label:qsTr("perfiles")}, {key:"community", label:qsTr("comunidad")},
        {key:"saves", label:qsTr("almacenamiento")}, {key:"settings", label:qsTr("ajustes")}
    ]
    readonly property var featuredGame: controller.games.length ? controller.games[0] : null
    readonly property var activeProfile: controller.players.profiles.find(p => p.xuid === controller.players.activeXuid)
    function navigate(key) { page = key; guide.close(); content.forceActiveFocus() }
    function nextPage(delta) {
        const n = pages.findIndex(p => p.key === page)
        navigate(pages[(n + delta + pages.length) % pages.length].key)
    }
    function back() { if (guide.opened) guide.close(); else if (page !== "home") navigate("home"); else guide.open() }
    Component.onCompleted: forceActiveFocus()
    onVisibleChanged: if (visible) Qt.callLater(function() {
        if (dashboard.visible) dashboard.forceActiveFocus()
    })
    Keys.onEscapePressed: back()
    Shortcut { sequence: "Home"; enabled: dashboard.visible; onActivated: guide.opened ? guide.close() : guide.open() }
    Shortcut { sequence: "PgDown"; enabled: dashboard.visible; onActivated: dashboard.nextPage(1) }
    Shortcut { sequence: "PgUp"; enabled: dashboard.visible; onActivated: dashboard.nextPage(-1) }
    Rectangle { anchors.fill: parent; color: Theme.consoleBackground }
    Rectangle { width: parent.width * 0.75; height: width; radius: width / 2; x: parent.width * 0.64; y: -height * 0.55; color: "transparent"; border.width: 64; border.color: "#dbe6d8" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: Math.max(28, dashboard.width * 0.045); spacing: 24
        RowLayout {
            Layout.fillWidth: true
            Image { source: "qrc:/assets/emulos360.png"; sourceSize: Qt.size(44,44); Layout.preferredWidth: 44; Layout.preferredHeight: 44 }
            Label { text: "Emulos360"; color: Theme.consoleText; font.pointSize: 20; font.weight: Font.Light }
            Item { Layout.fillWidth: true }
            Button {
                text: dashboard.activeProfile ? dashboard.activeProfile.gamertag : qsTr("Elegir perfil")
                font.pointSize: 14
                onClicked: dashboard.navigate("profiles")
                contentItem: Label { text: parent.text; color: Theme.consoleText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.activeFocus ? "#c5dfa9" : Theme.consoleTile; radius: 3 }
            }
            Button { text: qsTr("Guía ☰"); font.pointSize: 14; onClicked: guide.open() }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: dashboard.width < 1200 ? 8 : 20
            Repeater {
                model: dashboard.pages
                delegate: Button {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.label
                    padding: 3
                    onClicked: dashboard.navigate(modelData.key)
                    contentItem: Label { text: parent.text; color: dashboard.page === modelData.key ? Theme.consoleGreen : Theme.consoleMuted; font.pointSize: dashboard.width < 1200 ? 15 : 20; font.weight: Font.Light; horizontalAlignment: Text.AlignHCenter }
                    background: Rectangle { color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 3; color: Theme.consoleGreen; visible: dashboard.page === modelData.key || parent.parent.activeFocus } }
                }
            }
        }
        Loader {
            id: content
            Layout.fillWidth: true; Layout.fillHeight: true
            sourceComponent: dashboard.page === "home" ? home : innerPage
        }
        RowLayout {
            Label { text: qsTr("A / Intro  Seleccionar     B / Esc  Volver     LB · RB  Secciones     Menú / Inicio  Guía"); color: Theme.consoleMuted; font.pointSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Button { text: qsTr("Escritorio · F11"); onClicked: dashboard.controller.consoleMode = false }
        }
        Label { Layout.fillWidth: true; text: dashboard.controller.players.status; color: Theme.consoleText; font.pointSize: 11; elide: Text.ElideRight }
    }
    Component {
        id: home
        ColumnLayout {
            spacing: 16
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
                ConsoleTile {
                    id: hero
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 620
                    text: dashboard.featuredGame ? dashboard.featuredGame.title : qsTr("Tu próxima partida")
                    subtitle: dashboard.featuredGame ? qsTr("Jugar · Xbox 360") : qsTr("Añade tu colección para empezar")
                    symbol: "360"; featured: true; reducedMotion: dashboard.controller.reducedMotion
                    artwork: dashboard.featuredGame ? dashboard.controller.coverImages[dashboard.featuredGame.titleId] || "" : ""
                    onClicked: {
                        if (!dashboard.featuredGame) dashboard.chooseFolder()
                        else if (!dashboard.controller.players.activeXuid) dashboard.navigate("profiles")
                        else if (dashboard.featuredGame.complete && !dashboard.controller.players.busy) dashboard.controller.players.launchGame(dashboard.featuredGame.path, dashboard.featuredGame.titleId)
                    }
                    Component.onCompleted: forceActiveFocus()
                    KeyNavigation.right: gamesTile
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 300; spacing: 16
                    ConsoleTile { id: gamesTile; Layout.fillWidth: true; Layout.fillHeight: true; text: qsTr("Mis juegos"); subtitle: qsTr("%1 títulos en tu colección").arg(dashboard.controller.games.length); symbol: "▦"; reducedMotion: dashboard.controller.reducedMotion; onClicked: dashboard.navigate("library"); KeyNavigation.left: hero; KeyNavigation.down: onlineTile }
                    ConsoleTile { id: onlineTile; Layout.fillWidth: true; Layout.fillHeight: true; text: qsTr("Jugar juntos"); subtitle: qsTr("Netplay · amigos · System Link"); symbol: "◎"; reducedMotion: dashboard.controller.reducedMotion; onClicked: dashboard.navigate("community"); KeyNavigation.left: hero; KeyNavigation.up: gamesTile }
                }
            }
            RowLayout {
                Layout.fillWidth: true; Layout.preferredHeight: 130; spacing: 16
                ConsoleTile { Layout.fillWidth: true; Layout.fillHeight: true; text: qsTr("Mi perfil"); subtitle: qsTr("Tu identidad y tu avatar"); reducedMotion: dashboard.controller.reducedMotion; onClicked: dashboard.navigate("profiles") }
                ConsoleTile { Layout.fillWidth: true; Layout.fillHeight: true; text: qsTr("Almacenamiento"); subtitle: qsTr("Disco interno · disco externo"); reducedMotion: dashboard.controller.reducedMotion; onClicked: dashboard.navigate("saves") }
                ConsoleTile { Layout.fillWidth: true; Layout.fillHeight: true; text: qsTr("Personalizar"); subtitle: qsTr("Pantalla, audio y mandos"); reducedMotion: dashboard.controller.reducedMotion; onClicked: dashboard.navigate("settings") }
            }
        }
    }
    Component {
        id: innerPage
        Pane {
            padding: 22
            background: Rectangle { color: Theme.background; radius: 4 }
            contentItem: Loader {
                sourceComponent: dashboard.page === "library" ? library : dashboard.page === "achievements" ? achievements : dashboard.page === "profiles" ? profiles : dashboard.page === "saves" ? saves : dashboard.page === "community" ? community : settings
            }
        }
    }
    Component { id: library; LibraryScreen { controller: dashboard.controller; onChooseFolder: dashboard.chooseFolder() } }
    Component { id: profiles; ProfilesScreen { players: dashboard.controller.players } }
    Component { id: achievements; AchievementsScreen { players: dashboard.controller.players; coverImages: dashboard.controller.coverImages } }
    Component { id: saves; SavesScreen { players: dashboard.controller.players; games: dashboard.controller.games } }
    Component { id: community; CommunityScreen { players: dashboard.controller.players; settings: dashboard.controller.engineSettings; controller: dashboard.controller } }
    Component { id: settings; SettingsScreen { controller: dashboard.controller } }
    Drawer {
        id: guide
        width: Math.min(460, dashboard.width * 0.6); height: dashboard.height; edge: Qt.LeftEdge; modal: true; padding: 30
        background: Rectangle { color: Theme.panel; Rectangle { anchors.right: parent.right; width: 6; height: parent.height; color: Theme.consoleGreen } }
        ColumnLayout {
            anchors.fill: parent; spacing: 18
            Label { text: qsTr("Guía Emulos360"); color: Theme.text; font.pointSize: 24 }
            Label { text: dashboard.activeProfile ? dashboard.activeProfile.gamertag : qsTr("Sin perfil seleccionado"); color: Theme.accent; font.pointSize: 14 }
            Repeater {
                model: dashboard.pages
                delegate: ActionButton { required property var modelData; Layout.fillWidth: true; text: modelData.label; onClicked: dashboard.navigate(modelData.key) }
            }
            Item { Layout.fillHeight: true }
            ActionButton { Layout.fillWidth: true; text: qsTr("Cerrar guía"); onClicked: guide.close() }
            ActionButton { Layout.fillWidth: true; text: qsTr("Volver al escritorio"); onClicked: dashboard.controller.consoleMode = false }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los
ColumnLayout {
    id: page
    objectName: "achievementsScreen"
    required property PlayerServices players
    property var coverImages: ({})
    property string selectedTitleId: ""
    property bool refreshPending: true
    readonly property string activeXuid: players.activeXuid
    readonly property var profile: players.profiles.find(p => p.xuid === page.activeXuid)
    readonly property var filteredGames: players.achievementGames.filter(g =>
        (g.title + " " + g.titleId).toLowerCase().includes(search.text.toLowerCase()))
    readonly property var selectedGame: filteredGames.find(g => g.titleId === selectedTitleId) || null
    readonly property int earnedPoints: players.achievementGames.reduce((sum, g) => sum + g.points, 0)
    spacing: 14
    function refreshWhenReady() {
        if (visible && refreshPending && !players.busy) {
            refreshPending = false
            players.refreshAchievements()
        }
    }
    function selectFirstGame() {
        if (!selectedGame) selectedTitleId = filteredGames.length ? filteredGames[0].titleId : ""
    }
    onFilteredGamesChanged: selectFirstGame()
    onActiveXuidChanged: { selectedTitleId = ""; refreshPending = true; Qt.callLater(refreshWhenReady) }
    onVisibleChanged: if (visible) { refreshPending = true; Qt.callLater(refreshWhenReady) }
    Component.onCompleted: Qt.callLater(refreshWhenReady)
    Connections {
        target: page.players
        function onChanged() { Qt.callLater(page.refreshWhenReady) }
    }
    readonly property bool sessionRunning: players.sessionActive
    onSessionRunningChanged: if (!sessionRunning) { refreshPending = true; Qt.callLater(refreshWhenReady) }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Logros"); color: Theme.text; font.pointSize: Theme.display; font.weight: Font.DemiBold }
        Item { Layout.fillWidth: true }
        BusyIndicator { running: page.players.achievementsLoading; visible: running; Layout.preferredWidth: 36; Layout.preferredHeight: 36 }
        ActionButton { text: qsTr("Actualizar"); enabled: !page.players.busy && page.activeXuid.length > 0; onClicked: page.players.refreshAchievements() }
    }
    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: page.profile ? page.profile.gamertag : page.activeXuid.length ? qsTr("Perfil seleccionado") : qsTr("Sin perfil seleccionado")
            color: Theme.muted; font.pointSize: Theme.heading; elide: Text.ElideRight; textFormat: Text.PlainText
        }
        Label { text: qsTr("%1 G · %2 juegos").arg(page.earnedPoints).arg(page.players.achievementGames.length); color: Theme.accent; font.pointSize: Theme.heading }
    }
    Label {
        Layout.fillWidth: true; visible: page.sessionRunning
        text: qsTr("El juego sigue abierto. Los logros se actualizarán cuando cierres la sesión.")
        color: Theme.muted; wrapMode: Text.Wrap
    }
    TextField {
        id: search
        objectName: "achievementSearch"
        Layout.fillWidth: true
        placeholderText: qsTr("Buscar juego…"); Accessible.name: placeholderText
        color: Theme.text; placeholderTextColor: Theme.muted
    }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
        ListView {
            id: games
            objectName: "achievementGames"
            Layout.preferredWidth: Math.max(220, page.width * 0.32)
            Layout.fillHeight: true
            clip: true; spacing: 8; model: page.filteredGames
            activeFocusOnTab: true; keyNavigationEnabled: true
            onCurrentIndexChanged: if (currentIndex >= 0 && currentIndex < count) page.selectedTitleId = page.filteredGames[currentIndex].titleId
            Keys.onReturnPressed: detail.forceActiveFocus()
            KeyNavigation.right: detail
            delegate: ItemDelegate {
                id: gameRow
                required property var modelData
                required property int index
                width: games.width; height: 108
                highlighted: page.selectedTitleId === modelData.titleId
                onClicked: { games.currentIndex = index; page.selectedTitleId = modelData.titleId; games.forceActiveFocus() }
                Accessible.name: qsTr("%1, %2 de %3 logros").arg(modelData.title).arg(modelData.unlocked).arg(modelData.total)
                background: Rectangle { radius: 8; color: gameRow.highlighted ? Theme.raised : Theme.panel; border.width: gameRow.activeFocus || (games.activeFocus && gameRow.highlighted) ? 2 : 1; border.color: gameRow.highlighted ? Theme.accent : Theme.outline }
                contentItem: RowLayout {
                    spacing: 12
                    Image { source: page.coverImages[gameRow.modelData.titleId] || ""; visible: status === Image.Ready; sourceSize: Qt.size(64, 88); Layout.preferredWidth: 52; Layout.preferredHeight: 78; fillMode: Image.PreserveAspectFit; asynchronous: true }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 6
                        Label { Layout.fillWidth: true; text: gameRow.modelData.title; textFormat: Text.PlainText; color: Theme.text; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight; font.weight: Font.DemiBold }
                        Label { text: qsTr("%1 / %2 logros · %3 G").arg(gameRow.modelData.unlocked).arg(gameRow.modelData.total).arg(gameRow.modelData.points); color: Theme.muted }
                        ProgressBar {
                            id: progress
                            Layout.fillWidth: true
                            from: 0; to: Math.max(1, gameRow.modelData.total); value: gameRow.modelData.unlocked
                            Accessible.name: qsTr("Progreso de logros")
                            background: Rectangle { implicitHeight: 6; radius: 3; color: Theme.outline }
                            contentItem: Item { implicitHeight: 6; Rectangle { width: progress.visualPosition * parent.width; height: parent.height; radius: 3; color: Theme.accent } }
                        }
                    }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
        AchievementDetails {
            id: detail
            Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.fillHeight: true
            game: page.selectedGame
            KeyNavigation.left: games
        }
    }
    Label {
        Layout.fillWidth: true
        text: search.text.length && !page.filteredGames.length && page.players.achievementGames.length ? qsTr("No hay juegos que coincidan con la búsqueda.") : page.players.achievementStatus
        color: Theme.muted; wrapMode: Text.Wrap; textFormat: Text.PlainText
    }
}

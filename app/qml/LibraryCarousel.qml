pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Los

Item {
    id: carousel
    objectName: "gameCarousel"
    required property var games
    required property var coverImages
    required property GameCompatibility compatibility
    required property bool reducedMotion
    required property bool busy
    required property int totalGames
    readonly property var selectedGame: view.currentIndex >= 0 && view.currentIndex < games.length
                                        ? games[view.currentIndex] : null
    readonly property real cardHeight: Math.max(190, Math.min(height - 72, width * 0.43))
    readonly property real cardWidth: cardHeight * 0.69
    signal playRequested()
    signal coverRequested(var game)
    signal removeRequested(var game)

    function focusLibrary(): void { view.forceActiveFocus() }
    clip: true

    PathView {
        id: view
        objectName: "carouselPath"
        anchors.fill: parent
        anchors.bottomMargin: 30
        model: carousel.games
        pathItemCount: Math.min(count, width >= 1200 ? 5 : 3)
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        highlightRangeMode: PathView.StrictlyEnforceRange
        highlightMoveDuration: carousel.reducedMotion ? 0 : 240
        snapMode: PathView.SnapOneItem
        interactive: count > 1
        activeFocusOnTab: true
        Keys.onLeftPressed: decrementCurrentIndex()
        Keys.onRightPressed: incrementCurrentIndex()
        Keys.onUpPressed: (event) => { event.accepted = true }
        Keys.onDownPressed: (event) => { event.accepted = true }
        Keys.onReturnPressed: carousel.playRequested()
        Keys.onEnterPressed: carousel.playRequested()
        onCountChanged: if (count > 0 && (currentIndex < 0 || currentIndex >= count)) currentIndex = 0
        path: Path {
            startX: carousel.cardWidth * (view.pathItemCount >= 5 ? 0.24 : 0.4)
            startY: view.height / 2
            PathLine { x: view.width - carousel.cardWidth * (view.pathItemCount >= 5 ? 0.24 : 0.4); y: view.height / 2 }
        }
        delegate: GameCard {
            id: card
            required property var modelData
            required property int index
            width: carousel.cardWidth
            height: carousel.cardHeight
            game: modelData
            compatibility: carousel.compatibility
            artwork: carousel.coverImages[modelData.titleId] || ""
            selected: PathView.isCurrentItem
            scale: selected ? 1.06 : view.pathItemCount >= 5 ? 0.74 : 0.88
            opacity: selected ? 1 : 0.75
            z: selected ? 2 : 1
            Behavior on scale { enabled: !carousel.reducedMotion; NumberAnimation { duration: 180 } }
            onClicked: { view.currentIndex = index; view.forceActiveFocus() }
            onDoubleClicked: carousel.coverRequested(modelData)
            onRemoveRequested: { view.currentIndex = index; carousel.removeRequested(modelData) }
        }
    }
    ActionButton {
        objectName: "carouselPrevious"
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: view.verticalCenter
        width: 48
        leftPadding: 0
        rightPadding: 0
        text: "‹"
        font.pixelSize: 28
        enabled: view.count > 1
        Accessible.name: qsTr("Juego anterior")
        onClicked: { view.decrementCurrentIndex(); view.forceActiveFocus() }
    }
    ActionButton {
        objectName: "carouselNext"
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: view.verticalCenter
        width: 48
        leftPadding: 0
        rightPadding: 0
        text: "›"
        font.pixelSize: 28
        enabled: view.count > 1
        Accessible.name: qsTr("Juego siguiente")
        onClicked: { view.incrementCurrentIndex(); view.forceActiveFocus() }
    }
    Label {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: view.count ? qsTr("%1 / %2").arg(view.currentIndex + 1).arg(view.count) : ""
        color: Theme.muted
        font.pixelSize: 12
    }
    Label {
        anchors.centerIn: parent
        visible: view.count === 0 && !carousel.busy
        text: carousel.totalGames === 0 ? qsTr("Añade tu carpeta de juegos para comenzar.") : qsTr("No hay juegos que coincidan.")
        color: Theme.muted
    }
    BusyIndicator { anchors.centerIn: parent; running: carousel.busy; visible: running }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Los

ScrollView {
    id: page
    objectName: "aboutScreen"
    required property LibraryController controller
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    readonly property var originalWork: [
        {name: qsTr("Biblioteca y carátulas"), role: qsTr("Detecta GOD, XBLA y XEX; busca juegos y permite elegir carátulas por región con ratón o mando.")},
        {name: qsTr("Dashboard Metro y modo consola"), role: qsTr("Dashboard XEX propio, navegación con mando, menú Guía y regreso a la biblioteca al salir del juego.")},
        {name: qsTr("Perfiles, partidas y logros"), role: qsTr("Gestiona perfiles locales, almacenamiento, copias de partidas y progreso de logros por juego.")},
        {name: qsTr("Avatar Studio 3D"), role: qsTr("Edita avatares y conecta sus recursos y animaciones con los juegos compatibles del motor.")},
        {name: qsTr("Comunidad y salas Netplay"), role: qsTr("Muestra salas públicas y enlaza perfiles, configuración de red y lanzamiento de juegos con el motor.")},
        {name: qsTr("Salas de voz y charla"), role: qsTr("Crea salas locales dentro de Emulos360, descubre amigos en LAN y conecta directamente por IP y puerto sin servidor de terceros.")},
        {name: qsTr("Importación y tienda"), role: qsTr("Orquesta la extracción, conversión e instalación de juegos locales; consulta el catálogo y detecta descargas terminadas.")},
        {name: qsTr("Interfaz e intros Orbit 360"), role: qsTr("Interfaz nativa, controles de mando y dos vídeos de entrada creados para Emulos360.")}
    ]
    readonly property var integratedProjects: [
        {name: "Xenia Canary Netplay", role: qsTr("Motor incluido para ejecutar juegos Xbox 360 y ofrecer las funciones de Netplay."), url: "https://github.com/AdrianCassar/xenia-canary"},
        {name: "Xenia Canary", role: qsTr("Rama del emulador sobre la que se desarrolla el motor Netplay incluido."), url: "https://github.com/xenia-canary/xenia-canary"},
        {name: "Xenia", role: qsTr("Proyecto original de emulación Xbox 360 del que deriva Xenia Canary."), url: "https://github.com/xenia-project/xenia"},
        {name: "Qt 6", role: qsTr("Base de la interfaz Qt Quick, los avatares Qt Quick 3D, el audio/vídeo y los servicios de red."), url: "https://www.qt.io/"},
        {name: "iso2god-rs", role: qsTr("Conversor incluido para transformar imágenes ISO locales al formato GOD durante la importación."), url: "https://github.com/iliazeus/iso2god-rs"},
        {name: "7-Zip", role: qsTr("Extractor 7z incluido en el paquete para importar archivos comprimidos; su licencia se distribuye junto a la herramienta."), url: "https://www.7-zip.org/"},
        {name: "FFmpeg", role: qsTr("Bibliotecas multimedia usadas por el motor y por la reproducción de vídeo mediante Qt Multimedia."), url: "https://ffmpeg.org/"},
        {name: "libmspack", role: qsTr("Decodificador LZX empleado al leer recursos originales de avatares Xbox 360."), url: "https://www.cabextract.org.uk/libmspack/"},
        {name: "libdatachannel", role: qsTr("Conexiones WebRTC directas y cifradas para la voz y la charla de las salas."), url: "https://github.com/paullouisageneau/libdatachannel"},
        {name: "Opus", role: qsTr("Códec de baja latencia para comprimir la voz durante la partida."), url: "https://opus-codec.org/"},
        {name: "Mbed TLS", role: qsTr("Cifrado DTLS utilizado por los canales WebRTC integrados."), url: "https://github.com/Mbed-TLS/mbedtls"}
    ]

    ColumnLayout {
        width: page.availableWidth - 12
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Acerca de Emulos360")
            font.pointSize: Theme.display
            font.weight: Font.DemiBold
            color: Theme.text
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Emulos360 %1 · Orbit 360. Biblioteca, consola y herramientas de Xbox 360 reunidas en una aplicación de escritorio.").arg(Qt.application.version)
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            ActionButton {
                text: qsTr("Buscar actualizaciones")
                enabled: page.controller.updater.phase !== "checking" && page.controller.updater.phase !== "downloading"
                onClicked: page.controller.updater.check()
            }
            Label {
                Layout.fillWidth: true
                text: page.controller.updater.status
                color: Theme.muted
                wrapMode: Text.Wrap
            }
        }
        Label {
            Layout.topMargin: 10
            text: qsTr("Creado para Emulos360")
            font.pointSize: Theme.heading
            color: Theme.accent
        }
        Repeater {
            model: page.originalWork
            delegate: Rectangle {
                id: originalCard
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: originalBody.implicitHeight + 24
                color: Theme.panel
                radius: 10
                border.color: Theme.outline
                ColumnLayout {
                    id: originalBody
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 3
                    Label { text: originalCard.modelData.name; color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: 14 }
                    Label { Layout.fillWidth: true; text: originalCard.modelData.role; color: Theme.muted; wrapMode: Text.Wrap }
                }
            }
        }
        Label {
            Layout.topMargin: 12
            text: qsTr("Proyectos integrados")
            font.pointSize: Theme.heading
            color: Theme.accent
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Nombre original, función dentro de Emulos360 y enlace oficial. Estos componentes se incluyen en el paquete o forman parte del motor integrado.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Repeater {
            model: page.integratedProjects
            delegate: Rectangle {
                id: projectCard
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: projectBody.implicitHeight + 24
                color: Theme.panel
                radius: 10
                border.color: Theme.outline
                ColumnLayout {
                    id: projectBody
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 3
                    Label { text: projectCard.modelData.name; color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: 14 }
                    Label { Layout.fillWidth: true; text: projectCard.modelData.role; color: Theme.muted; wrapMode: Text.Wrap }
                    Text {
                        Layout.fillWidth: true
                        text: "<a href=\"" + projectCard.modelData.url + "\">" + projectCard.modelData.url + "</a>"
                        textFormat: Text.RichText
                        color: Theme.accent
                        linkColor: Theme.accent
                        wrapMode: Text.WrapAnywhere
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                }
            }
        }
        Label {
            Layout.topMargin: 12
            text: qsTr("Catálogo consultado")
            font.pointSize: Theme.heading
            color: Theme.accent
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Vimm's Lair aporta los títulos de la tienda. La descarga se realiza en el navegador predeterminado y Emulos360 importa el archivo después; el catálogo no forma parte de la aplicación.")
            color: Theme.muted
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            text: "<a href=\"https://vimm.net/vault/Xbox360\">https://vimm.net/vault/Xbox360</a>"
            textFormat: Text.RichText
            linkColor: Theme.accent
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        Item { Layout.preferredHeight: 12 }
    }
}

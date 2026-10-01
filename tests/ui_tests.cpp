#include "controller.h"
#include "../XEXplugins/MetroDashboard/host/guide_hold.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <limits>
#include <QQmlComponent>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QTest>
#include <QFile>
#include <QCryptographicHash>
#include <QImage>
#include <QImageReader>
#include <QProcess>
#include <QtEndian>
#include <QTcpServer>
#include <QSignalSpy>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>

class UiTests : public QObject {
    Q_OBJECT
private slots:
    void gameContentFiltersMixedPackages() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        Controller controller(directory.filePath("settings.ini"));
        const auto basePath = directory.filePath("base");
        QByteArray base(0x600, '\0'); base.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0x11223344, base.data() + 0x354);
        QFile baseFile(basePath); QVERIFY(baseFile.open(QIODevice::WriteOnly));
        QCOMPARE(baseFile.write(base), base.size()); baseFile.close();
        controller.content()->selectGame({{"title", "Prueba"}, {"titleId", "545408B4"}, {"path", basePath}});
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QCOMPARE(controller.content()->mediaId(), QStringLiteral("11223344"));

        auto makePackage = [](quint32 title, quint32 media, quint32 type) {
            QByteArray header(0x600, '\0'); header.replace(0, 4, "LIVE");
            qToBigEndian(type, header.data() + 0x344);
            qToBigEndian(media, header.data() + 0x354);
            qToBigEndian(title, header.data() + 0x360);
            return header;
        };
        const auto direct = directory.filePath("matching.stfs");
        QFile package(direct); QVERIFY(package.open(QIODevice::WriteOnly));
        auto matching = makePackage(0x545408B4, 0, 2);
        matching.replace(0x32C, 20, QByteArray(20, 'A'));
        QCOMPARE(package.write(matching), matching.size()); package.close();
        controller.content()->inspect(QUrl::fromLocalFile(direct));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QVERIFY2(controller.content()->candidates().size() == 1,
                 qPrintable(controller.content()->status()));
        QVERIFY(controller.content()->candidates().first().toMap().value("match").toBool());
        const auto candidate = controller.content()->candidates().first().toMap();
        QCOMPARE(candidate.value("fileName").toString(), QStringLiteral("matching.stfs"));
        QVERIFY(!controller.content()->isInstalled(candidate));
        const auto installedPath = QDir(controller.engineSettings()->contentPath()).filePath(
            "0000000000000000/545408B4/00000002/renamed.stfs");
        QVERIFY(QDir().mkpath(QFileInfo(installedPath).absolutePath()));
        QFile installedFile(installedPath); QVERIFY(installedFile.open(QIODevice::WriteOnly));
        QCOMPARE(installedFile.write(matching), matching.size()); installedFile.close();
        controller.content()->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QCOMPARE(controller.content()->installed().size(), 1);
        QVERIFY(controller.content()->isInstalled(candidate));
        auto otherPackage = candidate;
        otherPackage["contentId"] = QString(40, u'B');
        QVERIFY(!controller.content()->isInstalled(otherPackage));
        QVERIFY(QFile::remove(installedPath));
        controller.content()->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QVERIFY(!controller.content()->isInstalled(candidate));

        QByteArray iso(36 * 2048, '\0');
        iso.replace(32 * 2048, 20, "MICROSOFT*XBOX*MEDIA");
        qToLittleEndian<quint32>(33, iso.data() + 32 * 2048 + 20);
        qToLittleEndian<quint32>(64, iso.data() + 32 * 2048 + 24);
        auto node = [&](int pos, quint16 right, quint32 sector, const char* name) {
            qToLittleEndian<quint16>(right, iso.data() + pos + 2);
            qToLittleEndian<quint32>(sector, iso.data() + pos + 4);
            qToLittleEndian<quint32>(0x600, iso.data() + pos + 8);
            iso[pos + 13] = 5;
            iso.replace(pos + 14, 5, name);
        };
        node(33 * 2048, 8, 34, "DLC_A");
        node(33 * 2048 + 32, 0, 35, "DLC_B");
        iso.replace(34 * 2048, matching.size(), matching);
        const auto foreign = makePackage(0x5454087C, 0, 2);
        iso.replace(35 * 2048, foreign.size(), foreign);
        const auto isoPath = directory.filePath("mixed.iso");
        QFile disc(isoPath); QVERIFY(disc.open(QIODevice::WriteOnly));
        QCOMPARE(disc.write(iso), iso.size()); disc.close();
        controller.content()->inspect(QUrl::fromLocalFile(isoPath));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QCOMPARE(controller.content()->candidates().size(), 2);
        QVERIFY(controller.content()->candidates()[0].toMap().value("match").toBool());
        QVERIFY(!controller.content()->candidates()[1].toMap().value("match").toBool());

        const auto updatePath = directory.filePath("different-media.stfs");
        QFile update(updatePath); QVERIFY(update.open(QIODevice::WriteOnly));
        const auto differentMedia = makePackage(0x545408B4, 0xAABBCCDD, 0xB0000);
        QCOMPARE(update.write(differentMedia), differentMedia.size()); update.close();
        controller.content()->inspect(QUrl::fromLocalFile(updatePath));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 5000);
        QCOMPARE(controller.content()->candidates().size(), 1);
        QVERIFY(!controller.content()->candidates().first().toMap().value("match").toBool());

        const auto realIso = qEnvironmentVariable("EMULOS_TEST_CONTENT_ISO");
        if (!realIso.isEmpty()) {
            controller.content()->inspect(QUrl::fromLocalFile(realIso));
            QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 30000);
            QVERIFY2(controller.content()->candidates().size() >= 8,
                     qPrintable(controller.content()->status()));
            int compatible = 0, otherGame = 0;
            for (const auto& value : controller.content()->candidates()) {
                if (value.toMap().value("match").toBool()) ++compatible;
                else ++otherGame;
            }
            QVERIFY(compatible >= 7);
            QVERIFY(otherGame >= 1);
            if (qEnvironmentVariableIntValue("EMULOS_TEST_CONTENT_INSTALL") == 1) {
                QVERIFY(controller.engineSettings()->save());
                int smallest = -1;
                qulonglong smallestSize = std::numeric_limits<qulonglong>::max();
                for (int i = 0; i < controller.content()->candidates().size(); ++i) {
                    const auto candidate = controller.content()->candidates()[i].toMap();
                    const auto bytes = candidate.value("bytes").toULongLong();
                    if (candidate.value("match").toBool() && bytes < smallestSize) {
                        smallest = i;
                        smallestSize = bytes;
                    }
                }
                QVERIFY(smallest >= 0);
                controller.content()->install(smallest);
                QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 60000);
                QCOMPARE(controller.content()->installed().size(), 1);
                controller.content()->remove(0);
                QTRY_VERIFY_WITH_TIMEOUT(!controller.content()->busy(), 30000);
                QCOMPARE(controller.content()->installed().size(), 0);
            }
        }
    }
    void localCoverPersists() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto originalPath = directory.filePath("original.png");
        QImage original(1200, 1600, QImage::Format_RGB32);
        original.fill(QColor("#c65a2a"));
        QVERIFY(original.save(originalPath));
        const auto originalUrl = QUrl::fromLocalFile(originalPath);
        QUrl savedUrl;
        {
            Controller controller(directory.filePath("settings.ini"));
            QVERIFY(controller.useLocalCover("4D530919", originalUrl));
            savedUrl = controller.coverImages().value("4D530919").toUrl();
            QVERIFY(savedUrl.isLocalFile());
            QVERIFY(savedUrl.toLocalFile() != originalPath);
            QVERIFY(savedUrl.toLocalFile().startsWith(directory.filePath("covers/")));
            QCOMPARE(QImageReader(savedUrl.toLocalFile()).size(), QSize(900, 1200));
            QVERIFY(!controller.useLocalCover("4D530919", QUrl("https://example.com/cover.png")));
            QCOMPARE(controller.coverImages().value("4D530919").toUrl(), savedUrl);
            const auto invalidPath = directory.filePath("not-an-image.png");
            QFile invalid(invalidPath); QVERIFY(invalid.open(QIODevice::WriteOnly));
            invalid.write("not an image"); invalid.close();
            QVERIFY(!controller.useLocalCover("4D530919", QUrl::fromLocalFile(invalidPath)));
            QCOMPARE(controller.coverImages().value("4D530919").toUrl(), savedUrl);
        }
        QVERIFY(QFile::remove(originalPath));
        Controller restored(directory.filePath("settings.ini"));
        QCOMPARE(restored.coverImages().value("4D530919").toUrl(), savedUrl);
        QVERIFY(QFileInfo::exists(savedUrl.toLocalFile()));

        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&restored)}, {"introEnabled", false}});
        engine.loadFromModule("Los", "Main"); QVERIFY(!engine.rootObjects().isEmpty());
        auto* picker = engine.rootObjects().first()->findChild<QObject*>("libraryCoverPicker");
        QVERIFY(picker);
        QVERIFY(picker->findChild<QObject*>("chooseLocalCoverButton"));
    }
    void compatibilityIndicators() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QJsonArray reports;
        const QStringList states{"Playable", "Gameplay", "Loads", "Unplayable", "Unknown"};
        const QStringList names{"Juego jugable", "Juego con fallos", "Solo carga", "Juego no jugable", "Sin informe"};
        for (int index = 0; index < states.size(); ++index) {
            const auto id = QString::number(0x10000001 + index, 16).toUpper();
            reports.append(QJsonObject{{"id", id}, {"title", names[index]}, {"state", states[index]}});
            const auto folder = directory.filePath("games/" + id + "/000D0000");
            QVERIFY(QDir().mkpath(folder));
            QByteArray header(0x520, '\0'); header.replace(0, 4, "LIVE");
            qToBigEndian<quint32>(0xD0000, header.data() + 0x344);
            qToBigEndian<quint32>(0x10000001 + index, header.data() + 0x360);
            for (qsizetype letter = 0; letter < names[index].size(); ++letter)
                qToBigEndian<quint16>(names[index][letter].unicode(), header.data() + 0x411 + 2 * letter);
            QFile file(folder + "/game"); QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(header), header.size());
        }
        QVERIFY(QDir().mkpath(directory.filePath("compatibility")));
        QFile cache(directory.filePath("compatibility/canary.json")); QVERIFY(cache.open(QIODevice::WriteOnly));
        cache.write(QJsonDocument(reports).toJson()); cache.close();
        Controller controller(directory.filePath("settings.ini"));
        controller.setLibraryPath(directory.filePath("games"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 5);
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}, {"introEnabled", false}});
        engine.loadFromModule("Los", "Main"); QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
        window->setVisibility(QWindow::Windowed); window->resize(1440, 900);
        const auto findAll = [](QQuickItem* root, const QString& name, auto&& self) -> QList<QQuickItem*> {
            QList<QQuickItem*> found;
            if (root->objectName() == name) found.append(root);
            for (auto* child : root->childItems()) found.append(self(child, name, self));
            return found;
        };
        QTRY_COMPARE(findAll(window->contentItem(), "compatibilityBadge", findAll).size(), 5);
        QSet<QString> displayed;
        for (const auto* badge : findAll(window->contentItem(), "compatibilityBadge", findAll)) {
            displayed.insert(badge->property("statusCode").toString());
            QVERIFY(badge->width() > 0); QVERIFY(badge->height() > 0);
        }
        QCOMPARE(displayed, (QSet<QString>{"playable", "gameplay", "loads", "unplayable", "unknown"}));
        QTest::qWait(250);
        const auto capture = qEnvironmentVariable("EMULOS_COMPAT_CAPTURE_DIR");
        if (!capture.isEmpty()) QVERIFY(window->grabWindow().save(capture + "/library.png"));
        QQmlComponent component(&engine); component.loadFromModule("Los", "AppPreferences");
        QScopedPointer<QObject> preferences(component.createWithInitialProperties({{"controller", QVariant::fromValue(&controller)}}));
        QVERIFY2(preferences, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(preferences.data()); QVERIFY(item);
        window->contentItem()->childItems().first()->setVisible(false);
        item->setParentItem(window->contentItem()); item->setSize(QSizeF(window->size()));
        item->setProperty("section", "compatibility");
        QTRY_VERIFY(!findAll(item, "compatibilityLegend", findAll).isEmpty());
        QCOMPARE(findAll(item, "compatibilityBadge", findAll).size(), 5);
        QTest::qWait(250);
        if (!capture.isEmpty()) QVERIFY(window->grabWindow().save(capture + "/legend.png"));
        window->resize(980, 660); item->setSize(QSizeF(window->size())); QTest::qWait(100);
        for (const auto* badge : findAll(item, "compatibilityBadge", findAll)) {
            QVERIFY(badge->width() > 0); QVERIFY(badge->height() > 0);
        }
    }
    void compatibilityStoreRendering() {
        if (qEnvironmentVariable("EMULOS_COMPAT_LIVE") != "1") QSKIP("Set EMULOS_COMPAT_LIVE for the real catalog");
        QTemporaryDir directory; QVERIFY(directory.isValid());
        Controller controller(directory.filePath("settings.ini"));
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}, {"initialPage", "store"}, {"introEnabled", false}});
        engine.loadFromModule("Los", "Main"); QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
        window->setVisibility(QWindow::Windowed); window->resize(1440, 900);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.store()->busy(), 60000);
        QVERIFY2(!controller.store()->results().isEmpty(), qPrintable(controller.store()->status()));
        QTest::qWait(250);
        const auto capture = qEnvironmentVariable("EMULOS_COMPAT_CAPTURE_DIR");
        if (!capture.isEmpty()) QVERIFY(window->grabWindow().save(capture + "/store.png"));
    }
    void privatePanelRendering() {
        if (qEnvironmentVariable("EMULOS_PRIVATE_SERVER_DIR").isEmpty()) QSKIP("Set the packaged private server directory");
        QTemporaryDir data; QVERIFY(data.isValid());
        Controller controller(data.filePath("settings.ini"));
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}, {"initialPage", "multiplayer"}});
        engine.loadFromModule("Los", "Main"); QVERIFY(!engine.rootObjects().isEmpty());
        QTcpServer reservation; QVERIFY(reservation.listen(QHostAddress::LocalHost, 0));
        const auto port = reservation.serverPort(); reservation.close();
        auto* service = controller.privateNetplay();
        service->host("Panel privado", port, "127.0.0.1", false);
        QTRY_VERIFY_WITH_TIMEOUT(!service->busy(), 60000);
        QVERIFY2(service->connected(), qPrintable(service->status()));
        service->createInvitation("Amigo de prueba", 1);
        QTRY_VERIFY_WITH_TIMEOUT(!service->metrics().value("invitations").toList().isEmpty(), 15000);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
        QTest::qWait(300);
        const auto capture = qEnvironmentVariable("EMULOS_PRIVATE_PANEL_CAPTURE");
        if (!capture.isEmpty()) QVERIFY(window->grabWindow().save(capture));
        service->stop(); QTRY_VERIFY_WITH_TIMEOUT(!service->running(), 15000);
    }
    void privateNetplayLifecycle() {
        if (qEnvironmentVariable("EMULOS_PRIVATE_SERVER_DIR").isEmpty()) QSKIP("Set the packaged private server directory");
        QTemporaryDir folder; QVERIFY(folder.isValid());
        QTcpServer reservation; QVERIFY(reservation.listen(QHostAddress::LocalHost, 0));
        const auto port = reservation.serverPort(); reservation.close();
        PrivateNetplay host(folder.filePath("host")), client(folder.filePath("client"));
        QSignalSpy endpoints(&host, &PrivateNetplay::endpointChanged);
        host.host("Private test", port, "127.0.0.1", false);
        QTRY_VERIFY_WITH_TIMEOUT(!host.busy(), 60000);
        QVERIFY2(host.connected(), qPrintable(host.status())); QVERIFY(host.hosting());
        QVERIFY(!endpoints.isEmpty()); QVERIFY(endpoints.first().first().toString().startsWith("http://127.0.0.1:"));
        host.createInvitation("Friend", 1);
        QTRY_VERIFY_WITH_TIMEOUT(!host.invitation().isEmpty(), 15000);
        client.join(host.invitation(), "Qt friend");
        QTRY_VERIFY_WITH_TIMEOUT(!client.busy(), 30000);
        QVERIFY2(client.connected(), qPrintable(client.status())); QVERIFY(!client.hosting());
        QTRY_COMPARE_WITH_TIMEOUT(client.metrics().value("role").toString(), QString("client"), 15000);
        client.stop(); QTRY_VERIFY_WITH_TIMEOUT(!client.running(), 15000);
        host.stop(); QTRY_VERIFY_WITH_TIMEOUT(!host.running(), 15000);
        QVERIFY(endpoints.last().first().toString().isEmpty());
    }
    void vimmCatalogParser() {
        const QByteArray html = R"(<h1>Search results for Banjo</h1><table class="hovertable striped">
            <tr><th>Title</th></tr>
            <tr><td><a href="/vault/999999" style="display: none">9</a>
            <a href= "/vault/110456">Banjo-Kazooie &amp; Friends</a>
            <span class="redBorder" title="Xbox Live Arcade">XBLA</span></td>
            <td><img src="/images/flags/world.png" title="World"></td></tr>
            <tr><td><a href="https://other.example/game">External</a></td></tr></table>)";
        const auto games = GameStore::parseSearchHtml(html, true);
        QCOMPARE(games.size(), 1);
        const auto game = games.first().toMap();
        QCOMPARE(game.value("title").toString(), QString("Banjo-Kazooie & Friends"));
        QCOMPARE(game.value("url").toString(), QString("https://vimm.net/vault/110456"));
        QCOMPARE(game.value("kind").toString(), QString("Xbox Live Arcade"));
        QCOMPARE(game.value("region").toString(), QString("world"));
        QVERIFY(GameStore::parseSearchHtml("Just a moment", false).isEmpty());
    }

    void vimmLetterParser() {
        const QByteArray html = R"(<table class="rounded centered cellpadding1 hovertable striped">
          <tr><td><a href="/vault/999999" style="display: none">9</a>
          <a href="/vault/80736">Alan Wake &amp; More</a></td>
          <td><img src="/images/flags/europe.png"></td></tr></table>)";
        const auto games = GameStore::parseLetterHtml(html, false);
        QCOMPARE(games.size(), 1);
        QCOMPARE(games.first().toMap().value("title").toString(), QString("Alan Wake & More"));
        QCOMPARE(games.first().toMap().value("url").toString(), QString("https://vimm.net/vault/80736"));
        QVERIFY(GameStore::parseLetterHtml("Just a moment", false).isEmpty());
    }

    void vimmCurrentCatalogParser() {
        const QByteArray html =
            "<h2>Search results for &quot;Banjo&quot; in X360-D games</h2>"
            "<table><tr><td><a href=\"/vault/12345\">Sidebar title</a></td></tr></table>"
            "<table style=\"width:100%\" class='rounded striped centered hovertable cellpadding1'>"
            "<caption><table><tr><th>Title</th><th>Region</th></tr></table></caption>"
            "<tr><td><img src=\"/images/icons/digital.svg\" title=\"Digital media\">"
            "<a href=\"/vault/110456\">Banjo-Kazooie &amp; Friends</a>"
            "<span class=\"redBorder\" title=\"Xbox Live Arcade\">XBLA</span></td>"
            "<td><img src=\"/images/flags/world.png\"></td></tr>"
            "<tr><td><a href=\"/vault/110458\">Banjo-Kazooie</a>"
            "<b class=\"redBorder\" title=\"Title Update\">TU</b></td>"
            "<td><img src=\"/images/flags/europe.png\"><img src=\"/images/flags/usa.png\"></td></tr>"
            "<tr><td><a href=\"/vault/999999\" style=\"display: none\">9</a>"
            "<a href=\"https://other.example/game\">External</a></td></tr></table>"
            "<table><tr><td><a href=\"/vault/54321\">Footer title</a></td></tr></table>";
        const auto games = GameStore::parseLetterHtml(html, true);
        QCOMPARE(games.size(), 2);
        QCOMPARE(GameStore::parseSearchHtml(html, true), games);
        QCOMPARE(games.first().toMap().value("title").toString(), QString("Banjo-Kazooie & Friends"));
        QCOMPARE(games.first().toMap().value("url").toString(), QString("https://vimm.net/vault/110456"));
        QCOMPARE(games.last().toMap().value("kind").toString(), QString("Title Update"));
        QCOMPARE(games.last().toMap().value("region").toString(), QString("europe, usa"));
    }

    void rapidStoreLetterSwitches() {
        GameImporter importer;
        GameStore store(&importer);
        store.browseLetter(QStringLiteral("A"), false);
        store.browseLetter(QStringLiteral("B"), false);
        store.browseLetter(QStringLiteral("C"), true);
        store.search(QString(), false);
        QTest::qWait(50);
        QVERIFY(!store.busy());
        QVERIFY(store.results().isEmpty());
        QVERIFY(store.status().contains(QStringLiteral("dos caracteres")));
    }

    void liveVimmCatalog() {
        if (qEnvironmentVariable("EMULOS_LIVE_STORE_SEARCH") != QStringLiteral("1"))
            QSKIP("Only for an explicit live catalog check");
        GameImporter importer;
        GameStore store(&importer);
        store.search(QStringLiteral("Banjo"), true);
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 30000);
        QVERIFY2(!store.results().isEmpty(), qPrintable(store.status()));
        QVERIFY(store.results().first().toMap().value("url").toString().startsWith("https://vimm.net/vault/"));
        qInfo().noquote() << store.status();
        store.browseLetter(QStringLiteral("A"), false);
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 30000);
        QVERIFY2(store.results().size() >= 50, qPrintable(store.status()));
        qInfo().noquote() << store.status();
        store.browseLetter(QStringLiteral("B"), false);
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 30000);
        QVERIFY2(!store.results().isEmpty(), qPrintable(store.status()));
        qInfo().noquote() << store.status();
        store.browseLetter(QStringLiteral("A"), true);
        QTRY_VERIFY_WITH_TIMEOUT(!store.busy(), 60000);
        QVERIFY2(store.results().size() > 200, qPrintable(store.status()));
        qInfo().noquote() << store.status();
    }

    void downloadedArchiveInstallsAndCleansUp() {
        const QString sevenZip = QStringLiteral("C:/Program Files/7-Zip/7z.exe");
        if (!QFileInfo(sevenZip).isFile()) QSKIP("7-Zip is required for archive integration");
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto downloads = directory.filePath("downloads");
        const auto library = directory.filePath("library");
        QVERIFY(QDir().mkpath(downloads));
        QVERIFY(QDir().mkpath(library));
        QByteArray header(0x520, '\0');
        header.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0xD0000, header.data() + 0x344);
        qToBigEndian<quint32>(0x58410A73, header.data() + 0x360);
        const auto packagePath = directory.filePath("package");
        QFile package(packagePath);
        QVERIFY(package.open(QIODevice::WriteOnly));
        QCOMPARE(package.write(header), header.size());
        package.close();
        GameImporter importer;
        GameStore store(&importer, nullptr, downloads);
        store.watchDownload("Banjo-Kazooie", library);
        QVERIFY(store.awaitingDownload());
        const auto archive = QDir(downloads).filePath("Banjo-Kazooie (World).7z");
        QCOMPARE(QProcess::execute(sevenZip, {"a", "-t7z", archive, packagePath}), 0);
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(archive), 20000);
        QCOMPARE(los::scanLibrary(library).games.size(), 1);
        QVERIFY(store.status().contains("instalado"));
    }

    void importLocalXblaPreservesSource() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto libraryPath = directory.filePath("library");
        QVERIFY(QDir().mkpath(libraryPath));
        QByteArray header(0x520, '\0');
        header.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0xD0000, header.data() + 0x344);
        qToBigEndian<quint32>(0x58410A71, header.data() + 0x360);
        const auto sourcePath = directory.filePath("source-package");
        {
            QFile source(sourcePath);
            QVERIFY(source.open(QIODevice::WriteOnly));
            QCOMPARE(source.write(header), header.size());
        }
        GameImporter importer;
        importer.importFile(QUrl::fromLocalFile(sourcePath), libraryPath);
        QTRY_VERIFY_WITH_TIMEOUT(!importer.busy(), 10000);
        QVERIFY2(importer.status().contains("instalado"), qPrintable(importer.status()));
        QVERIFY(QFileInfo(sourcePath).isFile());
        const auto games = los::scanLibrary(libraryPath).games;
        QCOMPARE(games.size(), 1);
        QCOMPARE(games.first().toMap().value("format").toString(), QString("XBLA"));
        QVERIFY(QFileInfo(games.first().toMap().value("path").toString()).isFile());
        QVERIFY(QDir(libraryPath).entryList({".emulos-import-*"}, QDir::Dirs | QDir::Hidden).isEmpty());
        importer.importFile(QUrl::fromLocalFile(sourcePath), libraryPath);
        QTRY_VERIFY_WITH_TIMEOUT(!importer.busy(), 10000);
        QVERIFY(importer.status().contains("ya existe"));
        QVERIFY(QFileInfo(sourcePath).isFile());
        QCOMPARE(los::scanLibrary(libraryPath).games.size(), 1);

        const QString sevenZip = QStringLiteral("C:/Program Files/7-Zip/7z.exe");
        if (QFileInfo(sevenZip).isFile()) {
            qToBigEndian<quint32>(0x58410A72, header.data() + 0x360);
            const auto secondSource = directory.filePath("second-package");
            QFile second(secondSource);
            QVERIFY(second.open(QIODevice::WriteOnly));
            QCOMPARE(second.write(header), header.size());
            second.close();
            const auto archive = directory.filePath("second.7z");
            QCOMPARE(QProcess::execute(sevenZip, {"a", "-t7z", archive, secondSource}), 0);
            importer.importFile(QUrl::fromLocalFile(archive), libraryPath);
            QTRY_VERIFY_WITH_TIMEOUT(!importer.busy(), 10000);
            QVERIFY2(importer.status().contains("instalado"), qPrintable(importer.status()));
            QVERIFY(QFileInfo(archive).isFile());
            QCOMPARE(los::scanLibrary(libraryPath).games.size(), 2);
            QVERIFY(QDir(libraryPath).entryList({".emulos-import-*"}, QDir::Dirs | QDir::Hidden).isEmpty());
        }

        const auto badIso = directory.filePath("broken.iso");
        {
            QFile iso(badIso);
            QVERIFY(iso.open(QIODevice::WriteOnly));
            iso.write("invalid ISO");
        }
        importer.importFile(QUrl::fromLocalFile(badIso), libraryPath);
        QTRY_VERIFY_WITH_TIMEOUT(!importer.busy(), 10000);
        QVERIFY(!importer.status().contains("instalado"));
        QVERIFY(QFileInfo(badIso).isFile());
        QVERIFY(QDir(libraryPath).entryList({".emulos-import-*"}, QDir::Dirs | QDir::Hidden).isEmpty());
    }

    void metroGuideHold() {
        MetroGuideHold h;
        QVERIFY(!h.update(0,true,0));
        QVERIFY(!h.update(0,true,2499));
        QVERIFY(h.update(0,true,2500));
        QVERIFY(!h.update(0,true,7000));
        QVERIFY(!h.update(-1,true,8000));
        QVERIFY(!h.update(0,true,8100));
        QVERIFY(!h.update(1,true,10599));
        QVERIFY(!h.update(1,true,10600));
        QVERIFY(h.update(1,true,13099));
        QVERIFY(!h.update(1,false,14000));
        QVERIFY(!h.update(1,true,20000));
        QVERIFY(!h.update(-1,true,23000));
    }
    void metroProtocol() {
        const QString nonce(32, 'a');
        quint32 serial=0; int action=0,index=0;
        const auto request=[&](const QByteArray& seq,const QByteArray& cmd,const QByteArray& arg) {
            return "EMETRO1\n"+nonce.toLatin1()+'\n'+seq+'\n'+cmd+'\n'+arg+"\nEND\n";
        };
        QVERIFY(MetroDashboard::decodeRequest(request("1","1","0"),nonce,0,2,serial,action,index));
        QCOMPARE(serial,1u); QCOMPARE(action,1); QCOMPARE(index,0);
        QVERIFY(!MetroDashboard::decodeRequest(request("1","1","0"),nonce,1,2,serial,action,index));
        QVERIFY(!MetroDashboard::decodeRequest(request("2","1","2"),nonce,1,2,serial,action,index));
        QVERIFY(!MetroDashboard::decodeRequest(request("2","1","-1"),nonce,1,2,serial,action,index));
        QVERIFY(MetroDashboard::decodeRequest(request("2","11","1"),nonce,1,2,serial,action,index));
        QCOMPARE(action,11); QCOMPARE(index,1);
        QVERIFY(!MetroDashboard::decodeRequest(request("3","11","2"),nonce,2,2,serial,action,index));
        QVERIFY(MetroDashboard::decodeRequest(request("2","6","0"),nonce,1,2,serial,action,index)); // Host requires an armed native guide too.
        QVERIFY(!MetroDashboard::decodeRequest(request("2","9","0"),QString(32,'b'),1,2,serial,action,index));
        QVERIFY(!MetroDashboard::decodeRequest(request("2","9","0").chopped(2),nonce,1,2,serial,action,index));
        QVERIFY(!MetroDashboard::decodeRequest(QByteArray(257,'x'),nonce,1,2,serial,action,index));
        QVERIFY(MetroDashboard::decodeRequest(request("2","9","0"),nonce,1,2,serial,action,index));
        QCOMPARE(action,9); // Guest may open the menu, never shut down the PC.
    }
    void achievementLibrary() {
        const auto data = qEnvironmentVariable("EMULOS_ACHIEVEMENTS_DATA");
        const auto core = qEnvironmentVariable("EMULOS_TEST_CORE");
        if (data.isEmpty() || core.isEmpty()) QSKIP("Run scripts/check-achievements-bridge.py --ui to provide an isolated profile");
        EngineSettings settings(data + "/engine");
        PlayerServices players(&settings, data, nullptr, core);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadFromModule("Los", "AchievementsScreen");
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.createWithInitialProperties({{"players", QVariant::fromValue(&players)}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* screen = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(screen);
        QQuickWindow window;
        window.resize(1000, 700); screen->setParentItem(window.contentItem());
        screen->setSize(QSizeF(1000, 700)); window.show();
        QTRY_COMPARE_WITH_TIMEOUT(players.achievementGames().size(), 2, 60000);
        auto* search = screen->findChild<QObject*>("achievementSearch");
        auto* games = screen->findChild<QObject*>("achievementGames");
        auto* filter = screen->findChild<QObject*>("achievementFilter");
        auto* entries = screen->findChild<QObject*>("achievementEntries");
        QVERIFY(search && games && filter && entries);
        QTRY_COMPARE(entries->property("count").toInt(), 3);
        filter->setProperty("currentIndex", 1);
        QTRY_COMPARE(entries->property("count").toInt(), 2);
        filter->setProperty("currentIndex", 2);
        QTRY_COMPARE(entries->property("count").toInt(), 1);
        search->setProperty("text", "Sin detalle");
        QTRY_COMPARE(games->property("count").toInt(), 1);
        QTRY_COMPARE(entries->property("count").toInt(), 0);
        search->setProperty("text", "EE000001");
        QTRY_COMPARE(entries->property("count").toInt(), 3);
        auto* gameList = qobject_cast<QQuickItem*>(games);
        gameList->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Right);
        QTRY_VERIFY(qobject_cast<QQuickItem*>(entries)->hasActiveFocus());
        QVERIFY(settings.setValue("Profiles.logged_profile_slot_0_xuid", QString{}));
        QTRY_VERIFY(players.achievementGames().isEmpty());
        QTRY_COMPARE(entries->property("count").toInt(), 0);
        screen->setParentItem(nullptr);
    }
    void removesGameFilesWithoutTouchingSharedFolders() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto library = directory.filePath("library");
        QVERIFY(QDir().mkpath(library + "/title/00007000/package.data"));
        QByteArray godHeader(0x511, '\0');
        godHeader.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0x7000, godHeader.data() + 0x344);
        qToBigEndian<quint32>(0x4D5307D5, godHeader.data() + 0x360);
        qToBigEndian<quint32>(1, godHeader.data() + 0x39D);
        qToBigEndian<quint32>(1, godHeader.data() + 0x3A9);
        const auto package = library + "/title/00007000/package";
        { QFile file(package); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(godHeader), godHeader.size()); }
        { QFile file(package + ".data/Data0000"); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write("data"), 4); }

        Controller controller(directory.filePath("settings.ini"));
        controller.setLibraryPath(library);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 1);
        QVERIFY2(controller.removeGame(controller.games().first().toMap().value("path").toString()),
                 qPrintable(controller.status()));
        QVERIFY(!QFileInfo::exists(package));
        QVERIFY(!QFileInfo::exists(package + ".data"));
        QVERIFY(!QDir(library + "/title/00007000").exists());
        QVERIFY(!QDir(library + "/title").exists());
        QVERIFY(QDir(library).exists());
        QCOMPARE(controller.games().size(), 0);
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 0);

        QVERIFY(QDir().mkpath(library + "/shared"));
        QByteArray xexHeader(0x100, '\0');
        xexHeader.replace(0, 4, "XEX2");
        qToBigEndian<quint32>(1, xexHeader.data() + 4);
        qToBigEndian<quint32>(0x100, xexHeader.data() + 8);
        qToBigEndian<quint32>(1, xexHeader.data() + 0x14);
        qToBigEndian<quint32>(0x00040006, xexHeader.data() + 0x18);
        qToBigEndian<quint32>(0x40, xexHeader.data() + 0x1C);
        const auto first = library + "/shared/first.xex";
        const auto second = library + "/shared/second.xex";
        qToBigEndian<quint32>(0x454109AB, xexHeader.data() + 0x4C);
        { QFile file(first); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(xexHeader), xexHeader.size()); }
        qToBigEndian<quint32>(0x454109AC, xexHeader.data() + 0x4C);
        { QFile file(second); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(xexHeader), xexHeader.size()); }
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 2);
        QVERIFY(!controller.removeGame(first));
        QVERIFY(QFileInfo::exists(first));
        QVERIFY(QFileInfo::exists(second));
        QVERIFY(QDir(library + "/shared").exists());
        QVERIFY(QDir().mkpath(library + "/group/solo"));
        const auto solo = library + "/group/solo/default.xex";
        qToBigEndian<quint32>(0x454109AD, xexHeader.data() + 0x4C);
        { QFile file(solo); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(xexHeader), xexHeader.size()); }
        { QFile file(library + "/group/solo/assets.bin"); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write("asset"), 5); }
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 3);
        QVERIFY2(controller.removeGame(solo), qPrintable(controller.status()));
        QVERIFY(!QFileInfo::exists(library + "/group/solo"));
        QVERIFY(!QFileInfo::exists(library + "/group"));
        QVERIFY(QFileInfo::exists(first));
        QVERIFY(QFileInfo::exists(second));
        const auto rootXex = library + "/root.xex";
        qToBigEndian<quint32>(0x454109AE, xexHeader.data() + 0x4C);
        { QFile file(rootXex); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(xexHeader), xexHeader.size()); }
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY(!controller.removeGame(rootXex));
        QVERIFY(QFileInfo::exists(rootXex));
    }

    void scanSearchModesAndPersistence() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir(directory.path()).mkpath("games/00007000/game.data"));
        const auto settings = directory.filePath("settings.ini");
        {
            QByteArray header(0x511, '\0');
            header.replace(0, 4, "LIVE");
            qToBigEndian<quint32>(0x7000, header.data() + 0x344);
            qToBigEndian<quint32>(0x4D5307D5, header.data() + 0x360);
            qToBigEndian<quint32>(1, header.data() + 0x39D);
            qToBigEndian<quint32>(1, header.data() + 0x3A9);
            QFile file(directory.filePath("games/00007000/game"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(header);
            QFile fragment(directory.filePath("games/00007000/game.data/Data0000"));
            QVERIFY(fragment.open(QIODevice::WriteOnly));
            fragment.write("x");
        }
        QVERIFY(QDir().mkpath(directory.filePath("covers")));
        const auto coverUrl = QString("http://download.xbox.com/content/images/66acd000-77fe-1000-9115-d802%1/%2/boxartlg.jpg")
            .arg("4d5307d5").arg(3082);
        const auto coverHash = QCryptographicHash::hash(coverUrl.toUtf8(), QCryptographicHash::Sha256).toHex();
        QImage cachedCover(64, 64, QImage::Format_ARGB32);
        cachedCover.fill(Qt::green);
        QVERIFY(cachedCover.save(directory.filePath("covers/" + QString::fromLatin1(coverHash) + ".png")));
        Controller controller(settings);
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}});
        engine.loadFromModule("Los", "Main");
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        QTRY_COMPARE(window->visibility(), QWindow::Maximized);
        // The offscreen test platform has no real maximized screen geometry.
        window->setVisibility(QWindow::Windowed);
        window->resize(1320, 820);
        QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        QTRY_COMPARE(window->width(), 1320);
        controller.setLibraryPath(directory.filePath("games"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 1);
        // Loader-created items may be visually parented without QObject parenting.
        const auto findVisual = [](QQuickItem* root, const QString& name, auto&& self) -> QQuickItem* {
            if (root->objectName() == name) return root;
            for (auto* child : root->childItems())
                if (auto* match = self(child, name, self)) return match;
            return nullptr;
        };
        auto* grid = findVisual(window->contentItem(), "gameGrid", findVisual);
        auto* search = findVisual(window->contentItem(), "librarySearch", findVisual);
        auto* coverPicker = window->findChild<QObject*>("libraryCoverPicker");
        auto* consoleCoverPicker = window->findChild<QObject*>("consoleCoverPicker");
        auto* shell = window->findChild<QObject*>("mainShell");
        QVERIFY(grid);
        QVERIFY(search);
        QVERIFY(coverPicker);
        QVERIFY(consoleCoverPicker);
        QVERIFY(shell);
        QTRY_COMPARE(grid->property("count").toInt(), 1);
        auto* coverArt = findVisual(window->contentItem(), "gameCoverArt", findVisual);
        QVERIFY(coverArt);
        QImage manual(800, 1100, QImage::Format_RGB32);
        manual.fill(Qt::blue);
        const auto firstCover = directory.filePath("first-cover.png");
        QVERIFY(manual.save(firstCover));
        QVERIFY(controller.useLocalCover("4D5307D5", QUrl::fromLocalFile(firstCover)));
        const auto firstUrl = controller.coverImages().value("4D5307D5").toUrl();
        QTRY_COMPARE(coverArt->property("source").toUrl(), firstUrl);
        manual.fill(Qt::red);
        const auto secondCover = directory.filePath("second-cover.jpg");
        QVERIFY(manual.save(secondCover));
        QVERIFY(controller.useLocalCover("4D5307D5", QUrl::fromLocalFile(secondCover)));
        const auto secondUrl = controller.coverImages().value("4D5307D5").toUrl();
        QVERIFY(secondUrl != firstUrl);
        QTRY_COMPARE(coverArt->property("source").toUrl(), secondUrl);
        QVERIFY(controller.status().contains(QStringLiteral("Carátula local aplicada")));
        auto* removeDialog = window->findChild<QObject*>("removeGameDialog");
        QVERIFY(removeDialog);
        auto* cancelRemovalButton = removeDialog->findChild<QQuickItem*>("cancelGameRemovalButton");
        auto* confirmRemovalButton = removeDialog->findChild<QQuickItem*>("confirmGameRemovalButton");
        QVERIFY(cancelRemovalButton);
        QVERIFY(confirmRemovalButton);
        grid->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_F8);
        QTRY_VERIFY(removeDialog->property("visible").toBool());
        QCOMPARE(removeDialog->property("game").toMap().value("path").toString(),
                 controller.games().first().toMap().value("path").toString());
        QTRY_VERIFY(cancelRemovalButton->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_VERIFY(confirmRemovalButton->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Left);
        QTRY_VERIFY(cancelRemovalButton->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(!removeDialog->property("visible").toBool());
        QCOMPARE(controller.games().size(), 1);
        QTest::keyClick(window, Qt::Key_F8);
        QTRY_VERIFY(removeDialog->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!removeDialog->property("visible").toBool());
        QCOMPARE(controller.games().size(), 1);
        auto* card = findVisual(window->contentItem(), "gameCard", findVisual);
        QVERIFY(card);
        const auto cardCenter = card->mapToScene(QPointF(card->width() / 2, card->height() / 2));
        QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, cardCenter.toPoint());
        QTRY_VERIFY(removeDialog->property("visible").toBool());
        const auto gamePath = controller.games().first().toMap().value("path").toString();
        QByteArray packageBytes;
        QByteArray fragmentBytes;
        { QFile file(gamePath); QVERIFY(file.open(QIODevice::ReadOnly)); packageBytes = file.readAll(); }
        { QFile file(gamePath + ".data/Data0000"); QVERIFY(file.open(QIODevice::ReadOnly)); fragmentBytes = file.readAll(); }
        const auto confirmCenter = confirmRemovalButton->mapToScene(QPointF(confirmRemovalButton->width() / 2,
                                                                           confirmRemovalButton->height() / 2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, confirmCenter.toPoint());
        QTRY_VERIFY(!removeDialog->property("visible").toBool());
        QTRY_COMPARE(controller.games().size(), 0);
        QVERIFY(!QFileInfo::exists(gamePath));
        QVERIFY(!QFileInfo::exists(gamePath + ".data"));
        QVERIFY(QDir().mkpath(gamePath + ".data"));
        { QFile file(gamePath); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(packageBytes), packageBytes.size()); }
        { QFile file(gamePath + ".data/Data0000"); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(fragmentBytes), fragmentBytes.size()); }
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 1);
        grid->setProperty("currentIndex", 0);
        grid->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_F8);
        QTRY_VERIFY(removeDialog->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_VERIFY(confirmRemovalButton->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(!removeDialog->property("visible").toBool());
        QTRY_COMPARE(controller.games().size(), 0);
        QVERIFY(!QFileInfo::exists(gamePath));
        QVERIFY(QDir().mkpath(gamePath + ".data"));
        { QFile file(gamePath); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(packageBytes), packageBytes.size()); }
        { QFile file(gamePath + ".data/Data0000"); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(fragmentBytes), fragmentBytes.size()); }
        controller.scan();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 1);
        grid->setProperty("currentIndex", 0);
        grid->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_F6);
        QTRY_VERIFY(coverPicker->property("visible").toBool());
        auto* coverOptions = coverPicker->findChild<QQuickItem*>("coverOptionsGrid");
        QVERIFY(coverOptions);
        QTRY_VERIFY(coverOptions->hasActiveFocus());
        QTRY_COMPARE(coverOptions->property("count").toInt(), 8);
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_COMPARE(coverOptions->property("currentIndex").toInt(), 1);
        QTest::keyClick(window, Qt::Key_Left);
        QTRY_COMPARE(coverOptions->property("currentIndex").toInt(), 0);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(!coverPicker->property("visible").toBool());
        QVERIFY(controller.coverImages().contains("4D5307D5"));
        QTest::keyClick(window, Qt::Key_F6);
        QTRY_VERIFY(coverPicker->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!coverPicker->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(controller.metro(), "editCover", Q_ARG(QVariantMap, controller.games()[0].toMap())));
        QTRY_VERIFY(consoleCoverPicker->property("visible").toBool());
        auto* consoleOptions = consoleCoverPicker->findChild<QQuickItem*>("coverOptionsGrid");
        QVERIFY(consoleOptions);
        QTRY_VERIFY(consoleOptions->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!consoleCoverPicker->property("visible").toBool());
        QVERIFY(!window->property("editingMetroCover").toBool());
        QTRY_VERIFY(grid->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY2(controller.players()->status().contains("Selecciona un perfil"), qPrintable(controller.players()->status()));
        auto* operationError = window->findChild<QObject*>("operationErrorDialog");
        QVERIFY(operationError);
        QTRY_VERIFY(operationError->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!operationError->property("visible").toBool());
        shell->setProperty("page", "achievements");
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_COMPARE(shell->property("page").toString(), QString("library"));
        shell->setProperty("page", "settings");
        auto* settingsScreen = findVisual(window->contentItem(), "settingsScreen", findVisual);
        QVERIFY(settingsScreen);
        QVERIFY(settingsScreen->setProperty("category", "app"));
        QTRY_VERIFY(findVisual(window->contentItem(), "appPreferences", findVisual));
        auto* appPreferences = findVisual(window->contentItem(), "appPreferences", findVisual);
        QVERIFY(appPreferences->setProperty("section", "about"));
        QTRY_VERIFY(findVisual(window->contentItem(), "aboutScreen", findVisual));
        auto* aboutScreen = findVisual(window->contentItem(), "aboutScreen", findVisual);
        QCOMPARE(aboutScreen->property("integratedProjects").toList().size(), 11);
        QVERIFY(appPreferences->setProperty("section", "preferences"));
        QTRY_VERIFY(findVisual(window->contentItem(), "appAppearance", findVisual));
        auto* carouselOption = findVisual(window->contentItem(), "libraryCarouselOption", findVisual);
        auto* gridOption = findVisual(window->contentItem(), "libraryGridOption", findVisual);
        QVERIFY(carouselOption); QVERIFY(gridOption);
        QVERIFY(carouselOption->width() > 80);
        const auto carouselCenter = carouselOption->mapToScene(QPointF(carouselOption->width() / 2,
                                                                        carouselOption->height() / 2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, carouselCenter.toPoint());
        QCOMPARE(controller.libraryView(), QStringLiteral("carousel"));
        const auto gridCenter = gridOption->mapToScene(QPointF(gridOption->width() / 2,
                                                                gridOption->height() / 2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, gridCenter.toPoint());
        QCOMPARE(controller.libraryView(), QStringLiteral("grid"));
        shell->setProperty("page", "library");
        QTRY_VERIFY(findVisual(window->contentItem(), "gameGrid", findVisual));
        grid = findVisual(window->contentItem(), "gameGrid", findVisual);
        search = findVisual(window->contentItem(), "librarySearch", findVisual);
        QTRY_VERIFY(grid->hasActiveFocus());
        // Returning from the embedded session must restore keyboard/gamepad
        // navigation even if focus moved away while the library was hidden.
        search->forceActiveFocus();
        QVERIFY(!grid->hasActiveFocus());
        auto* libraryScreen = grid->parentItem();
        libraryScreen->setVisible(false);
        window->contentItem()->forceActiveFocus();
        QVERIFY(!grid->hasActiveFocus());
        libraryScreen->setVisible(true);
        QTRY_VERIFY(grid->hasActiveFocus());
        search->setProperty("text", "nonexistent-title");
        QTRY_COMPARE(grid->property("count").toInt(), 0);
        search->setProperty("text", "4d5307d5");
        QTRY_COMPARE(grid->property("count").toInt(), 1);
        controller.setConsoleMode(true);
        // The old Qt console wrapper is no longer a full-screen page.
        QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        controller.setConsoleMode(false);
        QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        // Temporary full-screen views must restore the user's window state.
        window->setVisibility(QWindow::Maximized);
        QTRY_COMPARE(window->visibility(), QWindow::Maximized);
        QVERIFY(window->setProperty("editingMetroCover", true));
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
        QVERIFY(window->setProperty("editingMetroCover", false));
        QTRY_COMPARE(window->visibility(), QWindow::Maximized);
        window->setVisibility(QWindow::Windowed);
        QTest::keyClick(window, Qt::Key_F11);
        QTRY_VERIFY(controller.metro()->status().contains("Dashboard Metro"));
        auto* session = qobject_cast<QQuickItem*>(window->findChild<QObject*>("gameSession"));
        QVERIFY(session);
        QVERIFY(QMetaObject::invokeMethod(session, "toggleFullScreen"));
        QVERIFY(window->property("gameFullScreen").toBool());
        auto* toolbar = findVisual(window->contentItem(), "gameSessionToolbar", findVisual);
        auto* sessionStatus = findVisual(window->contentItem(), "gameSessionStatus", findVisual);
        auto* sessionContent = findVisual(window->contentItem(), "gameSessionContent", findVisual);
        QVERIFY(toolbar);
        QVERIFY(sessionStatus);
        QVERIFY(sessionContent);
        session->setVisible(true);
        QVERIFY(window->setProperty("gameChromeVisible", false));
        QTRY_VERIFY(!toolbar->isVisible());
        QTRY_VERIFY(!sessionStatus->isVisible());
        QTRY_VERIFY(sessionContent->height() >= session->height() - 2);
        QVERIFY(window->setProperty("gameChromeVisible", true));
        QTRY_VERIFY(toolbar->isVisible());
        session->setVisible(false);
        QVERIFY(!controller.consoleMode());
        QVERIFY(QMetaObject::invokeMethod(session, "browseLibrary"));
        QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        QVERIFY(QMetaObject::invokeMethod(session, "confirmClose"));
        auto* closeDialog = window->findChild<QObject*>("closeSessionDialog");
        QVERIFY(closeDialog);
        QTRY_VERIFY(closeDialog->property("visible").toBool());
        QTRY_VERIFY2(closeDialog->property("height").toReal() > 120,
                     qPrintable(QString("dialog height=%1 implicit=%2 window=%3x%4")
                         .arg(closeDialog->property("height").toReal())
                         .arg(closeDialog->property("implicitHeight").toReal())
                         .arg(window->width()).arg(window->height())));
        QVERIFY(closeDialog->property("width").toReal() <= window->width() - 48);
        const auto capture = qEnvironmentVariable("EMULOS_SESSION_CAPTURE");
        if (!capture.isEmpty()) { QTest::qWait(100); QVERIFY(window->grabWindow().save(capture)); }
        QVERIFY(QMetaObject::invokeMethod(closeDialog, "reject"));
        QVERIFY(QMetaObject::invokeMethod(controller.metro(), "showPage", Q_ARG(QString, QStringLiteral("library"))));
        QTRY_VERIFY(window->property("maximizeAfterMetro").toBool());
        QTRY_COMPARE(window->visibility(), QWindow::Maximized);
        controller.setReducedMotion(true);
        controller.setIntroStyle("nova");
        QCOMPARE(controller.introStyle(), QStringLiteral("nova"));
        QVERIFY(controller.introVideo().toLocalFile().endsWith("intro/nova.mp4"));
        Controller reopened(settings);
        QCOMPARE(reopened.libraryPath(), controller.libraryPath());
        QVERIFY(reopened.reducedMotion());
        QVERIFY(!reopened.consoleMode());
        QCOMPARE(reopened.introStyle(), QStringLiteral("nova"));
    }
    void carouselViewNavigationAndPersistence() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        for (quint32 index = 0; index < 5; ++index) {
            const auto folder = directory.filePath(QStringLiteral("games/%1/00007000")
                .arg(index + 1, 8, 16, QChar(u'0')));
            QVERIFY(QDir().mkpath(folder));
            QByteArray header(0x511, '\0'); header.replace(0, 4, "LIVE");
            qToBigEndian<quint32>(0x7000, header.data() + 0x344);
            qToBigEndian<quint32>(0x4D530700 + index, header.data() + 0x360);
            qToBigEndian<quint32>(1, header.data() + 0x39D);
            qToBigEndian<quint32>(1, header.data() + 0x3A9);
            QFile file(folder + "/game"); QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(header), header.size());
            QVERIFY(QDir().mkpath(folder + "/game.data"));
            QFile fragment(folder + "/game.data/Data0000"); QVERIFY(fragment.open(QIODevice::WriteOnly));
            QCOMPARE(fragment.write("x"), 1LL);
        }
        const auto settings = directory.filePath("settings.ini");
        Controller controller(settings);
        QCOMPARE(controller.libraryView(), QStringLiteral("grid"));
        controller.setLibraryPath(directory.filePath("games"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.games().size(), 5);
        controller.setLibraryView(QStringLiteral("carousel"));
        QCOMPARE(controller.libraryView(), QStringLiteral("carousel"));

        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}, {"introEnabled", false}});
        engine.loadFromModule("Los", "Main"); QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
        window->setVisibility(QWindow::Windowed); window->resize(1440, 900);
        auto* carousel = window->findChild<QQuickItem*>("gameCarousel");
        auto* path = window->findChild<QQuickItem*>("carouselPath");
        QVERIFY(carousel); QVERIFY(path);
        QTRY_COMPARE(path->property("count").toInt(), 5);
        QTRY_VERIFY(path->hasActiveFocus());
        QVERIFY(carousel->width() > 700);
        QCOMPARE(path->property("pathItemCount").toInt(), 3);
        path->setProperty("currentIndex", 0);
        QTest::keyClick(window, Qt::Key_Left);
        QTRY_COMPARE(path->property("currentIndex").toInt(), 4);
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_COMPARE(path->property("currentIndex").toInt(), 0);
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_COMPARE(path->property("currentIndex").toInt(), 1);
        window->resize(1920, 1032);
        QTRY_COMPARE(path->property("pathItemCount").toInt(), 5);
        auto* search = window->findChild<QQuickItem*>("librarySearch"); QVERIFY(search);
        const auto chosen = controller.games().at(2).toMap();
        search->setProperty("text", chosen.value("titleId").toString());
        QTRY_COMPARE(path->property("count").toInt(), 1);
        QCOMPARE(carousel->property("selectedGame").toMap().value("titleId").toString(),
                 chosen.value("titleId").toString());
        const auto capture = qEnvironmentVariable("EMULOS_CAROUSEL_CAPTURE");
        if (!capture.isEmpty()) {
            search->setProperty("text", "");
            QTRY_COMPARE(path->property("count").toInt(), 5);
            QTest::qWait(300);
            QVERIFY(window->grabWindow().save(capture));
        }
        Controller reopened(settings);
        QCOMPARE(reopened.libraryView(), QStringLiteral("carousel"));
        reopened.setLibraryView(QStringLiteral("grid"));
        QCOMPARE(reopened.libraryView(), QStringLiteral("grid"));
    }
};
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    UiTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "ui_tests.moc"

#include "netplay_rooms.h"
#include "core_window_host.h"
#include "player_services.h"
#include "src/xenia/kernel/xam/emulos_avatar_manifest.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QScreen>
#include <QDirIterator>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

class NetplayIntegrationTests : public QObject {
    Q_OBJECT
private slots:
    void originalAvatarProfileRoundTrip() {
        const auto core=qEnvironmentVariable("EMULOS_TEST_CORE");
        const auto catalog=qEnvironmentVariable("EMULOS_TEST_AVATAR_CATALOG");
        if(core.isEmpty()||catalog.isEmpty())QSKIP("Set core and private original avatar catalog paths");
        QTemporaryDir directory;QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath("avatar-system")));
        QVERIFY(QFile::copy(catalog,directory.filePath("avatar-system/AvatarAssetPack.toc")));
        const auto rig=qEnvironmentVariable("EMULOS_TEST_AVATAR_RIG");
        if(!rig.isEmpty())QVERIFY(QFile::copy(rig,directory.filePath("avatar-system/avatar-skeleton.bin")));
        EngineSettings settings(directory.filePath("engine"));
        QVERIFY(settings.setValue("Live.network_mode","0"));QVERIFY(settings.setValue("General.discord",false));QVERIFY(settings.save());
        PlayerServices players(&settings,directory.path(),nullptr,core);
        QSignalSpy failures(&players,&PlayerServices::operationFailed);
        players.createProfile("AvatarCheck");QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);
        QVERIFY2(failures.isEmpty(),qPrintable(players.status()));QCOMPARE(players.profiles().size(),1);
        const auto xuid=players.profiles().first().toMap()["xuid"].toString();
        players.createProfile("AvatarOther");QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);QCOMPARE(players.profiles().size(),2);
        players.editAvatar(xuid);QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);
        auto* studio=players.avatarStudio();QVERIFY2(studio->ready(),qPrintable(studio->status()));
        QVERIFY(studio->dirty());QCOMPARE(studio->body(),1);QVERIFY(!studio->parts().empty());
        if(!rig.isEmpty()) {
            QVERIFY(studio->animated());auto* mesh=studio->parts().first().toMap()["geometry"].value<QQuick3DGeometry*>();QVERIFY(mesh);
            const auto before=mesh->vertexData();studio->setPreviewActive(true);QTest::qWait(150);QVERIFY(mesh->vertexData()!=before);studio->setPreviewActive(false);
        }
        studio->setBody(2);QVERIFY2(studio->ready(),qPrintable(studio->status()));QCOMPARE(studio->body(),2);
        studio->setCategory(0);QVERIFY(studio->choices().size()>3);
        studio->choose(studio->choices()[2].toMap()["id"].toString());QVERIFY2(studio->ready(),qPrintable(studio->status()));
        studio->setColor(0,QColor("#a67553"));
        QSignalSpy saved(studio,&AvatarStudio::saveRequested);
        studio->save();QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);
        QVERIFY2(failures.isEmpty(),qPrintable(players.status()));QVERIFY(!studio->dirty());QCOMPARE(saved.size(),1);
        QCOMPARE(players.activeXuid(),xuid);
        const QByteArray expected=saved.first()[1].toByteArray();QCOMPARE(expected.size(),1000);
        players.editAvatar(xuid);QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);
        QVERIFY2(studio->ready(),qPrintable(studio->status()));QCOMPARE(studio->body(),2);QVERIFY(!studio->dirty());
        QCOMPARE(studio->colors()[0].value<QColor>(),QColor("#a67553"));
        studio->save();QTRY_VERIFY_WITH_TIMEOUT(!players.busy(),60000);
        QCOMPARE(saved.size(),2);QCOMPARE(saved.last()[1].toByteArray(),expected);
        QVERIFY2(failures.isEmpty(),qPrintable(players.status()));
        QVERIFY(!QDir(directory.filePath("engine/profile-backups")).entryList(QDir::Dirs|QDir::NoDotAndDotDot).empty());
        const auto output=qEnvironmentVariable("EMULOS_TEST_AVATAR_MANIFEST");
        if(!output.isEmpty()){QFile file(output);QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(expected),1000);}
    }
    void originalAvatarManifest() {
        using xe::kernel::xam::emulos::AvatarBodyType;
        // Serialized asset identities from the original format (see AVATAR_ABI).
        QByteArray manifest(1000, '\0');
        auto bodyType = [&] { return AvatarBodyType({reinterpret_cast<const uint8_t*>(manifest.constData()), static_cast<size_t>(manifest.size())}); };
        QCOMPARE(bodyType(), 0);
        manifest.replace(0x120, 16, QByteArray::fromHex("0000000200000001c1c8f109a19cb2e0"));
        QCOMPARE(bodyType(), 1);
        manifest.replace(0x120, 16, QByteArray::fromHex("0000000200010002c1c8f109a19cb2e0"));
        QCOMPARE(bodyType(), 2);
        manifest[0x12F] = 0; QCOMPARE(bodyType(), 0);
        manifest.resize(0x12F); QCOMPARE(bodyType(), 0);
        manifest.clear(); QCOMPARE(bodyType(), 0);
    }
    void advertisedSessions() {
        QVariantList rows; QString error;
        const QByteArray json = R"({"Titles":[{"titleId":"4d53082d","name":"Gears","sessions":[{"mediaId":"12345678","version":"1.0","players":[{},{}],"total":4,"host_gamertag":"Example","host_presence":"Lobby"}]}]})";
        QVERIFY(NetplayRooms::decode(json, rows, error));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows[0].toMap()["titleId"].toString(), QString("4D53082D"));
        QCOMPARE(rows[0].toMap()["players"].toInt(), 2);
        QVERIFY(!NetplayRooms::decode("<html>offline</html>", rows, error));
        QVERIFY(rows.isEmpty());
        QVERIFY(!NetplayRooms::decode(R"({"Titles":[{"titleId":"../bad","sessions":[]}]})", rows, error));
        QVERIFY(NetplayRooms::decode(R"({"Titles":[]})", rows, error));
        QVERIFY(rows.isEmpty());
        QVERIFY(!NetplayRooms::decode(QByteArray(8*1024*1024+1, 'x'), rows, error));
    }
    void requestAndCancel() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        EngineSettings settings(directory.path());
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
        const QString endpoint = QString("http://127.0.0.1:%1/base/").arg(server.serverPort());
        QVERIFY(settings.setValue("Live.api_address", endpoint)); QVERIFY(settings.save());
        NetplayRooms rooms(&settings);
        QVERIFY(settings.setValue("GPU.vsync", false)); // Must not block read-only browsing.
        QVERIFY(settings.dirty());
        QByteArray received;
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, socket, [&,socket] {
                received += socket->readAll();
                if (received.contains("\r\n\r\n")) {
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 13\r\nConnection: close\r\n\r\n{\"Titles\":[]}");
                    socket->disconnectFromHost();
                }
            });
        });
        rooms.refresh(); QVERIFY(rooms.busy());
        QTRY_VERIFY_WITH_TIMEOUT(!rooms.busy(), 5000);
        QVERIFY(received.startsWith("GET /base/sessions HTTP/1.1"));
        QVERIFY(rooms.status().startsWith("0 sesiones"));
        rooms.refresh(); rooms.cancel(); QVERIFY(!rooms.busy());
        QVERIFY(rooms.rooms().isEmpty());
    }
    void profileConfigurationRoundTrip() {
        const auto executable = qEnvironmentVariable("EMULOS_TEST_CORE");
        if (executable.isEmpty()) QSKIP("Set EMULOS_TEST_CORE to exercise the real profile bridge");
        QTemporaryDir directory; QVERIFY(directory.isValid());
        EngineSettings settings(directory.filePath("engine"));
        QVERIFY(settings.setValue("Live.network_mode", "0"));
        QVERIFY(settings.setValue("General.discord", false));
        QVERIFY(settings.save());
        PlayerServices players(&settings, directory.path(), nullptr, executable);
        QSignalSpy failures(&players, &PlayerServices::operationFailed);
        players.createProfile("EmulosCheck");
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 60000);
        QVERIFY2(failures.isEmpty(), qPrintable(players.status()));
        QCOMPARE(players.profiles().size(), 1);
        const auto xuid = players.profiles().first().toMap()["xuid"].toString();
        QCOMPARE(players.activeXuid(), xuid);
        QVERIFY(!settings.dirty());
        EngineSettings reopened(directory.filePath("engine"));
        QCOMPARE(reopened.values()["Profiles.logged_profile_slot_0_xuid"].toString(), xuid);
        players.refreshProfiles();
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 60000);
        QCOMPARE(players.activeXuid(), xuid);
        QVERIFY(players.selectProfile(xuid));
        QVERIFY(settings.setValue("Live.network_mode", "2"));
        QVERIFY2(settings.save(), qPrintable(settings.status()));
        QVERIFY(settings.reload());
        QCOMPARE(settings.values()["Live.network_mode"].toString(), QString("2"));
        QVERIFY2(failures.isEmpty(), qPrintable(players.status()));
        QVERIFY(!players.profiles().first().toMap()["netplay"].toBool());
        // Startup discovery in Netplay mode also repairs an existing local-only
        // profile, without changing its local identity or requiring the core UI.
        players.refreshProfiles();
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 60000);
        QVERIFY2(players.profiles().first().toMap()["netplay"].toBool(), qPrintable(players.status()));
        QCOMPARE(players.activeXuid(), xuid);
        QVERIFY(!QDir(directory.filePath("engine/profile-backups")).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
        const auto backups = QDir(directory.filePath("engine/profile-backups")).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        players.enableNetplayProfile(xuid);
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 60000);
        QVERIFY(players.profiles().first().toMap()["netplay"].toBool());
        QCOMPARE(QDir(directory.filePath("engine/profile-backups")).entryList(QDir::Dirs | QDir::NoDotAndDotDot), backups);
        QVERIFY(!settings.dirty());

        const auto game = qEnvironmentVariable("EMULOS_TEST_GAME");
        if (!game.isEmpty()) {
            QVERIFY(settings.setValue("Live.network_mode", "0"));
            QVERIFY(settings.save());
            QQmlEngine engine;
            engine.rootContext()->setContextProperty("coreHost", players.coreWindow());
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nWindow { width: 1024; height: 720; visible: true; title: 'Emulos360 · comprobación de arranque'; WindowContainer { anchors.fill: parent; window: coreHost.window } }", QUrl());
            std::unique_ptr<QObject> root(component.create());
            QVERIFY2(root != nullptr, qPrintable(component.errorString()));
            players.launchGame(game);
            QTRY_VERIFY_WITH_TIMEOUT(players.coreWindow()->window() != nullptr || !players.busy(), 45000);
            QVERIFY2(players.sessionActive(), qPrintable(players.status()));
            QVERIFY(players.coreWindow()->window());
            players.coreWindow()->focus();
            QTest::qWait(45000);
            QVERIFY2(players.sessionActive(), qPrintable(players.status()));
            const auto report = qEnvironmentVariable("EMULOS_TEST_REPORT");
            if (!report.isEmpty()) {
                QGuiApplication::primaryScreen()->grabWindow(qobject_cast<QWindow*>(root.get())->winId()).save(report + ".png");
#ifdef Q_OS_WIN
                // Capture the guest GPU output via the core's own F12 action.
                // Qt's parent-window screenshot excludes foreign GPU children.
                const auto since = QDateTime::currentDateTime();
                const auto hwnd = reinterpret_cast<HWND>(players.coreWindow()->window()->winId());
                PostMessageW(hwnd, WM_KEYDOWN, VK_F12, 0);
                PostMessageW(hwnd, WM_KEYUP, VK_F12, 0);
                QString captured;
                auto findCapture = [&] {
                    QDirIterator files(QFileInfo(executable).absolutePath() + "/screenshots", {"*.png"}, QDir::Files, QDirIterator::Subdirectories);
                    while (files.hasNext()) { files.next(); if (files.fileInfo().lastModified() >= since) { captured = files.filePath(); return true; } }
                    return false;
                };
                QTRY_VERIFY_WITH_TIMEOUT(findCapture(), 15000);
                QTRY_VERIFY_WITH_TIMEOUT(!QImage(captured).isNull(), 10000);
                QVERIFY(QFile::copy(captured, report + "-guest.png"));
#endif
                QVERIFY(QFile::copy(directory.filePath("core.log"), report + ".log"));
            }
            players.coreWindow()->requestClose();
            QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 20000);
            QVERIFY2(failures.isEmpty(), qPrintable(players.status()));
        }
    }
    void publicRoomServer() {
        const auto endpoint = qEnvironmentVariable("EMULOS_TEST_ROOM_SERVER");
        if (endpoint.isEmpty()) QSKIP("Set EMULOS_TEST_ROOM_SERVER for a live HTTPS request");
        QTemporaryDir directory;
        EngineSettings settings(directory.path());
        QVERIFY(settings.setValue("Live.api_address", endpoint));
        NetplayRooms rooms(&settings);
        rooms.refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!rooms.busy(), 100000);
        qInfo().noquote() << rooms.status();
        QVERIFY2(rooms.status().contains("sesiones anunciadas"), qPrintable(rooms.status()));
    }
    void nativeCoreWindow() {
#ifdef Q_OS_WIN
        const auto executable = qEnvironmentVariable("EMULOS_TEST_CORE");
        if (executable.isEmpty()) QSKIP("Set EMULOS_TEST_CORE for the focused native-window integration check");
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QFile config(directory.filePath("test.toml")); QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("[Live]\nnetwork_mode=0\nupnp=false\n[General]\ndiscord=false\nauto_check_updates=false\n"); config.close();
        CoreWindowHost host;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("coreHost", &host);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nWindow { width: 800; height: 600; visible: true; WindowContainer { anchors.fill: parent; window: coreHost.window } }", QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root != nullptr, qPrintable(component.errorString()));
        auto window = qobject_cast<QWindow*>(root.get()); QVERIFY(window);
        QProcess process;
        process.setWorkingDirectory(directory.path());
        connect(&process, &QProcess::finished, &host, &CoreWindowHost::clear);
        process.start(executable, {"--config=" + config.fileName(), "--storage_root=" + directory.path(), "--log_file=" + directory.filePath("core.log"), "--emulos_embedded=true", "--fullscreen=false", "--network_mode=0", "--upnp=false", "--discord=false"});
        QVERIFY(process.waitForStarted()); host.watch(process.processId());
        // Dismiss only this empty test core's optional quickstart-link prompt.
        // Fresh sandbox users do not retain its HKCU acknowledgement.
        QTimer onboarding;
        connect(&onboarding, &QTimer::timeout, this, [&] {
            DWORD corePid = static_cast<DWORD>(process.processId());
            EnumWindows([](HWND hwnd, LPARAM data) -> BOOL {
                DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
                if (pid != *reinterpret_cast<DWORD*>(data)) return TRUE;
                wchar_t name[64]{}, title[64]{};
                GetClassNameW(hwnd, name, 64); GetWindowTextW(hwnd, title, 64);
                if (wcscmp(name, L"#32770") == 0 && wcscmp(title, L"Xenia") == 0 && GetDlgItem(hwnd, IDNO))
                    PostMessageW(hwnd, WM_COMMAND, IDNO, 0);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&corePid));
        });
        onboarding.start(100);
        QTRY_VERIFY_WITH_TIMEOUT(host.window() != nullptr, 45000);
        onboarding.stop();
        const auto child = reinterpret_cast<HWND>(host.window()->winId());
        QTRY_VERIFY(GetParent(child) == reinterpret_cast<HWND>(window->winId()));
        QVERIFY(GetWindowLongPtr(child, GWL_STYLE) & WS_CHILD);
        window->resize(1000, 700);
        RECT rect{};
        QTRY_VERIFY((GetClientRect(child, &rect) && rect.right >= 990 && rect.bottom >= 650));
        window->hide(); QTRY_VERIFY(!IsWindowVisible(child));
        window->show(); QTRY_VERIFY(IsWindowVisible(child));
        window->requestActivate();
        host.focus();
        GUITHREADINFO input{};
        input.cbSize = sizeof(input);
        const auto childThread = GetWindowThreadProcessId(child, nullptr);
        QTRY_VERIFY(GetGUIThreadInfo(childThread, &input) && input.hwndFocus == child);
        // Repeat after returning focus to the Qt toolbar's input queue.
        SetFocus(reinterpret_cast<HWND>(window->winId()));
        host.focus();
        QTRY_VERIFY(GetGUIThreadInfo(childThread, &input) && input.hwndFocus == child);
        // The native window is published before the renderer finishes setup.
        // Do not close the empty test process halfway through GPU startup.
        QTest::qWait(3000);
        host.requestClose();
        QTRY_COMPARE_WITH_TIMEOUT(process.state(), QProcess::NotRunning, 15000);
        const auto nativeReport = qEnvironmentVariable("EMULOS_TEST_REPORT");
        if (!nativeReport.isEmpty()) {
            QFile log(directory.filePath("core.log"));
            QFile report(nativeReport + ".native.log");
            if (log.open(QIODevice::ReadOnly) && report.open(QIODevice::WriteOnly)) report.write(log.readAll());
        }
        qInfo() << "Core exit code:" << process.exitCode();
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QVERIFY(host.window() == nullptr);
#else
        QSKIP("Native embedding is implemented for Windows");
#endif
    }
};
QTEST_MAIN(NetplayIntegrationTests)
#include "netplay_integration_tests.moc"

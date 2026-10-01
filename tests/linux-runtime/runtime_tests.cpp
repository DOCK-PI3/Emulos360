#include "avatar_studio.h"
#include "engine_settings.h"
#include "game_importer.h"
#include "library.h"
#include "player_services.h"
#include "runtime_paths.h"
#include "console_config.h"
#include "xenia/kernel/util/emulos_posix_socket_error.h"
#include "xenia/kernel/xconfig.h"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
#include <csignal>
#include <bit>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace {
void write(const QString& path, const QByteArray& bytes) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) qFatal("Cannot write fixture");
}
QByteArray read(const QString& path) {
    QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
}

class LinuxRuntimeTests : public QObject {
    Q_OBJECT
private slots:
    void closeFrontendWithRunningCore() {
        QTemporaryDir data; QVERIFY(data.isValid());
        const auto core = data.filePath("test-core");
        write(core, "#!/bin/sh\nexec sleep 30\n");
        QVERIFY(QFile::setPermissions(core, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        EngineSettings settings(data.filePath("engine"));
        {
            PlayerServices players(&settings, data.path(), nullptr, core);
            players.refreshProfiles();
            QTest::qWait(150);
            QVERIFY(players.busy());
        }
        QVERIFY(!settings.locked());
    }
    void packedConsoleSettingsPreserveBytes() {
        const auto original = los::consoleDefaults();
        const QVariantMap changes{{"Console.language", "5"}, {"Console.music_volume", "0.35"},
                                  {"Console.profile", "E0000123456789AB"}};
        const auto edited = los::writeConsole(original, changes);
        auto expected = original;
        using Data = xe::kernel::XConfigData;
        const size_t user = offsetof(Data, user);
        qToBigEndian<quint32>(5, expected.data() + user + offsetof(Data::User, language));
        qToBigEndian<quint32>(std::bit_cast<quint32>(0.35f), expected.data() + user + offsetof(Data::User, music_volume));
        qToBigEndian<quint64>(0xE0000123456789AB, expected.data() + user + offsetof(Data::User, default_profile));
        QCOMPARE(edited, expected);
        const auto values = los::readConsole(edited);
        QCOMPARE(values.value("Console.language").toString(), QString("5"));
        QCOMPARE(values.value("Console.profile").toString(), QString("E0000123456789AB"));
    }
    void posixSocketErrorsMatchGuestContract() {
        const int first = ::socket(AF_INET, SOCK_STREAM, 0);
        const int second = ::socket(AF_INET, SOCK_STREAM, 0);
        QVERIFY(first >= 0 && second >= 0);
        sockaddr_in address{}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        QCOMPARE(::bind(first, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
        socklen_t size = sizeof(address);
        QCOMPARE(::getsockname(first, reinterpret_cast<sockaddr*>(&address), &size), 0);
        QCOMPARE(::bind(second, reinterpret_cast<sockaddr*>(&address), size), -1);
        const auto error = errno;
        ::close(second); ::close(first);
        QCOMPARE(error, EADDRINUSE);
        QCOMPARE(xe::kernel::EmulosPosixSocketError(error), quint32(10048));
        QCOMPARE(xe::kernel::EmulosPosixSocketError(EACCES), quint32(10013));
        QCOMPARE(xe::kernel::EmulosPosixSocketError(EWOULDBLOCK), quint32(10035));
        QCOMPARE(xe::kernel::EmulosPosixSocketError(ECONNREFUSED), quint32(10061));
        QCOMPARE(xe::kernel::EmulosPosixSocketError(10013), quint32(10013));
    }
    void archiveWithLostExecutePermission() {
        const auto app = QCoreApplication::applicationDirPath();
        const auto bundled = app + "/tools/7z";
        QVERIFY2(!QFileInfo::exists(bundled), "Use an isolated test build directory");
        write(bundled, "#!/bin/sh\nexit 91\n");
        QVERIFY(QFile::setPermissions(bundled, QFile::ReadOwner | QFile::WriteOwner));
        QVERIFY(!QFileInfo(bundled).isExecutable());
        const auto extractor = emulos::findImportTool("7z.exe");
        QVERIFY(!extractor.isEmpty()); QVERIFY(extractor != bundled);
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto package = temporary.filePath("xbla-package");
        QByteArray header(0x520, '\0'); header.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0xD0000, header.data() + 0x344);
        qToBigEndian<quint32>(0x58410A71, header.data() + 0x360);
        write(package, header);
        const auto archive = temporary.filePath("fixture.7z");
        QCOMPARE(QProcess::execute(extractor, {"a", "-t7z", archive, package}), 0);
        const auto library = temporary.filePath("library"); QVERIFY(QDir().mkpath(library));
        GameImporter importer; importer.importFile(QUrl::fromLocalFile(archive), library);
        QTRY_VERIFY_WITH_TIMEOUT(!importer.busy(), 10000);
        QVERIFY2(importer.status().contains("instalado"), qPrintable(importer.status()));
        QCOMPARE(los::scanLibrary(library).games.size(), 1);
        QVERIFY(QFileInfo::exists(archive)); QVERIFY(QFileInfo::exists(package));
        QVERIFY(QFile::remove(bundled));
    }
    void windowsBackendsUseLinuxChoices() {
        QTemporaryDir data; QVERIFY(data.isValid());
        const auto config = data.filePath("xenia-canary.config.toml");
        const QByteArray bytes("[GPU]\ngpu='d3d12'\n[APU]\napu='xaudio2'\n[HID]\nhid='xinput'\n");
        write(config, bytes);
        EngineSettings settings(data.path());
        QCOMPARE(settings.values().value("GPU.gpu").toString(), QString("vulkan"));
        QCOMPARE(settings.values().value("APU.apu").toString(), QString("any"));
        QCOMPARE(settings.values().value("HID.hid").toString(), QString("any"));
        QVERIFY(!settings.setValue("GPU.gpu", "d3d12"));
        QCOMPARE(read(config), bytes);
    }
    void packagedAvatarWithEmptyUserData() {
        QTemporaryDir data; QVERIFY(data.isValid());
        AvatarStudio studio(data.path()); studio.openProfile({}, {});
        QVERIFY2(studio.ready(), qPrintable(studio.status()));
        QVERIFY(studio.animated());
        QVERIFY(studio.resourceRoot().endsWith("/assets/avatar-system"));
        QVERIFY(!QFileInfo::exists(data.filePath("avatar-system")));
        // An explicitly imported catalog remains the user's choice.
        write(data.filePath("avatar-system/AvatarAssetPack.toc"), "invalid");
        AvatarStudio imported(data.path()); imported.openProfile({}, {});
        QVERIFY(!imported.ready());
        QCOMPARE(imported.resourceRoot(), data.filePath("avatar-system"));
    }
    void realProfileAndGameLaunch() {
        const auto core = qEnvironmentVariable("EMULOS_LINUX_CORE");
        const auto target = qEnvironmentVariable("EMULOS_LINUX_GAME");
        if (core.isEmpty() || target.isEmpty()) QSKIP("Set the native core and local game paths");
        QTemporaryDir data; QVERIFY(data.isValid());
        EngineSettings settings(data.filePath("engine"));
        QVERIFY(settings.setValue("Live.network_mode", "0"));
        QVERIFY(settings.setValue("Live.upnp", false));
        QVERIFY(settings.save());
        PlayerServices players(&settings, data.path(), nullptr, core);
        players.createProfile("LinuxTest");
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 60000);
        QVERIFY2(!players.profiles().isEmpty(), qPrintable(players.status()));
        QVERIFY(players.selectProfile(players.profiles().first().toMap().value("xuid").toString()));
        players.launchGame(target, qEnvironmentVariable("EMULOS_LINUX_GAME_TITLE"));
        QTRY_VERIFY_WITH_TIMEOUT(players.sessionProcessId() > 0, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(read(data.filePath("core.log")).contains("VulkanPresenter: Created"), 20000);
        QTest::qWait(25000);
        QVERIFY2(players.sessionActive(), qPrintable(players.status()));
        const auto log = read(data.filePath("core.log"));
        QVERIFY(log.contains("XThread::Execute"));
        QVERIFY(!log.contains("Failed to setup emulator"));
        const auto destination = qEnvironmentVariable("EMULOS_LINUX_TEST_LOG_DIR");
        if (!destination.isEmpty()) {
            write(destination + "/game-core.log", log);
            write(destination + "/game-core-console.log", read(data.filePath("core-console.log")));
            const auto capture = qEnvironmentVariable("EMULOS_LINUX_CAPTURE_SCRIPT");
            if (!capture.isEmpty()) {
                QCOMPARE(QProcess::execute("/usr/bin/python3", {capture, QString::number(players.sessionProcessId()), destination + "/game-linux.png", "--close"}), 0);
                QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 15000);
                return;
            }
        }
        ::kill(pid_t(players.sessionProcessId()), SIGTERM);
        QTest::qWait(1000);
        if (players.sessionActive()) ::kill(pid_t(players.sessionProcessId()), SIGKILL);
        QTRY_VERIFY_WITH_TIMEOUT(!players.busy(), 10000);
    }
};

QTEST_MAIN(LinuxRuntimeTests)
#include "runtime_tests.moc"

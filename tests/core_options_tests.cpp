#include "core_options.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

namespace {
bool put(const QString& path, const QByteArray& bytes) {
    QDir().mkpath(QFileInfo(path).absolutePath()); QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray read(const QString& path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
}
class CoreOptionsTests : public QObject {
    Q_OBJECT
private slots:
    void selectionPersistsAndOverridesOnlyItsTitle() {
        QTemporaryDir directory;
        CoreOptions cores(directory.path(), nullptr, directory.path());
        QCOMPARE(cores.defaultCore(), QStringLiteral("netplay"));
        QCOMPARE(cores.rendererFor("545408B4"), QString{});
        cores.setDefaultCore("edge"); cores.setDefaultRenderer("vulkan");
        QVERIFY(cores.saveGameOptions("545408b4", "netplay", ""));
        QCOMPARE(cores.coreFor("545408B4"), QStringLiteral("netplay"));
        QCOMPARE(cores.rendererFor("545408B4"), QString{});
        QCOMPARE(cores.coreFor("454109C6"), QStringLiteral("edge"));
        QCOMPARE(cores.rendererFor("454109C6"), QStringLiteral("vulkan"));
        CoreOptions reopened(directory.path(), nullptr, directory.path());
        QCOMPARE(reopened.coreFor("545408B4"), QStringLiteral("netplay"));
        QVERIFY(reopened.saveGameOptions("545408B4", "", ""));
        QCOMPARE(reopened.coreFor("545408B4"), QStringLiteral("edge"));
        QCOMPARE(reopened.rendererFor("545408B4"), QStringLiteral("vulkan"));
    }
    void lockedOrInvalidSelectionsDoNotAlterPreferences() {
        QTemporaryDir directory; CoreOptions cores(directory.path(), nullptr, directory.filePath("app"));
        QVERIFY(!cores.saveGameOptions("../engine", "edge", "vulkan"));
        QVERIFY(!cores.saveGameOptions("545408B4", "unknown", ""));
        QVERIFY(!cores.saveGameOptions("545408B4", "edge", "unknown"));
        cores.setLocked(true); cores.setDefaultCore("edge");
        QVERIFY(!cores.saveGameOptions("545408B4", "edge", "vulkan"));
        QCOMPARE(cores.defaultCore(), QStringLiteral("netplay"));
        QVERIFY(!cores.gameOptions("545408B4")["available"].toBool());
        QVERIFY(cores.executable("../../elsewhere").isEmpty());
    }
    void configIsIndependentAndPreservesNativeEdits() {
        QTemporaryDir directory; CoreOptions cores(directory.path(), nullptr, directory.filePath("app"));
        const auto original = directory.filePath("engine/xenia-canary.config.toml");
        QVERIFY(put(original, "original config\n"));
        QVERIFY(cores.ensureEdgeConfig(5, true));
        QVERIFY(read(cores.edgeConfig()).contains("show_achievement_notification = true"));
        QVERIFY(put(cores.edgeConfig(), "native Edge options\n"));
        QVERIFY(cores.ensureEdgeConfig(1, false));
        QCOMPARE(read(cores.edgeConfig()), QByteArray("native Edge options\n"));
        QCOMPARE(read(original), QByteArray("original config\n"));
    }
    void importCopiesContentAndNeverReplacesProgress() {
        QTemporaryDir directory;
        const auto source = directory.filePath("current");
        const auto destination = directory.filePath("edge/content");
        const QString xuid = "E000000000000001", title = "545408B4";
        const auto profile = xuid + "/FFFE07D1/00010000/" + xuid + "/Account";
        const auto save = xuid + '/' + title + "/00000001/Save";
        const auto dlc = "0000000000000000/" + title + "/00000002/DLC";
        QVERIFY(put(source + '/' + profile, "profile"));
        QVERIFY(put(source + '/' + save, "old save"));
        QVERIFY(put(source + '/' + dlc, "content"));
        QCOMPARE(emulos::seedEdgeContent(source, {}, destination, xuid, title), QString{});
        QCOMPARE(read(destination + '/' + profile), QByteArray("profile"));
        QCOMPARE(read(destination + '/' + dlc), QByteArray("content"));
        QVERIFY(put(destination + '/' + save, "new Edge progress"));
        QCOMPARE(emulos::seedEdgeContent(source, {}, destination, xuid, title), QString{});
        QCOMPARE(read(destination + '/' + save), QByteArray("new Edge progress"));
        QCOMPARE(read(source + '/' + save), QByteArray("old save"));
        QCOMPARE(emulos::seedEdgeContent(source, {}, destination, xuid, "454109C6"), QString{});
        QCOMPARE(read(destination + '/' + save), QByteArray("new Edge progress"));
        QVERIFY(!emulos::seedEdgeContent(source, {}, destination, "invalid", title).isEmpty());
        QVERIFY(!emulos::seedEdgeContent(source, {}, source + "/nested", xuid, title).isEmpty());
    }
};
QTEST_GUILESS_MAIN(CoreOptionsTests)
#include "core_options_tests.moc"

#include "core_options.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

class CoreOptionsTests : public QObject {
    Q_OBJECT
private slots:
    void rendererPreferencesPersistByTitle() {
        QTemporaryDir directory; CoreOptions options(directory.path());
        QCOMPARE(options.rendererFor("545408B4"), QString{});
        options.setDefaultRenderer("vulkan");
        QCOMPARE(options.rendererFor("545408B4"), QString("vulkan"));
        QVERIFY(options.saveGameOptions("545408b4", ""));
        CoreOptions reopened(directory.path());
        QCOMPARE(reopened.defaultRenderer(), QString("vulkan"));
#ifdef Q_OS_WIN
        QVERIFY(reopened.saveGameOptions("545408B4", "d3d12"));
        QCOMPARE(reopened.rendererFor("545408B4"), QString("d3d12"));
        QCOMPARE(reopened.rendererFor("454109C6"), QString("vulkan"));
#endif
        QVERIFY(reopened.saveGameOptions("545408B4", ""));
        QCOMPARE(reopened.rendererFor("545408B4"), QString("vulkan"));
    }
    void invalidAndLockedChangesAreRejected() {
        QTemporaryDir directory; CoreOptions options(directory.path());
        QVERIFY(!options.saveGameOptions("../engine", "vulkan"));
        QVERIFY(!options.saveGameOptions("545408B4", "unknown"));
        options.setDefaultRenderer("unknown"); QCOMPARE(options.defaultRenderer(), QString{});
        options.setLocked(true); options.setDefaultRenderer("vulkan");
        QVERIFY(!options.saveGameOptions("545408B4", "vulkan"));
        QCOMPARE(options.defaultRenderer(), QString{});
        QVERIFY(!QFileInfo::exists(directory.filePath("cores.ini")));
    }
    void retiredCoreSelectionsAreBackedUpAndMainGpuPreserved() {
        QTemporaryDir directory; const auto path = directory.filePath("cores.ini");
        {
            QSettings legacy(path, QSettings::IniFormat);
            legacy.setValue("defaultCore", "edge"); legacy.setValue("renderers/edge", "vulkan");
#ifdef Q_OS_WIN
            legacy.setValue("renderers/netplay", "d3d12");
#endif
            legacy.setValue("games/545408B4/core", "edge");
            legacy.setValue("games/545408B4/renderer", "vulkan"); legacy.sync();
        }
        QFile original(path); QVERIFY(original.open(QIODevice::ReadOnly)); const auto bytes=original.readAll(); original.close();
        CoreOptions options(directory.path());
        QCOMPARE(options.rendererFor("545408B4"), QString("vulkan"));
#ifdef Q_OS_WIN
        QCOMPARE(options.defaultRenderer(), QString("d3d12"));
#else
        QCOMPARE(options.defaultRenderer(), QString{});
#endif
        QSettings migrated(path, QSettings::IniFormat);
        QVERIFY(!migrated.contains("defaultCore")); QVERIFY(!migrated.contains("renderers/edge"));
        QVERIFY(!migrated.contains("games/545408B4/core"));
        QFile backup(path+".before-single-core"); QVERIFY(backup.open(QIODevice::ReadOnly)); QCOMPARE(backup.readAll(),bytes);
        CoreOptions reopened(directory.path()); QCOMPARE(reopened.rendererFor("545408B4"),QString("vulkan"));
        QCOMPARE(QDir(directory.path()).entryList({"cores.ini.before-single-core*"},QDir::Files).size(),1);
    }
    void migrationDoesNotCreateOrChangeContent() {
        QTemporaryDir directory;
        for (const auto& relative : {QString("engine/content/profile/Account"),QString("cores/edge/content/save"),QString("cores/xblig/saves/game/native/save")}) {
            const auto path=directory.filePath(relative); QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
            QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(relative.toUtf8());
        }
        { QSettings legacy(directory.filePath("cores.ini"),QSettings::IniFormat); legacy.setValue("defaultCore","edge"); legacy.sync(); }
        CoreOptions options(directory.path()); options.setDefaultRenderer("vulkan");
        for (const auto& relative : {QString("engine/content/profile/Account"),QString("cores/edge/content/save"),QString("cores/xblig/saves/game/native/save")}) {
            QFile file(directory.filePath(relative)); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),relative.toUtf8());
        }
    }
};
QTEST_GUILESS_MAIN(CoreOptionsTests)
#include "core_options_tests.moc"

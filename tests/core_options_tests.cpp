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
    void graphicsProfilesPersistAndPerTitleOverridesAreIndependent() {
        QTemporaryDir directory; CoreOptions options(directory.path());
        QCOMPARE(options.defaultGraphicsProfile(), QString("custom"));
        QVERIFY(options.graphicsArgumentsFor("545408B4").isEmpty());
        QVERIFY(!QFileInfo::exists(directory.filePath("cores.ini")));
        options.setDefaultRenderer("vulkan"); options.setDefaultGraphicsProfile("4k-quality");
        QVERIFY(options.saveGameOptions("545408b4", "", "720p"));
        QCOMPARE(options.graphicsProfileFor("545408B4"), QString("720p"));
        QCOMPARE(options.graphicsProfileFor("454109C6"), QString("4k-quality"));
        QCOMPARE(options.rendererFor("545408B4"), QString("vulkan"));
        CoreOptions reopened(directory.path());
        QCOMPARE(reopened.defaultGraphicsProfile(), QString("4k-quality"));
        QCOMPARE(reopened.graphicsProfileFor("545408b4"), QString("720p"));
        QVERIFY(reopened.saveGameOptions("545408B4", "vulkan", "custom"));
        QVERIFY(reopened.graphicsArgumentsFor("545408B4").isEmpty());
        QVERIFY(reopened.saveGameOptions("545408B4", "", ""));
        QCOMPARE(reopened.graphicsProfileFor("545408B4"), QString("4k-quality"));
        QSettings ini(directory.filePath("cores.ini"), QSettings::IniFormat);
        QVERIFY(!ini.contains("games/545408B4/graphicsProfile"));
        QVERIFY(!ini.contains("games/545408B4/renderer"));
    }
    void invalidOrLockedGraphicsProfilesLeavePreferencesUnchanged() {
        QTemporaryDir directory; CoreOptions options(directory.path());
        options.setDefaultGraphicsProfile("default");
        QVERIFY(options.saveGameOptions("545408B4", "vulkan", "720p"));
        options.setDefaultGraphicsProfile("../bad");
        QCOMPARE(options.defaultGraphicsProfile(), QString("default"));
        QVERIFY(!options.saveGameOptions("545408B4", "vulkan", "bad"));
        QVERIFY(!options.saveGameOptions("../bad", "vulkan", "720p"));
        options.setLocked(true); options.setDefaultGraphicsProfile("4k-quality");
        QVERIFY(!options.saveGameOptions("545408B4", "", "1080p-fsr"));
        CoreOptions reopened(directory.path());
        QCOMPARE(reopened.defaultGraphicsProfile(), QString("default"));
        QCOMPARE(reopened.graphicsProfileFor("545408B4"), QString("720p"));
    }
    void corruptProfileIdsFallBackWithoutInventingArguments() {
        QTemporaryDir directory;
        { QSettings ini(directory.filePath("cores.ini"), QSettings::IniFormat);
          ini.setValue("graphics/defaultProfile", "unknown");
          ini.setValue("games/545408B4/graphicsProfile", "unknown"); ini.sync(); }
        CoreOptions options(directory.path());
        QCOMPARE(options.defaultGraphicsProfile(), QString("custom"));
        QVERIFY(options.graphicsArgumentsFor("545408B4").isEmpty());
        options.setDefaultGraphicsProfile("1080p-fsr");
        QCOMPARE(options.graphicsProfileFor("545408B4"), QString("1080p-fsr"));
    }
    void profilesOnlyOverrideResolutionAndPresentation() {
        QTemporaryDir directory; CoreOptions options(directory.path());
        const QSet<QString> allowed{"draw_resolution_scale_x", "draw_resolution_scale_y",
            "postprocess_antialiasing", "postprocess_scaling_and_sharpening",
            "postprocess_ffx_cas_additional_sharpness", "postprocess_ffx_fsr_max_upsampling_passes",
            "postprocess_ffx_fsr_sharpness_reduction", "postprocess_dither"};
        QSet<QString> ids;
        for (const auto& choice : options.graphicsProfileChoices()) {
            const auto row=choice.toMap(); const auto id=row["value"].toString();
            QVERIFY(!ids.contains(id)); ids.insert(id);
            QVERIFY(!row["label"].toString().isEmpty());
            QVERIFY(!options.graphicsProfileDescription(id).isEmpty());
            options.setDefaultGraphicsProfile(id);
            const auto args=options.graphicsArgumentsFor("545408B4");
            QCOMPARE(args.size(), id=="custom" ? 0 : 8);
            QSet<QString> seen;
            for (const auto& arg : args) {
                QVERIFY(arg.startsWith("--"));
                const auto name=arg.mid(2).section('=',0,0);
                QVERIFY(allowed.contains(name)); QVERIFY(!seen.contains(name)); seen.insert(name);
            }
        }
        QCOMPARE(ids.size(), 7);
        options.setDefaultGraphicsProfile("4k-quality");
        QVERIFY(options.graphicsArgumentsFor("").contains("--draw_resolution_scale_x=3"));
        options.setDefaultGraphicsProfile("default");
        QVERIFY(options.graphicsArgumentsFor("").contains("--draw_resolution_scale_x=1"));
        QVERIFY(options.graphicsArgumentsFor("").contains("--postprocess_scaling_and_sharpening="));
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

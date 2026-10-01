#include "engine_settings.h"
#include "save_store.h"
#include "community_package.h"
#include <QtEndian>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ServicesTests : public QObject {
    Q_OBJECT
    static void write(const QString& path,const QByteArray& bytes) {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); QCOMPARE(f.write(bytes),bytes.size());
    }
    static QByteArray read(const QString& path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
private slots:
    void storageDevicesAndMetadata() {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        EngineSettings settings(dir.filePath("engine"));
        QVERIFY(settings.externalContentPath()!=settings.contentPath());
        QVERIFY(settings.setValue("Storage.emulos_external_content_root",dir.filePath("usb")));
        QVERIFY(settings.save());
        EngineSettings reloaded(dir.filePath("engine"));
        QCOMPARE(reloaded.externalContentPath(),dir.filePath("usb"));
        QVERIFY(reloaded.setValue("Live.network_mode","1")); QVERIFY(reloaded.save());
        EngineSettings netplay(dir.filePath("engine"));
        QCOMPARE(netplay.values()["Live.network_mode"].toString(),QString("1"));
        const QString xuid="E000000000000001",title="4D5307D5";
        const auto internal=dir.filePath("internal"),external=dir.filePath("usb"), backups=dir.filePath("backups");
        const auto save='/'+xuid+'/'+title+"/00000001/slot/progress";
        const auto header='/'+xuid+'/'+title+"/Headers/00000001/slot.header";
        write(internal+save,"internal"); write(external+save,"external"); write(external+header,"metadata");
        QVERIFY(emulos::backupSave(external,backups,xuid,title).isEmpty());
        const auto copies=emulos::saveBackups(backups); QCOMPARE(copies.size(),1);
        const auto id=copies[0].toMap()["id"].toString();
        write(external+save,"changed"); write(external+header,"changed header");
        QVERIFY(emulos::restoreSave(external,backups,id).isEmpty());
        QCOMPARE(read(external+save),QByteArray("external")); QCOMPARE(read(external+header),QByteArray("metadata"));
        QCOMPARE(read(internal+save),QByteArray("internal"));
        write(backups+'/'+id+"/headers/slot.header","corrupt");
        QVERIFY(!emulos::restoreSave(external,backups,id).isEmpty());
        QCOMPARE(read(external+header),QByteArray("metadata"));
    }
    void communityPackageIntegrityAndPaths() {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const auto source=dir.filePath("input/HDD"), target=dir.filePath("imported");
        QByteArray xex(24,'\0'); xex.replace(0,4,"XEX2");
        qToBigEndian<quint32>(0xA,xex.data()+4); qToBigEndian<quint32>(24,xex.data()+8);
        const QByteArray ini="[Plugins]\nplugin1=Hdd:\\xbdm.xex\nplugin2=Hdd:\\xbGuard.xex\nplugin3=Hdd:\\JRPC2.xex\n[Settings]\ncustom_option=true\n";
        write(source+"/launch.ini",ini);
        for (const auto& name:{"xbdm.xex","xbGuard.xex","JRPC2.xex"}) write(source+'/'+name,xex);
        write(source+"/resources/theme.bin","resource");
        const auto report=emulos::inspectXbGuard(dir.filePath("input"),"HDD");
        QVERIFY(report["complete"].toBool()); QVERIFY(!report["runtimeSupported"].toBool());
        QCOMPARE(report["plugins"].toList().size(),3);
        QVERIFY(report["plugins"].toList()[1].toMap()["dll"].toBool());
        const auto imported=emulos::importXbGuard(source,"HDD",target); QVERIFY(imported["complete"].toBool());
        const auto path=imported["installedPath"].toString();
        QCOMPARE(read(path+"/launch.ini"),ini); QCOMPARE(read(path+"/resources/theme.bin"),QByteArray("resource"));
        QVERIFY(emulos::inspectXbGuard(path,"HDD")["complete"].toBool());
        QVERIFY(!emulos::inspectXbGuard(source,"USB")["complete"].toBool());
        write(source+"/launch.ini",QByteArray(ini).replace("Hdd:\\JRPC2.xex","Hdd:\\..\\JRPC2.xex"));
        QVERIFY(!emulos::importXbGuard(source,"HDD",target)["complete"].toBool());
        write(source+"/launch.ini",ini); write(source+"/xbdm.xex","XEX2");
        QVERIFY(!emulos::inspectXbGuard(source,"HDD")["complete"].toBool());
        QVERIFY(!emulos::inspectXex(xex.first(23))["valid"].toBool());
        qToBigEndian<quint32>(0xFFFFFFFF,xex.data()+20);
        QVERIFY(!emulos::inspectXex(xex)["valid"].toBool());
    }
    void settingsRoundTripAndConflict() {
        QTemporaryDir dir; QVERIFY(dir.isValid()); EngineSettings settings(dir.path());
        QCOMPARE(settings.entries().size(),288);
        QVERIFY(settings.setValue("GPU.vsync",false));
        QVERIFY(settings.setValue("Console.language","5"));
        QVERIFY(settings.save());
        EngineSettings reopened(dir.path());
        QCOMPARE(reopened.values().value("GPU.vsync"),QVariant(false));
        QCOMPARE(reopened.values().value("Console.language").toString(),QString("5"));
        const auto console=read(dir.filePath("xconfig.settings")); QVERIFY(!console.isEmpty());
        QVERIFY(reopened.setValue("GPU.vsync",true)); QVERIFY(reopened.save());
        QCOMPARE(read(dir.filePath("xconfig.settings")),console);
        QVERIFY(!reopened.setValue("GPU.draw_resolution_scale_x","-1")); QVERIFY(!reopened.save());
        QVERIFY(reopened.reload());
        write(reopened.configPath(),read(reopened.configPath())+"\n# external change\n");
        QVERIFY(reopened.setValue("GPU.vsync",false)); QVERIFY(!reopened.save());
        QVERIFY(read(reopened.configPath()).contains("external change"));
    }
    void unknownTomlKeysSurviveImport() {
        QTemporaryDir dir; EngineSettings settings(dir.filePath("engine"));
        const auto path=dir.filePath("import.toml"); write(path,"[Future]\nfoo = 'kept'\n");
        QVERIFY(settings.importConfig(QUrl::fromLocalFile(path))); QVERIFY(settings.dirty()); QVERIFY(settings.save());
        QVERIFY(read(settings.configPath()).contains("kept"));
    }
    void saveBackupRestoreAndCorruption() {
        QTemporaryDir dir; const auto content=dir.filePath("content"), backups=dir.filePath("backups");
        const auto xuid=QString("E030000000000001"), title=QString("4D5307D5");
        const auto path=content+'/'+xuid+'/'+title+"/00000001/slot/progress.bin";
        write(path,"chapter-1");
        QCOMPARE(emulos::savedGames(content).size(),1);
        QVERIFY(emulos::backupSave(content,backups,xuid,title).isEmpty());
        const auto list=emulos::saveBackups(backups); QCOMPARE(list.size(),1);
        const auto id=list[0].toMap()["id"].toString();
        write(path,"chapter-2"); QVERIFY(emulos::restoreSave(content,backups,id).isEmpty()); QCOMPARE(read(path),QByteArray("chapter-1"));
        const auto parent=content+'/'+xuid+'/'+title;
        QCOMPARE(QDir(parent).entryList({"00000001.emulos-before-*"},QDir::Dirs).size(),1);
        write(backups+'/'+id+"/payload/slot/progress.bin","corrupted");
        QVERIFY(!emulos::restoreSave(content,backups,id).isEmpty()); QCOMPARE(read(path),QByteArray("chapter-1"));
        QVERIFY(!emulos::restoreSave(content,backups,"../escape").isEmpty());
    }
};
QTEST_GUILESS_MAIN(ServicesTests)
#include "services_tests.moc"

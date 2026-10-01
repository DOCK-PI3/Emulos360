#include "library.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

class LibraryTests : public QObject {
    Q_OBJECT
private:
    static QByteArray header() {
        QByteArray bytes(0x511, '\0');
        bytes.replace(0, 4, "LIVE");
        qToBigEndian<quint32>(0x7000, bytes.data() + 0x344);
        qToBigEndian<quint32>(0x4D5307D5, bytes.data() + 0x360);
        qToBigEndian<quint32>(1, bytes.data() + 0x39D);
        qToBigEndian<quint32>(1, bytes.data() + 0x3A9);
        qToBigEndian<quint16>('G', bytes.data() + 0x411);
        return bytes;
    }
    static QByteArray xblaHeader() {
        auto bytes = header();
        qToBigEndian<quint32>(0xD0000, bytes.data() + 0x344);
        qToBigEndian<quint32>(0, bytes.data() + 0x39D);
        qToBigEndian<quint32>(0, bytes.data() + 0x3A9);
        return bytes;
    }
    static QByteArray xexHeader(quint32 titleId = 0x454109AB, quint32 flags = 1) {
        QByteArray bytes(0x100, '\0');
        bytes.replace(0, 4, "XEX2");
        qToBigEndian(flags, bytes.data() + 4);
        qToBigEndian<quint32>(0x100, bytes.data() + 8);
        qToBigEndian<quint32>(1, bytes.data() + 0x14);
        qToBigEndian<quint32>(0x00040006, bytes.data() + 0x18);
        qToBigEndian<quint32>(0x40, bytes.data() + 0x1C);
        qToBigEndian(titleId, bytes.data() + 0x4C);
        return bytes;
    }
private slots:
    void rejectsTruncatedHeaders() {
        const auto bytes = header();
        for (qsizetype size = 0; size < bytes.size(); ++size)
            QVERIFY(!los::parsePackageHeader(bytes.first(size)));
    }
    void readsBigEndianMetadata() {
        const auto package = los::parsePackageHeader(header());
        QVERIFY(package);
        QCOMPARE(package->titleId, QString("4D5307D5"));
        QCOMPARE(package->title, QString("G"));
        QCOMPARE(package->fragments, 1U);
        QCOMPARE(package->format, QString("GOD"));
    }
    void readsXblaStfsMetadata() {
        const auto package = los::parsePackageHeader(xblaHeader());
        QVERIFY(package);
        QCOMPARE(package->titleId, QString("4D5307D5"));
        QCOMPARE(package->fragments, 0U);
        QCOMPARE(package->format, QString("XBLA"));
    }
    void rejectsOtherContentAndUnboundedCounts() {
        auto bytes = header();
        qToBigEndian<quint32>(0x20000, bytes.data() + 0x344);
        QVERIFY(!los::parsePackageHeader(bytes));
        bytes = header();
        qToBigEndian<quint32>(0xFFFFFFFF, bytes.data() + 0x39D);
        QVERIFY(!los::parsePackageHeader(bytes));
    }
    void detectsXexTitleButNotPlugin() {
        const auto title = los::parseXexHeader(xexHeader());
        QVERIFY(title);
        QCOMPARE(title->titleId, QString("454109AB"));
        QCOMPARE(title->format, QString("XEX"));
        QVERIFY(!los::parseXexHeader(xexHeader(0x454109AB, 9))); // Game DLL.
        QVERIFY(!los::parseXexHeader(xexHeader(0xFFFE07D1)));    // System dashboard.
        QVERIFY(!los::parseXexHeader(xexHeader(0xF5D10000)));    // FATX helper.
        QVERIFY(!los::parseXexHeader(xexHeader().first(0x4B)));
        auto malformed = xexHeader();
        qToBigEndian<quint32>(0xFC, malformed.data() + 0x1C);
        QVERIFY(!los::parseXexHeader(malformed));
    }
    void scansExtractedGamesWithoutListingAuxiliaryXex() {
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        QVERIFY(QDir(folder.path()).mkpath("Army of Two/engine"));
        QVERIFY(QDir(folder.path()).mkpath("Sega Tennis"));
        QVERIFY(QDir(folder.path()).mkpath("Backup/B"));
        const auto writeFile = [&folder](const QString& name, const QByteArray& bytes) {
            QFile file(folder.filePath(name));
            if (!file.open(QIODevice::WriteOnly)) return false;
            return file.write(bytes) == bytes.size();
        };
        QVERIFY(writeFile("Army of Two/Default.xex", xexHeader()));
        QVERIFY(writeFile("Army of Two/engine/Main.xex", xexHeader(0x454109AB, 9)));
        QVERIFY(writeFile("Sega Tennis/tennis.xex", xexHeader(0x534507F5)));
        QVERIFY(writeFile("Backup/B/default.xex", xexHeader(0xFFFE07D1)));
        const auto result = los::scanLibrary(folder.path());
        QCOMPARE(result.games.size(), 2);
        for (const auto& item : result.games) {
            const auto game = item.toMap();
            QCOMPARE(game.value("format").toString(), QString("XEX"));
            QVERIFY(game.value("complete").toBool());
            QVERIFY(game.value("path").toString().endsWith(".xex", Qt::CaseInsensitive));
        }
    }
    void detectsXblaWithoutGodFragments() {
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        QVERIFY(QDir(folder.path()).mkpath("title/000D0000"));
        const auto path = folder.filePath("title/000D0000/package");
        { QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(xblaHeader()), xblaHeader().size()); }
        const auto result = los::scanLibrary(folder.path());
        QCOMPARE(result.games.size(), 1);
        const auto game = result.games.first().toMap();
        QVERIFY(game.value("complete").toBool());
        QCOMPARE(game.value("format").toString(), QString("XBLA"));
        QCOMPARE(game.value("fragments").toUInt(), 0U);
    }
    void detectsMissingFragmentsWithoutChangingSource() {
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        QVERIFY(QDir(folder.path()).mkpath("title/00007000/package.data"));
        const auto path = folder.filePath("title/00007000/package");
        { QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(header()), header().size()); }
        auto result = los::scanLibrary(folder.path());
        QCOMPARE(result.games.size(), 1);
        QVERIFY(!result.games.first().toMap().value("complete").toBool());
        { QFile file(path + ".data/Data0000"); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("test"); }
        result = los::scanLibrary(folder.path());
        QVERIFY(result.games.first().toMap().value("complete").toBool());
        QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), header());
    }
    void missingRootHasError() { QVERIFY(!los::scanLibrary("/nonexistent-los-library-31988").error.isEmpty()); }
};
QTEST_GUILESS_MAIN(LibraryTests)
#include "library_tests.moc"

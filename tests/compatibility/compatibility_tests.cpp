#include "game_compatibility.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

namespace {
QJsonObject report(const QString& id, const QString& title, const QString& state, const QString& date = "2026-09-28T12:00:00Z") {
    return {{"id", id}, {"title", title}, {"state", state}, {"updated", date},
        {"labels", QJsonObject{{"others", QJsonArray{"gpu-drawing-corrupt"}}}},
        {"url", "https://github.com/xenia-canary/game-compatibility/issues/216"}};
}
QByteArray catalog(std::initializer_list<QJsonObject> records) {
    QJsonArray array;
    for (const auto& item : records) array.append(item);
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}
bool cache(const QString& directory, const QByteArray& bytes) {
    if (!QDir().mkpath(directory + "/compatibility")) return false;
    QFile file(directory + "/compatibility/canary.json");
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray read(const QString& path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
}

class CompatibilityTests : public QObject {
    Q_OBJECT
private slots:
    void realBundledCatalog() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        GameCompatibility database(directory.path());
        QCOMPARE(database.lookup("4d530919", "Unrelated filename").value("state").toString(), QString("gameplay"));
        QVERIFY(database.lookup("4d530919").value("tooltip").toString().contains("gráficos"));
        QCOMPARE(database.legend().size(), 5);
        QVERIFY(!QFileInfo::exists(directory.path() + "/compatibility/canary.json"));
    }
    void matchingPreservesEditionsAndAmbiguity() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QVERIFY(cache(directory.path(), catalog({report("4D530919", "Halo 4", "Gameplay"),
            report("11111111", "Portal & Friends", "Playable"),
            report("22222222", "Same Name", "Loads"), report("33333333", "Same Name", "Playable")})));
        GameCompatibility database(directory.path());
        QCOMPARE(database.lookup("", "Halo 4 (USA) (XBLA)").value("state").toString(), QString("gameplay"));
        QVERIFY(database.lookup("", "Halo 4").value("matchedByName").toBool());
        QCOMPARE(database.lookup("", "Portal and Friends").value("state").toString(), QString("playable"));
        QCOMPARE(database.lookup("", "Halo 4 Demo").value("state").toString(), QString("unknown"));
        QCOMPARE(database.lookup("", "Halo 4 (Beta)").value("state").toString(), QString("unknown"));
        QCOMPARE(database.lookup("", "Same Name").value("state").toString(), QString("unknown"));
        QCOMPARE(database.lookup("22222222", "Same Name").value("state").toString(), QString("loads"));
        QCOMPARE(database.lookup("FFFFFFFF", "Halo 4").value("state").toString(), QString("unknown"));
    }
    void duplicateIdsSelectEditionAndNewestReport() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QVERIFY(cache(directory.path(), catalog({report("11111111", "Collection", "Loads", "2020-01-01T00:00:00Z"),
            report("11111111", "Collection", "Playable"), report("22222222", "Part One", "Playable"),
            report("22222222", "Part Two", "Unplayable")})));
        GameCompatibility database(directory.path());
        QCOMPARE(database.lookup("11111111", "Collection").value("state").toString(), QString("playable"));
        QCOMPARE(database.lookup("22222222", "Part Two").value("state").toString(), QString("unplayable"));
        QCOMPARE(database.lookup("22222222", "Other").value("state").toString(), QString("unknown"));
        QCOMPARE(database.lookup("22222222").value("state").toString(), QString("unknown"));
    }
    void corruptCacheKeepsBundledCatalog() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QVERIFY(cache(directory.path(), "<html>not a catalog</html>"));
        GameCompatibility database(directory.path());
        QCOMPARE(database.lookup("4D530919").value("state").toString(), QString("gameplay"));
    }
    void refreshIsBoundedAtomicAndCoalesced() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QByteArray response = catalog({report("4D530919", "Halo 4", "Loads")});
        int requests = 0;
        connect(&server, &QTcpServer::newConnection, &server, [&] {
            auto* socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                socket->readAll();
                if (socket->property("responded").toBool()) return;
                socket->setProperty("responded", true); ++requests;
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(response.size()) +
                    "\r\nConnection: close\r\n\r\n" + response);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
        GameCompatibility database(directory.path(), nullptr, QUrl(QString("http://127.0.0.1:%1/catalog").arg(server.serverPort())));
        database.refresh(); database.refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!database.busy(), 5000);
        QCOMPARE(requests, 1);
        QCOMPARE(database.lookup("4D530919").value("state").toString(), QString("loads"));
        const auto saved = read(directory.path() + "/compatibility/canary.json");
        QCOMPARE(saved, response);
        const auto revision = database.revision();
        response = "[{broken";
        database.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!database.busy(), 5000);
        QCOMPARE(database.revision(), revision);
        QCOMPARE(read(directory.path() + "/compatibility/canary.json"), saved);
        response = QByteArray(4 * 1024 * 1024 + 1, 'x');
        database.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!database.busy(), 5000);
        QCOMPARE(database.revision(), revision);
        GameCompatibility offline(directory.path());
        QCOMPARE(offline.lookup("4D530919").value("state").toString(), QString("loads"));
    }
};
QTEST_GUILESS_MAIN(CompatibilityTests)
#include "compatibility_tests.moc"

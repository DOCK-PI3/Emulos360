#include "private_netplay.h"
#include <QTest>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QSignalSpy>
class PrivateNativeTests : public QObject {
    Q_OBJECT
private slots:
    void lifecycle() {
        QVERIFY(!qEnvironmentVariable("EMULOS_PRIVATE_SERVER_DIR").isEmpty());
        QTemporaryDir data; QVERIFY(data.isValid());
        QTcpServer reservation; QVERIFY(reservation.listen(QHostAddress::LocalHost, 0));
        const auto port = reservation.serverPort(); reservation.close();
        PrivateNetplay host(data.filePath("host")), client(data.filePath("client"));
        QSignalSpy endpoints(&host, &PrivateNetplay::endpointChanged);
        host.host("Native sanitizer", port, "127.0.0.1", false);
        QTRY_VERIFY_WITH_TIMEOUT(!host.busy(), 60000);
        QVERIFY2(host.connected(), qPrintable(host.status()));
        QVERIFY(host.hosting()); QVERIFY(!endpoints.empty());
        host.createInvitation("Private friend", 1);
        QTRY_VERIFY_WITH_TIMEOUT(!host.invitation().isEmpty(), 15000);
        client.join(host.invitation(), "Native friend");
        QTRY_VERIFY_WITH_TIMEOUT(!client.busy(), 30000);
        QVERIFY2(client.connected(), qPrintable(client.status()));
        QTRY_COMPARE_WITH_TIMEOUT(client.metrics().value("role").toString(), QString("client"), 15000);
        client.stop(); QTRY_VERIFY_WITH_TIMEOUT(!client.running(), 15000);
        host.stop(); QTRY_VERIFY_WITH_TIMEOUT(!host.running(), 15000);
        QCOMPARE(endpoints.last().first().toString(), QString());
    }
};
QTEST_MAIN(PrivateNativeTests)
#include "private_native_tests.moc"

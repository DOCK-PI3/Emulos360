#include "voice_party.h"

#include <QElapsedTimer>
#include <QTest>
#include <opus.h>
#include <algorithm>
#include <array>
#include <memory>

class VoicePartyTests : public QObject {
    Q_OBJECT
private slots:
    void localPartyWithoutExternalServer() {
        qputenv("EMULOS_VOICE_NO_AUDIO", "1");
        VoiceParty host;
        VoiceParty guest;
        QVERIFY(host.host(QStringLiteral("Ana"), 0));
        QVERIFY(host.port() > 0);
        QCOMPARE(host.participants().size(), 1);
        guest.join(QStringLiteral("127.0.0.1"), host.port(), host.roomCode(), QStringLiteral("Luis"));
        QTRY_COMPARE_WITH_TIMEOUT(host.participants().size(), 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(guest.participants().size(), 2, 10000);

        QElapsedTimer timer;
        timer.start();
        while (host.messages().isEmpty() && timer.elapsed() < 15000) {
            guest.sendText(QStringLiteral("hola"));
            QTest::qWait(100);
        }
        QVERIFY2(!host.messages().isEmpty(), qPrintable(guest.status()));
        QCOMPARE(host.messages().last().toMap().value("text").toString(), QStringLiteral("hola"));
        host.sendText(QStringLiteral("listos"));
        QTRY_VERIFY_WITH_TIMEOUT(guest.messages().last().toMap().value("text").toString() == QStringLiteral("listos"), 5000);
        guest.leave();
        QTRY_COMPARE_WITH_TIMEOUT(host.participants().size(), 1, 5000);
        host.leave();
    }
    void rejectsWrongRoomCode() {
        qputenv("EMULOS_VOICE_NO_AUDIO", "1");
        VoiceParty host;
        VoiceParty guest;
        QVERIFY(host.host(QStringLiteral("Ana"), 0));
        const auto wrongCode = host.roomCode() == QStringLiteral("00000000")
            ? QStringLiteral("99999999") : QStringLiteral("00000000");
        guest.join(QStringLiteral("127.0.0.1"), host.port(), wrongCode, QStringLiteral("Luis"));
        QTRY_VERIFY_WITH_TIMEOUT(!guest.active(), 3000);
        QCOMPARE(host.participants().size(), 1);
        QVERIFY2(guest.status().contains(QStringLiteral("Código")), qPrintable(guest.status()));
    }
    void opusCodecRoundTrip() {
        int error = OPUS_OK;
        std::unique_ptr<OpusEncoder, decltype(&opus_encoder_destroy)> encoder(
            opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error), &opus_encoder_destroy);
        QVERIFY(encoder && error == OPUS_OK);
        std::unique_ptr<OpusDecoder, decltype(&opus_decoder_destroy)> decoder(
            opus_decoder_create(48000, 1, &error), &opus_decoder_destroy);
        QVERIFY(decoder && error == OPUS_OK);
        std::array<opus_int16, 960> input{};
        for (int i = 0; i < 960; ++i) input[i] = i % 80 < 40 ? 4000 : -4000;
        std::array<unsigned char, 1275> compressed{};
        const int bytes = opus_encode(encoder.get(), input.data(), 960, compressed.data(), 1275);
        QVERIFY(bytes > 0);
        std::array<opus_int16, 960> output{};
        QCOMPARE(opus_decode(decoder.get(), compressed.data(), bytes, output.data(), 960, 0), 960);
        QVERIFY(std::any_of(output.begin(), output.end(), [](auto sample) { return sample != 0; }));
    }
};

QTEST_GUILESS_MAIN(VoicePartyTests)
#include "voice_party_tests.moc"

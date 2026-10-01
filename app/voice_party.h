#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QVariantList>
#include <QJsonObject>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <mutex>

namespace rtc { class DataChannel; }

// A party is hosted by one Emulos360 process. TCP carries only WebRTC
// signaling; Opus voice and chat use encrypted WebRTC data channels.
class VoiceParty : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(VoiceParty)
    QML_UNCREATABLE("Owned by LibraryController")
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString roomCode READ roomCode NOTIFY changed)
    Q_PROPERTY(QString localAddresses READ localAddresses NOTIFY changed)
    Q_PROPERTY(quint16 port READ port NOTIFY changed)
    Q_PROPERTY(bool hosting READ hosting NOTIFY changed)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY changed)
    Q_PROPERTY(QVariantList participants READ participants NOTIFY changed)
    Q_PROPERTY(QVariantList nearby READ nearby NOTIFY changed)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY changed)
public:
    struct CallbackTarget {
        std::mutex mutex;
        VoiceParty* instance = nullptr;
    };
    explicit VoiceParty(QObject* parent = nullptr);
    ~VoiceParty() override;
    QString status() const { return status_; }
    QString roomCode() const { return roomCode_; }
    QString localAddresses() const;
    quint16 port() const { return port_; }
    bool hosting() const { return hosting_; }
    bool active() const { return active_; }
    bool muted() const { return muted_; }
    void setMuted(bool muted);
    QVariantList participants() const { return participants_; }
    QVariantList nearby() const { return nearby_; }
    QVariantList messages() const { return messages_; }
    Q_INVOKABLE bool host(const QString& name, int port = 46580);
    Q_INVOKABLE void join(const QString& address, int port, const QString& code, const QString& name);
    Q_INVOKABLE void leave();
    Q_INVOKABLE void sendText(const QString& text);
signals:
    void changed();
private:
    struct Peer;
    struct Audio;
    void setStatus(const QString& text);
    void acceptPeer();
    void attachSocket(QTcpSocket* socket, const std::shared_ptr<Peer>& peer);
    void readSignal(const std::shared_ptr<Peer>& peer);
    void handleSignal(const std::shared_ptr<Peer>& peer, const QJsonObject& object);
    void sendSignal(QTcpSocket* socket, const QJsonObject& object);
    void configurePeer(const std::shared_ptr<Peer>& peer);
    void attachChannel(const std::shared_ptr<Peer>& peer,
                       const std::shared_ptr<rtc::DataChannel>& channel);
    void handleChat(const std::shared_ptr<Peer>& peer, const QString& text);
    void handleVoice(const std::shared_ptr<Peer>& peer, const QByteArray& data);
    void publishRoster();
    void announce();
    void readDiscovery();
    void updateNearby();
    void startAudio();
    void stopAudio();
    void captureAudio();
    void playAudio();
    void emitVoice(const QByteArray& opusFrame);
    void decodeVoice(quint32 speaker, const QByteArray& opusFrame);

    QTcpServer server_;
    QUdpSocket discovery_;
    QTimer heartbeat_;
    QTimer playback_;
    QHash<QTcpSocket*, std::shared_ptr<Peer>> peers_;
    std::shared_ptr<CallbackTarget> callbacks_;
    std::unique_ptr<Audio> audio_;
    QString status_;
    QString roomCode_;
    QString displayName_;
    QString remoteAddress_;
    QString lastConnectionError_;
    QVariantList participants_;
    QVariantList nearby_;
    QVariantList messages_;
    QHash<QString, QVariantMap> discovered_;
    QHash<QString, qint64> seen_;
    bool hosting_ = false;
    bool active_ = false;
    bool muted_ = false;
    quint16 port_ = 0;
    quint32 selfId_ = 0;
    quint32 nextId_ = 2;
};

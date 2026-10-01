#pragma once
#include <QObject>
#include <QProcess>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>

class PrivateNetplay final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(PrivateNetplay)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool hosting READ hosting NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY changed)
    Q_PROPERTY(QString localAddress READ localAddress CONSTANT)
    Q_PROPERTY(QString publicAddress READ publicAddress NOTIFY changed)
    Q_PROPERTY(QString invitation READ invitation NOTIFY changed)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY changed)
public:
    explicit PrivateNetplay(QString data, QObject* parent = nullptr);
    ~PrivateNetplay() override;
    bool running() const { return process_.state() != QProcess::NotRunning; }
    bool busy() const { return starting_; }
    bool hosting() const { return hosting_; }
    bool connected() const { return connected_; }
    QString status() const { return status_; }
    QString endpoint() const { return endpoint_; }
    QString localAddress() const;
    QString publicAddress() const { return publicAddress_; }
    QString invitation() const { return invitation_; }
    QVariantMap metrics() const { return metrics_; }
    Q_INVOKABLE void host(const QString& name, int port, const QString& address, bool upnp);
    Q_INVOKABLE void join(const QString& invitation, const QString& name, const QString& address = {}, int port = 36000);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void createInvitation(const QString& label, int hours = 168);
    Q_INVOKABLE void revoke(const QString& id);
    Q_INVOKABLE void shareStats(bool enabled);
    Q_INVOKABLE void cleanup();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void findPublicAddress();
    Q_INVOKABLE void copyInvitation();
    Q_INVOKABLE void openPanel();
signals:
    void changed();
    void endpointChanged(const QString& bridge);
private:
    struct ProcessGroup;
    std::unique_ptr<ProcessGroup> processGroup_;
    void launch(QVariantMap config);
    void send(const QVariantMap& command);
    void readOutput();
    QString data_, status_, endpoint_, invitation_, adminURL_, publicAddress_;
    QProcess process_;
    QByteArray output_;
    QVariantMap metrics_;
    bool starting_ = false, hosting_ = false, connected_ = false;
};

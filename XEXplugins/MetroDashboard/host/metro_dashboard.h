#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
class Controller;
class MetroDashboard final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(MetroDashboard)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool powerOpen READ powerOpen NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    MetroDashboard(Controller* controller, const QString& data, QObject* parent=nullptr);
    bool active() const { return active_; }
    bool powerOpen() const { return powerOpen_; }
    QString status() const { return status_; }
    Q_INVOKABLE void start();
    Q_INVOKABLE void openPower();
    Q_INVOKABLE void cancelPower();
    Q_INVOKABLE void confirmPower(int action);
    Q_INVOKABLE void finishCoverEdit();
    static bool decodeRequest(const QByteArray& bytes, const QString& nonce, quint32 previous,
                              int gameCount, quint32& serial, int& action, int& argument);
signals:
    void changed();
    void showPage(const QString& page);
    void editCover(const QVariantMap& game);
private:
    void launchDashboard();
    void syncCovers();
    bool snapshot();
    void poll();
    void transition(int action, int argument=0);
    void ended(int code);
    void dispatch();
    void message(const QString& text);
    Controller* controller_;
    QString root_, nonce_, status_;
    QVariantList games_;
    QTimer timer_, closeTimeout_;
    quint32 serial_=0;
    int mode_=0, pending_=0, argument_=0;
    bool active_=false, powerOpen_=false, powerArmed_=false;
    quint32 guideSerial_=0;
};

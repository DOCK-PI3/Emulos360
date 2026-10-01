#pragma once
#include "engine_settings.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

// Public, read-only advertised-session browser. Joining remains owned by XAM/game.
class NetplayRooms final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(NetplayRooms)
    QML_UNCREATABLE("Owned by LibraryController")
    Q_PROPERTY(QVariantList rooms READ rooms NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString server READ server NOTIFY changed)
public:
    NetplayRooms(EngineSettings* settings, QObject* parent = nullptr);
    QVariantList rooms() const { return rooms_; }
    bool busy() const { return !reply_.isNull(); }
    QString status() const { return status_; }
    QString server() const;
    void setServerOverride(const QString& endpoint);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void cancel();
    static bool decode(const QByteArray& json, QVariantList& rows, QString& error);
signals:
    void changed();
private:
    EngineSettings* settings_;
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QByteArray buffer_;
    QVariantList rooms_;
    QString status_;
    QString serverOverride_;
};

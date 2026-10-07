#pragma once
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class CoreOptions final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(CoreOptions)
    QML_UNCREATABLE("Owned by PlayerServices")
    Q_PROPERTY(QVariantList rendererChoices READ rendererChoices CONSTANT)
    Q_PROPERTY(QString defaultRenderer READ defaultRenderer WRITE setDefaultRenderer NOTIFY changed)
    Q_PROPERTY(QVariantList graphicsProfileChoices READ graphicsProfileChoices CONSTANT)
    Q_PROPERTY(QString defaultGraphicsProfile READ defaultGraphicsProfile WRITE setDefaultGraphicsProfile NOTIFY changed)
    Q_PROPERTY(QVariantMap defaultGraphicsValues READ defaultGraphicsValues NOTIFY changed)
    Q_PROPERTY(bool locked READ locked NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit CoreOptions(const QString& data, QObject* parent = nullptr);
    QVariantList rendererChoices() const;
    QString defaultRenderer() const;
    void setDefaultRenderer(const QString& id);
    QVariantList graphicsProfileChoices() const;
    QString defaultGraphicsProfile() const;
    void setDefaultGraphicsProfile(const QString& id);
    QVariantMap defaultGraphicsValues() const;
    Q_INVOKABLE QString graphicsProfileDescription(const QString& id) const;
    QString graphicsProfileFor(const QString& titleId) const;
    QStringList graphicsArgumentsFor(const QString& titleId) const;
    bool locked() const { return locked_; }
    void setLocked(bool value);
    QString status() const { return status_; }
    Q_INVOKABLE QVariantMap gameOptions(const QString& titleId) const;
    Q_INVOKABLE bool saveGameOptions(const QString& titleId, const QString& renderer, const QString& graphicsProfile = {});
    QString rendererFor(const QString& titleId) const;
signals:
    void changed();
private:
    void migrateLegacyPreferences();
    bool fail(const QString& message);
    bool persist();
    bool validRenderer(const QString& value) const;
    bool validGraphicsProfile(const QString& value) const;
    QString status_;
    QSettings preferences_;
    bool locked_ = false;
};

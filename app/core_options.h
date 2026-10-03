#pragma once
#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class CoreOptions final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(CoreOptions)
    QML_UNCREATABLE("Owned by PlayerServices")
    Q_PROPERTY(QVariantList choices READ choices CONSTANT)
    Q_PROPERTY(QVariantList rendererChoices READ rendererChoices CONSTANT)
    Q_PROPERTY(QString defaultCore READ defaultCore WRITE setDefaultCore NOTIFY changed)
    Q_PROPERTY(QString defaultRenderer READ defaultRenderer WRITE setDefaultRenderer NOTIFY changed)
    Q_PROPERTY(bool locked READ locked NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit CoreOptions(const QString& data, QObject* parent = nullptr, const QString& applicationRoot = {});
    QVariantList choices() const;
    QVariantList rendererChoices() const;
    QString defaultCore() const;
    QString defaultRenderer() const;
    void setDefaultCore(const QString& id);
    void setDefaultRenderer(const QString& id);
    bool locked() const { return locked_; }
    void setLocked(bool value);
    QString status() const { return status_; }
    Q_INVOKABLE QVariantMap gameOptions(const QString& titleId) const;
    Q_INVOKABLE bool saveGameOptions(const QString& titleId, const QString& core, const QString& renderer);
    Q_INVOKABLE void openEdgeFolder();
    Q_INVOKABLE void openEdgeContent();
    QString executable(const QString& id) const;
    QString coreFor(const QString& titleId) const;
    QString rendererFor(const QString& titleId) const;
    QString edgeStorage() const;
    QString edgeContent() const;
    QString edgeConfig() const;
    bool ensureEdgeConfig(int language, bool notifications);
signals:
    void changed();
private:
    bool fail(const QString& message);
    bool persist();
    bool validRenderer(const QString& value) const;
    QString data_, applicationRoot_, status_;
    QSettings preferences_;
    bool locked_ = false;
};

namespace emulos {
// Copy only into the independent Edge store; existing progress is never replaced.
QString seedEdgeContent(const QString& internal, const QString& external,
                        const QString& destination, const QString& xuid, const QString& titleId);
}

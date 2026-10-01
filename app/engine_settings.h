#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class EngineSettings final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(EngineSettings)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(QVariantList entries READ entries CONSTANT)
    Q_PROPERTY(QVariantMap values READ values NOTIFY changed)
    Q_PROPERTY(QVariantMap errors READ errors NOTIFY changed)
    Q_PROPERTY(QVariantMap modified READ modified NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString storagePath READ storagePath NOTIFY changed)
public:
    explicit EngineSettings(const QString& root, QObject* parent = nullptr);
    QVariantList entries() const { return entries_; }
    QVariantMap values() const { return values_; }
    QVariantMap errors() const { return errors_; }
    QVariantMap modified() const;
    bool dirty() const;
    bool locked() const { return locked_; }
    void setLocked(bool locked);
    QString status() const { return status_; }
    QString configPath() const;
    QString storagePath() const;
    QString contentPath() const;
    QString externalContentPath() const;
    Q_INVOKABLE bool setValue(const QString& key, const QVariant& value);
    Q_INVOKABLE void resetValue(const QString& key);
    Q_INVOKABLE void resetGroup(const QString& group);
    Q_INVOKABLE bool reload();
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool importConfig(const QUrl& file);
    Q_INVOKABLE bool exportConfig(const QUrl& file);
    Q_INVOKABLE void openFolder();
signals:
    void changed();
private:
    bool loadToml(const QByteArray& source, QVariantMap& output, QString& error) const;
    QByteArray serializeToml(QString& error) const;
    bool normalize(const QVariantMap& entry, const QVariant& input, QVariant& output, QString& error) const;
    bool fail(const QString& message);
    QString root_;
    QString loadedConsolePath_;
    QVariantList entries_;
    QMap<QString, QVariantMap> schema_;
    QVariantMap values_, saved_, errors_;
    QByteArray tomlBytes_, diskToml_, consoleBytes_, diskConsole_;
    QString status_;
    bool locked_ = false;
    bool loadOk_ = false;
    bool imported_ = false;
};

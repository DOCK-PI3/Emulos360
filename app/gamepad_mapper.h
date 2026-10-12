#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QtQml/qqmlregistration.h>
#include <memory>

class GamepadMapper final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by LibraryController")
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)
    Q_PROPERTY(int selected READ selected NOTIFY changed)
    Q_PROPERTY(QString family READ family NOTIFY changed)
    Q_PROPERTY(QVariantList controls READ controls NOTIFY changed)
    Q_PROPERTY(QVariantMap values READ values NOTIFY inputChanged)
    Q_PROPERTY(QString target READ target NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool guided READ guided NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
public:
    explicit GamepadMapper(QString dataDirectory, QObject* parent = nullptr);
    ~GamepadMapper() override;
    bool active() const { return active_; }
    QVariantList devices() const { return devices_; }
    int selected() const { return selected_; }
    QString family() const { return family_; }
    QVariantList controls() const;
    QVariantMap values() const { return values_; }
    QString target() const { return target_; }
    QString status() const { return status_; }
    bool guided() const { return guided_; }
    bool dirty() const { return bindings_ != saved_; }
    int progress() const { return step_; }
    Q_INVOKABLE void begin();
    Q_INVOKABLE void end();
    Q_INVOKABLE void selectDevice(int index);
    Q_INVOKABLE void capture(const QString& target);
    Q_INVOKABLE void startGuided();
    Q_INVOKABLE void skip();
    Q_INVOKABLE void stopCapture();
    Q_INVOKABLE void restoreDefaults();
    Q_INVOKABLE void discard();
    Q_INVOKABLE bool save();
    void poll();
    static QStringList launchArguments(const QString& dataDirectory);
signals:
    void changed();
    void inputChanged();
private:
    struct Native;
    std::unique_ptr<Native> native_;
    void refreshDevices();
    void assign(const QString& source);
    void prepareCapture();
    QString mappingLine() const;
    QString data_;
    QTimer timer_;
    QVariantList devices_;
    QVariantMap values_;
    QMap<QString, QString> bindings_, saved_, defaults_;
    QString guid_, name_, family_ = "generic", target_, status_;
    int selected_ = -1, step_ = 0, refreshTicks_ = 0;
    bool active_ = false, guided_ = false, armed_ = false;
};

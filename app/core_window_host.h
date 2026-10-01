#pragma once
#include <QObject>
#include <QTimer>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

// A foreign window wrapper only. The core owns and destroys its native window.
class CoreWindowHost : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(CoreWindowHost)
    QML_UNCREATABLE("Owned by PlayerServices")
    Q_PROPERTY(QWindow* window READ window NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit CoreWindowHost(QObject* parent = nullptr);
    ~CoreWindowHost() override;
    QWindow* window() const { return window_; }
    QString status() const { return status_; }
    void watch(qint64 pid);
    void clear();
    Q_INVOKABLE void focus();
    Q_INVOKABLE void requestClose();
signals:
    void changed();
private:
    void discover();
    QTimer timer_;
    QWindow* window_ = nullptr;
    qint64 pid_ = 0;
    int attempts_ = 0;
    QString status_;
};

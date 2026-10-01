#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <atomic>

class GameImporter : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameImporter)
    QML_UNCREATABLE("Created by the C++ application")
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
public:
    explicit GameImporter(QObject* parent = nullptr);
    ~GameImporter() override;
    bool busy() const { return busy_; }
    QString status() const { return status_; }
    double progress() const { return progress_; }
    Q_INVOKABLE void importFile(const QUrl& source, const QString& libraryPath);
    Q_INVOKABLE void cancel();
signals:
    void changed();
    void installed();
    void completed(bool installed);
private:
    struct Result { bool ok = false; QString message; };
    void report(const QString& message, double progress);
    Result importWorker(const QString& source, const QString& libraryPath);
    QFutureWatcher<Result> watcher_;
    std::atomic_bool cancelled_{false};
    bool busy_ = false;
    QString status_;
    double progress_ = 0;
};

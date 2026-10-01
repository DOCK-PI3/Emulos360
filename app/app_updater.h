#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

class PlayerServices;
class QNetworkReply;
class QSaveFile;
class QTemporaryDir;

class AppUpdater final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(AppUpdater)
    QML_UNCREATABLE("Created by the C++ application")
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY changed)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY changed)
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString startupVersion READ startupVersion NOTIFY changed)
    Q_PROPERTY(QString startupNotes READ startupNotes NOTIFY changed)
    Q_PROPERTY(QString startupError READ startupError NOTIFY changed)
public:
    explicit AppUpdater(const QString& dataDir, PlayerServices* players, QObject* parent = nullptr);
    ~AppUpdater() override;
    QString currentVersion() const;
    QString availableVersion() const { return version_; }
    QString releaseNotes() const { return notes_; }
    QString phase() const { return phase_; }
    QString status() const { return status_; }
    int progress() const { return progress_; }
    QString startupVersion() const { return startupVersion_; }
    QString startupNotes() const { return startupNotes_; }
    QString startupError() const { return startupError_; }
    Q_INVOKABLE void check();
    Q_INVOKABLE void startUpdate();
    Q_INVOKABLE void clearStartupNews();
    Q_INVOKABLE void clearStartupError();
signals:
    void changed();
    void updateFound();
private:
    void fail(const QString& message);
    void installDownloaded();
    QString dataDir_;
    PlayerServices* players_ = nullptr;
    QNetworkAccessManager network_;
    QNetworkReply* reply_ = nullptr;
    std::unique_ptr<QTemporaryDir> temp_;
    std::unique_ptr<QSaveFile> download_;
    QString version_;
    QString notes_;
    QString assetName_;
    QString digest_;
    QUrl assetUrl_;
    QString phase_ = QStringLiteral("idle");
    QString status_;
    int progress_ = 0;
    QString startupVersion_;
    QString startupNotes_;
    QString startupError_;
};

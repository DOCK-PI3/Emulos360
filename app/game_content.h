#pragma once
#include <QFutureWatcher>
#include <QObject>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <functional>

class EngineSettings;
class PlayerServices;

class GameContent final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameContent)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString titleId READ titleId NOTIFY changed)
    Q_PROPERTY(QString mediaId READ mediaId NOTIFY changed)
    Q_PROPERTY(QVariantList candidates READ candidates NOTIFY changed)
    Q_PROPERTY(QVariantList installed READ installed NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    GameContent(EngineSettings* engine, PlayerServices* players, QObject* parent = nullptr);
    ~GameContent() override;
    QString title() const { return title_; }
    QString titleId() const { return titleId_; }
    QString mediaId() const { return mediaId_; }
    QVariantList candidates() const { return candidates_; }
    QVariantList installed() const { return installed_; }
    bool busy() const { return busy_; }
    QString status() const { return status_; }
    Q_INVOKABLE void selectGame(const QVariantMap& game);
    Q_INVOKABLE void inspect(const QUrl& source);
    Q_INVOKABLE void install(int index);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clearCandidates();
    Q_INVOKABLE bool isInstalled(const QVariantMap& candidate) const;
signals:
    void changed();
private:
    struct Result {
        QString error, message;
        QVariantList candidates, installed;
        QSharedPointer<QTemporaryDir> stage;
    };
    void startJob(std::function<Result()> job, bool updateCandidates, bool updateInstalled);
    void report(const QString& value);
    void releaseStage();
    EngineSettings* engine_;
    PlayerServices* players_;
    QFutureWatcher<Result> watcher_;
    QSharedPointer<QTemporaryDir> stage_;
    QString title_, titleId_, mediaId_, status_;
    QVariantList candidates_, installed_;
    bool updateCandidates_ = false, updateInstalled_ = false;
    bool busy_ = false;
    bool releaseAfterJob_ = false;
};

#pragma once
#include "engine_settings.h"
#include "core_window_host.h"
#include "avatar_studio.h"
#include <QFutureWatcher>
#include <QLockFile>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <memory>

class PlayerServices final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayerServices)
    QML_UNCREATABLE("Owned by Emulos360")
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QVariantList saves READ saves NOTIFY changed)
    Q_PROPERTY(QVariantList backups READ backups NOTIFY changed)
    Q_PROPERTY(QVariantList achievementGames READ achievementGames NOTIFY achievementsChanged)
    Q_PROPERTY(QString achievementStatus READ achievementStatus NOTIFY achievementsChanged)
    Q_PROPERTY(bool achievementsLoading READ achievementsLoading NOTIFY changed)
    Q_PROPERTY(QString activeXuid READ activeXuid NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool sessionActive READ sessionActive NOTIFY changed)
    Q_PROPERTY(CoreWindowHost* coreWindow READ coreWindow CONSTANT)
    Q_PROPERTY(QString communityXex READ communityXex NOTIFY changed)
    Q_PROPERTY(QString avatarEditorXex READ avatarEditorXex NOTIFY changed)
    Q_PROPERTY(AvatarStudio* avatarStudio READ avatarStudio CONSTANT)
    Q_PROPERTY(int saveDevice READ saveDevice WRITE setSaveDevice NOTIFY changed)
    Q_PROPERTY(QString internalStorage READ internalStorage NOTIFY changed)
    Q_PROPERTY(QString externalStorage READ externalStorage NOTIFY changed)
public:
    PlayerServices(EngineSettings* engine, const QString& data, QObject* parent = nullptr, const QString& coreExecutable = {});
    ~PlayerServices() override;
    QVariantList profiles() const { return profiles_; }
    QVariantList saves() const { return saves_; }
    QVariantList backups() const { return backups_; }
    QVariantList achievementGames() const { return achievementGames_; }
    QString achievementStatus() const { return achievementStatus_; }
    bool achievementsLoading() const { return command_ == "achievements" && busy(); }
    Q_INVOKABLE void refreshAchievements();
    QString activeXuid() const;
    QString status() const { return status_; }
    bool busy() const { return process_.state() != QProcess::NotRunning || fileJob_.isRunning(); }
    bool sessionActive() const { return process_.state() != QProcess::NotRunning && command_.isEmpty(); }
    qint64 sessionProcessId() const { return sessionActive() ? process_.processId() : 0; }
    CoreWindowHost* coreWindow() { return &coreWindow_; }
    QString communityXex() const;
    QString avatarEditorXex() const;
    AvatarStudio* avatarStudio() { return &avatarStudio_; }
    Q_INVOKABLE void editAvatar(const QString& xuid);
    Q_INVOKABLE void refreshProfiles();
    Q_INVOKABLE void createProfile(const QString& name);
    Q_INVOKABLE bool selectProfile(const QString& xuid);
    Q_INVOKABLE void enableNetplayProfile(const QString& xuid);
    Q_INVOKABLE void createAvatar(const QString& xuid, const QString& skin, const QString& shirt, int style);
    Q_INVOKABLE void importAvatar(const QString& xuid, const QUrl& file);
    Q_INVOKABLE void setCommunityXex(const QUrl& file);
    Q_INVOKABLE void setAvatarEditorXex(const QUrl& file);
    Q_INVOKABLE void launchCommunity();
    Q_INVOKABLE void launchAvatarEditor();
    Q_INVOKABLE void launchGame(const QString& path, const QString& titleId = {});
    void launchMetro(const QString& path);
    Q_INVOKABLE void launchNetplay();
    Q_INVOKABLE void chooseExternalStorage(const QUrl& folder);
    int saveDevice() const { return saveDevice_; }
    void setSaveDevice(int value);
    void setConsoleMode(bool enabled) { consoleMode_ = enabled; }
    void setPrivateApi(const QString& endpoint) { privateApi_ = endpoint; }
    QString internalStorage() const { return engine_->contentPath(); }
    QString externalStorage() const { return engine_->externalContentPath(); }
    Q_INVOKABLE void refreshSaves();
    Q_INVOKABLE void backupSave(int index);
    Q_INVOKABLE void restoreSave(int index);
    Q_INVOKABLE void openSaves();
    Q_INVOKABLE void openBackups();
signals:
    void sessionEnded(int code);
    void changed();
    void achievementsChanged();
    void operationFailed(const QString& reason);
private:
    QString privateApi_;
    QString corePath() const;
    QStringList arguments() const;
    bool prepare(bool requireProfile = false);
    QString saveContentPath() const;
    QString backupRoot() const;
    void release();
    void profileCommand(const QString& command, const QString& name = {}, const QString& xuid = {});
    void finishProcess(int code, QProcess::ExitStatus exit);
    void finishAchievements(int code, QProcess::ExitStatus exit);
    void launch(const QString& path, bool xexOnly, const QString& titleId = {}, bool writableGame = false);
    bool knownProfile(const QString& xuid) const;
    void saveAvatar(const QString& xuid, const QImage& image);
    void decorateProfiles();
    void message(const QString& text);
    bool fail(const QString& text);
    EngineSettings* engine_;
    QString data_, status_, output_, command_, knownContent_;
    QString coreExecutable_;
    QSettings preferences_;
    QProcess process_;
    AvatarStudio avatarStudio_;
    QString avatarProfile_, avatarInput_;
    CoreWindowHost coreWindow_;
    QTimer timeout_;
    QFutureWatcher<QString> fileJob_;
    std::unique_ptr<QLockFile> lock_;
    std::unique_ptr<QLockFile> externalLock_;
    int saveDevice_ = 0;
    bool consoleMode_ = false;
    QVariantList profiles_, saves_, backups_;
    QVariantList achievementGames_;
    QString achievementStatus_, achievementXuid_, achievementContent_;
};

#pragma once
#include "library.h"
#include "../XEXplugins/MetroDashboard/host/metro_dashboard.h"
#include "engine_settings.h"
#include "player_services.h"
#include "netplay_rooms.h"
#include "private_netplay.h"
#include "voice_party.h"
#include "game_importer.h"
#include "game_store.h"
#include "game_compatibility.h"
#include "game_content.h"
#include "app_updater.h"
#include <QFutureWatcher>
#include <QObject>
#include <QSettings>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtQml/qqmlregistration.h>

class Controller final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(LibraryController)
    QML_UNCREATABLE("Created by the C++ application")
    Q_PROPERTY(MetroDashboard* metro READ metro CONSTANT)
    Q_PROPERTY(QVariantList games READ games NOTIFY gamesChanged)
    Q_PROPERTY(QString libraryPath READ libraryPath WRITE setLibraryPath NOTIFY settingsChanged)
    Q_PROPERTY(bool consoleMode READ consoleMode WRITE setConsoleMode NOTIFY settingsChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY settingsChanged)
    Q_PROPERTY(QString libraryView READ libraryView WRITE setLibraryView NOTIFY settingsChanged)
    Q_PROPERTY(QString introStyle READ introStyle WRITE setIntroStyle NOTIFY settingsChanged)
    Q_PROPERTY(QUrl introVideo READ introVideo NOTIFY settingsChanged)
    Q_PROPERTY(QUrl introPoster READ introPoster NOTIFY settingsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(EngineSettings* engineSettings READ engineSettings CONSTANT)
    Q_PROPERTY(PlayerServices* players READ players CONSTANT)
    Q_PROPERTY(NetplayRooms* netplayRooms READ netplayRooms CONSTANT)
    Q_PROPERTY(PrivateNetplay* privateNetplay READ privateNetplay CONSTANT)
    Q_PROPERTY(VoiceParty* voiceParty READ voiceParty CONSTANT)
    Q_PROPERTY(GameImporter* importer READ importer CONSTANT)
    Q_PROPERTY(GameStore* store READ store CONSTANT)
    Q_PROPERTY(GameCompatibility* compatibility READ compatibility CONSTANT)
    Q_PROPERTY(GameContent* content READ content CONSTANT)
    Q_PROPERTY(AppUpdater* updater READ updater CONSTANT)
    Q_PROPERTY(QVariantList coverOptions READ coverOptions NOTIFY coversChanged)
    Q_PROPERTY(QVariantMap coverImages READ coverImages NOTIFY coversChanged)
    Q_PROPERTY(bool coverBusy READ coverBusy NOTIFY coversChanged)
    Q_PROPERTY(QString coverStatus READ coverStatus NOTIFY coversChanged)
public:
    explicit Controller(const QString& settingsPath, QObject* parent = nullptr);
    ~Controller() override;
    MetroDashboard* metro() { return metro_; }
    QVariantList games() const { return games_; }
    QString libraryPath() const;
    bool consoleMode() const;
    bool reducedMotion() const;
    QString libraryView() const;
    QString introStyle() const;
    QUrl introVideo() const;
    QUrl introPoster() const;
    bool busy() const { return scanning_; }
    QString status() const { return status_; }
    EngineSettings* engineSettings() { return &engineSettings_; }
    PlayerServices* players() { return &players_; }
    NetplayRooms* netplayRooms() { return &netplayRooms_; }
    PrivateNetplay* privateNetplay() { return &privateNetplay_; }
    VoiceParty* voiceParty() { return &voiceParty_; }
    GameImporter* importer() { return &importer_; }
    GameStore* store() { return &store_; }
    GameCompatibility* compatibility() { return &compatibility_; }
    GameContent* content() { return &content_; }
    AppUpdater* updater() { return &updater_; }
    QVariantList coverOptions() const { return coverOptions_; }
    QVariantMap coverImages() const { return coverImages_; }
    bool coverBusy() const { return !coverReplies_.isEmpty() || nextCover_ < coverOptions_.size(); }
    QString coverStatus() const { return coverStatus_; }
    void setLibraryPath(const QString& path);
    void setConsoleMode(bool value);
    void setReducedMotion(bool value);
    void setLibraryView(const QString& view);
    void setIntroStyle(const QString& style);
    Q_INVOKABLE QUrl introMediaUrl(const QString& style, bool poster = false) const;
    Q_INVOKABLE void scan();
    Q_INVOKABLE bool removeGame(const QString& path);
    void requestGameChromeToggle() { emit toggleGameChromeRequested(); }
    Q_INVOKABLE void chooseFolder(const QUrl& url);
    Q_INVOKABLE void findCovers(const QString& titleId);
    Q_INVOKABLE void cancelCovers();
    Q_INVOKABLE bool useCover(int index);
    Q_INVOKABLE bool useLocalCover(const QString& titleId, const QUrl& fileUrl);
signals:
    void gamesChanged();
    void settingsChanged();
    void statusChanged();
    void coversChanged();
    void toggleGameChromeRequested();
private:
    void fetchNextCover();
    void updateCoverStatus();
    bool saveCoverSelection(const QString& titleId, const QUrl& image,
                            const QString& source, const QString& market,
                            const QString& region);
    MetroDashboard* metro_ = nullptr;
    QSettings settings_;
    EngineSettings engineSettings_;
    PlayerServices players_;
    AppUpdater updater_;
    NetplayRooms netplayRooms_;
    PrivateNetplay privateNetplay_;
    VoiceParty voiceParty_;
    GameImporter importer_;
    GameStore store_;
    GameCompatibility compatibility_;
    GameContent content_;
    QNetworkAccessManager network_;
    QList<QNetworkReply*> coverReplies_;
    QVariantList coverOptions_;
    QVariantMap coverImages_;
    QString coverTitleId_;
    QString coverStatus_;
    QString coverCache_;
    qsizetype nextCover_ = 0;
    QFutureWatcher<los::ScanResult> watcher_;
    QVariantList games_;
    bool scanning_ = false;
    QString status_;
};

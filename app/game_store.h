#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class GameImporter;
class QNetworkReply;

class GameStore final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameStore)
    QML_UNCREATABLE("Created by the C++ application")
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool awaitingDownload READ awaitingDownload NOTIFY changed)
    Q_PROPERTY(QString downloadFolder READ downloadFolder CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit GameStore(GameImporter* importer, QObject* parent = nullptr,
                       const QString& downloadFolder = {});
    ~GameStore() override;
    QVariantList results() const { return results_; }
    bool busy() const { return busy_; }
    bool awaitingDownload() const { return awaitingDownload_; }
    QString downloadFolder() const { return downloadFolder_; }
    QString status() const { return status_; }
    Q_INVOKABLE void search(const QString& query, bool digital);
    Q_INVOKABLE void browseLetter(const QString& letter, bool digital);
    Q_INVOKABLE void openGame(int index, const QString& libraryPath);
    Q_INVOKABLE void openSearchInBrowser(const QString& query, bool digital);
    Q_INVOKABLE void openLetterInBrowser(const QString& letter, bool digital);
    Q_INVOKABLE void importDownload(const QUrl& file, const QString& libraryPath);
    Q_INVOKABLE void stopWaiting();
    void watchDownload(const QString& title, const QString& libraryPath);
    static QVariantList parseSearchHtml(const QByteArray& html, bool digital);
    static QVariantList parseLetterHtml(const QByteArray& html, bool digital);
signals:
    void changed();
private:
    static QUrl searchUrl(const QString& query, bool digital);
    static QUrl letterUrl(const QString& letter, bool digital, int page = 1);
    void cancelRequest();
    void fetchLetterPage(const QString& letter, bool digital, int page);
    void pollDownload();
    GameImporter* importer_;
    QNetworkAccessManager network_;
    QNetworkReply* reply_ = nullptr;
    QTimer downloadTimer_;
    QVariantList results_;
    QSet<QString> resultUrls_;
    QHash<QString, qint64> beforeDownloads_;
    QString downloadFolder_;
    QString selectedTitle_;
    QString libraryPath_;
    QString candidatePath_;
    QString autoArchive_;
    QString status_;
    qint64 candidateSize_ = -1;
    int stablePolls_ = 0;
    bool busy_ = false;
    bool awaitingDownload_ = false;
};

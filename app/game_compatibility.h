#pragma once

#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QNetworkReply;

class GameCompatibility final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameCompatibility)
    QML_UNCREATABLE("Created by the application")
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList legend READ legend CONSTANT)
public:
    explicit GameCompatibility(const QString& dataRoot, QObject* parent = nullptr,
        const QUrl& endpoint = QUrl("https://xenia-manager.github.io/database/data/game-compatibility/canary.json"));
    ~GameCompatibility() override;
    int revision() const { return revision_; }
    bool busy() const { return !reply_.isNull(); }
    QString status() const { return status_; }
    QVariantList legend() const;
    Q_INVOKABLE QVariantMap lookup(const QString& titleId, const QString& title = {}) const;
    Q_INVOKABLE void refresh();
    void refreshIfDue();
signals:
    void changed();
private:
    struct Report {
        QString id, title, key, state, url;
        QStringList labels;
        QDateTime updated;
    };
    bool load(const QByteArray& bytes);
    static QString titleKey(QString title);
    QVariantMap describe(const Report* report, bool nameMatch = false) const;
    const Report* choose(const QList<int>& candidates, const QString& key) const;
    QList<Report> reports_;
    QHash<QString, QList<int>> ids_, names_;
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QByteArray download_;
    QString cachePath_, status_;
    QUrl endpoint_;
    QDateTime fetched_;
    int revision_ = 0;
};

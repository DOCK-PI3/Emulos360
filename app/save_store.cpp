// Emulos360: game-save snapshots. Independent of the Xenia source tree.
#include "save_store.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>

namespace emulos {
namespace {
bool hex(const QString& s, int n) { return QRegularExpression(QString("^[0-9A-Fa-f]{%1}$").arg(n)).match(s).hasMatch(); }
bool link(const QFileInfo& f) {
    return f.isSymLink()
#ifdef Q_OS_WIN
        || f.isJunction()
#endif
        ;
}
bool plainPath(const QString& path) {
    QFileInfo current(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
    while (true) {
        if (link(current)) return false;
        const auto parent = current.absolutePath();
        if (parent == current.absoluteFilePath()) break;
        current = QFileInfo(parent);
    }
    return true;
}
QString savePath(const QString& content, const QString& xuid, const QString& title) {
    return QDir(content).filePath(xuid + '/' + title + "/00000001");
}
QString headerPath(const QString& content, const QString& xuid, const QString& title) {
    return QDir(content).filePath(xuid + '/' + title + "/Headers/00000001");
}
// Copy only ordinary files/directories. Hash the bytes actually written.
bool copyTree(const QString& source, const QString& target, QJsonArray& inventory, const QString& prefix = {}) {
    if (!plainPath(source) || !plainPath(target) || !QFileInfo(source).isDir() || !QDir().mkpath(target)) return false;
    const auto items = QDir(source).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name);
    for (const auto& item : items) {
        if (link(item)) return false;
        const auto name = prefix + item.fileName();
        const auto dest = QDir(target).filePath(item.fileName());
        if (item.isDir()) {
            inventory.append(QJsonObject{{"path", name + '/'}, {"directory", true}});
            if (!copyTree(item.absoluteFilePath(),dest,inventory,name + '/')) return false;
        } else if (item.isFile()) {
            QFile input(item.absoluteFilePath()); QSaveFile output(dest);
            if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) return false;
            QCryptographicHash hash(QCryptographicHash::Sha256);
            qint64 total = 0;
            while (!input.atEnd()) {
                const auto data = input.read(1024*1024);
                if (data.isEmpty() && input.error() != QFile::NoError) return false;
                if (output.write(data) != data.size()) return false;
                total += data.size(); hash.addData(data);
            }
            if (!output.commit()) return false;
            inventory.append(QJsonObject{{"path",name}, {"bytes",QString::number(total)}, {"sha256",QString::fromLatin1(hash.result().toHex())}});
        } else return false;
    }
    return true;
}
QJsonObject readManifest(const QString& path) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly) || f.size() > 16*1024*1024) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}
}
QVariantList savedGames(const QString& content) {
    QVariantList result;
    if (!plainPath(content)) return result;
    for (const auto& profile : QDir(content).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!hex(profile.fileName(),16) || link(profile)) continue;
        for (const auto& title : QDir(profile.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const auto path = savePath(content,profile.fileName(),title.fileName());
            if (hex(title.fileName(),8) && plainPath(path) && QFileInfo(path).isDir())
                result.append(QVariantMap{{"xuid",profile.fileName()},{"titleId",title.fileName()},{"path",path}});
        }
    }
    return result;
}
QVariantList saveBackups(const QString& root) {
    QVariantList result;
    if (!plainPath(root)) return result;
    for (const auto& dir : QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,QDir::Name | QDir::Reversed)) {
        if (link(dir)) continue;
        auto m = readManifest(dir.absoluteFilePath() + "/manifest.json");
        if ((m["version"].toInt() != 1 && m["version"].toInt() != 2) || !hex(m["xuid"].toString(),16) || !hex(m["titleId"].toString(),8)) continue;
        result.append(QVariantMap{{"id",dir.fileName()},{"xuid",m["xuid"].toString()},{"titleId",m["titleId"].toString()},{"created",m["created"].toString()}});
    }
    return result;
}
QString backupSave(const QString& content, const QString& root, const QString& xuid, const QString& title) {
    if (!hex(xuid,16) || !hex(title,8)) return QStringLiteral("Identificador de partida inválido.");
    const auto now = QDateTime::currentDateTimeUtc();
    const auto id = now.toString("yyyyMMdd-HHmmss-zzz") + '-' + QUuid::createUuid().toString(QUuid::Id128);
    const auto dest = QDir(root).filePath(id);
    QJsonArray files;
    if (!copyTree(savePath(content,xuid,title),dest + "/payload",files)) return QStringLiteral("No se pudo copiar la partida. Los originales se conservan.");
    QJsonArray headers;
    const auto headerSource = headerPath(content,xuid,title);
    const bool hasHeaders = QFileInfo::exists(headerSource);
    if (hasHeaders && !copyTree(headerSource,dest + "/headers",headers)) return QStringLiteral("No se pudieron copiar los metadatos de la partida.");
    const QJsonObject manifest{{"version",2},{"xuid",xuid},{"titleId",title},{"created",now.toString(Qt::ISODate)},{"files",files},{"hasHeaders",hasHeaders},{"headers",headers}};
    QSaveFile file(dest + "/manifest.json"); const auto data = QJsonDocument(manifest).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) return QStringLiteral("No se pudo completar la copia.");
    return {};
}
QString restoreSave(const QString& content, const QString& root, const QString& id) {
    if (!QRegularExpression("^[0-9]{8}-[0-9]{6}-[0-9]{3}-[a-f0-9]{32}$").match(id).hasMatch()) return QStringLiteral("Copia no válida.");
    const auto source = QDir(root).filePath(id);
    if (!plainPath(source)) return QStringLiteral("La copia contiene enlaces no admitidos.");
    const auto manifest = readManifest(source + "/manifest.json");
    const auto xuid = manifest["xuid"].toString(), title = manifest["titleId"].toString();
    const int version = manifest["version"].toInt();
    if ((version != 1 && version != 2) || !hex(xuid,16) || !hex(title,8) || !manifest["files"].isArray()) return QStringLiteral("Manifiesto de copia incompatible.");
    // Never restore into a different profile: Xbox saves may be bound to its XUID.
    const auto target = savePath(content,xuid,title);
    const auto suffix = QUuid::createUuid().toString(QUuid::Id128);
    const auto stage = target + ".emulos-restore-" + suffix;
    if (!plainPath(target)) return QStringLiteral("El destino contiene enlaces no admitidos.");
    QJsonArray inventory;
    if (!copyTree(source + "/payload",stage,inventory) || inventory != manifest["files"].toArray()) return QStringLiteral("La copia está incompleta o fue modificada. La partida actual se conserva.");
    QStringList targets{target}, stages{stage};
    if (version == 2) {
        const auto headers = headerPath(content,xuid,title), headerStage = headers + ".emulos-restore-" + suffix;
        if (!plainPath(headers) || !manifest["headers"].isArray() || !manifest["hasHeaders"].isBool()) return QStringLiteral("Metadatos de copia incompatibles.");
        QJsonArray headerInventory;
        const bool ok = manifest["hasHeaders"].toBool() ? copyTree(source+"/headers",headerStage,headerInventory) : QDir().mkpath(headerStage);
        if (!ok || headerInventory != manifest["headers"].toArray()) return QStringLiteral("Los metadatos fueron modificados. La partida actual se conserva.");
        targets << headers; stages << headerStage;
    }
    QList<bool> existed, activated;
    QStringList previous;
    for (qsizetype i=0;i<targets.size();++i) {
        existed << QFileInfo::exists(targets[i]); activated << false;
        previous << targets[i]+".emulos-before-"+suffix;
        if ((existed[i] && !QDir().rename(targets[i],previous[i])) || !QDir().rename(stages[i],targets[i])) {
            // Roll back both halves on ordinary errors. Never delete either snapshot.
            for (qsizetype j=i;j>=0;--j) {
                if (activated[j]) QDir().rename(targets[j],stages[j]);
                if (existed[j] && QFileInfo::exists(previous[j])) QDir().rename(previous[j],targets[j]);
            }
            return QStringLiteral("No se pudo completar la restauración. Conservamos las carpetas de recuperación .emulos-before y .emulos-restore junto a la partida.");
        }
        activated[i]=true;
    }
    return {};
}
}

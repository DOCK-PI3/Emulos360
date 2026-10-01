#include "game_content.h"
#include "engine_settings.h"
#include "player_services.h"
#include "runtime_paths.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QtConcurrentRun>
#include <QtEndian>
#include <functional>

namespace {
quint32 be32(const QByteArray& b, qsizetype n) { return qFromBigEndian<quint32>(b.constData() + n); }
quint32 le32(const QByteArray& b, qsizetype n) { return qFromLittleEndian<quint32>(b.constData() + n); }
quint16 le16(const QByteArray& b, qsizetype n) { return qFromLittleEndian<quint16>(b.constData() + n); }
QString hexId(quint32 value) { return QString::number(value, 16).rightJustified(8, u'0').toUpper(); }
bool validId(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9A-F]{8}$"));
    return pattern.match(value).hasMatch();
}
QString titleMedia(const QVariantMap& game) {
    QFile file(game.value("path").toString());
    if (!file.open(QIODevice::ReadOnly)) return {};
    const auto data = file.read(1024 * 1024);
    if (data.size() < 0x364) return {};
    if (data.first(4) == "LIVE" || data.first(4) == "PIRS" || data.first(4) == "CON ")
        return hexId(be32(data, 0x354));
    if (data.first(4) != "XEX2" || data.size() < 0x18) return {};
    const auto count = be32(data, 0x14);
    if (count > 512 || 0x18u + count * 8u > static_cast<quint32>(data.size())) return {};
    for (quint32 i = 0; i < count; ++i) {
        const qsizetype record = 0x18 + i * 8;
        if (be32(data, record) != 0x00040006) continue;
        const quint32 offset = be32(data, record + 4);
        if (offset > static_cast<quint32>(data.size()) - 0x18u) return {};
        return hexId(be32(data, offset));
    }
    return {};
}
QVariantMap packageInfo(const QByteArray& b, const QString& path, quint64 bytes,
                        const QString& expectedTitle, const QString& expectedMedia) {
    if (b.size() < 0x511 || (b.first(4) != "LIVE" && b.first(4) != "PIRS" && b.first(4) != "CON ")) return {};
    const quint32 type = be32(b, 0x344);
    if (type != 2 && type != 0xB0000) return {};
    const auto title = hexId(be32(b, 0x360));
    const auto media = hexId(be32(b, 0x354));
    const auto contentId = QString::fromLatin1(b.mid(0x32C, 0x14).toHex()).toUpper();
    const auto profileId = QString::fromLatin1(b.mid(0x371, 8).toHex()).toUpper();
    QString name;
    for (qsizetype i = 0x411; i < 0x511; i += 2) {
        const auto ch = qFromBigEndian<quint16>(b.constData() + i);
        if (!ch) break;
        name.append(QChar(ch));
    }
    name = name.trimmed();
    if (name.isEmpty()) name = QFileInfo(path).fileName();
    const bool sameTitle = title == expectedTitle;
    const bool sameMedia = type != 0xB0000 || expectedMedia.isEmpty() ||
                           expectedMedia == QStringLiteral("00000000") || media == QStringLiteral("00000000") ||
                           media == expectedMedia;
    const bool validProfile = profileId == QStringLiteral("0000000000000000");
    QString reason;
    if (!sameTitle) reason = QObject::tr("Pertenece al juego %1").arg(title);
    else if (!sameMedia) reason = QObject::tr("Media ID distinto: %1").arg(media);
    else if (!validProfile) reason = QObject::tr("Contenido ligado a otro perfil");
    return {{"name", name}, {"path", path}, {"fileName", QFileInfo(path).fileName()},
            {"titleId", title}, {"mediaId", media}, {"contentId", contentId}, {"profileId", profileId},
            {"type", type == 2 ? QObject::tr("Contenido adicional") : QObject::tr("Actualización")},
            {"typeId", hexId(type)}, {"size", QLocale().formattedDataSize(bytes)},
            {"bytes", static_cast<qulonglong>(bytes)}, {"match", reason.isEmpty()}, {"reason", reason}};
}
QByteArray headerAt(QFile& file, quint64 offset, quint64 size) {
    if (size < 0x511 || offset > static_cast<quint64>(file.size()) || size > static_cast<quint64>(file.size()) - offset ||
        !file.seek(static_cast<qint64>(offset))) return {};
    return file.read(0x511);
}
QVariantList scanIso(const QString& path, const QString& title, const QString& media, QString& error);
QVariantList scanFiles(const QString& directory, const QString& title, const QString& media, QString& error) {
    QVariantList found;
    QDirIterator it(directory, QDir::Files | QDir::Hidden | QDir::System | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext()) {
        it.next();
        if (++count > 50000) { error = QObject::tr("El paquete contiene demasiados archivos."); return {}; }
        if (QFileInfo(it.filePath()).suffix().compare(QStringLiteral("iso"), Qt::CaseInsensitive) == 0) {
            QString isoError;
            found.append(scanIso(it.filePath(), title, media, isoError));
            if (!isoError.isEmpty()) { error = isoError; return {}; }
            continue;
        }
        QFile file(it.filePath());
        if (!file.open(QIODevice::ReadOnly)) continue;
        const auto item = packageInfo(file.read(0x511), it.filePath(), static_cast<quint64>(file.size()), title, media);
        if (!item.isEmpty()) found.append(item);
    }
    return found;
}
QVariantList scanIso(const QString& path, const QString& title, const QString& media, QString& error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { error = QObject::tr("No se pudo abrir la ISO."); return {}; }
    constexpr quint64 sector = 2048;
    const quint64 offsets[] = {0, 0xFB20, 0x20600, 0x2080000, 0xFD90000, 0x18300000};
    quint64 base = 0;
    QByteArray descriptor;
    bool located = false;
    for (const auto offset : offsets) {
        if (offset + 32 * sector + 28 > static_cast<quint64>(file.size()) || !file.seek(offset + 32 * sector)) continue;
        const auto candidate = file.read(28);
        if (candidate.startsWith("MICROSOFT*XBOX*MEDIA")) { base = offset; descriptor = candidate; located = true; break; }
    }
    if (!located) { error = QObject::tr("No se encontró una partición Xbox 360 válida en la ISO."); return {}; }
    QVariantList found;
    QSet<QPair<quint32, quint32>> visitedDirs;
    int totalEntries = 0;
    std::function<bool(quint32, quint32, int)> walk;
    walk = [&](quint32 sectorNumber, quint32 length, int depth) -> bool {
        if (depth > 24 || !length || length > 32 * 1024 * 1024) return false;
        const quint64 offset = base + quint64(sectorNumber) * sector;
        if (offset > static_cast<quint64>(file.size()) || length > static_cast<quint64>(file.size()) - offset ||
            visitedDirs.contains({sectorNumber, length}) || !file.seek(offset)) return false;
        visitedDirs.insert({sectorNumber, length});
        const auto data = file.read(length);
        if (data.size() != length) return false;
        QSet<quint32> visitedNodes;
        std::function<bool(quint32)> node = [&](quint32 ordinal) -> bool {
            const quint64 pos = quint64(ordinal) * 4;
            if (pos + 14 > static_cast<quint64>(data.size()) || visitedNodes.contains(ordinal) || ++totalEntries > 50000) return false;
            visitedNodes.insert(ordinal);
            const auto left = le16(data, pos), right = le16(data, pos + 2);
            const auto itemSector = le32(data, pos + 4), itemSize = le32(data, pos + 8);
            const quint8 attributes = quint8(data.at(pos + 12));
            const quint8 nameLength = quint8(data.at(pos + 13));
            if (pos + 14 + nameLength > static_cast<quint64>(data.size())) return false;
            const auto name = QString::fromLatin1(data.mid(pos + 14, nameLength));
            if (left && !node(left)) return false;
            if (attributes & 0x10) {
                if (!walk(itemSector, itemSize, depth + 1)) return false;
            } else {
                const quint64 itemOffset = base + quint64(itemSector) * sector;
                const auto header = headerAt(file, itemOffset, itemSize);
                auto item = packageInfo(header, name, itemSize, title, media);
                if (!item.isEmpty()) {
                    item["path"] = path;
                    item["offset"] = static_cast<qulonglong>(itemOffset);
                    item["sourceName"] = name;
                    found.append(item);
                }
            }
            return !right || node(right);
        };
        return node(0);
    };
    if (!walk(le32(descriptor, 20), le32(descriptor, 24), 0)) {
        error = QObject::tr("La estructura de la ISO está dañada o contiene rutas repetidas."); return {};
    }
    return found;
}
bool extractIsoItem(const QVariantMap& item, const QString& destination, QString& error) {
    QFile source(item.value("path").toString());
    QFile target(destination);
    const quint64 offset = item.value("offset").toULongLong(), size = item.value("bytes").toULongLong();
    if (!source.open(QIODevice::ReadOnly) || !source.seek(static_cast<qint64>(offset)) ||
        !target.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        error = QObject::tr("No se pudo preparar el contenido de la ISO."); return false;
    }
    quint64 left = size;
    while (left) {
        const auto data = source.read(static_cast<qint64>(qMin<quint64>(left, 4 * 1024 * 1024)));
        if (data.isEmpty() || target.write(data) != data.size()) {
            target.remove(); error = QObject::tr("No se pudo extraer el paquete de la ISO."); return false;
        }
        left -= data.size();
    }
    return true;
}
bool safeArchive(const QByteArray& listing, QString& error) {
    const auto lines = QString::fromUtf8(listing).split(u'\n');
    int count = 0;
    quint64 size = 0;
    for (const auto& raw : lines) {
        if (raw.startsWith(QStringLiteral("Path = "))) {
            const auto path = QDir::fromNativeSeparators(raw.mid(7).trimmed());
            if (++count == 1) continue; // 7z's first record is the archive itself.
            if (path.isEmpty() || path.startsWith(u'/') || path.contains(u':') ||
                path.split(u'/').contains(QStringLiteral(".."))) {
                error = QObject::tr("El archivo contiene una ruta insegura: %1").arg(path); return false;
            }
            if (count > 50000) { error = QObject::tr("El archivo contiene demasiadas entradas."); return false; }
        } else if (raw.startsWith(QStringLiteral("Size = "))) {
            bool ok = false;
            const auto length = raw.mid(7).trimmed().toULongLong(&ok);
            if (ok) { size += length; if (size > 128ull * 1024 * 1024 * 1024) { error = QObject::tr("El archivo supera el límite de 128 GB."); return false; } }
        } else if (raw.startsWith(QStringLiteral("Attributes = ")) && raw.contains(u'L')) {
            error = QObject::tr("El archivo contiene enlaces y no se puede extraer con seguridad."); return false;
        }
    }
    if (count < 2) error = QObject::tr("El 7z no contiene archivos.");
    return count >= 2;
}
QVariantList installedItems(const QString& root, const QString& title, const QString& media) {
    QVariantList items;
    if (!validId(title)) return items;
    const QDir base(root);
    for (const auto& profile : base.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
        if (!QRegularExpression(QStringLiteral("^[0-9A-Fa-f]{16}$")).match(profile.fileName()).hasMatch()) continue;
        const QDir game(QDir(profile.absoluteFilePath()).filePath(title));
        if (!game.exists() || QFileInfo(game.path()).isSymLink() || QFileInfo(game.path()).isJunction()) continue;
        const auto canonicalGame = QFileInfo(game.path()).canonicalFilePath();
        if (canonicalGame.isEmpty()) continue;
        for (const auto& type : {QStringLiteral("00000002"), QStringLiteral("000B0000")}) {
            const QDir folder(game.filePath(type));
            if (!folder.exists() || QFileInfo(folder.path()).isSymLink() || QFileInfo(folder.path()).isJunction()) continue;
            for (const auto& info : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
                if (info.isJunction()) continue;
                const auto headerPath = info.isDir() ? game.filePath("Headers/" + type + "/" + info.fileName() + ".header") : info.absoluteFilePath();
                const auto headerInfo = QFileInfo(headerPath);
                if (headerInfo.isSymLink() || headerInfo.isJunction() ||
                    !headerInfo.canonicalFilePath().startsWith(canonicalGame + u'/')) continue;
                QFile header(headerPath);
                if (!header.open(QIODevice::ReadOnly)) continue;
                auto item = packageInfo(header.read(0x511), info.fileName(), static_cast<quint64>(info.size()), title, media);
                if (item.isEmpty() || item.value("titleId").toString() != title || item.value("typeId").toString() != type) continue;
                item["path"] = info.absoluteFilePath();
                item["header"] = info.isDir() ? headerPath : QString();
                item["profile"] = profile.fileName();
                items.append(item);
            }
        }
    }
    return items;
}
QString coreExecutable() {
#ifdef Q_OS_WIN
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("engine/Emulos360-core.exe"));
#else
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("engine/Emulos360-core"));
#endif
}
QString installViaCore(const QString& source, const QString& title, const QString& media,
                       const QString& storage, const QString& content, const QString& config) {
    QTemporaryDir ipc;
    if (!ipc.isValid()) return QObject::tr("No se pudo crear el archivo temporal de respuesta.");
    const auto resultFile = QDir(ipc.path()).filePath(QStringLiteral("result.toml"));
    QProcess process;
    process.setWorkingDirectory(QFileInfo(coreExecutable()).absolutePath());
    const QStringList args = {"--config=" + config, "--storage_root=" + storage, "--content_root=" + content,
        "--network_mode=0", "--auto_check_updates=false", "--emulos_profile_command=install-game-content",
        "--emulos_profile_output=" + resultFile, "--emulos_content_package_file=" + source,
        "--emulos_game_title_id=" + title, "--emulos_game_media_id=" + media,
        "--log_file=" + QDir(ipc.path()).filePath(QStringLiteral("content-install.log"))};
    process.start(coreExecutable(), args);
    if (!process.waitForStarted(15000)) return QObject::tr("No se pudo iniciar el motor: %1").arg(process.errorString());
    if (!process.waitForFinished(30 * 60 * 1000)) {
        process.kill(); process.waitForFinished();
        return QObject::tr("La instalación superó el límite de 30 minutos.");
    }
    QFile result(resultFile);
    if (!result.open(QIODevice::ReadOnly)) return QObject::tr("El motor terminó sin devolver el resultado (código %1).").arg(process.exitCode());
    const auto response = QString::fromUtf8(result.readAll());
    if (response.contains(QRegularExpression(QStringLiteral("(?m)^ok\\s*=\\s*true\\s*$")))) return {};
    const auto match = QRegularExpression(QStringLiteral("(?m)^error\\s*=\\s*\"([^\"]*)\"" )).match(response);
    return match.hasMatch() ? match.captured(1) : QObject::tr("El motor rechazó el paquete. Revisa el registro de instalación.");
}
}

GameContent::GameContent(EngineSettings* engine, PlayerServices* players, QObject* parent)
    : QObject(parent), engine_(engine), players_(players) {
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, [this] {
        const auto result = watcher_.result();
        busy_ = false;
        if (updateCandidates_) { candidates_ = result.candidates; stage_ = result.stage; }
        if (updateInstalled_) installed_ = result.installed;
        status_ = result.error.isEmpty() ? result.message : result.error;
        if (releaseAfterJob_) {
            releaseAfterJob_ = false;
            candidates_.clear();
            releaseStage();
        }
        emit changed();
    });
}
GameContent::~GameContent() { watcher_.waitForFinished(); }
void GameContent::releaseStage() {
    if (stage_) {
        const auto oldStage = stage_;
        stage_.clear();
        QtConcurrent::run([oldStage] {});
    }
}
void GameContent::clearCandidates() {
    if (busy()) { releaseAfterJob_ = true; return; }
    candidates_.clear();
    releaseStage();
    emit changed();
}
void GameContent::report(const QString& value) { status_ = value; emit changed(); }
void GameContent::startJob(std::function<Result()> job, bool candidates, bool installed) {
    if (busy()) { report(tr("Espera a que termine la operación actual.")); return; }
    updateCandidates_ = candidates;
    updateInstalled_ = installed;
    busy_ = true;
    watcher_.setFuture(QtConcurrent::run(std::move(job)));
    emit changed();
}
void GameContent::selectGame(const QVariantMap& game) {
    if (busy()) return;
    title_ = game.value("title").toString();
    titleId_ = game.value("titleId").toString().toUpper();
    mediaId_ = titleMedia(game);
    candidates_.clear(); releaseStage(); installed_.clear();
    report(tr("Selecciona un paquete DLC, una actualización, una ISO o un archivo 7z."));
    refresh();
}
void GameContent::inspect(const QUrl& source) {
    if (busy() || !validId(titleId_)) return;
    const auto path = source.toLocalFile();
    const auto title = titleId_, media = mediaId_;
    candidates_.clear(); releaseStage();
    report(tr("Analizando paquetes…"));
    startJob([path, title, media] {
        Result result;
        const QFileInfo info(path);
        if (!info.isFile() || info.isSymLink() || info.isJunction()) {
            result.error = QObject::tr("Elige un archivo local normal."); return result;
        }
        const auto suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("iso")) {
            result.candidates = scanIso(path, title, media, result.error);
        } else if (suffix == QStringLiteral("7z")) {
            const auto tool = emulos::findImportTool(QStringLiteral("7z.exe"));
            if (tool.isEmpty()) { result.error = QObject::tr("No se encuentra 7z para abrir este paquete."); return result; }
            QProcess list;
            list.start(tool, {"l", "-slt", path});
            if (!list.waitForFinished(120000) || list.exitCode() != 0 ||
                !safeArchive(list.readAllStandardOutput(), result.error)) {
                if (result.error.isEmpty()) result.error = QObject::tr("No se pudo comprobar el archivo 7z.");
                return result;
            }
            result.stage = QSharedPointer<QTemporaryDir>::create();
            if (!result.stage->isValid()) { result.error = QObject::tr("No hay carpeta temporal para extraer el 7z."); return result; }
            QProcess extract;
            extract.start(tool, {"x", "-y", "-o" + result.stage->path(), path});
            if (!extract.waitForFinished(30 * 60 * 1000) || extract.exitCode() != 0) {
                result.error = QObject::tr("No se pudo extraer el 7z."); return result;
            }
            result.candidates = scanFiles(result.stage->path(), title, media, result.error);
        } else {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                const auto item = packageInfo(file.read(0x511), path, static_cast<quint64>(file.size()), title, media);
                if (!item.isEmpty()) result.candidates.append(item);
            }
        }
        if (result.error.isEmpty()) result.message = result.candidates.isEmpty()
            ? QObject::tr("No se encontraron DLC ni actualizaciones Xbox 360 en el archivo.")
            : QObject::tr("%1 paquete(s) encontrados; solo se podrán instalar los que correspondan a este juego.").arg(result.candidates.size());
        return result;
    }, true, false);
}
void GameContent::install(int index) {
    if (busy() || players_->busy() || index < 0 || index >= candidates_.size()) return;
    const auto item = candidates_.at(index).toMap();
    if (!item.value("match").toBool()) { report(tr("Este paquete no corresponde al juego seleccionado.")); return; }
    const auto content = engine_->contentPath(), storage = engine_->storagePath(), config = engine_->configPath();
    const auto title = titleId_, media = mediaId_;
    const auto stage = stage_;
    report(tr("Instalando contenido; espera a que termine…"));
    startJob([item, content, storage, config, title, media, stage] {
        Result result;
        QDir().mkpath(content);
        QLockFile lock(QDir(content).filePath(QStringLiteral(".emulos-session.lock")));
        if (!lock.tryLock(0)) { result.error = QObject::tr("El motor está usando la carpeta de contenido. Cierra el juego antes de instalar."); return result; }
        if (!QFileInfo::exists(coreExecutable())) { result.error = QObject::tr("Falta Emulos360-core en la build."); return result; }
        QString source = item.value("path").toString();
        QTemporaryDir temporary;
        if (item.contains("offset")) {
            if (!temporary.isValid()) { result.error = QObject::tr("No se pudo crear la carpeta temporal."); return result; }
            const auto originalName = item.value("sourceName").toString();
            if (originalName.isEmpty() || originalName == "." || originalName == ".." ||
                originalName.contains(u'/') || originalName.contains(u'\\') || originalName.contains(u':')) {
                result.error = QObject::tr("La ISO contiene un nombre de paquete inseguro."); return result;
            }
            source = QDir(temporary.path()).filePath(originalName);
            if (!extractIsoItem(item, source, result.error)) return result;
        }
        result.error = installViaCore(source, title, media, storage, content, config);
        result.installed = installedItems(content, title, media);
        if (result.error.isEmpty()) result.message = QObject::tr("Contenido instalado y comprobado en la biblioteca del motor.");
        return result;
    }, false, true);
}
void GameContent::refresh() {
    if (busy() || !validId(titleId_)) return;
    const auto content = engine_->contentPath(), title = titleId_, media = mediaId_;
    startJob([content, title, media] { Result result; result.installed = installedItems(content, title, media); return result; }, false, true);
}
bool GameContent::isInstalled(const QVariantMap& candidate) const {
    const auto title = candidate.value("titleId").toString();
    const auto type = candidate.value("typeId").toString();
    const auto profile = candidate.value("profileId").toString();
    const auto contentId = candidate.value("contentId").toString();
    const auto fileName = candidate.value("fileName").toString();
    if (title.isEmpty() || type.isEmpty() || profile.isEmpty()) return false;
    const auto emptyId = QStringLiteral("0000000000000000000000000000000000000000");
    for (const auto& value : installed_) {
        const auto item = value.toMap();
        if (item.value("titleId").toString().compare(title, Qt::CaseInsensitive) != 0 ||
            item.value("typeId").toString().compare(type, Qt::CaseInsensitive) != 0 ||
            item.value("profile").toString().compare(profile, Qt::CaseInsensitive) != 0) continue;
        const auto installedId = item.value("contentId").toString();
        if (!contentId.isEmpty() && contentId != emptyId &&
            !installedId.isEmpty() && installedId != emptyId) {
            if (installedId.compare(contentId, Qt::CaseInsensitive) == 0) return true;
        } else if (!fileName.isEmpty() &&
                   item.value("fileName").toString().compare(fileName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}
void GameContent::remove(int index) {
    if (busy() || players_->busy() || index < 0 || index >= installed_.size()) return;
    const auto item = installed_.at(index).toMap();
    const auto root = engine_->contentPath(), title = titleId_, media = mediaId_;
    report(tr("Eliminando contenido seleccionado…"));
    startJob([item, root, title, media] {
        Result result;
        QLockFile lock(QDir(root).filePath(QStringLiteral(".emulos-session.lock")));
        if (!lock.tryLock(0)) { result.error = QObject::tr("Cierra el juego antes de eliminar contenido."); return result; }
        const auto path = item.value("path").toString(), headerPath = item.value("header").toString();
        bool stillKnown = false;
        for (const auto& entry : installedItems(root, title, media))
            if (entry.toMap().value("path").toString() == path) stillKnown = true;
        if (!stillKnown) { result.error = QObject::tr("El paquete cambió desde que se mostró la lista. Actualiza y vuelve a intentarlo."); return result; }
        const auto canonicalRoot = QFileInfo(root).canonicalFilePath();
        const auto canonical = QFileInfo(path).canonicalFilePath();
        if (canonicalRoot.isEmpty() || !canonical.startsWith(canonicalRoot + u'/') ||
            QFileInfo(path).isSymLink() || QFileInfo(path).isJunction()) {
            result.error = QObject::tr("Ruta de contenido insegura; no se ha eliminado."); return result;
        }
        if (QFileInfo(path).isDir()) {
            QDirIterator it(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) { it.next(); if (it.fileInfo().isSymLink() || it.fileInfo().isJunction()) {
                result.error = QObject::tr("El paquete contiene enlaces; no se eliminará automáticamente."); return result;
            } }
        }
        if (!QFile::moveToTrash(path) && !(QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path))) {
            result.error = QObject::tr("No se pudo eliminar el contenido."); return result;
        }
        if (!headerPath.isEmpty() && QFileInfo::exists(headerPath) && !QFile::remove(headerPath))
            result.error = QObject::tr("Se eliminó el paquete, pero no su cabecera; revisa %1.").arg(headerPath);
        result.installed = installedItems(root, title, media);
        if (result.error.isEmpty()) result.message = QObject::tr("Contenido eliminado. El juego y las partidas permanecen intactos.");
        return result;
    }, false, true);
}

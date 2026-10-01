#include "controller.h"
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QCryptographicHash>
#include <QImageReader>
#include <QSaveFile>
#include <QRegularExpression>
#include <QTimer>
#include <QtConcurrentRun>

namespace {
QString gameKey(const QString& path) {
    auto key = QFileInfo(path).canonicalFilePath();
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}
bool insideLibrary(const QString& root, const QString& path) {
    const auto relative = QDir(root).relativeFilePath(path);
    return !relative.isEmpty() && relative != "." && relative != ".." &&
           !relative.startsWith("../") && !QDir::isAbsolutePath(relative);
}
bool removeGamePath(const QString& path, bool directory, bool& permanentlyDeleted) {
    if (QFile::moveToTrash(path)) return true;
    if (directory) {
        QDirIterator entries(path, QDir::AllEntries | QDir::Hidden | QDir::System |
                                  QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (entries.hasNext()) {
            entries.next();
            const auto info = entries.fileInfo();
            if (info.isSymbolicLink() || info.isJunction()) return false;
        }
    }
    permanentlyDeleted = true;
    return directory ? QDir(path).removeRecursively() : QFile::remove(path);
}
int removeEmptyParents(const QString& root, QString directory) {
    int removed = 0;
    while (insideLibrary(root, directory)) {
        const QFileInfo info(directory);
        if (!info.isDir() || info.isSymbolicLink() || info.isJunction() ||
            !insideLibrary(root, info.canonicalFilePath()) ||
            !QDir(directory).entryList(QDir::AllEntries | QDir::Hidden | QDir::System |
                                       QDir::NoDotAndDotDot).isEmpty()) break;
        const auto parent = info.absolutePath();
        if (!QDir().rmdir(directory)) break;
        ++removed;
        directory = parent;
    }
    return removed;
}
}

Controller::Controller(const QString& settingsPath, QObject* parent)
    : QObject(parent), settings_(settingsPath, QSettings::IniFormat),
      engineSettings_(QFileInfo(settingsPath).absolutePath() + "/engine", this),
      players_(&engineSettings_,QFileInfo(settingsPath).absolutePath(),this),
      netplayRooms_(&engineSettings_,this), privateNetplay_(QFileInfo(settingsPath).absolutePath(),this),
      voiceParty_(this), importer_(this), store_(&importer_, this),
      compatibility_(QFileInfo(settingsPath).absolutePath(), this),
      content_(&engineSettings_, &players_, this) {
    QTimer::singleShot(2000, &compatibility_, &GameCompatibility::refreshIfDue);
    metro_ = new MetroDashboard(this,QFileInfo(settingsPath).absolutePath(),this);
    players_.setConsoleMode(consoleMode());
    connect(&privateNetplay_, &PrivateNetplay::endpointChanged, this, [this](const QString& bridge) {
        players_.setPrivateApi(bridge);
        netplayRooms_.setServerOverride(bridge);
    });
    coverCache_ = QFileInfo(settingsPath).absolutePath() + "/covers";
    settings_.beginGroup("covers");
    for (const auto& key : settings_.childGroups()) {
        const auto path = QDir(coverCache_).filePath(settings_.value(key + "/file").toString());
        if (QFileInfo(path).isFile()) coverImages_[key] = QUrl::fromLocalFile(path);
    }
    settings_.endGroup();
    connect(&watcher_, &QFutureWatcher<los::ScanResult>::finished, this, [this] {
        const auto result = watcher_.result();
        games_ = result.games;
        scanning_ = false;
        status_ = result.error.isEmpty()
            ? tr("%1 juegos GOD/XBLA/XEX detectados.").arg(games_.size())
            : result.error;
        emit gamesChanged();
        emit statusChanged();
    });
    connect(&importer_, &GameImporter::installed, this, &Controller::scan);
}
Controller::~Controller() { cancelCovers(); watcher_.waitForFinished(); }
QString Controller::libraryPath() const { return settings_.value("libraryPath").toString(); }
bool Controller::consoleMode() const { return settings_.value("consoleMode", false).toBool(); }
bool Controller::reducedMotion() const { return settings_.value("reducedMotion", false).toBool(); }
QString Controller::libraryView() const {
    const auto value = settings_.value("appearance/libraryView", "grid").toString();
    return value == QStringLiteral("carousel") ? value : QStringLiteral("grid");
}
void Controller::setLibraryView(const QString& view) {
    if ((view != QStringLiteral("grid") && view != QStringLiteral("carousel")) || view == libraryView()) return;
    settings_.setValue("appearance/libraryView", view);
    emit settingsChanged();
}
QString Controller::introStyle() const {
    const auto value = settings_.value("appearance/introStyle", "aurora").toString();
    return value == "nova" ? value : QStringLiteral("aurora");
}
QUrl Controller::introMediaUrl(const QString& style, bool poster) const {
    if (style != "aurora" && style != "nova") return {};
    return QUrl::fromLocalFile(QDir(QCoreApplication::applicationDirPath()).filePath(
        "intro/" + style + (poster ? ".png" : ".mp4")));
}
QUrl Controller::introVideo() const { return introMediaUrl(introStyle()); }
QUrl Controller::introPoster() const { return introMediaUrl(introStyle(), true); }
void Controller::setIntroStyle(const QString& style) {
    if ((style != "aurora" && style != "nova") || style == introStyle()) return;
    settings_.setValue("appearance/introStyle", style);
    emit settingsChanged();
}
void Controller::setLibraryPath(const QString& path) {
    if (busy()) return;
    settings_.setValue("libraryPath", QDir::cleanPath(path));
    emit settingsChanged();
    scan();
}

void Controller::cancelCovers() {
    nextCover_ = coverOptions_.size();
    const auto replies = coverReplies_;
    coverReplies_.clear();
    for (auto* reply : replies) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
    emit coversChanged();
}

void Controller::findCovers(const QString& titleId) {
    cancelCovers();
    coverOptions_.clear();
    coverTitleId_ = titleId.toUpper();
    nextCover_ = 0;
    static const QRegularExpression validId("^[0-9A-F]{8}$");
    if (!validId.match(coverTitleId_).hasMatch()) {
        coverStatus_ = tr("Identificador de juego inválido.");
        emit coversChanged();
        return;
    }
    struct Market { const char* name; const char* region; const char* locale; int lcid; };
    const Market markets[] = {
        {"España · Español", "Europa", "es-ES", 3082},
        {"Reino Unido · Inglés", "Europa", "en-GB", 2057},
        {"Francia · Francés", "Europa", "fr-FR", 1036},
        {"Alemania · Alemán", "Europa", "de-DE", 1031},
        {"Estados Unidos · Inglés", "América", "en-US", 1033},
        {"México · Español", "América", "es-MX", 2058},
        {"Japón · Japonés", "Asia", "ja-JP", 1041}
    };
    for (const auto& market : markets) {
        // This legacy public image CDN serves HTTP; do not disable TLS verification.
        const auto url = QString("http://download.xbox.com/content/images/66acd000-77fe-1000-9115-d802%1/%2/boxartlg.jpg")
            .arg(titleId.toLower()).arg(market.lcid);
        coverOptions_.append(QVariantMap{{"name", QString::fromUtf8(market.name)},
            {"region", QString::fromUtf8(market.region)}, {"locale", market.locale},
            {"provider", "Xbox Marketplace"}, {"url", url}, {"state", "pending"}});
    }
    coverOptions_.append(QVariantMap{{"name", tr("Archivo x360db")}, {"region", "Sin especificar"},
        {"locale", ""}, {"provider", "x360db"}, {"state", "pending"},
        {"url", QString("https://raw.githubusercontent.com/xenia-manager/x360db/main/titles/%1/artwork/boxart.jpg").arg(coverTitleId_)}});
    coverStatus_ = tr("Buscando carátulas disponibles…");
    emit coversChanged();
    fetchNextCover();
}

void Controller::fetchNextCover() {
    constexpr qsizetype maxBytes = 4 * 1024 * 1024;
    while (coverReplies_.size() < 3 && nextCover_ < coverOptions_.size()) {
        const auto index = nextCover_++;
        auto option = coverOptions_[index].toMap();
        const auto url = option.value("url").toString();
        const auto hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha256).toHex();
        const auto filePath = QDir(coverCache_).filePath(QString::fromLatin1(hash) + ".png");
        if (QFileInfo(filePath).isFile()) {
            option["image"] = QUrl::fromLocalFile(filePath);
            option["state"] = "ready";
            coverOptions_[index] = option;
            continue;
        }
        QNetworkRequest request{QUrl(url)};
        request.setRawHeader("User-Agent", "Emulos360/0.2 CoverLibrary");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(12000);
        auto* reply = network_.get(request);
        reply->setReadBufferSize(maxBytes + 1);
        coverReplies_.append(reply);
        // A hard deadline also stops servers which send bytes indefinitely.
        QTimer::singleShot(15000, reply, [reply] { if (reply->isRunning()) reply->abort(); });
        connect(reply, &QIODevice::readyRead, this, [reply] {
            if (reply->bytesAvailable() > maxBytes) reply->abort();
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, index, filePath] {
            auto result = coverOptions_[index].toMap();
            result["state"] = "unavailable";
            result["error"] = tr("No disponible en este origen");
            const auto bytes = reply->readAll();
            if (reply->error() == QNetworkReply::NoError && bytes.size() <= maxBytes) {
                QBuffer buffer;
                buffer.setData(bytes);
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);
                const auto size = reader.size();
                if (size.width() >= 64 && size.height() >= 64 && size.width() <= 4096 && size.height() <= 4096) {
                    reader.setScaledSize(size.scaled(900, 1200, Qt::KeepAspectRatio));
                    const auto image = reader.read();
                    if (!image.isNull() && QDir().mkpath(coverCache_)) {
                        QSaveFile file(filePath);
                        if (file.open(QIODevice::WriteOnly) && image.save(&file, "PNG") && file.commit()) {
                            result["image"] = QUrl::fromLocalFile(filePath);
                            result["state"] = "ready";
                            result.remove("error");
                        } else result["error"] = tr("No se pudo guardar la carátula");
                    }
                }
            } else if (reply->error() != QNetworkReply::ContentNotFoundError) {
                result["error"] = tr("Descarga fallida · puedes reintentar");
            }
            coverOptions_[index] = result;
            coverReplies_.removeOne(reply);
            reply->deleteLater();
            fetchNextCover();
        });
    }
    updateCoverStatus();
}

void Controller::updateCoverStatus() {
    int available = 0;
    for (const auto& item : coverOptions_) if (item.toMap().value("state") == "ready") ++available;
    coverStatus_ = coverBusy() ? tr("Buscando… %1 carátulas disponibles").arg(available)
        : available ? tr("%1 opciones disponibles. Elige la que prefieras.").arg(available)
                    : tr("Sin carátulas disponibles. Comprueba la conexión o vuelve a intentarlo.");
    emit coversChanged();
}

bool Controller::useCover(int index) {
    if (index < 0 || index >= coverOptions_.size()) return false;
    const auto option = coverOptions_[index].toMap();
    if (option.value("state") != "ready") return false;
    const auto image = option.value("image").toUrl();
    return saveCoverSelection(coverTitleId_, image, option.value("url").toString(),
                              option.value("name").toString(), option.value("region").toString());
}

bool Controller::useLocalCover(const QString& titleId, const QUrl& fileUrl) {
    const auto id = titleId.trimmed().toUpper();
    static const QRegularExpression validId("^[0-9A-F]{8}$");
    const auto fail = [this](const QString& message) {
        coverStatus_ = message;
        emit coversChanged();
        return false;
    };
    if (!validId.match(id).hasMatch() || !fileUrl.isLocalFile())
        return fail(tr("Selecciona una imagen local para este juego."));
    cancelCovers();

    const QFileInfo original(fileUrl.toLocalFile());
    constexpr qint64 maxFileBytes = 32 * 1024 * 1024;
    if (!original.isFile() || !original.isReadable() || original.size() > maxFileBytes)
        return fail(tr("No se puede leer la imagen o supera los 32 MB."));

    QImageReader reader(original.absoluteFilePath());
    reader.setAutoTransform(true);
    if (!reader.canRead()) return fail(tr("Formato de imagen no válido o no compatible."));
    const auto size = reader.size();
    if (size.width() < 64 || size.height() < 64 || size.width() > 8192 || size.height() > 8192)
        return fail(tr("La imagen debe medir entre 64 y 8192 píxeles por lado."));
    reader.setScaledSize(size.scaled(900, 1200, Qt::KeepAspectRatio));
    const auto image = reader.read();
    if (image.isNull()) return fail(tr("Formato de imagen no válido o no compatible."));

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        return fail(tr("No se pudo preparar la carátula."));
    const auto hash = QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex();
    const auto filePath = QDir(coverCache_).filePath(
        QStringLiteral("manual-%1-%2.png").arg(id, QString::fromLatin1(hash)));
    if (!QDir().mkpath(coverCache_)) return fail(tr("No se pudo crear la carpeta de carátulas."));
    if (!QFileInfo::exists(filePath)) {
        QSaveFile saved(filePath);
        if (!saved.open(QIODevice::WriteOnly) || saved.write(png) != png.size() || !saved.commit())
            return fail(tr("No se pudo guardar la carátula en Emulos360."));
    }
    return saveCoverSelection(id, QUrl::fromLocalFile(filePath),
                              QStringLiteral("local"), tr("Imagen del equipo"), QString());
}

bool Controller::saveCoverSelection(const QString& titleId, const QUrl& image,
                                    const QString& source, const QString& market,
                                    const QString& region) {
    const auto prefix = "covers/" + titleId + "/";
    settings_.setValue(prefix + "file", QFileInfo(image.toLocalFile()).fileName());
    settings_.setValue(prefix + "source", source);
    settings_.setValue(prefix + "market", market);
    settings_.setValue(prefix + "region", region);
    settings_.sync();
    if (settings_.status() != QSettings::NoError) {
        coverStatus_ = tr("No se pudo guardar la selección.");
        emit coversChanged();
        return false;
    }
    coverImages_[titleId] = image;
    emit coversChanged();
    return true;
}
void Controller::setConsoleMode(bool value) {
    players_.setConsoleMode(value);
    settings_.setValue("consoleMode", value);
    emit settingsChanged();
}
void Controller::setReducedMotion(bool value) {
    settings_.setValue("reducedMotion", value);
    emit settingsChanged();
}
void Controller::chooseFolder(const QUrl& url) {
    if (url.isLocalFile()) setLibraryPath(url.toLocalFile());
}
bool Controller::removeGame(const QString& path) {
    if (busy() || players_.busy()) return false;
    const auto key = gameKey(path);
    const auto root = QDir(libraryPath()).canonicalPath();
    const QFileInfo file(path);
    if (key.isEmpty() || root.isEmpty() || !insideLibrary(root, key) ||
        !file.isFile() || file.isSymLink()) {
        status_ = tr("No se puede borrar este juego: su archivo ya no está dentro de la biblioteca.");
        emit statusChanged();
        return false;
    }
    for (qsizetype index = 0; index < games_.size(); ++index) {
        const auto game = games_[index].toMap();
        if (gameKey(game.value("path").toString()) != key) continue;
        const auto format = game.value("format").toString();
        QString target = key;
        bool permanentlyDeleted = false;
        if (format == "XEX") {
            target = QFileInfo(key).absolutePath();
            if (!insideLibrary(root, target)) {
                status_ = tr("No se puede borrar un juego XEX situado en la raíz de la biblioteca.");
                emit statusChanged();
                return false;
            }
            for (const auto& other : games_) {
                const auto otherPath = gameKey(other.toMap().value("path").toString());
                if (otherPath != key && insideLibrary(target, otherPath)) {
                    status_ = tr("La carpeta contiene otros juegos. Sepáralos antes de borrar este XEX.");
                    emit statusChanged();
                    return false;
                }
            }
        } else if (format == "GOD") {
            const auto dataPath = key + QStringLiteral(".data");
            const QFileInfo data(dataPath);
            if (data.isSymbolicLink() || data.isJunction() ||
                (data.exists() && (!data.isDir() || !insideLibrary(root, data.canonicalFilePath())))) {
                status_ = tr("La carpeta de fragmentos no es segura para borrar.");
                emit statusChanged();
                return false;
            }
            if (data.exists() && !removeGamePath(dataPath, true, permanentlyDeleted)) {
                status_ = tr("No se pudieron eliminar los fragmentos del juego.");
                emit statusChanged();
                return false;
            }
        } else if (format != "XBLA") {
            status_ = tr("Formato de juego no admitido para borrar.");
            emit statusChanged();
            return false;
        }
        if (!removeGamePath(target, format == "XEX", permanentlyDeleted)) {
            status_ = tr("No se pudieron eliminar todos los archivos. Comprueba los permisos y los enlaces de la carpeta.");
            emit statusChanged();
            return false;
        }
        const int emptyFolders = removeEmptyParents(root, QFileInfo(target).absolutePath());
        games_.removeAt(index);
        status_ = permanentlyDeleted
            ? tr("%1 se borró definitivamente.").arg(game.value("title").toString())
            : tr("%1 se movió a la Papelera.").arg(game.value("title").toString());
        if (emptyFolders) status_ += tr(" También se eliminaron las carpetas vacías.");
        emit gamesChanged();
        emit statusChanged();
        return true;
    }
    status_ = tr("El juego seleccionado ya no está en la biblioteca.");
    emit statusChanged();
    return false;
}
void Controller::scan() {
    if (busy()) return;
    if (libraryPath().isEmpty()) {
        status_ = tr("Añade una carpeta para descubrir tu biblioteca Xbox 360.");
        emit statusChanged();
        return;
    }
    status_ = tr("Explorando biblioteca…");
    scanning_ = true;
    watcher_.setFuture(QtConcurrent::run(los::scanLibrary, libraryPath()));
    emit statusChanged();
}

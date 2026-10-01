#include "app_updater.h"
#include "player_services.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QVersionNumber>

namespace {
constexpr auto releaseEndpoint = "https://api.github.com/repos/DOCK-PI3/Emulos360/releases/latest";
constexpr qint64 maximumArchiveSize = 2LL * 1024 * 1024 * 1024;

QString assetSuffix() {
#ifdef Q_OS_WIN
    return QStringLiteral("-windows-x64.zip");
#else
    return QStringLiteral("-linux-x86_64.tar.gz");
#endif
}

bool validAssetUrl(const QUrl& url, const QString& tag, const QString& name) {
    return url.scheme() == QStringLiteral("https") &&
           url.host().compare(QStringLiteral("github.com"), Qt::CaseInsensitive) == 0 &&
           url.path() == QStringLiteral("/DOCK-PI3/Emulos360/releases/download/") + tag +
                         QStringLiteral("/") + name && !url.hasQuery() && !url.hasFragment();
}
}

AppUpdater::AppUpdater(const QString& dataDir, PlayerServices* players, QObject* parent)
    : QObject(parent), dataDir_(QDir(dataDir).absolutePath()), players_(players) {
    QFile news(QDir(dataDir_).filePath(QStringLiteral("update-news.json")));
    if (news.open(QIODevice::ReadOnly)) {
        const auto document = QJsonDocument::fromJson(news.readAll());
        startupVersion_ = document.object().value(QStringLiteral("version")).toString();
        startupNotes_ = document.object().value(QStringLiteral("notes")).toString();
        news.close();
        news.remove();
    }
    QFile error(QDir(dataDir_).filePath(QStringLiteral("update-error.txt")));
    if (error.open(QIODevice::ReadOnly)) {
        startupError_ = QString::fromUtf8(error.readAll());
        error.close();
        error.remove();
    }
}

AppUpdater::~AppUpdater() {
    if (reply_) reply_->abort();
}

QString AppUpdater::currentVersion() const {
    return QCoreApplication::applicationVersion();
}

void AppUpdater::clearStartupNews() {
    startupVersion_.clear();
    startupNotes_.clear();
    emit changed();
}

void AppUpdater::clearStartupError() {
    startupError_.clear();
    emit changed();
}

void AppUpdater::fail(const QString& message) {
    download_.reset();
    temp_.reset();
    phase_ = QStringLiteral("error");
    status_ = message;
    emit changed();
}

void AppUpdater::check() {
    if (reply_ || phase_ == QStringLiteral("downloading") ||
        phase_ == QStringLiteral("installing")) return;
    phase_ = QStringLiteral("checking");
    status_ = tr("Buscando versiones nuevas…");
    emit changed();
    QNetworkRequest request(QUrl(QString::fromLatin1(releaseEndpoint)));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", "Emulos360-updater/" + currentVersion().toUtf8());
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    reply_ = network_.get(request);
    connect(reply_, &QNetworkReply::finished, this, [this] {
        auto* finished = reply_;
        reply_ = nullptr;
        const auto data = finished->readAll();
        const auto networkError = finished->error();
        const auto code = finished->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        finished->deleteLater();
        if (networkError != QNetworkReply::NoError || code != 200 || data.size() > 1024 * 1024) {
            fail(tr("No se pudo consultar GitHub. Comprueba la conexión y vuelve a intentarlo."));
            return;
        }
        QJsonParseError parseError;
        const auto release = QJsonDocument::fromJson(data, &parseError).object();
        const auto tag = release.value(QStringLiteral("tag_name")).toString();
        static const QRegularExpression versionPattern(QStringLiteral("^v([0-9]+\\.[0-9]+\\.[0-9]+)$"));
        const auto match = versionPattern.match(tag);
        if (parseError.error != QJsonParseError::NoError || !match.hasMatch() ||
            release.value(QStringLiteral("draft")).toBool() ||
            release.value(QStringLiteral("prerelease")).toBool()) {
            fail(tr("La release publicada no tiene una versión estable válida."));
            return;
        }
        const auto latest = match.captured(1);
        if (QVersionNumber::compare(QVersionNumber::fromString(latest),
                                    QVersionNumber::fromString(currentVersion())) <= 0) {
            phase_ = QStringLiteral("current");
            status_ = tr("Emulos360 está actualizado (%1).").arg(currentVersion());
            emit changed();
            return;
        }
        const auto expectedName = QStringLiteral("Emulos360-") + tag + assetSuffix();
        for (const auto& value : release.value(QStringLiteral("assets")).toArray()) {
            const auto asset = value.toObject();
            if (asset.value(QStringLiteral("name")).toString() != expectedName ||
                asset.value(QStringLiteral("state")).toString() != QStringLiteral("uploaded")) continue;
            const auto url = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
            const auto digest = asset.value(QStringLiteral("digest")).toString();
            static const QRegularExpression hashPattern(QStringLiteral("^sha256:([0-9a-fA-F]{64})$"));
            const auto hash = hashPattern.match(digest);
            const auto size = asset.value(QStringLiteral("size")).toVariant().toLongLong();
            if (!validAssetUrl(url, tag, expectedName) || !hash.hasMatch() ||
                size <= 0 || size > maximumArchiveSize) {
                fail(tr("La release %1 no tiene un paquete verificable para este sistema.").arg(tag));
                return;
            }
            version_ = latest;
            notes_ = release.value(QStringLiteral("body")).toString();
            assetName_ = expectedName;
            assetUrl_ = url;
            digest_ = hash.captured(1).toLower();
            progress_ = 0;
            phase_ = QStringLiteral("available");
            status_ = tr("Disponible Emulos360 %1.").arg(latest);
            emit changed();
            emit updateFound();
            return;
        }
        fail(tr("La release %1 no incluye un paquete para este sistema.").arg(tag));
    });
}

void AppUpdater::startUpdate() {
    if (phase_ != QStringLiteral("available")) return;
    if (players_ && players_->busy()) {
        fail(tr("Cierra el juego y espera a que terminen las operaciones antes de actualizar Emulos360."));
        return;
    }
    temp_ = std::make_unique<QTemporaryDir>(QDir::tempPath() + QStringLiteral("/Emulos360-update-XXXXXX"));
    if (!temp_->isValid()) { fail(tr("No se pudo crear la carpeta temporal de actualización.")); return; }
    download_ = std::make_unique<QSaveFile>(temp_->filePath(assetName_));
    if (!download_->open(QIODevice::WriteOnly)) {
        fail(tr("No se pudo guardar la descarga de la actualización."));
        return;
    }
    phase_ = QStringLiteral("downloading");
    status_ = tr("Descargando Emulos360 %1…").arg(version_);
    progress_ = 0;
    emit changed();
    QNetworkRequest request(assetUrl_);
    request.setRawHeader("User-Agent", "Emulos360-updater/" + currentVersion().toUtf8());
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    reply_ = network_.get(request);
    connect(reply_, &QIODevice::readyRead, this, [this] {
        if (!reply_ || !download_) return;
        const auto bytes = reply_->readAll();
        if (download_->write(bytes) != bytes.size() || download_->size() > maximumArchiveSize)
            reply_->abort();
    });
    connect(reply_, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            progress_ = static_cast<int>(qBound<qint64>(qint64(0), received * 100 / total, qint64(100)));
            emit changed();
        }
    });
    connect(reply_, &QNetworkReply::finished, this, [this] {
        auto* finished = reply_;
        reply_ = nullptr;
        const auto remaining = finished->readAll();
        const auto networkError = finished->error();
        const auto code = finished->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        finished->deleteLater();
        if (networkError != QNetworkReply::NoError || code != 200 || !download_ ||
            download_->write(remaining) != remaining.size() || !download_->commit()) {
            fail(tr("La descarga se interrumpió. La instalación actual no se modificó."));
            return;
        }
        download_.reset();
        QFile archive(temp_->filePath(assetName_));
        if (!archive.open(QIODevice::ReadOnly) || archive.size() > maximumArchiveSize) {
            fail(tr("No se pudo verificar el archivo descargado."));
            return;
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!archive.atEnd()) hash.addData(archive.read(1024 * 1024));
        if (QString::fromLatin1(hash.result().toHex()) != digest_) {
            fail(tr("La descarga no coincide con el SHA-256 publicado en GitHub."));
            return;
        }
        archive.close();
        installDownloaded();
    });
}

void AppUpdater::installDownloaded() {
    if (players_ && players_->busy()) {
        fail(tr("Cierra el juego y espera a que terminen las operaciones antes de instalar la actualización."));
        return;
    }
    const auto installDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const auto helper = QDir(installDir).filePath(QStringLiteral("updates/apply-update-windows.ps1"));
    const auto copiedHelper = temp_->filePath(QStringLiteral("apply-update-windows.ps1"));
    const auto program = qEnvironmentVariable("SystemRoot", QStringLiteral("C:/Windows")) +
                         QStringLiteral("/System32/WindowsPowerShell/v1.0/powershell.exe");
#else
    const auto helper = QDir(installDir).filePath(QStringLiteral("updates/apply-update-linux.py"));
    const auto copiedHelper = temp_->filePath(QStringLiteral("apply-update-linux.py"));
    const auto program = QStringLiteral("python3");
#endif
    if (!QFileInfo::exists(helper) || !QFile::copy(helper, copiedHelper)) {
        fail(tr("Falta el instalador de actualizaciones en esta build."));
        return;
    }
    QSaveFile news(temp_->filePath(QStringLiteral("update-news.json")));
    const auto document = QJsonDocument(QJsonObject{{QStringLiteral("version"), version_},
                                                    {QStringLiteral("notes"), notes_}}).toJson();
    if (!news.open(QIODevice::WriteOnly) || news.write(document) != document.size() || !news.commit()) {
        fail(tr("No se pudieron preparar las novedades de la versión."));
        return;
    }
    const QStringList common{installDir, temp_->filePath(assetName_),
                             QString::number(QCoreApplication::applicationPid()), dataDir_,
                             temp_->filePath(QStringLiteral("update-news.json")), temp_->path()};
#ifdef Q_OS_WIN
    QStringList args{QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                     QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                     QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                     QStringLiteral("-File"), copiedHelper};
    args += common;
#else
    QStringList args{copiedHelper};
    args += common;
#endif
    if (!QProcess::startDetached(program, args, temp_->path())) {
        fail(tr("No se pudo iniciar el instalador de actualizaciones."));
        return;
    }
    temp_->setAutoRemove(false);
    phase_ = QStringLiteral("installing");
    status_ = tr("Instalando. Emulos360 se cerrará y se reiniciará automáticamente.");
    emit changed();
    QCoreApplication::quit();
}

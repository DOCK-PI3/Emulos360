#include "game_store.h"
#include "game_importer.h"
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QUrlQuery>

namespace {
QString normalized(const QString& value) {
    QString result = value.toLower().normalized(QString::NormalizationForm_KD);
    result.remove(QRegularExpression(QStringLiteral("[^a-z0-9]")));
    return result;
}

bool matchesTitle(const QString& fileName, const QString& title) {
    const auto file = normalized(fileName);
    const auto game = normalized(title);
    return !game.isEmpty() && file.contains(game);
}

QString catalogTable(const QString& page) {
    static const QRegularExpression tables(QStringLiteral(R"(</?table\b[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression catalogClass(QStringLiteral(R"(\bclass\s*=\s*["'][^"']*\bhovertable\b[^"']*["'])"),
        QRegularExpression::CaseInsensitiveOption);
    auto table = tables.globalMatch(page);
    qsizetype start = -1;
    int depth = 0;
    while (table.hasNext()) {
        const auto tag = table.next();
        const bool closing = tag.captured().startsWith(QStringLiteral("</"));
        if (start < 0) {
            if (!closing && catalogClass.match(tag.captured()).hasMatch()) {
                start = tag.capturedEnd();
                depth = 1;
            }
        } else if (closing) {
            if (--depth == 0) return page.mid(start, tag.capturedStart() - start);
        } else {
            ++depth;
        }
    }
    return {};
}

QVariantList parseRows(const QString& page, bool digital) {
    QVariantList results;
    static const QRegularExpression rows(QStringLiteral(R"(<tr\b[^>]*>(.*?)</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression firstCell(QStringLiteral(R"(<td\b[^>]*>(.*?)</td>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression links(QStringLiteral(R"(<a\b([^>]*\bhref\s*=\s*["']?\s*/vault/(\d+)["']?[^>]*)>(.*?)</a>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression flags(QStringLiteral(R"(/images/flags/([a-z-]+)\.png)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression type(QStringLiteral(R"(<(?:span|b)\b[^>]*\bredBorder\b[^>]*\btitle\s*=\s*["']([^"']+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QString catalog = catalogTable(page);
    auto row = rows.globalMatch(catalog);
    while (row.hasNext() && results.size() < 500) {
        const QString body = row.next().captured(1);
        const auto cell = firstCell.match(body);
        if (!cell.hasMatch()) continue;
        auto link = links.globalMatch(cell.captured(1));
        while (link.hasNext()) {
            const auto found = link.next();
            if (found.captured(2) == QStringLiteral("999999") ||
                found.captured(1).contains(QStringLiteral("display:"), Qt::CaseInsensitive)) continue;
            const QString title = QTextDocumentFragment::fromHtml(found.captured(3)).toPlainText().trimmed();
            if (title.isEmpty()) continue;
            QStringList regions;
            auto region = flags.globalMatch(body);
            while (region.hasNext()) {
                const auto name = region.next().captured(1);
                if (!regions.contains(name, Qt::CaseInsensitive)) regions.append(name);
            }
            const auto typeMatch = type.match(cell.captured(1));
            const auto kind = typeMatch.hasMatch() ? typeMatch.captured(1) :
                (digital ? QStringLiteral("Xbox 360 digital") : QStringLiteral("Xbox 360"));
            results.append(QVariantMap{
                {QStringLiteral("title"), title},
                {QStringLiteral("region"), regions.join(QStringLiteral(", "))},
                {QStringLiteral("kind"), kind},
                {QStringLiteral("url"), QStringLiteral("https://vimm.net/vault/") + found.captured(2)}
            });
            break;
        }
    }
    return results;
}
}

GameStore::GameStore(GameImporter* importer, QObject* parent, const QString& downloadFolder)
    : QObject(parent), importer_(importer),
      downloadFolder_(downloadFolder.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : downloadFolder) {
    downloadTimer_.setInterval(2500);
    connect(&downloadTimer_, &QTimer::timeout, this, &GameStore::pollDownload);
    connect(importer_, &GameImporter::completed, this, [this](bool installed) {
        if (autoArchive_.isEmpty()) return;
        if (installed) {
            if (QFile::remove(autoArchive_))
                status_ = tr("Juego instalado. Se eliminó el 7z descargado.");
            else
                status_ = tr("Juego instalado. No se pudo eliminar el 7z de Descargas.");
        } else {
            status_ = tr("La importación falló. Se conservó el 7z descargado: %1").arg(importer_->status());
        }
        autoArchive_.clear();
        emit changed();
    });
}

GameStore::~GameStore() {
    cancelRequest();
}

void GameStore::cancelRequest() {
    auto* previous = reply_;
    reply_ = nullptr;
    if (!previous) return;
    QObject::disconnect(previous, nullptr, this, nullptr);
    previous->abort();
    previous->deleteLater();
}

QUrl GameStore::searchUrl(const QString& query, bool digital) {
    QUrl url(QStringLiteral("https://vimm.net/vault/"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("p"), QStringLiteral("list"));
    parameters.addQueryItem(QStringLiteral("system"), digital ? QStringLiteral("X360-D") : QStringLiteral("Xbox360"));
    parameters.addQueryItem(QStringLiteral("q"), query.trimmed());
    url.setQuery(parameters);
    return url;
}

QUrl GameStore::letterUrl(const QString& letter, bool digital, int page) {
    const QString system = digital ? QStringLiteral("X360-D") : QStringLiteral("Xbox360");
    QUrl url(QStringLiteral("https://vimm.net/vault/") + system + u'/' + letter);
    if (page > 1) {
        url.setPath(url.path() + u'/');
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("page"), QString::number(page));
        url.setQuery(query);
    }
    return url;
}

QVariantList GameStore::parseSearchHtml(const QByteArray& html, bool digital) {
    const QString page = QString::fromUtf8(html);
    if (!page.contains(QStringLiteral("Search results for"), Qt::CaseInsensitive)) return {};
    return parseRows(page, digital);
}

QVariantList GameStore::parseLetterHtml(const QByteArray& html, bool digital) {
    return parseRows(QString::fromUtf8(html), digital);
}

void GameStore::search(const QString& query, bool digital) {
    cancelRequest();
    results_.clear();
    resultUrls_.clear();
    if (query.trimmed().size() < 2) {
        busy_ = false;
        status_ = tr("Escribe al menos dos caracteres para buscar.");
        emit changed();
        return;
    }
    const QUrl url = searchUrl(query, digital);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Emulos360/0.2"));
    request.setTransferTimeout(20000);
    request.setMaximumRedirectsAllowed(2);
    busy_ = true;
    status_ = tr("Buscando en Vimm…");
    emit changed();
    auto* response = network_.get(request);
    reply_ = response;
    connect(response, &QNetworkReply::finished, this, [this, response, digital] {
        if (reply_ != response) { response->deleteLater(); return; }
        reply_ = nullptr;
        busy_ = false;
        const auto body = response->readAll();
        const auto code = response->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (response->error() != QNetworkReply::NoError || code != 200 || body.size() > 4 * 1024 * 1024) {
            status_ = tr("No se pudo consultar el catálogo. Puedes abrir la búsqueda en tu navegador.");
        } else if (body.contains("Just a moment") || body.contains("cf-challenge")) {
            status_ = tr("Vimm solicita una comprobación. Abre la búsqueda en tu navegador.");
        } else {
            results_ = parseSearchHtml(body, digital);
            status_ = results_.isEmpty() ? tr("No se encontraron juegos, o Vimm cambió su catálogo.")
                                        : tr("%1 resultados. Abre una ficha para descargar.").arg(results_.size());
        }
        response->deleteLater();
        emit changed();
    });
}

void GameStore::browseLetter(const QString& letter, bool digital) {
    if (letter.size() != 1 || letter.at(0) < u'A' || letter.at(0) > u'Z') return;
    cancelRequest();
    results_.clear();
    resultUrls_.clear();
    busy_ = true;
    status_ = tr("Cargando la letra %1…").arg(letter);
    emit changed();
    fetchLetterPage(letter, digital, 1);
}

void GameStore::fetchLetterPage(const QString& letter, bool digital, int page) {
    QNetworkRequest request(letterUrl(letter, digital, page));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Emulos360/0.2"));
    request.setTransferTimeout(20000);
    request.setMaximumRedirectsAllowed(2);
    auto* response = network_.get(request);
    reply_ = response;
    connect(response, &QNetworkReply::finished, this, [this, response, letter, digital, page] {
        if (reply_ != response) { response->deleteLater(); return; }
        reply_ = nullptr;
        const QByteArray body = response->readAll();
        const auto code = response->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool failed = response->error() != QNetworkReply::NoError || code != 200 ||
                            body.size() > 4 * 1024 * 1024 || body.contains("Just a moment") ||
                            body.contains("cf-challenge");
        response->deleteLater();
        if (failed) {
            busy_ = false;
            status_ = results_.isEmpty()
                ? tr("No se pudo cargar la letra %1. Puedes abrirla en el navegador.").arg(letter)
                : tr("Se cargaron %1 títulos de %2; faltan páginas. Prueba otra vez o abre la web.")
                      .arg(results_.size()).arg(letter);
            emit changed();
            return;
        }
        const auto entries = parseLetterHtml(body, digital);
        if (entries.isEmpty() && page == 1) {
            busy_ = false;
            status_ = tr("No hay títulos en la letra %1, o Vimm cambió el catálogo.").arg(letter);
            emit changed();
            return;
        }
        for (const auto& entry : entries) {
            const auto url = entry.toMap().value(QStringLiteral("url")).toString();
            if (!resultUrls_.contains(url)) {
                resultUrls_.insert(url);
                results_.append(entry);
            }
        }
        static const QRegularExpression nextLink(QStringLiteral(
            R"(<a\s+href=["']/vault/(Xbox360|X360-D)/([A-Z])/?\?page=(\d+)["'][^>]*>\s*Next\s*</a>)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto next = nextLink.match(QString::fromUtf8(body));
        const bool hasNext = next.hasMatch() &&
            next.captured(1).compare(digital ? QStringLiteral("X360-D") : QStringLiteral("Xbox360"), Qt::CaseInsensitive) == 0 &&
            next.captured(2).compare(letter, Qt::CaseInsensitive) == 0 &&
            next.captured(3).toInt() == page + 1;
        if (hasNext && page < 100) {
            status_ = tr("Letra %1: %2 títulos cargados; buscando el resto…")
                .arg(letter).arg(results_.size());
            emit changed();
            fetchLetterPage(letter, digital, page + 1);
        } else {
            busy_ = false;
            status_ = hasNext
                ? tr("Letra %1: %2 títulos cargados; el índice supera el límite de páginas.")
                    .arg(letter).arg(results_.size())
                : tr("Letra %1: %2 títulos. Selecciona uno para abrir su ficha.")
                    .arg(letter).arg(results_.size());
            emit changed();
        }
    });
}

void GameStore::openSearchInBrowser(const QString& query, bool digital) {
    const QUrl url = query.trimmed().isEmpty()
        ? QUrl(digital ? QStringLiteral("https://vimm.net/vault/X360-D")
                       : QStringLiteral("https://vimm.net/vault/Xbox360"))
        : searchUrl(query, digital);
    status_ = QDesktopServices::openUrl(url)
        ? tr("Búsqueda abierta en el navegador. Puedes importar el 7z con «Elegir descarga».")
        : tr("No se pudo abrir el navegador predeterminado.");
    emit changed();
}

void GameStore::openLetterInBrowser(const QString& letter, bool digital) {
    if (letter.size() != 1 || letter.at(0) < u'A' || letter.at(0) > u'Z') return;
    status_ = QDesktopServices::openUrl(letterUrl(letter, digital))
        ? tr("Letra %1 abierta en el navegador.").arg(letter)
        : tr("No se pudo abrir el navegador predeterminado.");
    emit changed();
}

void GameStore::openGame(int index, const QString& libraryPath) {
    if (index < 0 || index >= results_.size()) return;
    const auto game = results_.at(index).toMap();
    const QUrl url(game.value(QStringLiteral("url")).toString());
    if (url.scheme() != QStringLiteral("https") || url.host() != QStringLiteral("vimm.net")) return;
    if (!QDesktopServices::openUrl(url)) {
        status_ = tr("No se pudo abrir el navegador predeterminado.");
        emit changed();
        return;
    }
    watchDownload(game.value(QStringLiteral("title")).toString(), libraryPath);
}

void GameStore::watchDownload(const QString& title, const QString& libraryPath) {
    stopWaiting();
    selectedTitle_ = title;
    libraryPath_ = libraryPath;
    beforeDownloads_.clear();
    const QDir downloads(downloadFolder_);
    for (const auto& file : downloads.entryInfoList({QStringLiteral("*.7z")}, QDir::Files | QDir::NoSymLinks))
        beforeDownloads_.insert(file.absoluteFilePath(), file.size());
    awaitingDownload_ = downloads.exists();
    if (awaitingDownload_) downloadTimer_.start();
    status_ = awaitingDownload_
        ? tr("Ficha abierta en el navegador. Espera la comprobación automática y pulsa Descargar allí. Vigilaré Descargas para importar el 7z al terminar.")
        : tr("Ficha abierta en el navegador. Elige el 7z descargado desde esta pantalla.");
    emit changed();
}

void GameStore::importDownload(const QUrl& file, const QString& libraryPath) {
    stopWaiting();
    autoArchive_.clear(); // A manually chosen original always belongs to the user.
    importer_->importFile(file, libraryPath);
    status_ = tr("Importando el archivo elegido; el original se conservará.");
    emit changed();
}

void GameStore::stopWaiting() {
    downloadTimer_.stop();
    awaitingDownload_ = false;
    candidatePath_.clear();
    candidateSize_ = -1;
    stablePolls_ = 0;
    emit changed();
}

void GameStore::pollDownload() {
    if (importer_->busy()) return;
    const QDir downloads(downloadFolder_);
    const auto files = downloads.entryInfoList({QStringLiteral("*.7z")}, QDir::Files | QDir::NoSymLinks,
                                               QDir::Time);
    for (const auto& file : files) {
        const auto path = file.absoluteFilePath();
        if (!matchesTitle(file.fileName(), selectedTitle_)) continue;
        if (beforeDownloads_.contains(path) && beforeDownloads_.value(path) == file.size()) continue;
        const auto stem = path.left(path.size() - 3);
        if (QFileInfo::exists(path + QStringLiteral(".crdownload")) ||
            QFileInfo::exists(path + QStringLiteral(".part")) ||
            QFileInfo::exists(stem + QStringLiteral(".part"))) continue;
        if (candidatePath_ == path && candidateSize_ == file.size()) ++stablePolls_;
        else { candidatePath_ = path; candidateSize_ = file.size(); stablePolls_ = 0; }
        if (file.size() <= 0 || stablePolls_ < 2) return;
        stopWaiting();
        autoArchive_ = path;
        importer_->importFile(QUrl::fromLocalFile(path), libraryPath_);
        if (!importer_->busy()) {
            autoArchive_.clear();
            status_ = tr("No se pudo empezar la importación: %1").arg(importer_->status());
        } else {
            status_ = tr("Descarga detectada. Extrayendo e instalando en la biblioteca…");
        }
        emit changed();
        return;
    }
    candidatePath_.clear();
    candidateSize_ = -1;
    stablePolls_ = 0;
}

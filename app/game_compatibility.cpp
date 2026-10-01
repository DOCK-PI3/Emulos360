#include "game_compatibility.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTimer>

namespace {
constexpr qsizetype maxBytes = 4 * 1024 * 1024;
const QSet<QString> states{"Playable", "Gameplay", "Loads", "Unplayable", "Unknown"};
}

GameCompatibility::GameCompatibility(const QString& dataRoot, QObject* parent, const QUrl& endpoint)
    : QObject(parent), cachePath_(QDir(dataRoot).filePath("compatibility/canary.json")), endpoint_(endpoint) {
    QFile bundled(":/app/compatibility-canary.json");
    if (bundled.open(QIODevice::ReadOnly)) load(bundled.read(maxBytes + 1));
    QFile cached(cachePath_);
    if (cached.open(QIODevice::ReadOnly) && load(cached.read(maxBytes + 1)))
        fetched_ = QFileInfo(cached).lastModified().toUTC();
    status_ = tr("%1 informes de Xenia Canary disponibles.").arg(reports_.size());
}

GameCompatibility::~GameCompatibility() {
    if (reply_) {
        disconnect(reply_, nullptr, this, nullptr);
        reply_->abort();
    }
}

QString GameCompatibility::titleKey(QString title) {
    // Remove only distribution/region suffixes, preserving demos, sequels and editions.
    static const QRegularExpression suffix(
        "\\s*\\((?:World|USA|Europe|Japan|Asia|Australia|Korea|China|Russia|France|Germany|Spain|Italy|UK|XBLA|Xbox Live Arcade|(?:En|Fr|De|Es|It|Ja|Ko|Zh|Ru|Pt|Nl|Sv|No|Da|Fi)(?:,(?:En|Fr|De|Es|It|Ja|Ko|Zh|Ru|Pt|Nl|Sv|No|Da|Fi))*)\\)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    title = title.trimmed();
    while (suffix.match(title).hasMatch()) title.remove(suffix);
    if (title.endsWith(", The", Qt::CaseInsensitive)) title = "The " + title.chopped(5);
    title.replace(u'&', " and ");
    title = title.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString key;
    for (const auto character : title) {
        if (character.isLetterOrNumber()) key += character;
        else if (character.category() != QChar::Mark_NonSpacing &&
                 character != QChar(0x2122) && character != QChar(0x00ae)) key += u' ';
    }
    return key.simplified();
}

bool GameCompatibility::load(const QByteArray& bytes) {
    if (bytes.size() > maxBytes) return false;
    const auto document = QJsonDocument::fromJson(bytes);
    if (!document.isArray() || document.array().size() > 10000) return false;
    QList<Report> reports;
    QHash<QString, QList<int>> ids, names;
    static const QRegularExpression idPattern("^[0-9A-F]{8}$");
    static const QRegularExpression reportUrl("^https://github\\.com/xenia-canary/game-compatibility/issues/[0-9]+$");
    for (const auto item : document.array()) {
        const auto object = item.toObject();
        Report report;
        report.id = object.value("id").toString().toUpper();
        report.title = object.value("title").toString().trimmed();
        report.state = object.value("state").toString();
        if (!idPattern.match(report.id).hasMatch() || report.id == "00000000" ||
            report.title.isEmpty() || report.title.size() > 512 || !states.contains(report.state)) continue;
        report.key = titleKey(report.title);
        if (report.key.isEmpty()) continue;
        report.updated = QDateTime::fromString(object.value("updated").toString(), Qt::ISODate);
        const auto url = object.value("url").toString();
        if (reportUrl.match(url).hasMatch()) report.url = url;
        auto labels = object.value("labels").toArray();
        if (object.value("labels").isObject()) labels = object.value("labels").toObject().value("others").toArray();
        for (const auto label : labels) {
            const auto value = label.toString();
            if (value.size() <= 100 && report.labels.size() < 64) report.labels.append(value);
        }
        const auto index = static_cast<int>(reports.size());
        ids[report.id].append(index);
        names[report.key].append(index);
        reports.append(std::move(report));
    }
    if (reports.isEmpty()) return false;
    reports_ = std::move(reports);
    ids_ = std::move(ids);
    names_ = std::move(names);
    ++revision_;
    return true;
}

const GameCompatibility::Report* GameCompatibility::choose(const QList<int>& candidates, const QString& key) const {
    const Report* selected = nullptr;
    for (const auto index : candidates) {
        const auto& report = reports_[index];
        if (candidates.size() > 1 && !key.isEmpty() && report.key != key) continue;
        if (selected && (selected->id != report.id || selected->key != report.key)) return nullptr;
        if (!selected || report.updated > selected->updated) selected = &report;
    }
    return selected;
}

QVariantMap GameCompatibility::lookup(const QString& titleId, const QString& title) const {
    const auto id = titleId.trimmed().toUpper();
    const auto key = titleKey(title);
    if (!id.isEmpty()) return describe(choose(ids_.value(id), key));
    return describe(choose(names_.value(key), key), true);
}

QVariantList GameCompatibility::legend() const {
    QVariantList entries;
    for (const auto* state : {"Playable", "Gameplay", "Loads", "Unplayable", "Unknown"}) {
        Report report;
        report.state = QString::fromLatin1(state);
        entries.append(describe(&report));
    }
    return entries;
}

QVariantMap GameCompatibility::describe(const Report* report, bool nameMatch) const {
    const auto state = report ? report->state : QStringLiteral("Unknown");
    QString code, label, description, color;
    if (state == "Playable") {
        code = "playable"; color = "#86d270"; label = tr("Jugable");
        description = tr("Se ha podido jugar de principio a fin con pocos fallos o ninguno.");
    } else if (state == "Gameplay") {
        code = "gameplay"; color = "#74b8ff"; label = tr("En juego");
        description = tr("Permite entrar y jugar, pero no está confirmado que se pueda terminar. Puede tener fallos.");
    } else if (state == "Loads") {
        code = "loads"; color = "#f8c85c"; label = tr("Solo arranca");
        description = tr("Arranca, pero no llega a una partida: puede quedarse en la introducción, los menús o la carga.");
    } else if (state == "Unplayable") {
        code = "unplayable"; color = "#ff827d"; label = tr("No jugable");
        description = tr("Se bloquea o presenta fallos graves que impiden jugar.");
    } else {
        code = "unknown"; color = "#a0aaa2"; label = tr("Sin datos");
        description = tr("No hay un estado conocido o no se puede identificar esta edición con seguridad.");
    }
    QStringList notes;
    if (report) {
        if (report->labels.contains("gpu-drawing-corrupt") || report->labels.contains("gpu-drawing-missing"))
            notes.append(tr("Errores gráficos reportados."));
        if (report->labels.contains("crash-always") || report->labels.contains("crash-random"))
            notes.append(tr("Cuelgues o cierres reportados."));
        if (report->labels.contains("apu-garbage") || report->labels.contains("apu-silent"))
            notes.append(tr("Fallos de audio reportados."));
        if (nameMatch) notes.append(tr("Coincidencia por nombre; la edición exacta no está verificada."));
        if (report->updated.isValid()) notes.append(tr("Informe actualizado: %1.").arg(report->updated.date().toString("dd/MM/yyyy")));
    }
    const auto detail = description + (notes.isEmpty() ? QString{} : "\n" + notes.join("\n"));
    return {{"state", code}, {"label", label}, {"color", color}, {"description", description},
            {"tooltip", label + "\n" + detail + "\n" + tr("Referencia comunitaria de Xenia Canary; puede variar según equipo, versión y ajustes.")},
            {"titleId", report ? report->id : QString{}}, {"url", report ? report->url : QString{}},
            {"matchedByName", report && nameMatch}};
}

void GameCompatibility::refreshIfDue() {
    if (!fetched_.isValid() || fetched_.secsTo(QDateTime::currentDateTimeUtc()) > 24 * 60 * 60) refresh();
}

void GameCompatibility::refresh() {
    if (busy()) return;
    QNetworkRequest request(endpoint_);
    request.setRawHeader("User-Agent", "Emulos360/0.2 Compatibility");
    request.setTransferTimeout(12000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    reply_ = network_.get(request);
    auto* reply = reply_.data();
    reply->setReadBufferSize(maxBytes + 1);
    download_.clear();
    status_ = tr("Actualizando informes de compatibilidad…");
    emit changed();
    QTimer::singleShot(15000, reply, [reply] { if (reply->isRunning()) reply->abort(); });
    connect(reply, &QIODevice::readyRead, this, [this, reply] {
        download_ += reply->readAll();
        if (download_.size() > maxBytes) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->isOpen()) download_ += reply->readAll();
        const bool valid = reply->error() == QNetworkReply::NoError && load(download_);
        reply_.clear();
        if (valid) {
            fetched_ = QDateTime::currentDateTimeUtc();
            QDir().mkpath(QFileInfo(cachePath_).absolutePath());
            QSaveFile file(cachePath_);
            const bool saved = file.open(QIODevice::WriteOnly) && file.write(download_) == download_.size() && file.commit();
            status_ = tr("%1 informes · actualizado el %2.").arg(reports_.size()).arg(fetched_.toLocalTime().toString("dd/MM/yyyy HH:mm"));
            if (!saved) status_ += tr(" No se pudo guardar la copia para usar sin conexión.");
        } else status_ = tr("No se pudo actualizar. Se conservan los %1 informes disponibles.").arg(reports_.size());
        download_.clear();
        reply->deleteLater();
        emit changed();
    });
}

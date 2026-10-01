#include "netplay_rooms.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {
constexpr qsizetype MaxResponse = 8 * 1024 * 1024;
constexpr int ServerTimeoutMs = 90 * 1000;
}
NetplayRooms::NetplayRooms(EngineSettings* settings, QObject* parent)
    : QObject(parent), settings_(settings) {
    status_ = tr("Pulsa Actualizar para consultar las sesiones anunciadas.");
    connect(settings_, &EngineSettings::changed, this, [this, previous = server()]() mutable {
        if (previous != server()) {
            cancel(); rooms_.clear(); previous = server();
            status_ = tr("Servidor cambiado. Actualiza las salas."); emit changed();
        }
    });
}
QString NetplayRooms::server() const { return serverOverride_.isEmpty() ? settings_->values().value("Live.api_address").toString().trimmed() : serverOverride_; }
void NetplayRooms::setServerOverride(const QString& endpoint) {
    if (serverOverride_ == endpoint) return;
    cancel(); serverOverride_ = endpoint; rooms_.clear();
    status_ = endpoint.isEmpty() ? tr("Servidor público seleccionado.") : tr("Servidor privado seleccionado. Actualiza las salas.");
    emit changed();
}
void NetplayRooms::cancel() {
    if (!reply_) return;
    auto old = reply_.data(); reply_.clear();
    disconnect(old, nullptr, this, nullptr); old->abort(); old->deleteLater();
    buffer_.clear(); rooms_.clear(); status_ = tr("Consulta cancelada."); emit changed();
}
void NetplayRooms::refresh() {
    if (busy()) return;
    rooms_.clear(); buffer_.clear();
    QUrl url(server());
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        (url.scheme() != "https" && url.scheme() != "http") || url.hasQuery() || url.hasFragment()) {
        status_ = tr("Configura una dirección HTTP o HTTPS válida para el servidor Netplay."); emit changed(); return;
    }
    // Browsing is read-only. Unrelated pending edits must not block an HTTP query.
    auto path = url.path(); if (!path.endsWith('/')) path += '/';
    url.setPath(path + "sessions");
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "Emulos360/0.2");
    request.setTransferTimeout(ServerTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    reply_ = network_.get(request);
    reply_->setReadBufferSize(64 * 1024);
    status_ = tr("Consultando el servidor… Puede tardar hasta 90 segundos."); emit changed();
    connect(reply_, &QNetworkReply::readyRead, this, [this] {
        buffer_ += reply_->readAll();
        if (buffer_.size() > MaxResponse) {
            cancel(); status_ = tr("La respuesta supera el límite de 8 MB."); emit changed();
        }
    });
    connect(reply_, &QNetworkReply::finished, this, [this] {
        auto reply = reply_.data(); reply_.clear();
        buffer_ += reply->readAll();
        QString error;
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::OperationCanceledError && httpStatus == 0) {
            status_ = tr("La consulta se interrumpió o agotó el tiempo de espera. Intenta actualizar de nuevo.");
        } else if (reply->error() != QNetworkReply::NoError && httpStatus == 0) {
            status_ = tr("No se pudo conectar con el servidor Netplay: %1").arg(reply->errorString());
        } else if (reply->error() != QNetworkReply::NoError || httpStatus != 200) {
            status_ = tr("No se pudieron consultar las salas (HTTP %1): %2").arg(httpStatus).arg(reply->errorString());
        } else if (!decode(buffer_, rooms_, error)) {
            status_ = error;
        } else {
            status_ = tr("%1 sesiones anunciadas · actualizado a las %2. El servidor puede agrupar varias sesiones del mismo anfitrión.")
                          .arg(rooms_.size()).arg(QTime::currentTime().toString("HH:mm:ss"));
        }
        buffer_.clear(); reply->deleteLater(); emit changed();
    });
}
bool NetplayRooms::decode(const QByteArray& json, QVariantList& rows, QString& error) {
    rows.clear();
    if (json.size() > MaxResponse) { error = tr("Respuesta demasiado grande."); return false; }
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject() || !doc.object().value("Titles").isArray()) {
        error = tr("El servidor no ofrece un listado de sesiones compatible."); return false;
    }
    const QRegularExpression titleIdPattern("^[0-9A-Fa-f]{8}$");
    QVariantList decoded;
    for (const auto& titleValue : doc.object()["Titles"].toArray()) {
        const auto title = titleValue.toObject();
        const auto id = title["titleId"].toString().toUpper();
        if (!titleIdPattern.match(id).hasMatch() || !title["sessions"].isArray()) {
            error = tr("El servidor devolvió un juego con datos inválidos."); return false;
        }
        for (const auto& sessionValue : title["sessions"].toArray()) {
            if (!sessionValue.isObject() || decoded.size() >= 1000) {
                error = tr("Listado de sesiones inválido o excesivo."); return false;
            }
            const auto session = sessionValue.toObject();
            const auto total = session["total"].toInt(-1);
            if (!session["players"].isArray() || total < 0 || total > 65535) {
                error = tr("Sesión con plazas o participantes inválidos."); return false;
            }
            decoded.append(QVariantMap{
                {"titleId", id}, {"title", title["name"].toString(id).left(160)},
                {"host", session["host_gamertag"].toString().left(64)},
                {"presence", session["host_presence"].toString().left(256)},
                {"players", session["players"].toArray().size()}, {"total", total},
                {"mediaId", session["mediaId"].toString().left(32)},
                {"version", session["version"].toString().left(32)}});
        }
    }
    rows = decoded; return true;
}

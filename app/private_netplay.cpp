#include "private_netplay.h"
#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

struct PrivateNetplay::ProcessGroup {
#ifdef Q_OS_WIN
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    ProcessGroup() {
        if (!job) return;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
            CloseHandle(job); job = nullptr;
        }
    }
    void assign(qint64 pid) const {
        if (!job) return;
        const auto handle = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
        if (handle) { AssignProcessToJobObject(job, handle); CloseHandle(handle); }
    }
    ~ProcessGroup() { if (job) CloseHandle(job); }
#else
    void assign(qint64) const {}
#endif
};

PrivateNetplay::PrivateNetplay(QString data, QObject* parent)
    : QObject(parent), data_(QDir(data).filePath("private-netplay")) {
    status_ = tr("Crea un servidor privado o entra con una invitación.");
    connect(&process_, &QProcess::started, this, [this] { processGroup_->assign(process_.processId()); });
    connect(&process_, &QProcess::readyReadStandardOutput, this, &PrivateNetplay::readOutput);
    connect(&process_, &QProcess::readyReadStandardError, this, [this] { process_.readAllStandardError(); });
    connect(&process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        starting_ = hosting_ = connected_ = false; endpoint_.clear(); adminURL_.clear(); metrics_.clear();
        processGroup_.reset();
        emit endpointChanged({});
        if (code == 0) status_ = tr("Desconectado. El servidor privado está detenido.");
        emit changed();
    });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            starting_ = false; status_ = tr("No se pudo iniciar el servicio privado: %1").arg(process_.errorString()); emit changed();
        }
    });
}
PrivateNetplay::~PrivateNetplay() {
    if (running()) {
        send({{"action", "stop"}}); process_.closeWriteChannel();
        if (!process_.waitForFinished(8000)) {
            process_.terminate();
            if (!process_.waitForFinished(2000)) { process_.kill(); process_.waitForFinished(2000); }
        }
    }
    disconnect(&process_, nullptr, this, nullptr);
    processGroup_.reset();
}
QString PrivateNetplay::localAddress() const {
    for (const auto& interface : QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsUp) || interface.flags() & QNetworkInterface::IsLoopBack) continue;
        for (const auto& entry : interface.addressEntries())
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) return entry.ip().toString();
    }
    return "127.0.0.1";
}
void PrivateNetplay::send(const QVariantMap& command) {
    if (running()) process_.write(QJsonDocument::fromVariant(command).toJson(QJsonDocument::Compact) + '\n');
}
void PrivateNetplay::launch(QVariantMap config) {
    if (running()) return;
    auto folder = qEnvironmentVariable("EMULOS_PRIVATE_SERVER_DIR");
    if (folder.isEmpty()) folder = QDir(QCoreApplication::applicationDirPath()).filePath("server");
#ifdef Q_OS_WIN
    const auto executable = QDir(folder).filePath("node.exe");
#else
    const auto executable = QDir(folder).filePath("node");
#endif
    const auto node = QFileInfo::exists(executable) ? executable : QStandardPaths::findExecutable("node");
    const auto script = QDir(folder).filePath("private-service.cjs");
    if (node.isEmpty() || !QFileInfo::exists(script)) {
        status_ = tr("Faltan las dependencias MultiP. Ejecuta el instalador incluido en esta versión."); emit changed(); return;
    }
    if (!QDir().mkpath(data_)) { status_ = tr("No se pudo crear la carpeta del servidor."); emit changed(); return; }
    config["data"] = data_; config["action"] = "start";
    output_.clear(); invitation_.clear(); metrics_.clear(); endpoint_.clear();
    starting_ = true; status_ = tr("Iniciando el servicio privado…");
    processGroup_ = std::make_unique<ProcessGroup>();
    process_.setWorkingDirectory(folder); process_.start(node, {script});
    send(config); emit changed();
}
void PrivateNetplay::host(const QString& name, int port, const QString& address, bool upnp) {
    launch({{"mode", "host"}, {"name", name}, {"port", port}, {"address", address}, {"upnp", upnp}});
}
void PrivateNetplay::join(const QString& invitation, const QString& name, const QString& address, int port) {
    launch({{"mode", "client"}, {"invitation", invitation}, {"name", name}, {"address", address}, {"port", port}});
}
void PrivateNetplay::stop() {
    if (!running()) return;
    status_ = tr("Cerrando el servicio y sus conexiones…"); send({{"action", "stop"}}); emit changed();
}
void PrivateNetplay::createInvitation(const QString& label, int hours) { send({{"action", "invite"}, {"label", label}, {"hours", hours}}); }
void PrivateNetplay::revoke(const QString& id) { send({{"action", "revoke"}, {"id", id}}); }
void PrivateNetplay::shareStats(bool enabled) { send({{"action", "permissions"}, {"shareStats", enabled}}); }
void PrivateNetplay::cleanup() { send({{"action", "cleanup"}}); }
void PrivateNetplay::refresh() { send({{"action", "refresh"}}); }
void PrivateNetplay::findPublicAddress() { send({{"action", "public-ip"}}); }
void PrivateNetplay::copyInvitation() { if (!invitation_.isEmpty()) QGuiApplication::clipboard()->setText(invitation_); }
void PrivateNetplay::openPanel() { if (!endpoint_.isEmpty()) QDesktopServices::openUrl(QUrl(hosting_ ? adminURL_ : endpoint_)); }
void PrivateNetplay::readOutput() {
    output_ += process_.readAllStandardOutput();
    if (output_.size() > 1024 * 1024) { output_.clear(); status_ = tr("El servicio devolvió datos excesivos."); stop(); return; }
    for (;;) {
        const auto end = output_.indexOf('\n'); if (end < 0) break;
        const auto line = output_.left(end); output_.remove(0, end + 1);
        const auto message = QJsonDocument::fromJson(line).object(); const auto event = message["event"].toString();
        if (event == "ready") {
            starting_ = false; hosting_ = message["hosting"].toBool(); connected_ = true;
            endpoint_ = message["endpoint"].toString(); adminURL_ = message["adminURL"].toString();
            status_ = hosting_ ? tr("Servidor privado activo. Crea una invitación para tus amigos.") : tr("Conectado al servidor privado. Abre el juego para crear o buscar una partida.");
            emit endpointChanged(message["bridge"].toString());
        } else if (event == "status") {
            metrics_ = message["status"].toObject().toVariantMap(); connected_ = true;
        } else if (event == "invitation") {
            invitation_ = message["invitation"].toString(); status_ = tr("Invitación creada. Cópiala y compártela con la persona invitada.");
        } else if (event == "error" || event == "disconnected") {
            status_ = message["message"].toString(); if (event == "disconnected") connected_ = false;
        } else if (event == "public-ip") {
            publicAddress_ = message["address"].toString();
            status_ = tr("IP pública detectada: %1. Para usarla en las invitaciones, reinicia el servidor con esa dirección.").arg(publicAddress_);
        }
        emit changed();
    }
}

// Emulos360 extension. No credentials, Account encryption or emulator code duplicated here.
#include "player_services.h"
#include "save_store.h"
#include "runtime_paths.h"
// Keep the parse_result API consistent with EngineSettings in every build.
#define TOML_EXCEPTIONS 0
#include "third_party/tomlplusplus/toml.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QLocale>
#include <QJsonDocument>
#include <QtEndian>
#include <QPainter>
#include <QRegularExpression>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QUuid>
#include <QtConcurrentRun>
#include <algorithm>

PlayerServices::PlayerServices(EngineSettings* engine,const QString& data,QObject* parent,const QString& coreExecutable)
    : QObject(parent),engine_(engine),data_(QDir(data).absolutePath()),coreExecutable_(coreExecutable),preferences_(data_ + "/players.ini",QSettings::IniFormat),avatarStudio_(data_,this) {
    connect(&avatarStudio_,&AvatarStudio::saveRequested,this,[this](const QString& xuid,const QByteArray& bytes) {
        if(busy() || !knownProfile(xuid)) { fail(tr("Cierra el juego antes de guardar el avatar.")); return; }
        QDir().mkpath(data_+"/ipc");
        avatarInput_=data_+"/ipc/"+QUuid::createUuid().toString(QUuid::Id128)+".avatar";
        QSaveFile file(avatarInput_);
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){fail(tr("No se pudo preparar el avatar para guardar."));return;}
        avatarProfile_=xuid;profileCommand("avatar-write",{},xuid);
    });
    knownContent_ = engine_->contentPath();
    process_.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_LINUX
    auto environment = QProcessEnvironment::systemEnvironment();
    // The native core currently creates an XCB Vulkan surface, even in Wayland sessions.
    environment.insert("GDK_BACKEND", "x11");
    process_.setProcessEnvironment(environment);
#endif
    connect(engine_,&EngineSettings::changed,this,[this] {
        if (achievementXuid_ != activeXuid() || achievementContent_ != engine_->contentPath()) {
            achievementGames_.clear(); achievementStatus_.clear();
            emit achievementsChanged();
        }
        if (knownContent_ != engine_->contentPath()) {
            knownContent_ = engine_->contentPath(); profiles_.clear(); saves_.clear();
        }
        emit changed();
    });
    connect(&process_,&QProcess::finished,this,&PlayerServices::finishProcess);
    connect(&process_,&QProcess::started,this,[this] {
        if (command_.isEmpty()) coreWindow_.watch(process_.processId());
        emit changed();
    });
    connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && command_ == "achievements") {
            achievementStatus_ = tr("No se pudo iniciar el motor para leer los logros.");
            emit achievementsChanged();
        }
        if (error == QProcess::FailedToStart) { timeout_.stop(); command_.clear(); release(); fail(tr("No se pudo iniciar el motor: %1").arg(process_.errorString())); }
    });
    timeout_.setSingleShot(true); timeout_.setInterval(60000);
    connect(&timeout_,&QTimer::timeout,this,[this] { process_.kill(); });
    connect(&fileJob_,&QFutureWatcher<QString>::finished,this,[this] {
        const auto error = fileJob_.result(); release(); refreshSaves();
        message(error.isEmpty() ? tr("Operación completada. Las copias anteriores se conservan.") : error);
    });
    status_ = tr("Crea o actualiza tus perfiles para empezar.");
}
QString PlayerServices::activeXuid() const { return engine_->values().value("Profiles.logged_profile_slot_0_xuid").toString().toUpper(); }
QString PlayerServices::communityXex() const { return preferences_.value("community/xex").toString(); }
QString PlayerServices::avatarEditorXex() const { return preferences_.value("avatar/editorXex").toString(); }
void PlayerServices::message(const QString& text) { status_ = text; emit changed(); }
bool PlayerServices::fail(const QString& text) { message(text); emit operationFailed(text); return false; }
PlayerServices::~PlayerServices() {
    // QProcess can emit finished while its destructor stops a running child.
    // Disconnect before CoreWindowHost and the other members are destroyed.
    disconnect(&process_, nullptr, this, nullptr);
    disconnect(&fileJob_, nullptr, this, nullptr);
    timeout_.stop();
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(5000);
    }
    fileJob_.waitForFinished();
    release();
}
QString PlayerServices::corePath() const {
    if (!coreExecutable_.isEmpty()) return coreExecutable_;
    const auto folder = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const auto name = QStringLiteral("Emulos360-core.exe");
#else
    const auto name = QStringLiteral("Emulos360-core");
#endif
    return QDir(folder).filePath("engine/" + name);
}
QStringList PlayerServices::arguments() const {
    const auto config = privateApi_.isEmpty() ? engine_->configPath() : data_ + "/private-netplay/core.config.toml";
    QStringList args{"--config=" + config,"--storage_root=" + engine_->storagePath(),"--content_root=" + engine_->contentPath(),
            "--emulos_external_content_root=" + engine_->externalContentPath(), "--storage_selection_dialog=true", "--auto_check_updates=false",
            "--log_file=" + data_ + (command_.isEmpty() ? "/core.log" : "/profile-helper.log")};
#ifdef Q_OS_LINUX
    args << "--gpu=" + engine_->values().value("GPU.gpu").toString()
         << "--apu=" + engine_->values().value("APU.apu").toString()
         << "--hid=" + engine_->values().value("HID.hid").toString();
#endif
    if (!privateApi_.isEmpty()) args << "--api_address=" + privateApi_ << "--api_list=" + privateApi_ + "," << "--network_mode=2";
    return args;
}
bool PlayerServices::prepare(bool requireProfile) {
    if (busy()) { message(tr("Espera a que termine la operación o cierra la ventana del motor.")); return false; }
#ifndef Q_OS_WIN
    if (!emulos::runnableFile(corePath())) return fail(tr("El motor Linux no tiene permiso de ejecución o falta en el paquete. Ejecuta bash install-dependencies.sh desde la carpeta de Emulos360. Si la unidad está montada con noexec, copia el programa a tu carpeta personal."));
#endif
    if (engine_->dirty()) return fail(tr("Hay ajustes pendientes. En Ajustes, pulsa Guardar o Recargar antes de jugar. %1").arg(engine_->status()));
    if (requireProfile && !QRegularExpression("^[A-Fa-f0-9]{16}$").match(activeXuid()).hasMatch()) return fail(tr("Selecciona un perfil en Perfiles y avatares. Si todavía no tienes uno, créalo allí para guardar tus partidas."));
    if (requireProfile && !knownProfile(activeXuid())) return fail(tr("El perfil seleccionado no está disponible. Actualiza y selecciona un perfil en Perfiles y avatares."));
    QDir().mkpath(engine_->contentPath());
    // Only provision the built-in virtual disk. Never recreate a disconnected USB path.
    if (engine_->values().value("Storage.emulos_external_content_root").toString().isEmpty()) QDir().mkpath(engine_->externalContentPath());
    const auto internal = QFileInfo(engine_->contentPath()).canonicalFilePath();
    const auto external = QFileInfo(engine_->externalContentPath()).canonicalFilePath();
    if (!external.isEmpty() && (external.compare(internal,Qt::CaseInsensitive)==0 || external.startsWith(internal+'/',Qt::CaseInsensitive) || internal.startsWith(external+'/',Qt::CaseInsensitive))) {
        message(tr("Las carpetas interna y externa deben ser independientes, sin estar una dentro de la otra.")); return false;
    }
    lock_ = std::make_unique<QLockFile>(engine_->contentPath() + "/.emulos-session.lock");
    if (!lock_->tryLock(0)) { message(tr("Otra instancia de Emulos360 está usando esta carpeta de partidas.")); lock_.reset(); return false; }
    if (!external.isEmpty()) {
        externalLock_ = std::make_unique<QLockFile>(external + "/.emulos-session.lock");
        if (!externalLock_->tryLock(0)) { release(); message(tr("El disco externo está en uso o no permite escritura.")); return false; }
    }
    if (!QFileInfo::exists(engine_->configPath()) && !engine_->save()) { release(); message(engine_->status()); return false; }
    if (!privateApi_.isEmpty()) {
        QDir().mkpath(data_ + "/private-netplay");
        QFile source(engine_->configPath()); QSaveFile session(data_ + "/private-netplay/core.config.toml");
        if (!source.open(QIODevice::ReadOnly) || !session.open(QIODevice::WriteOnly)) {
            release(); return fail(tr("No se pudo preparar la configuración de la partida privada."));
        }
        const auto bytes = source.readAll();
        if (session.write(bytes) != bytes.size() || !session.commit()) {
            release(); return fail(tr("No se pudo guardar la configuración temporal de la partida privada."));
        }
    }
    engine_->setLocked(true); return true;
}
void PlayerServices::release() { externalLock_.reset(); lock_.reset(); engine_->setLocked(false); }
void PlayerServices::profileCommand(const QString& command,const QString& name,const QString& xuid) {
    if (!QFileInfo::exists(corePath())) { message(tr("Falta engine/Emulos360-core. Genera el paquete completo.")); return; }
    if (!prepare()) return;
    QDir().mkpath(data_ + "/ipc");
    output_ = data_ + "/ipc/" + QUuid::createUuid().toString(QUuid::Id128) + ".toml";
    command_ = command;
    auto args = arguments();
    args << "--network_mode=0" << "--upnp=false" << "--discord=false";
    // Listing must not sign profiles in or write their GPDs as a side effect.
    for (int slot=0;slot<4;++slot) args << QString("--logged_profile_slot_%1_xuid=").arg(slot);
    args << "--emulos_profile_command=" + command << "--emulos_profile_output=" + output_;
    if(command=="avatar-write") args << "--emulos_avatar_manifest_file="+avatarInput_;
    if (!name.isEmpty()) args << "--emulos_profile_name=" + name;
    if (!xuid.isEmpty()) args << "--emulos_profile_xuid=" + xuid;
    process_.setWorkingDirectory(QFileInfo(corePath()).absolutePath());
    process_.setStandardOutputFile(data_ + "/profile-helper-console.log");
    process_.start(corePath(),args); timeout_.start(); message(tr("Leyendo perfiles del motor…"));
}
void PlayerServices::refreshProfiles() { profileCommand("list"); }
void PlayerServices::refreshAchievements() {
    if (busy()) return;
    achievementGames_.clear();
    achievementXuid_ = activeXuid(); achievementContent_ = engine_->contentPath();
    if (achievementXuid_.isEmpty()) {
        achievementStatus_ = tr("Selecciona un perfil en Perfiles y avatares para consultar sus logros.");
    } else {
        profileCommand("achievements", {}, achievementXuid_);
        achievementStatus_ = command_ == "achievements" ? tr("Leyendo tus logros…") : status_;
    }
    emit achievementsChanged();
}

void PlayerServices::finishAchievements(int code, QProcess::ExitStatus exit) {
    QFile file(output_);
    const auto finish = [this](const QString& text) {
        achievementStatus_ = text;
        emit achievementsChanged(); message(text);
    };
    if (!engine_->reload()) { finish(engine_->status()); return; }
    if (exit != QProcess::NormalExit || code != 0 || !file.open(QIODevice::ReadOnly) || file.size() > 64*1024*1024) {
        file.close(); QFile::remove(output_);
        finish(tr("No se pudieron leer los logros. Pulsa Actualizar para reintentar.")); return;
    }
    const auto parsed = toml::parse(file.readAll().toStdString());
    file.close(); QFile::remove(output_);
    if (!parsed || parsed["version"].value_or<int64_t>(0) != 1) {
        finish(tr("La respuesta de logros del motor no es válida.")); return;
    }
    if (!parsed["ok"].value_or(false)) {
        finish(QString::fromStdString(parsed["error"].value_or(std::string{"No se pudieron leer los logros."}))); return;
    }
    if (achievementXuid_ != activeXuid() || achievementContent_ != engine_->contentPath() ||
        QString::fromStdString(parsed["achievementXuid"].value_or(std::string{})) != achievementXuid_) {
        finish(tr("El perfil ha cambiado. Actualiza los logros.")); return;
    }
    const auto* titles = parsed["achievementTitles"].as_array();
    if (!titles) { finish(tr("Actualiza el motor para consultar los logros.")); return; }
    // This request may win the startup race with refreshProfiles. Its response
    // includes the same profile roster, so the visible identity stays available.
    if (const auto* profiles = parsed["profiles"].as_array()) {
        profiles_.clear();
        for (const auto& item : *profiles) if (const auto* profile = item.as_table()) {
            const auto xuid = QString::fromStdString((*profile)["xuid"].value_or(std::string{}));
            if (QRegularExpression("^[0-9A-F]{16}$").match(xuid).hasMatch())
                profiles_.append(QVariantMap{{"xuid", xuid},
                    {"gamertag", QString::fromStdString((*profile)["gamertag"].value_or(std::string{}))},
                    {"netplay", (*profile)["netplay"].value_or(false)}});
        }
        decorateProfiles();
    }
    const auto text = [](toml::node_view<const toml::node> node) { return QString::fromStdString(node.value_or(std::string{})); };
    for (const auto& rowNode : *titles) {
        const auto* table = rowNode.as_table();
        if (!table) continue;
        const auto& row = *table;
        QVariantList achievements;
        if (const auto* entries = row["achievements"].as_array()) for (const auto& entryNode : *entries) {
            const auto* entryTable = entryNode.as_table();
            if (!entryTable) continue;
            const auto& entry = *entryTable;
            const auto at = entry["unlockedAt"].value_or<int64_t>(0);
            const auto icon = QByteArray::fromHex(text(entry["icon"]).toLatin1());
            achievements.append(QVariantMap{
                {"name", text(entry["name"])}, {"description", text(entry["description"])},
                {"points", static_cast<qlonglong>(entry["points"].value_or<int64_t>(0))},
                {"unlocked", entry["unlocked"].value_or(false)}, {"secret", entry["secret"].value_or(false)},
                {"date", at > 0 ? QLocale().toString(QDateTime::fromSecsSinceEpoch(at), QLocale::ShortFormat) : QString{}},
                {"icon", icon.isEmpty() ? QString{} : "data:image/png;base64," + QString::fromLatin1(icon.toBase64())}});
        }
        achievementGames_.append(QVariantMap{
            {"titleId", text(row["titleId"])}, {"title", text(row["title"])},
            {"total", static_cast<qlonglong>(row["total"].value_or<int64_t>(0))},
            {"unlocked", static_cast<qlonglong>(row["unlocked"].value_or<int64_t>(0))},
            {"points", static_cast<qlonglong>(row["points"].value_or<int64_t>(0))},
            {"totalPoints", static_cast<qlonglong>(row["totalPoints"].value_or<int64_t>(0))},
            {"detailsAvailable", row["detailsAvailable"].value_or(false)}, {"achievements", achievements}});
    }
    std::sort(achievementGames_.begin(), achievementGames_.end(), [](const QVariant& a, const QVariant& b) {
        return QString::localeAwareCompare(a.toMap()["title"].toString(), b.toMap()["title"].toString()) < 0;
    });
    finish(achievementGames_.isEmpty() ? tr("Este perfil todavía no tiene juegos con logros registrados.")
                                      : tr("Logros de %1 juegos actualizados.").arg(achievementGames_.size()));
}
void PlayerServices::editAvatar(const QString& xuid) {
    if(!knownProfile(xuid)){fail(tr("Selecciona un perfil existente para editar su avatar."));return;}
    avatarProfile_=xuid;profileCommand("avatar-read",{},xuid);
}
void PlayerServices::enableNetplayProfile(const QString& xuid) {
    if (!knownProfile(xuid)) { fail(tr("Actualiza los perfiles y selecciona uno existente.")); return; }
    profileCommand("enable-netplay",{},xuid);
}
void PlayerServices::createProfile(const QString& name) {
    if (name.size() > 15 || !QRegularExpression("^[A-Za-z][A-Za-z0-9]*( [A-Za-z0-9]+)*$").match(name).hasMatch()) { message(tr("Usa de 1 a 15 letras, números y espacios; empieza por una letra.")); return; }
    profileCommand("create",name);
}
void PlayerServices::finishProcess(int code,QProcess::ExitStatus exit) {
    coreWindow_.clear();
    timeout_.stop(); release();
    if (command_.isEmpty()) {
        const bool loaded = engine_->reload(); refreshSaves();
        if (!loaded) fail(engine_->status());
        else if (exit != QProcess::NormalExit || code != 0) {
#ifndef Q_OS_WIN
            if (exit == QProcess::CrashExit && code == 9)
                fail(tr("El sistema finalizó el motor con SIGKILL (9). Puede ocurrir al forzar su cierre o por falta de memoria. Registros: %1/core.log y %1/core-console.log").arg(data_));
            else
#endif
                fail(tr("El motor terminó con un error (%1). Registros: %2/core.log y %2/core-console.log").arg(code).arg(data_));
        }
        else message(tr("Sesión terminada. Las partidas permanecen en la carpeta de contenido."));
        emit sessionEnded(exit == QProcess::NormalExit ? code : -1);
        return;
    }
    const auto completedCommand = command_;
    command_.clear();
    if (completedCommand == "achievements") { finishAchievements(code, exit); return; }
    // Setup may create/update xconfig.settings even for a read-only profile list.
    // prepare() rejected unsaved edits and locked settings for the whole process,
    // so refreshing this snapshot cannot discard an in-progress UI edit.
    if (!engine_->reload()) { fail(engine_->status()); return; }
    QFile file(output_);
    if (exit != QProcess::NormalExit || code != 0 || !file.open(QIODevice::ReadOnly)) { message(tr("El motor no completó la operación de perfiles. Revisa el registro del motor.")); return; }
    auto parsed = toml::parse(file.readAll().toStdString()); file.close(); QFile::remove(output_);
    if (!parsed || parsed["version"].value_or<int64_t>(0) != 1) { message(tr("Respuesta de perfiles incompatible.")); return; }
    if(completedCommand=="avatar-write") { QFile::remove(avatarInput_); avatarInput_.clear(); }
    if((completedCommand=="avatar-read"||completedCommand=="avatar-write") && parsed["ok"].value_or(false)) {
        const auto bytes=QByteArray::fromHex(QByteArray::fromStdString(parsed["avatar_manifest"].value_or(std::string{})));
        if(completedCommand=="avatar-read") avatarStudio_.openProfile(avatarProfile_,bytes);
        else avatarStudio_.saved();
    }
    profiles_.clear();
    if (const auto* profiles = parsed["profiles"].as_array()) {
        for (const auto& row : *profiles) {
            const auto* table = row.as_table();
            if (!table) continue;
            const auto xuid = QString::fromStdString((*table)["xuid"].value_or(std::string{}));
            if (QRegularExpression("^[0-9A-F]{16}$").match(xuid).hasMatch()) profiles_.append(QVariantMap{{"xuid",xuid},{"gamertag",QString::fromStdString((*table)["gamertag"].value_or(std::string{}))},{"netplay",(*table)["netplay"].value_or(false)}});
        }
    }
    decorateProfiles(); refreshSaves();
    // Restore the first-run flow, including profiles created by older builds.
    // Never replace an explicitly selected profile or choose between several.
    if (parsed["ok"].value_or(false) && activeXuid().isEmpty() && profiles_.size() == 1) {
        if (!selectProfile(profiles_.first().toMap()["xuid"].toString())) return;
    }
    // Selecting Netplay mode expresses the intent to use its community identity.
    // The bridge backs up the existing profile before enabling it, once only.
    if (parsed["ok"].value_or(false) && completedCommand != "enable-netplay" &&
        engine_->values().value("Live.network_mode").toInt() == 2) {
        for (const auto& item : profiles_) {
            const auto profile = item.toMap();
            if (profile["xuid"].toString() == activeXuid() && !profile["netplay"].toBool()) {
                enableNetplayProfile(activeXuid()); return;
            }
        }
    }
    if (!parsed["ok"].value_or(false)) { fail(QString::fromStdString(parsed["error"].value_or(std::string{"Error del motor"}))); return; }
    message(completedCommand == "enable-netplay" ? tr("Perfil habilitado para Netplay. Se conservan el perfil anterior y tus partidas.") :
        activeXuid().isEmpty() ? tr("%1 perfiles disponibles. Selecciona uno para jugar.").arg(profiles_.size()) : tr("Perfil guardado. Listo para jugar."));
}
bool PlayerServices::knownProfile(const QString& xuid) const {
    for (const auto& p : profiles_) if (p.toMap()["xuid"].toString() == xuid) return true;
    return false;
}
bool PlayerServices::selectProfile(const QString& xuid) {
    if (busy() || !knownProfile(xuid)) return false;
    if (engine_->dirty()) { message(tr("Guarda o descarta los ajustes antes de seleccionar un perfil.")); return false; }
    // The same profile cannot occupy two local player slots.
    for (int slot=1;slot<4;++slot) {
        const auto key = QString("Profiles.logged_profile_slot_%1_xuid").arg(slot);
        if (engine_->values().value(key).toString().compare(xuid,Qt::CaseInsensitive)==0) engine_->setValue(key,QString{});
    }
    const bool ok = engine_->setValue("Profiles.logged_profile_slot_0_xuid",xuid) && engine_->save();
    if (ok) message(tr("Perfil elegido para el jugador 1. Se iniciará al abrir el juego."));
    else fail(engine_->status());
    return ok;
}
void PlayerServices::decorateProfiles() {
    for (auto& item : profiles_) { auto profile=item.toMap(); profile["avatar"] = preferences_.value("avatars/"+profile["xuid"].toString()); item=profile; }
    emit changed();
}
void PlayerServices::saveAvatar(const QString& xuid,const QImage& image) {
    if (image.isNull() || !knownProfile(xuid)) return;
    const auto name = xuid + '-' + QUuid::createUuid().toString(QUuid::Id128) + ".png";
    QDir().mkpath(data_ + "/avatars"); QSaveFile file(data_ + "/avatars/" + name);
    if (!file.open(QIODevice::WriteOnly) || !image.scaled(256,256,Qt::IgnoreAspectRatio,Qt::SmoothTransformation).save(&file,"PNG") || !file.commit()) { message(tr("No se pudo guardar el avatar.")); return; }
    preferences_.setValue("avatars/"+xuid,QUrl::fromLocalFile(file.fileName()).toString()); preferences_.sync();
    decorateProfiles(); message(tr("Avatar de Emulos360 guardado."));
}
void PlayerServices::createAvatar(const QString& xuid,const QString& skin,const QString& shirt,int style) {
    if (!knownProfile(xuid) || !QColor(skin).isValid() || !QColor(shirt).isValid()) return;
    QImage image(256,256,QImage::Format_ARGB32_Premultiplied); image.fill(QColor("#202a25"));
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen);
    p.setBrush(QColor(shirt)); p.drawEllipse(QRectF(35,168,186,140));
    p.setBrush(QColor(skin)); p.drawRoundedRect(QRectF(108,147,40,51),16,16); p.drawEllipse(QRectF(68,43,120,132));
    p.setBrush(QColor("#252020"));
    if (style == 0) p.drawChord(QRectF(63,29,130,100),0,180*16);
    if (style == 1) { p.setBrush(QColor(shirt).lighter(125)); p.drawRoundedRect(QRectF(62,32,132,44),18,18); p.drawRoundedRect(QRectF(120,61,91,13),6,6); }
    p.setBrush(QColor("#252020")); p.drawEllipse(QRectF(95,102,10,13)); p.drawEllipse(QRectF(152,102,10,13));
    p.setPen(QPen(QColor("#753d38"),5,Qt::SolidLine,Qt::RoundCap)); p.drawArc(QRectF(109,119,40,25),200*16,140*16);
    p.end(); saveAvatar(xuid,image);
}
void PlayerServices::importAvatar(const QString& xuid,const QUrl& url) {
    if (!url.isLocalFile() || !knownProfile(xuid)) return;
    QImageReader reader(url.toLocalFile()); reader.setAutoTransform(true);
    const auto size=reader.size();
    if (!size.isValid() || static_cast<qint64>(size.width())*size.height()>32*1024*1024) { message(tr("Imagen demasiado grande o incompatible.")); return; }
    const auto image=reader.read(); if (image.isNull()) message(tr("No se pudo abrir la imagen.")); else saveAvatar(xuid,image);
}
void PlayerServices::setCommunityXex(const QUrl& file) { if (file.isLocalFile() && QFileInfo(file.toLocalFile()).suffix().compare("xex",Qt::CaseInsensitive)==0) { preferences_.setValue("community/xex",file.toLocalFile()); emit changed(); } }
void PlayerServices::setAvatarEditorXex(const QUrl& file) { if (file.isLocalFile() && QFileInfo(file.toLocalFile()).suffix().compare("xex",Qt::CaseInsensitive)==0) { preferences_.setValue("avatar/editorXex",file.toLocalFile()); emit changed(); } }
void PlayerServices::launchCommunity() { launch(communityXex(),true); }
void PlayerServices::launchAvatarEditor() { launch(avatarEditorXex(),true); }
void PlayerServices::launchGame(const QString& path, const QString& titleId) { launch(path,false,titleId); }
void PlayerServices::launchMetro(const QString& path) { launch(path,true,"454D3601",true); }
void PlayerServices::launchNetplay() {
    if (!QFileInfo::exists(corePath())) { message(tr("Falta el motor Netplay en el paquete.")); return; }
    if (!prepare()) return;
    command_.clear();
    process_.setWorkingDirectory(QFileInfo(corePath()).absolutePath());
    auto args = arguments();
#ifdef Q_OS_WIN
    args << "--emulos_embedded=true" << "--fullscreen=false";
#endif
    process_.start(corePath(),args);
    message(tr("Gestor Netplay abierto dentro de Emulos360. Usa Netplay → Settings / Manager y Profiles para configurar el perfil de red."));
}
QString PlayerServices::saveContentPath() const { return saveDevice_ == 1 ? engine_->externalContentPath() : engine_->contentPath(); }
QString PlayerServices::backupRoot() const { return data_ + (saveDevice_ == 1 ? "/save-backups/external" : "/save-backups"); }
void PlayerServices::setSaveDevice(int value) {
    if (busy() || value < 0 || value > 1 || value == saveDevice_) return;
    saveDevice_ = value; refreshSaves();
}
void PlayerServices::chooseExternalStorage(const QUrl& folder) {
    if (busy() || !folder.isLocalFile() || engine_->dirty()) { message(tr("Guarda primero los ajustes pendientes.")); return; }
    const auto path = QFileInfo(folder.toLocalFile()).canonicalFilePath();
    const auto internal = QFileInfo(engine_->contentPath()).canonicalFilePath();
    if (path.isEmpty() || internal.isEmpty() || path.compare(internal,Qt::CaseInsensitive)==0 || path.startsWith(internal+'/',Qt::CaseInsensitive) || internal.startsWith(path+'/',Qt::CaseInsensitive)) {
        message(tr("Elige una carpeta existente e independiente del disco interno.")); return;
    }
    if (engine_->setValue("Storage.emulos_external_content_root",path) && engine_->save()) {
        refreshSaves(); message(tr("Disco externo configurado. Las partidas anteriores no se mueven ni se borran."));
    } else message(engine_->status());
}
void PlayerServices::launch(const QString& path,bool xexOnly,const QString& titleId,bool writableGame) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || !QFileInfo::exists(corePath())) { message(tr("No se encuentra el ejecutable o el motor.")); return; }
    const auto header=file.read(8); const auto magic=header.first(4); file.close();
    if (magic=="XEX2" && header.size()==8 && (qFromBigEndian<quint32>(header.constData()+4)&8)) {
        message(tr("Este XEX es un módulo DLL, no una aplicación independiente. Los módulos DLL necesitan un cargador compatible.")); return;
    }
    if ((xexOnly && !magic.startsWith("XEX")) || (!xexOnly && !magic.startsWith("XEX") && magic!="LIVE" && magic!="PIRS" && magic!="CON ")) { message(tr("Selecciona un ejecutable XEX o un paquete Xbox 360 compatible.")); return; }
    if (!prepare(!writableGame)) return;
    command_.clear(); auto args=arguments(); args << "--target=" + QFileInfo(path).absoluteFilePath();
    // The dashboard XEX runs from a disposable IPC directory. Other titles remain read-only.
    if(writableGame) args << "--allow_game_relative_writes=true";
    // Doritos uses the audited ABI4, left-handed avatar layout. Other titles
    // keep their existing behavior until their guest contract is verified.
    if (titleId.compare("58410A71",Qt::CaseInsensitive)==0 &&
        QFileInfo::exists(avatarStudio_.resourceRoot()+"/AvatarAssetPack.toc") &&
        QFileInfo::exists(avatarStudio_.resourceRoot()+"/avatar-skeleton.bin")) {
        args << "--emulos_avatar_guest_assets=true"
             << "--emulos_avatar_resource_root="+avatarStudio_.resourceRoot();
    }
#ifdef Q_OS_WIN
    args << "--emulos_embedded=true" << "--fullscreen=false";
#else
    if (consoleMode_) args << "--fullscreen=true";
#endif
    process_.setWorkingDirectory(QFileInfo(corePath()).absolutePath());
    process_.setStandardOutputFile(data_ + "/core-console.log");
    process_.start(corePath(),args);
    message(xexOnly ? tr("XEX iniciado. El estado de autenticación lo determina el servicio dentro del motor.") : tr("Sesión en curso. Guarda desde el menú del juego antes de cerrar el motor."));
}
void PlayerServices::refreshSaves() { if (busy()) return; saves_=emulos::savedGames(saveContentPath()); backups_=emulos::saveBackups(backupRoot()); emit changed(); }
void PlayerServices::backupSave(int index) {
    if (!QFileInfo(saveContentPath()).isDir()) { message(tr("La unidad seleccionada no está conectada.")); return; }
    if (index<0 || index>=saves_.size() || !prepare()) return;
    const auto save=saves_[index].toMap(); const auto content=saveContentPath(), root=backupRoot();
    fileJob_.setFuture(QtConcurrent::run([content,root,save] { return emulos::backupSave(content,root,save["xuid"].toString(),save["titleId"].toString()); })); message(tr("Creando copia de la partida…"));
}
void PlayerServices::restoreSave(int index) {
    if (!QFileInfo(saveContentPath()).isDir()) { message(tr("La unidad seleccionada no está conectada.")); return; }
    if (index<0 || index>=backups_.size() || !prepare()) return;
    const auto id=backups_[index].toMap()["id"].toString(); const auto content=saveContentPath(),root=backupRoot();
    fileJob_.setFuture(QtConcurrent::run([content,root,id] { return emulos::restoreSave(content,root,id); })); message(tr("Restaurando la copia y conservando la partida anterior…"));
}
void PlayerServices::openSaves() { if (QFileInfo::exists(saveContentPath())) QDesktopServices::openUrl(QUrl::fromLocalFile(saveContentPath())); }
void PlayerServices::openBackups() { QDir().mkpath(backupRoot()); QDesktopServices::openUrl(QUrl::fromLocalFile(backupRoot())); }

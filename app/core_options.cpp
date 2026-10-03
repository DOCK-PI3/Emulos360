#include "core_options.h"
#include "runtime_paths.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrl>

namespace {
bool validTitle(const QString& id) { return QRegularExpression("^[0-9A-Fa-f]{8}$").match(id).hasMatch(); }
bool validCore(const QString& id) { return id == "netplay" || id == "edge"; }
bool writeBytes(const QString& path, const QByteArray& bytes) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
bool safeDirectory(const QString& path) {
    auto info = QFileInfo(path);
    for (;;) {
        if (info.isSymbolicLink() || info.isJunction()) return false;
        const auto parent = info.absoluteDir().absolutePath();
        if (parent == info.absoluteFilePath()) break;
        info = QFileInfo(parent);
    }
    return true;
}
QString copyMissing(const QString& source, const QString& destination) {
    if (!QFileInfo::exists(source)) return {};
    if (!safeDirectory(source) || !safeDirectory(destination))
        return QObject::tr("No se copiará contenido a través de enlaces de carpetas: %1").arg(source);
    QDirIterator files(source, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        files.next(); const auto info = files.fileInfo();
        if (info.isSymbolicLink() || info.isJunction())
            return QObject::tr("El contenido incluye un enlace: %1").arg(info.filePath());
        const auto target = QDir(destination).filePath(QDir(source).relativeFilePath(info.filePath()));
        if (!safeDirectory(target)) return QObject::tr("La carpeta de Edge contiene un enlace: %1").arg(target);
        if (info.isDir()) {
            if (!QDir().mkpath(target)) return QObject::tr("No se pudo crear %1").arg(target);
            continue;
        }
        if (!info.isFile()) return QObject::tr("Contenido no compatible: %1").arg(info.filePath());
        if (QFileInfo::exists(target)) continue;
        if (!QDir().mkpath(QFileInfo(target).absolutePath())) return QObject::tr("No se pudo crear la carpeta de Edge.");
        QFile input(info.filePath()); QSaveFile output(target);
        if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
            return QObject::tr("No se pudo copiar %1").arg(info.filePath());
        while (!input.atEnd()) {
            const auto block = input.read(1024 * 1024);
            if (block.isEmpty() || output.write(block) != block.size())
                return QObject::tr("No se pudo completar la copia. Comprueba el espacio libre.");
        }
        if (input.error() != QFile::NoError || !output.commit())
            return QObject::tr("No se pudo guardar la copia de Edge.");
    }
    return {};
}
}

CoreOptions::CoreOptions(const QString& data, QObject* parent, const QString& applicationRoot)
    : QObject(parent), data_(QDir(data).absolutePath()),
      applicationRoot_(applicationRoot.isEmpty() ? QCoreApplication::applicationDirPath() : applicationRoot),
      preferences_(data_ + "/cores.ini", QSettings::IniFormat) {}
QString CoreOptions::executable(const QString& id) const {
    if (!validCore(id)) return {};
#ifdef Q_OS_WIN
    return QDir(applicationRoot_).filePath(id == "edge" ? "engine/edge/xenia_edge.exe" : "engine/Emulos360-core.exe");
#else
    return QDir(applicationRoot_).filePath(id == "edge" ? "engine/edge/xenia_edge_linux.AppImage" : "engine/Emulos360-core");
#endif
}
QVariantList CoreOptions::choices() const {
    return {QVariantMap{{"value", "netplay"}, {"label", tr("Emulos360 · core actual")},
                        {"available", emulos::runnableFile(executable("netplay"))}},
            QVariantMap{{"value", "edge"}, {"label", tr("Canary Edge · experimental")},
                        {"available", emulos::runnableFile(executable("edge"))}}};
}
QVariantList CoreOptions::rendererChoices() const {
    QVariantList result{QVariantMap{{"value", ""}, {"label", tr("Predeterminado del core")}}};
#ifdef Q_OS_WIN
    result.append(QVariantMap{{"value", "d3d12"}, {"label", "Direct3D 12"}});
#endif
    result.append(QVariantMap{{"value", "vulkan"}, {"label", "Vulkan"}});
    return result;
}
bool CoreOptions::validRenderer(const QString& value) const {
    for (const auto& row : rendererChoices()) if (row.toMap()["value"].toString() == value) return true;
    return false;
}
QString CoreOptions::defaultCore() const {
    const auto id = preferences_.value("defaultCore", "netplay").toString();
    return validCore(id) ? id : QStringLiteral("netplay");
}
QString CoreOptions::defaultRenderer() const {
    const auto id = preferences_.value("renderers/" + defaultCore()).toString();
    return validRenderer(id) ? id : QString{};
}
bool CoreOptions::fail(const QString& text) { status_ = text; emit changed(); return false; }
bool CoreOptions::persist() {
    preferences_.sync();
    if (preferences_.status() != QSettings::NoError) return fail(tr("No se pudieron guardar las preferencias de los cores."));
    status_ = tr("Preferencias guardadas. Se aplicarán al próximo inicio del juego."); emit changed(); return true;
}
void CoreOptions::setLocked(bool value) { if (locked_ != value) { locked_ = value; emit changed(); } }
void CoreOptions::setDefaultCore(const QString& id) {
    if (locked_ || !validCore(id) || id == defaultCore()) return;
    preferences_.setValue("defaultCore", id); persist();
}
void CoreOptions::setDefaultRenderer(const QString& id) {
    if (locked_ || !validRenderer(id) || id == defaultRenderer()) return;
    preferences_.setValue("renderers/" + defaultCore(), id); persist();
}
QVariantMap CoreOptions::gameOptions(const QString& titleId) const {
    const auto group = "games/" + titleId.toUpper() + '/';
    const auto core = validTitle(titleId) ? preferences_.value(group + "core").toString() : QString{};
    const auto renderer = validTitle(titleId) ? preferences_.value(group + "renderer").toString() : QString{};
    const auto effectiveCore = validCore(core) ? core : defaultCore();
    const auto globalRenderer = preferences_.value("renderers/" + effectiveCore).toString();
    return {{"core", validCore(core) ? core : QString{}},
            {"renderer", validRenderer(renderer) ? renderer : QString{}},
            {"effectiveCore", effectiveCore},
            {"effectiveRenderer", !renderer.isEmpty() && validRenderer(renderer) ? renderer :
                                  validRenderer(globalRenderer) ? globalRenderer : QString{}},
            {"available", emulos::runnableFile(executable(effectiveCore))}};
}
QString CoreOptions::coreFor(const QString& titleId) const { return gameOptions(titleId)["effectiveCore"].toString(); }
QString CoreOptions::rendererFor(const QString& titleId) const { return gameOptions(titleId)["effectiveRenderer"].toString(); }
bool CoreOptions::saveGameOptions(const QString& titleId, const QString& core, const QString& renderer) {
    if (locked_) return fail(tr("Cierra el juego antes de cambiar su core."));
    if (!validTitle(titleId) || (!core.isEmpty() && !validCore(core)) || !validRenderer(renderer))
        return fail(tr("El título, core o motor gráfico no es válido."));
    const auto group = "games/" + titleId.toUpper() + '/';
    if (core.isEmpty()) preferences_.remove(group + "core"); else preferences_.setValue(group + "core", core);
    if (renderer.isEmpty()) preferences_.remove(group + "renderer"); else preferences_.setValue(group + "renderer", renderer);
    return persist();
}
QString CoreOptions::edgeStorage() const { return data_ + "/cores/edge"; }
QString CoreOptions::edgeContent() const { return edgeStorage() + "/content"; }
QString CoreOptions::edgeConfig() const { return edgeStorage() + "/xenia-edge.config.toml"; }
void CoreOptions::openEdgeFolder() { QDir().mkpath(edgeStorage()); QDesktopServices::openUrl(QUrl::fromLocalFile(edgeStorage())); }
void CoreOptions::openEdgeContent() { QDir().mkpath(edgeContent()); QDesktopServices::openUrl(QUrl::fromLocalFile(edgeContent())); }
bool CoreOptions::ensureEdgeConfig(int language, bool notifications) {
    if (QFileInfo::exists(edgeConfig())) return true;
    const auto bytes = QByteArray("[General]\ndiscord = false\n\n[Console]\nuser_language = ") +
        QByteArray::number(language >= 1 && language <= 17 ? language : 1) + "\n\n[UI]\nshow_achievement_notification = " +
        (notifications ? "true\n" : "false\n");
    return writeBytes(edgeConfig(), bytes) || fail(tr("No se pudo crear la configuración independiente de Edge."));
}

QString emulos::seedEdgeContent(const QString& internal, const QString& external,
                               const QString& destination, const QString& xuid, const QString& titleId) {
    if (!QRegularExpression("^[0-9A-F]{16}$").match(xuid).hasMatch() || !validTitle(titleId))
        return QObject::tr("Selecciona un perfil y un título válidos antes de iniciar Edge.");
    if (!safeDirectory(destination)) return QObject::tr("La carpeta de Edge contiene un enlace.");
    const auto targetRoot = QDir(destination).absolutePath();
    for (const auto& root : {internal, external}) {
        if (root.isEmpty() || !QFileInfo(root).isDir()) continue;
        const auto sourceRoot = QFileInfo(root).canonicalFilePath();
        if (targetRoot.compare(sourceRoot, Qt::CaseInsensitive) == 0 ||
            targetRoot.startsWith(sourceRoot + '/', Qt::CaseInsensitive) ||
            sourceRoot.startsWith(targetRoot + '/', Qt::CaseInsensitive))
            return QObject::tr("Las carpetas de Edge y del core actual deben ser independientes.");
    }
    const auto title = titleId.toUpper();
    const auto seed = QDir(destination).absoluteFilePath("../imports/" + xuid + '-' + title + ".done");
    if (QFileInfo::exists(seed)) {
        // New DLC / TU packages can be added without replacing Edge's saves.
        for (const auto& root : {internal, external}) {
            if (root.isEmpty()) continue;
            const auto relative = QStringLiteral("0000000000000000/") + title;
            const auto error = copyMissing(QDir(root).filePath(relative), QDir(destination).filePath(relative));
            if (!error.isEmpty()) return error;
        }
        return {};
    }
    for (const auto& root : {internal, external}) {
        if (root.isEmpty() || !QFileInfo(root).isDir()) continue;
        for (const auto& relative : {xuid + "/FFFE07D1", xuid + '/' + title,
                                     QStringLiteral("0000000000000000/") + title}) {
            const auto error = copyMissing(QDir(root).filePath(relative), QDir(destination).filePath(relative));
            if (!error.isEmpty()) return error;
        }
    }
    const auto profile = QDir(destination).filePath(xuid + "/FFFE07D1/00010000/" + xuid);
    if (!QFileInfo::exists(profile)) return QObject::tr("No se encuentra el paquete del perfil para copiarlo a Edge.");
    if (!writeBytes(seed, "Imported once. Existing Edge progress is never overwritten.\n"))
        return QObject::tr("No se pudo registrar la importación a Edge.");
    return {};
}

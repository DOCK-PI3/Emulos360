#include "core_options.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

namespace {
bool validTitle(const QString& id) {
    return QRegularExpression("^[0-9A-Fa-f]{8}$").match(id).hasMatch();
}
}
CoreOptions::CoreOptions(const QString& data, QObject* parent)
    : QObject(parent), preferences_(QDir(data).filePath("cores.ini"), QSettings::IniFormat) {
    migrateLegacyPreferences();
}
void CoreOptions::migrateLegacyPreferences() {
    QStringList obsolete;
    for (const auto& key : preferences_.allKeys()) {
        if (key == "defaultCore" || key == "renderers/edge" || key == "renderers/xblig" ||
            (key.startsWith("games/") && key.endsWith("/core"))) obsolete << key;
    }
    if (obsolete.isEmpty()) return;
    const auto path = preferences_.fileName();
    const QFileInfo info(path);
    if (info.isSymbolicLink() || info.isJunction() || preferences_.status() != QSettings::NoError) {
        fail(tr("No se pudieron migrar las preferencias antiguas. Se utilizará Xenia Canary.")); return;
    }
    auto backup = path + ".before-single-core";
    if (QFileInfo::exists(backup)) backup += '.' + QUuid::createUuid().toString(QUuid::Id128);
    if (!QFile::copy(path, backup)) {
        fail(tr("No se pudieron respaldar las preferencias antiguas. Se utilizará Xenia Canary.")); return;
    }
    // Preserve the main renderer and per-title GPU overrides. Never access content/profile roots.
    for (const auto& key : obsolete) preferences_.remove(key);
    preferences_.sync();
    if (preferences_.status() != QSettings::NoError)
        fail(tr("No se pudieron guardar las preferencias migradas. Su copia anterior está conservada."));
}
QVariantList CoreOptions::rendererChoices() const {
    QVariantList result{QVariantMap{{"value", ""}, {"label", tr("Usar ajustes del motor")}}};
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
QString CoreOptions::defaultRenderer() const {
    const auto id = preferences_.value("renderers/netplay").toString();
    return validRenderer(id) ? id : QString{};
}
bool CoreOptions::fail(const QString& text) { status_ = text; emit changed(); return false; }
bool CoreOptions::persist() {
    preferences_.sync();
    if (preferences_.status() != QSettings::NoError) return fail(tr("No se pudieron guardar las preferencias gráficas."));
    status_ = tr("Preferencias guardadas. Se aplicarán al próximo inicio del juego."); emit changed(); return true;
}
void CoreOptions::setLocked(bool value) { if (locked_ != value) { locked_ = value; emit changed(); } }
void CoreOptions::setDefaultRenderer(const QString& id) {
    if (locked_ || !validRenderer(id) || id == defaultRenderer()) return;
    preferences_.setValue("renderers/netplay", id); persist();
}
QVariantMap CoreOptions::gameOptions(const QString& titleId) const {
    const auto group = "games/" + titleId.toUpper() + '/';
    const auto renderer = validTitle(titleId) ? preferences_.value(group + "renderer").toString() : QString{};
    return {{"renderer", validRenderer(renderer) ? renderer : QString{}},
            {"effectiveRenderer", !renderer.isEmpty() && validRenderer(renderer) ? renderer : defaultRenderer()}};
}
QString CoreOptions::rendererFor(const QString& titleId) const { return gameOptions(titleId)["effectiveRenderer"].toString(); }
bool CoreOptions::saveGameOptions(const QString& titleId, const QString& renderer) {
    if (locked_) return fail(tr("Cierra el juego antes de cambiar sus ajustes gráficos."));
    if (!validTitle(titleId) || !validRenderer(renderer)) return fail(tr("El título o motor gráfico no es válido."));
    const auto key = "games/" + titleId.toUpper() + "/renderer";
    if (renderer.isEmpty()) preferences_.remove(key); else preferences_.setValue(key, renderer);
    return persist();
}

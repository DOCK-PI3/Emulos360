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
// A profile only controls resolution and presentation. Leave game-specific
// correctness settings (EDRAM, readback, shaders) and network settings intact.
QVariantMap graphicsValues(const QString& id) {
    if (id.isEmpty() || id == "custom") return {};
    QVariantMap values{{"GPU.draw_resolution_scale_x", "1"},
        {"GPU.draw_resolution_scale_y", "1"},
        {"Display.postprocess_antialiasing", ""},
        {"Display.postprocess_scaling_and_sharpening", ""},
        {"Display.postprocess_ffx_cas_additional_sharpness", "0"},
        {"Display.postprocess_ffx_fsr_max_upsampling_passes", "4"},
        {"Display.postprocess_ffx_fsr_sharpness_reduction", "0.20000000298023224"},
        {"Display.postprocess_dither", true}};
    if (id == "default") return values;
    values["Display.postprocess_antialiasing"] = "fxaa";
    if (id == "720p") {
        values["Display.postprocess_scaling_and_sharpening"] = "cas";
        values["Display.postprocess_ffx_cas_additional_sharpness"] = "0.15";
    } else if (id == "1080p-fsr") {
        values["Display.postprocess_scaling_and_sharpening"] = "fsr";
        values["Display.postprocess_ffx_fsr_max_upsampling_passes"] = "1";
    } else if (id == "1440p" || id == "4k-fsr" || id == "4k-quality") {
        const auto scale = id == "4k-quality" ? "3" : "2";
        values["GPU.draw_resolution_scale_x"] = scale;
        values["GPU.draw_resolution_scale_y"] = scale;
        values["Display.postprocess_scaling_and_sharpening"] = id == "4k-fsr" ? "fsr" : "cas";
        values["Display.postprocess_ffx_cas_additional_sharpness"] = id == "1440p" ? "0.1" : "0";
        values["Display.postprocess_ffx_fsr_max_upsampling_passes"] = "2";
    } else return {};
    return values;
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
QVariantList CoreOptions::graphicsProfileChoices() const {
    return {
        QVariantMap{{"value", "custom"}, {"label", tr("Personalizado · ajustes del motor")},
            {"description", tr("Utiliza tu configuración manual actual de resolución y postprocesado.")}},
        QVariantMap{{"value", "default"}, {"label", tr("Predeterminado · original")},
            {"description", tr("Escala 1×, sin nitidez ni suavizado adicionales. Recupera los valores originales de resolución y postprocesado.")}},
        QVariantMap{{"value", "720p"}, {"label", tr("720p mejorado")},
            {"description", tr("Resolución nativa (1×), CAS suave y FXAA en Direct3D 12. Prioriza un coste gráfico bajo.")}},
        QVariantMap{{"value", "1080p-fsr"}, {"label", tr("1080p · FSR")},
            {"description", tr("Resolución nativa (1×), FSR 1.0 y FXAA en Direct3D 12. Pensado para ampliar la imagen a una pantalla de 1080p.")}},
        QVariantMap{{"value", "1440p"}, {"label", tr("1440p · equilibrado")},
            {"description", tr("Escala interna 2×, CAS suave y FXAA en Direct3D 12. Más detalle con mayor uso de GPU y memoria.")}},
        QVariantMap{{"value", "4k-fsr"}, {"label", tr("4K · FSR")},
            {"description", tr("Escala interna 2×, FSR 1.0 y FXAA en Direct3D 12. Amplía hacia 4K con menor carga que la escala 3×.")}},
        QVariantMap{{"value", "4k-quality"}, {"label", tr("4K · calidad")},
            {"description", tr("Escala interna 3×, CAS y FXAA en Direct3D 12. Mayor detalle y consumo de GPU y memoria; algunos juegos no admiten este escalado.")}}
    };
}
bool CoreOptions::validGraphicsProfile(const QString& value) const {
    for (const auto& row : graphicsProfileChoices()) if (row.toMap()["value"].toString() == value) return true;
    return false;
}
QString CoreOptions::defaultGraphicsProfile() const {
    const auto id = preferences_.value("graphics/defaultProfile", "custom").toString();
    return validGraphicsProfile(id) ? id : QStringLiteral("custom");
}
void CoreOptions::setDefaultGraphicsProfile(const QString& id) {
    if (locked_) { fail(tr("Cierra el juego antes de cambiar el perfil gráfico.")); return; }
    if (!validGraphicsProfile(id)) { fail(tr("El perfil gráfico no es válido.")); return; }
    if (id == defaultGraphicsProfile()) return;
    const auto old = preferences_.value("graphics/defaultProfile");
    preferences_.setValue("graphics/defaultProfile", id);
    if (!persist()) {
        if (old.isValid()) {
            preferences_.setValue("graphics/defaultProfile", old);
        } else {
            preferences_.remove("graphics/defaultProfile");
        }
        emit changed();
    }
}
QVariantMap CoreOptions::defaultGraphicsValues() const { return graphicsValues(defaultGraphicsProfile()); }
QString CoreOptions::graphicsProfileDescription(const QString& id) const {
    for (const auto& row : graphicsProfileChoices()) if (row.toMap()["value"].toString() == id) return row.toMap()["description"].toString();
    return {};
}
QString CoreOptions::graphicsProfileFor(const QString& titleId) const { return gameOptions(titleId)["effectiveGraphicsProfile"].toString(); }
QStringList CoreOptions::graphicsArgumentsFor(const QString& titleId) const {
    QStringList arguments;
    const auto values = graphicsValues(graphicsProfileFor(titleId));
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        const auto value = it.value().metaType().id() == QMetaType::Bool ? (it.value().toBool() ? "true" : "false") : it.value().toString();
        arguments << "--" + it.key().section('.', -1) + '=' + value;
    }
    return arguments;
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
    const auto profile = validTitle(titleId) ? preferences_.value(group + "graphicsProfile").toString() : QString{};
    return {{"renderer", validRenderer(renderer) ? renderer : QString{}},
            {"effectiveRenderer", !renderer.isEmpty() && validRenderer(renderer) ? renderer : defaultRenderer()},
            {"graphicsProfile", validGraphicsProfile(profile) ? profile : QString{}},
            {"effectiveGraphicsProfile", validGraphicsProfile(profile) ? profile : defaultGraphicsProfile()}};
}
QString CoreOptions::rendererFor(const QString& titleId) const { return gameOptions(titleId)["effectiveRenderer"].toString(); }
bool CoreOptions::saveGameOptions(const QString& titleId, const QString& renderer, const QString& graphicsProfile) {
    if (locked_) return fail(tr("Cierra el juego antes de cambiar sus ajustes gráficos."));
    if (!validTitle(titleId) || !validRenderer(renderer) || (!graphicsProfile.isEmpty() && !validGraphicsProfile(graphicsProfile)))
        return fail(tr("El título, motor o perfil gráfico no es válido."));
    const auto group = "games/" + titleId.toUpper() + '/';
    const auto oldRenderer = preferences_.value(group + "renderer"), oldProfile = preferences_.value(group + "graphicsProfile");
    if (renderer.isEmpty()) preferences_.remove(group + "renderer"); else preferences_.setValue(group + "renderer", renderer);
    if (graphicsProfile.isEmpty()) preferences_.remove(group + "graphicsProfile"); else preferences_.setValue(group + "graphicsProfile", graphicsProfile);
    if (persist()) return true;
    const QVariantMap oldValues{{"renderer", oldRenderer}, {"graphicsProfile", oldProfile}};
    for (auto it = oldValues.cbegin(); it != oldValues.cend(); ++it) {
        if (it.value().isValid()) preferences_.setValue(group + it.key(), it.value());
        else preferences_.remove(group + it.key());
    }
    emit changed(); return false;
}

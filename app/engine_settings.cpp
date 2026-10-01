#include "engine_settings.h"
#include "console_config.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <cmath>
#include <limits>
#include <sstream>
// This code handles parse_result errors explicitly, including exception-enabled builds.
#define TOML_EXCEPTIONS 0
#include "third_party/tomlplusplus/toml.hpp"

namespace {
QByteArray readFile(const QString& path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); }
bool writeFile(const QString& path, const QByteArray& data) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}
toml::table* category(toml::table& root, const QString& name) {
    auto* node = &root;
    for (const auto& part : name.split('.')) {
        auto key = part.toStdString();
        if (!node->contains(key)) node->insert(key, toml::table{});
        node = node->get_as<toml::table>(key);
        if (!node) return nullptr;
    }
    return node;
}
}

EngineSettings::EngineSettings(const QString& root, QObject* parent) : QObject(parent), root_(QDir(root).absolutePath()) {
    const auto catalog = QJsonDocument::fromJson(readFile(":/app/settings-catalog.json")).object();
    entries_ = catalog.value("entries").toArray().toVariantList();
#ifdef Q_OS_LINUX
    for (auto& row : entries_) {
        auto item = row.toMap();
        const auto key = item["key"].toString();
        if (key != "GPU.gpu" && key != "APU.apu" && key != "HID.hid") continue;
        QVariantList choices;
        for (const auto& choice : item["choices"].toList()) {
            const auto value = choice.toMap()["value"].toString();
            if (value != "d3d12" && value != "xaudio2" && value != "xinput" && value != "winkey") choices.append(choice);
        }
        item["choices"] = choices;
        if (key == "GPU.gpu") item["default"] = "vulkan";
        row = item;
    }
#endif
    auto console = los::consoleCatalog(catalog.value("countries").toArray().toVariantList());
    for (auto& row : console) {
        auto item = row.toMap();
        if (item["group"] == "console_audio") item["group"] = "audio";
        if (item["group"] == "console_video") item["group"] = "display";
        row = item;
    }
    entries_.append(console);
    for (const auto& row : entries_) { const auto item = row.toMap(); schema_[item["key"].toString()] = item; }
    reload();
}
QString EngineSettings::configPath() const { return QDir(root_).filePath("xenia-canary.config.toml"); }
QString EngineSettings::storagePath() const {
    const auto path = values_.value("Storage.storage_root").toString();
    return path.isEmpty() ? root_ : QDir::cleanPath(QDir(root_).absoluteFilePath(path));
}
QString EngineSettings::contentPath() const {
    const auto path = values_.value("Storage.content_root").toString();
    return QDir(storagePath()).absoluteFilePath(path.isEmpty() ? "content" : path);
}
QString EngineSettings::externalContentPath() const {
    const auto path = values_.value("Storage.emulos_external_content_root").toString();
    return QDir(storagePath()).absoluteFilePath(path.isEmpty() ? "external-content" : path);
}
QVariantMap EngineSettings::modified() const {
    QVariantMap result;
    for (auto it = values_.cbegin(); it != values_.cend(); ++it) if (it.value() != saved_.value(it.key())) result[it.key()] = true;
    return result;
}
bool EngineSettings::dirty() const { return imported_ || values_ != saved_ || !errors_.isEmpty(); }
void EngineSettings::setLocked(bool value) { if (locked_ != value) { locked_ = value; emit changed(); } }
bool EngineSettings::fail(const QString& text) { status_ = text; emit changed(); return false; }

bool EngineSettings::normalize(const QVariantMap& item, const QVariant& input, QVariant& output, QString& error) const {
    const auto type = item["type"].toString();
    const auto text = input.toString().trimmed();
    bool ok = true;
    double numeric = 0;
    if (type == "bool") {
        ok = input.metaType().id() == QMetaType::Bool;
        output = input.toBool();
    } else if (type == "string" || type == "path") output = input.toString();
    else if (type.startsWith("hex")) {
        auto hex = text; hex.remove(QRegularExpression("[: -]"));
        const int length = type == "hex64" ? 16 : type == "hex6" ? 12 : 8;
        ok = QRegularExpression(QString("^[0-9a-fA-F]{%1}$").arg(length)).match(hex).hasMatch();
        output = type == "hex64" ? hex.toUpper() : QString::fromLatin1(QByteArray::fromHex(hex.toLatin1()).toHex(':').toUpper());
    } else if (type == "float" || type == "double") {
        auto decimal = text; decimal.replace(',', '.');
        numeric = QLocale::c().toDouble(decimal, &ok);
        ok = ok && std::isfinite(numeric) && (type != "float" || std::abs(numeric) <= std::numeric_limits<float>::max());
        output = QString::number(numeric, 'g', type == "float" ? 9 : 17);
    } else {
        const auto number = text.toLongLong(&ok, text.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10);
        if (type.startsWith("uint")) ok = ok && number >= 0;
        if (type == "int32") ok = ok && number >= INT32_MIN && number <= INT32_MAX;
        if (type == "uint32") ok = ok && number <= UINT32_MAX;
        numeric = static_cast<double>(number);
        output = QString::number(number);
    }
    if (item.contains("min")) ok = ok && numeric >= item["min"].toDouble();
    if (item.contains("max")) ok = ok && numeric <= item["max"].toDouble();
    const auto choices = item.value("choices").toList();
    if (!choices.isEmpty()) {
        bool found = false;
        for (const auto& choice : choices) if (choice.toMap()["value"].toString() == output.toString()) found = true;
        // Preserve unknown values loaded from a newer engine, but do not invent new enum values.
        ok = ok && (found || output == saved_.value(item["key"].toString()));
    }
    if (!ok) error = tr("Valor no válido. Revisa el tipo, el intervalo o las opciones disponibles.");
    return ok;
}

bool EngineSettings::setValue(const QString& key, const QVariant& value) {
    if (locked_) return fail(tr("Cierra la sesión del motor antes de cambiar ajustes."));
    if (!schema_.contains(key) || schema_[key]["readonly"].toBool()) return false;
    QVariant normalized; QString error;
    if (!normalize(schema_[key],value,normalized,error)) {
        errors_[key] = error; emit changed(); return false;
    }
    errors_.remove(key);
    values_[key] = normalized;
    status_ = tr("Cambios pendientes de guardar.");
    emit changed();
    return true;
}
void EngineSettings::resetValue(const QString& key) {
    if (schema_.contains(key)) setValue(key,schema_[key]["default"]);
}
void EngineSettings::resetGroup(const QString& group) {
    if (locked_) return;
    for (const auto& row : entries_) { const auto item = row.toMap(); if (item["group"] == group) resetValue(item["key"].toString()); }
}

bool EngineSettings::loadToml(const QByteArray& source, QVariantMap& output, QString& error) const {
    {
        auto parsed = toml::parse(source.toStdString());
        if (!parsed) { error = QString::fromUtf8(parsed.error().description().data(), static_cast<qsizetype>(parsed.error().description().size())); return false; }
        const auto& table = parsed.table();
        for (const auto& row : entries_) {
            const auto item = row.toMap(); const auto key = item["key"].toString();
            if (key.startsWith("Console.")) continue;
            auto value = item["default"];
            const auto node = table.at_path(key.toStdString());
            const auto type = item["type"].toString();
            if (node) {
                if (type == "bool" && node.is_boolean()) value = *node.value<bool>();
                else if ((type == "string" || type == "path") && node.is_string()) value = QString::fromStdString(*node.value<std::string>());
                else if ((type == "float" || type == "double") && node.is_number()) value = QString::number(*node.value<double>(),'g',type == "float" ? 9 : 17);
                else if (type.contains("int") && node.is_integer()) value = QString::number(*node.value<int64_t>());
                else { error = tr("Tipo incorrecto en %1.").arg(key); return false; }
            }
#ifdef Q_OS_LINUX
            // Configurations copied from Windows may select unavailable backends.
            if ((key == "GPU.gpu" && value == "d3d12") ||
                (key == "APU.apu" && value == "xaudio2") ||
                (key == "HID.hid" && (value == "xinput" || value == "winkey"))) value = item["default"];
#endif
            output[key] = value;
        }
        return true;
    }
}

bool EngineSettings::reload() {
    if (locked_) return fail(tr("Cierra el motor antes de recargar los ajustes."));
    if (schema_.isEmpty()) return fail(tr("No se pudo cargar el catálogo de ajustes."));
    if (QFileInfo::exists(configPath()) && !QFileInfo(configPath()).isReadable()) return fail(tr("No se puede leer el archivo de configuración."));
    auto source = readFile(configPath()); QVariantMap loaded; QString error;
    if (!loadToml(source,loaded,error)) { loadOk_ = false; return fail(tr("Configuración inválida: %1").arg(error)); }
    values_ = loaded;
    loadedConsolePath_ = QDir(storagePath()).filePath("xconfig.settings");
    diskConsole_ = readFile(loadedConsolePath_);
    if (QFileInfo::exists(loadedConsolePath_) && diskConsole_.size() != los::consoleConfigSize()) {
        loadOk_ = false; return fail(tr("xconfig.settings tiene un formato o tamaño incompatible. Se conserva sin modificar."));
    }
    consoleBytes_ = diskConsole_.isEmpty() ? los::consoleDefaults() : diskConsole_;
    const auto console = los::readConsole(consoleBytes_);
    for (auto it = console.cbegin(); it != console.cend(); ++it) values_[it.key()] = it.value();
    saved_ = values_; errors_.clear(); tomlBytes_ = source; diskToml_ = source; loadOk_ = true; imported_ = false;
    status_ = tr("Ajustes cargados. Los cambios se aplicarán en la próxima sesión.");
    emit changed(); return true;
}

QByteArray EngineSettings::serializeToml(QString& error) const {
    {
        auto parsed = toml::parse(tomlBytes_.toStdString());
        if (!parsed) { error = QString::fromUtf8(parsed.error().description().data(), static_cast<qsizetype>(parsed.error().description().size())); return {}; }
        auto& table = parsed.table();
        for (const auto& row : entries_) {
            const auto item = row.toMap(); const auto key = item["key"].toString();
            if (key.startsWith("Console.")) continue;
            auto* target = category(table,item["section"].toString());
            if (!target) { error = tr("Una sección del archivo tiene un tipo incompatible."); return {}; }
            const auto name = item["name"].toString().toStdString();
            const auto type = item["type"].toString(); const auto value = values_.value(key);
            if (type == "bool") target->insert_or_assign(name,value.toBool());
            else if (type == "string" || type == "path") target->insert_or_assign(name,value.toString().toStdString());
            else if (type == "float" || type == "double") target->insert_or_assign(name,value.toDouble());
            else target->insert_or_assign(name,static_cast<int64_t>(value.toLongLong()));
        }
        std::ostringstream stream; stream << toml::toml_formatter{table};
        return QByteArray::fromStdString(stream.str());
    }
}

bool EngineSettings::save() {
    if (locked_) return fail(tr("Cierra la sesión del motor antes de guardar."));
    if (!loadOk_ || !errors_.isEmpty()) return fail(tr("Corrige los errores antes de guardar."));
    if (readFile(configPath()) != diskToml_ || readFile(loadedConsolePath_) != diskConsole_) return fail(tr("El motor u otra aplicación cambió los archivos. Recarga antes de guardar."));
    QVariantMap changedConsole;
    for (auto it = values_.cbegin(); it != values_.cend(); ++it) if (it.key().startsWith("Console.") && it.value() != saved_.value(it.key())) changedConsole[it.key()] = it.value();
    const auto consolePath = QDir(storagePath()).filePath("xconfig.settings");
    if (consolePath != loadedConsolePath_ && !changedConsole.isEmpty()) return fail(tr("Guarda primero la carpeta de datos; después modifica los ajustes de consola."));
    QString error; const auto serialized = serializeToml(error);
    if (!error.isEmpty()) return fail(error);
    const auto stamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz");
    for (const auto& path : {configPath(), loadedConsolePath_}) {
        if (QFileInfo::exists(path) && !QFile::copy(path,path + "." + stamp + ".bak")) return fail(tr("No se pudo crear una copia de seguridad."));
    }
    if (!writeFile(configPath(),serialized)) return fail(tr("No se pudo guardar el TOML."));
    if (!changedConsole.isEmpty() && !writeFile(consolePath,los::writeConsole(consoleBytes_,changedConsole))) {
        const bool restored = writeFile(configPath(),diskToml_);
        return fail(restored ? tr("No se pudo guardar la consola; se recuperó el TOML anterior.") : tr("Error al guardar. Recupera los archivos desde las copias .bak."));
    }
    if (!reload()) return false;
    status_ = tr("Ajustes guardados para la próxima sesión."); emit changed(); return true;
}

bool EngineSettings::importConfig(const QUrl& file) {
    if (locked_ || !file.isLocalFile()) return false;
    QFile input(file.toLocalFile());
    if (!input.open(QIODevice::ReadOnly)) return fail(tr("No se pudo abrir el TOML seleccionado."));
    const auto source = input.readAll(); QVariantMap imported; QString error;
    if (!loadToml(source,imported,error)) return fail(tr("No se puede importar: %1").arg(error));
    for (auto it = imported.cbegin(); it != imported.cend(); ++it) values_[it.key()] = it.value();
    tomlBytes_ = source; errors_.clear(); imported_ = true;
    status_ = tr("TOML importado. Pulsa Guardar para aplicarlo."); emit changed(); return true;
}
bool EngineSettings::exportConfig(const QUrl& file) {
    if (!file.isLocalFile() || !loadOk_ || !errors_.isEmpty()) return false;
    QString error; const auto data = serializeToml(error);
    return error.isEmpty() && writeFile(file.toLocalFile(),data) ? true : fail(tr("No se pudo exportar el TOML."));
}
void EngineSettings::openFolder() { QDir().mkpath(root_); QDesktopServices::openUrl(QUrl::fromLocalFile(root_)); }

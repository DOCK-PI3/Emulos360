#include "console_config.h"
#include <QRegularExpression>
#include <cstring>
#include <algorithm>
#include <bit>
#include "xenia/kernel/xconfig.h"

namespace {
using xe::kernel::XConfigData;
using namespace xe::kernel;
// Xbox settings contain big endian numbers at byte offsets such as 0x332.
// Calling endian_store methods on those packed members violates their alignment.
template <typename T> T readPacked(const xe::be<T>* field) {
    std::array<std::byte, sizeof(T)> bytes;
    std::memcpy(bytes.data(), field, bytes.size());
    if constexpr (std::endian::native == std::endian::little) std::reverse(bytes.begin(), bytes.end());
    return std::bit_cast<T>(bytes);
}
template <typename T, typename V> void writePacked(xe::be<T>* field, V input) {
    auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(static_cast<T>(input));
    if constexpr (std::endian::native == std::endian::little) std::reverse(bytes.begin(), bytes.end());
    std::memcpy(field, bytes.data(), bytes.size());
}
XConfigData decode(const QByteArray& bytes) {
    XConfigData data{};
    if (bytes.size() == sizeof(data)) std::memcpy(&data, bytes.constData(), sizeof(data));
    return data;
}
QByteArray encode(const XConfigData& data) {
    return QByteArray(reinterpret_cast<const char*>(&data), sizeof(data));
}
void setTimezone(XConfigData& data, int index) {
    const auto& tz = kTimezones.at(static_cast<size_t>(index));
    writePacked(&data.user.time_zone_bias, readPacked(&tz.timezone_bias));
    data.user.tz_std_name = tz.tz_std_name;
    data.user.tz_dlt_name = tz.tz_dlt_name;
    data.user.tz_std_date = tz.tz_std_date;
    data.user.tz_dlt_date = tz.tz_dlt_date;
    writePacked(&data.user.tz_std_bias, readPacked(&tz.tz_std_bias));
    writePacked(&data.user.tz_dlt_bias, readPacked(&tz.tz_dlt_bias));
}
struct Flag { const char* name; const char* label; const char* group; uint32_t mask; bool audio; };
const Flag flags[] = {
    {"dst_off", "Desactivar horario de verano", "console", DSTOff, false},
    {"clock_24h", "Reloj de 24 horas", "console", TwentyFourHourClock, false},
    {"dashboard", "Dashboard inicializado", "console", DashboardInitialized, false},
    {"iptv", "IPTV inicializado", "console", IPTVEnabled, false},
    {"dvr", "DVR inicializado", "console", IPTVDVREnabled, false},
    {"kinect", "Kinect inicializado", "console", KinectInitialized, false},
    {"mono", "Audio mono", "console_audio", AnalogMono, true},
    {"prologic", "Dolby Pro Logic", "console_audio", DolbyProLogic, true},
    {"dolby", "Dolby Digital", "console_audio", DolbyDigital, true},
    {"wmapro", "Dolby Digital WMA Pro", "console_audio", DolbyDigitalWithWMAPRO, true},
    {"low_latency", "Baja latencia (sin soporte en el motor)", "console_audio", LowLatency, true}
};
QVariantMap choice(const QString& label, const QString& value) { return {{"label",label},{"value",value}}; }
}

namespace los {
qsizetype consoleConfigSize() { return sizeof(XConfigData); }
QByteArray consoleDefaults() {
    XConfigData data{};
    writePacked(&data.secured.av_region, X_AV_REGION::NTSCM);
    writePacked(&data.user.language, 1);
    data.user.country = 103;
    writePacked(&data.user.audio_flags, DolbyDigital | DolbyProLogic);
    writePacked(&data.user.av_pack_hdmi_sz, XHDTVResolution.at(1).to_host());
    writePacked(&data.user.av_pack_component_sz, XHDTVResolution.at(1).to_host());
    writePacked(&data.user.av_pack_vga_sz, XVGAResolution.at(3).to_host());
    writePacked(&data.user.retail_flags, DashboardInitialized);
    writePacked(&data.user.video_flags, RatioNormal);
    data.user.parental_control_flags = XBLAllowed | XBLMembershipCreationAllowed;
    writePacked(&data.user.parental_control_game, NoGameRestrictions);
    writePacked(&data.user.music_volume, 0.7f);
    setTimezone(data, 0x19);
    // Match upstream defaults without changing any unrelated bytes in existing files.
    const char16_t provider[] = u"Xenia TV";
    for (size_t i = 0; i < std::size(provider); ++i)
        data.iptv.service_provider_name[i] = xe::byte_swap(provider[i]);
    return encode(data);
}

QVariantMap readConsole(const QByteArray& bytes) {
    const auto d = decode(bytes);
    QVariantMap values;
    const auto put = [&values](const QString& key, const QVariant& value) { values["Console." + key] = value; };
    put("language", QString::number(readPacked(&d.user.language)));
    put("country", QString::number(d.user.country));
    put("profile", QString("%1").arg(readPacked(&d.user.default_profile), 16, 16, QChar('0')).toUpper());
    put("parental", bool(d.user.parental_control_flags & PCEnabled));
    put("av_region", QString::number(readPacked(&d.secured.av_region)));
    put("resolution", QString::number(readPacked(&d.user.av_pack_hdmi_sz)));
    put("widescreen", bool(readPacked(&d.user.video_flags) & Widescreen));
    put("music_volume", QString::number(readPacked(&d.user.music_volume), 'g', 8));
    put("mac", QByteArray(reinterpret_cast<const char*>(d.secured.mac_address.data()), 6).toHex(':').toUpper());
    put("network_id", QByteArray(reinterpret_cast<const char*>(d.secured.online_network_id.data()), 4).toHex(':').toUpper());
    int tzIndex = -1;
    for (size_t i = 0; i < kTimezones.size(); ++i) {
        const auto& tz = kTimezones[i];
        if (readPacked(&d.user.time_zone_bias) == readPacked(&tz.timezone_bias) && d.user.tz_std_name == tz.tz_std_name &&
            d.user.tz_dlt_name == tz.tz_dlt_name && d.user.tz_std_date == tz.tz_std_date &&
            d.user.tz_dlt_date == tz.tz_dlt_date && readPacked(&d.user.tz_std_bias) == readPacked(&tz.tz_std_bias) &&
            readPacked(&d.user.tz_dlt_bias) == readPacked(&tz.tz_dlt_bias)) { tzIndex = static_cast<int>(i); break; }
    }
    put("timezone", QString::number(tzIndex));
    for (const auto& f : flags) put(f.name, bool((f.audio ? readPacked(&d.user.audio_flags) : readPacked(&d.user.retail_flags)) & f.mask));
    return values;
}

QVariantList consoleCatalog(const QVariantList& countries) {
    const auto defaults = readConsole(consoleDefaults());
    QVariantList result;
    const auto add = [&](const QString& name, const QString& label, const QString& type, const QString& group, QVariantList choices = {}, const QString& help = "") {
        QVariantMap item{{"key", "Console." + name}, {"name", name}, {"label",label}, {"type",type},
            {"group",group}, {"section","Console"}, {"default", defaults.value("Console." + name)},
            {"choices",choices}, {"help",help}, {"description",""}, {"readonly",false}};
        result.append(item);
    };
    const std::pair<int,const char*> languages[] = {{1,"Inglés"},{2,"Japonés"},{3,"Alemán"},{4,"Francés"},{5,"Español"},{6,"Italiano"},{7,"Coreano"},{8,"Chino tradicional"},{9,"Portugués"},{11,"Polaco"},{12,"Ruso"},{13,"Sueco"},{14,"Turco"},{15,"Noruego"},{16,"Neerlandés"},{17,"Chino simplificado"}};
    QVariantList languageChoices;
    for (const auto& [id, name] : languages) languageChoices.append(choice(QString::fromUtf8(name),QString::number(id)));
    add("language","Idioma de la consola","uint32","console",languageChoices);
    add("country","País de la consola","uint32","console",countries,"Código de país de la consola virtual; no cambia la región de las carátulas.");
    QVariantList zones;
    for (size_t i = 0; i < kTimezones.size(); ++i) zones.append(choice(QString::fromStdString(kTimezones[i].name),QString::number(i)));
    add("timezone","Zona horaria","int32","console",zones);
    for (const auto& f : flags) add(f.name,QString::fromUtf8(f.label),"bool",f.group);
    add("profile","Perfil predeterminado de la consola","hex64","profiles",{},"XUID de 16 dígitos hexadecimales. 0000000000000000: ninguno.");
    add("parental","Control parental de la consola","bool","profiles");
    add("av_region","Región de vídeo de la consola","uint32","console_video",{choice("NTSC","4194560"),choice("NTSC-J","4194816"),choice("PAL","4195328"),choice("PAL 50 Hz","8389376")});
    QVariantList resolutions;
    for (const auto& res : XVGAResolution) resolutions.append(choice(QString::fromStdString(res.name_),QString::number(res.to_host())));
    add("resolution","Resolución de la consola","int32","console_video",resolutions,"Resolución que recibe el juego; diferente del escalado de renderizado.");
    add("widescreen","Formato panorámico","bool","console_video",{},"Las resoluciones panorámicas activan este indicador automáticamente.");
    add("music_volume","Volumen del reproductor de música","float","console_audio");
    auto volume = result.last().toMap(); volume["min"] = 0; volume["max"] = 1; result.last() = volume;
    add("mac","Dirección MAC virtual","hex6","network");
    add("network_id","Identificador de red virtual","hex4","network");
    return result;
}

QByteArray writeConsole(const QByteArray& original, const QVariantMap& changed) {
    auto d = decode(original);
    for (auto it = changed.cbegin(); it != changed.cend(); ++it) {
        const auto name = it.key().mid(8);
        const auto value = it.value();
        if (name == "language") writePacked(&d.user.language, value.toUInt());
        else if (name == "country") d.user.country = static_cast<uint8_t>(value.toUInt());
        else if (name == "profile") writePacked(&d.user.default_profile, value.toString().toULongLong(nullptr,16));
        else if (name == "parental") d.user.parental_control_flags = static_cast<uint8_t>(value.toBool() ? d.user.parental_control_flags | PCEnabled : d.user.parental_control_flags & ~PCEnabled);
        else if (name == "av_region") writePacked(&d.secured.av_region, value.toUInt());
        else if (name == "resolution") {
            writePacked(&d.user.av_pack_hdmi_sz, value.toInt());
            const Resolution res(readPacked(&d.user.av_pack_hdmi_sz));
            const auto flags = readPacked(&d.user.video_flags);
            writePacked(&d.user.video_flags, res.is_widescreen() ? flags | Widescreen : flags & ~Widescreen);
        } else if (name == "widescreen") {
            const bool wide = value.toBool() || Resolution(readPacked(&d.user.av_pack_hdmi_sz)).is_widescreen();
            const auto flags = readPacked(&d.user.video_flags);
            writePacked(&d.user.video_flags, wide ? flags | Widescreen : flags & ~Widescreen);
        } else if (name == "music_volume") writePacked(&d.user.music_volume, value.toFloat());
        else if (name == "timezone") setTimezone(d,value.toInt());
        else if (name == "mac" || name == "network_id") {
            auto hex = value.toString(); hex.remove(QRegularExpression("[: -]"));
            const auto raw = QByteArray::fromHex(hex.toLatin1());
            if (name == "mac") std::memcpy(d.secured.mac_address.data(), raw.constData(), 6);
            else std::memcpy(d.secured.online_network_id.data(), raw.constData(), 4);
        } else for (const auto& flag : flags) if (name == flag.name) {
            auto* field = flag.audio ? &d.user.audio_flags : &d.user.retail_flags;
            const auto bits = readPacked(field);
            writePacked(field, value.toBool() ? bits | flag.mask : bits & ~flag.mask);
        }
    }
    return encode(d);
}
}

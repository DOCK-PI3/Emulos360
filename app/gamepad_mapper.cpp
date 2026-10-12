#include "gamepad_mapper.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QWindow>
#include <QSaveFile>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
struct Control { const char* key; const char* label; };
constexpr Control kControls[] = {
    {"dpup", QT_TRANSLATE_NOOP("GamepadMapper", "Cruceta arriba")},
    {"dpdown", QT_TRANSLATE_NOOP("GamepadMapper", "Cruceta abajo")},
    {"dpleft", QT_TRANSLATE_NOOP("GamepadMapper", "Cruceta izquierda")},
    {"dpright", QT_TRANSLATE_NOOP("GamepadMapper", "Cruceta derecha")},
    {"a", "A / ×"}, {"b", "B / ○"}, {"x", "X / □"}, {"y", "Y / △"},
    {"leftshoulder", "LB / L1"}, {"rightshoulder", "RB / R1"},
    {"lefttrigger", "LT / L2"}, {"righttrigger", "RT / R2"},
    {"back", "Back / Select / Share"}, {"start", "Start / Options"},
    {"leftstick", "LS / L3"}, {"rightstick", "RS / R3"},
    {"-leftx", QT_TRANSLATE_NOOP("GamepadMapper", "Stick izquierdo ←")},
    {"+leftx", QT_TRANSLATE_NOOP("GamepadMapper", "Stick izquierdo →")},
    {"-lefty", QT_TRANSLATE_NOOP("GamepadMapper", "Stick izquierdo ↑")},
    {"+lefty", QT_TRANSLATE_NOOP("GamepadMapper", "Stick izquierdo ↓")},
    {"-rightx", QT_TRANSLATE_NOOP("GamepadMapper", "Stick derecho ←")},
    {"+rightx", QT_TRANSLATE_NOOP("GamepadMapper", "Stick derecho →")},
    {"-righty", QT_TRANSLATE_NOOP("GamepadMapper", "Stick derecho ↑")},
    {"+righty", QT_TRANSLATE_NOOP("GamepadMapper", "Stick derecho ↓")},
    {"guide", QT_TRANSLATE_NOOP("GamepadMapper", "Guía / PS")}
};
constexpr int kCount = int(std::size(kControls));
QString guidFor(int index) {
    char text[33]{};
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), text, sizeof(text));
    return QString::fromLatin1(text);
}
QString familyFor(int index) {
    switch (SDL_GameControllerTypeForIndex(index)) {
    case SDL_CONTROLLER_TYPE_XBOX360: return "xbox360";
    case SDL_CONTROLLER_TYPE_XBOXONE: return "xboxone";
    case SDL_CONTROLLER_TYPE_PS3: return "ps3";
    case SDL_CONTROLLER_TYPE_PS4: return "ps4";
    case SDL_CONTROLLER_TYPE_PS5: return "ps5";
    default: return "generic";
    }
}
QMap<QString, QString> parse(const QString& line) {
    QMap<QString, QString> result;
    const auto fields = line.split(',');
    const QRegularExpression sourcePattern("^[+-]?(?:b[0-9]+|a[0-9]+~?|h[0-9]+\\.[1248])$");
    for (int i = 2; i < fields.size(); ++i) {
        const auto pair = fields[i].split(':');
        if (pair.size() != 2 || !sourcePattern.match(pair[1]).hasMatch()) continue;
        const auto key = pair[0];
        if (key == "leftx" || key == "lefty" || key == "rightx" || key == "righty") {
            auto source = pair[1];
            const bool inverted = source.endsWith('~');
            if (inverted) source.chop(1);
            if (source.startsWith('a')) {
                result["-" + key] = (inverted ? "+" : "-") + source;
                result["+" + key] = (inverted ? "-" : "+") + source;
            } else {
                result["+" + key] = source;
            }
        } else {
            for (const auto& control : kControls)
                if (key == QLatin1String(control.key)) result[key] = pair[1];
        }
    }
    return result;
}
QString databasePath(const QString& data) { return QDir(data).filePath("controllers/gamecontrollerdb.txt"); }
QStringList databaseLines(const QString& data) {
    QFile file(databasePath(data));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) return {};
    return QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
}
QString bindingLabel(QString source) {
    if (source.isEmpty()) return {};
    const auto original = source;
    if (source.startsWith('+') || source.startsWith('-')) source.remove(0, 1);
    if (source.startsWith('b')) return GamepadMapper::tr("Botón %1").arg(source.mid(1).toInt() + 1);
    if (source.startsWith('a')) return GamepadMapper::tr("Eje %1 %2").arg(source.mid(1).remove('~').toInt() + 1)
        .arg(original.startsWith('-') ? "−" : original.startsWith('+') ? "+" : "");
    return GamepadMapper::tr("Cruceta %1").arg(source.mid(1).section('.', 0, 0).toInt() + 1);
}
bool overlaps(QString first, QString second) {
    if (first == second) return true;
    first.remove('~'); second.remove('~');
    const auto axis = [](QString value) {
        if (value.startsWith('+') || value.startsWith('-')) value.remove(0, 1);
        return value;
    };
    return axis(first).startsWith('a') && axis(first) == axis(second) &&
        (first.startsWith('a') || second.startsWith('a'));
}
}

struct GamepadMapper::Native {
    SDL_Joystick* joystick = nullptr;
    bool initialized = false;
    std::vector<int> neutral;
    ~Native() {
        if (joystick) SDL_JoystickClose(joystick);
        if (initialized) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
};
GamepadMapper::GamepadMapper(QString dataDirectory, QObject* parent)
    : QObject(parent), data_(std::move(dataDirectory)) {
    timer_.setInterval(16);
    connect(&timer_, &QTimer::timeout, this, &GamepadMapper::poll);
}
GamepadMapper::~GamepadMapper() = default;
QVariantList GamepadMapper::controls() const {
    QVariantList list;
    for (const auto& control : kControls) {
        const auto key = QString::fromLatin1(control.key);
        list.append(QVariantMap{{"key", key}, {"label", tr(control.label)},
                                {"binding", bindingLabel(bindings_.value(key))},
                                {"source", bindings_.value(key)}, {"assigned", bindings_.contains(key)}});
    }
    return list;
}
void GamepadMapper::begin() {
    if (active_) return;
    native_ = std::make_unique<Native>();
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    native_->initialized = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
    if (!native_->initialized) {
        status_ = tr("No se pudo iniciar la detección de mandos: %1").arg(QString::fromUtf8(SDL_GetError()));
        emit changed(); return;
    }
    active_ = true;
    selectDevice(-1);
    refreshDevices();
    timer_.start();
    emit changed();
}
void GamepadMapper::end() {
    timer_.stop();
    stopCapture();
    active_ = false;
    native_.reset();
    devices_.clear(); values_.clear(); selected_ = -1;
    emit inputChanged(); emit changed();
}
void GamepadMapper::refreshDevices() {
    QVariantList found;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        const auto type = SDL_JoystickGetDeviceType(i);
        if (type != SDL_JOYSTICK_TYPE_GAMECONTROLLER && type != SDL_JOYSTICK_TYPE_UNKNOWN) continue;
        const auto* deviceName = SDL_JoystickNameForIndex(i);
        found.append(QVariantMap{{"name", QString::fromUtf8(deviceName ? deviceName : "Gamepad")},
            {"index", i}, {"instance", SDL_JoystickGetDeviceInstanceID(i)},
            {"guid", guidFor(i)}, {"family", familyFor(i)}});
    }
    if (found == devices_) return;
    const int instance = native_->joystick ? SDL_JoystickInstanceID(native_->joystick) : -1;
    devices_ = found;
    int retained = -1;
    for (int i = 0; i < devices_.size(); ++i)
        if (devices_[i].toMap()["instance"].toInt() == instance) retained = i;
    if (retained >= 0) selected_ = retained;
    else selectDevice(instance >= 0 || devices_.isEmpty() ? -1 : 0);
    emit changed();
}
void GamepadMapper::selectDevice(int index) {
    if (!native_ || !native_->initialized) return;
    stopCapture();
    if (native_->joystick) SDL_JoystickClose(native_->joystick);
    native_->joystick = nullptr;
    selected_ = -1; bindings_.clear(); saved_.clear(); defaults_.clear();
    values_.clear(); guid_.clear(); name_.clear(); family_ = "generic";
    if (index >= 0 && index < devices_.size()) {
        const auto device = devices_[index].toMap();
        const int deviceIndex = device["index"].toInt();
        native_->joystick = SDL_JoystickOpen(deviceIndex);
        if (native_->joystick) {
            selected_ = index; guid_ = device["guid"].toString();
            name_ = device["name"].toString(); family_ = device["family"].toString();
            char* mapping = SDL_GameControllerMappingForDeviceIndex(deviceIndex);
            if (mapping) { defaults_ = parse(QString::fromUtf8(mapping)); SDL_free(mapping); }
            bindings_ = defaults_;
            for (const auto& line : databaseLines(data_))
                if (line.startsWith(guid_ + ',')) bindings_ = parse(line);
            saved_ = bindings_;
            SDL_JoystickUpdate();
            prepareCapture();
        }
    }
    status_ = selected_ >= 0 ? tr("Pulsa un control para probarlo o elige una asignación para cambiarla.")
                            : tr("Conecta un mando USB o Bluetooth. Se detectará automáticamente.");
    emit inputChanged(); emit changed();
}
void GamepadMapper::prepareCapture() {
    armed_ = false;
    native_->neutral.clear();
    if (!native_->joystick) return;
    for (int i = 0; i < SDL_JoystickNumAxes(native_->joystick); ++i) {
        Sint16 initial = 0;
        if (!SDL_JoystickGetAxisInitialState(native_->joystick, i, &initial))
            initial = SDL_JoystickGetAxis(native_->joystick, i);
        native_->neutral.push_back(initial);
    }
}
void GamepadMapper::capture(const QString& target) {
    if (!native_ || !native_->joystick || !SDL_JoystickGetAttached(native_->joystick)) return;
    const auto iterator = std::find_if(std::begin(kControls), std::end(kControls),
        [&target](const Control& c) { return target == QLatin1String(c.key); });
    if (iterator == std::end(kControls)) return;
    if (!guided_) step_ = 0;
    target_ = target; prepareCapture();
    status_ = tr("Suelta los controles y pulsa: %1").arg(tr(iterator->label));
    emit changed();
}
void GamepadMapper::startGuided() {
    guided_ = true; step_ = 0; capture(QString::fromLatin1(kControls[0].key));
    if (target_.isEmpty()) guided_ = false;
    emit changed();
}
void GamepadMapper::skip() {
    if (!guided_) return;
    if (++step_ < kCount) capture(QString::fromLatin1(kControls[step_].key));
    else { stopCapture(); step_ = kCount; status_ = tr("Asistente completado. Prueba el mando y guarda las asignaciones."); emit changed(); }
}
void GamepadMapper::stopCapture() {
    if (!target_.isEmpty()) status_ = tr("Captura detenida. Puedes probar el mando o elegir otro control.");
    guided_ = false; target_.clear(); armed_ = false; emit changed();
}
void GamepadMapper::assign(const QString& source) {
    // Swap an existing assignment rather than silently creating two actions on one button.
    const auto old = bindings_.value(target_);
    for (auto it = bindings_.begin(); it != bindings_.end(); ) {
        if (it.key() != target_ && overlaps(it.value(), source)) {
            if (old.isEmpty()) it = bindings_.erase(it);
            else { it.value() = old; ++it; }
        } else ++it;
    }
    bindings_[target_] = source;
    if (guided_) skip();
    else { stopCapture(); status_ = tr("Asignación capturada. Prueba el mando antes de guardar."); emit changed(); }
}
void GamepadMapper::restoreDefaults() {
    stopCapture(); bindings_ = defaults_;
    status_ = defaults_.isEmpty() ? tr("Este mando no tiene un mapa estándar. Usa el asistente para asignarlo.")
                                : tr("Mapa estándar restaurado. Pulsa Guardar para aplicarlo.");
    emit changed();
}
void GamepadMapper::discard() {
    stopCapture(); bindings_ = saved_;
    status_ = tr("Cambios descartados. Puedes probar el mando o seleccionar otro."); emit changed();
}
void GamepadMapper::poll() {
    if (!active_ || !native_ || !native_->initialized) return;
    SDL_PumpEvents(); SDL_JoystickUpdate();
    // This service polls state; discard its unused joystick events, not other SDL events.
    SDL_FlushEvents(SDL_JOYAXISMOTION, SDL_CONTROLLERDEVICEREMAPPED);
    if (++refreshTicks_ >= 30 || (native_->joystick && !SDL_JoystickGetAttached(native_->joystick))) {
        refreshTicks_ = 0; refreshDevices();
    }
    if (!native_->joystick || !SDL_JoystickGetAttached(native_->joystick)) return;
    auto* joystick = native_->joystick;
    const auto sourceValue = [joystick](QString source) -> double {
        bool inverted = source.endsWith('~'); if (inverted) source.chop(1);
        int sign = 0;
        if (source.startsWith('+') || source.startsWith('-')) { sign = source.startsWith('+') ? 1 : -1; source.remove(0, 1); }
        if (source.startsWith('b')) return SDL_JoystickGetButton(joystick, source.mid(1).toInt()) ? 1.0 : 0.0;
        if (source.startsWith('h')) {
            const auto parts = source.mid(1).split('.');
            return parts.size() == 2 && (SDL_JoystickGetHat(joystick, parts[0].toInt()) & parts[1].toInt()) ? 1.0 : 0.0;
        }
        if (source.startsWith('a')) {
            double value = SDL_JoystickGetAxis(joystick, source.mid(1).toInt()) / 32768.0;
            if (inverted) value = -value;
            return sign ? std::max(0.0, value * sign) : (value + 1.0) / 2.0;
        }
        return 0.0;
    };
    QVariantMap values;
    for (const auto& c : kControls) {
        const auto key = QString::fromLatin1(c.key);
        values[key] = sourceValue(bindings_.value(key));
    }
    if (values != values_) { values_ = values; emit inputChanged(); }
    if (target_.isEmpty()) return;
    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        const auto* window = QGuiApplication::focusWindow();
        if (!window || !window->isActive()) { armed_ = false; return; }
    }
    bool neutral = true;
    QString candidate;
    for (int i = 0; i < SDL_JoystickNumButtons(joystick); ++i)
        if (SDL_JoystickGetButton(joystick, i)) { neutral = false; if (candidate.isEmpty()) candidate = "b" + QString::number(i); }
    for (int i = 0; i < SDL_JoystickNumHats(joystick); ++i) {
        const int value = SDL_JoystickGetHat(joystick, i);
        if (value) neutral = false;
        if (value && (value & (value - 1)) == 0 && candidate.isEmpty()) candidate = QString("h%1.%2").arg(i).arg(value);
    }
    int largest = 0;
    QString axisCandidate;
    for (int i = 0; i < int(native_->neutral.size()); ++i) {
        const int value = SDL_JoystickGetAxis(joystick, i);
        const int delta = value - native_->neutral[size_t(i)];
        if (std::abs(delta) > 7000) neutral = false;
        if (std::abs(delta) > 18000 && std::abs(delta) > largest) {
            largest = std::abs(delta);
            const int rest = native_->neutral[size_t(i)];
            if ((target_ == "lefttrigger" || target_ == "righttrigger") && std::abs(rest) > 20000)
                axisCandidate = QString("a%1%2").arg(i).arg(delta < 0 ? "~" : "");
            else axisCandidate = QString("%1a%2").arg(delta < 0 ? "-" : "+").arg(i);
        }
    }
    if (!armed_) { if (neutral) armed_ = true; return; }
    if (candidate.isEmpty()) candidate = axisCandidate;
    if (!candidate.isEmpty()) assign(candidate);
}
QString GamepadMapper::mappingLine() const {
    QString name = name_; name.replace(',', ' '); name.replace('\n', ' '); name.replace('\r', ' ');
    QString line = guid_ + ',' + name + ',';
    for (auto it = bindings_.cbegin(); it != bindings_.cend(); ++it) line += it.key() + ':' + it.value() + ',';
    return line + "platform:" + QString::fromLatin1(SDL_GetPlatform()) + ',';
}
bool GamepadMapper::save() {
    if (!native_ || !native_->joystick || !SDL_JoystickGetAttached(native_->joystick) || !target_.isEmpty()) return false;
    if (bindings_.isEmpty()) { status_ = tr("Asigna los controles antes de guardar."); emit changed(); return false; }
    auto lines = databaseLines(data_);
    lines.removeIf([this](const QString& line) { return line.startsWith(guid_ + ','); });
    if (bindings_ != defaults_) lines.append(mappingLine());
    const auto path = databasePath(data_);
    QSaveFile file(path);
    const auto bytes = (lines.join('\n') + '\n').toUtf8();
    if (!QDir().mkpath(QFileInfo(path).absolutePath()) || !file.open(QIODevice::WriteOnly) ||
        file.write(bytes) != bytes.size() || !file.commit()) {
        status_ = tr("No se pudo guardar el mapa: %1").arg(file.errorString()); emit changed(); return false;
    }
    saved_ = bindings_;
    status_ = tr("Mapa guardado. Se aplicará al iniciar la próxima sesión de juego."); emit changed(); return true;
}
QStringList GamepadMapper::launchArguments(const QString& dataDirectory) {
    if (databaseLines(dataDirectory).isEmpty()) return {};
    // The Netplay fork's "any" factory uses SDL (skips XInput) and retains keyboard input.
    return {"--hid=any", "--mappings_file=" + QFileInfo(databasePath(dataDirectory)).absoluteFilePath()};
}

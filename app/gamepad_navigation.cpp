#include "gamepad_navigation.h"
#include "controller.h"
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLibrary>
#include <QTimer>
#include <QWindow>
#ifdef Q_OS_LINUX
#include <SDL.h>
#include <cstdint>
#include <memory>
#endif
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#endif

void installGamepadNavigation(Controller* controller, QObject* owner) {
#ifdef Q_OS_WIN
    auto library = new QLibrary(QStringLiteral("xinput1_4"), owner);
    using GetState = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
    auto getState = reinterpret_cast<GetState>(library->resolve("XInputGetState"));
    if (!getState) return;
    auto timer = new QTimer(owner);
    QObject::connect(timer,&QTimer::timeout,owner,[controller,getState,held=WORD(0),ticks=0,f8Held=false]() mutable {
        auto window = QGuiApplication::focusWindow();
        // A running core does not own input while the user browses the library.
        // Check native keyboard focus too: an embedded core can be a child of
        // the active Qt window while belonging to a different process.
        GUITHREADINFO focus{sizeof(GUITHREADINFO)};
        DWORD focusedProcess = 0;
        if (GetGUIThreadInfo(0, &focus) && focus.hwndFocus)
            GetWindowThreadProcessId(focus.hwndFocus, &focusedProcess);
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const auto coreWindow = controller->players()->coreWindow()->window();
        const bool sessionActive = controller->players()->sessionActive();
        // The last focus HWND may still identify the exited core when Qt's
        // parent window becomes foreground again. Only trust it while a game
        // is running; after exit route input to the active Qt window.
        if (!sessionActive && (!window || !window->isActive() || window == coreWindow)) {
            for (auto* candidate : QGuiApplication::topLevelWindows())
                if (candidate->isActive() && candidate != coreWindow) { window = candidate; break; }
        }
        const bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
        const bool ownWindowForeground = foregroundProcess == GetCurrentProcessId();
        if (ownWindowForeground && sessionActive &&
            focusedProcess == static_cast<DWORD>(controller->players()->sessionProcessId()) &&
            f8Down && !f8Held)
            controller->requestGameChromeToggle();
        f8Held = f8Down;
        if (!ownWindowForeground) { held=0; ticks=0; return; }
        XINPUT_STATE state{};
        bool connected=false;
        for (DWORD i=0;i<XUSER_MAX_COUNT;++i) if (getState(i,&state)==ERROR_SUCCESS) { connected=true; break; }
        if (!connected) { held=0; ticks=0; return; }
        WORD buttons=state.Gamepad.wButtons;
        if (state.Gamepad.sThumbLX > 18000) buttons |= XINPUT_GAMEPAD_DPAD_RIGHT;
        if (state.Gamepad.sThumbLX < -18000) buttons |= XINPUT_GAMEPAD_DPAD_LEFT;
        if (state.Gamepad.sThumbLY > 18000) buttons |= XINPUT_GAMEPAD_DPAD_UP;
        if (state.Gamepad.sThumbLY < -18000) buttons |= XINPUT_GAMEPAD_DPAD_DOWN;
        const WORD pressed=buttons & ~held;
        if (sessionActive && (pressed & XINPUT_GAMEPAD_RIGHT_THUMB))
            controller->requestGameChromeToggle();
        if (!window || !window->isActive() || window == coreWindow ||
            (sessionActive && focusedProcess != GetCurrentProcessId())) {
            held=buttons; ticks=0; return;
        }
        ticks=buttons==held ? ticks+1 : 0;
        const WORD repeat=ticks>=12 && ticks%4==0 ? buttons & 0xF : 0;
        held=buttons;
        const struct { WORD button; int key; } mapping[] = {
            {XINPUT_GAMEPAD_A,Qt::Key_Return}, {XINPUT_GAMEPAD_B,Qt::Key_Escape},
            {XINPUT_GAMEPAD_Y,Qt::Key_F6}, {XINPUT_GAMEPAD_X,Qt::Key_F8},
            {XINPUT_GAMEPAD_DPAD_UP,Qt::Key_Up}, {XINPUT_GAMEPAD_DPAD_DOWN,Qt::Key_Down},
            {XINPUT_GAMEPAD_DPAD_LEFT,Qt::Key_Left}, {XINPUT_GAMEPAD_DPAD_RIGHT,Qt::Key_Right},
            {XINPUT_GAMEPAD_LEFT_SHOULDER,Qt::Key_PageUp}, {XINPUT_GAMEPAD_RIGHT_SHOULDER,Qt::Key_PageDown},
            {XINPUT_GAMEPAD_START,Qt::Key_Home}, {XINPUT_GAMEPAD_BACK,Qt::Key_Tab}
        };
        for (const auto& item:mapping) if ((pressed|repeat)&item.button) {
            if (sessionActive && item.button == XINPUT_GAMEPAD_X) continue;
            QKeyEvent down(QEvent::KeyPress,item.key,Qt::NoModifier);
            QKeyEvent up(QEvent::KeyRelease,item.key,Qt::NoModifier);
            down.setAccepted(false);
            QCoreApplication::sendEvent(window,&down);
            QCoreApplication::sendEvent(window,&up);
            if (!down.isAccepted() && item.key >= Qt::Key_Left && item.key <= Qt::Key_Down) {
                const auto modifiers = item.key==Qt::Key_Left || item.key==Qt::Key_Up ? Qt::ShiftModifier : Qt::NoModifier;
                QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,modifiers);
                QKeyEvent release(QEvent::KeyRelease,Qt::Key_Tab,modifiers);
                QCoreApplication::sendEvent(window,&tab); QCoreApplication::sendEvent(window,&release);
            }
        }
    });
    timer->start(33);
#elif defined(Q_OS_LINUX)
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return;
    struct GamepadState {
        SDL_GameController* pad = nullptr;
        uint32_t held = 0;
        int ticks = 0;
        ~GamepadState() {
            if (pad) SDL_GameControllerClose(pad);
            SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        }
    };
    auto state = std::make_shared<GamepadState>();
    auto timer = new QTimer(owner);
    QObject::connect(timer, &QTimer::timeout, owner, [controller, state]() {
        auto window = QGuiApplication::focusWindow();
        const auto coreWindow = controller->players()->coreWindow()->window();
        SDL_GameControllerUpdate();
        if (state->pad && !SDL_GameControllerGetAttached(state->pad)) {
            SDL_GameControllerClose(state->pad);
            state->pad = nullptr;
        }
        if (!state->pad) {
            for (int i = 0; i < SDL_NumJoysticks(); ++i) {
                if (SDL_IsGameController(i)) {
                    state->pad = SDL_GameControllerOpen(i);
                    if (state->pad) break;
                }
            }
        }
        if (!state->pad) {
            state->held = 0;
            state->ticks = 0;
            return;
        }
        uint32_t buttons = 0;
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            if (SDL_GameControllerGetButton(state->pad, static_cast<SDL_GameControllerButton>(i)))
                buttons |= uint32_t(1) << i;
        }
        const uint32_t pressed = buttons & ~state->held;
        if (controller->players()->sessionActive() && window && window->isActive() &&
            window != coreWindow && (pressed & (uint32_t(1) << SDL_CONTROLLER_BUTTON_RIGHTSTICK)))
            controller->requestGameChromeToggle();
        if (!window || !window->isActive() || window == coreWindow ||
            controller->players()->sessionActive()) {
            state->held = buttons;
            state->ticks = 0;
            return;
        }
        const auto horizontal = SDL_GameControllerGetAxis(state->pad, SDL_CONTROLLER_AXIS_LEFTX);
        const auto vertical = SDL_GameControllerGetAxis(state->pad, SDL_CONTROLLER_AXIS_LEFTY);
        if (horizontal > 18000) buttons |= uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        if (horizontal < -18000) buttons |= uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        if (vertical > 18000) buttons |= uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        if (vertical < -18000) buttons |= uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_UP;
        state->ticks = buttons == state->held ? state->ticks + 1 : 0;
        const uint32_t directions = (uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_UP) |
                                    (uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_DOWN) |
                                    (uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_LEFT) |
                                    (uint32_t(1) << SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        const uint32_t repeat = state->ticks >= 12 && state->ticks % 4 == 0
                                    ? buttons & directions : 0;
        state->held = buttons;
        const struct { SDL_GameControllerButton button; int key; } mapping[] = {
            {SDL_CONTROLLER_BUTTON_A, Qt::Key_Return}, {SDL_CONTROLLER_BUTTON_B, Qt::Key_Escape},
            {SDL_CONTROLLER_BUTTON_Y, Qt::Key_F6}, {SDL_CONTROLLER_BUTTON_X, Qt::Key_F8},
            {SDL_CONTROLLER_BUTTON_DPAD_UP, Qt::Key_Up}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, Qt::Key_Down},
            {SDL_CONTROLLER_BUTTON_DPAD_LEFT, Qt::Key_Left}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Qt::Key_Right},
            {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, Qt::Key_PageUp},
            {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, Qt::Key_PageDown},
            {SDL_CONTROLLER_BUTTON_START, Qt::Key_Home}, {SDL_CONTROLLER_BUTTON_BACK, Qt::Key_Tab}
        };
        for (const auto& item : mapping) {
            if (!((pressed | repeat) & (uint32_t(1) << item.button))) continue;
            QKeyEvent down(QEvent::KeyPress, item.key, Qt::NoModifier);
            QKeyEvent up(QEvent::KeyRelease, item.key, Qt::NoModifier);
            down.setAccepted(false);
            QCoreApplication::sendEvent(window, &down);
            QCoreApplication::sendEvent(window, &up);
            if (!down.isAccepted() && item.key >= Qt::Key_Left && item.key <= Qt::Key_Down) {
                const auto modifiers = item.key == Qt::Key_Left || item.key == Qt::Key_Up
                                           ? Qt::ShiftModifier : Qt::NoModifier;
                QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, modifiers);
                QKeyEvent release(QEvent::KeyRelease, Qt::Key_Tab, modifiers);
                QCoreApplication::sendEvent(window, &tab);
                QCoreApplication::sendEvent(window, &release);
            }
        }
    });
    timer->start(33);
#else
    Q_UNUSED(controller)
    Q_UNUSED(owner)
#endif
}

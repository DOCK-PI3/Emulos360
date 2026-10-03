#include "core_window_host.h"
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

CoreWindowHost::CoreWindowHost(QObject* parent) : QObject(parent) {
    timer_.setInterval(100);
    connect(&timer_, &QTimer::timeout, this, &CoreWindowHost::discover);
}
CoreWindowHost::~CoreWindowHost() { clear(); }
void CoreWindowHost::clear() {
    timer_.stop(); pid_ = 0; nativeWindow_ = 0; embed_ = true;
    auto old = window_; window_ = nullptr;
    status_.clear(); emit changed();
    delete old;
}
void CoreWindowHost::watch(qint64 pid, bool embed) {
    clear(); pid_ = pid; embed_ = embed; attempts_ = 0;
#ifdef Q_OS_WIN
    status_ = tr("Preparando la ventana del motor…"); timer_.start(100);
#else
    status_ = tr("La integración de ventana está disponible en Windows. El motor se abre en una ventana independiente.");
#endif
    emit changed();
}
void CoreWindowHost::discover() {
#ifdef Q_OS_WIN
    struct Search { DWORD pid; bool embed; HWND hwnd = nullptr; } search{static_cast<DWORD>(pid_), embed_};
    EnumWindows([](HWND hwnd, LPARAM data) -> BOOL {
        auto* result = reinterpret_cast<Search*>(data);
        DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
        wchar_t className[128]{}, title[512]{};
        GetWindowTextW(hwnd, title, 512);
        if (pid == result->pid && !GetWindow(hwnd, GW_OWNER) && IsWindowVisible(hwnd) &&
            GetClassNameW(hwnd, className, 128) && (result->embed ? wcscmp(className, L"XeniaWindowClass") == 0 :
             QString::fromWCharArray(title).startsWith("Xenia-edge", Qt::CaseInsensitive))) {
            result->hwnd = hwnd; return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    if (search.hwnd) {
        nativeWindow_ = reinterpret_cast<quintptr>(search.hwnd);
        if (embed_) window_ = QWindow::fromWinId(reinterpret_cast<WId>(search.hwnd));
        timer_.stop();
        status_ = !embed_ ? tr("Canary Edge está abierto en su ventana nativa. Puedes controlar o cerrar la sesión desde aquí.") :
                          window_ ? tr("Motor integrado. Guarda la partida desde el juego antes de cerrar.")
                          : tr("No se pudo integrar la ventana. El motor sigue abierto por separado.");
        emit changed();
    } else if (++attempts_ == 600) {
        timer_.setInterval(1000);
        status_ = tr("Esperando la ventana del motor. Atiende sus avisos de inicio; se integrará cuando estén cerrados.");
        emit changed();
    }
#endif
}
void CoreWindowHost::focus() {
    if (!available()) return;
#ifdef Q_OS_WIN
    const auto child = reinterpret_cast<HWND>(nativeWindow_);
    DWORD processId = 0;
    const DWORD targetThread = GetWindowThreadProcessId(child, &processId);
    if (!targetThread || processId != static_cast<DWORD>(pid_) || !IsWindowVisible(child)) return;
    // requestActivate alone activates the Qt top-level, leaving keyboard and
    // XInput focus on its toolbar. A foreign child has a separate input queue.
    const HWND root = GetAncestor(child, GA_ROOT);
    SetForegroundWindow(root);
    const DWORD currentThread = GetCurrentThreadId();
    const bool attached = targetThread != currentThread &&
                          AttachThreadInput(currentThread, targetThread, TRUE);
    if (targetThread == currentThread || attached) {
        SetFocus(child);
        if (attached) AttachThreadInput(currentThread, targetThread, FALSE);
    }
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    const bool focused = GetGUIThreadInfo(targetThread, &info) &&
                         (info.hwndFocus == child || IsChild(child, info.hwndFocus));
    status_ = focused ? tr("Controlando el juego. Guarda la partida antes de cerrar.")
                      : tr("Pulsa dentro de la imagen del juego para darle el control.");
    emit changed();
#else
    window_->requestActivate();
#endif
}
void CoreWindowHost::requestClose() {
#ifdef Q_OS_WIN
    if (!available()) return;
    const auto hwnd = reinterpret_cast<HWND>(nativeWindow_);
    DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
    if (pid == static_cast<DWORD>(pid_)) PostMessageW(hwnd, WM_CLOSE, 0, 0);
#endif
}

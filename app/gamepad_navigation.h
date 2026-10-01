#pragma once
#include <QObject>
class Controller;
// Native Windows XInput-to-focus bridge; inactive when another app/core owns focus.
void installGamepadNavigation(Controller* controller, QObject* owner);

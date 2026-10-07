// The app defines this transient cvar in emulator_window.cc. CPU tests do not
// link that window; expose the same default through the real cvar registry.
#include "xenia/base/cvar.h"

DEFINE_transient_bool(emulos_embedded, false,
                      "Emulos360: let the Qt host own fullscreen state.",
                      "Emulos360");

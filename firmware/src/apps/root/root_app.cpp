#include <Arduino.h>

#include "coreback/coreback.h"

#include "root_app.h"
#include <apps/main/main_app.h>

namespace apps {
namespace root {

void root_app::init_core0() {
    // core0 is nominally the "primary" core, but we actually run primarily in core1,
    // leaving core0 to execute background tasks (via coreback).

    // As such, we actually have nothing to do here.
}

void root_app::init_core1() {
    Serial.begin();
    display.init();
    knob.init();

    // For now we unconditionally bring up the main app
    active_app = std::unique_ptr<base_app>(new main::main_app(display, knob));
    active_app->init();
}

void root_app::tick_core0() {
    // We claim ownership of core0, and only permit anything to run on it via
    // coreback.
    coreback::tick(/* blocking= */ true);
}

void root_app::tick_core1() {
    coreback::tick(/* blocking= */ false);
    knob.tick();
    if (active_app) {
        active_app->tick();
    }
}

}
}

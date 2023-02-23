#ifndef apps_root_root_app_h
#define apps_root_root_app_h

#include <optional>
#include <memory>
#include <hw/display.h>
#include <hw/knob.h>
#include "../base_app.h"
#include "../main/main_app.h"

namespace apps {
namespace root {

/**
 * root_app is what we boot into. It is responsible for configuring
 * basic hardware (display, buttons, knobs, etc.), and selecting
 * what will load next (either the config app or the main app).
*/
class root_app {
private:
    std::unique_ptr<base_app> active_app;
    hw::display display;
    hw::knob knob;

public:
    void init_core0();
    void init_core1();

    void tick_core0();
    void tick_core1();
};

}
}
#endif

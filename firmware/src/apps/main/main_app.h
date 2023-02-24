#ifndef apps_main_main_app_h
#define apps_main_main_app_h

#include <eightsleep/eightsleep.h>
#include <hw/display.h>
#include <hw/knob.h>
#include <scheduler/scheduler.h>
#include "../base_app.h"

namespace apps {
namespace main {

class main_app : public base_app {
private:
    bool ready = false;
    eightsleep::eightsleep client;
    hw::display& display;
    hw::knob& knob;
    scheduler::scheduler& scheduler;

    bool toning = false;

public:
    /**
     * main_app only holds on to references to the objects passed in here - the caller is
     * expected to outlive it.
     */
    main_app(hw::display& display, hw::knob& knob, scheduler::scheduler& scheduler) : display(display), knob(knob), scheduler(scheduler) {}

    void init();
    void tick();

private:
    void knob_pressed();
    void knob_rotated();
};

}
}

#endif

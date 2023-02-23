#include "apps/root/root_app.h"

apps::root::root_app app;

void setup() {
    app.init_core0();
}

void setup1() {    
    app.init_core1();
}

void loop() {
    app.tick_core0();
}

void loop1() {
    app.tick_core1();
}

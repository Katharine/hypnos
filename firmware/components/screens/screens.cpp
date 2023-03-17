#include "screens.h"

#include <esp_log.h>
#include <statics.h>

namespace screens {

void Stack::push(lv_obj_t *newScreen) {
    if (screens.empty()) {
        lv_scr_load(newScreen);
    } else {
        lv_scr_load_anim(newScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
    activateScreen(newScreen);
}

void Stack::replace(lv_obj_t *newScreen) {
    if (!screens.empty()) {
        destroyScreen(screens.back());
        screens.pop_back();
    }
    lv_scr_load_anim(newScreen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, true);
    activateScreen(newScreen);
}

void Stack::pop() {
    if (screens.size() == 1) {
        ESP_LOGW(TAG, "Asked to pop root window; ignoring.");
        return;
    }
    destroyScreen(screens.back());
    screens.pop_back();

    lv_obj_t *screen = screens.back();
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);
    activateScreen(screen);
    callCallbackForScreen(screen);
}

lv_obj_t *Stack::createScreen(Callback reappearCallback) {
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    groupMap[screen] = group;
    if (reappearCallback) {
        reappearCallbacks[screen] = std::move(reappearCallback);
    }
    return screen;
}

lv_group_t *Stack::groupForScreen(lv_obj_t *screen) {
    if (!groupMap.contains(screen)) {
        return nullptr;
    }
    return groupMap[screen];
}

void Stack::callCallbackForScreen(lv_obj_t *screen) {
    if (!reappearCallbacks.contains(screen)) {
        return;
    }
    reappearCallbacks[screen]();
}

void Stack::activateScreen(lv_obj_t *screen) {
    lv_group_t *group = groupForScreen(screen);
    if (group) {
        statics::statics.port->setActiveGroup(group);
    } else {
        ESP_LOGW(TAG, "Not activating group because we don't know what group we want");
    }
    screens.push_back(screen);
}

void Stack::destroyScreen(lv_obj_t *screen) {
    if (groupMap.contains(screen)) {
        lv_group_t *oldGroup = groupMap[screen];
        lv_group_del(oldGroup);
        groupMap.erase(screen);
    }
    if (reappearCallbacks.contains(screen)) {
        reappearCallbacks.erase(screen);
    }
}

}

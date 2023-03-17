#ifndef WINDOW_STACK_WINDOW_STACK_H
#define WINDOW_STACK_WINDOW_STACK_H

#include <vector>
#include <map>
#include <functional>
#include <lvgl.h>

namespace screens {

typedef std::function<void()> Callback;

class Stack {
    static constexpr const char* TAG = "WindowStack";
    std::vector<lv_obj_t*> screens{};
    std::map<lv_obj_t*, lv_group_t*> groupMap{};
    std::map<lv_obj_t*, Callback> reappearCallbacks{};

public:
    void push(lv_obj_t *newScreen);
    void replace(lv_obj_t *newScreen);
    void pop();
    lv_obj_t* createScreen(Callback reappearCallback = nullptr);
    lv_group_t *groupForScreen(lv_obj_t *screen);

private:
    void callCallbackForScreen(lv_obj_t *screen);
    void activateScreen(lv_obj_t *screen);
    void destroyScreen(lv_obj_t *screen);
};

}

#endif

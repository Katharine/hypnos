#ifndef COMPONENT_LVGL_PORT_LVGL_PORT_H
#define COMPONENT_LVGL_PORT_LVGL_PORT_H

#include <encoder.h>
#include <lvgl.h>
#include <esp_lcd_types.h>

namespace lvgl_port {

class LVGLPort {
    static constexpr char const * TAG = "LVGLPort";

    lv_indev_drv_t indev_drv;
    lv_indev_t *indev;
    encoder::Encoder encoder;

    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    lv_disp_t *disp;

    int last_brightness = -1;

    bool has_gone = false;

public:
    static const int DISPLAY_WIDTH = 240;
    static const int DISPLAY_HEIGHT = 240;

    LVGLPort();
    ~LVGLPort();
    void setActiveGroup(lv_group_t *group);
    void setBacklight(int level);
    static void resetDisplay();

private:
    void configureScreen();
    void configureEncoder();
    void configureBacklight();

    static void lvgl_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
};

class Lock {
public:
    Lock();
    ~Lock();
};

}

#endif
#include <encoder.h>
#include <lvgl.h>
#include <esp_lcd_types.h>

namespace lvgl_port {

class LVGLPort {
    const int DISPLAY_WIDTH = 240;
    const int DISPLAY_HEIGHT = 240;

    lv_indev_drv_t indev_drv;
    lv_indev_t *indev;
    encoder::Encoder encoder;

    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_panel_handle_t panel;
    lv_disp_t *disp;

    bool has_gone = false;

public:
    LVGLPort();
    LVGLPort(LVGLPort &&other) noexcept;
    ~LVGLPort();
    void setActiveGroup(lv_group_t *group);
    void setBacklight(int level);

private:
    void configureScreen();
    void configureEncoder();
    void configureBacklight();

    static void lvgl_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
};

}
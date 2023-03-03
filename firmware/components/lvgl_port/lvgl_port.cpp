#include "lvgl_port.h"

#include <esp_lvgl_port.h>
#include <driver/spi_common.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_gc9a01.h>

#include <utility>

namespace lvgl_port {

LVGLPort::LVGLPort() : encoder(CONFIG_PORT_ENCODER_A, CONFIG_PORT_ENCODER_B, CONFIG_PORT_ENCODER_BUTTON) {
    configureBacklight();
    configureScreen();
    configureEncoder();
}

LVGLPort::~LVGLPort() {
    if (indev) {
        lv_indev_delete(indev);
    }
    if (disp) {
        lvgl_port_remove_disp(disp);
    }
    if (!has_gone) {
        lv_deinit();
    }
}

void LVGLPort::configureEncoder() {
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.user_data = this;
    indev_drv.read_cb = &LVGLPort::lvgl_indev_read_cb;
    indev = lv_indev_drv_register(&indev_drv);
}

void LVGLPort::lvgl_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto *port = static_cast<LVGLPort*>(drv->user_data);
    static int last_reading = 0;
    int current_reading = port->encoder.position();
    data->enc_diff = static_cast<int16_t>(current_reading - last_reading);
    last_reading = current_reading;
    data->state = port->encoder.is_button_pressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void LVGLPort::configureScreen() {
    spi_bus_config_t bus_config{
        .mosi_io_num = CONFIG_PORT_DISPLAY_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_PORT_DISPLAY_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = static_cast<int>(DISPLAY_WIDTH * 80 * sizeof(uint16_t)),
        .flags = SPICOMMON_BUSFLAG_IOMUX_PINS,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config{
        .cs_gpio_num = CONFIG_PORT_DISPLAY_CS,
        .dc_gpio_num = CONFIG_PORT_DISPLAY_DC,
        .spi_mode = 0,
        .pclk_hz = CONFIG_PORT_DISPLAY_FREQ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = CONFIG_PORT_DISPLAY_RESET,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io, &panel_config, &panel));

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io,
        .panel_handle = panel,
        .buffer_size = static_cast<uint32_t>(DISPLAY_WIDTH * DISPLAY_HEIGHT),
        .double_buffer = true,
        .hres = static_cast<uint32_t>(DISPLAY_WIDTH),
        .vres = static_cast<uint32_t>(DISPLAY_HEIGHT),
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    disp = lvgl_port_add_disp(&disp_cfg);
}

void LVGLPort::configureBacklight() {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 8192,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .gpio_num = 9,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 1,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void LVGLPort::setBacklight(int level) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, level));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


void LVGLPort::setActiveGroup(lv_group_t *group) {
    lv_indev_set_group(indev, group);
}

//LVGLPort::LVGLPort(LVGLPort &&other) noexcept : indev_drv(other.indev_drv), indev(other.indev), encoder(std::move(other.encoder)), panel_io(other.panel_io), panel(other.panel), disp(other.disp)  {
//    other.disp = nullptr;
//    other.panel = nullptr;
//    other.panel_io = nullptr;
//    other.indev = nullptr;
//    other.indev_drv = {};
//    other.has_gone = true;
//}

}
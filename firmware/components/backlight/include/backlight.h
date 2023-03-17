#ifndef BACKLIGHT_BACKLIGHT_H
#define BACKLIGHT_BACKLIGHT_H

#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_continuous.h>

namespace backlight {

class Controller {
    static constexpr size_t ADC_MAX_BUFFER_SIZE = 4096;
    static constexpr size_t ADC_FRAME_SIZE = 1024;
    static constexpr adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_6;
    static constexpr char const *TAG = "Backlight";
    TaskHandle_t taskHandle = nullptr;
    adc_continuous_handle_t adcHandle = nullptr;
    adc_cali_handle_t caliHandle = nullptr;
    int8_t lastLevel = 0;

public:
    Controller();

private:
    void startTask();

    [[noreturn]] static void runTask(void *param);
    uint8_t rawReadings[ADC_FRAME_SIZE] = {0}; // this is half of our stack gone right here.
    void initADC();
    void updateBacklight(int raw, int voltage);
    static bool IRAM_ATTR convDone(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *ctx);
};

}

#endif
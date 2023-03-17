#include "backlight.h"
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_continuous.h>
#include <esp_log.h>
#include <statics.h>

namespace backlight {

Controller::Controller() {
    startTask();
    initADC();
}

void Controller::initADC() {
    adc_continuous_handle_cfg_t adcHandleConfig = {
        .max_store_buf_size = ADC_MAX_BUFFER_SIZE,
        .conv_frame_size = ADC_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adcHandleConfig, &adcHandle));

    adc_digi_pattern_config_t adc_pattern[1] = {{
        .atten = ADC_ATTENUATION,
        .channel = 5, // pin/gpio 6
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    }};
    adc_continuous_config_t adcConfig = {
        .pattern_num = 1,
        .adc_pattern = adc_pattern,
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_LOW,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2, // required for ESP32-S3
    };
    ESP_ERROR_CHECK(adc_continuous_config(adcHandle, &adcConfig));

    adc_cali_curve_fitting_config_t caliConfig = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_6,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&caliConfig, &caliHandle));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = &Controller::convDone,
    };

    adc_continuous_register_event_callbacks(adcHandle, &cbs, this);
    ESP_ERROR_CHECK(adc_continuous_start(adcHandle));
}

bool Controller::convDone(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *ctx) {
    auto *controller = static_cast<Controller*>(ctx);
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(controller->taskHandle, &mustYield);

    return mustYield;
}

void Controller::startTask() {
    ESP_LOGI(TAG, "Starting backlight control task");
    xTaskCreate(&Controller::runTask, "Backlight", 2048, this, 4, &taskHandle);
}

[[noreturn]] void Controller::runTask(void *param) {
    ESP_LOGI(TAG, "Starting backlight control running");
    auto *controller = static_cast<Controller*>(param);
    uint32_t resultCount = 0;
    while (true) {
        ulTaskNotifyTake(true, portMAX_DELAY);
        uint32_t total = 0;
        uint32_t count = 0;
        while (true) {
            esp_err_t ret = adc_continuous_read(controller->adcHandle, controller->rawReadings, ADC_FRAME_SIZE, &resultCount, 0);
            if (ret == ESP_ERR_TIMEOUT) {
                break;
            }
            ESP_ERROR_CHECK(ret);
            for (int i = 0; i < resultCount; i += SOC_ADC_DIGI_RESULT_BYTES) {
                auto *p = static_cast<adc_digi_output_data_t *>((void *) &controller->rawReadings[i]);
                total += p->type2.data;
                count += 1;
            }
            int millivolts = 0;
            uint32_t average = total / (count > 0 ? count : 1);
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(controller->caliHandle, average, &millivolts));
            controller->updateBacklight(static_cast<int>(average), millivolts);
        }
    }
    __unreachable();
}

void Controller::updateBacklight(int raw, int millivolts) {
    if ((lastLevel >= 4 && raw >= 4000) || raw >= 4095) { // Maxed out
        lastLevel = 4;
        statics::statics.port->setBacklight(4096); // 100% brightness
    } else if ((lastLevel >= 3 && millivolts >= 1400) || millivolts >= 1500) {
        lastLevel = 3;
        statics::statics.port->setBacklight(2048); // 50% brightness
    } else if ((lastLevel >= 2 && millivolts >= 700) || millivolts >= 800) {
        lastLevel = 2;
        statics::statics.port->setBacklight(1024); // 50% brightness
    } else if ((lastLevel >= 1 && millivolts >= 7) || millivolts >= 30) {
        lastLevel = 1;
        statics::statics.port->setBacklight(410); // 10% brightness
    } else {
        lastLevel = 0;
        statics::statics.port->setBacklight(3); // 0.07% brightness
    }
}

}
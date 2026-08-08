#include "app.h"
#include "app_config.h"
#include "air_mouse_platform.h"
#include "mouse_output_if.h"
#include "motion_service.h"
#include "logger.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app";

void app_start(void)
{
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    if (!APP_QUIET_STARTUP) ESP_LOGI(TAG, "starting ESP32 Air Mouse");
    for (int attempt = 1; attempt <= 5; ++attempt) {
        if (air_mouse_sensor_init()) break;
        if (!APP_QUIET_STARTUP) ESP_LOGW(TAG, "MPU6050 init attempt %d failed", attempt);
        if (attempt == 5) {
            if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "MPU6050 unavailable; check 3.3V, GND, pull-ups, SDA=GPIO4, SCL=GPIO5");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!mouse_output_init()) {
        if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "BLE mouse initialization failed");
        return;
    }
    motion_service_start();
}

#include "air_mouse_platform.h"
#include "mouse_output_if.h"
#include "app_config.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "motion";

static int8_t clamp_delta(float value)
{
    if (value > APP_MOUSE_MAX_DELTA) return APP_MOUSE_MAX_DELTA;
    if (value < -APP_MOUSE_MAX_DELTA) return -APP_MOUSE_MAX_DELTA;
    return (int8_t)value;
}

static void motion_task(void *arg)
{
    air_mouse_sample_t sample;
    float gyro_x_zero = 0.0f;
    float gyro_y_zero = 0.0f;
    for (int i = 0; i < 40; ++i) {
        if (air_mouse_sensor_read(&sample)) {
            gyro_x_zero += sample.gyro_x;
            gyro_y_zero += sample.gyro_y;
        }
        vTaskDelay(pdMS_TO_TICKS(APP_SAMPLE_PERIOD_MS));
    }
    gyro_x_zero /= 40.0f;
    gyro_y_zero /= 40.0f;
    ESP_LOGI(TAG, "stationary calibration complete");
    while (true) {
        if (air_mouse_sensor_read(&sample) && mouse_output_connected()) {
            float dx = (sample.gyro_y - gyro_y_zero) / 131.0f * APP_MOUSE_SCALE;
            float dy = (sample.gyro_x - gyro_x_zero) / 131.0f * APP_MOUSE_SCALE;
            if (fabsf(dx) < APP_MOUSE_DEADZONE) dx = 0;
            if (fabsf(dy) < APP_MOUSE_DEADZONE) dy = 0;
            if (dx != 0 || dy != 0) mouse_output_send(clamp_delta(dx), clamp_delta(dy));
        }
        vTaskDelay(pdMS_TO_TICKS(APP_SAMPLE_PERIOD_MS));
    }
}

void motion_service_start(void)
{
    xTaskCreate(motion_task, "motion", 4096, NULL, 5, NULL);
}

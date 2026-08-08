#include "air_mouse_platform.h"
#include "app_config.h"
#include "logger.h"
#include "driver/i2c_master.h"
#include <string.h>

static const char *TAG = "mpu6050";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;
static bool bus_ready;
static bool device_ready;

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return i2c_master_transmit(dev, data, sizeof(data), 100);
}

static esp_err_t read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
}

bool air_mouse_sensor_init(void)
{
    if (device_ready) return true;

    if (!bus_ready) {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = APP_I2C_SDA_GPIO,
            .scl_io_num = APP_I2C_SCL_GPIO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
        if (err != ESP_OK) {
            if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(err));
            return false;
        }
        bus_ready = true;
    }

    if (dev == NULL) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = APP_MPU6050_ADDRESS,
            .scl_speed_hz = APP_I2C_FREQ_HZ,
        };
        esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
        if (err != ESP_OK) {
            if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "MPU6050 device add failed: %s", esp_err_to_name(err));
            return false;
        }
    }

    uint8_t who = 0;
    esp_err_t err = read_regs(APP_MPU6050_WHO_AM_I, &who, 1);
    if (err != ESP_OK || (who & 0x7e) != 0x68) {
        if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "MPU6050 not detected, WHO_AM_I=0x%02x", who);
        return false;
    }
    if (write_reg(0x6b, 0x01) != ESP_OK || write_reg(0x1b, 0x00) != ESP_OK ||
        write_reg(0x1c, 0x00) != ESP_OK || write_reg(0x1a, 0x03) != ESP_OK) {
        if (!APP_QUIET_STARTUP) ESP_LOGE(TAG, "MPU6050 configuration failed");
        return false;
    }
    device_ready = true;
    ESP_LOGI(TAG, "MPU6050 ready at 0x%02x on SDA=%d SCL=%d", APP_MPU6050_ADDRESS, APP_I2C_SDA_GPIO, APP_I2C_SCL_GPIO);
    return true;
}

bool air_mouse_sensor_read(air_mouse_sample_t *sample)
{
    uint8_t raw[14];
    if (sample == NULL || !device_ready || read_regs(0x3b, raw, sizeof(raw)) != ESP_OK) return false;
    sample->accel_x = (int16_t)((raw[0] << 8) | raw[1]);
    sample->accel_y = (int16_t)((raw[2] << 8) | raw[3]);
    sample->accel_z = (int16_t)((raw[4] << 8) | raw[5]);
    sample->gyro_x = (int16_t)((raw[8] << 8) | raw[9]);
    sample->gyro_y = (int16_t)((raw[10] << 8) | raw[11]);
    sample->gyro_z = (int16_t)((raw[12] << 8) | raw[13]);
    return true;
}

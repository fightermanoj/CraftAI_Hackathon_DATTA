#ifndef AIR_MOUSE_PLATFORM_H
#define AIR_MOUSE_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} air_mouse_sample_t;

bool air_mouse_sensor_init(void);
bool air_mouse_sensor_read(air_mouse_sample_t *sample);
void air_mouse_hid_init(void);
bool air_mouse_hid_connected(void);
void air_mouse_hid_send(int8_t dx, int8_t dy);

#endif

#ifndef MOTION_SENSOR_IF_H
#define MOTION_SENSOR_IF_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
} motion_sample_t;

bool motion_sensor_init(void);
bool motion_sensor_read(motion_sample_t *sample);

#endif

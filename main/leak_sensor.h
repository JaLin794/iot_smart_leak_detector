#ifndef LEAK_SENSOR_H
#define LEAK_SENSOR_H

#include <stdbool.h>

void liquid_level_sensor_init(void);
int liquid_level_sensor_read(void);
int liquid_level_sensor_voltage (int sensor_value);
bool leak_detection(int voltage, int leak_threshold, int flush_threshold);
bool full_tank_detection(int voltage, int leak_threshhold);
bool flush_detection(int voltage, int flush_threshold);

#endif // LEAK_SENSOR_H
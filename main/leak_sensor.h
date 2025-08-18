#ifndef LEAK_SENSOR_H
#define LEAK_SENSOR_H

void liquid_level_sensor_init(void);
int liquid_level_sensor_read(void);
float liquid_level_sensor_voltage (int sensor_value);

#endif // LEAK_SENSOR_H
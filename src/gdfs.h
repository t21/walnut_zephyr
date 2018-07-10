/**
 *
 *
 */

#ifndef GDFS_H
#define GDFS_H

typedef enum {
    GDFS_DEVICE_DATA,
    GDFS_SENSOR_TEMPERATURE,
    GDFS_SENSOR_HUMIDITY,
    GDFS_SENSOR_ALS,
    GDFS_SENSOR_BARO_PRESSURE,
} gdfs_types_t;

typedef struct {
    uint32_t meas_interval;
} gdfs_sensor_data_t;

typedef struct {
    uint32_t adv_interval;
} gdfs_device_data_t;

int gdfs_init(void);
int gdfs_get_device_data(gdfs_device_data_t *data);
int gdfs_set_device_data(const gdfs_device_data_t *data);
int gdfs_get_sensor_data(gdfs_types_t sensor, gdfs_sensor_data_t *data);
int gdfs_set_sensor_data(gdfs_types_t sensor, const gdfs_sensor_data_t *data);

#endif /* GDFS_H */

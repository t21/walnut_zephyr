/**
 *
 *
 */

#ifndef _NV_H_
#define _NV_H_

/****************************************************************************
* Include Directives
***************************************************************************/
#include <zephyr/types.h>

/****************************************************************************
* Preprocessor Directives
***************************************************************************/

/****************************************************************************
* Public Type Declarations
***************************************************************************/

typedef enum {
    NV_DEVICE_DATA,
    NV_SENSOR_TEMPERATURE,
    NV_SENSOR_HUMIDITY,
    NV_SENSOR_AMBIENT_LIGHT,
    NV_SENSOR_BARO_PRESSURE,
} nv_types_t;

typedef struct {
    u8_t sampling_func;
    u32_t meas_period;
    u32_t update_interval;
    u8_t application;
    u8_t meas_uncertainty;
} nv_sensor_data_t;

typedef struct {
    u32_t adv_interval;
} nv_device_data_t;


/****************************************************************************
* Public Function Declarations
***************************************************************************/

int nv_init(void);
int nv_get_device_data(nv_device_data_t *data);
int nv_set_device_data(const nv_device_data_t *data);
int nv_get_sensor_data(nv_types_t sensor, nv_sensor_data_t *data);
int nv_set_sensor_data(nv_types_t sensor, const nv_sensor_data_t *data);

#endif /* _NV_H_ */

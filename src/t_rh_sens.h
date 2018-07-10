/**
 *
 *
 */

#ifndef T_RH_SENS_H
#define T_RH_SENS_H

#include <stdint.h>
#include <sensor.h>

typedef struct {
    struct sensor_value *temperature;
    struct sensor_value *humidity;
    bool temperature_updated;
    bool humidity_updated;
} t_rh_meas_t;

typedef void (*t_rh_meas_cb_t)(t_rh_meas_t *measurement);

void t_rh_sens_init(t_rh_meas_cb_t callback);
void t_rh_sens_meas(void);

#endif /* T_RH_SENS_H */

/**
 *
 *
 */

#ifndef BP_SENS_H
#define BP_SENS_H

typedef void (*bp_meas_cb_t)(struct sensor_value *baro_pressure);

void bp_sens_init(bp_meas_cb_t callback);
void bp_sens_meas(void);

#endif /* BP_SENS_H */

/**
 *
 *
 */

#ifndef ALS_H
#define ALS_H

typedef void (*als_meas_cb_t)(struct sensor_value *ambient_light);

void als_init(als_meas_cb_t callback);
int als_meas(void);

#endif /* ALS_H */

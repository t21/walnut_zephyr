/** @file
 *  @brief Environmental Sensor Service
 */

#ifndef ESS_H
#define ESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void ess_init(void);
void ess_temperature_update(int16_t temperature);
void ess_humidity_update(int16_t humidity);
void ess_als_update(int16_t sensor_val);
void ess_baro_press_update(uint32_t sensor_val);

#ifdef __cplusplus
}
#endif

#endif /* ESS_H */
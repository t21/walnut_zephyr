/** @file
 *  @brief Environmental Sensor Service
 */

#ifndef ESS_H
#define ESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ESS_MEAS_PERIOD_NOT_IN_USE              0
#define ESS_INTERNAL_UPDATE_INTERVAL_NOT_IN_USE 0

typedef enum {
    ESS_SAMPL_FUNC_UNSPECIFIED,
    ESS_SAMPL_FUNC_INSTANTANEOUS,
    ESS_SAMPL_FUNC_ARITHMETIC_MEAN,
    ESS_SAMPL_FUNC_RMS,
    ESS_SAMPL_FUNC_MAXIMUM,
    ESS_SAMPL_FUNC_MINIMUM,
    ESS_SAMPL_FUNC_ACCUMULATED,
    ESS_SAMPL_FUNC_COUNT,
} ess_sampl_func_t;

typedef enum {
    ESS_APPL_Unspecified,
    ESS_APPL_Air,
    ESS_APPL_Water,
    ESS_APPL_Barometric,
    ESS_APPL_Soil,
    ESS_APPL_Infrared,
    ESS_APPL_Map_Database,
    ESS_APPL_Barometric_Elevation_Source,
    ESS_APPL_GPS_only_Elevation_Source,
    ESS_APPL_GPS_and_Map_database_Elevation_Source,
    ESS_APPL_Vertical_datum_Elevation_Source,
    ESS_APPL_Onshore,
    ESS_APPL_Onboard_vessel_or_vehicle,
    ESS_APPL_Front,
    ESS_APPL_Back_Rear,
    ESS_APPL_Upper,
    ESS_APPL_Lower,
    ESS_APPL_Primary,
    ESS_APPL_Secondary,
    ESS_APPL_Outdoor,
    ESS_APPL_Indoor,
    ESS_APPL_Top,
    ESS_APPL_Bottom,
    ESS_APPL_Main,
    ESS_APPL_Backup,
    ESS_APPL_Auxiliary,
    ESS_APPL_Supplementary,
    ESS_APPL_Inside,
    ESS_APPL_Outside,
    ESS_APPL_Left,
    ESS_APPL_Right,
    ESS_APPL_Internal,
    ESS_APPL_External,
    ESS_APPL_Solar,
} ess_appl_t;


void ess_init(void);
void ess_temperature_update(int16_t temperature);
void ess_humidity_update(int16_t humidity);
void ess_als_update(int16_t sensor_val);
void ess_baro_press_update(uint32_t sensor_val);

#ifdef __cplusplus
}
#endif

#endif /* ESS_H */
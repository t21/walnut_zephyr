/**
 *
 *
 */
#ifndef BLE_H
#define BLE_H

void ble_init(void);

void ble_update_temp(double temperature);
void ble_update_humidity(double humidity);
void ble_update_ambient_light(double ambient_light);
void ble_update_baro_pressure(double pressure);
void ble_update_battery(uint8_t battery_capacity);

#endif /* BLE_H */
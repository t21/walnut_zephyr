/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr.h>
#include <sensor.h>

#include "fg.h"
#include "gdfs.h"
#include "ble.h"
#include "ess.h"
#include "t_rh_sens.h"
#include "als.h"
#include "bp_sens.h"

#define CONFIG_SYS_LOG_MAIN_LEVEL 4

#define SYS_LOG_DOMAIN "app"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_MAIN_LEVEL
#include <logging/sys_log.h>


static void fg_update_cb(uint8_t battery_capacity)
{
    SYS_LOG_INF("Battery_capacity=%d", battery_capacity);
    ble_update_battery(battery_capacity);
}

static void t_rh_meas_cb(t_rh_meas_t *measurement)
{
    if (measurement->temperature_updated) {
        struct sensor_value *temperature = (struct sensor_value *)measurement->temperature;
        SYS_LOG_INF("T:%d.%06d", temperature->val1, temperature->val2);
        ble_update_temp(sensor_value_to_double(temperature));
    }

    if (measurement->humidity_updated) {
        struct sensor_value *humidity = (struct sensor_value *)measurement->humidity;
        SYS_LOG_INF("RH:%d.%06d", humidity->val1, humidity->val2);
        ble_update_humidity(sensor_value_to_double(humidity));
    }
}

void main(void)
{
    SYS_LOG_INF("Starting app");

    fg_init(fg_update_cb);
    gdfs_init();
    t_rh_sens_init(t_rh_meas_cb);
    als_init();
    bp_sens_init();
    ble_init();

    while (1) {
        k_sleep(10 * MSEC_PER_SEC);
    }
}

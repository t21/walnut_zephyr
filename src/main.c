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
#include "nv.h"
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

static void al_meas_cb(struct sensor_value *ambient_light)
{
    SYS_LOG_INF("AL:%d.%06d", ambient_light->val1, ambient_light->val2);
    ble_update_ambient_light(sensor_value_to_double(ambient_light));
}

static void bp_meas_cb(struct sensor_value *baro_pressure)
{
    SYS_LOG_INF("AL:%d.%06d", baro_pressure->val1, baro_pressure->val2);
    ble_update_baro_pressure(sensor_value_to_double(baro_pressure));
}


void main(void)
{
    SYS_LOG_INF("Starting app");

    fg_init(fg_update_cb);
    nv_init();
    t_rh_sens_init(t_rh_meas_cb);
    als_init(al_meas_cb);
    bp_sens_init(bp_meas_cb);
    ble_init();

    while (1) {
        k_sleep(10 * MSEC_PER_SEC);
    }
}

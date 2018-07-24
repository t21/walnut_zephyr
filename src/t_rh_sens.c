/**
 *
 *
 *
 */

// #include <stdbool.h>
// #include <zephyr/types.h>
// #include <stddef.h>
// #include <string.h>
// #include <errno.h>
// #include <misc/printk.h>
// #include <misc/byteorder.h>
// #include <zephyr.h>
#include <kernel.h>
#include <sensor.h>

#include "t_rh_sens.h"
#include "ble.h"
#include "nv.h"
#include "ess.h"

#define CONFIG_SYS_LOG_T_RH_SENSOR_LEVEL 1

#define SYS_LOG_DOMAIN "t_rh_sens"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_T_RH_SENSOR_LEVEL
#include <logging/sys_log.h>


static struct device *si7020_dev;
static struct sensor_value rh_val;
static struct sensor_value t_val;
static struct k_timer meas_timer;
static struct k_work meas_work;
static t_rh_meas_cb_t meas_cb;


static nv_sensor_data_t default_sensor_data = {
    .sampling_func    = ESS_SAMPL_FUNC_INSTANTANEOUS,
    .meas_period      = ESS_MEAS_PERIOD_NOT_IN_USE,
    .update_interval  = 60,
    .application      = ESS_APPL_Air,
    .meas_uncertainty = 2,
};

static void meas_work_handler(struct k_work *work);
static void meas_timer_handler(struct k_timer *timer);


static void meas_work_handler(struct k_work *work)
{
    SYS_LOG_DBG("Periodic t and rh measurement");
    t_rh_sens_meas();

    if (meas_cb != NULL) {
        t_rh_meas_t meas;
        meas.temperature = &t_val;
        meas.humidity = &rh_val;
        meas.temperature_updated = true;
        meas.humidity_updated = true;
        meas_cb(&meas);
    }
}

static void meas_timer_handler(struct k_timer *timer)
{
    k_work_submit(&meas_work);
}

void t_rh_sens_meas(void)
{
    sensor_sample_fetch(si7020_dev);

    if (sensor_channel_get(si7020_dev, SENSOR_CHAN_HUMIDITY, &rh_val)) {
        SYS_LOG_ERR("Error reading si7020 data");
    } else {
        SYS_LOG_INF("RH:%d.%06d", rh_val.val1, rh_val.val2);
    }

    if (sensor_channel_get(si7020_dev, SENSOR_CHAN_AMBIENT_TEMP, &t_val)) {
        SYS_LOG_ERR("Error reading si7020 data");
    } else {
        SYS_LOG_INF("T:%d.%06d", t_val.val1, t_val.val2);
    }
}

void t_rh_sens_init(t_rh_meas_cb_t callback)
{
    int err = 0;
    nv_sensor_data_t rh_sensor_data;
    nv_sensor_data_t t_sensor_data;

    meas_cb = callback;

    si7020_dev = device_get_binding(CONFIG_SI7020_NAME);
    if (si7020_dev == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!", CONFIG_SI7020_NAME);
        return;
    }

    // Needs to make one initial measurement for the device to enter sleep
    sensor_sample_fetch(si7020_dev);

    err = nv_get_sensor_data(NV_SENSOR_HUMIDITY, &rh_sensor_data);
    if (err == -ENOENT) {
        nv_set_sensor_data(NV_SENSOR_HUMIDITY, &default_sensor_data);
        nv_get_sensor_data(NV_SENSOR_HUMIDITY, &rh_sensor_data);
    }

    err = nv_get_sensor_data(NV_SENSOR_TEMPERATURE, &t_sensor_data);
    if (err == -ENOENT) {
        nv_set_sensor_data(NV_SENSOR_TEMPERATURE, &default_sensor_data);
        nv_get_sensor_data(NV_SENSOR_TEMPERATURE, &t_sensor_data);
    }

    if (rh_sensor_data.update_interval == t_sensor_data.update_interval) {

    }

    k_work_init(&meas_work, meas_work_handler);
    k_timer_init(&meas_timer, meas_timer_handler, NULL);
    k_timer_start(&meas_timer, K_SECONDS(5), K_SECONDS(rh_sensor_data.update_interval));
}

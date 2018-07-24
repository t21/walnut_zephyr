/**
 *
 *
 *
 */

#include <stdlib.h>
#include <kernel.h>
#include <sensor.h>

#include "bp_sens.h"
#include "nv.h"
#include "ess.h"

#define CONFIG_SYS_LOG_BP_SENS_LEVEL 1
#define SYS_LOG_DOMAIN "bp_sens"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_BP_SENS_LEVEL
#include <logging/sys_log.h>


static struct device *bmp280_dev;
static struct sensor_value bp_val;
static struct k_timer meas_timer;
static struct k_work meas_work;
static bp_meas_cb_t meas_cb;

static nv_sensor_data_t _default_sensor_data = {
    .sampling_func    = ESS_SAMPL_FUNC_INSTANTANEOUS,
    .meas_period      = ESS_MEAS_PERIOD_NOT_IN_USE,
    .update_interval  = 60,
    .application      = ESS_APPL_Air,
    .meas_uncertainty = 0,
};

static void meas_work_handler(struct k_work *work);
static void meas_timer_handler(struct k_timer *timer);


static void meas_work_handler(struct k_work *work)
{
    SYS_LOG_DBG("Periodic barometric pressure measurement");
    bp_sens_meas();

    if (meas_cb != NULL) {
        meas_cb(&bp_val);
    }

}

static void meas_timer_handler(struct k_timer *timer)
{
    k_work_submit(&meas_work);
}

void bp_sens_meas(void)
{
    int err;

    err = sensor_sample_fetch(bmp280_dev);
    if (err != 0) {
        SYS_LOG_ERR("Error fetching barometric pressure sample");
        return;
    }

    err = sensor_channel_get(bmp280_dev, SENSOR_CHAN_PRESS, &bp_val);
    if (err != 0) {
        SYS_LOG_ERR("Error reading bmp280 data");
    } else {
        SYS_LOG_INF("BP:%d.%06d", bp_val.val1, bp_val.val2);
    }
}

void bp_sens_init(bp_meas_cb_t callback)
{
#ifdef CONFIG_BMP280
    int err;
    nv_sensor_data_t sensor_data;

    meas_cb = callback;

    bmp280_dev = device_get_binding(CONFIG_BMP280_DEV_NAME);
    if (bmp280_dev == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!", CONFIG_BMP280_DEV_NAME);
    }

    err = nv_get_sensor_data(NV_SENSOR_BARO_PRESSURE, &sensor_data);
    if (err == -ENOENT) {
        nv_set_sensor_data(NV_SENSOR_BARO_PRESSURE, &_default_sensor_data);
        nv_get_sensor_data(NV_SENSOR_BARO_PRESSURE, &sensor_data);
    }

    k_work_init(&meas_work, meas_work_handler);
    k_timer_init(&meas_timer, meas_timer_handler, NULL);
    k_timer_start(&meas_timer, K_SECONDS(5), K_SECONDS(sensor_data.update_interval));
#endif
}

/**
 *
 *
 *
 */

#include <stdlib.h>
#include <kernel.h>
#include <sensor.h>

#include "als.h"
#include "nv.h"
#include "ess.h"

#define CONFIG_SYS_LOG_ALS_LEVEL 1
#define SYS_LOG_DOMAIN "als"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ALS_LEVEL
#include <logging/sys_log.h>


static struct device *tsl4531_dev;
static struct sensor_value als_val;
static struct k_timer meas_timer;
static struct k_work meas_work;
static als_meas_cb_t meas_cb;

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
    SYS_LOG_DBG("Periodic als measurement");
    als_meas();

    if (meas_cb != NULL) {
        meas_cb(&als_val);
    }
}

static void meas_timer_handler(struct k_timer *timer)
{
    k_work_submit(&meas_work);
}

int als_meas(void)
{
#ifdef CONFIG_TSL4531
    if (sensor_sample_fetch(tsl4531_dev)) {
        SYS_LOG_ERR("Error fetching tsl4531 sample");
        return -1;
    }

    if (sensor_channel_get(tsl4531_dev, SENSOR_CHAN_LIGHT, &als_val)) {
        SYS_LOG_ERR("Error reading tsl4531 data");
        return -1;
    } else {
        // SYS_LOG_DBG("AL:%d.%d", (int)sensor_value_to_double(&als_val),
        //             (int)(sensor_value_to_double(&als_val) * 100) % 100);
        // SYS_LOG_DBG("RH1:%d", rh_sensor_value.val1);
        // SYS_LOG_DBG("RH2:%d", rh_sensor_value.val2);
        SYS_LOG_INF("ALS:%d.%06d", als_val.val1, als_val.val2);
    }
#endif
    return 0;
}

void als_init(als_meas_cb_t callback)
{
#ifdef CONFIG_TSL4531
    int err;
    nv_sensor_data_t sensor_data;

    meas_cb = callback;

    tsl4531_dev = device_get_binding(CONFIG_TSL4531_NAME);
    if (tsl4531_dev == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!", CONFIG_TSL4531_NAME);
    }

    err = nv_get_sensor_data(NV_SENSOR_AMBIENT_LIGHT, &sensor_data);
    if (err == -ENOENT) {
        nv_set_sensor_data(NV_SENSOR_AMBIENT_LIGHT, &_default_sensor_data);
        nv_get_sensor_data(NV_SENSOR_AMBIENT_LIGHT, &sensor_data);
    }

    k_work_init(&meas_work, meas_work_handler);
    k_timer_init(&meas_timer, meas_timer_handler, NULL);
    k_timer_start(&meas_timer, K_SECONDS(5), K_SECONDS(sensor_data.update_interval));
#endif
}

/**
 *
 *
 *
 */

#include <kernel.h>
#include <sensor.h>

#include "bp_sens.h"
#include "ble.h"

#define CONFIG_SYS_LOG_BP_SENS_LEVEL 3

#define SYS_LOG_DOMAIN "bp_sens"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_BP_SENS_LEVEL
#include <logging/sys_log.h>


static struct device *bmp280_dev;
static struct sensor_value bp_val;
static struct k_timer meas_timer;
static struct k_work meas_work;

static void meas_work_handler(struct k_work *work);
static void meas_timer_handler(struct k_timer *timer);


static void meas_work_handler(struct k_work *work)
{
    SYS_LOG_DBG("Periodic barometric pressure measurement");
    bp_sens_meas();
    ble_update_baro_pressure(sensor_value_to_double(&bp_val));
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

void bp_sens_init(void)
{
#ifdef CONFIG_BME280
    bmp280_dev = device_get_binding(CONFIG_BME280_DEV_NAME);
    if (bmp280_dev == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!", CONFIG_BME280_DEV_NAME);
    }

    k_work_init(&meas_work, meas_work_handler);
    k_timer_init(&meas_timer, meas_timer_handler, NULL);
    k_timer_start(&meas_timer, K_SECONDS(1), K_SECONDS(60));
#endif
}

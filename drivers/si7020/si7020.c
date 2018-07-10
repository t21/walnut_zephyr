/*
 * Copyright (c) 2018 Thomas Berg
 *
 */

#include <kernel.h>
#include <device.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "si7020.h"
#include "../i2c_wrap/i2c_wrap.h"

#define SYS_LOG_DOMAIN "si7020"
// #define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#define SYS_LOG_LEVEL 1
#include <logging/sys_log.h>


#define SI7020_I2C_ADDR    0x40

/* Commands */
#define CMD_MEASURE_HUMIDITY_HOLD       0xE5
#define CMD_MEASURE_HUMIDITY_NO_HOLD    0xF5
#define CMD_MEASURE_TEMPERATURE_HOLD    0xE3
#define CMD_MEASURE_TEMPERATURE_NO_HOLD 0xF3
#define CMD_READ_PREVIOUS_TEMPERATURE   0xE0
#define CMD_RESET                       0xFE
#define CMD_WRITE_REGISTER_1            0xE6
#define CMD_READ_REGISTER_1             0xE7
#define CMD_WRITE_HEATER_CONTROL        0x51
#define CMD_READ_HEATER_CONTROL         0x11
#define CMD_READ_ELECTRONIC_ID_BYTE1    0xFA 0x0F
#define CMD_READ_ELECTRONIC_ID_BYTE2    0xFC 0xC9
#define CMD_READ_FIRMWARE_REV           0x84 0xB8

/* User Register 1 */
#define REG1_RESOLUTION_MASK		0x81
#define REG1_RESOLUTION_H12_T14 	0x00
#define REG1_RESOLUTION_H08_T12 	0x01
#define REG1_RESOLUTION_H10_T13 	0x80
#define REG1_RESOLUTION_H11_T11 	0x81
#define REG1_LOW_VOLTAGE		0x40
#define REG1_ENABLE_HEATER		0x04

/* Device Identification */
#define SI7020_ID           0x14


static int reset(struct device *dev)
{
    u8_t buf = CMD_RESET;

    if (i2c_write_wrap(dev, &buf, 1, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    k_sleep(80);

    return 0;
}

static int check_id(struct device *dev)
{
    u8_t buf[6] = { 0xFC, 0xC9 };

    if (i2c_write_wrap(dev, buf, 2, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    if (i2c_read_wrap(dev, buf, 6, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("I2C read failed!");
        return -1;
    }

    if (buf[0] != SI7020_ID) {
        SYS_LOG_ERR("Error: Si7020 chip id does not match");
        return -1;
    }

    return 0;
}

static int set_resolution(struct device *dev)
{
    u8_t buf[2] = { CMD_WRITE_REGISTER_1, REG1_RESOLUTION_H12_T14 };

    if (i2c_write_wrap(dev, buf, 2, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    return 0;
}

static u16_t get_humi(struct device *dev)
{
    u16_t humidity = 0;
    u8_t buf[2] = { CMD_MEASURE_HUMIDITY_NO_HOLD, 0 };

    if (i2c_write_wrap(dev, buf, 1, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    k_sleep(25);

    if (i2c_read_wrap(dev, buf, 2, SI7020_I2C_ADDR)) {
        SYS_LOG_ERR("Failed to read humidity!");
        return -1;
    }

    humidity = (buf[0] << 8) | (buf[1] & 0xFC);

    return humidity;
}

static u16_t get_temp(struct device *dev)
{
    u16_t temperature = 0;
    u8_t buf[2] = { 0 };

    if (i2c_burst_read_wrap(dev, SI7020_I2C_ADDR, CMD_READ_PREVIOUS_TEMPERATURE, buf, 2))
    {
        SYS_LOG_ERR("Failed to read temperature!");
        return 0;
    }

    temperature = (buf[0] << 8) | (buf[1] & 0xFC);

    return temperature;
}

static int si7020_sample_fetch(struct device *dev, enum sensor_channel chan)
{
    struct si7020_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

    drv_data->rh_sample = get_humi(drv_data->i2c_wrap);
    SYS_LOG_INF("rh: %u", drv_data->rh_sample);
    drv_data->t_sample = get_temp(drv_data->i2c_wrap);
    SYS_LOG_INF("temp: %u", drv_data->t_sample);

    return 0;
}

static int si7020_channel_get(struct device *dev, enum sensor_channel chan,
                struct sensor_value *val)
{
    struct si7020_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP ||
            chan == SENSOR_CHAN_HUMIDITY);

    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        /* val = sample * 175.72 / 65536 - 46.85 */
        val->val1 = (drv_data->t_sample * 17572 / 65536 - 4685) / 100;
        val->val2 = (drv_data->t_sample * 17572 / 65536 - 4685) % 100 * 10000;
    } else {
        /* val =  sample * 125 / 65536 - 6 */
        val->val1 = (drv_data->rh_sample * 12500 / 65536 - 600) / 100;
        val->val2 = (drv_data->rh_sample * 12500 / 65536 - 600) % 100 * 10000;
    }

    return 0;
}

static const struct sensor_driver_api si7020_driver_api = {
    .sample_fetch = si7020_sample_fetch,
    .channel_get = si7020_channel_get,
};

static int si7020_init(struct device *dev)
{
    struct si7020_data *drv_data = dev->driver_data;

    SYS_LOG_INF("Init Si7020");

    // drv_data->i2c = device_get_binding(CONFIG_SI7020_I2C_MASTER_DEV_NAME);
    // if (drv_data->i2c == NULL) {
    //     SYS_LOG_ERR("Failed to get pointer to %s device!",
    //             CONFIG_SI7020_I2C_MASTER_DEV_NAME);
    //     return -EINVAL;
    // }

    drv_data->i2c_wrap = device_get_binding(CONFIG_I2C_WRAP_NAME);
    if (drv_data->i2c_wrap == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!",
                CONFIG_I2C_WRAP_NAME);
        return -EINVAL;
    }

    // struct device *als_pow_dev = (struct device *)drv_data->als_pow;
    // const struct al_sens_pow_driver_api *api = (struct al_sens_pow_driver_api *)als_pow_dev->driver_api;

    // api->enable(als_pow_dev);
    // w_sem_take(als_pow_dev);

    // al_sensor_power_enable();
    reset(drv_data->i2c_wrap);
    check_id(drv_data->i2c_wrap);
    set_resolution(drv_data->i2c_wrap);

    return 0;
}

static struct si7020_data si7020_driver;

DEVICE_AND_API_INIT(si7020, CONFIG_SI7020_NAME, si7020_init, &si7020_driver,
            NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
            &si7020_driver_api);

/*
 * Copyright (c) 2018 Thomas Berg
 *
 */

#include <kernel.h>
#include <device.h>
#include <i2c.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "tsl4531.h"
#include "../i2c_wrap/i2c_wrap.h"
// #include "i2c_wrap.h"

// #define ALS_VDD_GPIO_PIN_NUM 20

#define TSL4531_I2C_ADDR            0x29

/* Commands */
#define TSL4531_CMD_CONTROL         0x80
#define TSL4531_CMD_CONFIG          0x81
#define TSL4531_CMD_DATA_LOW        0x84
#define TSL4531_CMD_DATA_HIGH       0x85
#define TSL4531_CMD_ID              0x8A

/* Control register */
#define TSL4531_MODE_POWER_DOWN     0x00
#define TSL4531_MODE_SINGLE_SHOT    0x02
#define TSL4531_MODE_NORMAL         0x03

/* Config register */
#define TSL4531_CFG_PSAVESKIP_EN    0x08
#define TSL4531_CFG_PSAVESKIP_DIS   0x00
#define TSL4531_CFG_PSAVESKIP_MASK  0x08
#define TSL4531_CFG_TCNTRL_400MS    0x00
#define TSL4531_CFG_TCNTRL_200MS    0x01
#define TSL4531_CFG_TCNTRL_100MS    0x02
#define TSL4531_CFG_TCNTRL_MASK     0x03

/* Device Identification */
#define TSL45311_ID                 0xB0
#define TSL45313_ID                 0x90
#define TSL45315_ID                 0xA0
#define TSL45317_ID                 0x80


#if (CONFIG_SYS_LOG_SENSOR_LEVEL == 4)
static int check_id(struct device *dev)
{
    int err;
    u8_t buf = TSL4531_CMD_ID;

    err = i2c_write_wrap(dev, &buf, 1, TSL4531_I2C_ADDR);
    if (err) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    err = i2c_read_wrap(dev, &buf, 1, TSL4531_I2C_ADDR);
    if (err) {
        SYS_LOG_ERR("I2C read failed!");
        return -1;
    }

    // Filter out reserved bits
    buf &= 0xF0;

    SYS_LOG_DBG("ID:0x%02x", buf);

    if (buf == TSL45311_ID) {
        SYS_LOG_DBG("TSL45311 found");
    } else if (buf == TSL45313_ID) {
        SYS_LOG_DBG("TSL45313 found");
    } else if (buf == TSL45315_ID) {
        SYS_LOG_DBG("TSL45315 found");
    } else if (buf == TSL45317_ID) {
        SYS_LOG_DBG("TSL45317 found");
    } else {
        SYS_LOG_ERR("Error: TSL4531x chip id does not match");
        return -1;
    }

    return 0;
}
#endif

// static int set_resolution(struct device *dev)
// {
//     u8_t buf[2] = { CMD_WRITE_REGISTER_1, REG1_RESOLUTION_H12_T14 };

//     if (i2c_write(dev, buf, 2, SI7020_I2C_ADDR)) {
//         SYS_LOG_ERR("I2C write failed!");
//         return -1;
//     }

//     return 0;
// }

static int start_sample(struct device *dev)
{
    u8_t buf[2] = { TSL4531_CMD_CONTROL, TSL4531_MODE_SINGLE_SHOT };

    i2c_wrap_sem_take(dev);

    if (i2c_write_wrap(dev, buf, 2, TSL4531_I2C_ADDR)) {
        SYS_LOG_ERR("I2C write failed!");
        return -1;
    }

    return 0;
}

static u16_t get_ambient_light(struct device *dev)
{
    u16_t ambient_light = 0;
    u8_t buf[2] = { 0 };

    i2c_wrap_sem_give(dev);

    if (i2c_burst_read_wrap(dev, TSL4531_I2C_ADDR, TSL4531_CMD_DATA_LOW, buf, 2))
    {
        SYS_LOG_ERR("Failed to read ambient light!");
        return 0;
    }

    ambient_light = (buf[1] << 8) | buf[0];

    return ambient_light;
}

static int tsl4531_sample_fetch(struct device *dev, enum sensor_channel chan)
{
    struct tsl4531_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

    start_sample(drv_data->i2c_wrap);
    k_sleep(420);
    drv_data->al_sample = get_ambient_light(drv_data->i2c_wrap);
    SYS_LOG_DBG("Lux: %u", drv_data->al_sample);

    return 0;
}

static int tsl4531_channel_get(struct device *dev, enum sensor_channel chan,
                struct sensor_value *val)
{
    struct tsl4531_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_LIGHT);

    val->val1 = drv_data->al_sample;
    val->val2 = 0;

    return 0;
}

static const struct sensor_driver_api tsl4531_driver_api = {
    .sample_fetch = tsl4531_sample_fetch,
    .channel_get = tsl4531_channel_get,
};

static int tsl4531_init(struct device *dev)
{
    struct tsl4531_data *drv_data = dev->driver_data;

    SYS_LOG_INF("Init TSL4531");

    drv_data->i2c_wrap = device_get_binding(CONFIG_I2C_WRAP_NAME);
    if (drv_data->i2c_wrap == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!",
                CONFIG_TSL4531_I2C_MASTER_DEV_NAME);
        return -EINVAL;
    }

#if (CONFIG_SYS_LOG_SENSOR_LEVEL == 4)
    int err = check_id(drv_data->i2c_wrap);
    if (err) {
        SYS_LOG_ERR("TSL4531 device not found");
        return -EINVAL;
    }
#endif

    // set_resolution(drv_data->i2c_wrap);

    return 0;
}

static struct tsl4531_data tsl4531_driver;

DEVICE_AND_API_INIT(tsl4531, CONFIG_TSL4531_NAME, tsl4531_init, &tsl4531_driver,
            NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
            &tsl4531_driver_api);

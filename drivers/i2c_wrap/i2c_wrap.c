/*
 * Copyright (c) 2018 Thomas Berg
 *
 */

#include <kernel.h>
#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "i2c_wrap.h"

#define CONFIG_SYS_LOG_I2C_WRAP_LEVEL 1
#define SYS_LOG_DOMAIN "i2c_wrap"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_WRAP_LEVEL
#include <logging/sys_log.h>


#define ALS_VDD_GPIO_PIN_NUM 20
#define AL_SENSOR_MAX_NUM_USERS 1


static struct k_sem pow_sem;


static int w_i2c_write(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr)
{
    int err;
    struct i2c_wrap_data *drv_data = dev->driver_data;

    // Set GPIO high
    err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 1);
    if (err != 0) {
        SYS_LOG_ERR("Failed to set GPIO%d high", ALS_VDD_GPIO_PIN_NUM);
        return -1;
    }

    k_sleep(1);

    // Transfer I2C
    err = i2c_write(drv_data->i2c, buf, num_bytes, addr);
    if (err != 0) {
        SYS_LOG_ERR("I2C write failed");
        return -1;
    }

    // Check if semaphore is taken, otherwise set GPIO low
    if (k_sem_count_get(&pow_sem) == AL_SENSOR_MAX_NUM_USERS) {
        err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 0);
        if (err != 0) {
            SYS_LOG_ERR("Failed to set GPIO%d low", ALS_VDD_GPIO_PIN_NUM);
            return -1;
        }
    }

    return 0;
}

static int w_i2c_read(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr)
{
    int err;
    struct i2c_wrap_data *drv_data = dev->driver_data;

    // Set GPIO high
    err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 1);
    if (err != 0) {
        SYS_LOG_ERR("Failed to set GPIO%d high", ALS_VDD_GPIO_PIN_NUM);
        return -1;
    }

    k_sleep(1);

    // Transfer I2C
    err = i2c_read(drv_data->i2c, buf, num_bytes, addr);
    if (err != 0) {
        SYS_LOG_ERR("I2C read failed");
        return -1;
    }

    // Check if semaphore is taken, otherwise set GPIO low
    if (k_sem_count_get(&pow_sem) == AL_SENSOR_MAX_NUM_USERS) {
        err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 0);
        if (err != 0) {
            SYS_LOG_ERR("Failed to set GPIO%d low", ALS_VDD_GPIO_PIN_NUM);
            return -1;
        }
    }

    return 0;
}

static int w_i2c_burst_read(struct device *dev, u16_t dev_addr,
                 u8_t start_addr, u8_t *buf,
                 u8_t num_bytes)
{
    int err;
    struct i2c_wrap_data *drv_data = dev->driver_data;

    // Set GPIO high
    err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 1);
    if (err != 0) {
        SYS_LOG_ERR("Failed to set GPIO%d high", ALS_VDD_GPIO_PIN_NUM);
        return -1;
    }

    k_sleep(1);

    // Transfer I2C
    err = i2c_burst_read(drv_data->i2c, dev_addr, start_addr, buf, num_bytes);
    if (err != 0) {
        SYS_LOG_ERR("I2C burst read failed");
        return -1;
    }

    // Check if semaphore is taken, otherwise set GPIO low
    if (k_sem_count_get(&pow_sem) == AL_SENSOR_MAX_NUM_USERS) {
        err = gpio_pin_write(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM, 0);
        if (err != 0) {
            SYS_LOG_ERR("Failed to set GPIO%d low", ALS_VDD_GPIO_PIN_NUM);
            return -1;
        }
    }

    return 0;
}

static int w_sem_take(struct device *dev)
{
    int err;

    SYS_LOG_DBG("Attempting to take pow_sem:%d", k_sem_count_get(&pow_sem));

    err = k_sem_take(&pow_sem, K_MSEC(50));
    if (err != 0) {
        SYS_LOG_ERR("Failed to take power semaphore");
        return -1;
    }

    SYS_LOG_DBG("Succeded to take pow_sem:%d", k_sem_count_get(&pow_sem));

    return 0;
}

static int w_sem_give(struct device *dev)
{
    SYS_LOG_DBG("Attempting to give pow_sem:%d", k_sem_count_get(&pow_sem));

    k_sem_give(&pow_sem);

    SYS_LOG_DBG("Succeded to give pow_sem:%d", k_sem_count_get(&pow_sem));

    return 0;
}

static const struct i2c_wrap_driver_api i2c_wrap_driver_api = {
    .w_i2c_write      = w_i2c_write,
    .w_i2c_read       = w_i2c_read,
    .w_i2c_burst_read = w_i2c_burst_read,
    .w_sem_take       = w_sem_take,
    .w_sem_give       = w_sem_give,
};

static int i2c_wrap_init(struct device *dev)
{
    struct i2c_wrap_data *drv_data = dev->driver_data;

    SYS_LOG_DBG("Init ALS power");

    drv_data->gpio = device_get_binding(CONFIG_AL_SENS_POW_GPIO_DEV_NAME);
    if (drv_data->gpio == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!",
                CONFIG_AL_SENS_POW_GPIO_DEV_NAME);
        return -EINVAL;
    }

    gpio_pin_configure(drv_data->gpio, ALS_VDD_GPIO_PIN_NUM,
               GPIO_DIR_OUT | GPIO_POL_NORMAL | GPIO_DS_ALT_HIGH );

    k_sem_init(&pow_sem, AL_SENSOR_MAX_NUM_USERS, AL_SENSOR_MAX_NUM_USERS);

    drv_data->i2c = device_get_binding(CONFIG_SI7020_I2C_MASTER_DEV_NAME);
    if (drv_data->i2c == NULL) {
        SYS_LOG_ERR("Failed to get pointer to %s device!",
                CONFIG_SI7020_I2C_MASTER_DEV_NAME);
        return -EINVAL;
    }

    return 0;
}

static struct i2c_wrap_data i2c_wrap_driver;

DEVICE_AND_API_INIT(i2c_wrap, CONFIG_I2C_WRAP_NAME, i2c_wrap_init, &i2c_wrap_driver,
            NULL, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
            &i2c_wrap_driver_api);



int i2c_write_wrap(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr)
{
    const struct i2c_wrap_driver_api *api = dev->driver_api;

    return api->w_i2c_write(dev, buf, num_bytes, addr);
}

int i2c_read_wrap(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr)
{
    const struct i2c_wrap_driver_api *api = dev->driver_api;

    return api->w_i2c_read(dev, buf, num_bytes, addr);
}

int i2c_burst_read_wrap(struct device *dev, u16_t dev_addr,
                 u8_t start_addr, u8_t *buf,
                 u8_t num_bytes)
{
    const struct i2c_wrap_driver_api *api = dev->driver_api;

    return api->w_i2c_burst_read(dev, dev_addr, start_addr, buf, num_bytes);
}

int i2c_wrap_sem_take(struct device *dev)
{
    const struct i2c_wrap_driver_api *api = dev->driver_api;

    return api->w_sem_take(dev);
}

int i2c_wrap_sem_give(struct device *dev)
{
    const struct i2c_wrap_driver_api *api = dev->driver_api;

    return api->w_sem_give(dev);
}

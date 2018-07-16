/*
 * Copyright (c) 2018 Thomas Berg
 *
 */

#ifndef _SENSOR_TSL4531
#define _SENSOR_TSL4531

#include <device.h>
#include <misc/util.h>

struct tsl4531_data {
    struct device *i2c_wrap;
    u16_t al_sample;
};

#define SYS_LOG_DOMAIN "tsl4531"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_TSL4531 */

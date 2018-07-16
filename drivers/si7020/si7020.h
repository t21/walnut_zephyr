/*
 * Copyright (c) 2018 Thomas Berg
 *
 */

#ifndef _SENSOR_SI7020
#define _SENSOR_SI7020

#include <device.h>
#include <misc/util.h>

struct si7020_data {
    struct device *i2c_wrap;
    u16_t t_sample;
    u16_t rh_sample;
};

#define SYS_LOG_DOMAIN "si7020"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_SI7020 */

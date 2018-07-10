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

#endif /* _SENSOR_SI7020 */

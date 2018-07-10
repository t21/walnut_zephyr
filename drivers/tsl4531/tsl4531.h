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

#endif /* _SENSOR_TSL4531 */

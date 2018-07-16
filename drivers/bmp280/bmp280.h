/*
 *
 *
 */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_BMP280_H_
#define _SENSOR_BMP280_H_

#include <zephyr/types.h>
#include <device.h>

#define BMP280_REG_PRESS_MSB            0xF7
#define BMP280_REG_COMP_START           0x88
// #define BME280_REG_HUM_COMP_PART1       0xA1
// #define BME280_REG_HUM_COMP_PART2       0xE1
#define BMP280_REG_ID                   0xD0
#define BMP280_REG_CONFIG               0xF5
#define BMP280_REG_CTRL_MEAS            0xF4
// #define BME280_REG_CTRL_HUM             0xF2

// #define BMP280_CHIP_ID_SAMPLE_1         0x56
// #define BMP280_CHIP_ID_SAMPLE_2         0x57
#define BMP280_CHIP_ID               	0x58
// #define BME280_CHIP_ID                  0x60
#define BMP280_MODE_NORMAL              0x03
#define BMP280_SPI_3W_DISABLE           0x00

#if defined CONFIG_BMP280_TEMP_OVER_1X
#define BMP280_TEMP_OVER                (1 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_2X
#define BMP280_TEMP_OVER                (2 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_4X
#define BMP280_TEMP_OVER                (3 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_8X
#define BMP280_TEMP_OVER                (4 << 5)
#elif defined CONFIG_BMP280_TEMP_OVER_16X
#define BMP280_TEMP_OVER                (5 << 5)
#endif

#if defined CONFIG_BMP280_PRESS_OVER_1X
#define BMP280_PRESS_OVER               (1 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_2X
#define BMP280_PRESS_OVER               (2 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_4X
#define BMP280_PRESS_OVER               (3 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_8X
#define BMP280_PRESS_OVER               (4 << 2)
#elif defined CONFIG_BMP280_PRESS_OVER_16X
#define BMP280_PRESS_OVER               (5 << 2)
#endif

// #if defined CONFIG_BME280_HUMIDITY_OVER_1X
// #define BME280_HUMIDITY_OVER            1
// #elif defined CONFIG_BME280_HUMIDITY_OVER_2X
// #define BME280_HUMIDITY_OVER            2
// #elif defined CONFIG_BME280_HUMIDITY_OVER_4X
// #define BME280_HUMIDITY_OVER            3
// #elif defined CONFIG_BME280_HUMIDITY_OVER_8X
// #define BME280_HUMIDITY_OVER            4
// #elif defined CONFIG_BME280_HUMIDITY_OVER_16X
// #define BME280_HUMIDITY_OVER            5
// #endif

#if defined CONFIG_BMP280_STANDBY_05MS
#define BMP280_STANDBY                  0
#elif defined CONFIG_BMP280_STANDBY_62MS
#define BMP280_STANDBY                  (1 << 5)
#elif defined CONFIG_BMP280_STANDBY_125MS
#define BMP280_STANDBY                  (2 << 5)
#elif defined CONFIG_BMP280_STANDBY_250MS
#define BMP280_STANDBY                  (3 << 5)
#elif defined CONFIG_BMP280_STANDBY_500MS
#define BMP280_STANDBY                  (4 << 5)
#elif defined CONFIG_BMP280_STANDBY_1000MS
#define BMP280_STANDBY                  (5 << 5)
#elif defined CONFIG_BMP280_STANDBY_2000MS
#define BMP280_STANDBY                  (6 << 5)
#elif defined CONFIG_BMP280_STANDBY_4000MS
#define BMP280_STANDBY                  (7 << 5)
#endif

#if defined CONFIG_BMP280_FILTER_OFF
#define BMP280_FILTER                   0
#elif defined CONFIG_BMP280_FILTER_2
#define BMP280_FILTER                   (1 << 2)
#elif defined CONFIG_BMP280_FILTER_4
#define BMP280_FILTER                   (2 << 2)
#elif defined CONFIG_BMP280_FILTER_8
#define BMP280_FILTER                   (3 << 2)
#elif defined CONFIG_BMP280_FILTER_16
#define BMP280_FILTER                   (4 << 2)
#endif

#define BMP280_CTRL_MEAS_VAL            (BMP280_PRESS_OVER | \
					 BMP280_TEMP_OVER |  \
					 BMP280_MODE_NORMAL)
#define BMP280_CONFIG_VAL               (BMP280_STANDBY | \
					 BMP280_FILTER |  \
					 BMP280_SPI_3W_DISABLE)

#define BMP280_I2C_ADDR                 CONFIG_BMP280_I2C_ADDR

struct bmp280_data {
#ifdef CONFIG_BMP280_DEV_TYPE_I2C
	// struct device *i2c_master;
	struct device *i2c_wrap;
	u16_t i2c_slave_addr;
#elif defined CONFIG_BMP280_DEV_TYPE_SPI
	struct device *spi;
	struct spi_config spi_cfg;
#else
#error "BMP280 device type not specified"
#endif
	/* Compensation parameters. */
	u16_t dig_t1;
	s16_t dig_t2;
	s16_t dig_t3;
	u16_t dig_p1;
	s16_t dig_p2;
	s16_t dig_p3;
	s16_t dig_p4;
	s16_t dig_p5;
	s16_t dig_p6;
	s16_t dig_p7;
	s16_t dig_p8;
	s16_t dig_p9;
	// u8_t dig_h1;
	// s16_t dig_h2;
	// u8_t dig_h3;
	// s16_t dig_h4;
	// s16_t dig_h5;
	// s8_t dig_h6;

	/* Compensated values. */
	s32_t comp_temp;
	u32_t comp_press;
	// u32_t comp_humidity;

	/* Carryover between temperature and pressure/humidity compensation. */
	s32_t t_fine;

	u8_t chip_id;
};

#define SYS_LOG_DOMAIN "BMP280"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_BMP280_H_ */

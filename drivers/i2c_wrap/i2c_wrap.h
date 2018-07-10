/**
 * @typedef sensor_sample_fetch_t
 * @brief Callback API for fetching data from a sensor
 *
 * See sensor_sample_fetch() for argument description
 */
// typedef int (*enable_t)(struct device *dev);
/**
 * @typedef sensor_channel_get_t
 * @brief Callback API for getting a reading from a sensor
 *
 * See sensor_channel_get() for argument description
 */
// typedef int (*disable_t)(struct device *dev);

#ifndef I2C_WRAP_H
#define I2C_WRAP_H


struct i2c_wrap_data {
	struct device *gpio;
	struct device *i2c;
};


typedef int (*w_i2c_write_t)(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr);
typedef int (*w_i2c_read_t)(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr);
// typedef int (*w_i2c_read_t)(struct device *dev);
typedef int (*w_i2c_burst_read_t)(struct device *dev, u16_t dev_addr,
                 u8_t start_addr, u8_t *buf,
                 u8_t num_bytes);

typedef int (*w_sem_take_t)(struct device *dev);
typedef int (*w_sem_give_t)(struct device *dev);

struct i2c_wrap_driver_api {
    w_i2c_write_t       w_i2c_write;
    w_i2c_read_t        w_i2c_read;
    w_i2c_burst_read_t  w_i2c_burst_read;
    w_sem_take_t        w_sem_take;
    w_sem_give_t        w_sem_give;
};



int i2c_write_wrap(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr);

int i2c_read_wrap(struct device *dev, u8_t *buf,
                u32_t num_bytes, u16_t addr);

int i2c_burst_read_wrap(struct device *dev, u16_t dev_addr,
                 u8_t start_addr, u8_t *buf,
                 u8_t num_bytes);

int i2c_wrap_sem_take(struct device *dev);

int i2c_wrap_sem_give(struct device *dev);

#endif /* I2C_WRAP_H */


#include <zephyr.h>
// #include <misc/reboot.h>
#include <board.h>
#include <device.h>
#include <string.h>
#include <nvs/nvs.h>

#include "gdfs.h"

#define CONFIG_SYS_LOG_GDFS_LEVEL 1

#define SYS_LOG_DOMAIN "gdfs"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_GDFS_LEVEL
#include <logging/sys_log.h>


#define STORAGE_MAGIC 0xefbeadde // 0xdeadbeef

#define NVS_SECTOR_SIZE 1024 /* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_COUNT 2 /* At least 2 sectors */
#define NVS_STORAGE_OFFSET FLASH_AREA_STORAGE_OFFSET /* Start address of the
                              * filesystem in flash
                              */
#define NVS_MAX_ELEM_SIZE 256 /* Largest item that can be stored */

static struct nvs_fs fs = {
    .sector_size = NVS_SECTOR_SIZE,
    .sector_count = NVS_SECTOR_COUNT,
    .offset = NVS_STORAGE_OFFSET,
    .max_len = NVS_MAX_ELEM_SIZE,
};


int gdfs_get_device_data(gdfs_device_data_t *data)
{
    int read_len = 0;
    uint8_t *buf = (uint8_t *)data;
    int buf_len = sizeof(gdfs_device_data_t);

    SYS_LOG_INF("Read device data");

    read_len = nvs_read(&fs, GDFS_DEVICE_DATA, buf, buf_len);
    if (read_len == buf_len) {
        SYS_LOG_DBG("Read device data success");
    } else if (read_len > buf_len) {
        SYS_LOG_ERR("Error: all data not read");
    } else if (read_len < 0) {
        SYS_LOG_ERR("Error reading device data: %d", read_len);
        return read_len;
    }

    // printk("hexdump");

    // for (int i = 0; i < buf_len; i++) {
    //     if (i % 16 == 0) {
    //         printk("\n");
    //     }
    //     printk(" %02x", buf[i]);
    // }
    // printk("\n");

    return 0;
}

int gdfs_set_device_data(const gdfs_device_data_t *data)
{
    int write_len = 0;
    uint8_t *buf = (uint8_t *)data;
    int buf_len = sizeof(gdfs_device_data_t);

    SYS_LOG_INF("Write device data");

    write_len = nvs_write(&fs, GDFS_DEVICE_DATA, buf, buf_len);
    if (write_len == buf_len) {
        SYS_LOG_DBG("Write device data success");
    } else if (write_len < 0) {
        SYS_LOG_ERR("Error writing device data:%d", write_len);
        return write_len;
    }

    return 0;
}

int gdfs_get_sensor_data(gdfs_types_t sensor, gdfs_sensor_data_t *data)
{
    int read_len = 0;
    uint8_t *buf = (uint8_t *)data;
    int buf_len = sizeof(gdfs_sensor_data_t);

    SYS_LOG_INF("Read sensor%02d data", sensor);

    read_len = nvs_read(&fs, (uint16_t)sensor, buf, buf_len);
    if (read_len == buf_len) {
        SYS_LOG_DBG("Read sensor%02d data success", sensor);
    } else if (read_len > buf_len) {
        SYS_LOG_ERR("Error: all data not read");
    } else if (read_len < 0) {
        SYS_LOG_ERR("Error reading sensor%02d data: %d", sensor, read_len);
        return read_len;
    }

    // printk("hexdump");

    // for (int i = 0; i < buf_len; i++) {
    //     if (i % 16 == 0) {
    //         printk("\n");
    //     }
    //     printk(" %02x", buf[i]);
    // }
    // printk("\n");

    return 0;

}

int gdfs_set_sensor_data(gdfs_types_t sensor, const gdfs_sensor_data_t *data)
{
    int write_len = 0;
    uint8_t *buf = (uint8_t *)data;
    int buf_len = sizeof(gdfs_sensor_data_t);

    SYS_LOG_INF("Write sensor%02d data", sensor);

    write_len = nvs_write(&fs, (uint16_t)sensor, buf, buf_len);
    if (write_len == buf_len) {
        SYS_LOG_DBG("Write sensor%02d data success", sensor);
    } else if (write_len < 0) {
        SYS_LOG_ERR("Error writing sensor%02d data:%d", sensor, write_len);
        return write_len;
    }

    return 0;
}

// static void gdfs_test(void) {
//     int err = 0;
//     gdfs_device_data_t device_data;
//     gdfs_sensor_data_t sensor_data;

//     err = gdfs_get_sensor_data(GDFS_SENSOR_HUMIDITY, &sensor_data);
//     if (err == -ENOENT) {
//         sensor_data.meas_interval = 100;
//         gdfs_set_sensor_data(GDFS_SENSOR_HUMIDITY, &sensor_data);
//         gdfs_get_sensor_data(GDFS_SENSOR_HUMIDITY, &sensor_data);
//     }

//     err = gdfs_get_device_data(&device_data);
//     if (err == -ENOENT) {
//         device_data.adv_interval = 100;
//         gdfs_set_device_data(&device_data);
//         gdfs_get_device_data(&device_data);
//     }
// }

int gdfs_init(void)
{
    int err = 0;

    err = nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);
    if (err) {
        SYS_LOG_ERR("Flash Init failed");
    }

    nvs_clear(&fs);
    nvs_init(&fs, FLASH_DEV_NAME, STORAGE_MAGIC);

    // gdfs_test();

    return 0;
}
/** @file
 *  @brief Environmental Sensor Service
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "ess.h"

#define SYS_LOG_DOMAIN "ESS"
// #define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define TEMPERATURE_SENSOR_NAME				"Temperature Sensor"
#define SENSOR_2_NAME				"Temperature Sensor 2"
#define HUMIDITY_SENSOR_NAME				"Humidity Sensor"


struct es_measurement {
    u16_t flags; /* Reserved for Future Use */
    u8_t sampling_func;
    u32_t meas_period;
    u32_t update_interval;
    u8_t application;
    u8_t meas_uncertainty;
};

struct ess_sensor {
    s16_t value;

    /* Valid Range */
    s16_t lower_limit;
    s16_t upper_limit;

    /* ES trigger setting - Value Notification condition */
    u8_t condition;
    union {
        u32_t seconds;
        s16_t ref_val; /* Reference temperature */
    };

    struct bt_gatt_ccc_cfg  ccc_cfg[BT_GATT_CCC_MAX];
    struct es_measurement meas;
};

struct ess_pressure_sensor {
    uint32_t value;

    /* Valid Range */
    uint32_t lower_limit;
    uint32_t upper_limit;

    /* ES trigger setting - Value Notification condition */
    u8_t condition;
    union {
        u32_t seconds;
        uint32_t ref_val; /* Reference temperature */
    };

    struct bt_gatt_ccc_cfg  ccc_cfg[BT_GATT_CCC_MAX];
    struct es_measurement meas;
};


static struct ess_sensor temperature_sensor;
static struct ess_sensor rh_sensor;
static struct ess_sensor al_sensor;
static struct ess_pressure_sensor bp_sensor;

static u8_t is_temp_notify_enabled;

static inline void int_to_le24(u32_t value, u8_t *u24)
{
    u24[0] = value & 0xff;
    u24[1] = (value >> 8) & 0xff;
    u24[2] = (value >> 16) & 0xff;
}

static inline u32_t le24_to_int(const u8_t *u24)
{
    return ((u32_t)u24[0] |
        (u32_t)u24[1] << 8 |
        (u32_t)u24[2] << 16);
}


static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset)
{
    const u16_t *u16 = attr->user_data;
    u16_t value = sys_cpu_to_le16(*u16);

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
                 sizeof(value));
}

static ssize_t read_u32(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset)
{
    const u32_t *u32 = attr->user_data;
    u32_t value = sys_cpu_to_le32(*u32);

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
                 sizeof(value));
}


static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    is_temp_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void rh_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    // simulate_temp = value == BT_GATT_CCC_NOTIFY;
    // value == BT_GATT_CCC_NOTIFY;
}

static void al_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    // simulate_temp = value == BT_GATT_CCC_NOTIFY;
    // value == BT_GATT_CCC_NOTIFY;
}

static void bp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    // simulate_temp = value == BT_GATT_CCC_NOTIFY;
    // value == BT_GATT_CCC_NOTIFY;
}

struct read_es_measurement_rp {
    u16_t flags; /* Reserved for Future Use */
    u8_t sampling_function;
    u8_t measurement_period[3];
    u8_t update_interval[3];
    u8_t application;
    u8_t measurement_uncertainty;
} __packed;

static ssize_t read_es_measurement(struct bt_conn *conn,
                   const struct bt_gatt_attr *attr, void *buf,
                   u16_t len, u16_t offset)
{
    const struct es_measurement *value = attr->user_data;
    struct read_es_measurement_rp rsp;

    rsp.flags = sys_cpu_to_le16(value->flags);
    rsp.sampling_function = value->sampling_func;
    int_to_le24(value->meas_period, rsp.measurement_period);
    int_to_le24(value->update_interval, rsp.update_interval);
    rsp.application = value->application;
    rsp.measurement_uncertainty = value->meas_uncertainty;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &rsp,
                 sizeof(rsp));
}

static ssize_t read_valid_range(struct bt_conn *conn,
                     const struct bt_gatt_attr *attr, void *buf,
                     u16_t len, u16_t offset)
{
    const struct ess_sensor *sensor = attr->user_data;
    u16_t tmp[] = {sys_cpu_to_le16(sensor->lower_limit),
              sys_cpu_to_le16(sensor->upper_limit)};

    return bt_gatt_attr_read(conn, attr, buf, len, offset, tmp,
                 sizeof(tmp));
}

// static bool check_condition(u8_t condition, s16_t old_val, s16_t new_val,
//                 s16_t ref_val)
// {
//     switch (condition) {
//     case ESS_TRIGGER_INACTIVE:
//         return false;
//     case ESS_FIXED_TIME_INTERVAL:
//     case ESS_NO_LESS_THAN_SPECIFIED_TIME:
//         /* TODO: Check time requirements */
//         return false;
//     case ESS_VALUE_CHANGED:
//         return new_val != old_val;
//     case ESS_LESS_THAN_REF_VALUE:
//         return new_val < ref_val;
//     case ESS_LESS_OR_EQUAL_TO_REF_VALUE:
//         return new_val <= ref_val;
//     case ESS_GREATER_THAN_REF_VALUE:
//         return new_val > ref_val;
//     case ESS_GREATER_OR_EQUAL_TO_REF_VALUE:
//         return new_val >= ref_val;
//     case ESS_EQUAL_TO_REF_VALUE:
//         return new_val == ref_val;
//     case ESS_NOT_EQUAL_TO_REF_VALUE:
//         return new_val != ref_val;
//     default:
//         return false;
//     }
// }

static struct bt_gatt_attr ess_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

    /* Temperature Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16, NULL, &temperature_sensor.value),
    BT_GATT_CUD(TEMPERATURE_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
                    read_es_measurement, NULL, &temperature_sensor.meas),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &temperature_sensor),
    // BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
    //            BT_GATT_PERM_READ, read_temp_trigger_setting,
    //            NULL, &sensor_1),
    BT_GATT_CCC(temperature_sensor.ccc_cfg, temp_ccc_cfg_changed),

    /* Humidity Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16, NULL, &rh_sensor.value),
    BT_GATT_CUD(HUMIDITY_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &rh_sensor.meas),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &rh_sensor),
    BT_GATT_CCC(rh_sensor.ccc_cfg, rh_ccc_cfg_changed),

    /* Ambient Light Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_IRRADIANCE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16, NULL, &al_sensor.value),
    BT_GATT_CUD(HUMIDITY_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &al_sensor.meas),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &al_sensor),
    BT_GATT_CCC(al_sensor.ccc_cfg, al_ccc_cfg_changed),

    /* Barometric Pressure Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u32, NULL, &bp_sensor.value),
    BT_GATT_CUD(HUMIDITY_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &bp_sensor.meas),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &bp_sensor),
    BT_GATT_CCC(bp_sensor.ccc_cfg, bp_ccc_cfg_changed),
};

static struct bt_gatt_service ess_svc = BT_GATT_SERVICE(ess_attrs);


void ess_init(void)
{
    bt_gatt_service_register(&ess_svc);

    // Init temperature sensor data
    struct es_measurement *meas = &temperature_sensor.meas;
    meas->sampling_func = 1;
    temperature_sensor.lower_limit = -4000;
    temperature_sensor.upper_limit = 8500;

    // printk("%d\n", sizeof(ess_attrs) / sizeof(struct bt_gatt_attr));

    // Init humidity sensor data
    rh_sensor.lower_limit = 0;
    rh_sensor.upper_limit = 10000;

}

void ess_temperature_update(s16_t temperature)
// void ess_temperature_humidity_notify(si7020_meas_result_t meas)
{
    bool notify = true;
//     // bool notify = check_condition(sensor->condition,
//     //                   sensor->temp_value, value,
//     //                   sensor->ref_val);

//     SYS_LOG_DBG("T:%d", meas.temperature);
//     SYS_LOG_DBG("RH:%d", meas.humidity);

    /* Update temperature value */
    temperature_sensor.value = temperature;

    if (!is_temp_notify_enabled) {
        return;
    }

    /* Trigger notification if conditions are met */
    if (notify) {
//         // value = sys_cpu_to_le16(temperature_sensor.value);

//         // bt_gatt_notify(NULL, &ess_attrs[2], &value, sizeof(value));
        bt_gatt_notify(NULL, &ess_attrs[2], &temperature_sensor.value, sizeof(temperature_sensor.value));
    }

}

void ess_humidity_update(s16_t humidity)
{
    bool notify = true;
//     // bool notify = check_condition(sensor->condition,
//     //                   sensor->temp_value, value,
//     //                   sensor->ref_val);

//     SYS_LOG_DBG("T:%d", meas.temperature);
//     SYS_LOG_DBG("RH:%d", meas.humidity);

    /* Update humidity value */
    rh_sensor.value = humidity;

    /* Trigger notification if conditions are met */
    if (notify) {
//         // value = sys_cpu_to_le16(temperature_sensor.value);

//         // bt_gatt_notify(NULL, &ess_attrs[2], &value, sizeof(value));
        bt_gatt_notify(NULL, &ess_attrs[8], &rh_sensor.value, sizeof(rh_sensor.value));
    }

}

void ess_als_update(s16_t sensor_val)
{
    bool notify = true;
//     // bool notify = check_condition(sensor->condition,
//     //                   sensor->temp_value, value,
//     //                   sensor->ref_val);

    /* Update ambient light value */
    al_sensor.value = sensor_val;

    /* Trigger notification if conditions are met */
    if (notify) {
//         // value = sys_cpu_to_le16(temperature_sensor.value);

//         // bt_gatt_notify(NULL, &ess_attrs[2], &value, sizeof(value));
        bt_gatt_notify(NULL, &ess_attrs[14], &al_sensor.value, sizeof(al_sensor.value));
    }

}

void ess_baro_press_update(uint32_t sensor_val)
{
    bool notify = true;
//     // bool notify = check_condition(sensor->condition,
//     //                   sensor->temp_value, value,
//     //                   sensor->ref_val);

    /* Update barometric pressure value */
    bp_sensor.value = sensor_val;

    /* Trigger notification if conditions are met */
    if (notify) {
//         // value = sys_cpu_to_le16(temperature_sensor.value);

//         // bt_gatt_notify(NULL, &ess_attrs[2], &value, sizeof(value));
        bt_gatt_notify(NULL, &ess_attrs[20], &bp_sensor.value, sizeof(bp_sensor.value));
    }

}

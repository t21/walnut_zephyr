/**
 *
 *
 *
 */

#include <stdbool.h>
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

#include "bas.h"
#include "dis.h"
#include "ess.h"

#define SYS_LOG_DOMAIN "BLE"
// #define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define DEVICE_NAME				CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN				(sizeof(DEVICE_NAME) - 1)

/* Sensor Internal Update Interval [seconds] */
#define SENSOR_1_UPDATE_IVAL			5
#define SENSOR_2_UPDATE_IVAL			12
#define SENSOR_3_UPDATE_IVAL			60

/* ESS error definitions */
#define ESS_ERR_WRITE_REJECT			0x80
#define ESS_ERR_COND_NOT_SUPP			0x81

/* ESS Trigger Setting conditions */
#define ESS_TRIGGER_INACTIVE			0x00
#define ESS_FIXED_TIME_INTERVAL			0x01
#define ESS_NO_LESS_THAN_SPECIFIED_TIME		0x02
#define ESS_VALUE_CHANGED			0x03
#define ESS_LESS_THAN_REF_VALUE			0x04
#define ESS_LESS_OR_EQUAL_TO_REF_VALUE		0x05
#define ESS_GREATER_THAN_REF_VALUE		0x06
#define ESS_GREATER_OR_EQUAL_TO_REF_VALUE	0x07
#define ESS_EQUAL_TO_REF_VALUE			0x08
#define ESS_NOT_EQUAL_TO_REF_VALUE		0x09


#define ESS_ADV_SLOW BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_ADV_SLOW_INT_MIN, \
				       BT_GAP_ADV_SLOW_INT_MAX)

#define ESS_ADV_FAST_1 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_ADV_FAST_INT_MIN_1, \
				       BT_GAP_ADV_FAST_INT_MAX_1)

#define ESS_ADV_FAST_2 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2)


// static inline void int_to_le24(u32_t value, u8_t *u24)
// {
//     u24[0] = value & 0xff;
//     u24[1] = (value >> 8) & 0xff;
//     u24[2] = (value >> 16) & 0xff;
// }

// static inline u32_t le24_to_int(const u8_t *u24)
// {
//     return ((u32_t)u24[0] |
//         (u32_t)u24[1] << 8 |
//         (u32_t)u24[2] << 16);
// }

// static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
//             void *buf, u16_t len, u16_t offset)
// {
//     const u16_t *u16 = attr->user_data;
//     u16_t value = sys_cpu_to_le16(*u16);

//     return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
//                  sizeof(value));
// }

// /* Environmental Sensing Service Declaration */

// struct es_measurement {
//     u16_t flags; /* Reserved for Future Use */
//     u8_t sampling_func;
//     u32_t meas_period;
//     u32_t update_interval;
//     u8_t application;
//     u8_t meas_uncertainty;
// };

// struct temperature_sensor {
//     s16_t temp_value;

//     /* Valid Range */
//     s16_t lower_limit;
//     s16_t upper_limit;

//     /* ES trigger setting - Value Notification condition */
//     u8_t condition;
//     union {
//         u32_t seconds;
//         s16_t ref_val; /* Reference temperature */
//     };

//     struct bt_gatt_ccc_cfg  ccc_cfg[BT_GATT_CCC_MAX];
//     struct es_measurement meas;
// };

// struct humidity_sensor {
//     s16_t humid_value;

//     struct es_measurement meas;
// };

// static bool simulate_temp;
// static struct temperature_sensor sensor_1 = {
//         .temp_value = 1200,
//         .lower_limit = -10000,
//         .upper_limit = 10000,
//         .condition = ESS_VALUE_CHANGED,
//         .meas.sampling_func = 0x00,
//         .meas.meas_period = 0x01,
//         .meas.update_interval = SENSOR_1_UPDATE_IVAL,
//         .meas.application = 0x1c,
//         .meas.meas_uncertainty = 0x04,
// };

// static struct temperature_sensor sensor_2 = {
//         .temp_value = 1800,
//         .lower_limit = -1000,
//         .upper_limit = 5000,
//         .condition = ESS_VALUE_CHANGED,
//         .meas.sampling_func = 0x00,
//         .meas.meas_period = 0x01,
//         .meas.update_interval = SENSOR_2_UPDATE_IVAL,
//         .meas.application = 0x1b,
//         .meas.meas_uncertainty = 0x04,
// };

// static struct humidity_sensor sensor_3 = {
//         .humid_value = 6233,
//         .meas.sampling_func = 0x02,
//         .meas.meas_period = 0x0e10,
//         .meas.update_interval = SENSOR_3_UPDATE_IVAL,
//         .meas.application = 0x1c,
//         .meas.meas_uncertainty = 0x01,
// };

// static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
//                  u16_t value)
// {
//     simulate_temp = value == BT_GATT_CCC_NOTIFY;
// }

// struct read_es_measurement_rp {
//     u16_t flags; /* Reserved for Future Use */
//     u8_t sampling_function;
//     u8_t measurement_period[3];
//     u8_t update_interval[3];
//     u8_t application;
//     u8_t measurement_uncertainty;
// } __packed;

// static ssize_t read_es_measurement(struct bt_conn *conn,
//                    const struct bt_gatt_attr *attr, void *buf,
//                    u16_t len, u16_t offset)
// {
//     const struct es_measurement *value = attr->user_data;
//     struct read_es_measurement_rp rsp;

//     rsp.flags = sys_cpu_to_le16(value->flags);
//     rsp.sampling_function = value->sampling_func;
//     int_to_le24(value->meas_period, rsp.measurement_period);
//     int_to_le24(value->update_interval, rsp.update_interval);
//     rsp.application = value->application;
//     rsp.measurement_uncertainty = value->meas_uncertainty;

//     return bt_gatt_attr_read(conn, attr, buf, len, offset, &rsp,
//                  sizeof(rsp));
// }

// static ssize_t read_temp_valid_range(struct bt_conn *conn,
//                      const struct bt_gatt_attr *attr, void *buf,
//                      u16_t len, u16_t offset)
// {
//     const struct temperature_sensor *sensor = attr->user_data;
//     u16_t tmp[] = {sys_cpu_to_le16(sensor->lower_limit),
//               sys_cpu_to_le16(sensor->upper_limit)};

//     return bt_gatt_attr_read(conn, attr, buf, len, offset, tmp,
//                  sizeof(tmp));
// }

// struct es_trigger_setting_seconds {
//     u8_t condition;
//     u8_t sec[3];
// } __packed;

// struct es_trigger_setting_reference {
//     u8_t condition;
//     s16_t ref_val;
// } __packed;

// static ssize_t read_temp_trigger_setting(struct bt_conn *conn,
//                      const struct bt_gatt_attr *attr,
//                      void *buf, u16_t len,
//                      u16_t offset)
// {
//     const struct temperature_sensor *sensor = attr->user_data;

//     switch (sensor->condition) {
//     /* Operand N/A */
//     case ESS_TRIGGER_INACTIVE:
//         /* fallthrough */
//     case ESS_VALUE_CHANGED:
//         return bt_gatt_attr_read(conn, attr, buf, len, offset,
//                      &sensor->condition,
//                      sizeof(sensor->condition));
//     /* Seconds */
//     case ESS_FIXED_TIME_INTERVAL:
//         /* fallthrough */
//     case ESS_NO_LESS_THAN_SPECIFIED_TIME: {
//             struct es_trigger_setting_seconds rp;

//             rp.condition = sensor->condition;
//             int_to_le24(sensor->seconds, rp.sec);

//             return bt_gatt_attr_read(conn, attr, buf, len, offset,
//                          &rp, sizeof(rp));
//         }
//     /* Reference temperature */
//     default: {
//             struct es_trigger_setting_reference rp;

//             rp.condition = sensor->condition;
//             rp.ref_val = sys_cpu_to_le16(sensor->ref_val);

//             return bt_gatt_attr_read(conn, attr, buf, len, offset,
//                          &rp, sizeof(rp));
//         }
//     }
// }

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

// static void update_temperature(struct bt_conn *conn,
//                    const struct bt_gatt_attr *chrc, s16_t value,
//                    struct temperature_sensor *sensor)
// {
//     bool notify = check_condition(sensor->condition,
//                       sensor->temp_value, value,
//                       sensor->ref_val);

//     /* Update temperature value */
//     sensor->temp_value = value;

//     /* Trigger notification if conditions are met */
//     if (notify) {
//         value = sys_cpu_to_le16(sensor->temp_value);

//         bt_gatt_notify(conn, chrc, &value, sizeof(value));
//     }
// }


static uint8_t batt[3] = { 0x0f, 0x18, 0x00 };

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, 0x1a, 0x18),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_SVC_DATA16, batt, 3),
};

// static struct bt_data sd[] = {
//     BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
// };

static void connected(struct bt_conn *conn, u8_t err)
{
    if (err) {
        SYS_LOG_ERR("Connection failed (err %u)", err);
    } else {
        SYS_LOG_DBG("Connected");


    // struct bt_le_conn_param param = {
    //     .interval_min = 24,
    //     // .interval_max = 40,
    //     .interval_max = 100,
    //     .latency = 0,
    //     .timeout = 400,
    // };
    // bt_conn_le_param_update(conn, &param);

    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);

    struct bt_conn_le_info *le = &info.le;

    SYS_LOG_INF("Interval:%d", le->interval);
    SYS_LOG_INF("Latency:%d", le->latency);
    SYS_LOG_INF("Timeout:%d", le->timeout);

        // struct bt_le_conn_param param = {
        //     .interval_min = 24,
        //     .interval_max = 40,
        //     .latency = 0,
        //     .timeout = 400,
        // };
        // bt_conn_le_param_update(conn, &param);
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    SYS_LOG_DBG("Disconnected (reason %u)", reason);
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
                u16_t latency, u16_t timeout)
{
    SYS_LOG_INF("U:Interval:%d", interval);
    SYS_LOG_INF("U:Latency:%d", latency);
    SYS_LOG_INF("U:Timeout:%d", timeout);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_updated = le_param_updated,
};

static void bt_ready(int err)
{
    if (err) {
        SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    SYS_LOG_INF("Bluetooth initialized");

    // ToDo: Set up DIS
    dis_init(CONFIG_SOC, "ACME");
    ess_init();
    bas_init();

    err = bt_le_adv_start(ESS_ADV_SLOW, ad, ARRAY_SIZE(ad),
                //   sd, ARRAY_SIZE(sd));
                  NULL, 0);
    if (err) {
        SYS_LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    SYS_LOG_INF("Advertising successfully started");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

static void adv_update(uint8_t batt_capacity)
{
    int err;

    err = bt_le_adv_stop();
    if (err) {
        SYS_LOG_ERR("Failed to stop advertising");
    }

    batt[2] = batt_capacity;

    err = bt_le_adv_start(ESS_ADV_SLOW, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        SYS_LOG_ERR("start error");
    }
}

void ble_update_temp(double temperature)
{
    ess_temperature_update((int16_t)(100 * temperature));
}

void ble_update_humidity(double humidity)
{
    ess_humidity_update((int16_t)(100 * humidity));
}

void ble_update_ambient_light(double ambient_light)
{
    ess_als_update((int16_t)(100 * ambient_light));
}

void ble_update_baro_pressure(double pressure)
{
    ess_baro_press_update((uint32_t)(10000 * pressure));
}

void ble_update_battery(uint8_t battery_capacity)
{
    adv_update(battery_capacity);
    bas_update(battery_capacity);
}

void ble_init(void)
{
    int err;

    SYS_LOG_INF("Initializing BLE");

    err = bt_enable(bt_ready);
    if (err) {
        SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);
}

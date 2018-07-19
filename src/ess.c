/** @file
 *  @brief Environmental Sensor Service
 */

/****************************************************************************
* Include Directives
***************************************************************************/

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
#include "nv.h"

#define SYS_LOG_DOMAIN "ESS"
#define SYS_LOG_LEVEL 1
#include <logging/sys_log.h>


/****************************************************************************
* Preprocessor Directives
***************************************************************************/

#define TEMPERATURE_SENSOR_NAME     "Temperature Sensor"
#define HUMIDITY_SENSOR_NAME        "Humidity Sensor"
#define AMBIENT_LIGHT_SENSOR_NAME   "Ambient Light Sensor"
#define BARO_PRESSURE_SENSOR_NAME   "Barometric Pressure Sensor"

/* ESS Trigger Setting conditions */
#define ESS_TRIGGER_INACTIVE                0x00
#define ESS_FIXED_TIME_INTERVAL             0x01
#define ESS_NO_LESS_THAN_SPECIFIED_TIME     0x02
#define ESS_VALUE_CHANGED                   0x03
#define ESS_LESS_THAN_REF_VALUE             0x04
#define ESS_LESS_OR_EQUAL_TO_REF_VALUE      0x05
#define ESS_GREATER_THAN_REF_VALUE          0x06
#define ESS_GREATER_OR_EQUAL_TO_REF_VALUE   0x07
#define ESS_EQUAL_TO_REF_VALUE              0x08
#define ESS_NOT_EQUAL_TO_REF_VALUE          0x09


/****************************************************************************
* Private Type Declarations
***************************************************************************/

struct ess_meas_desc {
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
        s16_t ref_val;
    };

    struct bt_gatt_ccc_cfg  ccc_cfg[BT_GATT_CCC_MAX];
    struct ess_meas_desc meas_desc;
};

struct ess_pressure_sensor {
    u32_t value;

    /* Valid Range */
    u32_t lower_limit;
    u32_t upper_limit;

    /* ES trigger setting - Value Notification condition */
    u8_t condition;
    union {
        u32_t seconds;
        u32_t ref_val; /* Reference temperature */
    };

    struct bt_gatt_ccc_cfg  ccc_cfg[BT_GATT_CCC_MAX];
    struct ess_meas_desc meas_desc;
};

struct read_es_measurement_rp {
    u16_t flags; /* Reserved for Future Use */
    u8_t sampling_function;
    u8_t measurement_period[3];
    u8_t update_interval[3];
    u8_t application;
    u8_t measurement_uncertainty;
} __packed;

struct es_trigger_setting_seconds {
    u8_t condition;
    u8_t sec[3];
} __packed;

struct es_trigger_setting_reference {
    u8_t condition;
    s16_t ref_val;
} __packed;


/****************************************************************************
* Private Data Definitions
***************************************************************************/

static struct ess_sensor            _temperature;
static struct ess_sensor            _humidity;
static struct ess_sensor            _ambient_light;
static struct ess_pressure_sensor   _baro_pressure;

static u8_t _is_temp_notify_enabled;
static u8_t _is_humidity_notify_enabled;
static u8_t _is_ambient_light_notify_enabled;
static u8_t _is_baro_pressure_notify_enabled;

static ssize_t read_u16_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset);
static ssize_t read_u32_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset);
static ssize_t read_es_measurement(struct bt_conn *conn,
                   const struct bt_gatt_attr *attr, void *buf,
                   u16_t len, u16_t offset);
static ssize_t read_valid_range(struct bt_conn *conn,
                     const struct bt_gatt_attr *attr, void *buf,
                     u16_t len, u16_t offset);
static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value);
static void rh_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value);
static void al_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value);
static void bp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value);
static ssize_t read_trigger_setting(struct bt_conn *conn,
                     const struct bt_gatt_attr *attr,
                     void *buf, u16_t len,
                     u16_t offset);


static struct bt_gatt_attr ess_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

    /* Temperature Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16_value, NULL, &_temperature.value),
    BT_GATT_CUD(TEMPERATURE_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
                    read_es_measurement, NULL, &_temperature.meas_desc),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &_temperature),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
               BT_GATT_PERM_READ, read_trigger_setting,
               NULL, &_temperature),
    BT_GATT_CCC(_temperature.ccc_cfg, temp_ccc_cfg_changed),

    /* Humidity Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16_value, NULL, &_humidity.value),
    BT_GATT_CUD(HUMIDITY_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &_humidity.meas_desc),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &_humidity),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
               BT_GATT_PERM_READ, read_trigger_setting,
               NULL, &_humidity),
    BT_GATT_CCC(_humidity.ccc_cfg, rh_ccc_cfg_changed),

    /* Ambient Light Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_IRRADIANCE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u16_value, NULL, &_ambient_light.value),
    BT_GATT_CUD(AMBIENT_LIGHT_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &_ambient_light.meas_desc),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &_ambient_light),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
               BT_GATT_PERM_READ, read_trigger_setting,
               NULL, &_ambient_light),
    BT_GATT_CCC(_ambient_light.ccc_cfg, al_ccc_cfg_changed),

    /* Barometric Pressure Sensor */
    BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_u32_value, NULL, &_baro_pressure.value),
    BT_GATT_CUD(BARO_PRESSURE_SENSOR_NAME, BT_GATT_PERM_READ),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
               read_es_measurement, NULL, &_baro_pressure.meas_desc),
    BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
                    BT_GATT_PERM_READ,
                    read_valid_range, NULL, &_baro_pressure),
    BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
               BT_GATT_PERM_READ, read_trigger_setting,
               NULL, &_baro_pressure),
    BT_GATT_CCC(_baro_pressure.ccc_cfg, bp_ccc_cfg_changed),
};

static struct bt_gatt_service ess_svc = BT_GATT_SERVICE(ess_attrs);


/****************************************************************************
* Public Data Definitions
***************************************************************************/


/****************************************************************************
* Private Function Definitions
***************************************************************************/

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


static ssize_t read_u16_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset)
{
    const u16_t *u16 = attr->user_data;
    u16_t value = sys_cpu_to_le16(*u16);

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
                 sizeof(value));
}

static ssize_t read_u32_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
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
    _is_temp_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void rh_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    _is_humidity_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void al_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    _is_ambient_light_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void bp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                 u16_t value)
{
    _is_baro_pressure_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}


static ssize_t read_es_measurement(struct bt_conn *conn,
                   const struct bt_gatt_attr *attr, void *buf,
                   u16_t len, u16_t offset)
{
    const struct ess_meas_desc *value = attr->user_data;
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

static ssize_t read_trigger_setting(struct bt_conn *conn,
                     const struct bt_gatt_attr *attr,
                     void *buf, u16_t len,
                     u16_t offset)
{
    const struct ess_sensor *sensor = attr->user_data;

    switch (sensor->condition) {
    /* Operand N/A */
    case ESS_TRIGGER_INACTIVE:
        /* fallthrough */
    case ESS_VALUE_CHANGED:
        return bt_gatt_attr_read(conn, attr, buf, len, offset,
                     &sensor->condition,
                     sizeof(sensor->condition));
    /* Seconds */
    case ESS_FIXED_TIME_INTERVAL:
        /* fallthrough */
    case ESS_NO_LESS_THAN_SPECIFIED_TIME: {
            struct es_trigger_setting_seconds rp;

            rp.condition = sensor->condition;
            int_to_le24(sensor->seconds, rp.sec);

            return bt_gatt_attr_read(conn, attr, buf, len, offset,
                         &rp, sizeof(rp));
        }
    /* Reference value */
    default: {
            struct es_trigger_setting_reference rp;

            rp.condition = sensor->condition;
            rp.ref_val = sys_cpu_to_le16(sensor->ref_val);

            return bt_gatt_attr_read(conn, attr, buf, len, offset,
                         &rp, sizeof(rp));
        }
    }
}

static bool check_condition(u8_t condition, s16_t old_val, s16_t new_val,
                s16_t ref_val)
{
    switch (condition) {
    case ESS_TRIGGER_INACTIVE:
        return false;
    case ESS_FIXED_TIME_INTERVAL:
    case ESS_NO_LESS_THAN_SPECIFIED_TIME:
        /* TODO: Check time requirements */
        return false;
    case ESS_VALUE_CHANGED:
        return new_val != old_val;
    case ESS_LESS_THAN_REF_VALUE:
        return new_val < ref_val;
    case ESS_LESS_OR_EQUAL_TO_REF_VALUE:
        return new_val <= ref_val;
    case ESS_GREATER_THAN_REF_VALUE:
        return new_val > ref_val;
    case ESS_GREATER_OR_EQUAL_TO_REF_VALUE:
        return new_val >= ref_val;
    case ESS_EQUAL_TO_REF_VALUE:
        return new_val == ref_val;
    case ESS_NOT_EQUAL_TO_REF_VALUE:
        return new_val != ref_val;
    default:
        return false;
    }
}


static void ess_temperature_sensor_init()
{
    int err;
    nv_sensor_data_t sensor_data;
    struct ess_meas_desc *meas_desc = &_temperature.meas_desc;

    err = nv_get_sensor_data(NV_SENSOR_TEMPERATURE, &sensor_data);
    if (err) {
        SYS_LOG_ERR("Failed to get temperature NV data");
        return;
    }

    meas_desc->sampling_func    = sensor_data.sampling_func;
    meas_desc->meas_period      = sensor_data.meas_period;
    meas_desc->update_interval  = sensor_data.update_interval;
    meas_desc->application      = sensor_data.application;
    meas_desc->meas_uncertainty = sensor_data.meas_uncertainty;

    _temperature.condition = ESS_VALUE_CHANGED;
    _temperature.lower_limit = -4000;
    _temperature.upper_limit = 8500;
}

static void ess_humidity_sensor_init()
{
    int err;
    nv_sensor_data_t sensor_data;
    struct ess_meas_desc *meas_desc = &_humidity.meas_desc;

    err = nv_get_sensor_data(NV_SENSOR_HUMIDITY, &sensor_data);
    if (err) {
        SYS_LOG_ERR("Failed to get humidity NV data");
        return;
    }

    meas_desc->sampling_func    = sensor_data.sampling_func;
    meas_desc->meas_period      = sensor_data.meas_period;
    meas_desc->update_interval  = sensor_data.update_interval;
    meas_desc->application      = sensor_data.application;
    meas_desc->meas_uncertainty = sensor_data.meas_uncertainty;

    _humidity.condition = ESS_VALUE_CHANGED;
    _humidity.lower_limit = 0;
    _humidity.upper_limit = 10000;
}

static void ess_ambient_light_sensor_init()
{
    int err;
    nv_sensor_data_t sensor_data;
    struct ess_meas_desc *meas_desc = &_ambient_light.meas_desc;

    err = nv_get_sensor_data(NV_SENSOR_AMBIENT_LIGHT, &sensor_data);
    if (err) {
        SYS_LOG_ERR("Failed to get ambient light NV data");
        return;
    }

    meas_desc->sampling_func    = sensor_data.sampling_func;
    meas_desc->meas_period      = sensor_data.meas_period;
    meas_desc->update_interval  = sensor_data.update_interval;
    meas_desc->application      = sensor_data.application;
    meas_desc->meas_uncertainty = sensor_data.meas_uncertainty;

    _ambient_light.condition = ESS_VALUE_CHANGED;
    _ambient_light.lower_limit = 0;
    // _ambient_light.upper_limit = 655350;
    _ambient_light.upper_limit = 65535;
}

static void ess_baro_pressure_sensor_init()
{
    int err;
    nv_sensor_data_t sensor_data;
    struct ess_meas_desc *meas_desc = &_baro_pressure.meas_desc;

    err = nv_get_sensor_data(NV_SENSOR_BARO_PRESSURE, &sensor_data);
    if (err) {
        SYS_LOG_ERR("Failed to get barometric pressure NV data");
        return;
    }

    meas_desc->sampling_func    = sensor_data.sampling_func;
    meas_desc->meas_period      = sensor_data.meas_period;
    meas_desc->update_interval  = sensor_data.update_interval;
    meas_desc->application      = sensor_data.application;
    meas_desc->meas_uncertainty = sensor_data.meas_uncertainty;

    _baro_pressure.condition = ESS_VALUE_CHANGED;
    _baro_pressure.lower_limit = 950000;
    _baro_pressure.upper_limit = 1050000;
}


/****************************************************************************
* Public Function Definitions
***************************************************************************/

void ess_init(void)
{
    bt_gatt_service_register(&ess_svc);

    ess_temperature_sensor_init();
    ess_humidity_sensor_init();
    ess_ambient_light_sensor_init();
    ess_baro_pressure_sensor_init();
}

void ess_temperature_update(s16_t new_value)
{
    bool notify = check_condition(_temperature.condition,
                    _temperature.value, new_value,
                    _temperature.ref_val);

    /* Update temperature value */
    _temperature.value = new_value;

    if (!_is_temp_notify_enabled) {
        return;
    }

    /* Trigger notification if conditions are met */
    if (notify) {
        bt_gatt_notify(NULL, &ess_attrs[2], &_temperature.value, sizeof(_temperature.value));
    }

}

void ess_humidity_update(s16_t new_value)
{
    bool notify = check_condition(_humidity.condition,
                      _humidity.value, new_value,
                      _humidity.ref_val);

    /* Update humidity value */
    _humidity.value = new_value;

    if (!_is_humidity_notify_enabled) {
        return;
    }

    /* Trigger notification if conditions are met */
    if (notify) {
        bt_gatt_notify(NULL, &ess_attrs[9], &_humidity.value, sizeof(_humidity.value));
    }

}

void ess_als_update(s16_t new_value)
{
    bool notify = check_condition(_ambient_light.condition,
                      _ambient_light.value, new_value,
                      _ambient_light.ref_val);

    /* Update ambient light value */
    _ambient_light.value = new_value;

    if (!_is_ambient_light_notify_enabled) {
        return;
    }

    /* Trigger notification if conditions are met */
    if (notify) {
        bt_gatt_notify(NULL, &ess_attrs[16], &_ambient_light.value, sizeof(_ambient_light.value));
    }

}

void ess_baro_press_update(u32_t new_value)
{
    bool notify = check_condition(_baro_pressure.condition,
                      _baro_pressure.value, new_value,
                      _baro_pressure.ref_val);

    /* Update barometric pressure value */
    _baro_pressure.value = new_value;

    if (!_is_baro_pressure_notify_enabled) {
        return;
    }

    /* Trigger notification if conditions are met */
    if (notify) {
        bt_gatt_notify(NULL, &ess_attrs[23], &_baro_pressure.value, sizeof(_baro_pressure.value));
    }

}

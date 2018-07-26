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

#ifndef BUILD_VERSION
#define BUILD_VERSION UNKNOWN
#endif

#define DEVICE_SOFTWARE_VERSION     STRINGIFY(BUILD_VERSION)
#define DEVICE_HARDWARE_VERSION     STRINGIFY(BOARD_VARIANT)

#define ESS_ADV_SLOW BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
                       BT_GAP_ADV_SLOW_INT_MIN, \
                       BT_GAP_ADV_SLOW_INT_MAX)

#define ESS_ADV_FAST_1 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
                       BT_GAP_ADV_FAST_INT_MIN_1, \
                       BT_GAP_ADV_FAST_INT_MAX_1)

#define ESS_ADV_FAST_2 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
                       BT_GAP_ADV_FAST_INT_MIN_2, \
                       BT_GAP_ADV_FAST_INT_MAX_2)

// Default Device Information Service data
static dis_data_t dis_data = {
    .sw_rev = DEVICE_SOFTWARE_VERSION,
    .hw_rev = DEVICE_HARDWARE_VERSION,
};


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

    dis_init(&dis_data);
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

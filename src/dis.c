/** @file
 *  @brief DIS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include "dis.h"

// Device Information Service data
static dis_data_t *dis;

// Function prototype for reading software version
static ssize_t read_sw_vers(struct bt_conn *conn,
              const struct bt_gatt_attr *attr, void *buf,
              u16_t len, u16_t offset);

// Function prototype for reading hardware version
static ssize_t read_hw_vers(struct bt_conn *conn,
              const struct bt_gatt_attr *attr, void *buf,
              u16_t len, u16_t offset);

/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION,
                   BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                   read_sw_vers, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION,
                   BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                   read_hw_vers, NULL, NULL),
};
static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);


/**
* @private
* @brief Callback to read the software version characteristic
*/
static ssize_t read_sw_vers(struct bt_conn *conn,
              const struct bt_gatt_attr *attr, void *buf,
              u16_t len, u16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, dis->sw_rev,
                 strlen(dis->sw_rev));
}

/**
* @private
* @brief Callback to read the hardware version characteristic
*/
static ssize_t read_hw_vers(struct bt_conn *conn,
              const struct bt_gatt_attr *attr, void *buf,
              u16_t len, u16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, dis->hw_rev,
                 strlen(dis->hw_rev));
}


/****************************************************************************
* Public Function Definitions
***************************************************************************/

void dis_init(dis_data_t *dis_data)
{
    dis = dis_data;

    bt_gatt_service_register(&dis_svc);
}
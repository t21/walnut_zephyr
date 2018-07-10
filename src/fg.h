/*
 *
 *
 */

#ifndef _FG_H_
#define _FG_H_

#include <stdint.h>

typedef void (*fg_update_cb_t)(uint8_t battery_capacity);

void fg_init(fg_update_cb_t cb);

#endif /* _FG_H_ */
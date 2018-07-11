/** @file
 *  @brief DIS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DIS_H_
#define _DIS_H_

typedef struct {
    char *sw_rev;
    char *hw_rev;
} dis_data_t;

void dis_init(dis_data_t *dis_data);

#endif  /* _DIS_H_ */

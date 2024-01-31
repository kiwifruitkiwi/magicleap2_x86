/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/* Copyright (c) 2020 The Linux Foundation. All rights reserved.
 */

#ifndef DIAG_MHI_H
#define DIAG_MHI_H

struct mhi_device;

int diag_mhi_send(struct mhi_device *mhi_dev, unsigned char *buf, int len);

#endif

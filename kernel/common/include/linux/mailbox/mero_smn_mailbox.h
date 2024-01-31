/*
 * Mailbox register definition for Mero.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __MERO_SMN_MAILBOX_H__
#define __MERO_SMN_MAILBOX_H__

/* SMN BASE ADDRESS */
#define SMN_BASE_ADDRESS                0x19500000

/* PL320 SMN BASE ADDRESS */
#define CVIP_X86_MAILBOX_BASE_ADDRESS	(SMN_BASE_ADDRESS+0x00400000)
#define CVIP_ISP_MAILBOX_BASE_ADDRESS	(SMN_BASE_ADDRESS+0x00410000)
#define CVIP_GIC0_MAILBOX_BASE_ADDRESS	(SMN_BASE_ADDRESS+0x00420000)

/* PL320 SMU Register definitions
 * These registers are secured used by SMU for power notification
 */
#define SMU_M1SEND		(CVIP_GIC0_MAILBOX_BASE_ADDRESS+0x60)
#define SMU_M1DR0		(CVIP_GIC0_MAILBOX_BASE_ADDRESS+0x64)
#define SMU_M1DR1		(CVIP_GIC0_MAILBOX_BASE_ADDRESS+0x68)
#define SMU_M1DR2		(CVIP_GIC0_MAILBOX_BASE_ADDRESS+0x6c)
#define SMU_M1DR3		(CVIP_GIC0_MAILBOX_BASE_ADDRESS+0x70)

/* M1SEND Register */
#define SMU_M1SEND_SEND_STATUS	0x0001	/* bit0: Send status */

#endif /* __MERO_SMN_MAILBOX_H__ */

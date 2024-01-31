/*
 * Copyright (C) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 */

#ifndef __SMU_PROTOCOL_H__
#define __SMU_PROTOCOL_H__
#include <linux/types.h>

/**
 * SMU message list
 * Used for smu_msg.id(See definition of smu_msg)
 */
enum smu_msg_id {
	CVSMC_MSG_testmessage		= 0x1,
	CVSMC_MSG_getsmuversion,
	CVSMC_MSG_setsoftmina55clk,	/* CMD_SET_CLOCK_VALUE */
	CVSMC_MSG_setsoftminq6clk,
	CVSMC_MSG_setsoftminc5clk,
	CVSMC_MSG_setsoftminnicclk,
	CVSMC_MSG_setsoftmina55twoclk,
	CVSMC_MSG_setcoresightclk,
	CVSMC_MSG_setdecmpressclk,
	CVSMC_MSG_cvready		= 0xa,
	CVSMC_MSG_pciehotadd,
	CVSMC_MSG_pciehotremove,
	CVSMC_MSG_pciehpreset,
	CVSMC_MSG_getcurrentvoltage,
	CVSMC_MSG_getcurrentcurrent,
	CVSMC_MSG_getcurrentfreq,	/* CMD_GET_CLOCK_VALUE */
	CVSMC_MSG_getcurrentpower,
	CVSMC_MSG_getaveragepower,
	CVSMC_MSG_getaveragetemperature,
	CVSMC_MSG_getcurrenttemperature	= 0x14,
	CVSMC_MSG_setaveragepowertimeconstant,
	CVSMC_MSG_setaveragetemperaturetimeconstant,
	CVSMC_MSG_setmitigationendhysteresis,
	/*
	 * 1 -> CVIP-Offhead entry 2 -> CVIP-Offhead exit to active (S0)
	 * (Used as a CVIP pre-condition to enter whisper)
	 */
	CVSMC_MSG_cvipoffheadstateindication,
	CVSMC_MSG_forcefclkdpmlevel = 0x19,
	CVSMC_MSG_unforcefclkdpmlevel = 0x1A,
	CVSMC_MSG_calculatelclkbusyfromiohconly = 0x1B,
	CVSMC_MSG_updatelclkdpmconstants = 0x1C,
	SMU_MAX_MSG_COUNT,
	MSG_INVALID		= 0xff,
};

/**
 * Rail ID
 * Used for smu_msg.arg(See definition of smu_msg)
 */
enum smu_rail_id {
	RAIL_CPU,
	RAIL_SOC,
	RAIL_GFX,
	RAIL_CVIP,
	RAIL_INVALID
};

/**
 * IP ID
 * Used for smu_msg.arg(See definition of smu_msg)
 */
enum smu_ip_id {
	IP_CPU0,
	IP_CPU1,
	IP_CPU2,
	IP_CPU3,
	IP_SOC,
	IP_GFX,
	IP_CVIP,
	IP_INVALID
};

/**
 * Clock ID
 * Used for smu_msg.arg(See definition of smu_msg)
 */
enum smu_clk_id {
	A55CLK_ID,
	A55TWOCLK_ID,
	C5CLK_ID,
	Q6CLK_ID,
	NIC400CLK_ID,
	CORESIGHTCLK_ID,
	DECOMPRESSCLK_ID,
	CLK_INVALID
};

/**
 *  Off head state
 *  Used for argument of CVSMC_MSG_cvipoffheadstateindication msg
 */
enum offhead_state {
	OFFHEAD_ENTER = 1,
	OFFHEAD_EXIT = 2,
	STATE_INVALID
};

struct smu_msg {
	enum smu_msg_id id; /* msg id */
	u32 arg;  /* msg argument */
	u32 resp; /* smu return value */
	bool rst; /* true if send msg successfully */
};

/**
 * @brief initialize smu message.
 * @param[in] msg: message to be initialized.
 * @param[in] id: message id.
 * @param[in] arg: argment of message.
 */
static inline void smu_msg(struct smu_msg *msg, enum smu_msg_id id, u32 arg)
{
	msg->id = id;
	msg->arg = arg;
	msg->resp = 0;
};

/**
 * @brief send messages to smu firmware.
 *	NOTE: Can't be called in interrupt context.
 * @param[in] base: message base.
 * @param[in] num: message number.
 * @return the number of messages that been sent successfully.
 */
int smu_send_msgs(struct smu_msg *base, u32 num);

#ifdef CONFIG_MERO_SMU
/**
 * @brief send messages to smu firmware.
 *	NOTE: Can't be called in interrupt context.
 * @param[in] msg: message to be sent.
 * @return true if msg is sent successfully, otherwise return false.
 */
bool smu_send_single_msg(struct smu_msg *msg);

/**
 * @brief initiate SMU boot handshake.
 */
void smu_boot_handshake(void);
#else
static inline bool smu_send_single_msg(struct smu_msg *msg)
{
	return true;
}

static inline void smu_boot_handshake(void)
{
}
#endif

#endif

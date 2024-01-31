#ifndef _CMD_RESP_H_
#define _CMD_RESP_H_

/*
 *sizeof MetaInfo_t is 628
 *sizeof FrameInfo_t is 7224
 *sizeof CmdSetSensorProp_t is 40
 *sizeof CmdSetSensorHdrProp_t is 76
 *sizeof CmdSetDeviceScript_t is 200
 *sizeof CmdAwbSetLscMatrix_t is 2312
 *sizeof CmdAwbSetLscSector_t is 64
 *sizeof CmdFrameControl_t is 556
 *sizeof CmdSendFrameInfo_t is 7224
 */

#include "paramtypes.h"

#define FW_VERSION_MAJOR_SHIFT		(24)
#define FW_VERSION_MINOR_SHIFT		(16)
#define FW_VERSION_REVISION_SHIFT	(0)

#define FW_VERSION_MAJOR_MASK		(0xff << FW_VERSION_MAJOR_SHIFT)
#define FW_VERSION_MINOR_MASK		(0xff << FW_VERSION_MINOR_SHIFT)
#define FW_VERSION_REVISION_MASK	(0xffff << FW_VERSION_REVISION_SHIFT)

#define FW_VERSION_MAJOR		(0x2)
#define FW_VERSION_MINOR		(0xA)
#define FW_VERSION_REVISION		(0x5)
#define FW_VERSION_STRING "ISP Firmware Version: 2.A.5"
#define FW_VERSION (((FW_VERSION_MAJOR & 0xff) << FW_VERSION_MAJOR_SHIFT) |\
		((FW_VERSION_MINOR & 0xff) << FW_VERSION_MINOR_SHIFT) |\
		((FW_VERSION_REVISION & 0xffff) << FW_VERSION_REVISION_SHIFT))




//-----------                                        ------------
//|         |       ---------------------------      |          |
//|         |  ---->|  NonBlock Command       |----> |          |
//|         |       ---------------------------      |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       ---------------------------      |          |
//|         |  ---->|  Stream1 Block Command  |----> |          |
//|         |       ---------------------------      |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       ---------------------------      |          |
//|         |  ---->|  Stream2 Block Command  |----> |          |
//|         |       ---------------------------      |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       ---------------------------      |          |
//|         |  ---->|  Stream3 Block Command  |----> |          |
//|         |       ---------------------------      |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |                                        |          |
//|  HOST   |                                        | Firmware |
//|         |                                        |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       --------------------------       |          |
//|         |  <----|  Global Response       |<----  |          |
//|         |       --------------------------       |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       --------------------------       |          |
//|         |  <----|  Stream1 response      |<----  |          |
//|         |       --------------------------       |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       --------------------------       |          |
//|         |  <----|  Stream2 Response      |<----  |          |
//|         |       --------------------------       |          |
//|         |                                        |          |
//|         |                                        |          |
//|         |       --------------------------       |          |
//|         |  <----|  Stream3 Response      |<----  |          |
//|         |       --------------------------       |          |
//|         |                                        |          |
//-----------                                        ------------

//==================================
//Ring Buffer Registers Allocation:
//==================================
// NonBlock Command:
//mmISP_RB_BASE_LO1
//mmISP_RB_BASE_HI1, mmISP_RB_SIZE1, mmISP_RB_RPTR1, mmISP_RB_WPTR1
// Stream1 Block Command:
//mmISP_RB_BASE_LO2
//mmISP_RB_BASE_HI2, mmISP_RB_SIZE2, mmISP_RB_RPTR2, mmISP_RB_WPTR2
// Stream2 Block Command:
//mmISP_RB_BASE_LO3
//mmISP_RB_BASE_HI3, mmISP_RB_SIZE3, mmISP_RB_RPTR3, mmISP_RB_WPTR3
// Stream3 Block Command:
//mmISP_RB_BASE_LO4
//mmISP_RB_BASE_HI4, mmISP_RB_SIZE4, mmISP_RB_RPTR4, mmISP_RB_WPTR4
// Global Response:
//mmISP_RB_BASE_LO5
//mmISP_RB_BASE_HI5, mmISP_RB_SIZE5, mmISP_RB_RPTR5, mmISP_RB_WPTR5
// Stream1 Response:
//mmISP_RB_BASE_LO6
//mmISP_RB_BASE_HI6, mmISP_RB_SIZE6, mmISP_RB_RPTR6, mmISP_RB_WPTR6
// Stream2 Response:
//mmISP_RB_BASE_LO7
//mmISP_RB_BASE_HI7, mmISP_RB_SIZE7, mmISP_RB_RPTR7, mmISP_RB_WPTR7
// Stream3 Response:
//mmISP_RB_BASE_LO8
//mmISP_RB_BASE_HI8, mmISP_RB_SIZE8, mmISP_RB_RPTR8, mmISP_RB_WPTR8

/*-------------------------------------------------------------------------*/
/*			 Command ID List				   */
/*-------------------------------------------------------------------------*/

/* cmdId is in the format of following type:
 * |<-Bit31 ~ Bit24->|<-Bit23 ~ Bit16->|<-Bit15 ~ Bit0->|
 * |      type       |      group      |       id       |
 */

#define CMD_TYPE_SHIFT			(24)
#define CMD_TYPE_MASK			(0xff << CMD_TYPE_SHIFT)
#define CMD_GROUP_SHIFT			(16)
#define CMD_GROUP_MASK			(0xff << CMD_GROUP_SHIFT)
#define CMD_ID_MASK			(0xffff)


#define CMD_TYPE_GLOBAL_CONTROL		(0x1 << CMD_TYPE_SHIFT)
#define CMD_TYPE_STREAM_CONTROL		(0x2 << CMD_TYPE_SHIFT)


#define GET_CMD_TYPE_VALUE(cmdId) ((cmdId & CMD_TYPE_MASK) >> CMD_TYPE_SHIFT)
#define GET_CMD_GROUP_VALUE(cmdId) ((cmdId & CMD_GROUP_MASK) >> CMD_GROUP_SHIFT)
#define GET_CMD_ID_VALUE(cmdId)		(cmdId & CMD_ID_MASK)


/*Groups for CMD_TYPE_GLOBAL_CONTROL */
#define CMD_GROUP_GLOBAL_GENERAL	(0x1 << CMD_GROUP_SHIFT)
#define CMD_GROUP_GLOBAL_DEBUG		(0x2 << CMD_GROUP_SHIFT)
#define CMD_GROUP_GLOBAL_SENSOR		(0x3 << CMD_GROUP_SHIFT)
#define CMD_GROUP_GLOBAL_DEVICE		(0x4 << CMD_GROUP_SHIFT)
#define CMD_GROUP_GLOBAL_I2C		(0x5 << CMD_GROUP_SHIFT)
#define CMD_GROUP_GLOBAL_MAX		(0x6 << CMD_GROUP_SHIFT)

/*Groups for CMD_TYPE_STREAM_CONTROL */
#define CMD_GROUP_STREAM_GENERAL	(0x1 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_PREVIEW	(0x2 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_VIDEO		(0x3 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_ZSL		(0x4 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_RAW		(0x5 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_CONTROL	(0x6 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_INFO		(0x7 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_AE		(0x8 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_AWB		(0x9 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_AF		(0xa << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_CPROC		(0xb << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_GAMMA		(0xc << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_STNR		(0xd << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_BLS		(0xe << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_DEGAMMA	(0xf << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_DPF		(0x10 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_DPCC		(0x11 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_CAC		(0x12 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_WDR		(0x13 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_CNR		(0x14 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_DEMOSAIC	(0x15 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_FILTER		(0x16 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_FRAME		(0x17 << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_BUFFER		(0x18 << CMD_GROUP_SHIFT)


/*++++++++++++++++++++++++++*/
/*Global Control Command */
/*++++++++++++++++++++++++++*/

/*General Command */
#define CMD_ID_BIND_STREAM	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x1)
#define CMD_ID_UNBIND_STREAM	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x2)
#define CMD_ID_GET_FW_VERSION	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x3)
#define CMD_ID_SET_CLK_INFO	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x4)
#define CMD_ID_PREPARE_CHANGE_ICLK	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x5)
#define CMD_ID_CONFIG_GMC_PREFETCH	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x6)
#define CMD_ID_SET_RESIZE_TABLE	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_GENERAL | 0x7)

#define CMD_ID_READ_REG	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x1)
#define CMD_ID_WRITE_REG	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x2)
#define CMD_ID_LOG_SET_LEVEL	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x3)
#define CMD_ID_LOG_ENABLE_MOD	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x4)
#define CMD_ID_LOG_DISABLE_MOD	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x5)
#define CMD_ID_PROFILER_GET_RESULT	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x6)
#define CMD_ID_SET_LOG_MOD_EXT	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x7)
#define CMD_ID_PROFILER_GET_ID_RESULT	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x8)
#define CMD_ID_PROFILER_SET_ID	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEBUG | 0x9)

/*Sensor Related Command */
#define CMD_ID_SET_SENSOR_PROP	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_SENSOR | 0x1)
#define CMD_ID_SET_SENSOR_HDR_PROP	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_SENSOR | 0x2)
#define CMD_ID_SET_SENSOR_CALIBDB	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_SENSOR | 0x3)
#define CMD_ID_SET_SENSOR_M2M_CALIBDB	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_SENSOR | 0x4)
#define CMD_ID_SET_SENSOR_EMB_PROP	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_SENSOR | 0x5)

/*Device Control Command */
#define CMD_ID_ENABLE_DEVICE_SCRIPT_CONTROL	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x1)
#define CMD_ID_DISABLE_DEVICE_SCRIPT_CONTROL	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x2)
#define CMD_ID_SET_DEVICE_SCRIPT	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x3)
#define CMD_ID_ACK_SENSOR_GROUP_HOLD	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x4)
#define CMD_ID_ACK_SENSOR_GROUP_RELEASE	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x5)
#define CMD_ID_ACK_SENSOR_SET_AGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x6)
#define CMD_ID_ACK_SENSOR_SET_DGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x7)
#define CMD_ID_ACK_SENSOR_SET_ITIME	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x8)
#define CMD_ID_ACK_SENSOR_SET_HDR_HIGH_ITIME	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x9)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xa)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME_RATIO \
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xb)
#define CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_ITIME	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xc)
#define CMD_ID_ACK_SENSOR_SET_HDR_HIGH_AGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xd)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xe)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN_RATIO \
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0xf)
#define CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_AGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x10)
#define CMD_ID_ACK_SENSOR_SET_HDR_HIGH_DGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x11)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x12)
#define CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN_RATIO \
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x13)
#define CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_DGAIN	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x14)
#define CMD_ID_ACK_LENS_SET_POS	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x15)
#define CMD_ID_ACK_FLASH_SET_POWER	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x16)
#define CMD_ID_ACK_FLASH_SET_ON	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x17)
#define CMD_ID_ACK_FLASH_SET_OFF	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_DEVICE | 0x18)

/*I2c Control Command */
#define CMD_ID_SEND_I2C_MSG	\
		(CMD_TYPE_GLOBAL_CONTROL | CMD_GROUP_GLOBAL_I2C | 0x1)


/*++++++++++++++++++++++++++*/
/*Stream Control Command */
/*++++++++++++++++++++++++++*/

/*GeneralCommand */
#define CMD_ID_SET_STREAM_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x1)
#define CMD_ID_SET_INPUT_SENSOR	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x2)
#define CMD_ID_SET_ACQ_WINDOW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x3)
#define CMD_ID_ENABLE_CALIB	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x4)
#define CMD_ID_DISABLE_CALIB	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x5)
#define CMD_ID_SET_ASPECT_RATIO_WINDOW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x6)
#define CMD_ID_SET_ZOOM_WINDOW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x7)
#define CMD_ID_SET_IR_ILLU_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x8)
#define CMD_ID_SET_CSM_YUV_RANGE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GENERAL | 0x9)

/*PreviewCommand */
#define CMD_ID_SET_PREVIEW_OUT_PROP	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_PREVIEW | 0x1)
#define CMD_ID_SET_PREVIEW_FRAME_RATE_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_PREVIEW | 0x2)
#define CMD_ID_ENABLE_PREVIEW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_PREVIEW | 0x3)
#define CMD_ID_DISABLE_PREVIEW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_PREVIEW | 0x4)

/*VideoCommand */
#define CMD_ID_SET_VIDEO_OUT_PROP	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_VIDEO | 0x1)
#define CMD_ID_SET_VIDEO_FRAME_RATE_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_VIDEO | 0x2)
#define CMD_ID_ENABLE_VIDEO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_VIDEO | 0x3)
#define CMD_ID_DISABLE_VIDEO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_VIDEO | 0x4)

/*ZslCommand */
#define CMD_ID_SET_ZSL_OUT_PROP	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_ZSL | 0x1)
#define CMD_ID_SET_ZSL_FRAME_RATE_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_ZSL | 0x2)
#define CMD_ID_ENABLE_ZSL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_ZSL | 0x3)
#define CMD_ID_DISABLE_ZSL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_ZSL | 0x4)
#define CMD_ID_CAPTURE_YUV	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_ZSL | 0x5)

/*RawCommand */
#define CMD_ID_SET_RAW_ZSL_FRAME_RATE_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_RAW | 0x1)
#define CMD_ID_ENABLE_RAW_ZSL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_RAW | 0x2)
#define CMD_ID_DISABLE_RAW_ZSL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_RAW | 0x3)
#define CMD_ID_CAPTURE_RAW	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_RAW | 0x4)
#define CMD_ID_SET_RAW_PKT_FMT	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_RAW | 0x5)

/*StreamCommand */
#define CMD_ID_START_STREAM	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CONTROL | 0x1)
#define CMD_ID_STOP_STREAM	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CONTROL | 0x2)
#define CMD_ID_RESET_STREAM	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CONTROL | 0x3)
#define CMD_ID_IGNORE_META	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CONTROL | 0x4)

/*Frame Rate Info Command */
#define CMD_ID_SET_FRAME_RATE_INFO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_INFO | 0x1)
#define CMD_ID_SET_FRAME_CONTROL_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_INFO | 0x2)

/*AECommand */
#define CMD_ID_AE_SET_MODE	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x1)
#define CMD_ID_AE_SET_FLICKER	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x2)
#define CMD_ID_AE_SET_REGION	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x3)
#define CMD_ID_AE_LOCK	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x4)
#define CMD_ID_AE_UNLOCK	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x5)
#define CMD_ID_AE_SET_ANALOG_GAIN_RANGE	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x6)
#define CMD_ID_AE_SET_DIGITAL_GAIN_RANGE	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x7)
#define CMD_ID_AE_SET_ITIME_RANGE	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x8)
#define CMD_ID_AE_SET_ANALOG_GAIN	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x9)
#define CMD_ID_AE_SET_DIGITAL_GAIN	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xa)
#define CMD_ID_AE_SET_ITIME	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xb)
#define CMD_ID_AE_SET_ISO	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xc)
#define CMD_ID_AE_GET_ISO_CAPABILITY	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xd)
#define CMD_ID_AE_SET_EV	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xe)
#define CMD_ID_AE_GET_EV_CAPABILITY	\
			(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0xf)
#define CMD_ID_AE_PRECAPTURE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x10)
#define CMD_ID_AE_ENABLE_HDR	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x11)
#define CMD_ID_AE_DISABLE_HDR	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x12)
#define CMD_ID_AE_SET_FLASH_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x13)
#define CMD_ID_AE_SET_FLASH_POWER_LEVEL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x14)
#define CMD_ID_AE_SET_RED_EYE_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x15)
#define CMD_ID_AE_SET_START_EXPOSURE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x16)
#define CMD_ID_AE_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x17)
#define CMD_ID_AE_SET_SETPOINT	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x18)
#define CMD_ID_AE_GET_SETPOINT	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x19)
#define CMD_ID_AE_SET_ISO_RANGE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x1a)
#define CMD_ID_AE_GET_ISO_RANGE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AE | 0x1b)

/*AWBCommand */
#define CMD_ID_AWB_SET_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x1)
#define CMD_ID_AWB_SET_REGION	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x2)
#define CMD_ID_AWB_SET_LIGHT	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x3)
#define CMD_ID_AWB_SET_COLOR_TEMPERATURE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x4)
#define CMD_ID_AWB_SET_WB_GAIN	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x5)
#define CMD_ID_AWB_GET_WB_GAIN	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x6)
#define CMD_ID_AWB_SET_CC_MATRIX	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x7)
#define CMD_ID_AWB_GET_CC_MATRIX	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x8)
#define CMD_ID_AWB_SET_CC_OFFSET	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x9)
#define CMD_ID_AWB_GET_CC_OFFSET	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xa)
#define CMD_ID_AWB_SET_LSC_MATRIX	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xb)
#define CMD_ID_AWB_GET_LSC_MATRIX	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xc)
#define CMD_ID_AWB_SET_LSC_SECTOR	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xd)
#define CMD_ID_AWB_GET_LSC_SECTOR	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xe)
#define CMD_ID_AWB_LOCK	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0xf)
#define CMD_ID_AWB_UNLOCK	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x10)
#define CMD_ID_AWB_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x11)
#define CMD_ID_AWB_SET_AWBM_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x12)
#define CMD_ID_AWB_GET_AWBM_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x13)
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
#define CMD_ID_AWB_SET_WB_GAIN_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x14)
#define CMD_ID_AWB_GET_WB_GAIN_RATIO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AWB | 0x15)
#endif

/*AFCommand */
#define CMD_ID_AF_SET_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x1)
#define CMD_ID_AF_SET_LENS_RANGE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x2)
#define CMD_ID_AF_SET_LENS_POS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x3)
#define CMD_ID_AF_SET_REGION	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x4)
#define CMD_ID_AF_TRIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x5)
#define CMD_ID_AF_CANCEL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x6)
#define CMD_ID_AF_LOCK	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x7)
#define CMD_ID_AF_UNLOCK	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x8)
#define CMD_ID_AF_SET_FOCUS_ASSIST_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0x9)
#define CMD_ID_AF_SET_FOCUS_ASSIST_POWER_LEVEL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0xa)
#define CMD_ID_AF_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_AF | 0xb)

/*CPROCCommand */
#define CMD_ID_CPROC_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x1)
#define CMD_ID_CPROC_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x2)
#define CMD_ID_CPROC_SET_PROP	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x3)
#define CMD_ID_CPROC_SET_CONTRAST	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x4)
#define CMD_ID_CPROC_SET_BRIGHTNESS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x5)
#define CMD_ID_CPROC_SET_SATURATION	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x6)
#define CMD_ID_CPROC_SET_HUE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x7)
#define CMD_ID_CPROC_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CPROC | 0x8)

/*GAMMA Command */
#define CMD_ID_GAMMA_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x1)
#define CMD_ID_GAMMA_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x2)
#define CMD_ID_GAMMA_SET_CURVE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x3)
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
#define CMD_ID_GAMMA_MODE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x4)
#define CMD_ID_GAMMA_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x5)
#else
#define CMD_ID_GAMMA_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_GAMMA | 0x4)
#endif

/*STNR Command */
#define CMD_ID_SNR_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x1)
#define CMD_ID_SNR_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x2)
#define CMD_ID_TNR_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x3)
#define CMD_ID_TNR_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x4)
#define CMD_ID_STNR_UPDATE_PARAM	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x5)
#define CMD_ID_STNR_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_STNR | 0x6)

/*BLSCommand */
#define CMD_ID_BLS_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_BLS | 0x1)
#define CMD_ID_BLS_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_BLS | 0x2)
#define CMD_ID_BLS_SET_BLACK_LEVEL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_BLS | 0x3)
#define CMD_ID_BLS_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_BLS | 0x4)

/*DEGAMMACommand */
#define CMD_ID_DEGAMMA_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEGAMMA | 0x1)
#define CMD_ID_DEGAMMA_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEGAMMA | 0x2)
#define CMD_ID_DEGAMMA_SET_CURVE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEGAMMA | 0x3)
#define CMD_ID_DEGAMMA_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEGAMMA | 0x4)

/*DPFCommand */
#define CMD_ID_DPF_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPF | 0x1)
#define CMD_ID_DPF_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPF | 0x2)
#define CMD_ID_DPF_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPF | 0x3)
#define CMD_ID_DPF_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPF | 0x4)

/*DPCCCommand */
#define CMD_ID_DPCC_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPCC | 0x1)
#define CMD_ID_DPCC_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPCC | 0x2)
#define CMD_ID_DPCC_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPCC | 0x3)
#define CMD_ID_DPCC_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DPCC | 0x4)

/*CACCommand */
#define CMD_ID_CAC_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CAC | 0x1)
#define CMD_ID_CAC_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CAC | 0x2)
#define CMD_ID_CAC_CONFIG	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CAC | 0x3)
#define CMD_ID_CAC_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CAC | 0x4)

/*WDRCommand */
#define CMD_ID_WDR_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_WDR | 0x1)
#define CMD_ID_WDR_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_WDR | 0x2)
#define CMD_ID_WDR_SET_CURVE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_WDR | 0x3)
#define CMD_ID_WDR_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_WDR | 0x4)

/*CNRCommand */
#define CMD_ID_CNR_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CNR | 0x1)
#define CMD_ID_CNR_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CNR | 0x2)
#define CMD_ID_CNR_SET_PARAM	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CNR | 0x3)
#define CMD_ID_CNR_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_CNR | 0x4)

/*DEMOSAICCommand */
#define CMD_ID_DEMOSAIC_SET_THRESH	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEMOSAIC | 0x1)
#define CMD_ID_DEMOSAIC_GET_THRESH	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEMOSAIC | 0x2)
#define CMD_ID_DEMOSAIC_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_DEMOSAIC | 0x3)

/*FILTERCommand */
#define CMD_ID_FILTER_ENABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FILTER | 0x1)
#define CMD_ID_FILTER_DISABLE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FILTER | 0x2)
//For de-noising filter
#define CMD_ID_FILTER_SET_CURVE	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FILTER | 0x3)
#define CMD_ID_FILTER_GET_STATUS	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FILTER | 0x4)
//For sharpening filter
#define CMD_ID_FILTER_SET_SHARPEN	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FILTER | 0x5)

/*Frame ControlCommand */
#define CMD_ID_SEND_FRAME_CONTROL	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FRAME | 0x1)
#define CMD_ID_SEND_MODE3_FRAME_INFO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FRAME | 0x2)
#define CMD_ID_SEND_MODE4_FRAME_INFO	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_FRAME | 0x3)

/*BufferCommand */
#define CMD_ID_SEND_BUFFER	\
		(CMD_TYPE_STREAM_CONTROL | CMD_GROUP_STREAM_BUFFER | 0x1)


/*-------------------------------------------------------------------------*/
/*			Response ID List				   */
/*-------------------------------------------------------------------------*/

/* respId is in the format of following type:
 * |<-Bit31 ~ Bit24->|<-Bit23 ~ Bit16->|<-Bit15 ~ Bit0->|
 * |      0          |      group      |       id       |
 */

#define RESP_GROUP_SHIFT		(16)
#define RESP_GROUP_MASK			(0xff << RESP_GROUP_SHIFT)

#define GET_RESP_GROUP_VALUE(respId) \
	((respId & RESP_GROUP_MASK) >> RESP_GROUP_SHIFT)
#define GET_RESP_ID_VALUE(respId)		(respId & 0xffff)

#define RESP_GROUP_GENERAL			(0x1 << RESP_GROUP_SHIFT)
#define RESP_GROUP_ERROR			(0x2 << RESP_GROUP_SHIFT)
#define RESP_GROUP_SENSOR			(0x3 << RESP_GROUP_SHIFT)
#define RESP_GROUP_LENS				(0x4 << RESP_GROUP_SHIFT)
#define RESP_GROUP_FLASH			(0x5 << RESP_GROUP_SHIFT)


/*GeneralResponse */
#define RESP_ID_CMD_DONE			(RESP_GROUP_GENERAL | 0x1)
#define RESP_ID_FRAME_DONE			(RESP_GROUP_GENERAL | 0x2)
#define RESP_ID_FRAME_INFO			(RESP_GROUP_GENERAL | 0x3)
#define RESP_ID_ERROR				(RESP_GROUP_GENERAL | 0x4)
#define RESP_ID_HEART_BEAT			(RESP_GROUP_GENERAL | 0x5)

/*SensorResponse */
#define RESP_ID_SENSOR_GROUP_HOLD		(RESP_GROUP_SENSOR | 0x1)
#define RESP_ID_SENSOR_GROUP_RELEASE		(RESP_GROUP_SENSOR | 0x2)
#define RESP_ID_SENSOR_SET_AGAIN		(RESP_GROUP_SENSOR | 0x3)
#define RESP_ID_SENSOR_SET_DGAIN		(RESP_GROUP_SENSOR | 0x4)
#define RESP_ID_SENSOR_SET_ITIME		(RESP_GROUP_SENSOR | 0x5)
#define RESP_ID_SENSOR_SET_HDR_HIGH_ITIME	(RESP_GROUP_SENSOR | 0x6)
#define RESP_ID_SENSOR_SET_HDR_LOW_ITIME	(RESP_GROUP_SENSOR | 0x7)
#define RESP_ID_SENSOR_SET_HDR_LOW_ITIME_RATIO	(RESP_GROUP_SENSOR | 0x8)
#define RESP_ID_SENSOR_SET_HDR_EQUAL_ITIME	(RESP_GROUP_SENSOR | 0x9)
#define RESP_ID_SENSOR_SET_HDR_HIGH_AGAIN	(RESP_GROUP_SENSOR | 0xa)
#define RESP_ID_SENSOR_SET_HDR_LOW_AGAIN	(RESP_GROUP_SENSOR | 0xb)
#define RESP_ID_SENSOR_SET_HDR_LOW_AGAIN_RATIO	(RESP_GROUP_SENSOR | 0xc)
#define RESP_ID_SENSOR_SET_HDR_EQUAL_AGAIN	(RESP_GROUP_SENSOR | 0xd)
#define RESP_ID_SENSOR_SET_HDR_HIGH_DGAIN	(RESP_GROUP_SENSOR | 0xe)
#define RESP_ID_SENSOR_SET_HDR_LOW_DGAIN	(RESP_GROUP_SENSOR | 0xf)
#define RESP_ID_SENSOR_SET_HDR_LOW_DGAIN_RATIO	(RESP_GROUP_SENSOR | 0x10)
#define RESP_ID_SENSOR_SET_HDR_EQUAL_DGAIN	(RESP_GROUP_SENSOR | 0x11)

/* Lens Response */
#define RESP_ID_LENS_SET_POS			(RESP_GROUP_LENS | 0x1)

/* Flash Resonse */
#define RESP_ID_FLASH_SET_POWER			(RESP_GROUP_FLASH | 0x1)
#define RESP_ID_FLASH_SET_ON			(RESP_GROUP_FLASH | 0x2)
#define RESP_ID_FLASH_SET_OFF			(RESP_GROUP_FLASH | 0x3)

/* For fw rb log usage */
#define FW_LOG_RB_SIZE_REG mmISP_LOG_RB_SIZE
#define FW_LOG_RB_RPTR_REG mmISP_LOG_RB_RPTR
#define FW_LOG_RB_WPTR_REG mmISP_LOG_RB_WPTR
#define FW_LOG_RB_BASE_LO_REG mmISP_LOG_RB_BASE_LO
#define FW_LOG_RB_BASE_HI_REG mmISP_LOG_RB_BASE_HI


/*-------------------------------------------------------------------------*/
/*			 Command Param List				   */
/*-------------------------------------------------------------------------*/



/***************************************************************************
 *				Global Control Command
 ***************************************************************************
 */
/*
 * Each command is detailed in the following columns:
 *
 * CMD_ID_XXX:      The id fo the corresponding command.
 *
 *CmdChannel:Specify which command channel host should send this kind
 *command. "NonBlock" means this kind command should be push into the
 *NonBlock command channel. "StreamX" means this kind is stream related
 *command and should push into the corresponding stream command channel for
 *which host want to control.
 *
 *RespChannel: Specify which response channel the RESP_ID_CMD_DONE respone
 *for this command is returned.
 *"Global" means it is returned in the global response channel.
 *"StreamX" means it is returned in the corresponding stream response
 *channel, which the X for command and response should be same.
 *X can be 1, 2, 3 for the three defined streams.
 *
 *ParameterType: How the command parameter is included. "Direct" means the
 *command parameter is stored in the cmdParam field in the Cmd_t structure.
 *"Indirect" means the command parameter is in another buffer. The
 *buffer address, size and checksum is specified in a struct of
 *CmdParamPackage_t. The CmdParamPackage is stored in the cmdParam field in
 *the Cmd_t structure.
 *
 *Parameter: The command parameter definition for the specified command.
 *
 *Description: A detail description for the specified command
 *
 *Type:	Defined when the command can send. "Static" means the command can
 *only be send when the firmware state for of the command to set/get is not
 *in used. "Dynamic" means the command can be send at any time when host
 *want to change the state. Detail information can refer to the Description
 *section.
 *
 *ResponsePayload: Each host command will get a RESP_ID_CMD_DONE response
 *from firmware.
 *The ResponsePayload defined the payload in the RESP_ID_CMD_DONE response
 *for this command.
 *
 *ResponseTime:When the RESP_ID_CMD_DONE response will be send back to host.
 *"Processed" means the response will be send back after the command was
 *processed. "Enqueued" means the respsone will be send back after firmware
 *parsed the command and enqueued it into its internal queue. Host should
 *check the meta data information in RESP_ID_FRAME_DONE response to confirm
 *which frame the new state is applied.
 */





//General Command
//---------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_BIND_STREAM
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdBindStream_t {
	StreamId_t masterStream;
	StreamId_t slaveStream;
} CmdBindStream_t;
// Description: This command is used to bind two streams together in order
// to make it like one 3D stream. The slaveStreams 3A control will be refer
// to the master stream. This command is a static command, can only be
// send before the start streaming. By default, all the streams are
// independent. After binding, only the master streams command are accept,
// and the slave stream will be start/stop streaming under the control of
// the master stream.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_UNBIND_STREAM
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdUnBindStream_t {
	StreamId_t masterStream;
	StreamId_t slaveStream;
} CmdUnBindStream_t;
// @Description: This command unbind the two streams which were bind by the
// previous CMD_ID_BIND_STREAMcommand. It can be only send when the two
// streams are idle status.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GET_FW_VERSION
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used for host to get the current firmware
// version. The value will be returned by the response payload. Version is
// an uint32 number in the format of:
// Bit31 to Bit24 is the major version, Bit23 to Bit16 is the minor version,
// Bit15 to Bit0 is the revision number.
// The first version is 0x02000000, that is 2.0.0. Any bug fix will increase
// the revision number and any function added will increase the minor number
// and reset the revision to 0. The major number will only increase in a big
// change of the firmware. Host can send this command at any time after the
// firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: uint32 version
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_PREPARE_CHANGE_ICLK
// @CmdChannel: nonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter: none
// @Description: This command will be sent before HOST change the ICLK to
// inform firmware to do some preparing work(e.g. skip AF/AE, reset timer,
// etc.), host can change the ICLK after FW response this command.
// NOTE: later host must send CMD_ID_SET_CLK_INFO to tell firmware that
// clock change has finished.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_CLK_INFO
// @CmdChannel: nonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetClkInfo_t {
	uint32 sclk; //In KHz
	uint32 iclk; //In KHz
	uint32 refClk; //In KHz
} CmdSetClkInfo_t;
// @Description: This command send the current isp running clock information
// to firmware. Firmware may use these clock information to calculate the
// profiler and flash timer count. So, these information should be
// exactly the hardware running clock. Or some control may fail.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CONFIG_GMC_PREFETCH
// @CmdChannel: nonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdConfigGmcPrefetch_t {
	GmcPrefetchType_t type;
} CmdConfigGmcPrefetch_t;
// @Description: This command configures gmc prefetch type to firmware.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_RESIZE_TABLE
// @CmdChannel: nonBlock
// @RespChannel: Global
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSetResizeTable_t {
	ResizeTable_t resizeTable;
} CmdSetResizeTable_t;
// @Description: This command configures resize table to firmware to achieve
// different IQ.
// @Type: Static
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//Debug Command
//-------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_READ_REG
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdReadReg_t {
	uint32 regAddr;
} CmdReadReg_t;
// @Description: This command is used to ask firmware to read a register
// value. The result value will be send back to host by response. This
// command can be send at any time after firmware is boot up.
// @Type: Dynamic
// @ResponsePayload:
typedef struct _RegValue_t {
	uint32 regAddr;
	uint32 regValue;
} RegValue_t;
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_WRITE_REG
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdWriteReg_t {
	uint32 regAddr;
	uint32 regValue;
} CmdWriteReg_t;
// @Description: This command is used to ask firmware to write a register
// value. Firmware will send the RESP_ID_CMD_DONE response to host after
// this command is processed. This command can be send at
//	 any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_LOG_SET_LEVEL
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdLogSetLevel_t {
	LogLevel_t level;
} CmdLogSetLevel_t;
// @Description: This command is used to set the firmware log level in debug
// mode. This command can be send at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_LOG_ENABLE_MOD
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdLogEnableMod_t {
	ModId_t mod;
} CmdLogEnableMod_t;
// @Description: This command is used to enable a specific module to dump
// log. All module log are enabled by default. This command can be send at
// any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_LOG_DISABLE_MOD
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdLogDisableMod_t {
	ModId_t mod;
} CmdLogDisableMod_t;
// @Description: This command is used to disable a specific modules log
// dump. It can be send at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_LOG_MOD_EXT
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
// level_bits_A: Bit map A for log level of FW modules. 4 bits represent one
// FW modules. Bit[4*(mod_id - 1) ~ (4*mod_id-1)] represents FW module
// refers to mod_id, When 1 <= mod_id <= 8.
// level_bits_B: Bit map B for log level of FW modules. 4 bits represent one
// FW modules. Bit[4*(mod_id - 9) ~ (4*(mod_id - 9)+3)] represents FW module
// refers to mod_id, When 9 <= mod_id <= 16.
// level_bits_C: Bit map C for log level of FW modules. 4 bits represent one
// FW modules. Bit[4*(mod_id - 17) ~ (4*(mod_id - 17)+3)] represents FW
// module refers to mod_id, When 17 <= mod_id <= 24.
// level_bits_D: Bit map D for log level of FW modules. 4 bits represent one
// FW modules. Bit[4*(mod_id - 25) ~ (4*(mod_id - 25) +3)] represents FW
// module refers to mod_id, When 25 < mod_id <= 32.
// level_bits_E: Bit map E for log level of FW modules. 4 bits represent one
// FW modules. Bit[4*(mod_id - 33) ~ (4*(mod_id - 33)+3)] represents FW
// module refers to mod_id, When 33 <= mod_id <= 40.
typedef struct _CmdSetLogModExt_t {
	uint32 level_bits_A;
	uint32 level_bits_B;
	uint32 level_bits_C;
	uint32 level_bits_D;
	uint32 level_bits_E;
} CmdSetLogModExt_t;
// @Description: This command is used to set specific modules log dump at
//specific log levels(0~5). It can be sent at any time after the firmware is
//boot up. This extend log command can't be used with old log commands
//together for avoiding log level setting conflict.
//Currently, There are 33 FW modules and mod_ids aredefined in ModId_t.
//There are 5 log levels that are defined in LogLevel_t.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_PROFILER_GET_RESULT
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdProfilerGetResult_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdProfilerGetResult_t;
// @Description: This command is used to get the current profiler result. It
// is used for debug purpouse. The results will be written to the buffer
// specified in the CmdProfilerGetResult _t. The result is a struct of
// ProfilerResult_t.
// This command can be send at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//Sensor Related Command
//----------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_SENSOR_PROP
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSetSensorProp_t {
	SensorId_t sensorId;
	SensorProp_t sensorProp;
} CmdSetSensorProp_t;
// @Description: This command is used to set a sensor property for the
// specified sensor denoted by sensorId.
// The information will be used to setup the isp pipeline input interface
// when one stream is started from its related sensor. So, this command must
// be send before starting a stream and can't be changed after the stream is
// started.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_SENSOR_HDR_PROP
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSetSensorHdrProp_t {
	SensorId_t sensorId;
	SensorHdrProp_t hdrProp;
} CmdSetSensorHdrProp_t;
// @Description: This command is used to set a sensor hdr property for the
// specified sensor denoted by sensorId. This command must be send before
// enable hdr and can't be changed after the hdr is started.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_SENSOR_EMB_PROP
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSetSensorEmbProp_t {
	SensorId_t sensorId;
	SensorEmbProp_t embProp;
} CmdSetSensorEmbProp_t;
// @Description: This command is used to set a sensor emb property for the
// specified sensor denoted by sensorId. This command must be send before
// enable emb and can't be changed after the emb is started.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_SENSOR_CALIBDB
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetSensorCalibdb_t {
	StreamId_t streamId;
	uint32 bufAddrLo;//Low 32bit address of the calibration data
	uint32 bufAddrHi;//High 32bit address of the calibration data
	uint32 bufSize;//calibration data buffer size

	uint8 checkSum; //The byte sum of the calibration data.
} CmdSetSensorCalibdb_t;
// @Description: This command set the calibration data for the specified
// sensor. It should be sent before starting a stream with the calibration
// data enabled and can't be sent after a stream from this sensor is on
// streaming.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_SENSOR_M2M_CALIBDB
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetSensorM2MCalibdb_t {
	SensorId_t sensorId;
	//Low 32bit address of the module to module calibration data
	uint32 bufAddrLo;
	//High 32bit address of the module to module calibration data
	uint32 bufAddrHi;
	uint32 bufSize;//module to module calibration data buffer size

	uint8 checkSum; //The byte sum of the calibration data.
} CmdSetSensorM2MCalibdb_t;
// @Description: This command set the module to module variance calibration
// data for the specified sensor. It should be sent before startinga stream
// with the calibration data enabled and can't be sent after a stream from
// this sensor is on streaming.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//Device Control Command
//----------------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_DEVICE_SCRIPT_CONTROL
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the device script control
// for sensor, flash, lens, etc. It can be send at any time after the
// firmware is boot up. By default, the device scriptcontrol is not enabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_DEVICE_SCRIPT_CONTROL
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the device script control
// for sensor, flash, lens, etc. It can be
// send at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_DEVICE_SCRIPT
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSetDeviceScript_t {
	SensorId_t sensorId;
	DeviceScript_t deviceScript;
} CmdSetDeviceScript_t;
// @Description: This command is used to set the device script for the
// specified sensor with sensor, flash, lens control, etc. It can only be
// send when the device script control is not enabled.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_GROUP_HOLD
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorGroupHold_t {
	SensorId_t sensorId;
} CmdAckSensorGroupHold_t;
// @Description: This command is used to acknowledge the
// RESP_ID_SENSOR_GROUP_HOLD response.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_GROUP_RELEASE
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorGroupRelease_t {
	SensorId_t sensorId;
} CmdAckSensorGroupRelease_t;
// @Description: This command is used to acknowledge the
// RESP_ID_SENSOR_GROUP_RELEASE response.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_AGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetAGain_t {
	SensorId_t sensorId;
	uint32 aGain;
} CmdAckSensorSetAGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_AGAIN to firmware. The aGain is the result analog gain
// set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_DGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetDGain_t {
	SensorId_t sensorId;
	uint32 dGain;
} CmdAckSensorSetDGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_DGAIN to firmware. The dGain is the result digital
// gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_ITIME
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetItime_t {
	SensorId_t sensorId;
	uint32 itime;
} CmdAckSensorSetItime_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_ITIME to firmware. The itime is the result itime set
// into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_HIGH_ITIME
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrHighItime_t {
	SensorId_t sensorId;
	uint32 itime;
} CmdAckSensorSetHdrHighItime_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_HIGH_ITIME to firmware. The itime is the result
// high itime set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowItime_t {
	SensorId_t sensorId;
	uint32 itime;
} CmdAckSensorSetHdrLowItime_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_ITIME to firmware. The itime is the result low
// itime set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME_RATIO
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowItimeRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} CmdAckSensorSetHdrLowItimeRatio_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_ITIME_RATIOto firmware. The ratio is the
// result low itime ratio set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_ITIME
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrEqualItime_t {
	SensorId_t sensorId;
	uint32 itime;
} CmdAckSensorSetHdrEqualItime_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_EQUAL_ITIMEto firmware. The itime is the result
// equal itime set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_HIGH_AGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrHighAGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrHighAGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_HIGH_AGAIN to firmware. The aGain is the result
// high analog gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowAGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrLowAGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_AGAIN to firmware. The aGain is the result low
// analog gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN_RATIO
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowAGainRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} CmdAckSensorSetHdrLowAGainRatio_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_AGAIN_RATIOto firmware. The ratio is the
// result low analog gain ratio set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_AGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrEqualAGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrEqualAGain_t;
// @DescriptionThis command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_EQUAL_AGAINto firmware. The aGain is the result
// equal analog gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_HIGH_DGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrHighDGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrHighDGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_HIGH_DGAIN to firmware. The gain is the result
// high digital gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowDGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrLowDGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_DGAIN to firmware. The gain is the result low
// digital gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN_RATIO
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrLowDGainRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} CmdAckSensorSetHdrLowDGainRatio_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_LOW_DGAIN_RATIOto firmware. The ratio is the
// result low gain ratio set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_DGAIN
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckSensorSetHdrEqualDGain_t {
	SensorId_t sensorId;
	uint32 gain;
} CmdAckSensorSetHdrEqualDGain_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_SENSOR_SET_HDR_EQUAL_DGAINto firmware. The gain is the result
// equal digital gain set into the sensor.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_LENS_SET_POS
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckLensSetPos_t {
	SensorId_t sensorId;
	uint32 lensPos;
} CmdAckLensSetPos_t;
// @Description: This command is used to acknowledge the
// responseRESP_ID_LENS_SET_POS to firmware.
// The lensPos is the result lens set into the lens drive.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_FLASH_SET_POWER
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckFlashSetPower_t {
	SensorId_t sensorId;
	uint32 powerLevel;
} CmdAckFlashSetPower_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_FLASH_SET_POWER to firmware. The powerLevel is the result power
// level set into the flash.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_FLASH_SET_ON
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckFlashSetOn_t {
	SensorId_t sensorId;
} CmdAckFlashSetOn_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_FLASH_SET_ON to firmware.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ACK_FLASH_SET_OFF
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAckFlashSetOff_t {
	SensorId_t sensorId;
} CmdAckFlashSetOff_t;
// @Description: This command is used to acknowledge the response
// RESP_ID_FLASH_SET_OFF to firmware.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//I2C Control Command
//-------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SEND_I2C_MSG
// @CmdChannel: NonBlock
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSendI2cMsg_t {
	I2cDeviceId_t deviceId;
	I2cMsg_t msg;
} CmdSendI2cMsg_t;
// @Description: This command is used for host to control external i2c device
// through firmware after the firmware is started.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




/*****************************************************************************
 *					Stream Control Command
 *****************************************************************************
 */
//General Command
//---------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_STREAM_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetStreamMode_t {
	StreamMode_t streamMode;
} CmdSetStreamMode_t;
// @Description: This command set the stream mode for the stream. This command
//   should be sent before the start streaming command.
//	 Currently up to three streams are supported working simultaneously.
//   Each stream can be configured to different stream mode. However, the
//   combination of these modes are limited because of the hardware
//	 limitation. The supported use cases are list below:
//	 1.	MODE0+MODE1
//	 2.	MODE0+MODE2+MODE4
//	 3.	MODE0+MODE3+MODE4
//	 4.	MODE0+MODE4+MODE4
//	 5.	MODE2+MODE2+MODE4
//	 6.	MODE2+MODE3+MODE4
//	 7.	MODE2+MODE4+MODE4
//	 8.	MODE3+MODE3+MODE4
// For each use case, host can selected one, two or three stream to work at
//   any time.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_INPUT_SENSOR
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetInputSensor_t {
	SensorId_t sensorId;
} CmdSetInputSensor_t;
// @Description: This command set the input sensor for the stream.
// This command should be sent before the start streaming command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_ACQ_WINDOW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetAcqWindow_t {
	Window_t window;
} CmdSetAcqWindow_t;
// @Description: This command set the isp input acqisitioin window. This window
// should be in the range of the sensor output resolution.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_CALIB
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the calibration data usage for
// the stream. By default the calibration data is disabled in firmware. Host
// should send this command after the CMD_ID_SET_SENSOR_CALIB and setup the
// input sensor by CMD_ID_SET_INPUT_SENSOR to enable the calibration. It will
// initial the 3A parameters from the calibration data.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_CALIB
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the calibration data usage for
// the stream. It will reset the 3A parameters to default values.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_ASPECT_RATIO_WINDOW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetAspectRatioWindow_t {
	Window_t window;
} CmdSetAspectRatioWindow_t;
// @Description: This command is used to set the aspect ratio window to keep
// aspect ratio. The aspect window is cropped from the sensor acquisition
// window. This window is added in order to keep aspect ratio without to
// change the sensor acquisition window which need a corresponding calibration
// data. The default window is equal to the sensor acquisition window.This
// command can only be send in stream idle status.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_ZOOM_WINDOW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetZoomWindow_t {
	Window_t window;
} CmdSetZoomWindow_t;
// @Description: This command is used to set the zoom window for the stream.
// The zoom window is cropped from the aspect ratio window. This command can
// be send at any time after firmware boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct _CmdSetIRIlluConfig_t {
	IRIlluConfig_t IRIlluConfig;
} CmdSetIRIlluConfig_t;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_CSM_YUV_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetCsmYuvRange_t {
	CSMYUVRange_t CsmYuvRange;
} CmdSetCsmYuvRange_t;
// @Description: This command is used to set CSM YUV Range
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_PREVIEW_OUT_PROP
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetPreviewOutProp_t {
	ImageProp_t outProp;
} CmdSetPreviewOutProp_t;
// @Description: This command set the image output property for the preview
// channel of the stream. Host can send this command only when the preview is
// not enabled or the stream is totally idle. That means it is safety to send
// this command after firmwar boot up but before start streaming. When the
// stream is started and the peview output channel is not enabled, it is also
// safety to send this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_PREVIEW_FRAME_RATE_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetPreviewFrameRateRatio_t {
	uint32 ratio;
} CmdSetPreviewFrameRateRatio_t;
// @Description: This command set the preview frame rate ratio with the input
// sensor frame rate. The default
//	ratio is 1. Allowed values are: 1~30. The output frame rate of the video
//   channel is:
//	 1/ratio *SensorFrameRate.
//	 Host can send this command only when the preview is not enabled or the
//   stream is totally idle. That means it is safety to send this command after
//   firmwar boot up but before start streaming. When the stream is started
// and the preview output channel is not enabled, it is also safety to send
//   this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_PREVIEW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the preview output for the
// stream. Host should set the preview
//	 output property and preview buffer queue before send this command when
//  it is in streaming status. If the
//	 stream is not started, this command can send at any time.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_PREVIEW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the preview output for the
// stream. By default, the preview
// output is disabled. This command can be send at any time after firmware boot
// up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_VIDEO_OUT_PROP
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetVideoOutProp_t {
	ImageProp_t outProp;
} CmdSetVideoOutProp_t;
// @Description: This command set the image output property for the video
//  channel of the stream. Host can send this command only when the video is
//  not enabled or the stream is totally idle. That means it is safety to
//	 send this command after firmware boot up but before start streaming.
//   When the stream is started and the
// video output channel is not enabled, it is also safety to send this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_VIDEO_FRAME_RATE_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetVideoFrameRateRatio_t {
	uint32 ratio;
} CmdSetVideoFrameRateRatio_t;
// @Description: This command set the video frame rate ratio with the input
// sensor frame rate. The default ratio is 1. Allowed values are: 1~30. The
// output frame rate of the video channel is: 1/ratio *SensorFrameRate.
// Host can send this command only when the video is not enabled or the stream
// is totally idle. That means it is safety to send this command after firmwar
// boot up but before start streaming. When the stream is started and the
// video output channel is not enabled, it is also safety to send this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_VIDEO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the video output for the
// stream. Host should set the video output property and video buffer queue
// before send this command when it is in streaming status. If the
// streaming is not started, this command can send at any time.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_VIDEO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the video output for the
// stream. By default, the video output is disabled. This command can be send
// at any time after firmware boot up.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_ZSL_OUT_PROP
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetZslOutProp_t {
	ImageProp_t outProp;
} CmdSetZslOutProp_t;
// @Description: This command set the image output property for the zsl
// channel of the stream. Host can send this command only when the zsl is not
// enabled or the stream is totally idle. That means it is safety to send this
// command after firmwar boot up but before start streaming. When the stream
// is started and the zsl output channel is not enabled, it is also safety to
// send this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_ZSL_FRAME_RATE_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetZslFrameRateRatio_t {
	uint32 ratio;
} CmdSetZslFrameRateRatio_t;
// @Description: This command set the zsl frame rate ratio with the input
// sensor frame rate. The default ratio is 1. Allowed values are: 1~30. The
// output frame rate of the zsl channel is: 1/ratio *SensorFrameRate.
// Host can send this command only when the zsl is not enabled or the stream
// is totally idle. That means it is safety to send this command after firmwar
// boot up but before start streaming. When the stream is started and the zsl
// output channel is not enabled, it is also safety to send this command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_ZSL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the zsl output for the stream.
// Host should set the zsl output property and zsl buffer queue before send
// this command when it is in streaming status. If the streaming is not
// started, this command can send at any time.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_ZSL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the zsl output for the
// stream. By default, the zsl output is disabled. This command can be send at
// any time after firmware boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAPTURE_YUV
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdCaptureYuv_t {
	ImageProp_t imageProp;
	Buffer_t buffer;
	bool_t usePreCap;
} CmdCaptureYuv_t;
// @Description: This command is used to capture a yuv picture from the zsl
// channel. The image format and resolution are specified in the imageProp
// fieleds. The meata data and buffer will be returned in the
// RESP_ID_FRAME_DONE respone. The frame with this buffer will use the
// precapture measuring result to control the sensor and flash if the
// usePreCap is set to true. Host should make sure this command
// is send after CMD_ID_AE_PRECAUTRE command, otherwise the exposure parameter
// may error. If the usePreCap
// flag is set to false, the frame with this buffer will use normal 3A to
// calculate the sensor/flash/lens parameters. Firmware will send this
// response after the buffer is filled . Host should wait the
// RESP_ID_FRAME_DONE response to get the buffer capture done signal.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_RAW_ZSL_FRAME_RATE_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetRawZslFrameRateRatio_t {
	uint32 ratio;
} CmdSetRawZslFrameRateRatio_t;
// @Description: This command set the raw zsl frame rate ratio with the input
// sensor frame rate. The default ratio is 1. Allowed values are: 1~30. The
// output frame rate of the raw zsl channel is: 1/ratio *SensorFrameRate.
// Host can send this command only when the raw zsl is not enabled or the
// stream is totally idle. That means it is safety to send this command after
// firmwar boot up but before start streaming. When the stream is started and
// the raw zsl output channel is not enabled, it is also safety to send this
// command.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_ENABLE_RAW_ZSL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the raw zsl output for the
// stream. Host should set the raw zsl output property and raw zsl buffer
// queue before send this command when it is in streaming status. If the
// streaming is not started, this command can send at any time.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DISABLE_RAW_ZSL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the raw zsl output for the
// stream. By default, the raw zsl output is disabled. This command can be
// send at any time after firmware boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAPTURE_RAW
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCaptureRaw_t {
	Buffer_t buffer;
	bool_t usePreCap;
} CmdCaptureRaw_t;
// @Description: This command is used to capture a yuv picture from the zsl
// channel. The image format and resolution are specified in the imageProp
// fieleds. The meata data and buffer will be returned in the
// RESP_ID_FRAME_DONE respone. The frame with this buffer will use the
// precapture measuring result to control the sensor and flash if the
// usePreCap is set to true. Host should make sure this command is send after
// CMD_ID_AE_PRECAUTRE command, otherwise the exposure parameter may error. If
// the usePreCap flag is set to false, the frame with this buffer will use
// normal 3A to calculate the sensor/flash/lens parameters.
// Firmware will send this response after the buffer is filled . Host should
// wait the RESP_ID_FRAME_DONE response to get the buffer capture done signal.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_RAW_PKT_FMT
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetRawPktFmt_t {
	RawPktFmt_t rawPktFmt;
} CmdSetRawPktFmt_t;
// @Description: This command is used to set the raw output format for the
// stream. See the RawPktFmt_t definition for detail description. The default
// setting is RAW_PKT_FMT_0.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_START_STREAM
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command starts the stream for the corresponding stream.
// The Image output property,
// Input sensor, sensor property, sensor calibration data, image buffers, etc
// should be ready for this stream.
// @Type: Static
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_STOP_STREAM
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdStopStream_t {
	bool_t destroyBuffers;
} CmdStopStream_t;
// @Description: This command stops the stream for the corresponding stream.
// All the resource will be released after the command response send back to
// host if the destroyBuffers is set to true. However, the stream context is
// not reset. Host can speed warm start the stream again if the setting is not
// changed.Host should waiting the response back before sending any other
// command for a next start stream.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_IGNORE_META
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdIgnoreMeta_t {
	bool_t ignore;
} CmdIgnoreMeta_t;
// @Description: This command decides if continue stage2 process when there is
// no metadata buffer, setting ignore to 1 means go on even without metabuffer
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_RESET_STREAM
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to reset the stream context to default
// values. Host can send this command to reset context after
// CMD_ID_STOP_STREAM if it doesn't need warm start up.
// @Type: Static
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_FRAME_RATE_INFO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetFrameRateInfo_t {
	uint32 frameRate; //Actual framerate multiplied by 1000
} CmdSetFrameRateInfo_t;
// @Description: This command set the current fixed frame rate information for
// Ae, Af, Awb to calculate the loop parameters . If the itime range is
// defined in AFPS mode and the current itime shows the frame rate is lower
// than this frameRate shows, the itime will be used as the reference.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SET_FRAME_CONTROL_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSetFrameControlRatio_t {
	uint32 ratio;
} CmdSetFrameControlRatio_t;
// @Description: This s command set the frame rate ratio with the input sensor
// frame rate to process the frame control command CMD_ID_SEND_FRAME_CONTROL.
// The default ratio is 1. Allowed values are: 1~30.
// The frame rate to process the frame control command is: 1/ratio
// *SensorFrameRate.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//AE Command
//----------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetMode_t {
	AeMode_t mode;
} CmdAeSetMode_t;
// @Description: This command change the AE run mode for the stream. The
// default mode is AE_MODE_MANUAL.
// Host can send this command at any time to change the current ae mode after
// the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_FLICKER
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetFlicker_t {
	AeFlickerType_t flickerType;
} CmdAeSetFlicker_t;
// @Description: This command changes the flicker status for AE algorithm of
// the corresponding stream.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_REGION
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetRegion_t {
	Window_t window;
} CmdAeSetRegion_t;
// @Description: This command changes the AE measurement region of the
// corresponding stream. The default region is calculated by firmware
// according to the sensor acquisition resolution and aspect ratio window. Host
// can send this command at any time to change the region window after the
// firmware is boot up. Figure 2.9 shows the AE region window corresponding to
// the aspect ratio window. If the zoom is enabled, the AE region
// window will be clipped inside the zoom window by firmware.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_LOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeLock_t {
	LockType_t lockType;
} CmdAeLock_t;
// @Description: This command is used to do lock operation on the AE algorithm.
// The LOCK_TYPE_IMMEDIATELY will lock the AE algorithm immediately, no matter
// what the AE status now.
// The LOCK_TYPE_CONVERGENCE will lock the AE algorithm after the ae get to
// the convergence status.
// Host can send this command at any time to lock AE after the firmware is
// boot up.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_UNLOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to unlock the AE algorithm which is
// locked by the previous lock command. Host can send this command at any time
// to unlock AE after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ANALOG_GAIN_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetAnalogGainRange_t {
	uint32 minGain;//The minimum analog gain multiplied by 1000
	uint32 maxGain;//The maximum analog gain multiplied by 1000
} CmdAeSetAnalogGainRange_t;
// @Description: Set the analog gain range. The default minGain is 1000, and
// maxGain is 1000. The gain range will be used for AE auto mode and to
// calculate the ISO capability. Host can send this command at any time
// after firmware is boot up. When the AE is running in auto mode, host may
// use this command to change the gain region for changing the scene mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_DIGITAL_GAIN_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetDigitalGainRange_t {
	uint32 minGain;//The minimum analog gain multiplied by 1000
	uint32 maxGain;//The maximum analog gain multiplied by 1000
} CmdAeSetDigitalGainRange_t;
// @Description: Set the digital gain range. The default minGain is 1000 and
// maxGain is 1000. The gain range will be used for AE auto mode. Host can
// send this command at any time after firmware is boot up. When the
// AE is running in auto mode, host may use this command to change the gain
// region for changing the scene mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ITIME_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetItimeRange_t {
	uint32 minItime;//Normal light case. In microsecond
	uint32 maxItime;//Normal light case. In microsecond
	uint32 minItimeLowLight;//Low light case. In microsecond
	uint32 maxItimeLowLight;//Low light case. In microsecond
} CmdAeSetItimeRange_t;
// @Description: Set the itime range. The default minItime and maxItime is
// 1000. The itime range will be used for
// AE auto mode. Host can send this command at any time after firmware is boot
// up. When the AE is running in auto mode, host may use this command to
// change the itime region for changing the scene mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ANALOG_GAIN
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetAnalogGain_t {
	uint32 aGain;
} CmdAeSetAnalogGain_t;
// @Description: This command changes the sensor analog gain for the
// corresponding stream. The AE mode
// should be in manual mode when send this command in streaming status. When
// the streaming is in idle status, this command is used to set the initial AE
// analog gain of auto mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_DITIGAL_GAIN
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetDigitalGain_t {
	uint32 dGain;
} CmdAeSetDigitalGain_t;
// @Description: This command changes the sensor digital gain for the
// corresponding stream. The AE mode should be in manual mode when send this
// command in streaming status. When the streaming is in idle status, this
// command is used to set the initial AE digital gain of auto mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ITIME
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetItime_t {
	uint32 itime;
} CmdAeSetItime_t;
// @Description: This command changes the sensor integration time for the
// corresponding stream. The AE mode should be in manual mode when send this
// command in streaming status. When the streaming is in idle status, this
// command is used to set the initial AE integration time of auto mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ISO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetIso_t {
	AeIso_t iso;
} CmdAeSetIso_t;
// @Description: This command is used to set the ISO value of the AE algorithm.
// Firmware set AE_ISO_AUTO as the default value. The ISO value not equal to
// AE_ISO_AUTO will make the AE algorithm fix the analog gain according to the
// ISO value when it is running in auto mode. If the AE mode is set to manual
// mode, this command also changes the analog gain.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_GET_ISO_CAPABILITY
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used for host to query the ISO capability of
// the current sensor setting. The sensor property, analog gain range
// andsensor id for the stream should be set before send this command,
// otherwise this command will fail. The iso capability information will be
// returned in the RESP_ID_CMD_DONE payload.
// @Type: Dynamic
// @ResponsePayload:
typedef struct _AeIsoCapability_t {
	AeIso_t minIso;
	AeIso_t maxIso;
} AeIsoCapability_t;
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_ISO_RANGE & CMD_ID_AE_GET_ISO_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Description: CMD_ID_AE_SET_ISO_RANGE is used to set the ISO range of the
// AE algorithm. Firmware set AE_ISO_AUTO as the default mode. But the host
// can specify a ISO range that influence the ISO selection during AE.
// AE algorithm must respect the minimum and maximum ISO values and pick up a
// proper ISO value between them still in AUTO mode.
// If the AE mode is set to manual mode, this command does not take any effect.
// This command is also independent with CMD_ID_AE_SET_ISO, the last ISO value
// or range take effective.
// CMD_ID_AE_GET_ISO_RANGE is also used for host to query the effective ISO
// range of the current sensor setting. FW should return the ISO capability,
// if host has never set a ISO range before.
// @Parameter:
typedef struct _AeIsoRange_t {
	AeIso_t minIso;
	AeIso_t maxIso;
} AeIsoRange_t;
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_SETPOINT
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Description: This command changes the AE setpoint for the corresponding
// stream. The AE mode can be in auto or manual mode when send this command in
// streaming status. When the streaming is in idle status, this command is
// used to set the initial AE setpoint. This command is not mandatory, but it
// can override the values in tuning result. If the new setpoint equals -1
// from host, it means host is going to clear the manual setpoint, so FW
// should use the value from tuning result again.
// @Parameter:
typedef struct _CmdAeSetSetPoint_t {
	int32 setPoint; // The setpoint multiplied by 1000
} CmdAeSetSetPoint_t;
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_GET_SETPOINT
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Description: FW will return the current effective AE setpoint to host.
// @Parameter: None
// @Type: Dynamic
// @ResponsePayload:
typedef struct _AeSetPoint_t {
	uint32 setPoint;
} AeSetPoint_t;
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_EV
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetEv_t {
	AeEv_t ev;
} CmdAeSetEv_t;
// @Description: This command set the EV compensation for the AE auto
// algorithm. The default EV is 0. This parameter only take effects on auto
// mode.
// @Type: Dynamic
// @ResponsePayload: None.
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_GET_EV_CAPABILITY
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used for host to query the EV capability of
// the current sensor setting. The analog gain range, digital gain range and
// itime range for the stream should be set before send this command,
// otherwise this command will fail. The ev capability information will be
// returned in the RESP_ID_CMD_DONE payload.
// @Type: Dynamic
// @ResponsePayload:
typedef struct _AeEvCapability_t {
	AeEv_t stepEv;
	AeEv_t minEv;
	AeEv_t maxEv;
} AeEvCapability_t;
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_PRECAPTURE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command trigs a precapture metering according to the
// current flash and ae mode setting.
// It is only accept in auto mode.The metering result will be recorded for the
// usage of the next capture command.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_ENABLE_HDR
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the hdr. By default the hdr is
// disabled.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_DISABLE_HDR
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the hdr.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_FLASH_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetFlashMode_t {
	FlashMode_t mode;
} CmdAeSetFlashMode_t;
// @Description: This command set the ae flash light mode.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_FLASH_POWER_LEVEL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetFlashPowerLevel_t {
	uint32 powerLevel;
} CmdAeSetFlashPowerLevel_t;
// @Description: This command is used to set the flash power level for the ae.
// The powerLevel should be in the range of 0~100. 100 means 100 percent of
// the flash power level. The default power level is 80 percent.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_RED_EYE_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetRedEyeMode_t {
	RedEyeMode_t mode;
} CmdAeSetRedEyeMode_t;
// @Description: This command set ae flash red eye mode. The default mode is
// RED_EYE_MODE_OFF.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_SET_START_EXPOSURE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeSetStartExposure_t {
	uint32 startExposure; //The start exposure multiplied by 1000000
} CmdAeSetStartExposure_t;
// @Description: This command set ae start exposure. The start exposure is the
// exposure value when sensor is started. It can be get from sensor driver.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AE_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAeGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAeGetStatus_t;
// @Description: This command is used to get the current ae algorithm status.
// It is used for debug purpouse. The results will be written to the buffer
// specified in the CmdAeGetStatus_t. The result is a struct defined as
// follows:
typedef struct _AeStatus_t {
	AeMode_t mode;
	AeFlickerType_t flickerType;
	AeLockState_t lockState;
	AeSearchState_t searchState;
	AeIsoCapability_t isoCap;
	AeIso_t iso;
	AeEvCapability_t evCap;
	AeEv_t ev;
	Window_t region;
	uint32 itime;
	uint32 aGain;
	uint32 dGain;
	uint32 minItime;
	uint32 maxItime;
	uint32 minAGain;
	uint32 maxAGain;
	uint32 minDGain;
	uint32 maxDgain;
	bool_t hdrEnabled;
	HdrItimeType_t hdrItimeType;
	uint8 hdrItimeRatioList[MAX_HDR_RATIO_NUM];
	HdrAGainType_t hdrAGainType;
	uint8 hdrAGainRatioList[MAX_HDR_RATIO_NUM];
	HdrDGainType_t hdrDGainType;
	uint8 hdrDGainRatioList[MAX_HDR_RATIO_NUM];
} AeStatus_t;
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//AWB Command
//-----------



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetMode_t {
	AwbMode_t mode;
} CmdAwbSetMode_t;
// @Description: This command changes the AWB run mode for the stream. The
// default mode is AWB_MODE_MANUAL.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_REGION
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetRegion_t {
	Window_t window;
} CmdAwbSetRegion_t;
// @Description: This command changes the AWB measurement region of the
// corresponding stream. The default region is over the full image. However,
// this window may be clipped inside the zoom window when the digital zoom is
// running.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_LIGHT
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetLight_t {
	uint8 lightIndex;
} CmdAwbSetLight_t;
// @Description: This command change the current wb light when in manual mode
// or set the initial light for the auto algorithm. The light is selected by
// index, which should be exist in the calibration data.
// It is not a must for host to send this command to set the initial light for
// auto mode. Firmware will use the first light in the calibration data as the
// initial light by default. Host can send this command at any time to apply a
// new light setting when the wb mode is manual. For awb auto mode, this
// command should be send when the stream in idle status. Host should make
// sure the calibration data is send before send this command because this
// command use the calibration to get the corresponding data.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_COLOR_TEMPERATURE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetColorTemperature_t {
	//In K. For example, 1000 means 1000K, 5500 means 5500K
	uint32 colorTemperature;
} CmdAwbSetColorTemperature_t;
// @Description: This command changes the current wb temperature in manual
// mode. If the specified color temperature is not exist in the calibration
// data, firmware will interpolate the data. Host can send this command at any
// time to apply a new light setting when the wb mode is manual. For awb auto
// mode, this command should be send when the stream in idle status. Host
// should make sure the calibration data is send before send this command
// because this command use the calibration to get the corresponding data.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_WB_GAIN
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetWbGain_t {
	WbGain_t wbGain;
} CmdAwbSetWbGain_t;
// @Description: This command changes the current wb gain in manual mode. Host
// can send this command at any time to apply a new wb gain setting when the
// wb mode is manual. And can send this command in auto mode when
// the stream is not started to set the initial wb gain.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_WB_GAIN_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetWbGainRatio_t {
	WbGainRatio_t wbGainRatio;
} CmdAwbSetWbGainRatio_t;
// @Description: This command changes the wb gain bias ratio.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_WB_GAIN_RATIO
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbGetWbGainRatio_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAwbGetWbGainRatio_t;
// @Description: This command gets the wb gain bias ratio of all illuminators.
// Host can send this command at anytime after the firmware is boot up. The
// result will be written to the buffer specified in the
// CmdAwbGetWbGainRatio_t. The result is a struct of WbGainRatioAll_t.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
#endif




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_WB_GAIN
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the current wb gain used. It is used for
// debug. Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: WbGain_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_CC_MATRIX
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetCcMatrix_t {
	CcMatrix_t ccMatrix;
} CmdAwbSetCcMatrix_t;
// @Description: This command changes the current cc coefficients matrix in
// manual mode. Host can send this command at any time to apply a new cc
// matrix setting when the wb mode is manual. And can send this command in
// auto mode when the stream is not started to set the initial cc matrix.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_CC_MATRIX
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the current cc matrix used in awb loop.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: CcMatrix_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_CC_OFFSET
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbSetCcOffset_t {
	CcOffset_t ccOffset;
} CmdAwbSetCcOffset_t;
// @Description: This command changes the current cc offset in manual mode.
// Host can send this command at any time to apply a new cc offset setting
// when the wb mode is manual. And can send this command in auto mode when the
// stream is not started to set the initial cc offset.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_CC_OFFSET
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the current cc offset used in awb loop.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: CcOffset_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_LSC_MATRIX
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdAwbSetLscMatrix_t {
	LscMatrix_t lscMatrix;
} CmdAwbSetLscMatrix_t;
// @Description: This command sets the current lsc matrix in manual mode.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_LSC_MATRIX
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbGetLscMatrix_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAwbGetLscMatrix_t;
// @Description: This command gets the current lsc matrix used in awb loop.
// Host can send this command at anytime after the firmware is boot up. The
// result will be written to the buffer specified in the CmdAwbGetLscMatrix_t.
// The result is a struct of LscMatrix_t.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_LSC_SECTOR
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdAwbSetLscSector_t {
	PrepLscSector_t lscSector;
} CmdAwbSetLscSector_t;
// @Description: This command sets the current lsc sector in manual mode. Host
// can send this command at any time to apply a new lsc sector setting when
// the wb mode is manual. And can send this command in auto mode when
// the stream is not started to set the initial lsc sector.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_LSC_SECTOR
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbGetLscSector_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAwbGetLscSector_t;
// @Description: This command gets the current lsc sector(PrepLscSector_t)
// used in awb loop. Host can send this command at anytime after the firmware
// is boot up. The result will be written to the buffer specified int the
// CmdAwbGetLscSector_t.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_SET_AWBM_CONFIG
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdAwbSetAwbmConfig_t {
	uint8 maxY;
	uint8 refCr_maxR;
	uint8 minY_maxG;
	uint8 refCb_maxB;
	uint8 maxCSum;
	uint8 minC;
} CmdAwbSetAwbmConfig_t;
// @Description: This command sets the AWBM config
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_LOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbLock_t {
	LockType_t lockType;
} CmdAwbLock_t;
// @Description:
// This command is used to do lock operation on the AWB algorithm.
// The LOCK_TYPE_IMMEDIATELY will lock the AWB algorithm immediately, no
// matter what the AWB status now.
// The LOCK_TYPE_CONVERGENCE will lock the AWB algorithm after the ae settled.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_UNLOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This commandis used to unlock the AWB algorithm which is
// locked by the previous lock
//	 command.
//	 Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AWB_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAwbGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAwbGetStatus_t;
// @Description: This command is used to get the curreht awb algorithm status.
// It is used for debug purpouse. The result will be written to the buffer
// specified in the CmdAwbGetStatus_t. Host can send this command at any time
// after the firmware is boot up.
typedef struct _AwbStatus_t {
	AwbMode_t mode;
	Window_t regionWindow;
	AwbLockState_t lockState;
	AwbSearchState_t searchState;
	uint32 colorTemperature;
	WbGain_t wbGain;
	CcMatrix_t ccMatrix;
	CcOffset_t ccOffset;
	LscMatrix_t lscMatrix;
	PrepLscSector_t prepLscSector;
} AwbStatus_t;
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//AF Command
//----------



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetMode_t {
	AfMode_t mode;
} CmdAfSetMode_t;
// @Description: This command set the AF run mode for the stream. The default
// mode is AF_MODE_MANUAL. Host can send this command at any time to change
// the current af mode after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_LENS_RANGE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetLensRange_t {
	LensRange_t lensRange;
} CmdAfSetLensRange_t;
// @Description: This command changes the lens position range. It is mainly
// used for host to implement different lens range in different scenario like
// macro, full range, etc. Host can send this command at any time after the
// firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_LENS_POS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetLensPos_t {
	uint32 lensPos;
} CmdAfSetLensPos_t;
// @Description: This command change the lens position. In manual mode,
// firmware will not change the lens position. In CONTINUOUS_PICTURE or
// CONTINUOUS_VIDEO mode, and it is not paused by the command
// CMD_ID_AF_CANCEL, the auto algorithm may overwrite the lens position.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_REGION
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetRegion_t {
	AfWindowId_t windowId;
	Window_t window;
} CmdAfSetRegion_t;
// @Description: This command changes the AF measurement region of the
// corresponding stream. The default region is over the full image. However,
// this window may be clipped inside the zoom window when the digital
// zoom is running.
// Currently three focus window is supported simultaneously. However, these
// windows shouldn't be overlapped. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_TRIG
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command trigs a new focus searching. If the current mode
// is ONE_SHOT, the algorithm only search one time until convergence or
// failed. If the current mode is CONTINUOUS_PICTURE or CONTINUOUS_VIDEO, the
// algorithm will do a search until convergence or failed and still keep in
// tracking. If the current mode is CONTINUOUS_PICTURE or CONTINUOUS_VIDEO and
// was paused by the last command CMD_ID_AF_CANCEL, the CMD_ID_AF_TRIG command
// will resume the tracking.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_CANCEL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command cancels the one shot searching if it is still in
// searching or pause the tracking if it is in CONTINUOUS_PICTURE or
// CONTINUOUS_VIDEO mode. Host can send CMD_ID_AF_TRIG to resume the tracking
// for CONTINUOUS_PICTURE or CONTINUOUS_VIDEO mode, or start a new searching
// for ONE_SHOT mode. Host can send this command at any time after the stream
// is started.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_LOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfLock_t {
	LockType_t lockType;
} CmdAfLock_t;
// @Description: This command is used to do lock operation on the AF algorithm.
// The LOCK_TYPE_IMMEDIATELY will lock the AF algorithm immediately, no matter
// what the AE status now. The LOCK_TYPE_CONVERGENCE will lock the AF
// algorithm after the ae settled. The AF is locked after this command is
// finished. Host can't send any AF command except the UNLOCK command after
// sending this command.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_UNLOCK
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to unlock the AF algorithm which is
// locked by the previous lock command. Host can send this command at any time
// the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_FOCUS_ASSIST_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetFocusAssistMode_t {
	FocusAssistMode_t mode;
} CmdAfSetFocusAssistMode_t;
// @Description: This command is used to set the flash focus assist mode. By
//default it is off. In FOCUS_ASSIST_MODE_OFF mode, the flash for focus assist
// is always disabled. In FOCUS_ASSIST_MODE_ON mode, the flash for focus
// assist is always on when a CMD_ID_AF_TRIG is send to focus search or the
// algorithm need to do a focus search in auto mode. In
// FOCUS_ASSIST_MODE_AUTO, the auto focus algorithm will automatically
// determince whether to turn on or off the flash light to assist focus
// according to the current luminance.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_SET_FOCUS_ASSIST_POWER_LEVEL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfSetFocusAssistPowerLevel_t {
	uint32 powerLevel;
} CmdAfSetFocusAssistPowerLevel_t;
// @Description: This command is used to set the flash power level for the
// focus assist. The powerLevel should be in the range of 0~100. 100 means 100
// percent of the flash power level. The default power level for focus is
// 20 percent.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_AF_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdAfGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdAfGetStatus_t;
// @Description: This command is used to get the current af algorithm status.
// It is used for debug purpouse.
//The result will be written to the buffer specified in the CmdAfGetStatus_t.
//The result is a struct of AfStatus_t.
typedef struct _AfStatus_t {
	AfMode_t mode;
	LensRange_t lensRange;
	uint32 lensPos;
	Window_t regionWindow[AF_WINDOW_ID_MAX];
	float sharpness[AF_WINDOW_ID_MAX];
	AfLockState_t lockState;
	AfSearchState_t searchState;
} AfStatus_t;
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//CPROC Command
//-------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enables the cproc module. The default status is
// enabled. Host can send this command at any time after the firmware is boot
// up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disables the cproc module. The default status is
// enabled. Host can send this command at any time after the firmware
// is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_SET_PROP
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCprocSetProp_t {
	CprocProp_t cprocProp;
} CmdCprocSetProp_t;
// @Description: This command set the cproc process property. This command can
// only be sent in streaming idle status.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_SET_CONTRAST
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCprocSetContrast_t {
	uint8 contrast;
} CmdCprocSetContrast_t;
// @Description: This command set the current streams contrast. Host can send
// this command at any time afer the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_SET_BRIGHTNESS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCprocSetBrightness_t {
	uint8 brightness;
} CmdCprocSetBrightness_t;
// @Description: This command set the current streams brightness. Host can
// send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_SET_SATURATION
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCprocSetSaturation_t {
	uint8 saturation;
} CmdCprocSetSaturation_t;
// @Description: This command set the current streams saturation. Host can
// send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_SET_HUE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCprocSetHue_t {
	uint8 hue;
} CmdCprocSetHue_t;
// @Description: This command set the current streams hue. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CPROC_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to get the current cprocs setting.
// @Type: Dynamic
// @ResponsePayload: CprocStatus_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//Gama Command
//------------




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GAMMA_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: Enable the gamma out process. The default is disabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GAMMA_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: Disable the gamma out process. The default is disabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GAMMA_SET_CURVE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdGammaSetCurve_t {
	GammaSegMode_t segMode;
	GammaCurve_t curve;
} CmdGammaSetCurve_t;
// @Description:
// Change the current gamma out curve. The default is gamma 2.2 curve.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GAMMA_MODE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdGammaMode_t {
	GammaMode_t gammaMode;
} CmdGammaMode_t;
// @Description: Change the Gamma Mode.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_GAMMA_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdGammaGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdGammaGetStatus_t;
// @Description: This command gets the gamma status. The bufAddLo and bufAddrHi
// specified the address of a struct GammaStatus_t and the bufSize
//should equal to GammaStatus_t. Firmware will fill the status data to this
//buf and send the RESP_ID_CMD_DONE response. Host can send this command at
//any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: GammaStatus_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//STNR Command
//------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SNR_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the snr block. By default, the
// snr block is enabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SNR_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the snr block. By default,
// the snr block is enabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_TNR_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to enable the tnr block. By default, the
// tnr block is enabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_TNR_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command is used to disable the tnr block. By default,
// the tnr block is enabled.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_STNR_UPDATE_PARAM
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdStnrUpdateParam_t {
	TdbStnr_t tdbStnr;
} CmdStnrUpdateParam_t;
// @Description: This command set the stnr parameter for the stream. It is
// used for debug. Host can send this command at any time after the firmware
// is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_STNR_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
typedef struct _CmdStnrGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdStnrGetStatus_t;
// @Description: This command gets STNR status of the specified stream. The
// result will be returned in the response payload as a struct of
// StnrStatus_t. Host can send this command at any time after the firmware is
// boot up.
// @Type: Dynamic
// @ResponsePayload: StnrStatus_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//Miscellaneous Debug Command
//---------------------------


//BLS Command
//-----------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_BLS_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enables the bls unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_BLS_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disables the bls unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_BLS_SET_BLACK_LEVEL
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdBlsSetBlackLevel_t {
	BlackLevel_t blackLevel;
} CmdBlsSetBlackLevel_t;
// @Description: This command set the fixed black level value for the stream.
// It is used for debug. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_BLS_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the black level setting of the specified
// stream. The result will be returned in the response payload as a struct of
// BlsStatus_t. Host can send this command at any time after the
// firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BlsStatus_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//DEGAMMA Command
//---------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEGAMMA_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the degamma unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEGAMMA_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the degamma unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEGAMMA_SET_CURVE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdDegammaSetCurve_t {
	DegammaCurve_t curve;
} CmdDegammaSetCurve_t;
// @Description: This command change the degamma curve setting for the stream.
// The default degamma curve is get from the calibration data. Host can send
// this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEGAMMA_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdDegammaGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdDegammaGetStatus_t;
// @Description: This command get the degamma status. The bufAddLo and
// bufAddrHi specified the address of a struct DegammaStatus_t and the bufSize
// should equal to DegammaStatus_t. Firmware will fill the status data to this
// buf and send the RESP_ID_CMD_DONE response. Host can send this command at
// any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//DPF Command
//-----------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPF_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the dpf unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPF_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the dpf unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPF_CONFIG
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdDpfConfig_t {
	TdbDpf_t config;
} CmdDpfConfig_t;
// @Description: This command change the dpf config for the stream.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPF_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdDpfGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdDpfGetStatus_t;
// @Description: This command get the dpf status. The bufAddLo and bufAddrHi
// specified the address of a struct DpfStatus_t and the bufSize should equal
// to DpfStatus_t. Firmware will fill the status data to this buf and send the
// RESP_ID_CMD_DONE response. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//DPCC Command
//------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPCC_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the dpcc unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPCC_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the dpcc unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPCC_CONFIG
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdDpccConfig_t {
	DpccStaticConfig_t config;
} CmdDpccConfig_t;
// @Description: This command change the dpcc config for the stream.
// Host can send this command at any time after the firmware is boot up.
// @Type: Static
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DPCC_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdDpccGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdDpccGetStatus_t;
// @Description: This command get the dpcc status. The bufAddLo and bufAddrHi
// specified the address of a struct DpccStatus_t and the bufSize should equal
// to DpccStatus_t. Firmware will fill the
// status data to this buf and send the RESP_ID_CMD_DONE response. Host can
// send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//CAC Command
//-----------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAC_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the cac unit. Host can send this command
//  at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAC_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the cac unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAC_CONFIG
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCacConfig_t {
	TdbCac_t tdbCac;
} CmdCacConfig_t;
// @Description: This command change the cac config for the stream.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CAC_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCacGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdCacGetStatus_t;
// @Description: This command get the cac status. The bufAddLo and bufAddrHi
// specified the address of a struct CacStatus_t and the bufSize should equal
// toCacStatus_t. Firmware will fill the status data to this buf and send the
// RESP_ID_CMD_DONE response. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//WDR Command
//-----------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_WDR_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the wdr unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_WDR_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the wdr unit. Host can send this command
// at any time after the
//	 firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_WDR_SET_CURVE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdWdrSetCurve_t {
	WdrCurve_t curve;
} CmdWdrSetCurve_t;
// @Description: This command change the wdr curve setting for the stream.
// The default wdr curve is get from the calibration data. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_WDR_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdWdrGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdWdrGetStatus_t;
// @Description: This command get the wdr status. The bufAddLo and bufAddrHi
// specified the address of a struct WdrStatus_t and the bufSize should equal
// to WdrStatus_t. Firmware will fill the status data to this buf and send
// the RESP_ID_CMD_DONE response. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//CNR Command
//-----------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CNR_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the cnr unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CNR_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the cnr unit. Host can send this command
// at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CNR_SET_PARAM
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCnrSetParam_t {
	CnrParam_t cnrParam;
} CmdCnrSetParam_t;
// @Description: This command change the cnr parameter setting for the stream.
// Currently no CNR parameter is in the tuning data. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_CNR_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdCnrGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdCnrGetStatus_t;
// @Description:
// This command get the cnr status. The bufAddLo and bufAddrHi specified the
// address of a struct CnrStatus_t and the bufSize should equal to
// CnrStatus_t. Firmware will fill the status data to this buf and send the
// RESP_ID_CMD_DONE response. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//DEMOSAIC Command
//----------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEMOSAIC_SET_THRESH
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdDemosaicSetThresh_t {
	DemosaicThresh_t thresh;
} CmdDemosaicSetThresh_t;
// @Description:
// This command change the demosaic threshold value for the stream.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEMOSAIC_GET_THRESH
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the demosaic threshold value of the
// specified stream. The result will be returned in the response payload
// as a struct of DemosaicThresh_t. Host can send this command at any time
// after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: DemosaicThresh_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_DEMOSAIC_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command gets the demosaic status value of the specified
// stream. The result will be returned in
// the response payload as a struct of DemosaicStatus_t. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: DemosaicStatus_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//FILTER Command
//--------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_FILTER_ENABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command enable the filter unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_FILTER_DISABLE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter: None
// @Description: This command disable the filter unit. Host can send this
// command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_FILTER_SET_CURVE
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdFilterSetCurve_t {
	DenoiseFilter_t curve;
} CmdFilterSetCurve_t;
// @Description: This command changes the de-noising filter curve setting for
// the stream. The default filter
// curve is gotten from the calibration data. Host can send this command at
// any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_FILTER_GET_STATUS
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdFilterGetStatus_t {
	uint32 bufAddrLo;
	uint32 bufAddrHi;
	uint32 bufSize;
} CmdFilterGetStatus_t;
// @Description: This command gets the filter status. The bufAddLo and
// bufAddrHi specified the address of a struct IspFilterStatus_t and the
// bufSize should equal to IspFilterStatus_t. Firmware will fill the
// status data to this buf and send the RESP_ID_CMD_DONE response.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: BufCheckSum_t
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_FILTER_SET_SHARPEN
// @CmdChannel: StreamX
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdFilterSetSharpen_t {
	SharpenFilter_t curve;
} CmdFilterSetSharpen_t;
// @Description: This command changes the sharpening filter curve setting for
// the stream. The default filter
// curve is gotten from the calibration data. Host can send this command at
// any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//Frame Control Command
//---------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SEND_FRAME_CONTROL
// @CmdChannel: NonBlock
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdFrameControl_t {
	FrameControl_t frameControl;
} CmdFrameControl_t;
// @Description: This command is used to send a frame control command.
// Which is used to gather all the control information in one command.
// The control commands are classified by two types. The half control and
// full control. For the half control, firmware will do all the control from
// this command and at the same time also get the control information from
// the stream control queue like the output channels, zoom window, cproc
// settings, etc.
// For the full control, firmware will get all the control information from
// this command includes the channel output buffer such as preview buffer,
// video buffer, zsl buffer, raw buffer, etc.
// Host can send this command at any time after the firmware is boot up.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SEND_MODE3_FRAME_INFO
// @CmdChannel: NonBlock
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSendMode3FrameInfo_t {
	Mode3FrameInfoHost2FW_t frameInfo;
} CmdSendMode3FrameInfo_t;
// @Description: This command sends a mode3 post process frame info to
// firmware. It is used for STREAM_MODE_3
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SEND_MODE4_FRAME_INFO
// @CmdChannel: NonBlock
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter:
typedef struct _CmdSendMode4FrameInfo_t {
	Mode4FrameInfo_t frameInfo;
} CmdSendMode4FrameInfo_t;
// @Description: This command sends a mode4 post process frame info to
// firmware. It is used for STREAM_MODE_4
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Processed
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//BUFFER Command
//--------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @CmdId: CMD_ID_SEND_BUFFER
// @CmdChannel: NonBlock
// @RespChannel: StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _CmdSendBuffer_t {
	BufferType_t bufferType;
	Buffer_t buffer;
} CmdSendBuffer_t;
// @Description: This command sends a free buffer for firmware to use.
//1. For BUFFER_TYPE_PREVIEW, BUFFER_TYPE_VIDEO, BUFFER_TYPE_ZSL,
//   the buf A, B, C are used according to the current ImageProp_t setting
//   of the corresponding channel. Host can send this preview/video/zsl/rawzsl
//   buffer at any time after the firmware is boot up.
//   All the preview/video/zsl/rawzsl buffer will be released by firmware
//   after processing a CMD_ID_STOP_STREAM command.
//2. For BUFFER_TYPE_RAW_ZSL and BUFFER_TYPE_RAW_TEMP, only the buf A is used
//   to capture the raw frame in RGB Bayer mode. The bufA and bufB are used
//   for RGB-IR mode.
//   The buf A is for RGB Bayer data, and the buf B is for IR data.
//3. For BUFFER_TYPE_HDR_STATS_DATA, only the buf A is used. The buf A size
//   should be enough to store the hdr stats data for one frame. Host should
//   send 3 buffers in order to enable HDR.
//4. For BUFFER_TYPE_META_INFO, it is used as the storage buffer for response
//   RESP_ID_FRAME_DONE. Only the buf A is used and the buffer size should be
//   enough to store the MetaInfo_t.
//5. For BUFFER_TYPE_FRAME_INFO, it is used as the storage buffer for response
//   RESP_ID_FRAME_INFO. Only the buf A is used and the buffer size should be
//   enough to store the Mode3FrameInfo_t.
// @Type: Dynamic
// @ResponsePayload: None
// @ResponseTime: Enqueued
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






/*---------------------------------------------------------------------------*/
/*			 Response Param List				     */
/*-------------------------------------------------------------------------- */


/*
 *Each response is detailed in the following columns:
 *
 *RESP_ID_XXX: The id fo the corresponding response.
 *
 *RespChannel: Specify which response channelthis kind response will send.
 *"Global" means it will send in the global response channel.
 *StreamX means it will send in the stream response channel.
 *The X specify which stream response channel it will send.
 *It can be 1, 2 or 3, depend on which stream this response is for.
 *Some response can both send in global channel and stream channel.
 *It will be specified as Global+StreamX.
 *
 *ParameterType: Describe how the response parameter is included.
 *"Direct" means the response parameter is stored
 *in the respParam field in the Resp_t structure.
 *"Indirect" means the response parameter is in another buffer.
 *The buffer address, size and checksum is specified in a struct of
 *RespParamPackage_t. The RmdParamPackage will be stored
 *in the respParam field in the Resp_t structure.
 *
 *Parameter: The response parameter definition for the specified response.
 *
 *Description: A detail description for the specified response
 */


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_CMD_DONE
// @RespChannel: Global+StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _RespCmdDone_t {
	uint32 cmdSeqNum;//The host2fw command seqNum. To indicate which
				//command this response refer to.
	uint32 cmdId;	//The host2fw command id for host double check.
	uint16 cmdStatus;//Indicate the command process status. 0 means
			//success. 1 means fail. 2 means skipped
	uint16 errorCode;//If the cmdStatus is 1, that means the command is
			//processed fail, host can check the errorCode
			//to get the detail error information
	uint8 payload[36];//The response payload will be in different struct
			//type accroding to different cmd done response.
			//See the detailed command for reference.
} RespCmdDone_t;
// @Description: This response is used for response to each host command.
// Each host command will get this kind response after firmware has
// finished the command.
// For "Frame Control Command" and
// "Buffer Command", firmware will send the RESP_ID_CMD_DONE response
// after these commands are
// accepted. For other commands, the RESP_ID_CMD_DONE will send
// when it has been processed.
// @CmdStatus:
#define CMD_STATUS_SUCCESS (0)
#define CMD_STATUS_FAIL					(1)
#define CMD_STATUS_SKIPPED (2)
// @ErrorCode:
#define RESP_ERROR_CODE_NO_ERROR			(0)
#define RESP_ERROR_CODE_CALIB_NOT_SETUP			(1)
#define RESP_ERROR_CODE_SENSOR_PROP_NOT_SETUP		(2)
#define RESP_ERROR_CODE_UNSUPPORTED_SENSOR_INTF		(3)
#define RESP_ERROR_CODE_SENSOR_RESOLUTION_INVALID	(4)
#define RESP_ERROR_CODE_UNSUPPORTED_STREAM_MODE		(5)
#define RESP_ERROR_CODE_UNSUPPORTED_SENSOR_ID		(6)
#define RESP_ERROR_CODE_ASPECT_RATIO_WINDOW_INVALID	(7)
#define RESP_ERROR_CODE_UNSUPPORTED_IMAGE_FORMAT	(8)
#define RESP_ERROR_CODE_OUT_RESOLUTION_OUT_OF_RANGE	(9)
#define RESP_ERROR_CODE_LOG_WRONG_PARAMETER		(10)
#define RESP_ERROR_CODE_INVALID_BUFFER_SIZE		(11)
#define RESP_ERROR_CODE_INVALID_AWB_STATE		(12)
#define RESP_ERROR_CODE_INVALID_AF_STATE		(13)
#define RESP_ERROR_CODE_INVALID_AE_STATE		(14)
#define RESP_ERROR_CODE_UNSUPPORTED_CMD			(15)
#define RESP_ERROR_CODE_QUEUE_OVERFLOW			(16)
#define RESP_ERROR_CODE_SENSOR_ID_OUT_OF_RANGE		(17)
#define RESP_ERROR_CODE_CHECK_SUM_ERROR			(18)
#define RESP_ERROR_CODE_BUFFER_SIZE_ERROR		(19)
#define RESP_ERROR_CODE_UNSUPPORTED_BUFFER_TYPE		(20)
#define RESP_ERROR_CODE_UNSUPPORTED_COMMAND_ID		(21)
#define RESP_ERROR_CODE_STREAM_ID_OUT_OF_RANGE		(22)
#define RESP_ERROR_CODE_INVALID_STREAM_STATE		(23)
#define RESP_ERROR_CODE_INVALID_STREAM_PARAM		(24)
#define RESP_ERROR_CODE_UNSUPPORTED_MULTI_STREAM_MODE1	(25)
#define RESP_ERROR_CODE_UNSUPPORTED_LOG_DEBUG		(26)
#define RESP_ERROR_CODE_PACKAGE_SIZE_ERROR		(27)
#define RESP_ERROR_CODE_PACKAGE_CHECK_SUM_ERROR		(28)
#define RESP_ERROR_CODE_INVALID_PARAM			(29)
#define RESP_ERROR_CODE_TIME_OUT			(30)
#define RESP_ERROR_CODE_CANCEL				(31)
#define RESP_ERROR_CODE_REPEAT_TNR_REF_BUF		(32)
#define RESP_ERROR_CODE_ZOOM_WIINDOW_ALIGN_ERROR	(33)
#define RESP_ERROR_CODE_ZOOM_WIINDOW_SIZE_ERROR		(34)
#define RESP_ERROR_CODE_HARDWARE_ERROR			(35)
#define RESP_ERROR_CODE_MEMORY_LACK			(36)


//TODO: Add more error code here
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_FRAME_DONE
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter: MetaInfo_t
// @Description: This response is used for return a group of filled frame
// buffer to host. The buffer returned in this
//	 response is captured from the same sensor input frame.
// The indirect buffer is from the MetaInfo buffer queue.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_FRAME_INFO
// @RespChannel: StreamX
// @ParameterType: Indirect
// @Parameter: FrameInfo_t
// @Description: This response is used to return a Mode3FrameInfo_t to host
// for STREAM_MODE_3. The indirect buffer is from the
//	 FrameInfo buffer queue.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_ERROR
// @RespChannel: Global+StreamX
// @ParameterType: Direct
// @Parameter:
typedef struct _RespError_t {
	ErrorLevel_t errorLevel;

	ErrorCode_t errorCode;
} RespError_t;
// @Description: This response is used for firmware to tell host that some
// error happened inside firmware.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_HEART_BEAT
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespHeartBeat_t {
	uint32 cycleCountHi;
	uint32 cycleCountLo;
} RespHeartBeat_t;
// @Description: This response is used for firmware to inform host that a time
// of heartbeat tick happened(30ms each tick), the payload of RespHeartBeat_t
// has two fields which indicate the cycle count in iclk speed which after the
// firmware is boot up.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_GROUP_HOLD
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorGroupHold_t {
	SensorId_t sensorId;
} RespSensorGroupHold_t;
// @Description: This response is used for firmware to tell host to hold the
// group register of the sensor.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_GROUP_RELEASE
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorGroupRelease_t {
	SensorId_t sensorId;
} RespSensorGroupRelease_t;
// @Description: This response is used for firmware to tell host to release
// the group register of the sensor.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_AGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetAGain_t {
	SensorId_t sensorId;
	uint32 aGain;
} RespSensorSetAGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor analog gain. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_DGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetDGain_t {
	SensorId_t sensorId;
	uint32 dGain;
} RespSensorSetDGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor digital gain. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_ITIME
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetItime_t {
	SensorId_t sensorId;
	uint32 itime;
} RespSensorSetItime_t;
// @Description: This response is used for firmware to tell host to control
// sensor itime. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_HIGH_ITIME
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrHighItime_t {
	SensorId_t sensorId;
	uint32 itime; //In microsecond.
} RespSensorSetHdrHighItime_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr high itime. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_ITIME
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowItime_t {
	SensorId_t sensorId;
	uint32 itime; //In microsecond.
} RespSensorSetHdrLowItime_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low itime. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_ITIME_RATIO
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowItimeRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} RespSensorSetHdrLowItimeRatio_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low itime ratio. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_EQUAL_ITIME
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrEqualItime_t {
	SensorId_t sensorId;
	uint32 itime;
} RespSensorSetHdrEqualItime_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr time when the itime type is equal for high and low itime.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_HIGH_AGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrHighAGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrHighAGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr high analog gain. It is for debug.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_AGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowAGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrLowAGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low analog gain. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_AGAIN_RATIO
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowAGainRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} RespSensorSetHdrLowAGainRatio_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low analog gain ratio. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_EQUAL_AGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrEqualAGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrEqualAGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr time when the analog gain type is equal for high and
// low analog gain.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_HIGH_DGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrHighDGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrHighDGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr high digital gain. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_DGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowDGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrLowDGain_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low digital gain. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_LOW_DGAIN_RATIO
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrLowDGainRatio_t {
	SensorId_t sensorId;
	uint32 ratio;
} RespSensorSetHdrLowDGainRatio_t;
// @Description: This response is used for firmware to tell host to control
// sensor hdr low digital gain ratio. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_SENSOR_SET_HDR_EQUAL_DGAIN
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespSensorSetHdrEqualDGain_t {
	SensorId_t sensorId;
	uint32 gain; //Multiplied by 1000
} RespSensorSetHdrEqualDGain_t;
// @Description:
// This response is used for firmware to tell host to control sensor hdr time
//when the digital gain type is equal for high and low digital gain.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_LENS_SET_POS
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespLensSetPos_t {
	SensorId_t sensorId;
	uint32 lensPos;
} RespLensSetPos_t;
// @Description:
// This response is used for firmware to tell host to control
// sensor lens position. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_FLASH_SET_POWER
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespFlashSetPower_t {
	SensorId_t sensorId;
	uint32 powerLevel;
} RespFlashSetPower_t;
// @Description:
// This response is used for firmware to tell host to set the power level of
// the flash light. It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_FLASH_SET_ON
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespFlashSetOn_t {
	SensorId_t sensorId;
	uint32 time; //In microsecond
} RespFlashSetOn_t;
// @Description:
// This response is used for firmware to tell host to set the flash light
// on with the time It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @RespId: RESP_ID_FLASH_SET_OFF
// @RespChannel: Global
// @ParameterType: Direct
// @Parameter:
typedef struct _RespFlashSetOff_t {
	SensorId_t sensorId;
} RespFlashSetOff_t;
// @Description:
// This response is used for firmware to tell host to set the flash light off.
// It is for debug.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



#endif

/*
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _CAM_USERSPACE_H_
#define _CAM_USERSPACE_H_

#define MAX_SENSOR_COUNT 4

#define IOCTL_SENSOR (1<<8)
#define IOCTL_VCM (1<<9)
#define IOCTL_FLASH (1<<10)

#define SENSOR_IOCTL_POWERON			0
#define SENSOR_IOCTL_RESET			1
#define SENSOR_IOCTL_CONFIG			2
#define SENSOR_IOCTL_START			3
#define SENSOR_IOCTL_GET_CAPS			4
#define SENSOR_IOCTL_GET_GAIN_LIMIT		5
#define SENSOR_IOCTL_GET_INTEGRATION_TIME_LIMIT	6
#define SENSOR_IOCTL_STOP			7
#define SENSOR_IOCTL_IF_SUPPORT_AF		8
#define SENSOR_IOCTL_STREAM_ON			9
#define SENSOR_IOCTL_RESET_PHY			13
#define SENSOR_IOCTL_GROUP_HOLD			14
#define SENSOR_IOCTL_GROUP_RELEASE		15
#define SENSOR_IOCTL_START_PHY			16
/*
 *#define SENSOR_IOCTL_EXPOSURE_CONTROL		7
 *#define SENSOR_IOCTL_HDR_SUPPORT		18
 *#define SENSOR_IOCTL_HDR_GET_EXPO_RATIO	19
 *#define SENSOR_IOCTL_HDR_SET_EXPO_RATIO	20
 *#define SENSOR_IOCTL_HDR_ENABLE		21
 *#define SENSOR_IOCTL_HDR_DISABLE		22
 *#define SENSOR_IOCTL_HDR_SET_GAIN_MODE	24
 *#define SENSOR_IOCTL_HDR_SET_GAIN		25
 */

#define SENSOR_IOCTL_GET_RES_FPS		23
#define SENSOR_IOCTL_RELEASE			10
#define SENSOR_IOCTL_INIT			31
#define SENSOR_IOCTL_CONFIG_0			17
#define SENSOR_IOCTL_CONFIG_1			26
#define SENSOR_IOCTL_CONFIG_2			27
#define SENSOR_IOCTL_CONFIG_3			28
#define SENSOR_IOCTL_CONFIG_4			29
#define SENSOR_IOCTL_CONFIG_5			30
#define SENSOR_IOCTL_MAX_NUM			32

#define VCM_IOCTL_SETUP_MOTO_DRIVE		0
#define VCM_IOCTL_SET_FOCUS			1
#define VCM_IOCTL_GET_FOCUS			2
#define VCM_IOCTL_DEV_INIT			3
#define VCM_IOCTL_RELEASE			10
#define VCM_IOCTL_INIT				31
#define VCM_IOCTL_MAX_NUM			32

#define FLASH_IOCTL_CONFIG_FLASHLIGHT		0
#define FLASH_IOCTL_TRIGGER_FLASHLIGHT		1
#define FLASH_IOCTL_OFF_FLASHLIGHT		2
#define FLASH_IOCTL_CHECK_CONNECTION		3
#define FLASH_IOCTL_RELEASE			10
#define FLASH_IOCTL_INIT			31
#define FLASH_IOCTL_MAX_NUM			32

#define STORAGE_IOCTL_READ			0
#define STORAGE_IOCTL_MAX_LEN			1
#define STORAGE_IOCTL_RELEASE			10
#define STORAGE_IOCTL_INIT			31
#define STORAGE_IOCTL_MAX_NUM			32

#define ANYSIZE_ARRAY				1

/*
 *OPCODE -
 *	[15:0]- instruction
 *	[23:16] - reserved
 *	[31:24] - flags
 *
 *
 *
 * instruction opcode
 */
#define OPCODE_LABLE	0
#define OPCODE_MOV		1
#define OPCODE_ADD		2
#define OPCODE_SUB		3
#define OPCODE_MUL		4
#define OPCODE_DIV		5
#define OPCODE_AND		6
#define OPCODE_OR		7
#define OPCODE_NOT		8
#define OPCODE_EQU		9
#define OPCODE_BIG		10
#define OPCODE_SHL		11
#define OPCODE_SHR		12
#define OPCODE_JE		13
#define OPCODE_JNE		14
#define OPCODE_JB		15
#define OPCODE_JNB		16
#define OPCODE_JMP		17
#define OPCODE_UDELAY	18
#define OPCODE_I2CWR	19
#define OPCODE_I2CWR16	20
#define OPCODE_I2CRD	21
#define OPCODE_I2CRD16	22

#define OPCODE_FLAG_LDST 0x80000000
#define OPCODE_FLAG_IDST 0x40000000
#define OPCODE_FLAG_ODST 0x20000000
#define OPCODE_FLAG_CDST 0x10000000
#define OPCODE_FLAG_GDST 0x00800000

#define OPCODE_FLAG_LSRC 0x08000000
#define OPCODE_FLAG_ISRC 0x04000000
#define OPCODE_FLAG_OSRC 0x02000000
#define OPCODE_FLAG_CSRC 0x01000000
#define OPCODE_FLAG_GSRC 0x00080000

enum sensor_driver_mode {
	SENSOR_DRIVER_MODE_UNKNOWN = 0,
	SENSOR_DRIVER_MODE_STATIC,
	SENSOR_DRIVER_MODE_XML,
	SENSOR_DRIVER_MODE_XML_DCE,
};

/*
 * sensor information
 */
struct amdisp_sensor_info {
	uint32 sensor_id;
	uint32 interface_type;
	uint32 interface_address;
	uint32 program_count;
	uint32 name_size;
	uint32 unused;
	char name[8 * 8];
};

/*
 * sensor instruction
 */
struct amdisp_sensor_inst {
	uint32 op_code;
	uint32 dst;
	uint32 src;
	uint32 unused;
};

/*
 * sensor program
 */
struct amdisp_sensor_prog {
	uint32 io_code;
	uint32 instruction_count;
	uint32 lv_count;
	uint32 gv_count;
	struct amdisp_sensor_inst instruction[ANYSIZE_ARRAY];
};

/*
 * sensor initialization
 */
struct amdisp_sensor_init {
	uint32 mode;
	uint32 size;
	union {
		void *user_buffer;
		uint64 reserved;
	} b;
	struct amdisp_sensor_info sensor_info;

	struct amdisp_sensor_prog sensor_prog[ANYSIZE_ARRAY];
};

struct amd_sensor_ioctl {
	uint32 cmd;
	void *arg;
};

struct sensor_ioctl_pseudo_header {
	uint32 sensor_id;
	void *inbuf;
	void *outbuf;
};

struct storage_data {
	uint32 len;
	uint8 data[1];
};

#endif

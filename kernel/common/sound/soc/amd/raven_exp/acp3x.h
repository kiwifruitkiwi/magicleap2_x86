/*
 * header file for ACP PCI Driver
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __ACP3X_H__
#define __ACP3X_H__

#include "chip_offset_byte.h"
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>
#include<linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/pm_runtime.h>
#include "acp3x-fw-driver-common.h"

#define STATUS_SUCCESS 0x00
#define STATUS_UNSUCCESSFUL 0x01
#define STATUS_DEVICE_NOT_READY 0x02

#define ACP3x_PHY_BASE_ADDRESS 0x1240000
#define	ACP3x_I2S_MODE	0
#define ACP3x_SDW_MODE  0x01
#define	ACP3x_REG_START	0x1240000
#define	ACP3x_REG_END	0x1250200

#define ACP_SRAM_PTE_OFFSET	0x02050000
#define PAGE_SIZE_4K_ENABLE 0x2
#define ACP_SCRATCH_MEMORY_ADDRESS 0x02050000

#define ACP_CLK_ENABLE 0x01
#define ACP_CLK_DISABLE 0x00

#define ACP_PGFSM_CNTL_POWER_ON_MASK	0x01
#define ACP_PGFSM_CNTL_POWER_OFF_MASK	0x00
#define ACP_PGFSM_STATUS_MASK		0x03
#define ACP_POWERED_ON			0x00
#define ACP_POWER_ON_IN_PROGRESS	0x01
#define ACP_POWERED_OFF			0x02
#define ACP_POWER_OFF_IN_PROGRESS	0x03
#define ACP_SOFT_RESET_SOFTRESET_AUDDONE_MASK	0x00010001

#define IMAGEID_LENGTH                      (16)
#define PSP_VERSION                         (0x2)

#define XTS_PARAMETERS_BLOCK_SIZE           (256)
#define RSA_SIGNATURE_BLOCK_SIZE            (256)

#define N_IN_BYTES                          (256)

#define MODULUS_OFFSET                      (64 + N_IN_BYTES)
#define PUBLIC_EXPONENT_OFFSET              (64)
#define RSA_SIGNATURE_BLOCK_SIZE_CZ         (N_IN_BYTES)

#define ACP_RESPONSE_LIST_SIZE      256

#define FIRMWARE_ACP3X "fwimage_3_0.sbin"
#define FIRMWARE_ACP3X_UNSIGNED "fwimage_3_0.bin"
#define FIRMWARE_ACP3X_DATA_BIN "fwdata_3_0.bin"
#define ACP_ERROR_MASK                                0x20000000
#define ACP_DMA_MASK                                  0xFF
#define ACP_SHA_STAT                                  0x8000
#define ACP_EXT_INTR_ERROR_STAT                       0x20000000
#define ACP_DMA_STAT				      0xFF
#define FW_BIN_PTE_OFFSET                             0x00
#define FW_DATA_BIN_PTE_OFFSET                        0x08
#define ACP_LOGGER_PTE_OFFSET			      0x10

#define ACP_SYSTEM_MEMORY_WINDOW                      0x4000000
#define ACP_IRAM_BASE_ADDRESS                         0x000000
#define ACP_DATA_RAM_BASE_ADDRESS                     0x01000000
#define ACP_DEFAULT_DRAM_LENGTH                       0x00080000
#define ACP_DRAM_IRAM_LENGTH                          0x000A0000
#define ACP_SRBM_DATA_RAM_BASE_OFFSET		      0x0100C000
#define ACP_SRBM_DRAM_DMA_BASE_ADDR                   0x01014000
#define ACPBUS_REG_BASE_OFFSET                        ACP_DMA_CNTL_0
/** Maximum size of packets in command buffer */
#define	ACP_COMMAND_PACKET_SIZE     MAX_RESPONSE_BUFFER_SIZE

/** Time to wait for commands to be processed by firmware when command
 * ring buffer is full, in 10 msec units
 */
#define EV_COMMAND_WAIT_TIME            100       /* 1 sec */
#define EV_COMMAND_WAIT_TIME_JTAG       (1 * 6 * 100)  /* 5 mins */

/** Response polling period - in usec */
#define ACP_RESPONSE_POLL_PERIOD        5000

/* Ring buffer offset difference */
#define AOS_RING_OFFSET_DIFFERENCE      0x00000004

#define ACP_LOG_BUFFER_SIZE  0x1000
#define ACP_PAGE_SIZE 0x1000
#define ACP_FW_RUN_STATUS_ACK  0x1234
#define ACP_DRAM_PAGE_COUNT 128
#define ACP_DMA_CH_RUN 0x02
#define ACP_DMA_CH_IOC_ENABLE 0x04
#define ACP_DEVICE_ID 0x15e2

#define ACP_MAXIMUM_ACLK     0x1C9C3800      /* 480MHz */
#define ACP_MINIMUM_ACLK     0x0BEBC200      /* 200Mhz */
#define ACP_200MHZ_ACLK      0x0BEBC200      /* 200Mhz */
#define ACP_400MHZ_ACLK      0x17D78400      /* 400Mhz */
#define ACP_DMA_BUFFER_SIZE      0x2000
#define ACP_DMA_NORMAL       0x00
#define ACP_DMA_CIRCULAR     0x01
#define ACP_LOG_BUFFER_PAGE_COUNT 0x01
#define ACP_DMA_BUFFER_PAGE_COUNT 0x02
#define ACP_DMA_RUN_IOC_DISABLE_MASK 0x19
#define ACP_DMA_DISABLE_MASK 0x11

#define I2S_SP_INSTANCE 0x00
#define I2S_BT_INSTANCE 0x01

/* The following security header is for rv platform
 * image_id[IMAGEID_LENGTH] - set at build time (16 bytes)
 * header_version - version of the header (4 bytes)
 * size_fw_to_be_signed - Size of FW portion included in signature in bytes –
 * set at build time (4 bytes)
 * encryption_option - Encryption Option – set at sign time (4bytes)
 * 0: Image Not Encrypted (Current POR)
 * 1: Image Encrypted
 * encrypt_alg_id - Encryption algorithm ID – default to 0 (4 bytes)
 * encrypt_params[4] - Encryption Parameters – default to 0 (16 bytes)
 * sign_option - Signing option: 0 - not signed 1 – signed (4 bytes)
 * sign_alg_id - Signature algorithm ID (4 bytes)
 * sign_params[4] - Signature parameter (16 bytes)
 * image_compression_option - Compression option – set at build time (4 bytes)
 * 0: Image Not Compressed (Current POR)
 * 1: Image Compressed
 * compression_alg_id - Compression Algorithm ID (4 bytes)
 * compression_params[4] - Compression Parameters (16 bytes)
 * fw_version - Firmware Version – Set at build time (4 bytes)
 * apu_family_id - APU Family ID or SoC ID – Set at build time (4 bytes)
 * There is no global list. Each IP can use their own values
 * fw_load_addr - Firmware Load address, defaults to 0 (4 bytes)
 * signed_fw_image_size - Size of the entire image after signing –
 * set at sign time (4 bytes)
 * size_fw_unsigned - Size of the unsigned portion of the firmware –
 * set at build time (4 bytes)
 * reserved[3] - Reserved field -  Set to zero at build time (12 bytes)
 * vendor_info[4] - FW Vendor specific info – set at build time (16 bytes)
 * For most IP this should not be required
 * signing_tool_info[4] - Signing tool specific information (16 bytes)
 * reserved1[24] - Reserved field -  Set to zero at build time (96 bytes)
 *
 */
struct app_secr_header_rv {
	unsigned char   image_id[IMAGEID_LENGTH];
	unsigned int    header_version;
	unsigned int    size_fw_to_be_signed;
	unsigned int    encryption_option;
	unsigned int    encrypt_alg_id;
	unsigned int    encrypt_params[4];
	unsigned int    sign_option;
	unsigned int    sign_alg_id;
	unsigned int    sign_params[4];
	unsigned int    image_compression_option;
	unsigned int    compression_alg_id;
	unsigned int    compression_params[4];
	unsigned int    fw_version;
	unsigned int    apu_family_id;
	unsigned int    fw_load_addr;
	unsigned int    signed_fw_image_size;
	unsigned int    size_fw_unsigned;
	unsigned int    reserved[3];
	unsigned int    vendor_info[4];
	unsigned int    signing_tool_info[4];
	unsigned int    reserved1[24];

};

/* Structure of PSP token format attached to the signed binary
 * version_id - Version for key format structure - 4 bytes
 * this_key_id[4] - A universally unique key identifier - 16 bytes
 * certifying_key_id[4] - A universally unique key identifier corresponds
 * to the certifying key  - 16 bytes
 * key_usage_flag -  Signing Key Usage Flag - 4 bytes
 *       0 – Key authorized to sign AMD developed PSP Boot Loader
 *       and AMD developed PSP FW components and SMU FW.
 *       1 – Key authorized to sign BIOS
 *       2 – Key authorized to PSP FW (both AMD developed and OEM developed)
 * reserved[4] - Reserved -   16 bytes
 * public_exponent_size - Public Exponent Size  -   4 bytes
 * modulus_size - Modulus size  -   4 bytes
 * pub_exp[64] - public exponent size  - 64 bytes
 * N = 256 or 512 depending on public exponent size
 * modulus[64] -  N = 256 or 512 depending on modulus size
 * Modulus value zero extended to size of N*8 bits
 * rsa_signature_public_key_using_private_key[256];
 */

struct psp_token_format {
	unsigned int    version_id;
	unsigned int    this_key_id[4];
	unsigned int    certifying_key_id[4];
	unsigned int    key_usage_flag;
	unsigned int    reserved[4];
	unsigned int    public_exponent_size;
	unsigned int    modulus_size;
	unsigned int    pub_exp[64];
	unsigned int    modulus[64];
	char    rsa_signature_public_key_using_private_key[256];

};

struct acp_response {
	unsigned short  command_id;
	unsigned int   command_number;
	int   response_status;
	unsigned int *p_event;
};

struct acp_response_list {
	struct acp_response resp[ACP_RESPONSE_LIST_SIZE];
	int last_resp_write_index;
	int last_resp_update_index;
};

struct acpev_command {
	unsigned short cmd_id;
	unsigned short cmd_size;
};

enum acp_dma_priority_level {
	NORMAL = 0x0,
	HIGH = 0x1,
	FORCE_SIZE = 0xFF
};

struct acpev_resp_status {
	u32    cmd_number;
	u32    status;
};

struct acpev_resp_version {
	u32 cmd_number;
	u32 version_id;
};

/* acpx_dev_data - ACP PCI Driver private data structure
 * acp_fw - pointer to firmware image
 * acp_fw_data - pointer to firmware data bin
 * dma_addr - allocated buffer physical address for firmware image
 * virt_addr - allocated buffer vitural address for firmware image
 * fw_data_dma_addr - allocated buffer physical address for firmware data bin
 * fw_data_virt_addr - allocated buffer virtual address for firmware data bin
 * acp_log_dma_addr - acp log buffer physical address
 * acp_log_virt_addr - acp log buffer virtual address
 * acp_dma_virt_addr - acp dma test buffer virtual address
 * acp_dma_addr - acp dma test buffer physical address
 * fw_bin_size - firmware image size
 * fw_data_bin_size - firmware data bin size
 * sha_irq_stat - flag will set when IRQ received after SHA DMA completion
 * ev_command_wait_time - ev command wait time
 * fw_bin_page_count - firmware image page count
 * fw_data_bin_page_count - firmware data bin page count
 * acp_sregmap - structure mapped to scratch memory
 * acp_sregmap_context - structure to store scratch memory context
 * is_acp_active - this flag is used to check status before sending
 * ev command. This flag will be set/cleared during suspend/resume callbacks.
 * acp_ev_cmd_mutex - acp command buffer mutex
 * acp_ev_res_mutex - acp response buffer mutex
 * cmd_number - acp ev command number
 * current_command_number - acp current ev command number
 * response_wait_event - acp host driver will wait for clear this event when
 * acp response list is full
 * evresp_event - This event is set when response is received for blocking
 * ev commands.
 * fw_load_error - this flag is set when acp firmware loading is failed
 * dma_ioc_event - this flag is set when ACP DMA ioc stat is received
 * aclk_clock_freq - ACP ACLK clock frequency
 * acp_response_list - acp response list structure
 * command_response - acp response structure
 * evresponse_work - bottom half for ev response handling
 * probe_work - work queue for acp pci driver probe continuation
 * evresp - ev response wait queue
 * wait_queue_sha_dma - wait queue for polling sha dma completion
 * ev_event - wait queue for polling events for ev commands
 * dma_queue - acp dma transfer wait queue
 * packet - acp response buffer
 * acp_config_dma_descriptor - acp dma descriptor structure
 */
struct acp3x_dev_data {
	void __iomem *acp3x_base;
	const struct firmware *acp_fw;
	const struct firmware *acp_fw_data;
	dma_addr_t dma_addr;
	u8 *virt_addr;
	dma_addr_t fw_data_dma_addr;
	u8 *fw_data_virt_addr;

	dma_addr_t acp_log_dma_addr;
	void *acp_log_virt_addr;

	dma_addr_t acp_dma_addr;
	void *acp_dma_virt_addr;

	unsigned int fw_bin_size;
	unsigned int fw_data_bin_size;

	u32 sha_irq_stat;
	u32 ev_command_wait_time;
	u32 fw_bin_page_count;
	u32 fw_data_bin_page_count;
	struct acp_scratch_register_config *acp_sregmap;
	struct acp_scratch_register_config *acp_sregmap_context;

	unsigned int is_acp_active;
	/* acp command buffer mutex */
	struct mutex acp_ev_cmd_mutex;
	/* acp response buffer mutex */
	struct mutex acp_ev_res_mutex;
	unsigned int cmd_number;
	unsigned int current_command_number;
	unsigned int response_wait_event;
	unsigned int evresp_event;
	unsigned int fw_load_error;
	unsigned int aclk_clock_freq;
	unsigned int dma_ioc_event;
	struct acp_response_list resp_list;
	struct acp_response command_response;

	struct work_struct evresponse_work;
	struct work_struct probe_work;

	wait_queue_head_t evresp;
	wait_queue_head_t wait_queue_sha_dma;
	wait_queue_head_t ev_event;
	wait_queue_head_t dma_queue;
	unsigned char    packet[ACP_COMMAND_PACKET_SIZE];
	struct acp_config_dma_descriptor dscr_info[1024];

};

enum buffer_type {
	FW_BIN = 0,
	FW_DATA_BIN,
	ACP_LOGGER,
	ACP_DMA_BUFFER
};

struct stream_param {
	unsigned int sample_rate;
	unsigned int bit_depth;
	unsigned int ch_count;
	unsigned int i2s_instance;
};

struct i2s_dma_stop_param {
	unsigned int cmd_id;
	unsigned int i2s_instance;
};

static inline u32 rv_readl(void __iomem *base_addr)
{
	return readl(base_addr - ACP3x_PHY_BASE_ADDRESS);
}

static inline void rv_writel(u32 val, void __iomem *base_addr)
{
	writel(val, base_addr - ACP3x_PHY_BASE_ADDRESS);
}

void acpbus_trigger_host_to_dsp_swintr(struct acp3x_dev_data *adata);

int send_ev_command(struct acp3x_dev_data *adata, unsigned int val);

int acp3x_set_fw_logging(struct acp3x_dev_data *adata);

int acp3x_proxy_driver_init(void);

void acp3x_proxy_driver_exit(void);

void acp_copy_to_scratch_memory(unsigned char *ring_buffer,
				u32  offset,
				void *write_buffer,
				u32 bytes_to_write);
void acp_copy_from_scratch_memory(unsigned char *ring_buffer,
				  u32  offset,
				  void *read_buffer,
				  u32 bytes_to_write);
int ev_acp_i2s_dma_start(struct acp3x_dev_data *acp3x_data,
			 struct stream_param *param);

int ev_acp_i2s_dma_stop(struct acp3x_dev_data *acp3x_data,
			unsigned int instance);
int ev_set_acp_clk_frequency(u32 clock_type,
			     u64 clock_freq,
			     struct acp3x_dev_data *acp3x_data);
int ev_acp_memory_pg_shutdown_enable(struct acp3x_dev_data *acp3x_data,
				     unsigned int mem_bank);
int ev_acp_memory_pg_shutdown_disable(struct acp3x_dev_data *acp3x_data,
				      unsigned int mem_bank);
int ev_acp_memory_pg_deepsleep_on(struct acp3x_dev_data *acp3x_data,
				  unsigned int mem_bank);
int ev_acp_memory_pg_deepsleep_off(struct acp3x_dev_data *acp3x_data,
				   unsigned int mem_bank);
#endif

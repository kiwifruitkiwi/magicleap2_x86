#ifndef _PARAM_TYPES_H_
#define _PARAM_TYPES_H_

#include "SensorTdb.h"

/*--------------------------------------------------------------------------- */
/*			 Command Structure				      */
/*--------------------------------------------------------------------------- */

//The Cmd_t should be totally 64 bytes.
typedef struct _Cmd_t {
	uint32 cmdSeqNum; /*The cmdSeqNum is the sequence number of host
			   *command in all command channels.
			   *It should be increased 1 by one command.
			   *The first command should be set to 1.
			   */
	uint32 cmdId;	 /*The actual command identifier */

	uint32 cmdParam[12];/*The command parameter. For the direct command,
			     *it is defined by the command
			     *For the indirect command, it is a struct of
			     *CmdParamPackage_t
			     */
	uint16 cmdStreamId; /*Used to identify which stream this command is
			     *applied for the stream command.
			     *See the StreamId_t type for reference. For global
			     *command, this field is ignored and should be set
			     *to STREAM_ID_INVALID.
			     */
	uint8 reserved[5];

#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	uint8 reserved_1[192];
#endif

	uint8 cmdCheckSum; /*The byte sum of all the fieldes defined in Cmd_t
			    *except the cmdCheckSum.
			    */
} Cmd_t;


typedef struct _CmdParamPackage_t {
	uint32 packageAddrLo;/*The low 32 bit address of the package address */
	uint32 packageAddrHi; //The high 32 bit address of the package address
	uint16 packageSize; /*The total package size in bytes. */
	uint8 packageCheckSum; /*The byte sum of the package */
} CmdParamPackage_t;


/*-------------------------------------------------------------------------- */
/*			 Response Structure				     */
/*-------------------------------------------------------------------------- */

//The Resp_t should be totally 64 bytes.
typedef struct _Resp_t {
	uint32 respSeqNum;/*The respSeqNunm is the sequence number of firmware
			   *response. It should be increased 1 by one response.
			   *The initial number should be 1
			   */
	uint32 respId;	/*The response identifier */
	uint32 respParam[12]; /*The payload of the response. Different respId
			       *may have different payload structure definition
			       */

	uint8 breserved[7];
#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	uint8 reserved_1[192];
#endif
	uint8 respCheckSum;/*The byte sum of all the fieldes defined in Resp_t
			    *except respCheckSum.
			    */
} Resp_t;

typedef struct _RespParamPackage_t {
	uint32 packageAddrLo; /*The low 32 bit address of the package address*/
	uint32 packageAddrHi; //The high 32 bit address of the package address
	uint16 packageSize; /*The total package size in bytes. */

	uint8 packageCheckSum; /*The byte sum of the package */
} RespParamPackage_t;

#define MAX_LIGHT_NAME_LENGTH		(32)
#define MAX_LIGHT_SOURCE_INDEX		(32)


typedef enum _StreamId_t {
	STREAM_ID_INVALID = -1,
	STREAM_ID_1 = 0,
	STREAM_ID_2 = 1,
	STREAM_ID_3 = 2,
	STREAM_ID_MAX
} StreamId_t;

typedef enum _SensorId_t {
	SENSOR_ID_INVALID = -1,
	SENSOR_ID_A = 0,
	SENSOR_ID_B = 1,
	SENSOR_ID_RDMA = 2,
	SENSOR_ID_MAX
} SensorId_t;

typedef enum _I2cDeviceId_t {
	I2C_DEVICE_ID_INVALID = -1,
	I2C_DEVICE_ID_A = 0,
	I2C_DEVICE_ID_B = 1,
	I2C_DEVICE_ID_MAX
} I2cDeviceId_t;

typedef enum _StreamMode_t {
	STREAM_MODE_INVALID,
	STREAM_MODE_0,
	STREAM_MODE_1,
	STREAM_MODE_2,
	STREAM_MODE_3,
	STREAM_MODE_4,
	STREAM_MODE_MAX
} StreamMode_t;

typedef enum _ModId_t {
	MOD_ID_ALL		= 0,
	MOD_ID_SC_MGR		= 1,
	MOD_ID_CMD_DISPATCH	= 2,
	MOD_ID_CMD_RESP_AGENT	= 3,
	MOD_ID_PIPE_MGR_RT	= 4,
	MOD_ID_PIPE_MGR_CORE	= 5,
	MOD_ID_PIPE_CMD		= 6,
	MOD_ID_AF_MGR		= 7,
	MOD_ID_AE_MGR		= 8,
	MOD_ID_AWB_MGR		= 9,
	MOD_ID_TASK_SCHEDULE	= 10,
	MOD_ID_MAIN		= 11,
	MOD_ID_QUEUE		= 12,
	MOD_ID_RING_ARRAY	= 13,
	MOD_ID_PROFILER		= 14,
	MOD_ID_INTR_DISPATCH	= 15,
	MOD_ID_ASSERT		= 16,
	MOD_ID_CAMERIC		= 17,
	MOD_ID_TDB_MGR		= 18,
	MOD_ID_PIPE_MGR_INTR	= 19,
	MOD_ID_PMB		= 20,
	MOD_ID_HEART_BEAT	= 21,
	MOD_ID_TIMER_MGR	= 22,
	MOD_ID_DEVICE_SCRIPT	= 23,
	MOD_ID_DEVICE_MGR	= 24,
	MOD_ID_I2C_CORE		= 25,
	MOD_ID_STREAM_MGR	= 26,
	MOD_ID_MODULE_SETUP	= 27,
	MOD_ID_PIPE_MGR_RT_CMN	= 28,
	MOD_ID_UTL_CMN		= 29,
	MOD_ID_MEM_POOL		= 30,
	MOD_ID_PIPE_MGR_EXT	= 31,
	MOD_ID_PIPE_MGR_END	= 32,
	MOD_ID_GMC		= 33,
	MOD_ID_SCENE_DETECT_MGR = 34,
	MOD_ID_MAX
} ModId_t;

typedef enum _LogLevel_t {
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_ERROR = 3,
	LOG_LEVEL_MAX
} LogLevel_t;

typedef enum _GmcPrefetchType_t {
	GMC_PREFETCH_TYPE_DISABLE,	//disable all
	GMC_PREFETCH_TYPE_ENABLE_STAGE1,//only enable stage1
	GMC_PREFETCH_TYPE_ENABLE_STAGE2,//only enable stage2
	GMC_PREFETCH_TYPE_ENABLE,	//enable both stage1 and stage2
	GMC_PREFETCH_TYPE_MAX
} GmcPrefetchType_t;

typedef enum _SensorIntfType_t {
	SENSOR_INTF_TYPE_MIPI = 0,
	SENSOR_INTF_TYPE_PARALLEL = 1,
	SENSOR_INTF_TYPE_RDMA = 2,
	SENSOR_INTF_TYPE_MAX
} SensorIntfType_t;

typedef enum _ImageFormat_t {
	IMAGE_FORMAT_INVALID,
	IMAGE_FORMAT_NV12,
	IMAGE_FORMAT_NV21,
	IMAGE_FORMAT_I420,
	IMAGE_FORMAT_YV12,
	IMAGE_FORMAT_YUV422PLANAR,
	IMAGE_FORMAT_YUV422SEMIPLANAR,
	IMAGE_FORMAT_YUV422INTERLEAVED,
	IMAGE_FORMAT_RGBBAYER8,
	IMAGE_FORMAT_RGBBAYER10,
	IMAGE_FORMAT_RGBBAYER12,
	IMAGE_FORMAT_RGBIR8,
	IMAGE_FORMAT_RGBIR10,
	IMAGE_FORMAT_RGBIR12,
	IMAGE_FORMAT_MAX
} ImageFormat_t;

typedef struct _Window_t {
	uint32 h_offset;
	uint32 v_offset;
	uint32 h_size;
	uint32 v_size;
} Window_t, window_t;


typedef enum _ProfilerId_t {
	PROFILER_ID_INVALID = -1,
	//---SCHEDULER
	PROFILER_ID_SCHEDULER_HAS_PENDING_TASK = 0,
	PROFILER_ID_SCHEDULER_CMD_DISPATCH_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_PIPE_RT_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_PIPE_CORE_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_AF_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_AE_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_AWB_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_I2C_HAS_PENDING_TASK,
	PROFILER_ID_SCHEDULER_HEART_BEAT_HAS_PENDING_TASK,
	//---AWB
	PROFILER_ID_AWB_SCHEDULE,
	PROFILER_ID_AWB_WP_REVERT,
	PROFILER_ID_AWB_ILLU_EST,
	PROFILER_ID_AWB_WB_GAIN,
	PROFILER_ID_AWB_ACC,
	PROFILER_ID_AWB_ALSC,
	PROFILER_ID_AWB_PUSH_AWB_CTRL_INFO,
	PROFILER_ID_AWB_PUSH_AWBM_INFO,
	PROFILER_ID_AWB_GET_CTRL_INFO,
	//---AE
	PROFILER_ID_AE_SCHEDULE,
	PROFILER_ID_AE_ALG_RUN,
	PROFILER_ID_AE_ALG_DONE,
	PROFILER_ID_AE_PUSH_AEM_INFO,
	PROFILER_ID_AE_GET_CTRL_INFO,
	//---AF
	PROFILER_ID_AF_SCHEDULE,
	PROFILER_ID_AF_PROCESS_ONE_FRAME,
	PROFILER_ID_AF_SET_AFM_INFO,
	PROFILER_ID_AF_GET_CTRL_INFO,
	//---PIPE RT
	PROFILER_ID_PIPE_RT_SCHEDULE,
	PROFILER_ID_PIPE_RT_START,
	PROFILER_ID_PIPE_RT_RESETUP_FRAME,
	//i2c access in rt resetup frame
	PROFILER_ID_PIPE_RT_RESETUP_FRAME_DEVICE,
	PROFILER_ID_PIPE_RT_SEND_FRAMEINFO,
	//---PIPE CORE
	PROFILER_ID_PIPE_CORE_SCHEDULE,
	PROFILER_ID_PIPE_CORE_GET_CFG_INFO,
	PROFILER_ID_PIPE_CORE_GET_CTRL_INFO,
	PROFILER_ID_PIPE_CORE_SEND_FRAME_DONE,
	PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME,
	PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME_PREP, //prep in stg2 one frame
	PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME_ISP, // isp in stg2 one frame
	PROFILER_ID_PIPE_CORE_MODE2_INIT_CORE_FRAMEINFO,
	PROFILER_ID_PIPE_CORE_MODE3_INIT_CORE_FRAMEINFO,
	PROFILER_ID_PIPE_CORE_MODE4_INIT_CORE_FRAMEINFO,
	PROFILER_ID_PIPE_CORE_PREP_LSC_TABLE_CONFIG,
	PROFILER_ID_PIPE_CORE_LSC_TABLE_CONFIG,
	//---I2C CORE
	PROFILER_ID_I2C_CORE_SCHEDULE,
	PROFILER_ID_I2C_READ,
	PROFILER_ID_I2C_WRITE,
	//---CMD DISPATCH
	PROFILER_ID_CMD_DISPATCH_SCHEDULE,
	PROFILER_ID_CMD_DISPATCH_FLUSH,
	PROFILER_ID_CMD_DISPATCH_GLB_EXCEPT_CALIB,
	PROFILER_ID_CMD_DISPATCH_STR_EXCEPT_CALIB,
	//---CMD
	PROFILER_ID_CMD_GET_CMD_FROM_HOST,
	PROFILER_ID_CMD_START_STREAM,
	PROFILER_ID_CMD_ENABLE_CALIB,
	PROFILER_ID_CMD_SEND_BUFFER,
	PROFILER_ID_CMD_SET_SENSOR_CALIBDB,
	PROFILER_ID_CMD_SET_OUT_PROP,
	PROFILER_ID_CMD_ENABLE_CHANNEL,
	//---RESP
	PROFILER_ID_RESP_SEND_CMD_DONE_RESP,
	PROFILER_ID_RESP_SEND_NON_PACKAGE_RESP,
	PROFILER_ID_RESP_SEND_PACKAGE_RESP,
	//---INTR
	PROFILER_ID_INTR_ISP_KERNEL_MI,
	PROFILER_ID_INTR_RING,
	PROFILER_ID_INTR_PREP_STNR,
	PROFILER_ID_INTR_MIPI_PIPE0,
	PROFILER_ID_INTR_MIPI_PIPE1,
	PROFILER_ID_INTR_I2C,
	PROFILER_ID_INTR_FLASH0,
	PROFILER_ID_INTR_FLASH1,

	PROFILER_ID_ONE_LOOPBACK_FRAME_PROCESS,

	//---HEART BEAT
	PROFILER_ID_HEART_BEAT_SCHEDULE,
	//---HARDWARE
	PROFILER_ID_HW_CORE_PROCESS_ONE_FRAME,
	PROFILER_ID_MAX
} ProfilerId_t;



typedef enum _CcpuBar_t {
	CCPU_BAR_INVALID = -1,
	CCPU_BAR_CACHE_0,
	CCPU_BAR_CACHE_1,
	CCPU_BAR_CACHE_2,
	CCPU_BAR_CACHE_3,
	CCPU_BAR_CACHE_4,
	CCPU_BAR_CACHE_5,
	CCPU_BAR_CACHE_6,
	CCPU_BAR_CACHE_7,
	CCPU_BAR_CACHE_8,
	CCPU_BAR_CACHE_9,
	CCPU_BAR_CACHE_A,
	CCPU_BAR_CACHE_B,
	CCPU_BAR_CACHE_C,
	CCPU_BAR_CACHE_D,
	CCPU_BAR_CACHE_E,
	CCPU_BAR_CACHE_F,
	CCPU_BAR_NCACHE_00,
	CCPU_BAR_NCACHE_01,
	CCPU_BAR_NCACHE_02,
	CCPU_BAR_NCACHE_03,
	CCPU_BAR_NCACHE_04,
	CCPU_BAR_NCACHE_05,
	CCPU_BAR_NCACHE_06,
	CCPU_BAR_NCACHE_07,
	CCPU_BAR_NCACHE_10,
	CCPU_BAR_NCACHE_11,
	CCPU_BAR_NCACHE_12,
	CCPU_BAR_NCACHE_13,
	CCPU_BAR_NCACHE_14,
	CCPU_BAR_NCACHE_15,
	CCPU_BAR_NCACHE_16,
	CCPU_BAR_NCACHE_17,
	CCPU_BAR_MAX
} CcpuBar_t;


typedef enum _CcpuReqType_t {
	CCPU_REQ_TYPE_INVALID = -1,
	CCPU_REQ_TYPE_RD,
	CCPU_REQ_TYPE_WR,
	CCPU_REQ_TYPE_MAX
} CcpuReqType_t;



typedef enum _PfmClient_t {
	PFM_CLIENT_INVALID = -1,
	PFM_CLIENT_CCPU,
	PFM_CLIENT_MP,
	PFM_CLIENT_VP,
	PFM_CLIENT_PP,
	PFM_CLIENT_MIPI0,
	PFM_CLIENT_MIPI1,
	PFM_CLIENT_RD,
	PFM_CLIENT_MAX
} PfmClient_t;


typedef enum _PfmRegion_t {
	PFM_REGION_INVALID = -1,
	PFM_REGION_0,
	PFM_REGION_1,
	PFM_REGION_2,
	PFM_REGION_3,
	PFM_REGION_4,
	PFM_REGION_5,
	PFM_REGION_6,
	PFM_REGION_7,
	PFM_REGION_MAX
} PfmRegion_t;


typedef struct _ProfilerResult_t {
	//Function cycles profiling
	uint32 totalCountHi[PROFILER_ID_MAX];
	uint32 totalCountLo[PROFILER_ID_MAX];
	uint32 minCountHi[PROFILER_ID_MAX];
	uint32 minCountLo[PROFILER_ID_MAX];
	uint32 maxCountHi[PROFILER_ID_MAX];
	uint32 maxCountLo[PROFILER_ID_MAX];
	uint32 accessTimes[PROFILER_ID_MAX];

	//Ccpu Bar profiling
	uint32
	ccpuBarTotalCountHi[CCPU_BAR_MAX][CCPU_REQ_TYPE_MAX][PROFILER_ID_MAX];
	uint32
	ccpuBarTotalCountLo[CCPU_BAR_MAX][CCPU_REQ_TYPE_MAX][PROFILER_ID_MAX];
	uint32
	ccpuBarAccessTimes[CCPU_BAR_MAX][CCPU_REQ_TYPE_MAX][PROFILER_ID_MAX];
	uint32
	ccpuBarMinCount[CCPU_BAR_MAX][CCPU_REQ_TYPE_MAX][PROFILER_ID_MAX];
	uint32
	ccpuBarMaxCount[CCPU_BAR_MAX][CCPU_REQ_TYPE_MAX][PROFILER_ID_MAX];

	//PFM profiling
	uint32 pfmRegion[PFM_CLIENT_MAX][PFM_REGION_MAX];
	uint32 pfmCnt[PFM_CLIENT_MAX][PFM_REGION_MAX];
} ProfilerResult_t;

typedef struct _ProfilerIdResult_t {
	//Function cycles profiling
	uint32 accessTimes;
	//Ccpu Bar profiling //3(code, stack and heap)
	uint32 ccpuBarTotal[3][CCPU_REQ_TYPE_MAX];
} ProfilerIdResult_t;


typedef enum _ErrorLevel_t {
	ERROR_LEVEL_INVALID,
	ERROR_LEVEL_FATAL,/*The error has caused the stream stopped */
	ERROR_LEVEL_WARN, /*Firmware has automatically restarted the stream */
	ERROR_LEVEL_NOTICE, /*Should take notice of this error which may lead
			     *some other error
			     */
	ERROR_LEVEL_MAX
} ErrorLevel_t;


#define ERROR_CODE1_MIPI_ERR_CS				(1 << 0)
#define ERROR_CODE1_MIPI_ERR_ECC1			(1 << 1)
#define ERROR_CODE1_MIPI_ERR_ECC2			(1 << 2)
#define ERROR_CODE1_MIPI_ERR_PROTOCOL			(1 << 3)
#define ERROR_CODE1_MIPI_ERR_CONTROL			(1 << 4)
#define ERROR_CODE1_MIPI_ERR_EOT_SYNC			(1 << 5)
#define ERROR_CODE1_MIPI_ERR_SOT_SYNC			(1 << 6)
#define ERROR_CODE1_MIPI_ERR_SOT			(1 << 7)
#define ERROR_CODE1_MIPI_SYNC_FIFO_OVFLW		(1 << 8)
#define ERROR_CODE1_MIPI_FORM0_INFORM_SIZE_ERR		(1 << 9)
#define ERROR_CODE1_MIPI_FORM0_OUTFORM_SIZE_ERR		(1 << 10)
#define ERROR_CODE1_MIPI_FORM1_INFORM_SIZE_ERR		(1 << 11)
#define ERROR_CODE1_MIPI_FORM1_OUTFORM_SIZE_ERR		(1 << 12)
#define ERROR_CODE1_MIPI_FORM2_INFORM_SIZE_ERR		(1 << 13)
#define ERROR_CODE1_MIPI_FORM2_OUTFORM_SIZE_ERR		(1 << 14)
#define ERROR_CODE1_MIPI_WRAP_Y				(1 << 15)
#define ERROR_CODE1_MIPI_WRAP_CB			(1 << 16)
#define ERROR_CODE1_MIPI_WRAP_CR			(1 << 17)
#define ERROR_CODE1_MIPI_MAF				(1 << 18)
#define ERROR_CODE1_MIPI_AXI_TMOUT			(1 << 19)


#define ERROR_CODE2_ISP_AFM_LUM_OF			(1 << 0)
#define ERROR_CODE2_ISP_AFM_SUM_OF			(1 << 1)
#define ERROR_CODE2_ISP_PIC_SIZE_ERR			(1 << 2)
#define ERROR_CODE2_ISP_DATA_LOSS			(1 << 3)
#define ERROR_CODE2_MI_MP_WRAP_Y			(1 << 4)
#define ERROR_CODE2_MI_MP_WRAP_CB			(1 << 5)
#define ERROR_CODE2_MI_MP_WRAP_CR			(1 << 6)
#define ERROR_CODE2_MI_MP_MAF				(1 << 7)
#define ERROR_CODE2_MI_MP_AXI_TMOUT			(1 << 8)
#define ERROR_CODE2_MI_VP_WRAP_Y			(1 << 9)
#define ERROR_CODE2_MI_VP_WRAP_CB			(1 << 10)
#define ERROR_CODE2_MI_VP_WRAP_CR			(1 << 11)
#define ERROR_CODE2_MI_VP_MAF				(1 << 12)
#define ERROR_CODE2_MI_VP_AXI_TMOUT			(1 << 13)
#define ERROR_CODE2_MI_PP_WRAP_Y			(1 << 14)
#define ERROR_CODE2_MI_PP_WRAP_CB			(1 << 15)
#define ERROR_CODE2_MI_PP_WRAP_CR			(1 << 16)
#define ERROR_CODE2_MI_PP_MAF				(1 << 17)
#define ERROR_CODE2_MI_PP_AXI_TMOUT			(1 << 18)
#define ERROR_CODE2_MI_RD_MAF				(1 << 19)
#define ERROR_CODE2_MI_RD_AXI_TMOUT			(1 << 20)

#define ERROR_CODE3_PREP_INFORM_SIZE_ERR		(1 << 0)
#define ERROR_CODE3_PREP_OUTFORM_SIZE_ERR		(1 << 1)
#define ERROR_CODE3_STNR_TNR_HW_SETA			(1 << 2)
#define ERROR_CODE3_STNR_TNR_AUTO_REC			(1 << 3)
#define ERROR_CODE3_STNR_RDMA_RNACK1_ERR		(1 << 4)
#define ERROR_CODE3_STNR_RDMA_RNACK2_ERR		(1 << 5)
#define ERROR_CODE3_STNR_RDMA_RNACK3_ERR		(1 << 6)
#define ERROR_CODE3_STNR_RDMA_RRESP_ERR			(1 << 7)
#define ERROR_CODE3_STNR_WDMA_RNACK1_ERR		(1 << 8)
#define ERROR_CODE3_STNR_WDMA_RNACK2_ERR		(1 << 9)
#define ERROR_CODE3_STNR_WDMA_RNACK3_ERR		(1 << 10)
#define ERROR_CODE3_STNR_WDMA_RRESP_ERR			(1 << 11)

#define ERROR_CODE4_AE_CALC_FAIL			(1 << 0)
#define ERROR_CODE4_AF_CALC_FAIL			(1 << 1)
#define ERROR_CODE4_AWB_CALC_FAIL			(1 << 2)
#define ERROR_CODE4_RT_FRAME_TMOUT			(1 << 3)
#define ERROR_CODE4_CORE_FRAME_TMOUT			(1 << 4)
#define ERROR_CODE4_RT_PROCESS_FRAME_FAIL		(1 << 5)

/**
 * @id ERROR_CODE5_I2C_7B_ADDR_NOACK
 * Master is in 7-bit address mode and the address sentwas not
 * acknowledged by any slave
 */
#define ERROR_CODE5_I2C_7B_ADDR_NOACK			(1 << 0)
/**
 * @id ERROR_CODE5_I2C_10ADDR1_NOACK
 * Master is in 10-bit address mode and the first 10-bit address
 * byte was not acknowledged by any slave
 */
#define ERROR_CODE5_I2C_10ADDR1_NOACK			(1 << 1)
/**
 * @id ERROR_CODE5_I2C_10ADDR2_NOACK
 * Master is in 10-bit address mode and the second address byte of
 * the 10-bit address was not acknowledged by any slave
 */
#define ERROR_CODE5_I2C_10ADDR2_NOACK			(1 << 2)
/**
 * @id ERROR_CODE5_I2C_TXDATA_NOACK
 * Master has received an acknowledgment for the address, but when
 * it set data bytes(s) following the address, it did not receive an
 * acknowledge from the remote slave(s).
 */
#define ERROR_CODE5_I2C_TXDATA_NOACK			(1 << 3)
/**
 * @id ERROR_CODE5_I2C_GCALL_NOACK
 * Send a General Call and no slave on the bus acknowledged the General
 * Call.
 */
#define ERROR_CODE5_I2C_GCALL_NOACK			(1 << 4)
/**
 * @id  ERROR_CODE5_I2C_GCALL_READ
 * Send a General Call but the user programmed the byte following the
 * General Call to be a read from the bus (IC_DATA_CMD[9] is set to 1).
 */
#define ERROR_CODE5_I2C_GCALL_READ			(1 << 5)
/**
 * @id ERROR_CODE5_I2C_HS_ACKDET
 * Master is in High Speed mode and the High Speed Master code was
 * acknowledged (wrong behavior).
 */
#define ERROR_CODE5_I2C_HS_ACKDET			(1 << 6)
/**
 * @id ERROR_CODE5_I2C_SBYTE_ACKDET
 * Master has sent a START Byte and the START Byte was acknowledged
 * (wrong behavior).
 */
#define ERROR_CODE5_I2C_SBYTE_ACKDET			(1 << 7)
/**
 * @id ERROR_CODE5_I2C_HS_NORSTRT
 * The restart is disabled (IC_RESTART_EN bit (IC_CON[5]) = 0) and the
 * user is trying to use the master to transfer data in High Speed mode.
 */
#define ERROR_CODE5_I2C_HS_NORSTRT			(1 << 8)
/**
 * @id ERROR_CODE5_I2C_SBYTE_NORSTRT
 * The restart is disabled (IC_RESTART_EN) bit (IC_CON[5]) = 0) and the
 * user is trying to send a START Byte.
 */
#define ERROR_CODE5_I2C_SBYTE_NORSTRT			(1 << 9)
/**
 * @id ERROR_CODE5_I2C_10B_RD_NORSTRT
 * The restart is disabled (IC_RESTART_EN bit (IC_CON[5] = 0) and the master
 * sends a read command in 10-bit addressing mode.
 */
#define ERROR_CODE5_I2C_10B_RD_NORSTRT			(1 << 10)
/**
 * @id ERROR_CODE5_I2C_MASTER_DIS
 * User tries to initiate a Master operation with the Master mode disabled.
 */
#define ERROR_CODE5_I2C_MASTER_DIS			(1 << 11)
/**
 * @id ERROR_CODE5_I2C_ARB_LOST
 * Master has lost arbitration, or if IC_TX_ABRT_SOURCE[14] is also set, then
 * the slave transmitter has lost arbitration.
 * Note: I2C can be both master and slave at the same time.
 */
#define ERROR_CODE5_I2C_ARB_LOST			(1 << 12)
/**
 * @id ERROR_CODE5_I2C_SLVFLUSH_TXFIFO
 * Slave has received a read command and some data exists in the TX FIFO so the
 * slave issue a TX_ABRT interrupt to flush old data in TX FIFO.
 */
#define ERROR_CODE5_I2C_SLVFLUSH_TXFIFO			(1 << 13)
/**
 * @id ERROR_CODE5_I2C_SLV_ARBLOST
 * Slave lost the bus while transmitting data to a remote master.
 * IC_TX_ABRT_SOURCE[12] is set at the same time.
 * Note: Even though the slave never "owns" the bus, something could go wrong on
 * the bus. This is a fail safe check. For instance, during a data transmissioin
 * at the low-to-high transition of SCL, if what is on the data bus is not what
 * is supposed to be transmitted, then I2C controller no longer own the bus.
 */
#define ERROR_CODE5_I2C_SLV_ARBLOST			(1 << 14)
/**
 *@id ERROR_CODE5_I2C_SLVRD_INTX
 *When the processor side responds to a slave mode request for data to be
 *transmitted to a remote master and user writes a 1 in CMD(bit8) of
 *IC_DATA_CMD register.
 */
#define ERROR_CODE5_I2C_SLVRD_INTX			(1 << 15)

/**
 * @id ERROR_CODE5_SVM_INVALID_INSTR
 * An invalid instruction exists in the svm script.
 */
#define ERROR_CODE5_SVM_INVALID_INSTR			(1 << 16)



typedef struct _ErrorCode_t {
	uint32 code1;/*See ERROR_CODE1_XXX for reference */
	uint32 code2;/*See ERROR_CODE2_XXX for reference */
	uint32 code3;/*See ERROR_CODE3_XXX for reference */
	uint32 code4;/*See ERROR_CODE4_XXX for reference */
	uint32 code5;/*See ERROR_CODE5_XXX for reference */
} ErrorCode_t;



typedef enum _MipiVirtualChannel_t {
	MIPI_VIRTUAL_CHANNEL_0 = 0x0, //!< virtual channel 0
	MIPI_VIRTUAL_CHANNEL_1 = 0x1, //!< virtual channel 1
	MIPI_VIRTUAL_CHANNEL_2 = 0x2, //!< virtual channel 2
	MIPI_VIRTUAL_CHANNEL_3 = 0x3, //!< virtual channel 3
	MIPI_VIRTUAL_CHANNEL_MAX
} MipiVirtualChannel_t;



typedef enum _MipiDataType_t {
	MIPI_DATA_TYPE_FSC = 0x00, //!< frame start code
	MIPI_DATA_TYPE_FEC = 0x01, //!< frame end code
	MIPI_DATA_TYPE_LSC = 0x02, //!< line start code
	MIPI_DATA_TYPE_LEC = 0x03, //!< line end code

	// 0x04 .. 0x07 reserved

	MIPI_DATA_TYPE_GSPC1 = 0x08, //!< generic short packet code 1
	MIPI_DATA_TYPE_GSPC2 = 0x09, //!< generic short packet code 2
	MIPI_DATA_TYPE_GSPC3 = 0x0A, //!< generic short packet code 3
	MIPI_DATA_TYPE_GSPC4 = 0x0B, //!< generic short packet code 4
	MIPI_DATA_TYPE_GSPC5 = 0x0C, //!< generic short packet code 5
	MIPI_DATA_TYPE_GSPC6 = 0x0D, //!< generic short packet code 6
	MIPI_DATA_TYPE_GSPC7 = 0x0E, //!< generic short packet code 7
	MIPI_DATA_TYPE_GSPC8 = 0x0F, //!< generic short packet code 8


	MIPI_DATA_TYPE_NULL = 0x10, //!< null
	MIPI_DATA_TYPE_BLANKING = 0x11, //!< blanking data
	MIPI_DATA_TYPE_EMBEDDED = 0x12, //!< embedded 8-bit non image data

	// 0x13 .. 0x17 reserved

	MIPI_DATA_TYPE_YUV420_8 = 0x18, //!< YUV 420 8-Bit
	MIPI_DATA_TYPE_YUV420_10 = 0x19, //!< YUV 420 10-Bit
	MIPI_DATA_TYPE_LEGACY_YUV420_8 = 0x1A, //!< YUV 420 8-Bit
	// 0x1B reserved
	//!< YUV 420 8-Bit (chroma shifted pixel sampling)
	MIPI_DATA_TYPE_YUV420_8_CSPS = 0x1C,
	//!< YUV 420 10-Bit (chroma shifted pixel sampling)
	MIPI_DATA_TYPE_YUV420_10_CSPS = 0x1D,
	MIPI_DATA_TYPE_YUV422_8 = 0x1E, //!< YUV 422 8-Bit
	MIPI_DATA_TYPE_YUV422_10 = 0x1F, //!< YUV 422 10-Bit

	MIPI_DATA_TYPE_RGB444 = 0x20, //!< RGB444
	MIPI_DATA_TYPE_RGB555 = 0x21, //!< RGB555
	MIPI_DATA_TYPE_RGB565 = 0x22, //!< RGB565
	MIPI_DATA_TYPE_RGB666 = 0x23, //!< RGB666
	MIPI_DATA_TYPE_RGB888 = 0x24, //!< RGB888

	// 0x25 .. 0x27 reserved

	MIPI_DATA_TYPE_RAW_6 = 0x28, //!< RAW6
	MIPI_DATA_TYPE_RAW_7 = 0x29, //!< RAW7
	MIPI_DATA_TYPE_RAW_8 = 0x2A, //!< RAW8
	MIPI_DATA_TYPE_RAW_10 = 0x2B, //!< RAW10
	MIPI_DATA_TYPE_RAW_12 = 0x2C, //!< RAW12
	MIPI_DATA_TYPE_RAW_14 = 0x2D, //!< RAW14

	// 0x2E .. 0x2F reserved

	MIPI_DATA_TYPE_USER_1 = 0x30, //!< user defined 1
	MIPI_DATA_TYPE_USER_2 = 0x31, //!< user defined 2
	MIPI_DATA_TYPE_USER_3 = 0x32, //!< user defined 3
	MIPI_DATA_TYPE_USER_4 = 0x33, //!< user defined 4
	MIPI_DATA_TYPE_USER_5 = 0x34, //!< user defined 5
	MIPI_DATA_TYPE_USER_6 = 0x35, //!< user defined 6
	MIPI_DATA_TYPE_USER_7 = 0x36, //!< user defined 7
	MIPI_DATA_TYPE_USER_8 = 0x37, //!< user defined 8
	MIPI_DATA_TYPE_MAX
} MipiDataType_t;


typedef enum _MipiCompScheme_t {
	MIPI_COMP_SCHEME_NONE = 0, //!< NONE
	MIPI_COMP_SCHEME_12_8_12 = 1, //!< 12_8_12
	MIPI_COMP_SCHEME_12_7_12 = 2, //!< 12_7_12
	MIPI_COMP_SCHEME_12_6_12 = 3, //!< 12_6_12
	MIPI_COMP_SCHEME_10_8_10 = 4, //!< 10_8_10
	MIPI_COMP_SCHEME_10_7_10 = 5, //!< 10_7_10
	MIPI_COMP_SCHEME_10_6_10 = 6, //!< 10_6_10
	MIPI_COMP_SCHEME_MAX
} MipiCompScheme_t;


typedef enum _MipiPredBlock_t {
	MIPI_PRED_BLOCK_INVALID = 0, //!< invalid
	MIPI_PRED_BLOCK_1 = 1, //!< Predictor1 (simple algorithm)
	MIPI_PRED_BLOCK_2 = 2, //!< Predictor2 (more complex algorithm)
	MIPI_PRED_BLOCK_MAX
} MipiPredBlock_t;


//Color filter array pattern
typedef enum _CFAPattern_t {
	CFA_PATTERN_INVALID = 0,
	CFA_PATTERN_RGGB = 1,
	CFA_PATTERN_GRBG = 2,
	CFA_PATTERN_GBRG = 3,
	CFA_PATTERN_BGGR = 4,
	CFA_PATTERN_PURE_IR = 5,
	CFA_PATTERN_RIGB = 6,
	CFA_PATTERN_RGIB = 7,
	CFA_PATTERN_IRBG = 8,
	CFA_PATTERN_GRBI = 9,
	CFA_PATTERN_IBRG = 10,
	CFA_PATTERN_GBRI = 11,
	CFA_PATTERN_BIGR = 12,
	CFA_PATTERN_BGIR = 13,
	CFA_PATTERN_BGRGGIGI = 14,
	CFA_PATTERN_RGBGGIGI = 15,
	CFA_PATTERN_MAX
} CFAPattern_t;


//Deprecated, use CFAPattern_t instead
typedef enum _BayerPattern_t {
	BAYER_PATTERN_INVALID = 0,
	BAYER_PATTERN_RGGB = CFA_PATTERN_RGGB,
	BAYER_PATTERN_GRBG = CFA_PATTERN_GRBG,
	BAYER_PATTERN_GBRG = CFA_PATTERN_GBRG,
	BAYER_PATTERN_BGGR = CFA_PATTERN_BGGR,
	BAYER_PATTERN_MAX
} BayerPattern_t;


typedef struct _MipiIntfProp_t {
	uint8 numLanes;
	MipiVirtualChannel_t virtChannel;
	MipiDataType_t dataType;
	MipiCompScheme_t compScheme;
	MipiPredBlock_t predBlock;
} MipiIntfProp_t;


typedef enum _ParallelDataType_t {
	PARALLEL_DATA_TYPE_INVALID = 0,
	PARALLEL_DATA_TYPE_RAW8 = 1,
	PARALLEL_DATA_TYPE_RAW10 = 2,
	PARALLEL_DATA_TYPE_RAW12 = 3,
	PARALLEL_DATA_TYPE_YUV420_8BIT = 4,
	PARALLEL_DATA_TYPE_YUV420_10BIT = 5,
	PARALLEL_DATA_TYPE_YUV422_8BIT = 6,
	PARALLEL_DATA_TYPE_YUV422_10BIT = 7,
	PARALLEL_DATA_TYPE_MAX
} ParallelDataType_t;


typedef enum _Polarity_t {
	POLARITY_INVALID = 0,
	POLARITY_HIGH = 1,
	POLARITY_LOW = 2,
	POLARITY_MAX
} Polarity_t;


typedef enum _SampleEdge_t {
	SAMPLE_EDGE_INVALID = 0,
	SAMPLE_EDGE_NEG = 1,
	SAMPLE_EDGE_POS = 2,
	SAMPLE_EDGE_MAX
} SampleEdge_t;


typedef enum _YCSequence_t {
	YCSEQUENCE_INVALID = 0,
	YCSEQUENCE_YCBYCR = 1,
	YCSEQUENCE_YCRYCB = 2,
	YCSEQUENCE_CBYCRY = 3,
	YCSEQUENCE_CRYCBY = 4,
	YCSEQUENCE_MAX
} YCSequence_t;


typedef enum _ColorSubSampling_t {
	COLOR_SUB_SAMPLING_INVALID = 0,
	COLOR_SUB_SAMPLING_CONV422_COSITED = 1,
	COLOR_SUB_SAMPLING_CONV422_INTER = 2,
	COLOR_SUB_SAMPLING_CONV422_NOCOSITED = 3,
	COLOR_SUB_SAMPLING_MAX
} ColorSubSampling_t;


typedef enum _SensorType_t {
	SENSOR_TYPE_INVALID = 0,
	SENSOR_TYPE_RGBBAYER = 1,
	SENSOR_TYPE_IR = 2,
	SENSOR_TYPE_RGBIR = 3,
	SENSOR_TYPE_MAX
} SensorType_t;

typedef struct _ParallelIntfProp_t {
	ParallelDataType_t dataType;
	Polarity_t hPol;
	Polarity_t vPol;
	SampleEdge_t edge;
	YCSequence_t ycSeq;
	ColorSubSampling_t subSampling;
} ParallelIntfProp_t;


typedef enum _RDmaDataType_t {
	RDMA_DATA_TYPE_INVALID = 0,
	RDMA_DATA_TYPE_RAW8 = 1,
	RDMA_DATA_TYPE_RAW10 = 2,
	RDMA_DATA_TYPE_RAW12 = 3,
	RDMA_DATA_TYPE_MAX
} RDmaDataType_t;


typedef struct _RDmaIntfProp_t {
	RDmaDataType_t dataType;
} RDmaIntfProp_t;


typedef enum _SensorShutterType_t {
	SENSOR_SHUTTER_TYPE_GLOBAL,
	SENSOR_SHUTTER_TYPE_ROLLING,
	SENSOR_SHUTTER_TYPE_MAX,
} SensorShutterType_t;

/**
 *@desp: The IrCoord_t is used to specify the IR extract
 *coordinate in STREAM_MODE_3. IR extract will extract one
 *pixel from 4 pixels. See the following diagram for the
 *reference coordinate.
 *
 *   +----+----+
 *   | 00 | 01 |
 *   +----+----+
 *   | 10 | 11 |
 *   +----|----+
 */
typedef enum _IrCoord_t {
	IR_COORD_INVALID = -1,
	IR_COORD_00 = 0x0,
	IR_COORD_01 = 0x1,
	IR_COORD_10 = 0x2,
	IR_COORD_11 = 0x3,
	IR_COORD_MAX
} IrCoord_t;

typedef struct _SensorProp_t {
	SensorIntfType_t intfType;
	union {
		MipiIntfProp_t mipi;
		ParallelIntfProp_t parallel;
		RDmaIntfProp_t rdma;
	} intfProp;
	CFAPattern_t cfaPattern;
	SensorShutterType_t sensorShutterType;
	bool_t hasEmbeddedData;
	uint32 itimeDelayFrames;
	uint32 gainDelayFrames;
} SensorProp_t;


typedef enum _HdrItimeType_t {
	HDR_ITIME_TYPE_INVALID,
	//!< The hdr itime for high exposure and
	//low exposure has separated control registers
	HDR_ITIME_TYPE_SEPERATE,
	//!< The hdr itime for low exposure is a ratio of the high exposure
	HDR_ITIME_TYPE_RATIO,
	//!< The hdr itime for high and low exposure should be euqal.
	HDR_ITIME_TYPE_EQUAL,
	HDR_ITIME_TYPE_MAX
} HdrItimeType_t;


typedef enum _HdrAGainType_t {
	HDR_AGAIN_TYPE_INVALID,
	//!< The hdr analog gain for high exposure and
	//low exposure has separated control registers
	HDR_AGAIN_TYPE_SEPERATE,
	//!< The hdr analog gain for low exposure is a ratio of the high
	//exposure
	HDR_AGAIN_TYPE_RATIO,
	//!< The hdr analog gain for high and low exposure should be euqal.
	HDR_AGAIN_TYPE_EQUAL,
	HDR_AGAIN_TYPE_MAX
} HdrAGainType_t;


typedef enum _HdrDGainType_t {
	HDR_DGAIN_TYPE_INVALID,
	//!< The hdr digital gain for high exposure and low exposure has
	//separated control registers
	HDR_DGAIN_TYPE_SEPERATE,
	//!< The hdr digital gain for low exposure is a ratio of the high
	//exposure
	HDR_DGAIN_TYPE_RATIO,
	//!< The hdr digital gain for high and low exposure should be euqal.
	HDR_DGAIN_TYPE_EQUAL,
	HDR_DGAIN_TYPE_MAX
} HdrDGainType_t;

typedef enum _HdrStatDataMatrixId_t {
	HDR_STAT_DATA_MATRIX_ID_INVALID = 0,
	HDR_STAT_DATA_MATRIX_ID_16X16 = 1,
	HDR_STAT_DATA_MATRIX_ID_8X8 = 2,
	HDR_STAT_DATA_MATRIX_ID_4X4 = 3,
	HDR_STAT_DATA_MATRIX_ID_1X1 = 4,
	HDR_STAT_DATA_MATRIX_ID_MAX,
} HdrStatDataMatrixId_t;

#define MAX_HDR_RATIO_NUM (16)
typedef struct _SensorHdrProp_t {
	MipiVirtualChannel_t virtChannel;
	MipiDataType_t dataType;
	HdrStatDataMatrixId_t matrixId;
	Window_t hdrStatWindow;
	HdrItimeType_t itimeType;
	uint8 itimeRatioList[MAX_HDR_RATIO_NUM];
	HdrAGainType_t aGainType;
	uint8 aGainRatioList[MAX_HDR_RATIO_NUM];
	HdrDGainType_t dGainType;
	uint8 dGainRatioList[MAX_HDR_RATIO_NUM];
} SensorHdrProp_t;

typedef struct _SensorEmbProp_t {
	MipiVirtualChannel_t virtChannel;
	MipiDataType_t dataType;
	Window_t embDataWindow;
	uint32 expoStartByteOffset;
	uint32 expoNeededBytes;
} SensorEmbProp_t;

typedef enum _instr_op_t {
	/*#define */INSTR_OP_INVALID = (0x0),
	/*#define */INSTR_OP_ADD = (0x1),
	/*#define */INSTR_OP_SUB = (0x2),
	/*#define */INSTR_OP_MUL = (0x3),
	/*#define */INSTR_OP_DIV = (0x4),
	/*#define */INSTR_OP_SHL = (0x5),
	/*#define */INSTR_OP_SHR = (0x6),
	/*#define */INSTR_OP_AND = (0x7),
	/*#define */INSTR_OP_OR = (0x8),
	/*#define */INSTR_OP_XOR = (0x9),
	/*#define */INSTR_OP_MOVB = (0xa),
	/*#define */INSTR_OP_MOVW = (0xb),
	/*#define */INSTR_OP_MOVDW = (0xc),
	/*#define */INSTR_OP_JNZ = (0xd),
	/*#define */INSTR_OP_JEZ = (0xe),
	/*#define */INSTR_OP_JLZ = (0xf),
	/*#define */INSTR_OP_JGZ = (0x10),
	/*#define */INSTR_OP_JGEZ = (0x11),
	/*#define */INSTR_OP_JLEZ = (0x12),
	/*#define */INSTR_OP_CALL = (0x13),
	/*#define */INSTR_OP_WI2C = (0x14),
	/*#define */INSTR_OP_RI2C = (0x15),
	/*#define */INSTR_OP_PRINT = (0x16)
} instr_op_t;

typedef enum _instr_arg_type_t {
	/*#define */INSTR_ARG_TYPE_INVALID = (0x0),
	/*#define */INSTR_ARG_TYPE_REG = (0x1),
	/*#define */INSTR_ARG_TYPE_MEM = (0x2),
	/*#define */INSTR_ARG_TYPE_IMM = (0x3),
	/*#define */INSTR_ARG_TYPE_LABEL = (0x4),
	/*#define */INSTR_ARG_TYPE_FUNC = (0x5),
	/*#define */INSTR_ARG_TYPE_MAX = (0x6)
} instr_arg_type_t;

#define INSTR_MEM_OFFSET_TYPE_REG (0x0)
#define INSTR_MEM_OFFSET_TYPE_IMM (0x1)


//================================
//JUMP Instruction
//List:
// INSTR_OP_JNZ
// INSTR_OP_JEZ
// INSTR_OP_JLZ
// INSTR_OP_JGZ
// INSTR_OP_JGEZ
// INSTR_OP_JLEZ
//Syntax:
//JNZ Rn, @labelName
//JNZ $imm, @labelName
//================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: arg1 type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit0]:instruction index
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reg index(0~31) or immediate value according to the
//[bit26~bit25] of instrPart1


//=======================
//CALL Instruction
//List:
//INSTR_OP_CALL
//Syntax:
//CALL @funcName
//=======================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: reserved(0)
//[bit24~bit0]:function index (range 0~MAX_SCRIPT_FUNCS-1)
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reserved(0)

//=============================================
//ADD/SUB/MUL/DIV/AND/OR/XOR Instruction
//List:
//INSTR_OP_ADD
//INSTR_OP_SUB
//INSTR_OP_MUL
//INSTR_OP_DIV
//INSTR_OP_SHL
//INSTR_OP_SHR
//INSTR_OP_AND
//INSTR_OP_OR
//INSTR_OP_XOR
//Syntax:
//ADD Rd, Rs, Rp
//ADD Rd, Rs, $imm
//=============================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit22]: Rd register index
//[bit21~bit17]: Rs register index
//[bit16~bit15]: arg3 type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit14~bit0]:reserved(0)
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reg index(0~31) or immediate value according to the
//[bit16~bit15] of instrPart1


//=========================================
//MOVB/MOVW/MOVDW Instruction
//List:
//INSTR_OP_MOVB
//INSTR_OP_MOVW
//INSTR_OP_MOVDW
//Syntax:
//MOVB dst, src
//=========================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: dst arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM)
//[bit24~bit7]:
//  (1): if dst arg type is INISTR_ARG_TYPE_REG,
//	[bit24~bit20] defines the register index{0~31}
//	[bit19~bit7] reserved
//  (2): if dst arg type is INSTR_ARG_MEM,
//	[bit24] is mem{0~1} index,
//	[bit23] is dst mem offset type (INSTR_MEM_OFFSET_TYPE_REG or
//	INSTR_MEM_OFFSET_TYPE_IMM)
//[bit22~bit7]:
//  (1) if dst mem offset type is INSTR_MEM_OFFSET_TYPE_REG
//	[bit22~bit18] is register index{0~31}
//	[bit17~bit7]reserved
//  (2) if dst mem offset type is INSTR_MEM_OFFSET_TYPE_IMM
//	[bit22~bit7] is the 16 bit unsigned imm value
//[bit6~bit5]:
//  src arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM or INSTR_ARG_TYPE_IMM)
//[bit4~bit0]:
//  (1) if src arg type is INSTR_ARG_TYPE_REG,
//	[bit4~bit0] defines the register index{0~31}
//  (2) if src arg type is INSTR_ARG_MEM,
//	[bit4] is mem{0~1} index
//	[bit3] is src mem offset type
//		(INSTR_MEM_OFFSET_TYPE_REG or INSTR_MEM_OFFSET_TYPE_IMM)
//	[bit2~bit0] reserved
//  (3) if src arg type is INSTR_ARG_TYPE_IMM
//	[bit4~bit0] reserved
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]:
//  (1) if src arg type is INSTR_ARG_TYPE_IMM
//	[bit31~bit0] defines a unsigned 32 bit immedage value
//  (2) if src arg type is INSTR_ARG_TYPE_MEM and the mem offset type is
//	INSTR_MEM_OFFSET_TYPE_REG
//	[bit31~bit27] is the register index{0~31}
//	[bit26~bit0] reserved
//  (3) if src arg type is INSTR_ARG_TYPE_MEM and the mem offset type is
//	INSTR_MEM_OFFSET_TYPE_IMM
//[bit31~bit16] is the 16bit unsigned immediate value
//[bit15~bit0} reserved


//=========================================
//WI2C Instruction
//List: INSTR_OP_WI2C
//Syntax:
//WI2C i2cRegIndex, value, length
//=========================================
//----------------
//instrPart1:
//----------------
//[bit31~bit27]: op
//[bit26~bit25]: i2cRegIndex arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit23]: value arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit22~bit21]: length arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit20~bit16]: register index(0~31) for length if its type is
//	INSTR_ARG_TYPE_REG, or an immediate value for length(range 0~4).
//[bit15~bit0]:
//  register index(0~31) for i2cRegIndex if its type is INSTR_ARG_TYPE_REG or
//  a 16bit immediate value for i2cRegIndex if its type is INSTR_ARG_TYPE_IMM
//  according to [bit26~bit25] in instrPart1
//---------------
//instrPart2:
//---------------
//[bit31~bit0]:
//  immediate value for value arg if its type is INSTR_ARG_TYPE_IMM or register
//  index(0~31) for value arg if its type is INSTR_ARG_TYPE_REG according to
//  [bit24~bit23] in instrPart1


//==========================================
//RI2C Instruction
//List: INSTR_OP_RI2C
//Syntax:
//RI2C Rd, i2cRegIndex, length
//==========================================
//----------------
//instrPart1:
//----------------
//[bit31~bit27]: op
//[bit26~bit25]: i2cRegIndex arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit23]: reserved
//[bit22~bit21]: length arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit20~bit16]: register index(0~31) for length if its type is
//  INSTR_ARG_TYPE_REG, or an immediate value for length(range 0~4).
//[bit15~bit0]:register index(0~31) for i2cRegIndex if its type is
//  INSTR_ARG_TYPE_REG or a 16bit immediate value for i2cRegIndex if its type
//  is INSTR_ARG_TYPE_IMM according to [bit26~bit25] in instrPart1
//---------------
//instrPart2:
//---------------
//[bit31~bit27]: Rd register index.
//[bit26~bit0]:reserved

#define MAX_SCRIPT_INSTRS		(500)
#define MAX_SCRIPT_FUNCS		(40)
#define MAX_SCRIPT_FUNC_NAME_LENGTH	(32)
#define MAX_SCRIPT_FUNC_ARGS		(10)

typedef struct _Instr_t {
	uint32 instrPart1;
	uint32 instrPart2;
} Instr_t;

typedef struct _FuncMap_t {
	uint8 funcName[MAX_SCRIPT_FUNC_NAME_LENGTH];
	//The start instructions idx associated to this function
	uint32 startIdx;
	uint32 endIdx;//The end instruction idx associated to this function
} FuncMap_t;

typedef struct _InstrSeq_t {
	Instr_t instrs[MAX_SCRIPT_INSTRS];
	FuncMap_t funcs[MAX_SCRIPT_FUNCS];

	uint32 instrNum;
	uint32 funcNum;
} InstrSeq_t;


typedef struct _ScriptFuncArg_t {
	int32 args[MAX_SCRIPT_FUNC_ARGS];
	int32 numArgs;
} ScriptFuncArg_t;


typedef enum _I2cDeviceAddrType_t {
	I2C_DEVICE_ADDR_TYPE_INVALID,
	I2C_DEVICE_ADDR_TYPE_7BIT,
	I2C_DEVICE_ADDR_TYPE_10BIT,
	I2C_DEVICE_ADDR_TYPE_MAX
} I2cDeviceAddrType_t;


typedef enum _I2cRegAddrType_t {
	I2C_REG_ADDR_TYPE_INVALID,
	I2C_REG_ADDR_TYPE_8BIT,
	I2C_REG_ADDR_TYPE_16BIT,
	I2C_REG_ADDR_TYPE_MAX
} I2cRegAddrType_t;


typedef struct _ScriptI2cArg_t {
	I2cDeviceId_t deviceId;
	//!< Whether enable the restart for the read command
	bool_t enRestart;
	I2cDeviceAddrType_t deviceAddrType;
	I2cRegAddrType_t regAddrType;
	uint16 deviceAddr; //!< 7bit or 10bit device address
} ScriptI2cArg_t;

typedef struct _DeviceScript_t {
	InstrSeq_t instrSeq;//!< The script instructions
	ScriptFuncArg_t argSensor;
	ScriptFuncArg_t argLens;
	ScriptFuncArg_t argFlash;

	ScriptI2cArg_t i2cSensor;
	ScriptI2cArg_t i2cLens;
	ScriptI2cArg_t i2cFlash;
} DeviceScript_t;


typedef enum _I2cMsgType_t {
	I2C_MSG_TYPE_INVALID,
	I2C_MSG_TYPE_READ,
	I2C_MSG_TYPE_WRITE,
	I2C_MSG_TYPE_MAX
} I2cMsgType_t;


typedef struct _I2cMsg_t {
	I2cMsgType_t msgType;
	bool_t enRestart;//!< Whether enable the restart for the read command
	I2cDeviceAddrType_t deviceAddrType;
	I2cRegAddrType_t regAddrType;
	uint16 deviceAddr; //7bit or 10bit device address
	uint16 regAddr;
	uint16 dataLen;//!< The total data length for read/write

	uint8 dataCheckSum; //!< The byte sum of the data buffer.
	uint32 dataAddrLo; //!< Low 32 bit of the data buffer
	uint32 dataAddrHi; //!< High 32 bit of the data buffer
} I2cMsg_t;


typedef enum _CSM_YUV_Range_t {
	CSM_YUV_RANGE_BT601,
	CSM_YUV_RANGE_FULL,
} CSMYUVRange_t;

typedef struct _ImageProp_t {
	ImageFormat_t imageFormat;
	uint32 width;
	uint32 height;
	uint32 lumaPitch;
	uint32 chromaPitch;
} ImageProp_t;


typedef enum _BufferType_t {
	BUFFER_TYPE_INVALID,
	BUFFER_TYPE_PREVIEW,
	BUFFER_TYPE_VIDEO,
	BUFFER_TYPE_ZSL,
	BUFFER_TYPE_RAW_ZSL,
	BUFFER_TYPE_RAW_TEMP,
	BUFFER_TYPE_PREVIEW_TEMP,
	BUFFER_TYPE_VIDEO_TEMP,
	BUFFER_TYPE_ZSL_TEMP,
	BUFFER_TYPE_HDR_STATS_DATA,
	BUFFER_TYPE_META_INFO,
	BUFFER_TYPE_FRAME_INFO,
	BUFFER_TYPE_TNR_REF,
	BUFFER_TYPE_STAGE2_RECYCLE,
	BUFFER_TYPE_EMB_DATA,
	BUFFER_TYPE_MAX
} BufferType_t;


typedef enum _AddrSpaceType_t {
	ADDR_SPACE_TYPE_GUEST_VA = 0,
	ADDR_SPACE_TYPE_GUEST_PA = 1,
	ADDR_SPACE_TYPE_SYSTEM_PA = 2,
	ADDR_SPACE_TYPE_FRAME_BUFFER_PA = 3,
	ADDR_SPACE_TYPE_GPU_VA = 4,
	ADDR_SPACE_TYPE_MAX = 5
} AddrSpaceType_t;


typedef struct _Buffer_t {
	// A check num for debug usage, host need to
	// set the bufTags to different number
	uint32 bufTags;
	uint32 vmidSpace; //Vmid[31:16], Space[15:0]

	uint32 bufBaseALo;
	uint32 bufBaseAHi;
	uint32 bufSizeA;

	uint32 bufBaseBLo;
	uint32 bufBaseBHi;
	uint32 bufSizeB;

	uint32 bufBaseCLo;
	uint32 bufBaseCHi;
	uint32 bufSizeC;
} Buffer_t;



/**
 * Suppose the image pixel is in the sequence of:
 *             A B C D E F
 *             G H I J K L
 *             M N O P Q R
 *             ...
 * The following RawPktFmt define the raw picture output format.
 * For each format, different raw pixel width will have different memory
 * filling format. The raw pixel width is set by the SesorProp_t.
 *
 * RAW_PKT_FMT_0:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT   |  0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT  |  A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT  |  A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT  |  A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_1:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT     A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT     A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_2:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_3:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_4:
 *    (1) 8-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    (2) 10-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B5  B4  B3  B2  B1  B0  A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               D1  D0  C9  C8  C7  C6  C5  C4  C3  C2  C1  C0  B9  B8  B7  B6
 *               E7  E6  E5  E4  E3  E2  E1  E0  D9  D8  D7  D6  D5  D4  D3  D2
 *               G3  G2  G1  G0  F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E9  E8
 *               H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G9  G8  G7  G6  G5  G4
 *    (3) 12-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B3  B2  B1  B0  A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C7  C6  C5  C4  C3  C2  C1  C0  B11 B10 B9  B8  B7  B6  B5  B4
 *               D11 D10 D9  D8  D7  D6  D5  D4  D3  D2  D1  D0  C11 C10 C9  C8
 *    (4) 14-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B1  B0  A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C3  C2  C1  C0  B13 B12 B11 B10 B9  B8  B7  B6  B5  B4  B3  B2
 *               D5  D4  D3  D2  D1  D0  C13 C12 C11 C10 C9  C8  C7  C6  C5  C4
 *               E7  E6  E5  E4  E3  E2  E1  E0  D13 D12 D11 D10 D9  D8  D7  D6
 *               F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E13 E12 E11 E10 E9  E8
 *              G11 G10 G9  G8  G7  G6  G5  G4  G3  G2  G1  G0  F13 F12 F11 F10
 *              H13 H12 H11 H10 H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G13 G12
 *
 * RAW_PKT_FMT_5:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_6:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      A7  A6  A5  A4  A3  A2  A1  A0  B7  B6  B5  B4  B3  B2  B1  B0
 *    10-BIT     A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0   0   0
 *    12-BIT     A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0
 */
typedef enum _RawPktFmt_t {
	RAW_PKT_FMT_0,/*Default(ISP1P1 legacy format) */
	/*ISP1P1 legacy format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_1,
	RAW_PKT_FMT_2,/*Android RAW16 format */
	/*Android RAW16 format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_3,
	RAW_PKT_FMT_4,/*ISP2.0 bubble-free format */
	RAW_PKT_FMT_5,/*RGB-IR format for GPU process */
	RAW_PKT_FMT_6,/*RGB-IR format for GPU process with data swapped */
	RAW_PKT_FMT_MAX
} RawPktFmt_t;



/***********************************************************
 *                        3A
 ***********************************************************
 */


typedef enum _LockType_t {
	LOCK_TYPE_INVALID,
	LOCK_TYPE_IMMEDIATELY,
	LOCK_TYPE_CONVERGENCE,
	LOCK_TYPE_MAX
} LockType_t;



//AE
//-------

typedef enum _AeMode_t {
	AE_MODE_INVALID,
	AE_MODE_MANUAL,
	AE_MODE_AUTO,
	AE_MODE_MAX
} AeMode_t;


typedef enum _AeFlickerType_t {
	AE_FLICKER_TYPE_INVALID,
	AE_FLICKER_TYPE_OFF,
	AE_FLICKER_TYPE_50HZ,
	AE_FLICKER_TYPE_60HZ,
	AE_FLICKER_TYPE_MAX
} AeFlickerType_t;


typedef struct _AeIso_t {
	uint32 iso;
} AeIso_t;


typedef struct _AeEv_t {
	int32 numerator;
	int32 denominator;
} AeEv_t;


typedef enum _AeLockState_t {
	AE_LOCK_STATE_INVALID,
	AE_LOCK_STATE_LOCKED,
	AE_LOCK_STATE_UNLOCKED,
	AE_LOCK_STATE_MAX
} AeLockState_t;



typedef enum _AeSearchState_t {
	AE_SEARCH_STATE_INVALID,
	AE_SEARCH_STATE_SEARCHING,
	AE_SEARCH_STATE_CONVERGED,
	AE_SEARCH_STATE_MAX
} AeSearchState_t;




typedef struct _AeExposure_t {
	uint32 itime;//!< integration time in microsecond
	uint32 aGain;//!< analog gain muptiplied by 1000
	uint32 dGain;//!< digital gain multiplied by 1000
} AeExposure_t;


typedef enum _FlashMode_t {
	FLASH_MODE_INVALID,
	FLASH_MODE_OFF,
	FLASH_MODE_ON,
	FLASH_MODE_AUTO,
	FLASH_MODE_MAX
} FlashMode_t;


typedef enum _RedEyeMode_t {
	RED_EYE_MODE_INVALID,
	RED_EYE_MODE_OFF,
	RED_EYE_MODE_ON,
	RED_EYE_MODE_MAX
} RedEyeMode_t;



// AWB
//------

typedef enum _AwbMode_t {
	AWB_MODE_INVALID,
	AWB_MODE_MANUAL,
	AWB_MODE_AUTO,
	AWB_MODE_MAX
} AwbMode_t;

typedef struct _CcMatrix_t {
	float coeff[9];
} CcMatrix_t;


typedef struct _CcOffset_t {
	float coeff[3];
} CcOffset_t;

#define MAX_LSC_SECTORS		(16)
#define LSC_DATA_TBL_SIZE	((MAX_LSC_SECTORS + 1) * (MAX_LSC_SECTORS + 1))

typedef struct _LscMatrix_t {
	uint16 dataR[LSC_DATA_TBL_SIZE];
	uint16 dataGR[LSC_DATA_TBL_SIZE];
	uint16 dataGB[LSC_DATA_TBL_SIZE];
	uint16 dataB[LSC_DATA_TBL_SIZE];
} LscMatrix_t;


#define LSC_GRAD_TBL_SIZE (8)
#define LSC_SIZE_TBL_SIZE (8)
typedef struct _LscSector_t {
	uint16 lscXGradTbl[LSC_GRAD_TBL_SIZE];
	uint16 lscYGradTbl[LSC_GRAD_TBL_SIZE];
	uint16 lscXSizeTbl[LSC_SIZE_TBL_SIZE];
	uint16 lscYSizeTbl[LSC_SIZE_TBL_SIZE];
} LscSector_t;


#define PREP_LSC_GRAD_TBL_SIZE (8)
#define PREP_LSC_SIZE_TBL_SIZE (16)
typedef struct _PrepLscSector_t {
	uint16 lscXGradTbl[PREP_LSC_GRAD_TBL_SIZE];
	uint16 lscYGradTbl[PREP_LSC_GRAD_TBL_SIZE];
	uint16 lscXSizeTbl[PREP_LSC_SIZE_TBL_SIZE];
	uint16 lscYSizeTbl[PREP_LSC_SIZE_TBL_SIZE];
} PrepLscSector_t;


typedef enum _LscRamSetId_t {
	LSC_RAM_SET_ID_INVALID = -1,
	LSC_RAM_SET_ID_0 = 0,
	LSC_RAM_SET_ID_1 = 1,
	LSC_RAM_SET_ID_2 = 2,
	LSC_RAM_SET_ID_3 = 3,
	LSC_RAM_SET_ID_4 = 4,
	LSC_RAM_SET_ID_5 = 5,
	LSC_RAM_SET_ID_MAX = 6,
} LscRamSetId_t;


typedef struct _illuLikeHood_t {
	float coeff[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
} illuLikeHood_t;


typedef enum _AwbLockState_t {
	AWB_LOCK_STATE_INVALID,
	AWB_LOCK_STATE_LOCKED,
	AWB_LOCK_STATE_UNLOCKED,
	AWB_LOCK_STATE_MAX
} AwbLockState_t;


typedef enum _AwbSearchState_t {
	AWB_SEARCH_STATE_INVALID,
	AWB_SEARCH_STATE_SEARCHING,
	AWB_SEARCH_STATE_CONVERGED,
	AWB_SEARCH_STATE_MAX
} AwbSearchState_t;


typedef struct _WbGain_t {
	float red;
	float greenR;
	float greenB;
	float blue;
} WbGain_t;

typedef struct _AwbmConfig_t {
	uint8 maxY;
	uint8 refCr_maxR;
	uint8 minY_maxG;
	uint8 refCb_maxB;
	uint8 maxCSum;
	uint8 minC;
} AwbmConfig_t;


typedef struct _WbGainRatio_t {
	uint8 lightIdx;
	float rGainRatio;
	float bGainRatio;
} WbGainRatio_t;


typedef struct _WbGainRatioAll_t {
	WbGainRatio_t ratio[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
} WbGainRatioAll_t;


//  AF
//-------

typedef enum _AfMode_t {
	AF_MODE_INVALID,
	AF_MODE_MANUAL,
	AF_MODE_ONE_SHOT,
	AF_MODE_CONTINUOUS_PICTURE,
	AF_MODE_CONTINUOUS_VIDEO,
	AF_MODE_MAX
} AfMode_t;


typedef struct _LensRange_t {
	uint32 minLens;
	uint32 maxLens;
	uint32 stepLens;
} LensRange_t;


typedef enum _AfWindowId_t {
	AF_WINDOW_ID_A,
	AF_WINDOW_ID_B,
	AF_WINDOW_ID_C,
	AF_WINDOW_ID_MAX
} AfWindowId_t;


typedef enum _FocusAssistMode_t {
	FOCUS_ASSIST_MODE_INVALID,
	FOCUS_ASSIST_MODE_OFF,
	FOCUS_ASSIST_MODE_ON,
	FOCUS_ASSIST_MODE_AUTO,
	FOCUS_ASSIST_MODE_MAX
} FocusAssistMode_t;



typedef enum _AfLockState_t {
	AF_LOCK_STATE_INVALID,
	AF_LOCK_STATE_LOCKED,
	AF_LOCK_STATE_UNLOCKED,
	AF_LOCK_STATE_MAX
} AfLockState_t;


typedef enum _AfSearchState_t {
	AF_SEARCH_STATE_INVALID,
	AF_SEARCH_STATE_INIT,
	AF_SEARCH_STATE_SEARCHING,
	AF_SEARCH_STATE_CONVERGED,
	AF_SEARCH_STATE_FAILED,
	AF_SEARCH_STATE_MAX
} AfSearchState_t;



// CPROC
//--------

typedef enum _CprocChromOutRange_t {
	CPROC_CHROM_OUT_RANGE_BT601,
	CPROC_CHROM_OUT_RANGE_FULL,
	CPROC_CHROM_OUT_RANGE_MAX
} CprocChromOutRange_t;

typedef enum _CprocLumInRange_t {
	CPROC_LUM_IN_RANGE_BT601,
	CPROC_LUM_IN_RANGE_FULL,
	CPROC_LUM_IN_RANGE_MAX
} CprocLumInRange_t;



typedef enum _CprocLumOutRange_t {
	CPROC_LUM_OUT_RANGE_BT601,
	CPROC_LUM_OUT_RANGE_FULL,
	CPROC_LUM_OUT_RANGE_MAX
} CprocLumOutRange_t;

typedef struct _CprocProp_t {
	CprocChromOutRange_t chromOutRange;

	CprocLumInRange_t lumInRange;
	CprocLumOutRange_t lumOutRange;
} CprocProp_t;




// GAMMA
//-------

typedef enum _GammaSegMode_t {
	GAMMA_SEG_MODE_LOG = 0,
	GAMMA_SEG_MODE_EQU = 1,
	GAMMA_SEG_MODE_MAX
} GammaSegMode_t;


#define GAMMA_CURVE_SIZE (17)
typedef struct _GammaCurve_t {
	uint16 gammaY[GAMMA_CURVE_SIZE];
} GammaCurve_t;



//  BLS
//-------

typedef struct _BlackLevel_t {
	uint16 blsValue[4];
} BlackLevel_t;


// DEGAMMA
//---------
#define DEGAMMA_CURVE_SIZE		(17)
typedef struct _DegammaCurve_t {
	uint8 segment[DEGAMMA_CURVE_SIZE - 1]; /*x_i segment size*/
	uint16 red[DEGAMMA_CURVE_SIZE]; /*red point */
	uint16 green[DEGAMMA_CURVE_SIZE]; /*green point */
	uint16 blue[DEGAMMA_CURVE_SIZE];/*blue point */
} DegammaCurve_t;





// FRAME CONTROL
//---------------

typedef enum _FrameControlType_t {
	FRAME_CONTROL_TYPE_INVALID,
	FRAME_CONTROL_TYPE_HALF,
	FRAME_CONTROL_TYPE_FULL,
	FRAME_CONTROL_TYPE_MAX
} FrameControlType_t;


typedef enum _ActionType_t {
	ACTION_TYPE_INVALID = -1,
	ACTION_TYPE_KEEP_STATE = 0,//Don't change the state
	ACTION_TYPE_APPLY_ONCE = 1,//Change the state only for one frame
	ACTION_TYPE_APPLY_NEW = 2,//Change the state for the following frames
	//Use the stream control state. Only can be set in half control.
	ACTION_TYPE_USE_STREAM = 3,
	ACTION_TYPE_MAX
} ActionType_t;




// AE Frame Control
//------------------

typedef struct _ActionAeMode_t {
	ActionType_t actionType;
	AeMode_t mode;
} ActionAeMode_t;

typedef struct _ActionAeFlickerType_t {
	ActionType_t actionType;
	AeFlickerType_t flickerType;
} ActionAeFlickerType_t;


typedef struct _ActionAeRegion_t {
	ActionType_t actionType;
	Window_t region;
} ActionAeRegion_t;


typedef struct _ActionAeLock_t {
	ActionType_t actionType;
	LockType_t lockType;
} ActionAeLock_t;



typedef struct _ActioinAeExposure_t {
	ActionType_t actionType;
	AeExposure_t exposure;
} ActionAeExposure_t;


typedef struct _ActionAeIso_t {
	ActionType_t actionType;
	AeIso_t iso;
} ActionAeIso_t;


typedef struct _ActionAeEv_t {
	ActionType_t actionType;
	AeEv_t ev;
} ActionAeEv_t;


typedef struct _ActionAeFlashMode_t {
	ActionType_t actionType;
	FlashMode_t flashMode;
} ActionAeFlashMode_t;


typedef struct _ActionAeFlashPowerLevel_t {
	ActionType_t actionType;
	uint32 powerLevel;
} ActionAeFlashPowerLevel_t;


typedef struct _ActionAeRedEyeMode_t {
	ActionType_t actionType;
	RedEyeMode_t redEyeMode;
} ActionAeRedEyeMode_t;


typedef struct _FrameControlAe_t {
	ActionAeMode_t aeMode;
	ActionAeFlickerType_t aeFlickerType;
	ActionAeRegion_t aeRegion;
	ActionAeLock_t aeLock;
	ActionAeExposure_t aeExposure;
	ActionAeIso_t aeIso;
	ActionAeEv_t aeEv;
	ActionAeFlashMode_t aeFlashMode;
	ActionAeFlashPowerLevel_t aeFlashPowerLevel;
	ActionAeRedEyeMode_t aeRedEyeMode;
} FrameControlAe_t;



// AWB Frame Control
//------------------

typedef struct _ActionAwbMode_t {
	ActionType_t actionType;
	AwbMode_t mode;
} ActionAwbMode_t;


typedef struct _ActionAwbLock_t {
	ActionType_t actionType;
	LockType_t lockType;
} ActionAwbLock_t;


typedef struct _ActionAwbGain_t {
	ActionType_t actionType;
	WbGain_t wbGain;
} ActionAwbGain_t;


typedef struct _ActionAwbCcMatrix_t {
	ActionType_t actionType;
	CcMatrix_t ccMatrix;
} ActionAwbCcMatrix_t;


typedef struct _ActionAwbCcOffset_t {
	ActionType_t actionType;
	CcOffset_t ccOffset;
} ActionAwbCcOffset_t;



typedef struct _ActionAwbColorTemperature_t {
	ActionType_t actionType;
	uint32 colorTemperature;
} ActionAwbColorTemperature_t;



typedef struct _FrameControlAwb_t {
	ActionAwbMode_t awbMode;
	ActionAwbLock_t awbLock;
	ActionAwbGain_t awbGain;
	ActionAwbCcMatrix_t awbCcMatrix;
	ActionAwbCcOffset_t awbCcOffset;
	ActionAwbColorTemperature_t awbColorTemp;
} FrameControlAwb_t;



// AWB Frame Control
//------------------

typedef struct _ActionAfMode_t {
	ActionType_t actionType;
	AfMode_t mode;
} ActionAfMode_t;


typedef struct _ActionAfRegion_t {
	ActionType_t actionType;
	Window_t region[AF_WINDOW_ID_MAX];
} ActionAfRegion_t;


typedef struct _ActionAfLensPos_t {
	ActionType_t actionType;
	uint32 lensPos;
} ActionAfLensPos_t;


typedef struct _ActionAfTrig_t {
	ActionType_t actionType;
	bool_t trig;
} ActionAfTrig_t;


typedef struct _ActionAfCancel_t {
	ActionType_t actionType;
	bool_t cancel;
} ActionAfCancel_t;


typedef struct _ActionAfFocusAssist_t {
	ActionType_t actionType;
	FocusAssistMode_t focusAssistMode;
} ActionAfFocusAssist_t;



typedef struct _FrameControlAf_t {
	ActionAfMode_t afMode;
	ActionAfRegion_t afRegion;
	ActionAfLensPos_t afLensPos;
	ActionAfTrig_t afTrig;
	ActionAfCancel_t afCancel;
	ActionAfFocusAssist_t afFocusAssist;
} FrameControlAf_t;




typedef struct _ActionPreviewBuf_t {
	bool_t enabled;
	Buffer_t buffer;
	ImageProp_t imageProp;
} ActionPreviewBuf_t;


typedef struct _ActionVideoBuf_t {
	bool_t enabled;
	Buffer_t buffer;
	ImageProp_t imageProp;
} ActionVideoBuf_t;


typedef struct _ActionZslBuf_t {
	bool_t enabled;
	Buffer_t buffer;
	ImageProp_t imageProp;
} ActionZslBuf_t;


typedef struct _ActionRawBuf_t {
	bool_t enabled;
	Buffer_t buffer;
} ActionRawBuf_t;

typedef enum _FrameType_t {
	FRAME_TYPE_INVALID = 0,
	FRAME_TYPE_RGBBAYER = 1,
	FRAME_TYPE_IR = 2,
	FRAME_TYPE_MAX
} FrameType_t;

typedef struct _FrameControl_t {
	FrameControlType_t controlType;
	uint16 tags;
	FrameControlAe_t ae;
	FrameControlAf_t af;
	FrameControlAwb_t awb;
	ActionPreviewBuf_t previewBuf;
	ActionVideoBuf_t videoBuf;
	ActionZslBuf_t zslBuf;
	ActionRawBuf_t rawBuf;
} FrameControl_t;



//  FrameInfo
//-------------


typedef struct _AwbmResult_t {
	uint32 whiteCnt;
	uint8 meanY_G;
	uint8 meanCb_B;
	uint8 meanCr_R;
} AwbmResult_t;


#define HIST_NUM_BINS (16)
typedef struct _HistResult_t {
	uint32 bins[HIST_NUM_BINS];
} HistResult_t;




typedef struct _AwbCtrlInfo_t {
	AwbLockState_t lockState;
	CcMatrix_t ccMatrix;
	CcOffset_t ccOffset;
	WbGain_t wbGain;
	LscMatrix_t lscMatrix;
#ifdef SUPPORT_NEW_AWB
	AwbmConfig_t awbWpRegion;
#endif
	PrepLscSector_t prepLscSector;
#ifdef CONFIG_ENABLE_LSC_IN_ISP_KERNEL
	LscSector_t lscSector;
#endif
	Window_t regionWindow;
	illuLikeHood_t illuLikeHood;

	//For meta data
	AwbMode_t mode;
	AwbSearchState_t searchState;
	uint32 colorTemperature;
} AwbCtrlInfo_t;


typedef struct _AwbmInfo_t {
	bool_t enabled;
	// The frame white points measuring result.
	AwbmResult_t awbmResult;
	HistResult_t histResult;

	// The coefficient applied on this frame.
	CcMatrix_t ccMatrix;
	CcOffset_t ccOffset;
	WbGain_t wbGain;
	LscMatrix_t lscMatrix;
#ifdef SUPPORT_NEW_AWB
	AwbmConfig_t awbWpRegion;
#endif
	uint32 aGain;
	uint32 itime;

	bool_t flashOn;
	uint32 flashPowerLevel;
} AwbmInfo_t;


typedef struct _ExpResult_t {
	uint8 luma[25];
} ExpResult_t;

typedef enum _IRIlluStatus_t {
	IR_ILLU_STATUS_UNKNOWN,
	IR_ILLU_STATUS_ON,
	IR_ILLU_STATUS_OFF,
	IR_ILLU_STATUS_MAX,
} IRIlluStatus_t;

typedef struct _IRMetaInfo_t {
	IRIlluStatus_t iRIlluStatus;
} IRMetaInfo_t;

typedef struct _AemInfo_t {
	//Normal AE measuring result
	bool_t expEnabled;
	ExpResult_t expResult;

	HistResult_t histResult;

	//HDR AE measuring result
	bool_t hdrEnabled;
	HdrStatDataMatrixId_t matrixId;
	uint16 lumaValue[256];

	//Whether it is a precapture measuring frame
	bool_t preCapMeasure;
	uint32 aGain;
	uint32 dGain;
	uint32 itime;

	FrameType_t frameType;

	uint32 frameRate; //Actual framerate multiplied by 1000

	//The flash status for the measuring frame
	bool_t flashOn;
	uint32 flashPowerLevel;

	// For IR frame only
	// The IR illuminator status for the measuring frame
	IRIlluStatus_t iRIlluStatus;
} AemInfo_t;

typedef struct _FrameExpInfo_t {
	uint32 itime;
	uint32 aGain;
	uint32 dGain;
	uint32 hdrHighItime;
	uint32 hdrLowItime;
	uint32 hdrLowItimeRatio;
	uint32 hdrEqualItime;
	uint32 hdrHighAGain;
	uint32 hdrLowAGain;
	uint32 hdrLowAGainRatio;
	uint32 hdrEqualAGain;
	uint32 hdrHighDGain;
	uint32 hdrLowDGain;
	uint32 hdrLowDGainRatio;
	uint32 hdrEqualDGain;
} FrameExpInfo_t;

typedef struct _AeCtrlInfo_t {
	// For ctrl info
	AeLockState_t lockState;
	Window_t region;
	uint32 itime;
	uint32 aGain;
	uint32 dGain;
	bool_t hdrEnabled;
	bool_t hdrParamValid;
	HdrItimeType_t hdrItimeType;
	uint32 hdrHighItime;
	uint32 hdrLowItime;
	uint32 hdrLowItimeRatio;
	uint32 hdrEqualItime;
	HdrAGainType_t hdrAGainType;
	uint32 hdrHighAGain;
	uint32 hdrLowAGain;
	uint32 hdrLowAGainRatio;
	uint32 hdrEqualAGain;
	HdrDGainType_t hdrDGainType;
	uint32 hdrHighDGain;
	uint32 hdrLowDGain;
	uint32 hdrLowDGainRatio;
	uint32 hdrEqualDGain;
	bool_t flashOn;
	uint32 flashPowerLevel;
	// Whether it is a precapture measuring frame, this flag will be back
	//to AeMgr from
	bool_t preCapMeasure;
	// the AemInfo_t.
	// For meta data
	AeMode_t mode;
	AeFlickerType_t flickerType;
	AeSearchState_t searchState;
	AeIso_t iso;
	AeEv_t ev;
	FlashMode_t flashMode;

	RedEyeMode_t redEyeMode;
} AeCtrlInfo_t;

typedef struct _AfmResult_t {
	uint32 sharpnessA;
	uint32 sharpnessB;
	uint32 sharpnessC;

	uint32 luminanceA;
	uint32 luminanceB;
	uint32 luminanceC;

	uint32 pixelCntA;
	uint32 pixelCntB;
	uint32 pixelCntC;

	uint32 maxPixelCnt;

	uint32 lumaShift;
	uint32 afmShift;
} AfmResult_t;



typedef struct _AfmInfo_t {
	bool_t enabled;
	AfmResult_t afmResult;

	uint32 lensPos;

	Window_t regionWindow[AF_WINDOW_ID_MAX];

	//The flash status for the measuring frame
	bool_t flashOn;
	uint32 flashPowerLevel;
} AfmInfo_t;


typedef struct _AfCtrlInfo_t {
	//For ctrl and meta data
	AfLockState_t lockState;
	bool_t bSupportFocus;
	uint32 lensPos;
	Window_t regionWindow[AF_WINDOW_ID_MAX];
	//Indicate whether to turn on the flash light for the current frame
	bool_t focusAssistOn;
	//Indicate the flash power level for the current frame.
	uint32 focusAssistPowerLevel;

	//For meta data
	AfMode_t mode;
	FocusAssistMode_t focusAssistMode;
	AfSearchState_t searchState;
} AfCtrlInfo_t;


typedef enum _BufferStatus_t {
	BUFFER_STATUS_INVALID,
	BUFFER_STATUS_SKIPPED, //The buffer is not filled with image data
	BUFFER_STATUS_EXIST, //The buffer is exist and waiting for filled.
	BUFFER_STATUS_DONE,//The buffer is filled with image data
	BUFFER_STATUS_LACK,//The buffer is unavaiable
	//The buffer is dirty, probably caused by LMI leakage
	BUFFER_STATUS_DIRTY,
	BUFFER_STATUS_MAX
} BufferStatus_t;

typedef struct _BufferStatusEx_t {
	BufferStatus_t status;
	ErrorCode_t err;
} BufferStatusEx_t;

typedef enum _BufferSource_t {
	BUFFER_SOURCE_INVALID,
	BUFFER_SOURCE_CMD_CAPTURE,//The buffer is from a capture command
	//The buffer is from a per frame control command
	BUFFER_SOURCE_FRAME_CONTROL,
	BUFFER_SOURCE_STREAM, //The buffer is from the stream buffer queue.
	//The buffer is added by host post process in mode3 post frame info.
	BUFFER_SOURCE_HOST_POST,
	BUFFER_SOURCE_TEMP,
	BUFFER_SOURCE_RRECYCLE,
	BUFFER_SOURCE_MAX
} BufferSource_t;


typedef struct _BufferFrameInfo_t {
	bool_t enabled;
	BufferStatusEx_t statusEx;

	BufferSource_t source;
	ImageProp_t imageProp;
	Buffer_t buffer;
} BufferFrameInfo_t;


typedef struct _CprocStatus_t {
	bool_t enabled;
	CprocProp_t cprocProp;
	uint8 contrast;
	uint8 brightness;
	uint8 saturation;
	uint8 hue;
} CprocStatus_t;


#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
typedef enum GammaMode_t {
	GAMMA_MODE_INVALID,
	GAMMA_MODE_MANUAL,
	GAMMA_MODE_AUTO,
	GAMMA_MODE_MAX
} GammaMode_t;
#endif


typedef struct _GammaStatus_t {
	bool_t enabled;

	GammaSegMode_t segMode;
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
	uint8 activeGammaCurveNum;
	GammaMode_t gammaMode;
	GammaCurve_t curveResult;
	GammaCurve_t curve[TDB_GAMMA_CURVE_MAX_NUM];
	uint32 exposureTheshold[TDB_GAMMA_CURVE_MAX_NUM - 1];
	float dampingCoef;
#else
	GammaCurve_t curve;
#endif
} GammaStatus_t;

typedef struct _StnrStatus_t {
	bool_t snrEnabled;
	bool_t tnrEnabled;
	TdbStnr_t tdbStnr;
} StnrStatus_t;


typedef struct _BlsStatus_t {
	bool_t enabled;
	BlackLevel_t blackLevel;
} BlsStatus_t;


typedef struct _DegammaStatus_t {
	bool_t enabled;
	DegammaCurve_t curve;
} DegammaStatus_t;


typedef enum _DpfGainUsage_t {
	DPF_GAIN_USAGE_INVALID = 0,
	DPF_GAIN_USAGE_DISABLED = 1,
	DPF_GAIN_USAGE_NF_GAINS = 2,
	DPF_GAIN_USAGE_LSC_GAINS = 3,
	DPF_GAIN_USAGE_NF_LSC_GAINS = 4,
	DPF_GAIN_USAGE_AWB_GAINS = 5,
	DPF_GAIN_USAGE_AWB_LSC_GAINS = 6,

	DPF_GAIN_USAGE_MAX
} DpfGainUsage_t;


typedef enum _DpfRBFilterSize_t {
	DPF_RB_FILTER_SIZE_INVLAID = 0,
	DPF_RB_FILTER_SIZE_9x9 = 1,
	DPF_RB_FILTER_SIZE_13x9 = 2,

	DPF_RB_FILTER_SIZE_MAX
} DpfRBFilterSize_t;


#define DPF_MAX_SPATIAL_COEFFS		(6)
typedef struct _DpfSpatial_t {
	uint8 weightCoeff[DPF_MAX_SPATIAL_COEFFS];
} DpfSpatial_t;


typedef struct _DpfNfGain_t {
	uint16 red;
	uint16 greenR;
	uint16 greenB;
	uint16 blue;
} DpfNfGain_t;


typedef struct _DpfInvStrength_t {
	uint8 weightR;
	uint8 weightG;
	uint8 weightB;
} DpfInvStrength_t;


typedef enum _DpfNoiseLevelLookUpScale_t {
	DPF_NOISE_LEVEL_LOOK_UP_SCALE_LINEAR = 0,
	DPF_NOISE_LEVEL_LOOK_UP_SCALE_LOGARITHMIC = 1,
} DpfNoiseLevelLookUpScale_t;


#define DPF_MAX_NLF_COEFFS		(17)
typedef struct _DpfNoiseLevelLookUp_t {
	uint16 nllCoeff[DPF_MAX_NLF_COEFFS];
	DpfNoiseLevelLookUpScale_t xScale;
} DpfNoiseLevelLookUp_t;


typedef struct _DpfSubConfig_t {
	DpfGainUsage_t gainUsage;
	DpfRBFilterSize_t rbFilterSize;

	bool_t processRedPixel;
	bool_t processGreenRPixel;
	bool_t processGreenBPixel;
	bool_t processBluePixel;

	DpfSpatial_t spatialG;
	DpfSpatial_t spatialRB;

} DpfSubConfig_t;


typedef struct _DpfConfig_t {
	DpfSubConfig_t subConfig;
	DpfNfGain_t nfGain;
	DpfInvStrength_t dynInvStrength;
	DpfNoiseLevelLookUp_t nll;

	float gradient;
	float offset;
} DpfConfig_t;


typedef struct _DpfStatus_t {
	bool_t enabled;
	DpfConfig_t dpfConfig;
} DpfStatus_t;


typedef struct _DpccStaticConfig_t {
	uint32 isp_dpcc_mode;
	uint32 isp_dpcc_output_mode;
	uint32 isp_dpcc_set_use;

	uint32 isp_dpcc_methods_set_1;
	uint32 isp_dpcc_methods_set_2;
	uint32 isp_dpcc_methods_set_3;

	uint32 isp_dpcc_line_thresh_1;
	uint32 isp_dpcc_line_mad_fac_1;
	uint32 isp_dpcc_pg_fac_1;
	uint32 isp_dpcc_rnd_thresh_1;
	uint32 isp_dpcc_rg_fac_1;

	uint32 isp_dpcc_line_thresh_2;
	uint32 isp_dpcc_line_mad_fac_2;
	uint32 isp_dpcc_pg_fac_2;
	uint32 isp_dpcc_rnd_thresh_2;
	uint32 isp_dpcc_rg_fac_2;

	uint32 isp_dpcc_line_thresh_3;
	uint32 isp_dpcc_line_mad_fac_3;
	uint32 isp_dpcc_pg_fac_3;
	uint32 isp_dpcc_rnd_thresh_3;
	uint32 isp_dpcc_rg_fac_3;

	uint32 isp_dpcc_ro_limits;
	uint32 isp_dpcc_rnd_offs;
} DpccStaticConfig_t;

typedef struct _DpccStatus_t {
	bool_t enabled;
	TdbDpcc_t tdbDpcc;
} DpccStatus_t;


typedef enum _CacHClipMode_t {
	CAC_H_CLIP_MODE_FIX4,
	CAC_H_CLIP_MODE_DYN5,
	CAC_H_CLIP_MODE_MAX
} CacHClipMode_t;


typedef enum _CacVClipMode_t {
	CAC_V_CLIP_MODE_FIX2,
	CAC_V_CLIP_MODE_FIX3,
	CAC_V_CLIP_MODE_DYN4,
	CAC_V_CLIP_MODE_MAX
} CacVClipMode_t;


typedef struct _CacConfig_t {
	uint16 width;
	uint16 height;
	int16 hCenterOffset;
	int16 vCenterOffset;

	CacHClipMode_t hClipMode;
	CacVClipMode_t vClipMode;
	uint16 aBlue;
	uint16 aRed;
	uint16 bBlue;
	uint16 bRed;
	uint16 cBlue;
	uint16 cRed;
	uint8 xNs;
	uint8 xNf;
	uint8 yNs;
	uint8 yNf;
} CacConfig_t;


typedef struct _CacStatus_t {
	bool_t enabled;
	TdbCac_t tdbCac;
} CacStatus_t;


#define WDR_DX_SIZE (33)
#define WDR_DY_SIZE (33)
typedef struct _WdrCurve_t {
	uint16 dx[WDR_DX_SIZE];
	uint16 dy[WDR_DY_SIZE];
} WdrCurve_t;



typedef struct _WdrStatus_t {
	bool_t enabled;
	WdrCurve_t curve;
} WdrStatus_t;


typedef struct _CnrParam_t {
	uint32 threshold1;
	uint32 threshold2;
} CnrParam_t;


typedef struct _CnrStatus_t {
	bool_t enabled;
	CnrParam_t param;
} CnrStatus_t;


typedef struct _DemosaicThresh_t {
	uint8 thresh;
} DemosaicThresh_t;


typedef struct _DemosaicStatus_t {
	uint8 thresh;
	bool_t enabled;
} DemosaicStatus_t;


typedef struct _DenoiseFilter_t {
	int16 coeff[TDB_ISP_FILTER_DENOISE_SIZE];
} DenoiseFilter_t;


typedef struct _SharpenFilter_t {
	int8 coeff[TDB_ISP_FILTER_SHARP_SIZE];
} SharpenFilter_t;


typedef struct _IspFilterStatus_t {
	bool_t enabled;
	TdbIspFilter_t tdbIspFilter;
	bool_t hostSetDenoiseCurve;
	bool_t hostSetSharpenCurve;
} IspFilterStatus_t;


typedef struct _YuvCapCmd_t {
	Cmd_t cmd;
	ImageProp_t imageProp;

	Buffer_t buffer;
	bool_t usePreCap;
} YuvCapCmd_t;


typedef struct _RawCapCmd_t {
	Cmd_t cmd;
	Buffer_t buffer;

	bool_t usePreCap;
} RawCapCmd_t;


typedef struct _FrameControlCmd_t {
	/*The host commamnd structure which send this FrameControlCmd_t */
	Cmd_t cmd;
	FrameControl_t frameControl;
} FrameControlCmd_t;


//For 3A report generation by host
#define AE_REPORT_HORI_MAX_CNT		(19)
#define AE_REPORT_VERT_MAX_CNT		(13)
#define AWB_REPORT_HORI_MAX_CNT		(19)
#define AWB_REPORT_VERT_MAX_CNT		(13)
#define AF_REPORT_HORI_MAX_CNT		(19)
#define AF_REPORT_VERT_MAX_CNT		(13)

#define AE_REPORT_ZONE_H_SIZE		(32)
#define AE_REPORT_ZONE_V_SIZE		(32)
#define AWB_REPORT_ZONE_H_SIZE		(32)
#define AWB_REPORT_ZONE_V_SIZE		(32)
#define AF_REPORT_ZONE_H_SIZE		(32)
#define AF_REPORT_ZONE_V_SIZE		(32)

#define AE_REPORT_STAT_HORI_MAX \
	(AE_REPORT_HORI_MAX_CNT * AE_REPORT_ZONE_H_SIZE)
#define AE_REPORT_STAT_VERT_MAX \
	(AE_REPORT_VERT_MAX_CNT * AE_REPORT_ZONE_V_SIZE)
#define AWB_REPORT_STAT_HORI_MAX \
	(AWB_REPORT_HORI_MAX_CNT * AWB_REPORT_ZONE_H_SIZE)
#define AWB_REPORT_STAT_VERT_MAX \
	(AWB_REPORT_VERT_MAX_CNT * AWB_REPORT_ZONE_V_SIZE)
#define AF_REPORT_STAT_HORI_MAX \
	(AF_REPORT_HORI_MAX_CNT * AF_REPORT_ZONE_H_SIZE)
#define AF_REPORT_STAT_VERT_MAX \
	(AF_REPORT_VERT_MAX_CNT * AF_REPORT_ZONE_V_SIZE)

#define AE_REPORT_UNIT_SIZE	(2) //2 bytes for each AE report
#define AWB_REPORT_UNIT_SIZE	(2) //2 bytes for each AWB report
#define AF_REPORT_UNIT_SIZE	(2) //2 bytes for each AF report

//Since uint16 or uint32 is used and therefore no need to x2 or x4 here
#define AE_REPORT_MAX_SIZE \
	(AE_REPORT_HORI_MAX_CNT * AE_REPORT_VERT_MAX_CNT)
#define AWB_REPORT_MAX_SIZE \
	(AWB_REPORT_HORI_MAX_CNT * AWB_REPORT_VERT_MAX_CNT)
#define AF_REPORT_MAX_SIZE \
	(AF_REPORT_HORI_MAX_CNT * AF_REPORT_VERT_MAX_CNT)
#define AE_AWB_AF_REPORT_MAX_TOTAL_SIZE \
	(AE_REPORT_MAX_SIZE + AWB_REPORT_MAX_SIZE + AF_REPORT_MAX_SIZE)


typedef struct _Mode3FrameInfo_t {
	uint32 poc;
	uint32 timeStampLo;
	uint32 timeStampHi;

	bool_t enableFrameControl;
	FrameControlCmd_t frameControlCmd;

	//Yuv Capture Command
	bool_t enableYuvCap;
	YuvCapCmd_t yuvCapCmd;

	//Raw Capture Command
	bool_t enableRawCap;
	RawCapCmd_t rawCapCmd;

	uint32 flashPowerLevel;
	bool_t flashOn;

	uint32 frameRate; //Actual framerate multiplied by 1000

	Window_t zoomWindow;
	bool_t hasZoomCmd;
	Cmd_t zoomCmd;

	AfCtrlInfo_t afCtrlInfo;
	AeCtrlInfo_t aeCtrlInfo;
	BufferFrameInfo_t rawBufferFrameInfo;
	RawPktFmt_t rawPktFmt;
	uint32 rawWidthInByte;
	CFAPattern_t cfaPattern;
	FrameType_t frameType;
	IRIlluStatus_t iRIlluStatus;
	FrameExpInfo_t frameExpInfo;
} Mode3FrameInfo_t;


typedef struct _Shader3AReportInfo_t {
	//3A report from host
	//Use this flag to indicate host gives 3A report info or does not give.
	//Host can give 3A report in some use cases and not in other use cases.
	bool_t isValid;

	uint16 aeReport[AE_REPORT_HORI_MAX_CNT][AE_REPORT_VERT_MAX_CNT];
	uint16 awbReport[AWB_REPORT_HORI_MAX_CNT][AWB_REPORT_VERT_MAX_CNT];
	uint16 afReport[AF_REPORT_HORI_MAX_CNT][AF_REPORT_VERT_MAX_CNT];

	uint8 aeReportHoriCnt;
	uint8 aeReportVertCnt;
	uint8 awbReportHoriCnt;
	uint8 awbReportVertCnt;
	uint8 afReportHoriCnt;
	uint8 afReportVertCnt;
} Shader3AReportInfo_t;


typedef struct _Mode3FrameInfoHost2FW_t {
	Mode3FrameInfo_t baseMode3FrameInfo;
#ifdef HOST_PROCESSING_3A
	Shader3AReportInfo_t shader3AReportInfo;
#endif
} Mode3FrameInfoHost2FW_t;


typedef struct _Mode4FrameInfo_t {
	uint32 poc;
	uint32 timeStampLo;
	uint32 timeStampHi;

	FrameType_t frameType;

	uint32 frameRate; //Actual framerate multiplied by 1000

	Window_t zoomWindow;

	AfCtrlInfo_t afCtrlInfo;
	AeCtrlInfo_t aeCtrlInfo;
	BufferFrameInfo_t rawBufferFrameInfo;
	RawPktFmt_t rawPktFmt;
	uint32 rawWidthInByte;
	CFAPattern_t cfaPattern;

	IRIlluStatus_t iRIlluStatus;
	FrameExpInfo_t frameExpInfo;
} Mode4FrameInfo_t;


#define RESIZE_TBL_SIZE (128)
typedef struct _ResizeTable_t {
	int16 coeff[RESIZE_TBL_SIZE];
} ResizeTable_t;



// MetaInfo
//-----------

typedef struct _BufferMetaInfo_t {
	bool_t enabled;
	BufferStatusEx_t statusEx;
	BufferSource_t source;
	ImageProp_t imageProp;
	Buffer_t buffer;
} BufferMetaInfo_t;

typedef struct _AeMetaInfo_t {
	AeMode_t mode;
	AeFlickerType_t flickerType;
	Window_t regionWindow;
	AeLockState_t lockState;
	AeSearchState_t searchState;
	AeIso_t iso;
	AeEv_t ev;
	uint32 itime;
	uint32 aGain;
	uint32 dGain;
	bool_t hdrEnabled;
	HdrItimeType_t hdrItimeType;
	uint32 hdrHighItime;
	uint32 hdrLowItime;
	uint32 hdrLowItimeRatio;
	uint32 hdrEqualItime;
	HdrAGainType_t hdrAGainType;
	uint32 hdrHighAGain;
	uint32 hdrLowAGain;
	uint32 hdrLowAGainRatio;
	uint32 hdrEqualAGain;
	HdrDGainType_t hdrDGainType;
	uint32 hdrHighDGain;
	uint32 hdrLowDGain;
	uint32 hdrLowDGainRatio;
	uint32 hdrEqualDGain;
} AeMetaInfo_t;

typedef struct _AwbMetaInfo_t {
	AwbMode_t mode;
	Window_t regionWindow;
	AwbLockState_t lockState;
	WbGain_t wbGain;
	CcMatrix_t ccMatrix;
	CcOffset_t ccOffset;
	illuLikeHood_t illuLikeHood;
	AwbSearchState_t searchState;
	uint32 colorTemperature;
} AwbMetaInfo_t;

typedef struct _AfMetaInfo_t {
	AfMode_t mode;
	uint32 lensPos;
	Window_t regionWindow[AF_WINDOW_ID_MAX];
	float sharpness[AF_WINDOW_ID_MAX];
	AfLockState_t lockState;
	AfSearchState_t searchState;
} AfMetaInfo_t;

typedef struct _FlashMetaInfo_t {
	FlashMode_t flashMode; //For Ae
	RedEyeMode_t redEyeMode;//For Ae

	FocusAssistMode_t focusAssistMode; //For Af
	//The result power level according to the AE flash mode and AF focus
	//assist setting
	uint32 powerLevel;
	//Indicate whether the flash is on for the frame according to the
	//current AE parameters.
	bool_t flashOn;
} FlashMetaInfo_t;


typedef struct _MetaInfo_t {
	uint32 poc;
	uint32 timeStampLo;
	uint32 timeStampHi;
	BufferMetaInfo_t preview;
	BufferMetaInfo_t video;
	BufferMetaInfo_t zsl;
	BufferMetaInfo_t raw;
	/*The raw buffer packet format if the raw is exist */
	RawPktFmt_t rawPktFmt;
	Window_t zoomWindow;
	AeMetaInfo_t ae;
	AfMetaInfo_t af;
	AwbMetaInfo_t awb;
	FlashMetaInfo_t flash;

	/*
	 *The tags in FrameControl_t command if this frame has
	 *been applied a frame control command.
	 *If no frame control command is applied on this frame,
	 *the fcTags will be set to 0
	 */
	uint16 fcTags;
	IRMetaInfo_t IRMeta;
	FrameType_t frameType;
} MetaInfo_t;


/*The BufCheckSum_t is used to be the RESP_ID_CMD_DONE payload for the
 *command which use an indirect buffer to get the result, firmware will
 *use this kind payload to do the checksum for the buffer
 */
typedef struct _BufCheckSum_t {
	uint32 bufAddrLo; /*The low 32bit address of the indirect buffer */
	uint32 bufAddrHi; /*The high 32bit address of the indirect buffer */
	uint32 bufSize; /*The size of the indirect buffer */

	uint8 bufCheckSum; /*The byte check sum of the indirect buffer */
} BufCheckSum_t;

/*For RGBIR or pure IR case, need to configure the illuminator.
 *The illuminator clock source: REF_CLK for SOC, ISP_CLK for FPGA.
 */
typedef struct _IRIlluConfig_t {
	bool_t bEnable;
	bool_t bUseDropFrameMethod;
	SensorShutterType_t sensorShutterType; /*get from sensor driver */
	uint32 delay;	 /*in flash control clock count */
	uint32 time;		/*in flash control clock count */
} IRIlluConfig_t;

#endif


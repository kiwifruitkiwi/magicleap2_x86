/*
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#define CONFIG_ENABLE_TDB_V1_0
#define CONFIG_ENABLE_TDB_V2_0

#include "os_base_type.h"
#include "AaaStats.h"
#ifdef CONFIG_ENABLE_TDB_V2_0
#include "SensorTdb.h"
#include "IspHwPipeCalibDb.h"
#else
#include "SensorTdbCdb1p0.h"
#include "IspHwPipeCalibDbCdb1p0.h"
#endif


/* ---------------------------------------------------------- */
/*                                     Command Structure      */
/* ---------------------------------------------------------- */

//The struct _Cmd_t should be totally 64 bytes.
struct _Cmd_t {
	unsigned int cmdSeqNum;
	//The cmdSeqNum is the sequence number of host command
	//in all command channels. It should be increased 1 by
	//one command. The first command should be set to 1.
	unsigned int cmdId;	//The actual command identifier

	unsigned int cmdParam[12];
	//The command parameter. For the direct command,
	//it is defined by the command.For the indirect
	//command, it is a struct of struct _CmdParamPackage_t
	unsigned short cmdStreamId;
	// Used to identify which stream this command is
	//applied for the stream command.
	//See the enum _StreamId_t type for reference. For
	//global command, this field is ignored and
	//should be set to STREAM_ID_INVALID.
	unsigned char reserved[5];

#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	unsigned char  reserved_1[192];
#endif
	//The byte sum of all the fieldes defined in
	//struct _Cmd_t except the cmdCheckSum.
	unsigned char cmdCheckSum;
} __aligned(16);

enum _CmdParamPackageDirection_t {
	CMD_PARAM_PACKAGE_DIRECTION_INVALID     = 0,
	CMD_PARAM_PACKAGE_DIRECTION_GET         = 1,
	// Host get data from FW
	CMD_PARAM_PACKAGE_DIRECTION_SET         = 2,
	// Host set data to FW
	CMD_PARAM_PACKAGE_DIRECTION_BIDIRECTION = 3,
	// Host and FW access data both
	CMD_PARAM_PACKAGE_DIRECTION_MAX         = 4
};

struct _CmdParamPackage_t {
	unsigned int packageAddrLo;
	/* The low 32 bit address of the package address */
	unsigned int packageAddrHi;
	//The high 32 bit address of the package address
	unsigned int packageSize;
	/* The total package size in bytes. */
	unsigned char  packageCheckSum; /* The byte sum of the package */
};


/* ------------------------------------------------------------------- */
/*                                     Response Structure              */
/* ------------------------------------------------------------------- */

//The struct _Resp_t should be totally 64 bytes.
struct _Resp_t {
	unsigned int respSeqNum;
	// The respSeqNunm is the sequence number of
	//firmware response. It should be increased 1 by
	//one response. The initial number should be 1
	unsigned int respId;
	// The response identifier
	unsigned int respParam[12];
	// The payload of the response. Different respId
	// may have different payload structure
	//definition

	unsigned char  reserved[7];
#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	unsigned char  reserved_1[192];
#endif
	unsigned char  respCheckSum;
// The byte sum of all the fieldes defined in
	//struct _Resp_t except respCheckSum.
} __aligned(16);

struct _RespParamPackage_t {
	unsigned int packageAddrLo;
	//The low 32 bit address of the package address
	unsigned int packageAddrHi;
	//The high 32 bit address of the package address
	unsigned int packageSize;
	//The total package size in bytes.
	unsigned char  packageCheckSum;
	//The byte sum of the package
};

#define  MAX_LIGHT_NAME_LENGTH  (32)
enum _StreamId_t {
	STREAM_ID_INVALID = -1,
	STREAM_ID_1 = 0,
	STREAM_ID_2 = 1,
	STREAM_ID_3 = 2,
	STREAM_ID_MAX
};

enum _SensorId_t {
	SENSOR_ID_INVALID = -1,
	SENSOR_ID_A    = 0,
	SENSOR_ID_B    = 1,
	SENSOR_ID_C    = 2,
	SENSOR_ID_RDMA = 3,
	SENSOR_ID_MAX
};

enum _I2cDeviceId_t {
	I2C_DEVICE_ID_INVALID = -1,
	I2C_DEVICE_ID_A  = 0,
	I2C_DEVICE_ID_B  = 1,
	I2C_DEVICE_ID_C  = 2,
	I2C_DEVICE_ID_MAX
};


//If you add a module id here, please add it in g_modIdString of
//LogMgr.c as well.
//Otherwise, you cannot get your expect log belong to the module id.
enum _ModId_t {
	MOD_ID_ALL               = 0,
	MOD_ID_AC_MGR            = 1,
	MOD_ID_CMD_DISPATCH      = 2,
	MOD_ID_MIPI_PIPE_MGR     = 3,
	MOD_ID_MIPI_PIPE_MGR_END = 4,
	MOD_ID_ISP_PIPE_MGR      = 5,
	MOD_ID_ISP_PIPE_MGR_END  = 6,
	MOD_ID_I2C_CORE          = 7,
	MOD_ID_STREAM_MGR        = 8,
	MOD_ID_AF_MGR            = 9,
	MOD_ID_AE_MGR            = 10,
	MOD_ID_AWB_MGR           = 11,
	MOD_ID_MODULE_SETUP      = 12,
	MOD_ID_TDB_MGR           = 13,
	MOD_ID_MAIN              = 14,
	MOD_ID_FRAMEWORK         = 15,
	MOD_ID_CMD_RESP_AGENT    = 16,
	MOD_ID_OSAL_BM           = 17,
	MOD_ID_OSAL_XOS          = 18,
	MOD_ID_TASK_MGR          = 19,
	MOD_ID_RING_BUFFER       = 20,
	MOD_ID_PROFILER          = 21,
	MOD_ID_INTR_MGR          = 22,
	MOD_ID_UTL               = 23,
	MOD_ID_UTL_QUEUE         = 24,
	MOD_ID_UTL_MEM_POOL      = 25,
	MOD_ID_UTL_RING_ARRAY    = 26,
	MOD_ID_UTL_ASSERT        = 27,
	MOD_ID_HAL               = 28,
	MOD_ID_HAL_ISP_HW        = 29,
	MOD_ID_HAL_PERF_MON      = 30,
	MOD_ID_HAL_GMC           = 31,
	MOD_ID_HAL_PMB           = 32,
	MOD_ID_TIMER_SERVICE     = 33,
	MOD_ID_DEVICE_SCRIPT     = 34,
	MOD_ID_DEVICE_MGR        = 35,
	MOD_ID_IH                = 36,
	MOD_ID_LOWLEVELINTR      = 37,
	MOD_ID_LOG_MGR           = 38,
	MOD_ID_DMA               = 39,
	MOD_ID_IMC_MGR           = 40,
	MOD_ID_IMC_UTIL          = 41,
	MOD_ID_CVIP              = 42,
	MOD_ID_IMC_HW            = 43,
	MOD_ID_AAA_STATS_UTIL    = 44,
	MOD_ID_UC_ALGO           = 45,
	MOD_ID_CRISP_MGR         = 46,
	MOD_ID_LSC_MGR           = 47,
	MOD_ID_MMHUB             = 48,
	MOD_ID_ISP_STATS_MGR     = 49,
	MOD_ID_MODULE_ISP_DYNAMIC = 50,
	MOD_ID_MODULE_ISP_AUTOCONTRAST = 51,
	MOD_ID_MODULE_ISP_3DLUT  = 52,
	MOD_ID_AF_ALGO           = 53,
	MOD_ID_AE_ALGO           = 54,
	MOD_ID_IRIDIX_ALGO       = 55,
	MOD_ID_AWB_ALGO          = 56,
	MOD_ID_MODULE_ISP_AUTOCONTRAST_ALGO = 57,
	MOD_ID_MODULE_ISP_3DLUT_ALGO  = 58,
	MOD_ID_LSC_ALGO           = 59,
	MOD_ID_MAX
};

enum _LogLevel_t {
	LOG_LEVEL_DEBUG  = 0,
	LOG_LEVEL_INFO   = 1,
	LOG_LEVEL_WARN   = 2,
	LOG_LEVEL_ERROR  = 3,
	LOG_LEVEL_MAX
};

enum _SensorIntfType_t {
	SENSOR_INTF_TYPE_MIPI      = 0,
	SENSOR_INTF_TYPE_PARALLEL  = 1,
	SENSOR_INTF_TYPE_RDMA      = 2,
	SENSOR_INTF_TYPE_MAX
};


enum _PdafType_t {
	PDAF_TYPE_HALF_SHIELD_OV,
	PDAF_TYPE_HALF_SHIELD_S0NY,
	PDAF_TYPE_2X1_OV,
	PDAF_TYPE_2X1_S0NY,
	PDAF_TYPE_DUAL_PD, //Listed here but not support by ISP3.0 HW
	PDAF_TYPE_2x2_PD,  //Listed here but not support by ISP3.0 HW
	PDAF_TYPE_MAX
};

enum  _CaptureFullYuvType_t {
	CAPTURE_FULL_YUV_TYPE_INVALID,
	CAPTURE_FULL_YUV_TYPE_ONE_SHOT_ISP,
	CAPTURE_FULL_YUV_TYPE_MULTI_FRAME_ISP,
	CAPTURE_FULL_YUV_TYPE_CRISP,
};

//enum _HdrItimeType_t
//{
//    HDR_ITIME_TYPE_INVALID,
//    HDR_ITIME_TYPE_SEPARATE,  //!< The hdr itime for high exposure and low
				//exposure has separated control registers
//    HDR_ITIME_TYPE_RATIO,     //!< The hdr itime for low exposure is a
				//ratio of the high exposure
//    HDR_ITIME_TYPE_EQUAL,     //!< The hdr itime for high and low exposure
				//should be euqal.
//    HDR_ITIME_TYPE_MAX
//};
//
//enum _HdrAGainType_t
//{
//    HDR_AGAIN_TYPE_INVALID,
//    HDR_AGAIN_TYPE_SEPARATE,  //!< The hdr analog gain for high exposure and
				//low exposure has separated control registers
//    HDR_AGAIN_TYPE_RATIO,     //!< The hdr analog gain for low exposure is a
				//ratio of the high exposure
//    HDR_AGAIN_TYPE_EQUAL,     //!< The hdr analog gain for high and low
				//exposure should be euqal.
//    HDR_AGAIN_TYPE_MAX
//};
//
//enum _HdrDGainType_t
//{
//    HDR_DGAIN_TYPE_INVALID,
//    HDR_DGAIN_TYPE_SEPARATE,  //!< The hdr digital gain for high exposure and
				//low exposure has separated control registers
//    HDR_DGAIN_TYPE_RATIO,     //!< The hdr digital gain for low exposure is
				//a ratio of the high exposure
//    HDR_DGAIN_TYPE_EQUAL,     //!< The hdr digital gain for high and low
				//exposure should be euqal.
//    HDR_DGAIN_TYPE_MAX
//};
//
//enum _HdrStatDataMatrixId_t
//{
//    HDR_STAT_DATA_MATRIX_ID_INVALID = 0,
//    HDR_STAT_DATA_MATRIX_ID_16X16   = 1,
//    HDR_STAT_DATA_MATRIX_ID_8X8     = 2,
//    HDR_STAT_DATA_MATRIX_ID_4X4     = 3,
//    HDR_STAT_DATA_MATRIX_ID_1X1     = 4,
//    HDR_STAT_DATA_MATRIX_ID_MAX,
//};


//#define MAX_HDR_RATIO_NUM (16)
enum _HdrMode_t {
	HDR_MODE_KERNEL_INVALID    = 0,
	HDR_MODE_KERNEL_NO_HDR     = 1,
	HDR_MODE_KERNEL_ZIGZAG_HDR = 2,
	HDR_MODE_KERNEL_2_DOL_HDR  = 3,
	HDR_MODE_KERNEL_3_DOL_HDR  = 4,
	HDR_MODE_KERNEL_MAX        = 5,
};

enum _ResMode_t {
	RES_MODE_INVALID	= 0,
	RES_MODE_FULL		= 1,
	RES_MODE_V2_BINNING	= 2,
	RES_MODE_H2V2_BINNING	= 3, //2x2 binning
	RES_MODE_H4V4_BINNING	= 4, //4x4 binning
	RES_MODE_4K2K		= 5,
	RES_MODE_MAX
};

enum _TestPattern_t {
	TEST_PATTERN_INVALID    = 0,
	TEST_PATTERN_NO         = 1,
	TEST_PATTERN_1          = 2,
	TEST_PATTERN_2          = 3,
	TEST_PATTERN_3          = 4,
	TEST_PATTERN_MAX        = 5,
};

enum _RawBitDepth_t {
	RAW_BIT_DEPTH_INVALID   = 0,
	RAW_BIT_DEPTH_8_BIT     = 1,
	RAW_BIT_DEPTH_10_BIT    = 2,
	RAW_BIT_DEPTH_12_BIT    = 3,
	RAW_BIT_DEPTH_MAX       = 4,
};

enum _CvipBufType_t {
	CVIP_BUF_TYPE_INVALID       = 0,
	CVIP_BUF_TYPE_PREPOST_BUF   = 1,
	CVIP_BUF_TYPE_STATS_BUF     = 2,
	CVIP_BUF_TYPE_MAX
};

enum _ImageFormat_t {
	IMAGE_FORMAT_INVALID,
	IMAGE_FORMAT_NV12,
	IMAGE_FORMAT_NV21,
	IMAGE_FORMAT_I420,
	IMAGE_FORMAT_YV12,
	IMAGE_FORMAT_YUV422PLANAR,
	IMAGE_FORMAT_YUV422SEMIPLANAR,
	IMAGE_FORMAT_YUV422INTERLEAVED,
	IMAGE_FORMAT_P010,  //SemiPlanar, 4:2:0, 10-bit
	IMAGE_FORMAT_Y210,  //Packed, 4:2:2, 10-bit
	IMAGE_FORMAT_L8,    //Only Y 8-bit
	IMAGE_FORMAT_RGBBAYER8,
	IMAGE_FORMAT_RGBBAYER10,
	IMAGE_FORMAT_RGBBAYER12,
	IMAGE_FORMAT_RGBBAYER14,
	IMAGE_FORMAT_RGBBAYER16,
	IMAGE_FORMAT_RGBBAYER20,
	IMAGE_FORMAT_RGBIR8,
	IMAGE_FORMAT_RGBIR10,
	IMAGE_FORMAT_RGBIR12,
	IMAGE_FORMAT_Y210BF,  //interleave, 4:2:2, 10-bit bubble free
	IMAGE_FORMAT_MAX
};

enum _WindowType_t {
	RESIZE0_CROP_WINDOW,
	RESIZE1_CROP_WINDOW,
	RESIZE2_CROP_WINDOW,
	RESIZE3_CROP_WINDOW,
};

struct _Window_t {
	unsigned int h_offset;
	unsigned int v_offset;
	unsigned int h_size;
	unsigned int v_size;
};

#define FPS_RANGE_SCALE_FACTOR  1000
struct _Range_t {
	unsigned int min;
	unsigned int max;
};

enum _RoiType_t {
	ROI_TYPE_INVALID   = 0,
	ROI_TYPE_NORMAL     = 1,
	ROI_TYPE_FACE       = 2,
	ROI_TYPE_MAX        = 3
};

#define ROI_WEIGHT_SCALE_FACTOR 1000
struct _Roi_t {
	enum _RoiType_t type;
	struct _Window_t  window;
	unsigned int    weight; // Range is from 0 ~ 1000
};

/*table define for mipi bayer scaler*/
#define BRSZ_TBL_SIZE (128)
struct _BrszTable_t {
	short coeff[BRSZ_TBL_SIZE];
};



enum _BlsWindType_t {
	BLS_WIND_TYPE_INVALID = 0,
	BLS_WIND_TYPE_ONE_WIND, //Config only one bls window
	BLS_WIND_TYPE_TWO_WIND, //Config two bls window
	BLS_WIND_TYPE_MAX,
};

enum _ProfilerId_t {
	PROFILER_ID_INVALID = -1,

	PROFILER_ID_PIPE_RT_START,
	PROFILER_ID_PIPE_RT_RESETUP_FRAME_DEVICE,
	PROFILER_ID_CMD_ENABLE_CALIB,
	PROFILER_ID_CMD_SET_SENSOR_CALIBDB,
	PROFILER_ID_CMD_START_STREAM,
	//every frame
	PROFILER_ID_ISP_PIPE_CFG_ONE_FRAME,
	PROFILER_ID_ISP_PIPE_TRIG_ONE_FRAME,
	PROFILER_ID_ISP_HW_PROCESS_ONE_FRAME,
	PROFILER_ID_DUMP_STATS,
	PROFILER_ID_AE_ALG_RUN,
	PROFILER_ID_AWB_ALG_RUN,
	PROFILER_ID_3DLUT_ALG_RUN,
	PROFILER_ID_LSC_ALG_RUN,

	//comment
	//PROFILER_ID_CMD_DISPATCH_GLB_EXCEPT_CALIB,
	//PROFILER_ID_CMD_DISPATCH_STR_EXCEPT_CALIB,
	//PROFILER_ID_I2C_READ,
	//PROFILER_ID_I2C_WRITE,
	//PROFILER_ID_CMD_DISPATCH_FLUSH,
	//PROFILER_ID_CMD_ENABLE_CHANNEL,
	//PROFILER_ID_RESP_SEND_NON_PACKAGE_RESP,
	//PROFILER_ID_RESP_SEND_PACKAGE_RESP,
	//PROFILER_ID_CMD_SEND_BUFFER,

	//PROFILER_ID_SCHEDULER_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_CMD_DISPATCH_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_PIPE_RT_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_PIPE_CORE_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_AF_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_AE_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_AWB_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_I2C_HAS_PENDING_TASK,
	//PROFILER_ID_SCHEDULER_HEART_BEAT_HAS_PENDING_TASK,
	//PROFILER_ID_AWB_SCHEDULE,
	//PROFILER_ID_AWB_WP_REVERT,
	//PROFILER_ID_AWB_ILLU_EST,
	//PROFILER_ID_AWB_WB_GAIN,
	//PROFILER_ID_AWB_ACC,
	//PROFILER_ID_AWB_ALSC,
	//PROFILER_ID_AWB_PUSH_AWB_CTRL_INFO,
	//PROFILER_ID_AWB_PUSH_AWBM_INFO,
	//PROFILER_ID_AWB_GET_CTRL_INFO,
	//PROFILER_ID_AE_SCHEDULE,
	//PROFILER_ID_AE_ALG_RUN,
	//PROFILER_ID_AE_ALG_DONE,
	//PROFILER_ID_AE_PUSH_AEM_INFO,
	//PROFILER_ID_AE_GET_CTRL_INFO,
	//PROFILER_ID_AF_SCHEDULE,
	//PROFILER_ID_AF_PROCESS_ONE_FRAME,
	//PROFILER_ID_AF_SET_AFM_INFO,
	//PROFILER_ID_AF_GET_CTRL_INFO,
	//PROFILER_ID_PIPE_RT_SCHEDULE,
	//PROFILER_ID_PIPE_RT_RESETUP_FRAME,
	//PROFILER_ID_PIPE_RT_SEND_FRAMEINFO,
	//PROFILER_ID_PIPE_CORE_SCHEDULE,
	//PROFILER_ID_PIPE_CORE_GET_CFG_INFO,
	//PROFILER_ID_PIPE_CORE_GET_CTRL_INFO,
	//PROFILER_ID_PIPE_CORE_SEND_FRAME_DONE,
	//PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME,
	//PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME_PREP,
	//PROFILER_ID_PIPE_CORE_PROCESS_ONE_FRAME_ISP,
	//PROFILER_ID_PIPE_CORE_MODE2_INIT_CORE_FRAMEINFO,
	//PROFILER_ID_PIPE_CORE_MODE3_INIT_CORE_FRAMEINFO,
	//PROFILER_ID_PIPE_CORE_MODE4_INIT_CORE_FRAMEINFO,
	//PROFILER_ID_PIPE_CORE_PREP_LSC_TABLE_CONFIG,
	//PROFILER_ID_PIPE_CORE_LSC_TABLE_CONFIG,
	//PROFILER_ID_I2C_CORE_SCHEDULE,
	//PROFILER_ID_CMD_DISPATCH_SCHEDULE,
	//PROFILER_ID_CMD_GET_CMD_FROM_HOST,
	//PROFILER_ID_CMD_SET_OUT_PROP,
	//PROFILER_ID_RESP_SEND_CMD_DONE_RESP,
	//PROFILER_ID_INTR_ISP_KERNEL_MI,
	//PROFILER_ID_INTR_RING,
	//PROFILER_ID_INTR_PREP_STNR,
	//PROFILER_ID_INTR_MIPI_PIPE0,
	//PROFILER_ID_INTR_MIPI_PIPE1,
	//PROFILER_ID_INTR_I2C,
	//PROFILER_ID_INTR_FLASH0,
	//PROFILER_ID_INTR_FLASH1,
	//PROFILER_ID_ONE_LOOPBACK_FRAME_PROCESS,
	//PROFILER_ID_HEART_BEAT_SCHEDULE,
	//PROFILER_ID_HW_CORE_PROCESS_ONE_FRAME,
	//PROFILER_ID_PROCESS_ONE_FRAME,
	PROFILER_ID_MAX
};



enum _CcpuBar_t {
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
};


enum _CcpuReqType_t {
	CCPU_REQ_TYPE_INVALID = -1,
	CCPU_REQ_TYPE_RD,
	CCPU_REQ_TYPE_WR,
	CCPU_REQ_TYPE_MAX
};

enum _MiPfmClient_t {
	MI_PFM_CLIENT_INVALID = -1,
	MI_PFM_CLIENT_CCPU,
	MI_PFM_CLIENT_MIPI0,
	MI_PFM_CLIENT_MIPI1,
	MI_PFM_CLIENT_MIPI2,
	MI_PFM_CLIENT_MAX
};


enum _LatencyRegion_t {
	LATENCY_REGION_INVALID = -1,
	LATENCY_REGION_0,
	LATENCY_REGION_1,
	LATENCY_REGION_2,
	LATENCY_REGION_3,
	LATENCY_REGION_4,
	LATENCY_REGION_5,
	LATENCY_REGION_6,
	LATENCY_REGION_7,
	LATENCY_REGION_MAX
};

enum _DeviceControlMode_t {
	DEVICE_CONTROL_MODE_INVALID = -1,
	DEVICE_CONTROL_MODE_CVP = 0,  //device controlled by cvp
	DEVICE_CONTROL_MODE_SCRIPT,   //device controlled by svm script in fw
	DEVICE_CONTROL_MODE_HOST,     //device controlled by host
	DEVICE_CONTROL_MODE_MAX
};

struct _ProfilerCpu_t {
	unsigned int accessTimes;
	unsigned long long totalCount;
	unsigned long long minCount;
	unsigned long long maxCount;

	// CPU cycles.
	unsigned long long totalCpuCycleCount;
	unsigned long long lastCpuCycleCount;
	unsigned long long minCpuCycleCount;
	unsigned long long maxCpuCycleCount;

	// I-Cache miss.
	unsigned long long totalIMissTimes;
	unsigned long long lastIMissTimes;
	unsigned long long minIMissTimes;
	unsigned long long maxIMissTimes;

	// I-Cache hit.
	unsigned long long totalIHitTimes;
	unsigned long long lastIHitTimes;
	unsigned long long minIHitTimes;
	unsigned long long maxIHitTimes;

	// D-Cache miss.
	unsigned long long totalDMissTimes;
	unsigned long long lastDMissTimes;
	unsigned long long minDMissTimes;
	unsigned long long maxDMissTimes;

	// D-Cache hit.
	unsigned long long totalDHitTimes;
	unsigned long long lastDHitTimes;
	unsigned long long minDHitTimes;
	unsigned long long maxDHitTimes;
};

struct _ProfilerResult_t {
	struct _ProfilerCpu_t profilerCpu[PROFILER_ID_MAX];
	// DMA PFM profiling
	unsigned int pfmRegion[MI_PFM_CLIENT_MAX][LATENCY_REGION_MAX];
	unsigned int pfmCnt[MI_PFM_CLIENT_MAX][LATENCY_REGION_MAX];
};

struct _ProfilerIdResult_t {
	//Function cycles profiling
	unsigned int accessTimes;
	//Ccpu Bar profiling //3(code, stack and heap)
	unsigned int ccpuBarTotal[3][CCPU_REQ_TYPE_MAX];
};

#define TASK_NAME_MAX_LENGTH (16)
struct _OneTaskStats_t {
	char   name[TASK_NAME_MAX_LENGTH]; // Task name.
	// CPU use % for this task from the last time called
	// TaskMgrCmdGetCpuLoad().
	unsigned int cpuPercent;
	unsigned long long cycleCount;
	// Number of cycles consumed since boot up.
	// Task priority. < 0: invalid; the higher the number, the lower
	// the priority.
	int  priority;
	// Max stack usage size in bytes. 0 means the stack usage statistics
	// not enabled.
	unsigned int stackUsageMax;
};

#define TASK_NUM_MAX (20)
struct _AllTaskStats_t {
	struct _OneTaskStats_t TaskStats[TASK_NUM_MAX];
};

enum _ErrorLevel_t {
	ERROR_LEVEL_INVALID,
	ERROR_LEVEL_FATAL,  /* The error has caused the stream stopped */
	ERROR_LEVEL_WARN,/* Firmware has automatically restarted the stream */
	/* Should take notice of this error which may lead some other error */
	ERROR_LEVEL_NOTICE,
	ERROR_LEVEL_MAX
};


#define ERROR_CODE1_MIPI_ERR_CS                   (1 << 0)
#define ERROR_CODE1_MIPI_ERR_ECC1                 (1 << 1)
#define ERROR_CODE1_MIPI_ERR_ECC2                 (1 << 2)
#define ERROR_CODE1_MIPI_ERR_PROTOCOL             (1 << 3)
#define ERROR_CODE1_MIPI_ERR_CONTROL              (1 << 4)
#define ERROR_CODE1_MIPI_ERR_EOT_SYNC             (1 << 5)
#define ERROR_CODE1_MIPI_ERR_SOT_SYNC             (1 << 6)
#define ERROR_CODE1_MIPI_ERR_SOT                  (1 << 7)
#define ERROR_CODE1_MIPI_SYNC_FIFO_OVFLW          (1 << 8)
#define ERROR_CODE1_MIPI_FORM0_INFORM_SIZE_ERR    (1 << 9)
#define ERROR_CODE1_MIPI_FORM0_OUTFORM_SIZE_ERR   (1 << 10)
#define ERROR_CODE1_MIPI_FORM1_INFORM_SIZE_ERR    (1 << 11)
#define ERROR_CODE1_MIPI_FORM1_OUTFORM_SIZE_ERR   (1 << 12)
#define ERROR_CODE1_MIPI_FORM2_INFORM_SIZE_ERR    (1 << 13)
#define ERROR_CODE1_MIPI_FORM2_OUTFORM_SIZE_ERR   (1 << 14)
#define ERROR_CODE1_MIPI_WRAP_Y                   (1 << 15)
#define ERROR_CODE1_MIPI_WRAP_CB                  (1 << 16)
#define ERROR_CODE1_MIPI_WRAP_CR                  (1 << 17)
#define ERROR_CODE1_MIPI_MAF                      (1 << 18)
#define ERROR_CODE1_MIPI_AXI_TMOUT                (1 << 19)
#define ERROR_CODE1_MIPI_FORM3_INFORM_SIZE_ERR    (1 << 20)
#define ERROR_CODE1_MIPI_FORM3_OUTFORM_SIZE_ERR   (1 << 21)

#define ERROR_CODE2_ISP_AFM_LUM_OF                (1 << 0)
#define ERROR_CODE2_ISP_AFM_SUM_OF                (1 << 1)
#define ERROR_CODE2_ISP_PIC_SIZE_ERR              (1 << 2)
#define ERROR_CODE2_ISP_DATA_LOSS                 (1 << 3)
#define ERROR_CODE2_MI_MP_WRAP_Y                  (1 << 4)
#define ERROR_CODE2_MI_MP_WRAP_CB                 (1 << 5)
#define ERROR_CODE2_MI_MP_WRAP_CR                 (1 << 6)
#define ERROR_CODE2_MI_MP_MAF                     (1 << 7)
#define ERROR_CODE2_MI_MP_AXI_TMOUT               (1 << 8)
#define ERROR_CODE2_MI_VP_WRAP_Y                  (1 << 9)
#define ERROR_CODE2_MI_VP_WRAP_CB                 (1 << 10)
#define ERROR_CODE2_MI_VP_WRAP_CR                 (1 << 11)
#define ERROR_CODE2_MI_VP_MAF                     (1 << 12)
#define ERROR_CODE2_MI_VP_AXI_TMOUT               (1 << 13)
#define ERROR_CODE2_MI_PP_WRAP_Y                  (1 << 14)
#define ERROR_CODE2_MI_PP_WRAP_CB                 (1 << 15)
#define ERROR_CODE2_MI_PP_WRAP_CR                 (1 << 16)
#define ERROR_CODE2_MI_PP_MAF                     (1 << 17)
#define ERROR_CODE2_MI_PP_AXI_TMOUT               (1 << 18)
#define ERROR_CODE2_MI_RD_MAF                     (1 << 19)
#define ERROR_CODE2_MI_RD_AXI_TMOUT               (1 << 20)

#define ERROR_CODE3_PREP_INFORM_SIZE_ERR          (1 << 0)
#define ERROR_CODE3_PREP_OUTFORM_SIZE_ERR         (1 << 1)
#define ERROR_CODE3_STNR_TNR_HW_SETA              (1 << 2)
#define ERROR_CODE3_STNR_TNR_AUTO_REC             (1 << 3)
#define ERROR_CODE3_STNR_RDMA_RNACK1_ERR          (1 << 4)
#define ERROR_CODE3_STNR_RDMA_RNACK2_ERR          (1 << 5)
#define ERROR_CODE3_STNR_RDMA_RNACK3_ERR          (1 << 6)
#define ERROR_CODE3_STNR_RDMA_RRESP_ERR           (1 << 7)
#define ERROR_CODE3_STNR_WDMA_RNACK1_ERR          (1 << 8)
#define ERROR_CODE3_STNR_WDMA_RNACK2_ERR          (1 << 9)
#define ERROR_CODE3_STNR_WDMA_RNACK3_ERR          (1 << 10)
#define ERROR_CODE3_STNR_WDMA_RRESP_ERR           (1 << 11)

#define ERROR_CODE4_AE_CALC_FAIL                  (1 << 0)
#define ERROR_CODE4_AF_CALC_FAIL                  (1 << 1)
#define ERROR_CODE4_AWB_CALC_FAIL                 (1 << 2)
#define ERROR_CODE4_MIPI_FRAME_TMOUT              (1 << 3)
#define ERROR_CODE4_CORE_FRAME_TMOUT              (1 << 4)
#define ERROR_CODE4_CORE_FRAME_TMOUT_SLICE_MISS   (1 << 5)
#define ERROR_CODE4_CORE_FRAME_TMOUT_DMA_QOS      (1 << 6)
#define ERROR_CODE4_MIPI_PROCESS_FRAME_FAIL       (1 << 7)
#define ERROR_CODE4_STREAM_CTRL_LEAK              (1 << 8)
#define ERROR_CODE4_CORE_FRAME_CVIP_DROP          (1 << 9)
#define ERROR_CODE4_MIPI_START_FAIL               (1 << 10)
#define ERROR_CODE4_CORE_FRAME_IMC_DROP           (1 << 11)
#define ERROR_CODE4_IMC_INBUF_FIFO_OVERFLOW       (1 << 12)

/**
 * @id ERROR_CODE5_I2C_7B_ADDR_NOACK
 * Master is in 7-bit address mode and the address sentwas not
 * acknowledged by any slave
 */
#define ERROR_CODE5_I2C_7B_ADDR_NOACK             (1 << 0)
/**
 * @id ERROR_CODE5_I2C_10ADDR1_NOACK
 * Master is in 10-bit address mode and the first 10-bit address
 * byte was not acknowledged by any slave
 */
#define ERROR_CODE5_I2C_10ADDR1_NOACK             (1 << 1)
/**
 * @id ERROR_CODE5_I2C_10ADDR2_NOACK
 * Master is in 10-bit address mode and the second address byte of
 * the 10-bit address was not acknowledged by any slave
 */
#define ERROR_CODE5_I2C_10ADDR2_NOACK             (1 << 2)
/**
 * @id ERROR_CODE5_I2C_TXDATA_NOACK
 * Master has received an acknowledgment for the address, but when
 * it set data bytes(s) following the address, it did not receive an
 * acknowledge from the remote slave(s).
 */
#define ERROR_CODE5_I2C_TXDATA_NOACK              (1 << 3)
/**
 * @id ERROR_CODE5_I2C_GCALL_NOACK
 * Send a General Call and no slave on the bus acknowledged the General
 * Call.
 */
#define ERROR_CODE5_I2C_GCALL_NOACK               (1 << 4)
/**
 * @id  ERROR_CODE5_I2C_GCALL_READ
 * Send a General Call but the user programmed the byte following the
 * General Call to be a read from the bus (IC_DATA_CMD[9] is set to 1).
 */
#define ERROR_CODE5_I2C_GCALL_READ                (1 << 5)
/**
 * @id ERROR_CODE5_I2C_HS_ACKDET
 * Master is in High Speed mode and the High Speed Master code was
 * acknowledged (wrong behavior).
 */
#define ERROR_CODE5_I2C_HS_ACKDET                 (1 << 6)
/**
 * @id ERROR_CODE5_I2C_SBYTE_ACKDET
 * Master has sent a START Byte and the START Byte was acknowledged
 * (wrong behavior).
 */
#define ERROR_CODE5_I2C_SBYTE_ACKDET              (1 << 7)
/**
 * @id ERROR_CODE5_I2C_HS_NORSTRT
 * The restart is disabled (IC_RESTART_EN bit (IC_CON[5]) = 0) and the
 * user is trying to use the master to transfer data in High Speed mode.
 */
#define ERROR_CODE5_I2C_HS_NORSTRT                (1 << 8)
/**
 * @id ERROR_CODE5_I2C_SBYTE_NORSTRT
 * The restart is disabled (IC_RESTART_EN) bit (IC_CON[5]) = 0) and the
 * user is trying to send a START Byte.
 */
#define ERROR_CODE5_I2C_SBYTE_NORSTRT             (1 << 9)
/**
 * @id ERROR_CODE5_I2C_10B_RD_NORSTRT
 * The restart is disabled (IC_RESTART_EN bit (IC_CON[5] = 0) and the master
 * sends a read command in 10-bit addressing mode.
 */
#define ERROR_CODE5_I2C_10B_RD_NORSTRT            (1 << 10)
/**
 * @id ERROR_CODE5_I2C_MASTER_DIS
 * User tries to initiate a Master operation with the Master mode disabled.
 */
#define ERROR_CODE5_I2C_MASTER_DIS                (1 << 11)
/**
 * @id ERROR_CODE5_I2C_ARB_LOST
 * Master has lost arbitration, or if IC_TX_ABRT_SOURCE[14] is also set, then
 * the slave transmitter has lost arbitration.
 * Note: I2C can be both master and slave at the same time.
 */
#define ERROR_CODE5_I2C_ARB_LOST                  (1 << 12)
/**
 * @id ERROR_CODE5_I2C_SLVFLUSH_TXFIFO
 * Slave has received a read command and some data exists in the TX FIFO so the
 * slave issue a TX_ABRT interrupt to flush old data in TX FIFO.
 */
#define ERROR_CODE5_I2C_SLVFLUSH_TXFIFO           (1 << 13)
/**
 * @id ERROR_CODE5_I2C_SLV_ARBLOST
 * Slave lost the bus while transmitting data to a remote master.
 * IC_TX_ABRT_SOURCE[12] is set at the same time.
 * Note: Even though the slave never "owns" the bus, something could go wrong on
 * the bus. This is a fail safe check. For instance, during a data transmissioin
 * at the low-to-high transition of SCL, if what is on the data bus is not what
 * is supposed to be transmitted, then I2C controller no longer own the bus.
 */
#define ERROR_CODE5_I2C_SLV_ARBLOST               (1 << 14)
/**
 * @id ERROR_CODE5_I2C_SLVRD_INTX
 * When the processor side responds to a slave mode request for data to
 * be transmitted
 * to a remote master and user writes a 1 in CMD(bit8) of IC_DATA_CMD register.
 */
#define ERROR_CODE5_I2C_SLVRD_INTX                (1 << 15)

/**
 * @id ERROR_CODE5_SVM_INVALID_INSTR
 * An invalid instruction exists in the svm script.
 */
#define ERROR_CODE5_SVM_INVALID_INSTR             (1 << 16)



struct _ErrorCode_t {
	unsigned int code1;  /* See ERROR_CODE1_XXX for reference */
	unsigned int code2;  /* See ERROR_CODE2_XXX for reference */
	unsigned int code3;  /* See ERROR_CODE3_XXX for reference */
	unsigned int code4;  /* See ERROR_CODE4_XXX for reference */
	unsigned int code5;  /* See ERROR_CODE5_XXX for reference */
};



enum _MipiVirtualChannel_t {
	MIPI_VIRTUAL_CHANNEL_0 = 0x0, //!< virtual channel 0
	MIPI_VIRTUAL_CHANNEL_1 = 0x1, //!< virtual channel 1
	MIPI_VIRTUAL_CHANNEL_2 = 0x2, //!< virtual channel 2
	MIPI_VIRTUAL_CHANNEL_3 = 0x3, //!< virtual channel 3
	MIPI_VIRTUAL_CHANNEL_MAX
};



enum _MipiDataType_t {
	MIPI_DATA_TYPE_FSC              = 0x00, //!< frame start code
	MIPI_DATA_TYPE_FEC              = 0x01, //!< frame end code
	MIPI_DATA_TYPE_LSC              = 0x02, //!< line start code
	MIPI_DATA_TYPE_LEC              = 0x03, //!< line end code

	// 0x04 .. 0x07 reserved

	MIPI_DATA_TYPE_GSPC1            = 0x08, //!< generic short packet code 1
	MIPI_DATA_TYPE_GSPC2            = 0x09, //!< generic short packet code 2
	MIPI_DATA_TYPE_GSPC3            = 0x0A, //!< generic short packet code 3
	MIPI_DATA_TYPE_GSPC4            = 0x0B, //!< generic short packet code 4
	MIPI_DATA_TYPE_GSPC5            = 0x0C, //!< generic short packet code 5
	MIPI_DATA_TYPE_GSPC6            = 0x0D, //!< generic short packet code 6
	MIPI_DATA_TYPE_GSPC7            = 0x0E, //!< generic short packet code 7
	MIPI_DATA_TYPE_GSPC8            = 0x0F, //!< generic short packet code 8


	MIPI_DATA_TYPE_NULL             = 0x10, //!< null
	MIPI_DATA_TYPE_BLANKING         = 0x11, //!< blanking data
	//!< embedded 8-bit non image data
	MIPI_DATA_TYPE_EMBEDDED         = 0x12,

	// 0x13 .. 0x17 reserved

	MIPI_DATA_TYPE_YUV420_8         = 0x18, //!< YUV 420 8-Bit
	MIPI_DATA_TYPE_YUV420_10        = 0x19, //!< YUV 420 10-Bit
	MIPI_DATA_TYPE_LEGACY_YUV420_8  = 0x1A, //!< YUV 420 8-Bit
	//   0x1B reserved
	//!< YUV 420 8-Bit (chroma shifted pixel sampling)
	MIPI_DATA_TYPE_YUV420_8_CSPS    = 0x1C,
	//!< YUV 420 10-Bit (chroma shifted pixel sampling)
	MIPI_DATA_TYPE_YUV420_10_CSPS   = 0x1D,
	MIPI_DATA_TYPE_YUV422_8         = 0x1E, //!< YUV 422 8-Bit
	MIPI_DATA_TYPE_YUV422_10        = 0x1F, //!< YUV 422 10-Bit

	MIPI_DATA_TYPE_RGB444           = 0x20, //!< RGB444
	MIPI_DATA_TYPE_RGB555           = 0x21, //!< RGB555
	MIPI_DATA_TYPE_RGB565           = 0x22, //!< RGB565
	MIPI_DATA_TYPE_RGB666           = 0x23, //!< RGB666
	MIPI_DATA_TYPE_RGB888           = 0x24, //!< RGB888

	//   0x25 .. 0x27 reserved

	MIPI_DATA_TYPE_RAW_6            = 0x28, //!< RAW6
	MIPI_DATA_TYPE_RAW_7            = 0x29, //!< RAW7
	MIPI_DATA_TYPE_RAW_8            = 0x2A, //!< RAW8
	MIPI_DATA_TYPE_RAW_10           = 0x2B, //!< RAW10
	MIPI_DATA_TYPE_RAW_12           = 0x2C, //!< RAW12
	MIPI_DATA_TYPE_RAW_14           = 0x2D, //!< RAW14

	//   0x2E .. 0x2F reserved

	MIPI_DATA_TYPE_USER_1           = 0x30, //!< user defined 1
	MIPI_DATA_TYPE_USER_2           = 0x31, //!< user defined 2
	MIPI_DATA_TYPE_USER_3           = 0x32, //!< user defined 3
	MIPI_DATA_TYPE_USER_4           = 0x33, //!< user defined 4
	MIPI_DATA_TYPE_USER_5           = 0x34, //!< user defined 5
	MIPI_DATA_TYPE_USER_6           = 0x35, //!< user defined 6
	MIPI_DATA_TYPE_USER_7           = 0x36, //!< user defined 7
	MIPI_DATA_TYPE_USER_8           = 0x37, //!< user defined 8
	MIPI_DATA_TYPE_MAX
};


enum _MipiCompScheme_t {
	MIPI_COMP_SCHEME_NONE    = 0,   //!< NONE
	MIPI_COMP_SCHEME_12_8_12 = 1,   //!< 12_8_12
	MIPI_COMP_SCHEME_12_7_12 = 2,   //!< 12_7_12
	MIPI_COMP_SCHEME_12_6_12 = 3,   //!< 12_6_12
	MIPI_COMP_SCHEME_10_8_10 = 4,   //!< 10_8_10
	MIPI_COMP_SCHEME_10_7_10 = 5,   //!< 10_7_10
	MIPI_COMP_SCHEME_10_6_10 = 6,   //!< 10_6_10
	MIPI_COMP_SCHEME_MAX
};


enum _MipiPredBlock_t {
	MIPI_PRED_BLOCK_INVALID = 0,   //!< invalid
	MIPI_PRED_BLOCK_1       = 1,   //!< Predictor1 (simple algorithm)
	MIPI_PRED_BLOCK_2       = 2,   //!< Predictor2 (more complex algorithm)
	MIPI_PRED_BLOCK_MAX
};


//Color filter array pattern
enum _CFAPattern_t {
	CFA_PATTERN_INVALID      = 0,
	CFA_PATTERN_RGGB         = 1,
	CFA_PATTERN_GRBG         = 2,
	CFA_PATTERN_GBRG         = 3,
	CFA_PATTERN_BGGR         = 4,
	CFA_PATTERN_PURE_IR     =  5,
	CFA_PATTERN_RIGB         = 6,
	CFA_PATTERN_RGIB         = 7,
	CFA_PATTERN_IRBG         = 8,
	CFA_PATTERN_GRBI         = 9,
	CFA_PATTERN_IBRG         = 10,
	CFA_PATTERN_GBRI         = 11,
	CFA_PATTERN_BIGR         = 12,
	CFA_PATTERN_BGIR         = 13,
	CFA_PATTERN_BGRGGIGI     = 14,
	CFA_PATTERN_RGBGGIGI     = 15,
	CFA_PATTERN_MAX
};


//Deprecated, use CFAPattern_t instead
enum _BayerPattern_t {
	BAYER_PATTERN_INVALID      = 0,
	BAYER_PATTERN_RGGB         = CFA_PATTERN_RGGB,
	BAYER_PATTERN_GRBG         = CFA_PATTERN_GRBG,
	BAYER_PATTERN_GBRG         = CFA_PATTERN_GBRG,
	BAYER_PATTERN_BGGR         = CFA_PATTERN_BGGR,
	BAYER_PATTERN_MAX
};


struct _MipiIntfProp_t {
	unsigned char numLanes;
	enum _MipiVirtualChannel_t virtChannel;
	enum _MipiDataType_t dataType;
	enum _MipiCompScheme_t compScheme;
	enum _MipiPredBlock_t predBlock;
};


enum _ParallelDataType_t {
	PARALLEL_DATA_TYPE_INVALID       = 0,
	PARALLEL_DATA_TYPE_RAW8          = 1,
	PARALLEL_DATA_TYPE_RAW10         = 2,
	PARALLEL_DATA_TYPE_RAW12         = 3,
	PARALLEL_DATA_TYPE_YUV420_8BIT   = 4,
	PARALLEL_DATA_TYPE_YUV420_10BIT  = 5,
	PARALLEL_DATA_TYPE_YUV422_8BIT   = 6,
	PARALLEL_DATA_TYPE_YUV422_10BIT  = 7,
	PARALLEL_DATA_TYPE_MAX
};


enum _Polarity_t {
	POLARITY_INVALID      = 0,
	POLARITY_HIGH         = 1,
	POLARITY_LOW          = 2,
	POLARITY_MAX
};


enum _SampleEdge_t {
	SAMPLE_EDGE_INVALID       = 0,
	SAMPLE_EDGE_NEG           = 1,
	SAMPLE_EDGE_POS           = 2,
	SAMPLE_EDGE_MAX
};


enum _YCSequence_t {
	YCSEQUENCE_INVALID      = 0,
	YCSEQUENCE_YCBYCR       = 1,
	YCSEQUENCE_YCRYCB       = 2,
	YCSEQUENCE_CBYCRY       = 3,
	YCSEQUENCE_CRYCBY       = 4,
	YCSEQUENCE_MAX
};


enum _ColorSubSampling_t {
	COLOR_SUB_SAMPLING_INVALID             = 0,
	COLOR_SUB_SAMPLING_CONV422_COSITED     = 1,
	COLOR_SUB_SAMPLING_CONV422_INTER       = 2,
	COLOR_SUB_SAMPLING_CONV422_NOCOSITED   = 3,
	COLOR_SUB_SAMPLING_MAX
};


struct _ParallelIntfProp_t {
	enum _ParallelDataType_t dataType;
	enum _Polarity_t hPol;
	enum _Polarity_t vPol;
	enum _SampleEdge_t edge;
	enum _YCSequence_t ycSeq;
	enum _ColorSubSampling_t subSampling;
};


enum _RDmaDataType_t {
	RDMA_DATA_TYPE_INVALID       = 0,
	RDMA_DATA_TYPE_RAW8          = 1,
	RDMA_DATA_TYPE_RAW10         = 2,
	RDMA_DATA_TYPE_RAW12         = 3,
	RDMA_DATA_TYPE_MAX
};


struct _RDmaIntfProp_t {
	enum _RDmaDataType_t dataType;
};

enum _MipiPathOutType_t {
	MIPI_PATH_OUT_TYPE_INVALID          = -1,
	MIPI_PATH_OUT_TYPE_DMABUF_TO_ISP    = 0, //to stage 2 input buffer queue
	MIPI_PATH_OUT_TYPE_DMABUF_TO_HOST   = 1,    //to host
	MIPI_PATH_OUT_TYPE_DIRECT_TO_ISP    = 2,    //online process
	MIPI_PATH_OUT_TYPE_MAX
};

struct _MipiPipePathCfg_t {
	int              bEnable;    //If disabled, the RAW image only can be
					// from host
	int              bCvpCase;   //CVP handle the sensor input, and will
					//put RAW into TMR buffer
	enum _SensorId_t          sensorId;
	enum _MipiPathOutType_t   mipiPathOutType;
};

enum _IspPipeOutCh_t {
	ISP_PIPE_OUT_CH_PREVIEW = 0,
	ISP_PIPE_OUT_CH_VIDEO,
	ISP_PIPE_OUT_CH_STILL,
	ISP_PIPE_OUT_CH_CVP_CV,
	ISP_PIPE_OUT_CH_X86_CV,
	ISP_PIPE_OUT_CH_IR, //X86 or CVP
	ISP_PIPE_OUT_CH_RAW,
	ISP_PIPE_OUT_CH_FULL_STILL,
	ISP_PIPE_OUT_CH_FULL_RAW,
	ISP_PIPE_OUT_CH_MAX,
};

struct _IspPipePathCfg_t {
	unsigned int bEnablePreview:1;//Preview
	unsigned int bEnableVideo:1;//Video
	unsigned int bEnableStill:1;
	//Still , still and video cannot supported
	//simultaneously for mero
	unsigned int bEnableCvpCv:1;//CVP CV
	unsigned int bEnableX86Cv:1;//X86 CV
	unsigned int bEnableIr:1;//Ir
	unsigned int bEnableRaw:1;
	//ISP pipe raw output, only for Mero.
	//For VG, raw output is from mipi pipe
	unsigned int bEnableFullStill:1;//Full Still
	unsigned int bEnableFullRaw:1;//Full Raw
	unsigned int bReserve1:7;
	unsigned int bReserve2:8;
	unsigned int bReserve3:8;
};

struct _StreamPathCfg_t {
	struct _MipiPipePathCfg_t mipiPipePathCfg; //Cvp mipi or Isp mipi
	struct _IspPipePathCfg_t  ispPipePathCfg;
};

enum _SensorShutterType_t {
	SENSOR_SHUTTER_TYPE_GLOBAL,
	SENSOR_SHUTTER_TYPE_ROLLING,
	SENSOR_SHUTTER_TYPE_MAX
};

//enum _PdafType_t
//{
//    PDAF_TYPE_HALF_SHIELD_OV,
//    PDAF_TYPE_HALF_SHIELD_S0NY,
//    PDAF_TYPE_2X1_OV,
//    PDAF_TYPE_2X1_S0NY,
//    PDAF_TYPE_DUAL_PD, //Listed here but not support by ISP3.0 HW
//    PDAF_TYPE_2x2_PD,  //Listed here but not support by ISP3.0 HW
//    PDAF_TYPE_MAX
//};
//

/*PD Extract */
struct  _MipiFormPdExtractConfig_t {
	struct _Window_t            pdWindow;
	/*pd roi */
	unsigned char               pdUnitX;
	/*Hsize of pd unit*/
	unsigned char               pdUnitY;
	/*Vsize of pd unit*/
	unsigned char               pdNum;
	/*all pd pixels in a pd pattern*/
	unsigned char               pdNumH;
	/*pd pixels in a pd line*/
	unsigned char               pdPixelIdx;
	/*index of PD pixel table*/
	/*read write data to/from the pd pixel table*/
	unsigned int              pdPixelData;
	unsigned char               xPdlIdx[128];
	unsigned char               yPdlIdx[128];
};

/** @struct _MipiFormPdDataConfig_t
 * Sensor PD data property
 */
struct  _MipiFormPdDataConfig_t {
	enum _MipiVirtualChannel_t  virtChannel;    ///< vitural channel
	enum _MipiDataType_t        dataType;       ///< datatype
	struct _Window_t              pdDataWindow;   ///< pd data window
};

struct _SensorPdProp_t {
	struct _MipiFormPdExtractConfig_t pdConfig;    ///< PD extract config
	struct  _MipiFormPdDataConfig_t pdDataConfig;     ///< PD data config
};

enum _SensorAePropType_t {
	SENSOR_AE_PROP_TYPE_INVALID =  0,
	// Analog gain formula: weight1 / (wiehgt2 - param1)
	SENSOR_AE_PROP_TYPE_S0NY    =  1,
	// Analog gain formula: (param1 / weight1) << shift
	// Analog gain formula: gain = (param / weight1) << shift
	SENSOR_AE_PROP_TYPE_OV      =  2,
	// AE use script to adjust expo/gain settings
	SENSOR_AE_PROP_TYPE_SCRIPT  =  3,
	SENSOR_AE_PROP_TYPE_MAX
};

struct _SensorAeGainFormula_t {
	unsigned int weight1;     // constant a
	unsigned int weight2;     // constant b
	unsigned int minShift;    // minimum S
	unsigned int maxShift;    // maximum S
	unsigned int minParam;    // minimum X
	unsigned int maxParam;    // maximum X
};

struct _SensorAeProp_t {
	// Sensor property related
	// Sensor property for Analog gain calculation
	enum _SensorAePropType_t type;
	// minimum exposure line
	unsigned int minExpoLine;
	// maximum exposure line
	unsigned int maxExpoLine;
	// exposure line alpha for correct frame rate,
	// i.e. expo line < frame line - alpha
	unsigned int expoLineAlpha;
	// minimum analog gain, 1000-based fixed point
	unsigned int minAnalogGain;
	// maximum analog gain, 1000-based fixed point
	unsigned int maxAnalogGain;
	// HDR LE/SE share same analog gain
	bool sharedAgain;
	struct _SensorAeGainFormula_t formula;

	// Sensor profile related
	// time of line in nanosecond precise
	unsigned int timeOfLine;
	// frame length as exposure line per sensor profile
	unsigned int frameLengthLines;
	// extra exposure time in nanosecond when calculating time of line.
	// TOL * line + offset = real exposure time.
	unsigned int expoOffset;

	// Sensor calib related
	// how many ISO is equal to 1.x gain
	unsigned int baseIso;
	// Initial itegration time, 1000-based fixed point
	unsigned int initItime;
	// Initial analog gain, 1000-based fixed point
	unsigned int initAgain;
};

struct _SensorAfProp_t {
	unsigned int min_pos;
	unsigned int max_pos;
	unsigned int init_pos;
};

/** @struct _PdOutputType_t
 * Sensor pd output prop
 */
enum _PdOutputType_t {
	PD_OUTPUT_INVALID = 0,        ///< pd output invalid
	PD_OUTPUT_PIXEL   = 1,        ///< pd output pixel
	PD_OUTPUT_DATA    = 2,        ///< pd output data
	PD_OUTPUT_MAX,                ///< pd output max
};

/** @struct _SensorProp_t
 * Sensor prop
 */
struct _SensorProp_t {
	enum _SensorIntfType_t intfType;
	union {
		struct _MipiIntfProp_t mipi;
		struct _ParallelIntfProp_t parallel;
		struct _RDmaIntfProp_t rdma;
	} intfProp;
	enum _CFAPattern_t cfaPattern;
	enum _SensorShutterType_t sensorShutterType;
	int hasEmbeddedData;
	//Distinguish pre and post for embedded data
	unsigned int itimeDelayFrames;
	unsigned int gainDelayFrames;
	int isPdafSensor;
	enum _PdOutputType_t PdOutputType;   ///< Pd output type
	enum _PdafType_t pdafType;
	struct _SensorAeProp_t ae;
	struct _SensorAfProp_t af;
	//SensorProfile_t profile[RES_MODE_MAX];
};

enum _HdrType_t {
	HDR_TYPE_INVALID            = 0,
	HDR_TYPE_NONE_HDR           = 1,
	HDR_TYPE_COMPOUND           = 2,
	HDR_TYPE_ZIGZAG_NORMAL      = 3,
	HDR_TYPE_ZIGZAG_V2_BINNING  = 4,
	HDR_TYPE_DOL_2_EXPOSURE     = 5,
	HDR_TYPE_DOL_3_EXPOSURE     = 6,
	HDR_TYPE_MAX
};

enum _HdrItimeType_t {
	HDR_ITIME_TYPE_INVALID,
	HDR_ITIME_TYPE_SEPARATE,  //!< The hdr itime for high exposure and low
				  //exposure has separated control registers
	HDR_ITIME_TYPE_RATIO,     //!< The hdr itime for low exposure is a ratio
				  //of the high exposure
	HDR_ITIME_TYPE_EQUAL,     //!< The hdr itime for high and low exposure
				  //should be euqal.
	HDR_ITIME_TYPE_MAX
};

enum _HdrAGainType_t {
	HDR_AGAIN_TYPE_INVALID,
	HDR_AGAIN_TYPE_SEPARATE,  //!< The hdr analog gain for high exposure and
				  //low exposure has separated control registers
	HDR_AGAIN_TYPE_RATIO,     //!< The hdr analog gain for low exposure is a
				  //ratio of the high exposure
	HDR_AGAIN_TYPE_EQUAL,     //!< The hdr analog gain for high and low
				  //exposure should be euqal.
	HDR_AGAIN_TYPE_MAX
};


enum _HdrDGainType_t {
	HDR_DGAIN_TYPE_INVALID,
	//!< The hdr digital gain for high exposure and low exposure has
	//separated control registers
	HDR_DGAIN_TYPE_SEPARATE,
	//!< The hdr digital gain for low exposure is a ratio of the high
	//exposure
	HDR_DGAIN_TYPE_RATIO,
	//!< The hdr digital gain for high and low exposure should be euqal.
	HDR_DGAIN_TYPE_EQUAL,
	HDR_DGAIN_TYPE_MAX
};

enum _HdrStatDataMatrixId_t {
	HDR_STAT_DATA_MATRIX_ID_INVALID = 0,
	HDR_STAT_DATA_MATRIX_ID_16X16   = 1,
	HDR_STAT_DATA_MATRIX_ID_8X8     = 2,
	HDR_STAT_DATA_MATRIX_ID_4X4     = 3,
	HDR_STAT_DATA_MATRIX_ID_1X1     = 4,
	HDR_STAT_DATA_MATRIX_ID_MAX,
};

/**
 * In HDR image, long exposure pixels and short exposure pixels exist in a
 * fixed pattern for a specific sensor type.
 * The HDR pattern always in 4x4 format; (1: long exposure, 0: short exposure)
 * as shown in following table
 *
 *
 *	 [1,1,0,1;
 *	  0,1,0,0;	= ZZHDR_PATTERN_0(0xD471)
 *	  0,1,1,1;
 *	  0,0,0,1]
 *
 */

enum _ZzhdrPattern_t {
	ZZHDR_PATTERN_INVALID      = 0,
	ZZHDR_PATTERN_0         = 0xD471,
	ZZHDR_PATTERN_1         = 0xB8E2,
	ZZHDR_PATTERN_2         = 0x471D,
	ZZHDR_PATTERN_3         = 0x8E2B,
	ZZHDR_PATTERN_4         = 0x71D4,
	ZZHDR_PATTERN_5         = 0xE2B8,
	ZZHDR_PATTERN_6         = 0x1D47,
	ZZHDR_PATTERN_7         = 0x2B8E,
	ZZHDR_PATTERN_8         = 0xD174,
	ZZHDR_PATTERN_MAX
};

#define MAX_HDR_RATIO_NUM (16)
struct _SensorHdrProp_t {
	enum _HdrType_t             hdrType;
	enum _ZzhdrPattern_t		  zzPattern;
	enum _MipiVirtualChannel_t  virtChannel;
	enum _MipiDataType_t        dataType;
	enum _HdrStatDataMatrixId_t matrixId;
	struct _Window_t              hdrStatWindow;
	enum _HdrItimeType_t        itimeType;
	unsigned char                 itimeRatioList[MAX_HDR_RATIO_NUM];
	enum _HdrAGainType_t        aGainType;
	unsigned char                 aGainRatioList[MAX_HDR_RATIO_NUM];
	enum _HdrDGainType_t        dGainType;
	unsigned char                 dGainRatioList[MAX_HDR_RATIO_NUM];
};

struct _SensorEmbProp_t {
	enum _MipiVirtualChannel_t  virtChannel;
	enum _MipiDataType_t        dataType;
	struct _Window_t              embDataWindow;
	unsigned int                expoStartByteOffset;
	unsigned int                expoNeededBytes;
};

enum _instr_op_t {
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
};

enum _instr_arg_type_t {
	/*#define */INSTR_ARG_TYPE_INVALID = (0x0),
	/*#define */INSTR_ARG_TYPE_REG = (0x1),
	/*#define */INSTR_ARG_TYPE_MEM = (0x2),
	/*#define */INSTR_ARG_TYPE_IMM = (0x3),
	/*#define */INSTR_ARG_TYPE_LABEL = (0x4),
	/*#define */INSTR_ARG_TYPE_FUNC = (0x5),
	/*#define */INSTR_ARG_TYPE_MAX = (0x6)
};

#define INSTR_MEM_OFFSET_TYPE_REG (0x0)
#define INSTR_MEM_OFFSET_TYPE_IMM (0x1)


//================================
//JUMP Instruction
//List:
//     INSTR_OP_JNZ
//     INSTR_OP_JEZ
//     INSTR_OP_JLZ
//     INSTR_OP_JGZ
//     INSTR_OP_JGEZ
//     INSTR_OP_JLEZ
//Syntax:
//  JNZ Rn, @labelName
//  JNZ $imm, @labelName
//================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: arg1 type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit0]:  instruction index
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reg index(0~31) or immediate value according to
//		     the [bit26~bit25] of instrPart1


//=======================
//CALL Instruction
//List:
//    INSTR_OP_CALL
//Syntax:
//  CALL @funcName
//=======================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: reserved(0)
//[bit24~bit0]:  function index (range 0~MAX_SCRIPT_FUNCS-1)
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reserved(0)

//=============================================
//ADD/SUB/MUL/DIV/AND/OR/XOR Instruction
//List:
//      INSTR_OP_ADD
//      INSTR_OP_SUB
//      INSTR_OP_MUL
//      INSTR_OP_DIV
//      INSTR_OP_SHL
//      INSTR_OP_SHR
//      INSTR_OP_AND
//      INSTR_OP_OR
//      INSTR_OP_XOR
//Syntax:
//  ADD Rd, Rs, Rp
//  ADD Rd, Rs, $imm
//=============================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit22]: Rd register index
//[bit21~bit17]: Rs register index
//[bit16~bit15]: arg3 type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit14~bit0]:  reserved(0)
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]: reg index(0~31) or immediate value according to the
//		[bit16~bit15] of instrPart1


//=========================================
//MOVB/MOVW/MOVDW Instruction
//List:
//      INSTR_OP_MOVB
//      INSTR_OP_MOVW
//      INSTR_OP_MOVDW
//Syntax:
//  MOVB dst, src
//=========================================
//-------------------
//instrPart1:
//-------------------
//[bit31~bit27]: op
//[bit26~bit25]: dst arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM)
//[bit24~bit7]:  (1): if dst arg type is INISTR_ARG_TYPE_REG,
//                    [bit24~bit20] defines the register index{0~31}
//                    [bit19~bit7] reserved
//               (2): if dst arg type is INSTR_ARG_MEM,
//                    [bit24] is mem{0~1} index,
//                   [bit23] is dst mem offset type (INSTR_MEM_OFFSET_TYPE_REG
//				or INSTR_MEM_OFFSET_TYPE_IMM)
//                    [bit22~bit7]: (1) if dst mem offset type is
//					INSTR_MEM_OFFSET_TYPE_REG
//                                      [bit22~bit18] is register index{0~31}
//                                      [bit17~bit7]  reserved
//                                  (2) if dst mem offset type is
//					INSTR_MEM_OFFSET_TYPE_IMM
//                                      [bit22~bit7] is the 16 bit
//					unsigned imm value
//[bit6~bit5]:   src arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM or
//		     INSTR_ARG_TYPE_IMM)
//[bit4~bit0]:   (1) if src arg type is INSTR_ARG_TYPE_REG,
//                   [bit4~bit0] defines the register index{0~31}
//               (2) if src arg type is INSTR_ARG_MEM,
//                   [bit4] is mem{0~1} index
//                   [bit3] is src mem offset type (INSTR_MEM_OFFSET_TYPE_REG
//		     or INSTR_MEM_OFFSET_TYPE_IMM)
//                   [bit2~bit0] reserved
//               (3) if src arg type is INSTR_ARG_TYPE_IMM
//                   [bit4~bit0] reserved
//-------------------
//instrPart2:
//-------------------
//[bit31~bit0]:  (1) if src arg type is INSTR_ARG_TYPE_IMM
//                   [bit31~bit0] defines a unsigned 32 bit immedage value
//               (2) if src arg type is INSTR_ARG_TYPE_MEM and the mem offset
//		     type is INSTR_MEM_OFFSET_TYPE_REG
//                   [bit31~bit27] is the register index{0~31}
//                   [bit26~bit0] reserved
//               (3) if src arg type is INSTR_ARG_TYPE_MEM and the mem offset
//		     type is INSTR_MEM_OFFSET_TYPE_IMM
//                   [bit31~bit16] is the 16bit unsigned immediate value
//                   [bit15~bit0} reserved


//=========================================
//WI2C Instruction
//List: INSTR_OP_WI2C
//Syntax:
//  WI2C i2cRegIndex, value, length
//=========================================
//----------------
//instrPart1:
//----------------
//[bit31~bit27]: op
//[bit26~bit25]:
//i2cRegIndex arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit23]: value arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit22~bit21]: length arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit20~bit16]: register index(0~31) for length if its type is
//		INSTR_ARG_TYPE_REG,or an immediate value for length(range 0~4).
//[bit15~bit0]:  register index(0~31) for i2cRegIndex if its type is
//		 INSTR_ARG_TYPE_REG or
//               a 16bit immediate value for i2cRegIndex if its type is
//		 INSTR_ARG_TYPE_IMM
//               according to [bit26~bit25] in instrPart1
//---------------
//instrPart2:
//---------------
//[bit31~bit0]:  immediate value for value arg if its type is INSTR_ARG_TYPE_IMM
//		 or register
//               index(0~31) for value arg if its type is INSTR_ARG_TYPE_REG
//		 according to
//               [bit24~bit23] in instrPart1


//==========================================
//RI2C Instruction
//List: INSTR_OP_RI2C
//Syntax:
//  RI2C Rd, i2cRegIndex, length
//==========================================
//----------------
//instrPart1:
//----------------
//[bit31~bit27]: op
//[bit26~bit25]:
//i2cRegIndex arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit24~bit23]: reserved
//[bit22~bit21]: length arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
//[bit20~bit16]: register index(0~31) for length if its type is
//               INSTR_ARG_TYPE_REG,or an immediate value for length(range 0~4).
//[bit15~bit0]:  register index(0~31) for i2cRegIndex if its type is
//		 INSTR_ARG_TYPE_REG or
//               a 16bit immediate value for i2cRegIndex if its type is
//		 INSTR_ARG_TYPE_IMM
//               according to [bit26~bit25] in instrPart1
//---------------
//instrPart2:
//---------------
//[bit31~bit27]: Rd register index.
//[bit26~bit0]:  reserved

#define MAX_SCRIPT_INSTRS               (500)
#define MAX_SCRIPT_FUNCS                (40)
#define MAX_SCRIPT_FUNC_NAME_LENGTH     (32)
#define MAX_SCRIPT_FUNC_ARGS            (10)

struct _Instr_t {
	unsigned int instrPart1;
	unsigned int instrPart2;
};

struct _FuncMap_t {
	unsigned char  funcName[MAX_SCRIPT_FUNC_NAME_LENGTH];
	unsigned int startIdx;
	//The start instructions idx associated to this function
	unsigned int endIdx;
	//The end instruction idx associated to this function
};

struct _InstrSeq_t {
	struct _Instr_t   instrs[MAX_SCRIPT_INSTRS];
	struct _FuncMap_t funcs[MAX_SCRIPT_FUNCS];
	unsigned int    instrNum;
	unsigned int    funcNum;
};


struct _ScriptFuncArg_t {
	int args[MAX_SCRIPT_FUNC_ARGS];
	int numArgs;
};


enum _I2cDeviceAddrType_t {
	I2C_DEVICE_ADDR_TYPE_INVALID,
	I2C_DEVICE_ADDR_TYPE_7BIT,
	I2C_DEVICE_ADDR_TYPE_10BIT,
	I2C_DEVICE_ADDR_TYPE_MAX
};


enum _I2cRegAddrType_t {
	I2C_REG_ADDR_TYPE_INVALID,
	I2C_REG_ADDR_TYPE_8BIT,
	I2C_REG_ADDR_TYPE_16BIT,
	I2C_REG_ADDR_TYPE_MAX
};


struct _ScriptI2cArg_t {
	enum _I2cDeviceId_t deviceId;
	int enRestart; //!< Whether enable the restart for the read command
	enum _I2cDeviceAddrType_t deviceAddrType;
	enum _I2cRegAddrType_t regAddrType;
	unsigned short deviceAddr; //!< 7bit or 10bit device address
};

struct _DeviceScript_t {
	struct _InstrSeq_t instrSeq;  //!< The script instructions
	struct _ScriptFuncArg_t argSensor;
	struct _ScriptFuncArg_t argLens;
	struct _ScriptFuncArg_t argFlash;
	struct _ScriptI2cArg_t  i2cSensor;
	struct _ScriptI2cArg_t  i2cLens;
	struct _ScriptI2cArg_t  i2cFlash;
};


enum _I2cMsgType_t {
	I2C_MSG_TYPE_INVALID,
	I2C_MSG_TYPE_READ,
	I2C_MSG_TYPE_WRITE,
	I2C_MSG_TYPE_MAX
};

struct _I2cMsg_t {
	enum _I2cMsgType_t msgType;
	int enRestart;//!< Whether enable the restart for the read command
	enum _I2cDeviceAddrType_t deviceAddrType;
	enum _I2cRegAddrType_t regAddrType;
	unsigned short deviceAddr;   //7bit or 10bit device address
	unsigned short regAddr;
	unsigned short dataLen;      //!< The total data length for read/write
	unsigned char  dataCheckSum; //!< The byte sum of the data buffer.
	unsigned int dataAddrLo;   //!< Low 32 bit of the data buffer
	unsigned int dataAddrHi;   //!< High 32 bit of the data buffer
};

enum _CSM_YUV_Range_t {
	CSM_YUV_RANGE_BT601,
	CSM_YUV_RANGE_FULL,
};

#if defined(DELETED)
//Fix point
struct _HwDirParam_t {
	//Bad Pixel
	unsigned short dpu_th;
	unsigned short dpa_th;

	//Saturation
	unsigned short rgb_th;
	unsigned short ir_th;
	unsigned char  diff;

	//Decontamination
	unsigned short r_coeff[9];
	unsigned short g_coeff[9];
	unsigned short b_coeff[9];
	unsigned short fgb2ir_r_coeff;
	unsigned short fgb2ir_g_coeff;
	unsigned short fgb2ir_b_coeff;

	//RGB Adjust
	unsigned short rgb_uth;
	unsigned short rb_lth;
	unsigned char comp_ratio;
	unsigned char cut_ratio;
	unsigned short arc;
	unsigned short agc;
	unsigned short abc;
	unsigned short arfc;
	unsigned short agfc;
	unsigned short abfc;

	//Tolerance factor for 4x4 interpolation
	unsigned char rb_tf;
	unsigned char g_tf;
	unsigned char i_tf;

	//Color difference threshold for 4x4 interpolation
	unsigned short cd_ath;
	unsigned short cd_uth;
};
#endif

struct _ImageProp_t {
	enum _ImageFormat_t imageFormat;
	unsigned int width;
	unsigned int height;
	unsigned int lumaPitch;
	unsigned int chromaPitch;
};

/**
 *	Suppose the image pixel is in the sequence of:
 *             A B C D E F
 *             G H I J K L
 *             M N O P Q R
 *             ...
 * The following RawPktFmt define the raw picture output format.
 * For each format, different raw pixel width will have different memory
 * filling format. The raw pixel width is set by the SesorProp_t.
 *
 * RAW_PKT_FMT_0:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT   |  0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT  |  A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT  |  A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT  |  A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_1:
 *    --------+-------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT     A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT     A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_2:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT      0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_3:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_4:
 *    (1) 8-BIT:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *               B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    (2) 10-BIT:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *               B5  B4  B3  B2  B1  B0  A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               D1  D0  C9  C8  C7  C6  C5  C4  C3  C2  C1  C0  B9  B8  B7  B6
 *               E7  E6  E5  E4  E3  E2  E1  E0  D9  D8  D7  D6  D5  D4  D3  D2
 *               G3  G2  G1  G0  F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E9  E8
 *               H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G9  G8  G7  G6  G5  G4
 *    (3) 12-BIT:
 *    --------+--------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *               B3  B2  B1  B0  A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C7  C6  C5  C4  C3  C2  C1  C0  B11 B10 B9  B8  B7  B6  B5  B4
 *               D11 D10 D9  D8  D7  D6  D5  D4  D3  D2  D1  D0  C11 C10 C9  C8
 *    (4) 14-BIT:
 *    --------+--------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+--------------------------------------------------------
 *               B1  B0  A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C3  C2  C1  C0  B13 B12 B11 B10 B9  B8  B7  B6  B5  B4  B3  B2
 *               D5  D4  D3  D2  D1  D0  C13 C12 C11 C10 C9  C8  C7  C6  C5  C4
 *               E7  E6  E5  E4  E3  E2  E1  E0  D13 D12 D11 D10 D9  D8  D7  D6
 *               F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E13 E12 E11 E10 E9  E8
 *               G11 G10 G9  G8  G7  G6  G5  G4  G3  G2  G1  G0  F13 F12 F11 F10
 *              H13 H12 H11 H10 H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G13 G12
 *
 * RAW_PKT_FMT_5:
 *    --------+---------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_6:
 *    --------+--------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+---------------------------------------------------------
 *    8-BIT      A7  A6  A5  A4  A3  A2  A1  A0  B7  B6  B5  B4  B3  B2  B1  B0
 *    10-BIT     A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0   0   0
 *    12-BIT     A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0
 */
enum _RawPktFmt_t {
	RAW_PKT_FMT_0,  /* Default(ISP1P1 legacy format) */
	/* ISP1P1 legacy format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_1,
	RAW_PKT_FMT_2,  /* Android RAW16 format */
	/* Android RAW16 format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_3,
	RAW_PKT_FMT_4,  /* ISP2.0 bubble-free format */
	RAW_PKT_FMT_5,  /* RGB-IR format for GPU process */
	RAW_PKT_FMT_6,  /* RGB-IR format for GPU process with data swapped */
	RAW_PKT_FMT_MAX
};


// ***********************************************************
//                        3A
// ***********************************************************

enum _LockType_t {
	LOCK_TYPE_INVALID,
	LOCK_TYPE_IMMEDIATELY,
	LOCK_TYPE_CONVERGENCE,
	LOCK_TYPE_UNLOCK,
	LOCK_TYPE_MAX
};

#define AAA_REGIONS_MAX 20
struct _AaaRegion_t {
	struct _Roi_t   region[AAA_REGIONS_MAX];
	unsigned char   regionCnt;
};

//  AE
//-------

enum _AeMode_t {
	AE_MODE_INVALID,
	AE_MODE_MANUAL,
	AE_MODE_AUTO,
	AE_MODE_MAX
};

enum _AeApplyMode_t {
	/* won't wait cfg applied */
	AE_APPLY_MODE_ASYNC = 0,
	/* Wait cfg applied, then calculate. HDR should not use this.
	 * We shouldn't use this in normal case.
	 */
	AE_APPLY_MODE_SYNC = 1,
	AE_APPLY_MODE_MAX
};

enum _AeFlickerType_t {
	AE_FLICKER_TYPE_INVALID,
	AE_FLICKER_TYPE_OFF,
	AE_FLICKER_TYPE_50HZ,
	AE_FLICKER_TYPE_60HZ,
	AE_FLICKER_TYPE_MAX
};


struct _AeIso_t {
	unsigned int iso;
};


struct _AeEv_t {
	int numerator;
	int denominator;
};

enum _BufferType_t {
	BUFFER_TYPE_INVALID,

	BUFFER_TYPE_RAW, //BUFFER_TYPE_RAW_ZSL,
	BUFFER_TYPE_FULL_RAW,
	BUFFER_TYPE_RAW_TEMP,
	BUFFER_TYPE_FULL_RAW_TEMP,
	BUFFER_TYPE_EMB_DATA,
	BUFFER_TYPE_HDR_STATS_DATA,
	BUFFER_TYPE_PD_DATA, //stg1 or stg2

	BUFFER_TYPE_STILL,
	BUFFER_TYPE_FULL_STILL,
	BUFFER_TYPE_PREVIEW,
	BUFFER_TYPE_VIDEO,
	BUFFER_TYPE_X86_CV, // X86 CV
	BUFFER_TYPE_YUV_TEMP, // for zoom input
	BUFFER_TYPE_RGBIR_IR,  //X86 IR
	BUFFER_TYPE_TNR_REF,
	BUFFER_TYPE_TNR_FULL_STILL_REF,

	BUFFER_TYPE_META_INFO,
	BUFFER_TYPE_FRAME_INFO,

	// BUFFER_TYPE_CRISP_PRE_INPUT_TMP,
	// use rawFslTempBufferQ for pre input
	BUFFER_TYPE_CRISP_POST_INPUT_TMP, //For VS
	BUFFER_TYPE_ENGINEERING_DATA,
    // BUFFER_TYPE_CRISP_META_INFO,
	BUFFER_TYPE_MAX
};


enum _AddrSpaceType_t {
	ADDR_SPACE_TYPE_GUEST_VA        = 0,
	ADDR_SPACE_TYPE_GUEST_PA        = 1,
	ADDR_SPACE_TYPE_SYSTEM_PA       = 2,
	ADDR_SPACE_TYPE_FRAME_BUFFER_PA = 3,
	ADDR_SPACE_TYPE_GPU_VA          = 4,
	ADDR_SPACE_TYPE_MAX             = 5
};


struct _Buffer_t {
	// A check num for debug usage, host need to
	// set the bufTags to different number

	unsigned int bufTags;
	union {
		unsigned int value; //Vmid[31:16], Space[15:0]
	    struct {
			unsigned int	space :	16;
			unsigned int	vmid  :	16;
	    } bit;
	} vmidSpace;
	unsigned int bufBaseALo;
	unsigned int bufBaseAHi;
	unsigned int bufSizeA;

	unsigned int bufBaseBLo;
	unsigned int bufBaseBHi;
	unsigned int bufSizeB;

	unsigned int bufBaseCLo;
	unsigned int bufBaseCHi;
	unsigned int bufSizeC;
};


enum _AeLockState_t {
	AE_LOCK_STATE_INVALID,
	AE_LOCK_STATE_LOCKED,
	AE_LOCK_STATE_UNLOCKED,
	AE_LOCK_STATE_MAX
};



enum _AeSearchState_t {
	AE_SEARCH_STATE_INVALID     = 0,
	AE_SEARCH_STATE_SEARCHING   = 1,
	AE_SEARCH_STATE_PRECAPTURE  = 2,
	AE_SEARCH_STATE_CONVERGED   = 3,
	AE_SEARCH_STATE_MAX
};


struct _AeExposure_t {
	unsigned int itime;    //!< integration time in microsecond
	unsigned int aGain;    //!< analog gain muptiplied by 1000
	unsigned int dGain;    //!< digital gain multiplied by 1000
};


// AWB
//------

enum _AwbMode_t {
	AWB_MODE_INVALID            = 0,
	AWB_MODE_MANUAL             = 1,
	AWB_MODE_AUTO               = 2,
	AWB_MODE_LIGHT_SOURCE       = 3,
	AWB_MODE_COLOR_TEMPERATURE  = 4,
	AWB_MODE_MAX                = 5
};

enum _AwbLightSource_t {
	AWB_LIGHT_SOURCE_UNKNOWN            = 0,
	AWB_LIGHT_SOURCE_AUTO               = 1,
	// Android: N/A               Windows: SHADE
	// AWB Algo: AWB_MANUAL_MODE_SHADE
	AWB_LIGHT_SOURCE_SHADE              = 2,
	// Android: CLOUDY_DAYLIGHT   Windows: CLOUDY (?)
	// AWB Algo: AWB_MANUAL_MODE_CLOUDY
	AWB_LIGHT_SOURCE_CLOUDY             = 3,
	// Android: DAYLIGHT          Windows: DAYLIGHT (WB)
	// AWB Algo: AWB_MANUAL_MODE_SUNLIGHT
	AWB_LIGHT_SOURCE_DAYLIGHT           = 4,
	// Android: N/A               Windows: SUNSET (scene)
	// AWB Algo: AWB_MANUAL_MODE_SUNSET
	AWB_LIGHT_SOURCE_SUNSET             = 5,
	// Android: N/A               Windows: N/A
	// AWB Algo: AWB_MANUAL_MODE_FLH (CIE F2)
	AWB_LIGHT_SOURCE_COOL_FLUORESCENT   = 6,
	// Android: FLUORESCENT       Windows: FLUORESCENT (WB)
	// AWB Algo: AWB_MANUAL_MODE_FLM (CIE F3)
	AWB_LIGHT_SOURCE_FLUORESCENT        = 7,
	// Android: WARM_FLUORESCENT  Windows: N/A
	// AWB Algo: AWB_MANUAL_MODE_FLL (CIE F4)
	AWB_LIGHT_SOURCE_WARM_FLUORESCENT   = 8,
	// Android: INCANDESCENT      Windows: TUNGSTEN (WB)
	// AWB Algo: AWB_MANUAL_MODE_INCANDESCENT
	AWB_LIGHT_SOURCE_INCANDESCENT       = 9,
	// Android: TWILIGHT          Windows: CANDLELIGHT (scene)
	// AWB Algo: AWB_MANUAL_MODE_CANDLE
	AWB_LIGHT_SOURCE_TWILIGHT           = 10,
	// Android: N/A               Windows: FLASH (WB)
	// AWB Algo: N/A
	AWB_LIGHT_SOURCE_FLASH              = 11,
	AWB_LIGHT_SOURCE_MAX                = 12
};

struct _CcMatrix_t {
	float coeff[9];
};


struct _CcOffset_t {
	float coeff[3];
};

struct _CcGain_t {
	float coeff[3];
};

#define MAX_LSC_SECTORS   (16)
#define LSC_DATA_TBL_SIZE ((MAX_LSC_SECTORS + 1) * (MAX_LSC_SECTORS + 1))

struct _LscMatrix_t {
	unsigned short dataR[LSC_DATA_TBL_SIZE];
	unsigned short dataGR[LSC_DATA_TBL_SIZE];
	unsigned short dataGB[LSC_DATA_TBL_SIZE];
	unsigned short dataB[LSC_DATA_TBL_SIZE];
};
struct _StatisticsLscMap_t {
	unsigned short dataR[LSC_DATA_TBL_SIZE];
	unsigned short dataGR[LSC_DATA_TBL_SIZE];
	unsigned short dataGB[LSC_DATA_TBL_SIZE];
	unsigned short dataB[LSC_DATA_TBL_SIZE];
};


#define LSC_GRAD_TBL_SIZE (8)
#define LSC_SIZE_TBL_SIZE (8)
struct _LscSector_t {
	unsigned short lscXGradTbl[LSC_GRAD_TBL_SIZE];
	unsigned short lscYGradTbl[LSC_GRAD_TBL_SIZE];
	unsigned short lscXSizeTbl[LSC_SIZE_TBL_SIZE];
	unsigned short lscYSizeTbl[LSC_SIZE_TBL_SIZE];
};


#define PREP_LSC_GRAD_TBL_SIZE (8)
#define PREP_LSC_SIZE_TBL_SIZE (16)
struct _PrepLscSector_t {
	unsigned short lscXGradTbl[PREP_LSC_GRAD_TBL_SIZE];
	unsigned short lscYGradTbl[PREP_LSC_GRAD_TBL_SIZE];
	unsigned short lscXSizeTbl[PREP_LSC_SIZE_TBL_SIZE];
	unsigned short lscYSizeTbl[PREP_LSC_SIZE_TBL_SIZE];
};


enum _LscRamSetId_t {
	LSC_RAM_SET_ID_INVALID   = -1,
	LSC_RAM_SET_ID_0   = 0,
	LSC_RAM_SET_ID_1   = 1,
	LSC_RAM_SET_ID_2   = 2,
	LSC_RAM_SET_ID_3   = 3,
	LSC_RAM_SET_ID_4   = 4,
	LSC_RAM_SET_ID_5   = 5,
	LSC_RAM_SET_ID_MAX = 6,
};


struct _illuLikelihood_t {
	float coeff[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
};


enum _AwbLockState_t {
	AWB_LOCK_STATE_INVALID,
	AWB_LOCK_STATE_LOCKED,
	AWB_LOCK_STATE_UNLOCKED,
	AWB_LOCK_STATE_MAX
};


enum _AwbSearchState_t {
	AWB_SEARCH_STATE_INVALID,
	AWB_SEARCH_STATE_SEARCHING,
	AWB_SEARCH_STATE_CONVERGED,
	AWB_SEARCH_STATE_MAX
};


struct _WbGain_t {
	float red;
	float greenR;
	float greenB;
	float blue;
};


struct _AwbmConfig_t {
	unsigned char maxY;
	unsigned char refCr_maxR;
	unsigned char minY_maxG;
	unsigned char refCb_maxB;
	unsigned char maxCSum;
	unsigned char minC;
};

//  AF
//-------

enum _AfMode_t {
	AF_MODE_INVALID             = 0,
	AF_MODE_MANUAL              = 1,
	AF_MODE_ONE_SHOT            = 2,
	AF_MODE_CONTINUOUS_PICTURE  = 3,
	AF_MODE_CONTINUOUS_VIDEO    = 4,
	AF_MODE_CALIBRATION		    = 5,
	AF_MODE_AUTO				= 6,
	AF_MODE_MAX
};

enum _LensState_t {
	LENS_STATE_INVALID      = 0,
	LENS_STATE_SEARCHING    = 1,
	LENS_STATE_CONVERGED    = 2,
	LENS_STATE_MAX,
};

enum _LensPosUnit_t {
	POS_UNIT_DIOPTER = 0,
	POS_UNIT_LENGTH  = 1,
	POS_UNIT_MAX,
};

struct _LensRange_t {
	unsigned int minLens;
	unsigned int maxLens;
	unsigned int stepLens;
};


enum _FocusAssistMode_t {
	FOCUS_ASSIST_MODE_INVALID,
	FOCUS_ASSIST_MODE_OFF,
	FOCUS_ASSIST_MODE_ON,
	FOCUS_ASSIST_MODE_AUTO,
	FOCUS_ASSIST_MODE_MAX
};



enum _AfLockState_t {
	AF_LOCK_STATE_INVALID,
	AF_LOCK_STATE_LOCKED,
	AF_LOCK_STATE_UNLOCKED,
	AF_LOCK_STATE_MAX
};


enum _AfSearchState_t {
	AF_SEARCH_STATE_INVALID,
	AF_SEARCH_STATE_INIT,
	AF_SEARCH_STATE_SEARCHING,
	AF_SEARCH_STATE_CONVERGED,
	AF_SEARCH_STATE_FAILED,
	AF_SEARCH_STATE_MAX
};

enum _AfSearchRangeMode_t {
	AF_SEARCH_RANGE_MODE_NONE = 0,
	AF_SEARCH_RANGE_MODE_FULL = 1,
	AF_SEARCH_RANGE_MODE_NORMAL,
	AF_SEARCH_RANGE_MODE_MACRO,
	AF_SEARCH_RANGE_MODE_INF,
	AF_SEARCH_RANGE_MODE_HYPER,
};

enum _AfCalibrationType_t {
	AF_CALIBRATION_TYPE_1 = 1,
	AF_CALIBRATION_TYPE_2,
	AF_CALIBRATION_TYPE_3,
	AF_CALIBRATION_TYPE_4,
	AF_CALIBRATION_TYPE_5,
	AF_CALIBRATION_TYPE_6,
	AF_CALIBRATION_TYPE_7,
	AF_CALIBRATION_TYPE_8,
	AF_CALIBRATION_TYPE_9,
	AF_CALIBRATION_TYPE_10,
	AF_CALIBRATION_TYPE_11,
	AF_CALIBRATION_TYPE_12,
	AF_CALIBRATION_TYPE_13,
	AF_CALIBRATION_TYPE_14,
	AF_CALIBRATION_TYPE_15 = 15,
};
//  DynamicIQ LSC
//-------

enum _LscMode_t {
	LSC_MODE_INVALID,
	LSC_MODE_MANUAL,
	LSC_MODE_ONE_SHOT,
	LSC_MODE_CONTINUOUS_PICTURE,
	LSC_MODE_CONTINUOUS_VIDEO,
	LSC_MODE_MAX
};

#define TDB_IDX_0                (0x1 << 0)
#define TDB_IDX_1                (0x1 << 1)
#define TDB_IDX_2                (0x1 << 2)

// ***************************************COREPIPE DEFINITION*********//
//Notice: the following definition should not be treaded as finnal version, it
//just add for first release,
//we should refine it later.
enum _InputPortPresetMode_t {
	INPUT_PORT_PRESET_MODE_INVALID = 0,
	INPUT_PORT_PRESET_MODE_2 = 2,
	INPUT_PORT_PRESET_MODE_6 = 6,   //Support for windowing
	INPUT_PORT_PRESET_MODE_MAX
};

enum _InputPortModeRequest_t {
	INPUT_PORT_MODE_REQUEST_INVALID = -1,
	INPUT_PORT_MODE_REQUEST_SAFE_STOP = 0,
	INPUT_PORT_MODE_REQUEST_SAFE_START = 1,
	INPUT_PORT_MODE_REQUEST_MAX
};

struct _InportPortWindow_t {
	unsigned short hcStart0;
	unsigned short hcStart1;
	unsigned short vcStart;
	unsigned short hcSize0;
	unsigned short hcSize1;
	unsigned short vcSize;
};

struct _InputPortParams_t {
	enum _InputPortPresetMode_t   mode;
	enum _InputPortModeRequest_t  request;
	struct _InportPortWindow_t      window;
};

struct _CrossBarChannelSel_t {
	unsigned char ch1;
	unsigned char ch2;
	unsigned char ch3;
	unsigned char ch4;
};

enum _TestPattenType_t {
	//TEST_PATTERN_INVALID = -1,
	TEST_PATTERN_TYPE_INVALID = -1,
	TEST_PATTERN_FLAT_FIELD,
	TEST_PATTERN_HORIZONTAL_GRADIENT,
	TEST_PATTERN_VERTICAL_GRADIENT,
	TEST_PATTERN_VERTICAL_BARS,
	TEST_PATTERN_RECTANGLE,
	TEST_PATTERN_DEFAULT,
	//TEST_PATTERN_MAX
	TEST_PATTERN_TYPE_MAX
};

enum _VideoTypeSelect_t {
	INPUT_VIDEO_TYPE_INVALID = -1,
	INPUT_VIDEO_TYPE_BAYER,
	INPUT_VIDEO_TYPE_RGB,
	INPUT_VIDEO_TYPE_MAX
};

enum _InputVideoGenMode_t {
	INPUT_VIDEO_GEN_MODE_INVALID = -1,
	INPUT_VIDEO_GEN_MODE_ONE_SHOT,
	INPUT_VIDEO_GEN_MODE_FREE_RUN,
	INPUT_VIDEO_GEN_MODE_MAX
};

struct _RgbBackGnd_t {
	unsigned int rBackGnd;
	unsigned int gBackGnd;
	unsigned int bBackGnd;
};

struct _RgbForeGnd_t {
	unsigned int rforeGnd;
	unsigned int gforeGnd;
	unsigned int bforeGnd;
};

struct _RectangleWindow_t {
	unsigned short rectBot;
	unsigned short rectTop;
	unsigned short rectRight;
	unsigned short rectLeft;
};

struct _TestPattenGenParams_t {
	int              enabled;
	int              useDefaultSetting;
	enum _VideoTypeSelect_t   inputSelect;
	enum _VideoTypeSelect_t   outputSelect;
	enum _InputVideoGenMode_t generateMode;
	enum _TestPattenType_t    pattenType;
	struct _RgbBackGnd_t        rgbBackGnd;
	struct _RgbForeGnd_t        rgbForeGnd;
	unsigned short              rgbGradient;
	unsigned int              rgbGradientStart;
	struct _RectangleWindow_t   rectangleWindow;
};

enum _InputMode_t {
	INPUT_MODE_INVALID = -1,
	INPUT_MODE_LINEAR_DATA,
	INPUT_MODE_2_3_MULTIPLE_EXPOSURE_MULTIPLEXING,
	INPUT_MODE_LOGARITHMIC_ENCODING,
	INPUT_MODE_COMPANDING_CURVE_WITH_KNEE_POINTS,
	INPUT_MODE_16BIT_LINEAR_12BIT_VS,
	INPUT_MODE_RESERVED,
	INPUT_MODE_PASS_THROUGH_MODE,
	INPUT_MODE_MAX
};

enum _InputBitWidth_t {
	INPUT_BITWIDTH_INVALID = -1,
	INPUT_BITWIDTH_8BIT,
	INPUT_BITWIDTH_10BIT,
	INPUT_BITWIDTH_12BIT,
	INPUT_BITWIDTH_14BIT,
	INPUT_BITWIDTH_16BIT,
	INPUT_BITWIDTH_20BIT,
	INPUT_BITWIDTH_MAX
};

struct _KneePoint_t {
	unsigned short kneePoint0;
	unsigned short kneePoint1;
	unsigned short kneePoint2;
};

struct _SlopSlect_t {
	unsigned char slope0Select;
	unsigned char slope1Select;
	unsigned char slope2Select;
	unsigned char slope3Select;
};

struct _InputFormatterParams_t {
	enum _InputMode_t     inputMode;
	enum _InputBitWidth_t bitWidth;
	struct _KneePoint_t     kneePoint;
	struct _SlopSlect_t     slopSlect;
};

struct _GreenEqualizationParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int  enabled;
#endif
	struct _CdbGreenEqualization_t ge;
};

struct _DynamicDPCParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int  enabled;
#endif
#ifndef CONFIG_ENABLE_TDB_V1_0
	//0x00 Replace detected defects with non-directional replacement value;
	//0xFF Replace detected defects with directional replacement value
	unsigned char   dpBlend;
	//False color strength on edges, defalut value in sample code is 0x8000L
	unsigned short  dpDevThreshold;
	//Manually override noise estimation, defalut value in sample
	//code is 0x00L
	unsigned short  sigmaIn;
	//Controls directional nature of replacement algorithm, defalut
	//value in sample code is 0x00L
	unsigned short  lineThresh;
#endif
	struct _CdbDPCorrection_t dpCorrection;
};

#define STATICDPCLUTSIZE (4096)

struct _StaticDPCParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int correctionEnable;
#endif
	int detectionEnable;  //should be 0
	unsigned short defectPixelCountIn;
	unsigned int dpcLut[STATICDPCLUTSIZE];
};

struct _NoiseLevel_t {
	unsigned char noiseLevel0;
	unsigned char noiseLevel1;
	unsigned char noiseLevel2;
	unsigned char noiseLevel3;
};

struct _ExposureThresh_t {
	unsigned short exposureThresh1;
	unsigned short exposureThresh2;
	unsigned short exposureThresh3;
};

struct _SinterRadialParams_t {
	int rmEnable;
	unsigned short rmCenterX;
	unsigned short rmCenterY;
	unsigned short rmOffCenterMult;
	//round((2^31)/((rm_centre_x^2)+(rm_centre_y^2)))
};

struct _SinterParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int          enabled;
#endif
	struct _CdbSinter_t     sinter;
};

struct _BankBaseWriter_t {
	unsigned int lsbBankBaseWriter;
	unsigned int msbBankBaseWriter;
};

struct _BankBaseReader_t {
	unsigned int lsbBankBaseReader;
	unsigned int msbBankBaseReader;
};

#ifndef CONFIG_ENABLE_TDB_V2_0
enum _TemperMode_t {
	TEMPER_MODE_INVALID = -1,
	TEMPER_MODE_3,
	TEMPER_MODE_2,
	TEMPER_MODE_MAX
};
#endif
struct _TemperParams_t {
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _CdbTemper_t temper;
#else
	int enabled;
	TemperMode_t temperMode;
	struct _CdbTemperStrength_t   temperStrength;
#endif
};

struct _CaCorrectionParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int      enabled;
#endif
#ifndef CONFIG_ENABLE_TDB_V1_0
	unsigned char       meshScale;
#endif
	struct _CdbCAC_t    cac;
};

struct _RadialShadingParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int                   enabled;
#endif
	struct _CdbRadialShading_t       radiaShading;
};

enum _MeshPage_t {
	 MESH_PAGE_INVALID = -1,
	 MESH_PAGE_R,
	 MESH_PAGE_G,
	 MESH_PAGE_B,
	 MESH_PAGE_IR,
	 MESH_PAGE_MAX
};

struct _MeshAlpha_t {
	unsigned char meshAlphaR;
	unsigned char meshAlphaG;
	unsigned char meshAlphaB;
	unsigned char meshAlphaIR;
};

struct _MeshAlphaBank_t {
	unsigned char meshAlphaBankR;
	unsigned char meshAlphaBankG;
	unsigned char meshAlphaBankB;
	unsigned char meshAlphaBankIR;
};

struct _MeshShadingParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int  enabled;
#endif
#ifndef CONFIG_ENABLE_TDB_V1_0
	unsigned char   meshScale;
	unsigned char   meshAlphaMode;// initial value should be 3
	MeshAlphaBank_t     meshAlphaBank;
	MeshAlpha_t         meshAlpha;
#endif
	struct _CdbShading_t        meshShading;
};

enum _IridixMode_t {
	IRIDIX_MODE_IVALID = -1,
	IRIDIX_MODE_AUTO = 0,
	IRIDIX_MODE_MAUNAL,
	IRIDIX_MODE_MAX
};

struct _IridixManualParams_t {
	unsigned short iridixGlobalDg;
	unsigned char varianceSpace;
	unsigned char varianceIntensity;
	unsigned int blackLevel;
	unsigned int whiteLevel;
	unsigned short collectionCorrection;
	unsigned char fwdPerceptControl;
	unsigned char revPerceptControl;
	unsigned short strengthInroi;
	unsigned short strengthOutroi;
	unsigned short horRoiStart;
	unsigned short horRoiEnd;
	unsigned short verRoiStart;
	unsigned short verRoiEnd;
	unsigned char svariance;
	unsigned char brightPr;
	unsigned char contrast;
	unsigned short darkEnh;
};

struct _IridixParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int  enabled;
#endif
	enum _IridixMode_t  mode;
#ifndef CONFIG_ENABLE_TDB_V1_0
	unsigned char   variance_space;       // 0x7f;
	unsigned char   fwdPerceptCtl;        // 0x02;
	unsigned char   revPerceptCtl;        // 0x02;
	unsigned char   filterMux;            // 0x01;
	unsigned char   svariance;            // 0x0a;
	unsigned char   brightPr;             // 0xdc;
	unsigned char   contrast;             //0xb4;
	unsigned short  darkEnh;
	unsigned short  strengthInroi;
	unsigned short  collectionCorrection;
	unsigned int  blackLevel;          // 0x00;
	unsigned int  whiteLevel;          // 0xd9999;
#endif
	struct _CdbIridix_t                 iridix;
	struct _CdbCalibraitonAeControl_t   calibAeCtl;
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _CdbIridixReg_t              iridixReg;
#else
	IridixManualParams_t        iridixManualParams;
#endif
};

struct _SharpDemosaicParams_t {
	struct _CdbSharp_t       sharp;
};

struct _DemosaicParams_t {
	struct _CdbDemosaic_t    demosaic;
#ifdef CONFIG_DISABLE_TDB_NEW_ELEMENT
	SharpDemosaicParams_t sharpDemosaic;
#endif
};

struct _PfParams_t {
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _CdbPf_t    pf;
#else
	struct _CdbPfRadial_t    pfRadial;
#endif
};

struct _CcmParams_t {
	struct _CdbCcm_t         ccm;
};

struct _CnrParams_t {
#ifndef CONFIG_ENABLE_TDB_V1_0
	int sqrtEnable;
#endif
#ifndef CONFIG_ENABLE_TDB_V2_0
	int enabled;
#endif
	struct _CdbCnrUvDelta12Slop_t   cnrUvDelta12Slop;
};

struct _RgbGammaGain_t {
	unsigned short gainR;
	unsigned short gainG;
	unsigned short gainB;
};

struct _RgbGammaOffset_t {
	unsigned short offsetR;
	unsigned short offsetG;
	unsigned short offsetB;
};

struct _RgbGammaParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int                  enabled;
#endif
	struct _CdbGamma_t              gammaLut;
#ifndef CONFIG_ENABLE_TDB_V1_0
	RgbGammaGain_t          rgbGammaGain;
	RgbGammaOffset_t        rgbGammaOffset;
#endif
	struct _CdbAutoLevelControl_t   autoLevelControl;
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _CdbFrGamma_t            frGamma;
#endif
};

struct _SharpenFrParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int  enabled;
#endif
#ifndef CONFIG_ENABLE_TDB_V1_0
	unsigned char   controlR ;        // 0x4c;
	unsigned char   controlB ;        // 0x00;
	unsigned char   alphaUndershoot ; // 0xa;
	unsigned short  lumaThreshLow ;  // 0x12c;
	unsigned short  lumaSlopeLow ;   //0x03e8;
	unsigned short  lumaThreshHigh ; // 0x3e8;
	unsigned short  lumaSlopeHigh ;  // 0x6a4;
	unsigned short  clipStrMax ;     //0x118;
	unsigned short  clipStrMin ;     // 0x118;
#endif
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _CdbFrSharpen_t    frSharpen;
#else
	struct _CdbSharpenFrStrength_t    sharpenFrStrength;
#endif
};

enum _RgbCsConvColorMode_t {
	CS_CONV_COLOR_MODE_INVALID = 0,
	CS_CONV_COLOR_MODE_NORMAL,
	CS_CONV_COLOR_MODE_BLACK_AND_WHITE,
	CS_CONV_COLOR_MODE_NEGATIVE,
	CS_CONV_COLOR_MODE_SEPIA,
	CS_CONV_COLOR_MODE_VIVID,
	CS_CONV_COLOR_MODE_MAX
};

enum _RgbCsConvOutPutFormat_t {
	CS_CONV_OUTPUT_FORMAT_INVALID = 0,
	CS_CONV_OUTPUT_FORMAT_YUV420,
	CS_CONV_OUTPUT_FORMAT_YUV422,
	CS_CONV_OUTPUT_FORMAT_YUV444,
	CS_CONV_OUTPUT_FORMAT_RGB,
	CS_CONV_OUTPUT_FORMAT_MAX
};

struct _RgbCsConvParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int enabled;
#endif
#ifndef CONFIG_ENABLE_TDB_V1_0
	int enableMatrix;
	int enableFilter;
	unsigned int hueTheta;                   //180;
	unsigned int saturationStrength;         //128;
	unsigned int contrastStrength ;          // 128;
	unsigned int brightnessStrength ;        //128;
	struct _RgbCsConvOutPutFormat_t frPipeOutputFormat;
	//CS_COLOR_MODE_NORMAL;
	struct _RgbCsConvColorMode_t colorMode;
#endif
	struct _CdbRgb2YuvConversion_t      rgb2YuvConversion;
};

struct _NoiseProfileParams_t {
	struct _CdbNoiseProfile_t           noiseProfile;
};
#ifdef ENABLE_SCR
struct _CustSettingsParams_t {
	struct _CdbCustSettings_t           custSettings;
};
#endif
struct _SensorCalibs_t {
	void *calibs[CALIB_MAX];
};

// ******************************COREPIPE DEFINITION END****************




// GAMMA
//-------

enum _GammaSegMode_t {
	GAMMA_SEG_MODE_LOG  = 0,
	GAMMA_SEG_MODE_EQU  = 1,
	GAMMA_SEG_MODE_MAX
};


#define GAMMA_CURVE_SIZE (17)
struct _GammaCurve_t {
	unsigned short gammaY[GAMMA_CURVE_SIZE];
};



//  BLS
//-------
struct _BlackLevel_t {
	unsigned short blsValue[16];
};


// DEGAMMA
//---------
#define DEGAMMA_CURVE_SIZE      (17)
struct _DegammaCurve_t {
	unsigned char  segment[DEGAMMA_CURVE_SIZE-1]; /* x_i segment size*/
	unsigned short red[DEGAMMA_CURVE_SIZE];       /* red point */
	unsigned short green[DEGAMMA_CURVE_SIZE];     /* green point */
	unsigned short blue[DEGAMMA_CURVE_SIZE];      /* blue point */
	unsigned short ir[DEGAMMA_CURVE_SIZE];      /* ir point */
};





// FRAME CONTROL
//---------------

enum _FrameControlType_t {
	FRAME_CONTROL_TYPE_INVALID,
	FRAME_CONTROL_TYPE_HALF,
	FRAME_CONTROL_TYPE_FULL,
	FRAME_CONTROL_TYPE_MAX
};

// AE Frame Control
//------------------

enum _AePreCaputreTrigger_t {
	AE_PRE_CAPTURE_TRIGGER_IDLE,
	AE_PRE_CAPTURE_TRIGGER_START,
	AE_PRE_CAPTURE_TRIGGER_CANCEL,
	AE_PRE_CAPTURE_TRIGGER_MAX
};

struct _FrameControlAe_t {
	enum _AeMode_t                mode;// CONTROL_AE_MODE
	enum _LockType_t              lockType;// CONTROL_AE_LOCK
	struct _AaaRegion_t             region;// CONTROL_AE_REGIONS
	enum _AeFlickerType_t         flickerType;
	// CONTROL_AE_ANTIBANDING_MODE
	// CONTROL_AE_EXPOSURE_COMPENSATION
	struct _AeEv_t                  evCompensation;
	struct _Range_t                 fpsRange;
	// CONTROL_AE_TARGET_FPS_RANGE
	// CONTROL_AE_PRECAPTURE_TRIGGER
	enum _AePreCaputreTrigger_t   precaptureTrigger;
	struct _Range_t                 iTimeRange;// unit is microsecond
	struct _Range_t                 isoRange;
	unsigned int                  ispGain;
	// CONTROL_POST_RAW_SENSITIVITY_BOOST
	unsigned int                  iTime;// SENSOR_EXPOSURE_TIME
	unsigned int                  aGain;// SENSOR_SENSITIVITY
	unsigned int                  frameDuration;
	unsigned int                  touchTarget;       //touchTarget
	int                           touchTargetWeight; //touchTargetWeight
	// SENSOR_FRAME_DURATION
};


// AWB Frame Control
//------------------
struct _FrameControlAwb_t {
	enum _AwbMode_t mode;// CONTROL_AWB_MODE
	enum _AwbLightSource_t lightSource;// CONTROL_AWB_MODE
	unsigned int colortemperature;
	enum _LockType_t lockType;// CONTROL_AWB_LOCK
	struct _WbGain_t wbGain; // COLOR_CORRECTION_GAINS
	struct _CcMatrix_t CCM; //COLOR_CORRECTION_TRANSFORM
};

// AF Frame Control
//------------------
struct _LensPos_t {
	enum _LensPosUnit_t	unit;
	float	position;
};

struct _FrameControlAf_t {
	enum _AfMode_t	mode;
	enum _LockType_t	lockType;
	struct _AaaRegion_t	region;
	//diopters in cvip case
	struct _LensPos_t	lensPos;
};

struct _LensMetaInfo_t {
	enum _LensState_t lensState;
	//focus range
	float   focusRangeNear;
	float   focusRangeFar;
};

enum _FrameType_t {
	FRAME_TYPE_INVALID  = 0,
	FRAME_TYPE_RGBBAYER = 1,
	FRAME_TYPE_IR       = 2,
	FRAME_TYPE_MAX
};

struct _OutputBuf_t {
	int enabled;
	struct _Buffer_t buffer;
	struct _ImageProp_t imageProp;
};

enum _BlcLockState_t {
	BLC_LOCK_STATE_INVALID,
	BLC_LOCK_STATE_LOCKED,
	BLC_LOCK_STATE_UNLOCKED,
	BLC_LOCK_STATE_MAX
};

struct _FrameControlBlc_t {
	enum _BlcLockState_t lockState;
};

struct _FrameControlDpc_t {
	int	enabled;
};

struct _MetaInfoDpc_t {
	int  enabled;
};

struct _FrameControlLsc_t {
	bool  enabled;
};

struct _MetaInfoLsc_t {
	bool	enabled;
};

struct _FrameControlCac_t {
	int enabled;
};

struct _FrameControlSharpen_t {
	int enabled;
};

#define NR_STRENGTH_MAX    (10)
#define NR_STRENGTH_MIN    (1)

struct _FrameControlNr_t {
	unsigned char  strength;                        //Android range[1-10]
	int enabled;
};

struct _FrameControlCrop_t {
	struct _Window_t window;
};

#define CURVE_POINTS_NUM    (129)
#define CURVE_VALUE_MAX     (0XFFF)
#define GAMMA_VALUE_MAX     (5.0)
#define GAMMA_VALUE_MIN     (1.0)

enum _TONEMAP_MODE {
	TONEMAP_TYPE_CONTRAST_CURVE,
	TONEMAP_TYPE_FAST,
	TONEMAP_TYPE_PRESET_CURVE,
	TONEMAP_TYPE_GAMMA
};

enum _TONEMAP_PRESET_CURVE_TYPE {
	TONEMAP_PRESET_CURVE_TYPE_REC709,
	TONEMAP_PRESET_CURVE_TYPE_SRGB
};

struct _FrameControlGamma_t {
	enum _TONEMAP_MODE                mode;//android.tonemap.mode
	float                       gamma;//android.tonemap.gamma
	//android.tonemap.curveRed
	unsigned short                      curveRed[CURVE_POINTS_NUM];
	//android.tonemap.curveGreen
	unsigned short                      curveGreen[CURVE_POINTS_NUM];
	//android.tonemap.curveBlue
	unsigned short                      curveBlue[CURVE_POINTS_NUM];
	//android.tonemap.presetCurve
	enum _TONEMAP_PRESET_CURVE_TYPE   presetCurve;
};

struct _FrameControlImageEffect_t {
	enum _IeMode_t    mode;
	//android.control.effectMode
	int      enabled;
	//disabled as effectMode=OFF
};

struct _FrameControlCaptureIntent_t {
	enum _ScenarioMode_t mode;
};

struct _MetaInfoCaptureIntent_t {
	enum _ScenarioMode_t mode;
};

struct _FrameControlSceneMode_t {
	enum _SceneMode_t mode;               //android.control.sceneMode
};

struct _FrameControlStatistics_t {
	int histogramEnabled;        //AE Short histogram
	int afEnabled;               //AF metering
	bool lensShadingMapEnabled;   //android.statistics.lensShadingMap
};

struct _FrameControlEngineerMode_t {
	int          enabled;
	unsigned int          bufBaseLo;
	unsigned int          bufBaseHi;
	unsigned int          bufSize;
};

enum _FrameControlBitmask_t {
	FC_BIT_AWB_MODE = 0x0000000000000001ull, //android.control.awbMode
	//android.control.awbMode
	FC_BIT_AWB_LIGHT_SOURCE = 0x0000000000000002ull,
	FC_BIT_AWB_LOCK = 0x0000000000000004ull,//android.control.awbLock
	FC_BIT_AWB_GAIN = 0x0000000000000008ull,//android.colorCorrection.gains
	//android.colorCorrection.transform
	FC_BIT_AWB_CCM = 0x0000000000000010ull,
	FC_BIT_AE_MODE = 0x0000000000000020ull, //android.control.aeMode
	//android.control.aeAntibandingMode
	FC_BIT_AE_FLIKER = 0x0000000000000040ull,
	FC_BIT_AE_REGIONS = 0x0000000000000080ull,//android.control.aeRegions
	FC_BIT_AE_LOCK = 0x0000000000000100ull, //android.control.aeLock
	//android.control.aeExposureCompensation
	FC_BIT_AE_EV_COMPENSATION       = 0x0000000000000200ull,
	//android.control.aeTargetFpsRange
	FC_BIT_AE_FPS_RANGE = 0x0000000000000400ull,
	//android.control.aePrecaptureTrigger
	FC_BIT_AE_PRE_TRIGGER = 0x0000000000000800ull,
	FC_BIT_AE_ITIME_RANGE           = 0x0000000000001000ull,
	FC_BIT_AE_ISO_RANGE             = 0x0000000000002000ull,
	//android.control.postRawSensitivityBoost
	FC_BIT_AE_ISP_GAIN              = 0x0000000000004000ull,
	FC_BIT_AE_ITIME = 0x0000000000008000ull,//android.sensor.exposureTime
	FC_BIT_AE_AGAIN = 0x0000000000010000ull, //android.sensor.sensitivity
	//android.sensor.frameDuration
	FC_BIT_AE_FRAME_DURATION        = 0x0000000000020000ull,
	FC_BIT_AF_MODE                  = 0x0000000000040000ull,
	FC_BIT_AF_LOCK                  = 0x0000000000080000ull,
	FC_BIT_AF_REGION                = 0x0000000000100000ull,
	FC_BIT_SCENE_MODE = 0x0000000000200000ull, //android.control.sceneMode
	FC_BIT_BLC = 0x0000000000400000ull, //android.blackLevel.lock
	FC_BIT_DPC = 0x0000000000800000ull, //android.hotPixel.mode
	FC_BIT_LSC = 0x0000000001000000ull, //android.shading.mode
	//android.colorCorrection.aberrationMode
	FC_BIT_CAC = 0x0000000002000000ull,
	FC_BIT_SHARPEN = 0x0000000004000000ull, //android.edge.mode
	FC_BIT_NR = 0x0000000008000000ull, //android.noiseReduction.mode
	FC_BIT_CROP = 0x0000000010000000ull, //android.scaler.cropRegion
	//android.tonemap.mode
	FC_BIT_GAMMA                    = 0x0000000020000000ull,
	//android.control.effectMode
	FC_BIT_IE                       = 0x0000000040000000ull,
	//android.lens.focusDistance
	FC_BIT_LENS_POS                 = 0x0000000080000000ull,
	//amd.control.afMode
	FC_BIT_CVIP_AF_MODE             = 0x0000000100000000ull,
	//amd.control.lens_sweep_distance_range
	FC_BIT_CVIP_AF_LENS_RANGE       = 0x0000000200000000ull,
	//amd.control.afTrigger
	FC_BIT_CVIP_AF_TRIGGER          = 0x0000000400000000ull,
	//android.control.captureIntent
	FC_BIT_CAPTIRE_INTENT           = 0x0000000800000000ull,
	FC_BIT_AE_TOUCH_TARGET          = 0x0000001000000000ull,
	FC_BIT_AE_TOUCH_TARGET_WEIGHT   = 0x0000002000000000ull,
	//android.control.af.roi
	FC_BIT_CVIP_AF_ROI              = 0x0000004000000000ull,
	//android.lens.focusDistance
	FC_BIT_CVIP_AF_DISTANCE         = 0x0000008000000000ull,
};

#define PRIVAT_DATA_LEN 16
struct _FrameControlPrivateData_t {
	unsigned char    data[PRIVAT_DATA_LEN];
};

/** @enum _TdbSwitchExpoLimit_t
 * Expo limit define for privateData.data[1]
 */
enum _TdbSwitchExpoLimit_t {
	SWITCH_LIMIT_NO_SWITCH = 0, ///< No switch
	SWITCH_LIMIT_LONG = 1,     ///< Switch to long expoure
	SWITCH_LIMIT_SHORT         ///< Switch to short expoure
};

#define CVIP_AF_ROI_MAX_NUMBER (3)

struct _CvipAfRoiWindow_t {
	unsigned int xmin; /* the minimal x value of ROI*/
	unsigned int ymin; /* the minimal y value of ROI*/
	unsigned int xmax; /* the maximal x value of ROI*/
	unsigned int ymax; /* he maximal y value of ROI*/
	unsigned int weight; /* the weight of ROI*/
};

struct _CvipAfRoi_t {
	unsigned int numRoi; /* the number of effective RoI*/
	/* ROI coordinates*/
	struct _CvipAfRoiWindow_t roiAf[CVIP_AF_ROI_MAX_NUMBER];
};

enum _CvipAfMode_t {
	CVIP_AF_CONTROL_MODE_OFF, // CVIP AF Mode: Off
	CVIP_AF_CONTROL_MODE_AUTO, // CVIP AF Mode: Auto
	CVIP_AF_CONTROL_MODE_MACRO, // CVIP AF Mode: Macro
	CVIP_AF_CONTROL_MODE_CONTINUOUS_VIDEO, // CVIP AF Mode: Continuous video
	CVIP_AF_CONTROL_MODE_CONTINUOUS_PICTURE, // CVIP AF Mode: Continuous pic
};

enum _CvipAfTrigger_t {
	CVIP_AF_CONTROL_TRIGGER_IDLE, // CVIP AF Trigger: idle
	CVIP_AF_CONTROL_TRIGGER_START, // CVIP AF Trigger: start
	CVIP_AF_CONTROL_TRIGGER_CANCEL, // CVIP AF Trigger: cancel
};

enum _CvipAfState_t {
	CVIP_AF_STATE_INACTIVE, // CVIP AF state: inactive
	CVIP_AF_STATE_PASSIVE_SCAN, // CVIP AF state: passive scane
	CVIP_AF_STATE_PASSIVE_FOCUSED, // CVIP AF state: passive focused
	CVIP_AF_STATE_ACTIVE_SCAN, // CVIP AF state: active scan
	CVIP_AF_STATE_FOCUSED_LOCKED, // CVIP AF state: focused locked
	CVIP_AF_STATE_NOT_FOCUSED_LOCKED, // CVIP AF state: not focused locked
	CVIP_AF_STATE_PASSIVE_UNFOCUSED, // CVIP AF state: passive unfocused
};

enum _CvipAfSceneChange_t {
	CVIP_AF_SCENE_CHANGE_NOT_DETECTED, // CVIP AF Scene change: not detected
	CVIP_AF_SCENE_CHANGE_DETECTED, // CVIP AF Scene change: detected
};

// CVIP AF distance 24Bit : Q8.16
#define CVIP_AF_DISTANCE_FIX_POINT_BASE (1 << 16)
#define CVIP_AF_DISTANCE_FIX_POINT_MAX ((1 << 25) - 1)

struct _CvipAfData_t {
	enum _CvipAfMode_t mode; /* CVIP AF mode*/
	enum _CvipAfTrigger_t trigger; /* CVIP AF trigger*/
	struct _CvipAfRoi_t cvipAfRoi; /* CVIP AF RoI*/
	float distance; /* lens pos : diopters*/
	float distanceNear; /* vendor tag distance near, diopters*/
	float distanceFar; /* vendor tag distance far, diopters*/
};

enum _CvipLensState_t {
	CVIP_LENS_STATE_INVALID      = 0, /* CVIP_LENS_STATE_INVALID*/
	CVIP_LENS_STATE_SEARCHING    = 1, /* CVIP_LENS_STATE_SEARCHING*/
	CVIP_LENS_STATE_CONVERGED    = 2, /* CVIP_LENS_STATE_CONVERGED*/
	CVIP_LENS_STATE_MAX,              /* CVIP_LENS_STATE_CONVERGED*/
};

struct _CvipMetaAf_t {
	//Af
	enum _CvipAfMode_t mode; /* CVIP AF mode*/
	enum _CvipAfTrigger_t trigger; /* CVIP AF trigger*/
	enum _CvipAfState_t state; /* CVIP AF state*/
	enum _CvipAfSceneChange_t scene_change; /* CVIP AF scene change*/
	struct _CvipAfRoi_t cvipAfRoi; /* CVIP AF RoI*/
	unsigned int afFrameId; /* CVIP AF frame ID*/

	//lens
	enum _CvipLensState_t lensState; /* lensState*/
	float distance; /* lens pos : diopters */
	float distanceNear;  // distance near, diopters, vendor tag
	float distanceFar;   // distance far, diopters, vendor tag
	float focusRangeNear;     /* focusRangeNear diopters, android tag*/
	float focusRangeFar;      /* focusRangeFar diopters, android tag*/
};

struct _CvipStatsInfo_t {
	struct _AaaStatsData_t aaaStats; /* statistics data*/
	struct _CvipAfRoi_t cvipAfRoi; /* CVIP AF RoI*/
};

struct _FrameControlCvipSensor_t {
	struct _CvipAfData_t afData; /* the cvip af information in Pre data*/
};

//To be defined by driver
struct _FrameControl_t {
	enum _FrameControlType_t controlType;//full/half, android always full.
	unsigned long long tags;//FC_BITS_MASK to show which part has changed
	unsigned int                      fcId;       //frame control id
	struct _FrameControlCac_t cac;//android.colorCorrection.aberrationMode*
	struct _FrameControlAe_t            ae;         //android.control.ae*
	//android.control.af* for AMD af only
	struct _FrameControlAf_t            af;
	struct _FrameControlAwb_t           awb;        //android.control.awb*
	struct _FrameControlImageEffect_t   ie; //android.control.effectMode*
	// android.control.captureIntent*
	struct _FrameControlCaptureIntent_t captureIntent;
	struct _FrameControlSceneMode_t     sceneMode;
	struct _FrameControlSharpen_t       sharpen;    //android.edge.*
	struct _FrameControlDpc_t           dpc;        //android.hotPixel.*
	struct _FrameControlNr_t            nr; //android.noiseReduction.*
	struct _FrameControlCrop_t          crop; //android.scaler.cropRegion
	struct _FrameControlLsc_t           lsc;        //android.shading.*
	struct _FrameControlStatistics_t    statistic;  //android.statistics.*
	struct _FrameControlGamma_t       tonemap;    //android.tonemap.*
	struct _FrameControlBlc_t           blc;        //android.blackLevel.*
	//android.control.af.* for CVIP af only
	struct _FrameControlCvipSensor_t    cvipSensor;
	struct _OutputBuf_t                 preview;    //pure ir
	struct _OutputBuf_t                 video;      //can be used as RGBIR
	struct _OutputBuf_t                 still;      //for yuv zsl
	struct _OutputBuf_t                 x86cv;
	//fw ignore cvp buffer address
	struct _OutputBuf_t                 cvpcv;
	struct _OutputBuf_t                 raw;
	struct _OutputBuf_t                 ir;
	//@Jean will define engineer api
	struct _FrameControlEngineerMode_t  engineerMode;
	//used by driver, fw not change it
	struct _FrameControlPrivateData_t   privateData;
	//driver need to malloc,fw to fill
	struct _Buffer_t                    metadata;
};



//  FrameInfo
//-------------
enum _AaaIrStatus_t {
	AAA_IR_STATUS_INVALID   = 0,
	AAA_IR_STATUS_ON        = 1,
	AAA_IR_STATUS_OFF       = 2,
	AAA_IR_STATUS_AUTO_ON   = 3,
	AAA_IR_STATUS_AUTO_OFF  = 4,
	AAA_IR_STATUS_MAX       = 5
};

struct _AwbmResult_t {
	unsigned int  whiteCnt;
	unsigned char   meanY_G;
	unsigned char   meanCb_B;
	unsigned char   meanCr_R;
};


#define HIST_NUM_BINS (16)
struct _HistResult_t {
	unsigned int bins[HIST_NUM_BINS];
};


struct _AwbCtrlInfo_t {
	enum _AwbMode_t mode;
	enum _AwbLockState_t lockState;
	enum _AwbSearchState_t searchState;
	struct _CcMatrix_t ccMatrix;
	struct _WbGain_t wbGain;
	enum _AwbLightSource_t lightSource;
	unsigned int colorTemperature;
	struct _AaaRegion_t region;
	unsigned short targetRgain;
	unsigned short targetGgain;
	unsigned short targetBgain;
	unsigned short targetColorTemperature;
	short  targetCCM[9];
	unsigned short lvx100;
	struct _Window_t faceWin[AAA_REGIONS_MAX];
};

struct _ExpResult_t {
	unsigned char luma[25];
};

enum _IRIlluStatus_t {
	IR_ILLU_STATUS_UNKNOWN,
	IR_ILLU_STATUS_ON,
	IR_ILLU_STATUS_OFF,
	IR_ILLU_STATUS_MAX,
};

struct _IRMetaInfo_t {
	enum _IRIlluStatus_t iRIlluStatus;
};

#define MAX_EXPO_DEPTH 3
struct _FrameExpInfo_t {
	int expoDepth;// Depth of exposure
	// The real itime setting for sensor, unit is exposure line
	unsigned int iTimeSetting[MAX_EXPO_DEPTH];
	// The real again setting for sensor, register-related value
	unsigned int aGainSetting[MAX_EXPO_DEPTH];
	// Reserved because we have not idea how convert gain value into real
	// register setting yet. Values MAX_EXPO_DEPTH be 1000 always
	unsigned int dGainSetting[MAX_EXPO_DEPTH];
	// The real duration setting for sensor, unit is exposure line.
	// sesnor driver should handled the cause when itime is bigger
	// than duration.
	unsigned int frameDurationSetting;

	unsigned int aePairIndex;                 // Index for AE pairing
};
struct _CvipFrameExpInfo_t {
	bool	isValid;
	struct _FrameExpInfo_t	expInfo;
};

struct _IridixCtrlInfo_t {
	int iridixEnable;
	unsigned short iridixGlobalDg;
	int blackLevelAmp;
	unsigned char statMult;
	unsigned char varianceSpace;
	unsigned char varianceIntensity;
	unsigned int blackLevel;
	unsigned int whiteLevel;
	unsigned short collectionCorrection;
	unsigned char fwdPerceptControl;
	unsigned char revPerceptControl;
	unsigned short strengthInroi;
	unsigned short strengthOutroi;
	unsigned short horRoiStart;
	unsigned short horRoiEnd;
	unsigned short verRoiStart;
	unsigned short verRoiEnd;
	unsigned char svariance;
	unsigned char brightPr;
	unsigned char contrast;
	unsigned short darkEnh;
	unsigned short contextNo;
	unsigned int asymmetryLUT[65];
};

struct _IridixStatus_t {
	enum _IridixMode_t mode;
	struct _IridixCtrlInfo_t ctrlInfo;
};

struct _AeCtrlInfo_t {
	// Status
	enum _AeMode_t mode;
	enum _AeApplyMode_t modeApply;
	enum _AeLockState_t lockState;
	enum _AeSearchState_t searchState;
	enum _AeFlickerType_t flickerType;

	// Algo-relatived info
	float effectEv[MAX_EXPO_DEPTH];
	int deltaEv[MAX_EXPO_DEPTH];
	struct _AeEv_t compensateEv;
	int LV;
	unsigned int CWV;
	unsigned int CWVSe;
	unsigned int equivalentIso;
	unsigned int aGainIso;
	unsigned int ispGainIso;

	unsigned int expoDepth;

	// normal setting
	unsigned int itime[MAX_EXPO_DEPTH];
	unsigned int aGain[MAX_EXPO_DEPTH];
	unsigned int ispGain[MAX_EXPO_DEPTH];
	unsigned int dGain[MAX_EXPO_DEPTH];
	unsigned int frameDuration;

	struct _IridixCtrlInfo_t iridixCtrlInfo;
	struct _AaaRegion_t region;
	bool isFace;
	unsigned int faceY;
	unsigned int touchTarget;//target of AE touch ROI
	int touchTargetWeight;//target weight of AE touch ROI

	enum _ScenarioMode_t scenarioMode;
	enum _SceneMode_t sceneMode;
};

struct _AfCtrlInfo_t {
	//For ctrl and meta data
	enum _AfLockState_t lockState;
	int bSupportFocus;
	unsigned int lensPos;
//	struct _LensRange_t lensRange;
	//Indicate whether to turn on the flash light for the current frame
	int focusAssistOn;
	//Indicate the flash power level for the current frame.
	unsigned int focusAssistPowerLevel;

	//For meta data
	enum _AfMode_t mode;
	enum _FocusAssistMode_t focusAssistMode;
	enum _AfSearchState_t searchState;

	//For metering configuration
	unsigned int statHoriZone;
	unsigned int statVertZone;
	unsigned int statKernel;
	struct _AaaRegion_t region;
	enum _AfSearchRangeMode_t searchRangeMode;
	enum _AfCalibrationType_t calibType;
};


struct _LscCtrlInfo_t {
	unsigned char meshAlphaMode;
	unsigned char meshScale;
	unsigned char meshWidth;
	unsigned char meshHeight;
	struct _CdbCctThreshold_t	cctThreshold;
	struct _CdbMeshShadingStrength_t
		meshShadingStrength;
	unsigned short	appliedStrength;
	unsigned short	previousStrength;
	unsigned int log2GainPreviousStrength;
	struct _CdbMeshShadingSmoothRatio_t smoothRatio;
	struct _CdbShadingChannel_t	shading;
	struct _CdbRadialShading_t	radialShading;
	struct _TdbPreLscGlobal_t	lscGlobal;
	struct _TdbPreLscMatrix_t	lscMatrix;
};

enum _BufferStatus_t {
	BUFFER_STATUS_INVALID,
	BUFFER_STATUS_SKIPPED, //The buffer is not filled with image data
	BUFFER_STATUS_EXIST,   //The buffer is exist and waiting for filled.
	BUFFER_STATUS_DONE,    //The buffer is filled with image data
	BUFFER_STATUS_LACK,    //The buffer is unavaiable
	//The buffer is dirty, probably caused by LMI leakage
	BUFFER_STATUS_DIRTY,
	BUFFER_STATUS_MAX
};


enum _BufferSource_t {
	BUFFER_SOURCE_INVALID,
	BUFFER_SOURCE_CMD_CAPTURE,  //The buffer is from a capture command
	//The buffer is from a per frame control command
	BUFFER_SOURCE_FRAME_CONTROL,
	BUFFER_SOURCE_STREAM, //The buffer is from the stream buffer queue.
	//The buffer is added by host post process in mode3 post frame info.
	BUFFER_SOURCE_HOST_POST,
	BUFFER_SOURCE_TEMP,
	BUFFER_SOURCE_RRECYCLE,
	BUFFER_SOURCE_CVIP,
	BUFFER_SOURCE_CRISP,
	BUFFER_SOURCE_MAX
};


struct _BufferFrameInfo_t {
	int          enabled;
	enum _BufferStatus_t  status;
	struct _ErrorCode_t     err;
	enum _BufferSource_t  source;
	struct _ImageProp_t     imageProp;
	struct _Buffer_t        buffer;
	//save Stg2OutChannelId_t relevant to CdbIspPipeOutCh_t
	//for scaler module
	unsigned char           chId;
};


struct _CprocStatus_t {
	int enabled;
	unsigned short contrast;
	short brightness;
	unsigned short saturation;
	char hue;
};

/*Digital Gain*/
struct _DgHdrConfig_t {
	unsigned short lowDigitalGain;
	unsigned short highDigitalGain;
};

struct _DgHdrStatus_t {
	int enabled;
	struct _DgHdrConfig_t dgHdrConfig;
};

/*mipi bayer scaler status*/
struct _MipiBrszStatus_t {
	int mipiBrszEn;
	struct _Window_t mipiBrszOutWin;
};

/*YUV NR Struct*/
struct _YuvNrConfig_t {
	// unsigned short  yuvNrImageWidth;
	// unsigned short  yuvNrImageHeight;
	struct _TdbYuvNrY_t   yuvNrY;
	struct _TdbYuvNrUv_t  yuvNrUv;
};

struct _YuvNrStatus_t {
	int enabled;
	struct _YuvNrConfig_t yuvNrConfig;
};

#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
enum GammaMode_t {
	GAMMA_MODE_INVALID,
	GAMMA_MODE_MANUAL,
	GAMMA_MODE_AUTO,
	GAMMA_MODE_MAX
};
#endif


struct _GammaStatus_t {
	int              enabled;
	enum _GammaSegMode_t      segMode;
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	unsigned char               activeGammaCurveNum;
	GammaMode_t         gammaMode;
	GammaCurve_t        curveResult;
	GammaCurve_t        curve[TDB_GAMMA_CURVE_MAX_NUM];
	unsigned int              exposureTheshold[TDB_GAMMA_CURVE_MAX_NUM - 1];
	float               dampingCoef;
#else
	struct _GammaCurve_t        curve;
#endif
};


struct _SnrStatus_t {
	int snrEnabled;
	struct _TdbSnr_t tdbSnr;
};

struct _TnrStatus_t {
	int tnrEnabled;
	struct _TdbTnr_t tdbTnr;
};


struct _BlsStatus_t {
	int enabled;
	enum _BlsMode_t mode;
#ifdef CONFIG_ENABLE_TDB_V2_0
	struct _PreBlackLevel_t level;
#else
	BlackLevel_t blackLevel;
#endif
	struct _Window_t wind1;
	struct _Window_t wind2;
};

struct _MetaInfoBlc_t {
	enum _BlcLockState_t  lockState;
	struct _BlackLevel_t    blackLevel;
	unsigned int          whiteLevel;
	struct _Window_t        wind1;
	struct _Window_t        wind2;
};

struct _DegammaStatus_t {
	int enabled;
	struct _DegammaCurve_t curve;
};


enum _DpfGainUsage_t {
	DPF_GAIN_USAGE_INVALID        = 0,
	DPF_GAIN_USAGE_DISABLED       = 1,
	DPF_GAIN_USAGE_NF_GAINS       = 2,
	DPF_GAIN_USAGE_LSC_GAINS      = 3,
	DPF_GAIN_USAGE_NF_LSC_GAINS   = 4,
	DPF_GAIN_USAGE_AWB_GAINS      = 5,
	DPF_GAIN_USAGE_AWB_LSC_GAINS  = 6,
	DPF_GAIN_USAGE_MAX
};


enum _DpfRBFilterSize_t {
	DPF_RB_FILTER_SIZE_INVLAID = 0,
	DPF_RB_FILTER_SIZE_9x9     = 1,
	DPF_RB_FILTER_SIZE_13x9    = 2,
	DPF_RB_FILTER_SIZE_MAX
};


#define DPF_MAX_SPATIAL_COEFFS (6)
struct _DpfSpatial_t {
	unsigned char weightCoeff[DPF_MAX_SPATIAL_COEFFS];
};


struct _DpfNfGain_t {
	unsigned short red;
	unsigned short greenR;
	unsigned short greenB;
	unsigned short blue;
};


struct _DpfInvStrength_t {
	unsigned char weightR;
	unsigned char weightG;
	unsigned char weightB;
};


enum _DpfNoiseLevelLookUpScale_t {
	DPF_NOISE_LEVEL_LOOK_UP_SCALE_LINEAR      = 0,
	DPF_NOISE_LEVEL_LOOK_UP_SCALE_LOGARITHMIC = 1,
};


#define DPF_MAX_NLF_COEFFS  (17)
struct _DpfNoiseLevelLookUp_t {
	unsigned short nllCoeff[DPF_MAX_NLF_COEFFS];
	enum _DpfNoiseLevelLookUpScale_t xScale;
};


struct _DpfSubConfig_t {
	enum _DpfGainUsage_t gainUsage;
	enum _DpfRBFilterSize_t rbFilterSize;

	int processRedPixel;
	int processGreenRPixel;
	int processGreenBPixel;
	int processBluePixel;

	struct _DpfSpatial_t spatialG;
	struct _DpfSpatial_t spatialRB;
};


struct _DpfConfig_t {
	struct _DpfSubConfig_t subConfig;
	struct _DpfNfGain_t nfGain;
	struct _DpfInvStrength_t dynInvStrength;
	struct _DpfNoiseLevelLookUp_t nll;

	float gradient;
	float offset;
};


struct _DpfStatus_t {
	int      enabled;
	struct _DpfConfig_t dpfConfig;
};


struct _DpccStaticConfig_t {
	unsigned int isp_dpcc_mode;
	unsigned int isp_dpcc_output_mode;
	unsigned int isp_dpcc_set_use;

	unsigned int isp_dpcc_methods_set_1;
	unsigned int isp_dpcc_methods_set_2;
	unsigned int isp_dpcc_methods_set_3;

	unsigned int isp_dpcc_line_thresh_1;
	unsigned int isp_dpcc_line_mad_fac_1;
	unsigned int isp_dpcc_pg_fac_1;
	unsigned int isp_dpcc_rnd_thresh_1;
	unsigned int isp_dpcc_rg_fac_1;

	unsigned int isp_dpcc_line_thresh_2;
	unsigned int isp_dpcc_line_mad_fac_2;
	unsigned int isp_dpcc_pg_fac_2;
	unsigned int isp_dpcc_rnd_thresh_2;
	unsigned int isp_dpcc_rg_fac_2;

	unsigned int isp_dpcc_line_thresh_3;
	unsigned int isp_dpcc_line_mad_fac_3;
	unsigned int isp_dpcc_pg_fac_3;
	unsigned int isp_dpcc_rnd_thresh_3;
	unsigned int isp_dpcc_rg_fac_3;

	unsigned int isp_dpcc_ro_limits;
	unsigned int isp_dpcc_rnd_offs;
};

struct _DpccStatus_t {
	int              enabled;
	struct _TdbDpcc_t           tdbDpcc;
};


enum _CacHClipMode_t {
	CAC_H_CLIP_MODE_FIX4,
	CAC_H_CLIP_MODE_DYN5,
	CAC_H_CLIP_MODE_MAX
};


enum _CacVClipMode_t {
	CAC_V_CLIP_MODE_FIX2,
	CAC_V_CLIP_MODE_FIX3,
	CAC_V_CLIP_MODE_DYN4,
	CAC_V_CLIP_MODE_MAX
};


struct _CacConfig_t {
	unsigned short          width;
	unsigned short          height;
	short           hCenterOffset;
	short           vCenterOffset;
	enum _CacHClipMode_t  hClipMode;
	enum _CacVClipMode_t  vClipMode;
	unsigned short          aBlue;
	unsigned short          aRed;
	unsigned short          bBlue;
	unsigned short          bRed;
	unsigned short          cBlue;
	unsigned short          cRed;
	unsigned char           xNs;
	unsigned char           xNf;
	unsigned char           yNs;
	unsigned char           yNf;
};


struct _CacStatus_t {
	int      enabled;
	struct _TdbCac_t    tdbCac;
};


#define WDR_DX_SIZE (33)
#define WDR_DY_SIZE (33)
struct _WdrCurve_t {
	unsigned short dx[WDR_DX_SIZE];
	unsigned short dy[WDR_DY_SIZE];
};



struct _WdrStatus_t {
	int     enabled;
	struct _WdrCurve_t curve;
};


struct _CnrParam_t {
	unsigned int threshold1;
	unsigned int threshold2;
};

struct _CnrStatus_t {
	int enabled;
	struct _CnrParam_t param;
};


struct _DemosaicThresh_t {
	unsigned char thresh;
};


struct _DemosaicStatus_t {
	unsigned char thresh;
	int enabled;
};


struct _DenoiseFilter_t {
	short coeff[TDB_ISP_FILTER_DENOISE_SIZE];
};


struct _SharpenFilter_t {
	char coeff[TDB_ISP_FILTER_SHARP_SIZE];
};

struct _IspFilterStatus_t {
	int enabled;
	struct _TdbIspFilter_t tdbIspFilter;
	int hostSetDenoiseCurve;
	int hostSetSharpenCurve;
};

struct _YuvCapCmd_t {
	struct _Cmd_t       cmd;
	struct _ImageProp_t imageProp;
	struct _Buffer_t    buffer;
};

struct _RawCapCmd_t {
	struct _Cmd_t       cmd;
	int      bNeedFull;
	struct _Buffer_t    buffer;
};

struct _FullYuvCapCmd_t {
	struct _Cmd_t                   cmd;
	enum _CaptureFullYuvType_t    captureType;
	struct _ImageProp_t             imageProp;
	struct _Buffer_t                buffer;
};

struct _FrameControlCmd_t {
	// The host commamnd structure which
	// send this FrameControlCmd_t
	struct _Cmd_t          cmd;
	struct _FrameControl_t frameControl;
};

//struct _Mode3FrameInfo_t
struct _FrameInfo_t {
	unsigned int              poc;
	unsigned int              timeStampLo;
	unsigned int              timeStampHi;

	//Yuv Capture Command
	int              bEnableYuvCap;
	struct _YuvCapCmd_t         yuvCapCmd;

	//Raw Capture Command
	int              bEnableRawCap;
	struct _RawCapCmd_t         rawCapCmd;

	struct _Window_t            zoomWindow;
	int              bHasZoomCmd;
	struct _Cmd_t               zoomCmd;

	unsigned int              frameRate;
	//Actual framerate multiplied by 1000

	struct _AfCtrlInfo_t        afCtrlInfo;
	// This is temporary solution for loopback.
	// Current IspPipeMgr does not set ISP
	// setting properly, such as
	// AwbCtrlInfo_t.
	// struct _AeCtrlInfo_t is pure-ISP setting now,
	// we may review loopback and
	// change aeCtrlInfoTemporary too.
	struct _AeCtrlInfo_t        aeCtrlInfoTemporary;
	struct _BufferFrameInfo_t   rawBufferFrameInfo;
	enum _RawPktFmt_t         rawPktFmt;
	unsigned int              rawWidthInByte;
	enum _CFAPattern_t        cfaPattern;
	unsigned int              dolNum;
	enum _FrameType_t         frameType;
	enum _IRIlluStatus_t      iRIlluStatus;
	struct _FrameExpInfo_t      frameExpInfo;

	int              bEnableFrameControl;
	struct _FrameControlCmd_t   frameControlCmd;

	int              bEnableZzHdr;
	enum _ZzhdrPattern_t      zzPattern;
	enum _HdrType_t           hdrType;
	unsigned int              zzRatio;
};

// MetaInfo
//-----------
struct _MetaCrc_t {
	unsigned int crc[8];
};

struct _BufferMetaInfo_t {
	int enabled;
	enum _BufferStatus_t  status;
	struct _ErrorCode_t     err;
	enum _BufferSource_t source;
	struct _ImageProp_t imageProp;
	struct _Buffer_t buffer;
	struct _MetaCrc_t wdmaCrc;
};

struct _AeMetaInfo_t {
	enum _AeMode_t        mode;
	enum _AeApplyMode_t   modeApply;
	enum _AeFlickerType_t flickerType;
	struct _AaaRegion_t     region;
	enum _AeLockState_t   lockState;
	enum _AeSearchState_t searchState;
	unsigned int          iso;
	unsigned int          aGainIso;
	unsigned int          ispGainIso;
	struct _AeEv_t          compensateEv;
	int          hdrEnabled;
	unsigned int          expoDepth;
	unsigned int itime[MAX_EXPO_DEPTH];
	unsigned int aGain[MAX_EXPO_DEPTH];
	unsigned int ispGain[MAX_EXPO_DEPTH];
	unsigned int dGain[MAX_EXPO_DEPTH];
	unsigned int          frameDuration;
	unsigned int          touchTarget;//touchTarget
	int          touchTargetWeight;//touchTargetWeight
};

struct _AwbMetaInfo_t {
	enum _AwbMode_t           mode;
	enum _AwbLockState_t      lockState;
	struct _WbGain_t            wbGain;
	struct _CcMatrix_t          ccMatrix;
	enum _AwbSearchState_t    searchState;
	unsigned int colorTemperature;
	enum _AwbLightSource_t    lightSource;
};

struct _AfMetaInfo_t {
	enum _AfMode_t mode;
	unsigned int lensPos;
	struct _AaaRegion_t region;
	enum _AfLockState_t lockState;
	enum _AfSearchState_t searchState;
};

#define AE_HIST_STATS_SIZE       (512*4*3)
#define AF_STATS_SIZE             (33*33*8)

struct _StatisticsHistogram_t {
	unsigned char aeHistogram[AE_HIST_STATS_SIZE];
};

struct _StatisticsAfMetering_t {
	unsigned char metering[AF_STATS_SIZE];
};

struct _MetaInfoStatistics_t {
	struct _FrameControlStatistics_t  mode;
	//android.statistics.histogram
	IDMA_COPY_ADDR_ALIGN struct _StatisticsHistogram_t histogram;
	//android.statistics.sharpnessMap
	IDMA_COPY_ADDR_ALIGN struct _StatisticsAfMetering_t af;
	//android.statistics.lensShadingCorrectionMap
	struct _StatisticsLscMap_t	lscMap;
};

struct _MetaInfoCrispPostVSMDResult_t {
	float mVSMD[6];
};
struct _MetaInfoCrispPostVSInfo {
	int mEnabled;
	struct _MetaInfoCrispPostVSMDResult_t mVSMDResult;
};

struct _MetaInfoCrispPostFDResult_t {
	float mClass;
	float mScore;
	float mX1;
	float mY1;
	float mX2;
	float mY2;
};
#define CRISP_POST_FD_RESULT_NUM  (100)
struct _MetaInfoCrispPostFDInfo_t {
	int mEnabled;
	struct _MetaInfoCrispPostFDResult_t
	mFDResult[CRISP_POST_FD_RESULT_NUM];
};

struct _MetaInfoCrispPostSTDResult_t {
	float mSkin;
	float mScore;
	float mX1;
	float mY1;
	float mX2;
	float mY2;
};

#define CRISP_POST_STD_RESULT_NUM  (5)

struct _MetaInfoCrispPostSTDInfo_t {
	int mEnabled;
	struct _MetaInfoCrispPostSTDResult_t
	mSTDResult[CRISP_POST_STD_RESULT_NUM];
};

struct _MetaInfoCrispPostSDResult_t {
	float mScoreIndoor;
	float mScoreOutdoorDay;
	float mSorceOutdoorNight;
};
struct _MetaInfoCrispPostSDInfo_t {
	int mEnabled;
	struct _MetaInfoCrispPostSDResult_t mSTDResult;
};

struct _MetaInfoCrispPostInfo_t {
	struct _MetaInfoCrispPostVSInfo mVSInfo;
	struct _MetaInfoCrispPostFDInfo_t mFDInfo;
	struct _MetaInfoCrispPostSTDInfo_t mSTDInfo;
	struct _MetaInfoCrispPostSDInfo_t mSDInfo;
};

// enum _MetaBufferSrc_t {
//     META_BUFFER_SOURCE_ISP = 0,
//     META_BUFFER_SOURCE_CRISP = 1,
//     META_BUFFER_SOURCE_MAX
// };

struct _CvipExpData_t {
	//The real itime setting for sensor, unit is exposure line
	unsigned int iTime;
	//The real again setting for sensor, register-related value
	// For S0ny sensor, it is the value of sensor register ANA_GAIN_GLOBAL
	//(ex. reg 0x204/205 of IMX577)
	//For OV sensor, it is the value of sensor register LONG_GAIN
	//(ex. reg 0x03508/3509 of OV13858)
	// for Samsung sensor, it is the value of sensor register
	//analogue_gain_code_global (ex. reg 0x0204/205 of S573L6XX)
	//Reserved because we have not idea how convert gain value into real
	//register setting yet. Values will be 1000 always
	unsigned int aGain;
	unsigned int dGain;
};

struct _CvipTimestampInPre_t {
	// readout start timestamp in us
	unsigned long long readoutStartTimestampUs;
	// centroid timestamp in us
	unsigned long long centroidTimestampUs;
	// seq win timestamp in us
	unsigned long long seqWinTimestampUs;
};

struct _PreData_t {
	//SensorEmbData_t sensorEmbData;
	//expData
	struct _CvipExpData_t expData;
	//Timestamp in Pre data
	struct _CvipTimestampInPre_t timestampInPre;
	//the cvip af information in Pre data
	struct _CvipMetaAf_t afMetaData;
};

struct _CvipTimestampInPost_t {
	//readout end timestamp in Us
	unsigned long long readoutEndTimestampUs;
};

struct _PostData_t {
	//Timestamp in post data
	struct _CvipTimestampInPost_t  timestampInPost;
};

struct _MetaInfo_t {
	unsigned int                      poc;        //frame id
	unsigned int                      fcId;       //frame ctl id
	unsigned int                      timeStampLo;
	unsigned int                      timeStampHi;
	struct _BufferMetaInfo_t            preview;
	struct _BufferMetaInfo_t            video;
	struct _BufferMetaInfo_t            still;      //for yuv zsl
	struct _BufferMetaInfo_t            fullStill;
	struct _BufferMetaInfo_t            cv;         //x86 cv
	struct _BufferMetaInfo_t            ir;         //x86 ir
	struct _BufferMetaInfo_t            raw;        //x86 raw
	struct _BufferMetaInfo_t            fullRaw;        //x86 raw
	struct _BufferMetaInfo_t            cvpcv;      //cvp cv
	int                      isFullYuvImgCfm; //only for the VG
	//The raw buffer packet format if the raw is exist
	enum _RawPktFmt_t                 rawPktFmt;
	struct _Window_t                    zoomWindow;
	struct _AeMetaInfo_t                ae;
	struct _AfMetaInfo_t                af;
	struct _PreData_t                   preData;
	struct _PostData_t                  postData;
	struct _LensMetaInfo_t              Lens;
	struct _AwbMetaInfo_t               awb;
	struct _IRMetaInfo_t                IRMeta;
	unsigned long long                      fcTags;
	//if frame contrl has applied
	float	bayerScalerRatio;   //driver requirement
	unsigned int	tdbIdx;//driver requirement

	struct _FrameControlSceneMode_t         sceneMode;
	struct _MetaInfoCaptureIntent_t     captureIntend;
	struct _MetaInfoBlc_t               blc;
	struct _FrameControlDpc_t               dpc;
	struct _FrameControlLsc_t               lsc;
	struct _FrameControlCac_t               cac;
	struct _FrameControlSharpen_t           sharpen;
	struct _FrameControlGamma_t           tonemap;
	struct _FrameControlNr_t                nr;
	struct _FrameControlCrop_t              crop;
	struct _FrameControlImageEffect_t       ie;
	struct _MetaInfoStatistics_t        statistics;

	struct _FrameControlPrivateData_t       privateData;
	struct _FrameControlEngineerMode_t      engineerMode;
};


/* Engineer meta data of 3A is the implemnetation of 3A
 * and it is a issue when we exposure the detail of
 * implementation in this header
 * A equivalent size of array is declared in EngineerMetaInfo_t
 * and 3A manager have static check to ensure the sizes are the
 * same as real structure
 */
#define ENGINEER_META_INFO_VERSION (3002211109U)

#define ENGINEER_META_INFO_AE_SIZE  (137708)
#define ENGINEER_META_INFO_AWB_SIZE (14112)
#define ENGINEER_META_INFO_AF_SIZE  (258056)
#define ENGINEER_META_INFO_AUTOCONTRAST_SIZE (5752)
#define ENGINEER_META_INFO_3DLUT_SIZE (14588)
#define ENGINEER_META_INFO_LSC_SIZE (147052)


#define ENGINEER_META_INFO_PREP_REG_SIZE (2560 * 4 * 2 + 4)
#define ENGINEER_META_INFO_CORE_REG_SIZE (113 * 1024)//actual is 115472 Bytes
#define ENGINEER_META_INFO_POST_REG_SIZE (800 * 4 * 2 + 4)

#define ENGINEER_META_INFO_POST_YUVNR_OUTPUT_SIZE  (128)

//replace size with exact figure
#define ENGINEER_META_INFO_PREP_POST_CFG_SIZE (26912 + 2 * 1024)

struct _EngineerCoreInfo_t {
	unsigned int rawWidth;
	unsigned int rawHeight;
	unsigned int log2Gain;
	unsigned int colorTemperature;
	struct _LscCtrlInfo_t LscCtrlInfo;
};

struct _EngineerMetaInfoDesc_t {
	unsigned int version;
	unsigned int sizeStatsData;
	unsigned int offsetStatsData;
	unsigned int sizeAe;
	unsigned int offsetAe;
	unsigned int sizeAwb;
	unsigned int offsetAwb;
	unsigned int sizeAf;
	unsigned int offsetAf;
	unsigned int sizeAutoContrast;
	unsigned int offsetAutoContrast;
	unsigned int sizeLut3D;
	unsigned int offsetLut3D;
	unsigned int sizeLsc;
	unsigned int offsetLsc;
	unsigned int sizePrepReg;
	unsigned int offsetPrepReg;
	unsigned int sizeCoreReg;
	unsigned int offsetCoreReg;
	unsigned int sizePostReg;
	unsigned int offsetPostReg;
	unsigned int sizePrepPostCfgParam;
	unsigned int offsetPrepPostCfgParam;
	unsigned int sizeYuvnrOutputInfo;
	unsigned int offsetYuvnrOutputInfo;
	unsigned int sizeCoreInfo;
	unsigned int offsetCoreInfo;
};

struct _EngineerMetaInfo_t {
	struct _EngineerMetaInfoDesc_t description;
	IDMA_COPY_ADDR_ALIGN struct _AaaStatsData_t statsData;

	unsigned char ae[ENGINEER_META_INFO_AE_SIZE];
	unsigned char awb[ENGINEER_META_INFO_AWB_SIZE];
	unsigned char af[ENGINEER_META_INFO_AF_SIZE];

	unsigned char autoContrast[ENGINEER_META_INFO_AUTOCONTRAST_SIZE];
	unsigned char lut3D[ENGINEER_META_INFO_3DLUT_SIZE];
	unsigned char lsc[ENGINEER_META_INFO_LSC_SIZE];
	unsigned char prepReg[ENGINEER_META_INFO_PREP_REG_SIZE];
	unsigned char coreReg[ENGINEER_META_INFO_CORE_REG_SIZE];
	unsigned char postReg[ENGINEER_META_INFO_POST_REG_SIZE];
	unsigned char prepPostCfgParam[ENGINEER_META_INFO_PREP_POST_CFG_SIZE];
	unsigned char yuvnrOutputInfo
				[ENGINEER_META_INFO_POST_YUVNR_OUTPUT_SIZE];
	struct _EngineerCoreInfo_t coreInfo;
};


/*The BufCheckSum_t is used to be the RESP_ID_CMD_DONE payload for the
 *command which use an indirect buffer to get the result, firmware will
 *use this kind payload to do the checksum for the buffer
 */
struct _BufCheckSum_t {
	unsigned int bufAddrLo;
	/* The low 32bit address of the indirect buffer */
	unsigned int bufAddrHi;
	/* The high 32bit address of the indirect buffer */
	unsigned int bufSize;
	/* The size of the indirect buffer */
	unsigned char bufCheckSum;
	/* The byte check sum of the indirect buffer */
};


/*For RGBIR or pure IR case, need to configure the illuminator.
 *The illuminator clock source: REF_CLK for SOC, ISP_CLK for FPGA.
 */
enum _IRIlluMode_t {
	IR_ILLU_MODE_INVALID,
	IR_ILLU_MODE_OFF,
	IR_ILLU_MODE_ON,
	IR_ILLU_MODE_AUTO,
	IR_ILLU_MODE_MAX
};

struct _IRIlluConfig_t {
	enum _IRIlluMode_t        mode;
	int              bUseDropFrameMethod;
	enum _SensorShutterType_t sensorShutterType;//get from sensor driver
	unsigned int              delay; /* in flash control clock count */
	unsigned int              time; /* in flash control clock count */
};

/*For SharpenConfig parameter, we use what in the sensorTdb.h directly
 *Instead of defining it again here
 */
enum _SharpenId_t {
	SHARPEN_ID_INVALID = -1,
	SHARPEN_ID_0,
	SHARPEN_ID_1,
	SHARPEN_ID_2,
	SHARPEN_ID_MAXIMUM,
};

enum _SharpenOutputMode_t {
	SHARPEN_OUTPUT_MODE_INVALID = -1,
	SHARPEN_OUTPUT_MODE_NORMAL,
	SHARPEN_OUTPUT_MODE_FRE,
	SHARPEN_OUTPUT_MODE_PEAKING,
	SHARPEN_OUTPUT_MODE_HIGH_PASS,
	SHARPEN_OUTPUT_MODE_BAND_PASS,
	SHARPEN_OUTPUT_MODE_MIX_BAND,
	SHARPEN_OUTPUT_MODE_Y_INK,
	SHARPEN_OUTPUT_MODE_Y_GAIN,
	SHARPEN_OUTPUT_MODE_SATURATE_PROTECT,
	SHARPEN_OUTPUT_MODE_AC_SMOOTH,
	SHARPEN_OUTPUT_MODE_MAGNITUDE_INK,
	SHARPEN_OUTPUT_MODE_DIRECTION_INK,
	SHARPEN_OUTPUT_MODE_MAX
};

struct _PostSharpenParam_t {
	struct _TdbPostSharp_t   sharpenParam;
};

struct _PostDitheringByChannel_t {
	int enabled;
};

struct _PostFclipByChannel_t {
	int enabled;
};

struct _PostDithFclipByChannel_t {
	struct _PostDitheringByChannel_t    dith;
	struct _PostFclipByChannel_t        fclip;
};

struct _PostDithFclipParam_t {
	struct _PostDithFclipByChannel_t dithFclip[CDB_ISP_PIPE_OUT_CH_MAX-1];
};


struct _PostHflipByChannel_t {
	int enabled;
};

struct _PostHflipParam_t {
	struct _PostHflipByChannel_t hflip[CDB_ISP_PIPE_OUT_CH_MAX-1];
};


struct _ZzhdrParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int hdrEn;
	int pdEn;
#endif
	struct _TdbZzhdr_t  zzhdrConfig;
};

struct _PdParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int     pdcEn;
	int     pdeEn;
	int     pdncEn;
	int     pdbpcEn;
	int     pdcipEn;
#endif
	struct _TdbPdc_t   pdcConfig;
	struct _TdbPde_t   pdeConfig;
	struct _TdbPdnc_t  pdncConfig;
	struct _TdbBpc_t   pdbpcConfig;
	struct _TdbPdcip_t pdcipConfig;
};

#ifdef CONFIG_ENABLE_TDB_V2_0
struct _DirParam_t {
	/*
	 * All unsigned short value is bitdepth = 16 equivalent,
	 * these values need not shift.
	 * 10-bit RAW image will be left shift 2 bits by hardware.
	 */

	/* Bad Pixel */
	unsigned short dpuTh;
	unsigned short dpaTh;

	/* Saturation */
	unsigned short rgbTh;
	unsigned short irTh;
	unsigned char diff;

	/* Decontamination */
	unsigned short rCoeff[9];
	unsigned short gCoeff[9];
	unsigned short bCoeff[9];
	unsigned short rgb2IrCoeffR;
	unsigned short rgb2IrCoeffG;
	unsigned short rgb2IrCoeffB;

	/* RGB Adjust */
	unsigned short rgbUth;
	unsigned short rbLth;
	unsigned char compRatio;
	unsigned char cutRatio;
	unsigned short arc;
	unsigned short agc;
	unsigned short abc;
	unsigned short arfc;
	unsigned short agfc;
	unsigned short abfc;

	/* Tolerance factor for interpolation */
	unsigned char rbTf;
	unsigned char gTf;
	unsigned char iTf;

	/* Color difference threshold for interpolation */
	unsigned short cdaTh;
	unsigned short cduTh;
};
#endif

struct _IeStatus_t {
	int enabled;
	struct _IeConfig_t status;
};

struct _DirStatus_t {
	int bpcEnabled;
	int deconEnabled;
	int cipEnabled;
	int rgbAdjustEnabled;
	int isDeconIr;
};

//TODO Use ConfigSensorCtrl_t and SensorProfile_t instead of
// ConfigCvipSensorParam_t
//struct _SensorProfile_t
//{
//    unsigned int width;   //Image width of image sensor output
//    unsigned int height;  //Image height of image sensor output
//    unsigned int fps;
//    RawBitDepth_t bitDepth;
//    HdrMode_t hdrMode;
//    //TODO  it will be added by Chiket
//};
//
//struct _ConfigSensorCtrl_t
//{
//    ResMode_t resMode;
//    TestPattern_t testPattern;
//    unsigned int dolNum;
//};

struct _ConfigCvipSensorParam_t {
	enum _ResMode_t resMode;
	unsigned int width;   //Image width of image sensor output
	unsigned int height;  //Image height of image sensor output
	unsigned int fps;
	enum _RawBitDepth_t bitDepth;
	enum _HdrMode_t hdrMode;
	enum _TestPattern_t testPattern;
	unsigned int dolNum;
};

struct _PrepH2BinParams_t {
#ifndef CONFIG_ENABLE_TDB_V2_0
	int enabled;
#endif
	struct _TdbHbin_t hBinWeight;
};

struct _PrepAeLsc_t {
	int enabled;
	struct _TdbPreLsc_t lsc;
};

struct _PrepAwbLsc_t {
	int enabled;
	struct _TdbPreLsc_t lsc;
};

struct _Guid_t {
	unsigned int guidData1;
	unsigned short guidData2;
	unsigned short guidData3;
	unsigned char  guidData4[8];
};

enum _MipiPathMode_t {
	MIPI_PATH_MODE_INVALID = -1,
	MIPI_PATH_MODE_0,
	MIPI_PATH_MODE_1,
	MIPI_PATH_MODE_MAX,
};

//FullYuvCaptureCmdFrameState_t
enum _FYCCmdState_t {
	FYC_CMD_STATE_INVALID = -1,
	FYC_CMD_STATE_FIRST_FRAME,
	FYC_CMD_STATE_MIDWAY_FRAME,
	FYC_CMD_STATE_LAST_FRAME,
};

enum _CvipAckType_t {
	CVIP_ACK_TYPE_INVALID           = 0,
	CVIP_ACK_TYPE_IN_BUF_RLS        = 1, //input buffer release
	CVIP_ACK_TYPE_PRE_DATA_RLS      = 2, //pre data release
	CVIP_ACK_TYPE_POST_DATA_RLS     = 3, //post data release
	CVIP_ACK_TYPE_RGB_STATUS_NTF    = 4, //rgbStatus buffer notify
	CVIP_ACK_TYPE_IR_STATUS_NTF     = 5, //irStatus buffer notify
	CVIP_ACK_TYPE_DMA_STATUS        = 6, //dma status
};


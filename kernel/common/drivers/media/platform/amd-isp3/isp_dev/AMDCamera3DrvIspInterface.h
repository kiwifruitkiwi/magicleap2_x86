/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _AMD_CAMERA3_DRV_ISP_INTERFACE_H_
#define _AMD_CAMERA3_DRV_ISP_INTERFACE_H_

#define META_VALID_BIT_FREQ		(1 << 0)
#define META_VALID_BIT_EV		(1 << 1)
#define META_VALID_BIT_AF_MODE		(1 << 2)
#define META_VALID_BIT_AWB_MODE		(1 << 3)
#define META_VALID_BIT_AEC_MODE		(1 << 4)
#define META_VALID_BIT_SCENE_MODE	(1 << 6)
#define META_VALID_BIT_FL_MODE		(1 << 7)

enum _ispSensorId_t {
	ISP_SENSOR_ID_INVALID = -1,
	ISP_SENSOR_ID_A = 0,// MIPI0_0
	ISP_SENSOR_ID_B = 1,// MIPI0_1
	ISP_SENSOR_ID_C = 2,// MIPI1
	ISP_SENSOR_ID_MAX
};

//======================================================================
//		Metadata interface data structures
//======================================================================
//this struct defines the metadata returned from FW
//the metadata buf is set by driver and returned by FW in cmd response

enum _ispAfStatus_t {
	ISP_AF_STATUS_UNINITIALIZED = 1,
	ISP_AF_STATUS_LOST = 2,
	ISP_AF_STATUS_SEARCHING = 4,
	ISP_AF_STATUS_FOCUSED = 8,
	ISP_AF_STATUS_FAILED = 16,
	ISP_AF_STATUS_LOCKED = 32,
	ISP_AF_STATUS_STOPPED = 64
};

enum _ispAfMode_t {
	ISP_AF_MODE_INVALID = 0, /**< invalid mode (only for initialization) */
	ISP_AF_MODE_MANUAL = 1, /**< manual mode */
	ISP_AF_MODE_AUTO = 2, /**< single auto mode */
	ISP_AF_MODE_CONTINUOUS_STILL_FOCUS = 3, /**< continuous auto mode */
	ISP_AF_MODE_CONTINUOUS_VIDEO_FOCUS = 4, /**< continuous auto mode */
	ISP_AF_MODE_FIX = 5,/**< fixed focus mode */
	ISP_AF_MODE_MAX
};

enum _ispAwbStatus_t {
	ISP_AWB_STATUS_UNINITIALIZED = 1,
	ISP_AWB_STATUS_SEARCHING = 2,
	ISP_AWB_STATUS_CONVERGED = 4,
	ISP_AWB_STATUS_LOCKED = 8,
	ISP_AWB_STATUS_FAILED = 16,
	ISP_AWB_STATUS_STOPPED = 32
};

enum _ispAecStatus_t {
	ISP_AEC_STATUS_UNINITIALIZED = 1,
	ISP_AEC_STATUS_SEARCHING = 2,
	ISP_AEC_STATUS_CONVERGED = 4,
	ISP_AEC_STATUS_LOCKED = 8,
	ISP_AEC_STATUS_FLASH_REQUIRED = 16,
	ISP_AEC_STATUS_PRECAPTURE = 32,
	ISP_AEC_STATUS_FAILED = 64,
	ISP_AEC_STATUS_STOPPED = 128
};

enum _ispAecMode_t {
	ISP_AEC_MODE_INVALID = 0,/**< invalid mode (only for initialization) */
	ISP_AEC_MODE_MANUAL = 1,/**< manual mode */
	ISP_AEC_MODE_AUTO = 2,/**< run auto mode */
	ISP_AEC_MODE_MAX
};


enum _ispFlickerPeriod_t {
	ISP_FLICKER_OFF,
	ISP_FLICKER_100HZ,
	ISP_FLICKER_120HZ
};

enum _isoValue_t {
	ISO_VALUE_AUTO = 0,
	ISO_VALUE_100 = 1,
	ISO_VALUE_200 = 2,
	ISO_VALUE_400 = 3,
	ISO_VALUE_800 = 4,
	ISO_VALUE_1600 = 5,
	ISO_VALUE_3200 = 6,
	ISO_VALUE_6400 = 7,
	ISO_VALUE_12800 = 8,
	ISO_VALUE_25600 = 9
};


enum _flashStatus_t {
	FLASH_STATUS_UNAVAILABLE,
	FLASH_STATUS_CHARGING,
	FLASH_STATUS_READY,
	FLASH_STATUS_FIRED,
	FLASH_STATUS_PARTIAL
};

enum _flashlightMode_t {
	FLASHLIGHT_MODE_INVALID = 0,
	FLASHLIGHT_MODE_OFF = 1,
	FLASHLIGHT_MODE_ON = 2,
	FLASHLIGHT_MODE_ON_ADJUSTABLE_POWER = 2,
	FLASHLIGHT_MODE_AUTO = 3,
	FLASHLIGHT_MODE_AUTO_ADJUSTABLE_POWER = 4,
	FLASHLIGHT_MODE_REDEYE_REDUCTION = 5,
	FLASHLIGHT_MODE_SINGLE_FLASH = 6,//only valid in photo sequence mode
	FLASHLIGHT_MODE_MAX
};

enum _ispAwbMode_t {
	ISP_AWB_MODE_INVALID = 0,/**< invalid mode (only for initialization) */
	ISP_AWB_MODE_MANUAL = 1,/**< manual mode */
	ISP_AWB_MODE_AUTO = 2,/**< run auto mode */
	ISP_AWB_MODE_MAX
};


struct _ispEvValue_t {
	int32_t valueNumerator;
	int32_t valueDenominator;
};

struct _metaDataAf_t {
	enum _ispAfStatus_t afStatus;// value is from enum af_status
	float sharpness[3]; // AF sharpness value
	uint32_t lensPos; // Lens pos gotten from AF
	uint32_t afValidBits; // Ored META_VALID_BIT_XX
	enum _ispAfMode_t afMode;// afMode, indicated by META_VALID_BIT_AF_MODE
};

struct _metaDataAec_t {
	enum _ispAecStatus_t aecStatus; // value is from enum aec_status
	uint32_t itime; // real integration time multiple 1000000
	uint32_t analogGain; // real analog gain multiple 1000
	uint32_t digitalGain; // real digital gain multiple 1000
	enum _isoValue_t iso; // value is from enum iso_value
	uint32_t hdrRatio; // HDR short exposure para is gotten by long/ratio
	uint32_t histogram[16];
	uint32_t aecValidBits; // Ored META_VALID_BIT_XX
	enum _ispAecMode_t aecMode; // aeExposureCompensation,
				// indicated by META_VALID_BIT_AE_MODE
	enum _ispFlickerPeriod_t freq; // aeAntibandingMode,
				// indicated by META_VALID_BIT_FREQ
	struct _ispEvValue_t ev; // aeExposureCompensation,
			// indicated by META_VALID_BIT_EV
};

struct _metaDataAwb_t {
	enum _ispAwbStatus_t awbStatus;// value is from enum aec_status
	float colorCorrectionGain[4]; // AWB gain
	float colorCorrectionMatrix[9]; // x3 color correction matrix
	uint32_t awbValidBits; // Ored META_VALID_BIT_XX
	enum _ispAwbMode_t awbMode;// indicated by META_VALID_BIT_AWB_MODE
};

struct _metaDataFlash_t {
	enum _flashStatus_t flashStatus; // value is from enum flash_status
	uint32_t flashValidBits; // Ored META_VALID_BIT_XX
	enum _flashlightMode_t flMode; // flash mode,
				 // indicated by META_VALID_BIT_FL_MODE
};

struct _metaDataInfo_t {
	//the sensor that the meta data corresponds to
	enum _ispSensorId_t sensorId;
	uint32_t poc;
	uint32_t timeStamp;
	struct _metaDataAf_t metaAf;
	struct _metaDataAec_t metaAec;
	struct _metaDataAwb_t metaAwb;
	struct _metaDataFlash_t metaFlash;
	uint32_t reserved[968];
};

#endif


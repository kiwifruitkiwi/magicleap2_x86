/*
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/uaccess.h>
#include <linux/amd_cam_metadata.h>

#define VENDOR_SECTION_COUNT \
	(AMD_CAM_VENDOR_SECTION_END - AMD_CAM_VENDOR_CONTROL)

//Tag information
unsigned int amd_cam_metadata_section_bounds[AMD_CAM_SECTION_COUNT][2] = {
	[AMD_CAM_COLOR_CORRECTION] = {AMD_CAM_COLOR_CORRECTION_START,
				      AMD_CAM_COLOR_CORRECTION_END},
	[AMD_CAM_CONTROL] = {AMD_CAM_CONTROL_START,
			     AMD_CAM_CONTROL_END},
	[AMD_CAM_DEMOSAIC] = {AMD_CAM_DEMOSAIC_START,
			      AMD_CAM_DEMOSAIC_END},
	[AMD_CAM_EDGE] = {AMD_CAM_EDGE_START,
			  AMD_CAM_EDGE_END},
	[AMD_CAM_FLASH] = {AMD_CAM_FLASH_START,
			   AMD_CAM_FLASH_END},
	[AMD_CAM_FLASH_INFO] = {AMD_CAM_FLASH_INFO_START,
				AMD_CAM_FLASH_INFO_END},
	[AMD_CAM_HOT_PIXEL] = {AMD_CAM_HOT_PIXEL_START,
			       AMD_CAM_HOT_PIXEL_END},
	[AMD_CAM_JPEG] = {AMD_CAM_JPEG_START,
			  AMD_CAM_JPEG_END},
	[AMD_CAM_LENS] = {AMD_CAM_LENS_START,
			  AMD_CAM_LENS_END},
	[AMD_CAM_LENS_INFO] = {AMD_CAM_LENS_INFO_START,
			       AMD_CAM_LENS_INFO_END},
	[AMD_CAM_NOISE_REDUCTION] = {AMD_CAM_NOISE_REDUCTION_START,
				     AMD_CAM_NOISE_REDUCTION_END},
	[AMD_CAM_QUIRKS] = {AMD_CAM_QUIRKS_START,
			    AMD_CAM_QUIRKS_END},
	[AMD_CAM_REQUEST] = {AMD_CAM_REQUEST_START,
			     AMD_CAM_REQUEST_END},
	[AMD_CAM_SCALER] = {AMD_CAM_SCALER_START,
			    AMD_CAM_SCALER_END},
	[AMD_CAM_SENSOR] = {AMD_CAM_SENSOR_START,
			    AMD_CAM_SENSOR_END},
	[AMD_CAM_SENSOR_INFO] = {AMD_CAM_SENSOR_INFO_START,
				 AMD_CAM_SENSOR_INFO_END},
	[AMD_CAM_SHADING] = {AMD_CAM_SHADING_START,
			     AMD_CAM_SHADING_END},
	[AMD_CAM_STATISTICS] = {AMD_CAM_STATISTICS_START,
				AMD_CAM_STATISTICS_END},
	[AMD_CAM_STATISTICS_INFO] = {AMD_CAM_STATISTICS_INFO_START,
				     AMD_CAM_STATISTICS_INFO_END},
	[AMD_CAM_TONEMAP] = {AMD_CAM_TONEMAP_START,
			     AMD_CAM_TONEMAP_END},
	[AMD_CAM_LED] = {AMD_CAM_LED_START,
			 AMD_CAM_LED_END},
	[AMD_CAM_INFO] = {AMD_CAM_INFO_START,
			  AMD_CAM_INFO_END},
	[AMD_CAM_BLACK_LEVEL] = {AMD_CAM_BLACK_LEVEL_START,
				 AMD_CAM_BLACK_LEVEL_END},
	[AMD_CAM_SYNC] = {AMD_CAM_SYNC_START,
			  AMD_CAM_SYNC_END},
	[AMD_CAM_REPROCESS] = {AMD_CAM_REPROCESS_START,
			       AMD_CAM_REPROCESS_END},
	[AMD_CAM_DEPTH] = {AMD_CAM_DEPTH_START,
			   AMD_CAM_DEPTH_END},
	[AMD_CAM_LOGICAL_MULTI_CAMERA] = {AMD_CAM_LOGICAL_MULTI_CAMERA_START,
					  AMD_CAM_LOGICAL_MULTI_CAMERA_END},
	[AMD_CAM_DISTORTION_CORRECTION]
	    = {AMD_CAM_DISTORTION_CORRECTION_START,
	       AMD_CAM_DISTORTION_CORRECTION_END},
};

unsigned int
	amd_cam_metadata_vendor_section_bounds[VENDOR_SECTION_COUNT][2] = {
	[AMD_CAM_VENDOR_CONTROL - AMD_CAM_VENDOR_SECTION] = {
		AMD_CAM_VENDOR_CONTROL_START, AMD_CAM_VENDOR_CONTROL_END},
};

static struct tag_info amd_cam_color_correction[AMD_CAM_COLOR_CORRECTION_END -
					AMD_CAM_COLOR_CORRECTION_START] = {
	[AMD_CAM_COLOR_CORRECTION_MODE - AMD_CAM_COLOR_CORRECTION_START] = {
	 "mode", TYPE_BYTE},
	[AMD_CAM_COLOR_CORRECTION_TRANSFORM - AMD_CAM_COLOR_CORRECTION_START]
	= {
	 "transform", TYPE_FLOAT},
	[AMD_CAM_COLOR_CORRECTION_GAINS - AMD_CAM_COLOR_CORRECTION_START] = {
	 "gains", TYPE_FLOAT},
	[AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE -
	 AMD_CAM_COLOR_CORRECTION_START] = {"aberrationMode", TYPE_BYTE},
	[AMD_CAM_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES -
	 AMD_CAM_COLOR_CORRECTION_START] = {
	 "availableAberrationModes", TYPE_BYTE},
};

static struct tag_info amd_cam_control[AMD_CAM_CONTROL_END -
				       AMD_CAM_CONTROL_START] = {
	[AMD_CAM_CONTROL_AE_ANTIBANDING_MODE - AMD_CAM_CONTROL_START] = {
	"aeAntibandingMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_EXPOSURE_COMPENSATION - AMD_CAM_CONTROL_START] = {
	"aeExposureCompensation", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_LOCK - AMD_CAM_CONTROL_START] = {
	"aeLock", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_MODE - AMD_CAM_CONTROL_START] = {
	"aeMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_REGIONS - AMD_CAM_CONTROL_START] = {
	"aeRegions", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_TARGET_FPS_RANGE - AMD_CAM_CONTROL_START] = {
	"aeTargetFpsRange", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER - AMD_CAM_CONTROL_START] = {
	"aePrecaptureTrigger",  TYPE_BYTE},
	[AMD_CAM_CONTROL_AF_MODE - AMD_CAM_CONTROL_START] = {
	"afMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_AF_REGIONS - AMD_CAM_CONTROL_START] = {
	"afRegions", TYPE_INT32},
	[AMD_CAM_CONTROL_AF_TRIGGER - AMD_CAM_CONTROL_START] = {
	"afTrigger", TYPE_BYTE},
	[AMD_CAM_CONTROL_AWB_LOCK - AMD_CAM_CONTROL_START] = {
	"awbLock", TYPE_BYTE},
	[AMD_CAM_CONTROL_AWB_MODE - AMD_CAM_CONTROL_START] = {
	"awbMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_AWB_REGIONS - AMD_CAM_CONTROL_START] = {
	"awbRegions", TYPE_INT32},
	[AMD_CAM_CONTROL_CAPTURE_INTENT - AMD_CAM_CONTROL_START] = {
	"captureIntent", TYPE_BYTE},
	[AMD_CAM_CONTROL_EFFECT_MODE - AMD_CAM_CONTROL_START] = {
	"effectMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_MODE - AMD_CAM_CONTROL_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_CONTROL_SCENE_MODE - AMD_CAM_CONTROL_START] = {
	"sceneMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_VIDEO_STABILIZATION_MODE - AMD_CAM_CONTROL_START] = {
	"videoStabilizationMode", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES -
	 AMD_CAM_CONTROL_START] = {"aeAvailableAntibandingModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_AVAILABLE_MODES - AMD_CAM_CONTROL_START] = {
	"aeAvailableModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES -
	 AMD_CAM_CONTROL_START] = {"aeAvailableTargetFpsRanges", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_COMPENSATION_RANGE - AMD_CAM_CONTROL_START] = {
	"aeCompensationRange", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_COMPENSATION_STEP - AMD_CAM_CONTROL_START] = {
	"aeCompensationStep", TYPE_RATIONAL},
	[AMD_CAM_CONTROL_AF_AVAILABLE_MODES - AMD_CAM_CONTROL_START] = {
	"afAvailableModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_AVAILABLE_EFFECTS - AMD_CAM_CONTROL_START] = {
	"availableEffects", TYPE_BYTE},
	[AMD_CAM_CONTROL_AVAILABLE_SCENE_MODES - AMD_CAM_CONTROL_START] = {
	"availableSceneModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES -
	 AMD_CAM_CONTROL_START] = {"availableVideoStabilizationModes",
				   TYPE_BYTE},
	[AMD_CAM_CONTROL_AWB_AVAILABLE_MODES - AMD_CAM_CONTROL_START] = {
	"awbAvailableModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_MAX_REGIONS - AMD_CAM_CONTROL_START] = {
	"maxRegions", TYPE_INT32},
	[AMD_CAM_CONTROL_SCENE_MODE_OVERRIDES - AMD_CAM_CONTROL_START] = {
	"sceneModeOverrides", TYPE_BYTE},
	[AMD_CAM_CONTROL_AE_PRECAPTURE_ID - AMD_CAM_CONTROL_START] = {
	"aePrecaptureId", TYPE_INT32},
	[AMD_CAM_CONTROL_AE_STATE - AMD_CAM_CONTROL_START] = {
	"aeState", TYPE_BYTE},
	[AMD_CAM_CONTROL_AF_STATE - AMD_CAM_CONTROL_START] = {
	"afState", TYPE_BYTE},
	[AMD_CAM_CONTROL_AF_TRIGGER_ID - AMD_CAM_CONTROL_START] = {
	"afTriggerId", TYPE_INT32},
	[AMD_CAM_CONTROL_AWB_STATE - AMD_CAM_CONTROL_START] = {
	"awbState", TYPE_BYTE},
	[AMD_CAM_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS -
	 AMD_CAM_CONTROL_START] = {"availableHighSpeedVideoConfigurations",
				   TYPE_INT32},
	[AMD_CAM_CONTROL_AE_LOCK_AVAILABLE - AMD_CAM_CONTROL_START] = {
	"aeLockAvailable", TYPE_BYTE},
	[AMD_CAM_CONTROL_AWB_LOCK_AVAILABLE - AMD_CAM_CONTROL_START] = {
	"awbLockAvailable", TYPE_BYTE},
	[AMD_CAM_CONTROL_AVAILABLE_MODES - AMD_CAM_CONTROL_START] = {
	"availableModes", TYPE_BYTE},
	[AMD_CAM_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE -
	 AMD_CAM_CONTROL_START] = {"postRawSensitivityBoostRange", TYPE_INT32},
	[AMD_CAM_CONTROL_POST_RAW_SENSITIVITY_BOOST - AMD_CAM_CONTROL_START] = {
	"postRawSensitivityBoost", TYPE_INT32},
	[AMD_CAM_CONTROL_ENABLE_ZSL - AMD_CAM_CONTROL_START] = {
	"enableZsl", TYPE_BYTE},
	[AMD_CAM_CONTROL_AF_SCENE_CHANGE - AMD_CAM_CONTROL_START] = {
	"afSceneChange",  TYPE_BYTE},
};

static struct tag_info amd_cam_demosaic[AMD_CAM_DEMOSAIC_END -
					AMD_CAM_DEMOSAIC_START] = {
	[AMD_CAM_DEMOSAIC_MODE - AMD_CAM_DEMOSAIC_START] = {"mode", TYPE_BYTE},
};

static struct tag_info amd_cam_edge[AMD_CAM_EDGE_END - AMD_CAM_EDGE_START] = {
	[AMD_CAM_EDGE_MODE - AMD_CAM_EDGE_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_EDGE_STRENGTH - AMD_CAM_EDGE_START] = {"strength", TYPE_BYTE},
	[AMD_CAM_EDGE_AVAILABLE_EDGE_MODES - AMD_CAM_EDGE_START] = {
	"availableEdgeModes", TYPE_BYTE},
};

static struct tag_info
		amd_cam_flash[AMD_CAM_FLASH_END - AMD_CAM_FLASH_START] = {
	[AMD_CAM_FLASH_FIRING_POWER - AMD_CAM_FLASH_START] = {
	"firingPower", TYPE_BYTE},
	[AMD_CAM_FLASH_FIRING_TIME - AMD_CAM_FLASH_START] = {
	"firingTime", TYPE_INT64},
	[AMD_CAM_FLASH_MODE - AMD_CAM_FLASH_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_FLASH_COLOR_TEMPERATURE - AMD_CAM_FLASH_START] = {
	"colorTemperature", TYPE_BYTE},
	[AMD_CAM_FLASH_MAX_ENERGY - AMD_CAM_FLASH_START] = {
	"maxEnergy", TYPE_BYTE},
	[AMD_CAM_FLASH_STATE - AMD_CAM_FLASH_START] = {"state", TYPE_BYTE},
};

static struct tag_info amd_cam_flash_info[AMD_CAM_FLASH_INFO_END -
					  AMD_CAM_FLASH_INFO_START] = {
	[AMD_CAM_FLASH_INFO_AVAILABLE - AMD_CAM_FLASH_INFO_START] = {
	"available", TYPE_BYTE},
	[AMD_CAM_FLASH_INFO_CHARGE_DURATION - AMD_CAM_FLASH_INFO_START] = {
	"chargeDuration", TYPE_INT64},
};

static struct tag_info amd_cam_hot_pixel[AMD_CAM_HOT_PIXEL_END -
					 AMD_CAM_HOT_PIXEL_START] = {
	[AMD_CAM_HOT_PIXEL_MODE - AMD_CAM_HOT_PIXEL_START] = {
	"mode", TYPE_BYTE},
	[AMD_CAM_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES -
	 AMD_CAM_HOT_PIXEL_START] = {"availableHotPixelModes", TYPE_BYTE},
};

static struct tag_info amd_cam_jpeg[AMD_CAM_JPEG_END - AMD_CAM_JPEG_START] = {
	[AMD_CAM_JPEG_GPS_COORDINATES - AMD_CAM_JPEG_START] = {
	"gpsCoordinates", TYPE_DOUBLE},
	[AMD_CAM_JPEG_GPS_PROCESSING_METHOD - AMD_CAM_JPEG_START] = {
	"gpsProcessingMethod", TYPE_BYTE},
	[AMD_CAM_JPEG_GPS_TIMESTAMP - AMD_CAM_JPEG_START] = {
	"gpsTimestamp", TYPE_INT64},
	[AMD_CAM_JPEG_ORIENTATION - AMD_CAM_JPEG_START] = {
	"orientation", TYPE_INT32},
	[AMD_CAM_JPEG_QUALITY - AMD_CAM_JPEG_START] = {"quality", TYPE_BYTE},
	[AMD_CAM_JPEG_THUMBNAIL_QUALITY - AMD_CAM_JPEG_START] = {
	"thumbnailQuality", TYPE_BYTE},
	[AMD_CAM_JPEG_THUMBNAIL_SIZE - AMD_CAM_JPEG_START] = {
	 "thumbnailSize", TYPE_INT32},
	[AMD_CAM_JPEG_AVAILABLE_THUMBNAIL_SIZES - AMD_CAM_JPEG_START] = {
	"availableThumbnailSizes", TYPE_INT32},
	[AMD_CAM_JPEG_MAX_SIZE - AMD_CAM_JPEG_START] = {"maxSize", TYPE_INT32},
	[AMD_CAM_JPEG_SIZE - AMD_CAM_JPEG_START] = {"size", TYPE_INT32},
};

static struct tag_info amd_cam_lens[AMD_CAM_LENS_END - AMD_CAM_LENS_START] = {
	[AMD_CAM_LENS_APERTURE - AMD_CAM_LENS_START] = {"aperture", TYPE_FLOAT},
	[AMD_CAM_LENS_FILTER_DENSITY - AMD_CAM_LENS_START] = {
	"filterDensity", TYPE_FLOAT},
	[AMD_CAM_LENS_FOCAL_LENGTH - AMD_CAM_LENS_START] = {
	"focalLength", TYPE_FLOAT},
	[AMD_CAM_LENS_FOCUS_DISTANCE - AMD_CAM_LENS_START] = {
	"focusDistance", TYPE_FLOAT},
	[AMD_CAM_LENS_OPTICAL_STABILIZATION_MODE - AMD_CAM_LENS_START] = {
	"opticalStabilizationMode", TYPE_BYTE},
	[AMD_CAM_LENS_FACING - AMD_CAM_LENS_START] = {"facing", TYPE_BYTE},
	[AMD_CAM_LENS_POSE_ROTATION - AMD_CAM_LENS_START] = {
	"poseRotation", TYPE_FLOAT},
	[AMD_CAM_LENS_POSE_TRANSLATION - AMD_CAM_LENS_START] = {
	"poseTranslation", TYPE_FLOAT},
	[AMD_CAM_LENS_FOCUS_RANGE - AMD_CAM_LENS_START] = {
	"focusRange", TYPE_FLOAT},
	[AMD_CAM_LENS_STATE - AMD_CAM_LENS_START] = {"state", TYPE_BYTE},
	[AMD_CAM_LENS_INTRINSIC_CALIBRATION - AMD_CAM_LENS_START] = {
	"intrinsicCalibration", TYPE_FLOAT},
	[AMD_CAM_LENS_RADIAL_DISTORTION - AMD_CAM_LENS_START] = {
	"radialDistortion", TYPE_FLOAT},
	[AMD_CAM_LENS_POSE_REFERENCE - AMD_CAM_LENS_START] = {
	"poseReference", TYPE_BYTE},
	[AMD_CAM_LENS_DISTORTION - AMD_CAM_LENS_START] = {
	"distortion", TYPE_FLOAT},
};

static struct tag_info amd_cam_lens_info[AMD_CAM_LENS_INFO_END -
					 AMD_CAM_LENS_INFO_START] = {
	[AMD_CAM_LENS_INFO_AVAILABLE_APERTURES - AMD_CAM_LENS_INFO_START] = {
	"availableApertures", TYPE_FLOAT},
	[AMD_CAM_LENS_INFO_AVAILABLE_FILTER_DENSITIES -
	 AMD_CAM_LENS_INFO_START] = {"availableFilterDensities", TYPE_FLOAT},
	[AMD_CAM_LENS_INFO_AVAILABLE_FOCAL_LENGTHS - AMD_CAM_LENS_INFO_START]
	= {
	"availableFocalLengths", TYPE_FLOAT},
	[AMD_CAM_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION -
	 AMD_CAM_LENS_INFO_START] = {
	"availableOpticalStabilization", TYPE_BYTE},
	[AMD_CAM_LENS_INFO_HYPERFOCAL_DISTANCE - AMD_CAM_LENS_INFO_START] = {
	"hyperfocalDistance", TYPE_FLOAT},
	[AMD_CAM_LENS_INFO_MINIMUM_FOCUS_DISTANCE - AMD_CAM_LENS_INFO_START] = {
	"minimumFocusDistance", TYPE_FLOAT},
	[AMD_CAM_LENS_INFO_SHADING_MAP_SIZE - AMD_CAM_LENS_INFO_START] = {
	"shadingMapSize", TYPE_INT32},
	[AMD_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION -
	 AMD_CAM_LENS_INFO_START] = {"focusDistanceCalibration", TYPE_BYTE},
};

static struct tag_info amd_cam_noise_reduction[AMD_CAM_NOISE_REDUCTION_END -
	AMD_CAM_NOISE_REDUCTION_START] = {
	[AMD_CAM_NOISE_REDUCTION_MODE - AMD_CAM_NOISE_REDUCTION_START] = {
	"mode", TYPE_BYTE},
	[AMD_CAM_NOISE_REDUCTION_STRENGTH - AMD_CAM_NOISE_REDUCTION_START] = {
	"strength", TYPE_BYTE},
	[AMD_CAM_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES -
	 AMD_CAM_NOISE_REDUCTION_START] = {
	"availableNoiseReductionModes", TYPE_BYTE},
};

static struct tag_info amd_cam_quirks[AMD_CAM_QUIRKS_END -
				      AMD_CAM_QUIRKS_START] = {
	[AMD_CAM_QUIRKS_METERING_CROP_REGION - AMD_CAM_QUIRKS_START] = {
	"meteringCropRegion", TYPE_BYTE},
	[AMD_CAM_QUIRKS_TRIGGER_AF_WITH_AUTO - AMD_CAM_QUIRKS_START] = {
	"triggerAfWithAuto", TYPE_BYTE},
	[AMD_CAM_QUIRKS_USE_ZSL_FORMAT - AMD_CAM_QUIRKS_START] = {
	"useZslFormat", TYPE_BYTE},
	[AMD_CAM_QUIRKS_USE_PARTIAL_RESULT - AMD_CAM_QUIRKS_START] = {
	"usePartialResult", TYPE_BYTE},
	[AMD_CAM_QUIRKS_PARTIAL_RESULT - AMD_CAM_QUIRKS_START] = {
	"partialResult", TYPE_BYTE},
};

static struct tag_info amd_cam_request[AMD_CAM_REQUEST_END -
				       AMD_CAM_REQUEST_START] = {
	[AMD_CAM_REQUEST_FRAME_COUNT - AMD_CAM_REQUEST_START] = {
	"frameCount", TYPE_INT32},
	[AMD_CAM_REQUEST_ID - AMD_CAM_REQUEST_START] = {"id", TYPE_INT32},
	[AMD_CAM_REQUEST_INPUT_STREAMS - AMD_CAM_REQUEST_START] = {
	"inputStreams", TYPE_INT32},
	[AMD_CAM_REQUEST_METADATA_MODE - AMD_CAM_REQUEST_START] = {
	"metadataMode", TYPE_BYTE},
	[AMD_CAM_REQUEST_OUTPUT_STREAMS - AMD_CAM_REQUEST_START] = {
	"outputStreams", TYPE_INT32},
	[AMD_CAM_REQUEST_TYPE - AMD_CAM_REQUEST_START] = {"type", TYPE_BYTE},
	[AMD_CAM_REQUEST_MAX_NUM_OUTPUT_STREAMS - AMD_CAM_REQUEST_START] = {
	"maxNumOutputStreams", TYPE_INT32},
	[AMD_CAM_REQUEST_MAX_NUM_REPROCESS_STREAMS - AMD_CAM_REQUEST_START] = {
	"maxNumReprocessStreams", TYPE_INT32},
	[AMD_CAM_REQUEST_MAX_NUM_INPUT_STREAMS - AMD_CAM_REQUEST_START] = {
	"maxNumInputStreams", TYPE_INT32},
	[AMD_CAM_REQUEST_PIPELINE_DEPTH - AMD_CAM_REQUEST_START] = {
	"pipelineDepth", TYPE_BYTE},
	[AMD_CAM_REQUEST_PIPELINE_MAX_DEPTH - AMD_CAM_REQUEST_START] = {
	"pipelineMaxDepth", TYPE_BYTE},
	[AMD_CAM_REQUEST_PARTIAL_RESULT_COUNT - AMD_CAM_REQUEST_START] = {
	"partialResultCount", TYPE_INT32},
	[AMD_CAM_REQUEST_AVAILABLE_CAPABILITIES - AMD_CAM_REQUEST_START] = {
	"availableCapabilities", TYPE_BYTE},
	[AMD_CAM_REQUEST_AVAILABLE_REQUEST_KEYS - AMD_CAM_REQUEST_START] = {
	"availableRequestKeys", TYPE_INT32},
	[AMD_CAM_REQUEST_AVAILABLE_RESULT_KEYS - AMD_CAM_REQUEST_START] = {
	"availableResultKeys", TYPE_INT32},
	[AMD_CAM_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS -
	 AMD_CAM_REQUEST_START] = {"availableCharacteristicsKeys", TYPE_INT32},
	[AMD_CAM_REQUEST_AVAILABLE_SESSION_KEYS - AMD_CAM_REQUEST_START] = {
	"availableSessionKeys", TYPE_INT32},
	[AMD_CAM_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS -
	 AMD_CAM_REQUEST_START] = {"availablePhysicalCameraRequestKeys",
				   TYPE_INT32},
};

static struct tag_info amd_cam_scaler[AMD_CAM_SCALER_END -
				      AMD_CAM_SCALER_START] = {
	[AMD_CAM_SCALER_CROP_REGION - AMD_CAM_SCALER_START] = {
	"cropRegion", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_FORMATS - AMD_CAM_SCALER_START] = {
	"availableFormats", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_JPEG_MIN_DURATIONS - AMD_CAM_SCALER_START] = {
	"availableJpegMinDurations", TYPE_INT64},
	[AMD_CAM_SCALER_AVAILABLE_JPEG_SIZES - AMD_CAM_SCALER_START] = {
	"availableJpegSizes", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM - AMD_CAM_SCALER_START] = {
	"availableMaxDigitalZoom", TYPE_FLOAT},
	[AMD_CAM_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS -
	 AMD_CAM_SCALER_START] = {"availableProcessedMinDurations",
				  TYPE_INT64},
	[AMD_CAM_SCALER_AVAILABLE_PROCESSED_SIZES - AMD_CAM_SCALER_START] = {
	"availableProcessedSizes", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_RAW_MIN_DURATIONS - AMD_CAM_SCALER_START] = {
	"availableRawMinDurations", TYPE_INT64},
	[AMD_CAM_SCALER_AVAILABLE_RAW_SIZES - AMD_CAM_SCALER_START] = {
	"availableRawSizes", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP -
	 AMD_CAM_SCALER_START] = {"availableInputOutputFormatsMap",
				  TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_STREAM_CONFIGURATIONS -
	 AMD_CAM_SCALER_START] = {"availableStreamConfigurations", TYPE_INT32},
	[AMD_CAM_SCALER_AVAILABLE_MIN_FRAME_DURATIONS - AMD_CAM_SCALER_START]
	= {
	"availableMinFrameDurations", TYPE_INT64},
	[AMD_CAM_SCALER_AVAILABLE_STALL_DURATIONS - AMD_CAM_SCALER_START] = {
	"availableStallDurations", TYPE_INT64},
	[AMD_CAM_SCALER_CROPPING_TYPE - AMD_CAM_SCALER_START] = {
	"croppingType", TYPE_BYTE},
};

static struct tag_info amd_cam_sensor[AMD_CAM_SENSOR_END -
				      AMD_CAM_SENSOR_START] = {
	[AMD_CAM_SENSOR_EXPOSURE_TIME - AMD_CAM_SENSOR_START] = {
	"exposureTime", TYPE_INT64},
	[AMD_CAM_SENSOR_FRAME_DURATION - AMD_CAM_SENSOR_START] = {
	"frameDuration", TYPE_INT64},
	[AMD_CAM_SENSOR_SENSITIVITY - AMD_CAM_SENSOR_START] = {
	"sensitivity", TYPE_INT32},
	[AMD_CAM_SENSOR_REFERENCE_ILLUMINANT1 - AMD_CAM_SENSOR_START] = {
	"referenceIlluminant1", TYPE_BYTE},
	[AMD_CAM_SENSOR_REFERENCE_ILLUMINANT2 - AMD_CAM_SENSOR_START] = {
	"referenceIlluminant2", TYPE_BYTE},
	[AMD_CAM_SENSOR_CALIBRATION_TRANSFORM1 - AMD_CAM_SENSOR_START] = {
	"calibrationTransform1", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_CALIBRATION_TRANSFORM2 - AMD_CAM_SENSOR_START] = {
	"calibrationTransform2", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_COLOR_TRANSFORM1 - AMD_CAM_SENSOR_START] = {
	"colorTransform1", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_COLOR_TRANSFORM2 - AMD_CAM_SENSOR_START] = {
	"colorTransform2", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_FORWARD_MATRIX1 - AMD_CAM_SENSOR_START] = {
	"forwardMatrix1", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_FORWARD_MATRIX2 - AMD_CAM_SENSOR_START] = {
	"forwardMatrix2", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_BASE_GAIN_FACTOR - AMD_CAM_SENSOR_START] = {
	"baseGainFactor", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_BLACK_LEVEL_PATTERN - AMD_CAM_SENSOR_START] = {
	"blackLevelPattern", TYPE_INT32},
	[AMD_CAM_SENSOR_MAX_ANALOG_SENSITIVITY - AMD_CAM_SENSOR_START] = {
	"maxAnalogSensitivity", TYPE_INT32},
	[AMD_CAM_SENSOR_ORIENTATION - AMD_CAM_SENSOR_START] = {
	"orientation", TYPE_INT32},
	[AMD_CAM_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS - AMD_CAM_SENSOR_START]
	= {
	"profileHueSatMapDimensions", TYPE_INT32},
	[AMD_CAM_SENSOR_TIMESTAMP - AMD_CAM_SENSOR_START] = {
	"timestamp", TYPE_INT64},
	[AMD_CAM_SENSOR_TEMPERATURE - AMD_CAM_SENSOR_START] = {
	"temperature", TYPE_FLOAT},
	[AMD_CAM_SENSOR_NEUTRAL_COLOR_POINT - AMD_CAM_SENSOR_START] = {
	"neutralColorPoint", TYPE_RATIONAL},
	[AMD_CAM_SENSOR_NOISE_PROFILE - AMD_CAM_SENSOR_START] = {
	"noiseProfile", TYPE_DOUBLE},
	[AMD_CAM_SENSOR_PROFILE_HUE_SAT_MAP - AMD_CAM_SENSOR_START] = {
	"profileHueSatMap", TYPE_FLOAT},
	[AMD_CAM_SENSOR_PROFILE_TONE_CURVE - AMD_CAM_SENSOR_START] = {
	"profileToneCurve", TYPE_FLOAT},
	[AMD_CAM_SENSOR_GREEN_SPLIT - AMD_CAM_SENSOR_START] = {
	"greenSplit", TYPE_FLOAT},
	[AMD_CAM_SENSOR_TEST_PATTERN_DATA - AMD_CAM_SENSOR_START] = {
	"testPatternData", TYPE_INT32},
	[AMD_CAM_SENSOR_TEST_PATTERN_MODE - AMD_CAM_SENSOR_START] = {
	"testPatternMode", TYPE_INT32},
	[AMD_CAM_SENSOR_AVAILABLE_TEST_PATTERN_MODES - AMD_CAM_SENSOR_START] = {
	"availableTestPatternModes", TYPE_INT32},
	[AMD_CAM_SENSOR_ROLLING_SHUTTER_SKEW - AMD_CAM_SENSOR_START] = {
	"rollingShutterSkew", TYPE_INT64},
	[AMD_CAM_SENSOR_OPTICAL_BLACK_REGIONS - AMD_CAM_SENSOR_START] = {
	"opticalBlackRegions", TYPE_INT32},
	[AMD_CAM_SENSOR_DYNAMIC_BLACK_LEVEL - AMD_CAM_SENSOR_START] = {
	"dynamicBlackLevel", TYPE_INT32},
	[AMD_CAM_SENSOR_DYNAMIC_WHITE_LEVEL - AMD_CAM_SENSOR_START] = {
	"dynamicWhiteLevel", TYPE_INT32},
	[AMD_CAM_SENSOR_OPAQUE_RAW_SIZE - AMD_CAM_SENSOR_START] = {
	"opaqueRawSize", TYPE_INT32},
};

static struct tag_info amd_cam_sensor_info[AMD_CAM_SENSOR_INFO_END -
					   AMD_CAM_SENSOR_INFO_START] = {
	[AMD_CAM_SENSOR_INFO_ACTIVE_ARRAY_SIZE - AMD_CAM_SENSOR_INFO_START] = {
	"activeArraySize", TYPE_INT32},
	[AMD_CAM_SENSOR_INFO_SENSITIVITY_RANGE - AMD_CAM_SENSOR_INFO_START] = {
	"sensitivityRange", TYPE_INT32},
	[AMD_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT -
	 AMD_CAM_SENSOR_INFO_START] = {"colorFilterArrangement", TYPE_BYTE},
	[AMD_CAM_SENSOR_INFO_EXPOSURE_TIME_RANGE -
	AMD_CAM_SENSOR_INFO_START] = {
	"exposureTimeRange", TYPE_INT64},
	[AMD_CAM_SENSOR_INFO_MAX_FRAME_DURATION - AMD_CAM_SENSOR_INFO_START] = {
	"maxFrameDuration", TYPE_INT64},
	[AMD_CAM_SENSOR_INFO_PHYSICAL_SIZE - AMD_CAM_SENSOR_INFO_START] = {
	"physicalSize", TYPE_FLOAT},
	[AMD_CAM_SENSOR_INFO_PIXEL_ARRAY_SIZE - AMD_CAM_SENSOR_INFO_START] = {
	"pixelArraySize", TYPE_INT32},
	[AMD_CAM_SENSOR_INFO_WHITE_LEVEL - AMD_CAM_SENSOR_INFO_START] = {
	"whiteLevel", TYPE_INT32},
	[AMD_CAM_SENSOR_INFO_TIMESTAMP_SOURCE - AMD_CAM_SENSOR_INFO_START] = {
	"timestampSource", TYPE_BYTE},
	[AMD_CAM_SENSOR_INFO_LENS_SHADING_APPLIED -
	AMD_CAM_SENSOR_INFO_START] = {
	"lensShadingApplied", TYPE_BYTE},
	[AMD_CAM_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE -
	 AMD_CAM_SENSOR_INFO_START] = {
	"preCorrectionActiveArraySize", TYPE_INT32},
};

static struct tag_info amd_cam_shading[AMD_CAM_SHADING_END -
				       AMD_CAM_SHADING_START] = {
	[AMD_CAM_SHADING_MODE - AMD_CAM_SHADING_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_SHADING_STRENGTH - AMD_CAM_SHADING_START] = {
	"strength", TYPE_BYTE},
	[AMD_CAM_SHADING_AVAILABLE_MODES - AMD_CAM_SHADING_START] = {
	"availableModes", TYPE_BYTE},
};

static struct tag_info amd_cam_statistics[AMD_CAM_STATISTICS_END -
					  AMD_CAM_STATISTICS_START] = {
	[AMD_CAM_STATISTICS_FACE_DETECT_MODE - AMD_CAM_STATISTICS_START] = {
	"faceDetectMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_HISTOGRAM_MODE - AMD_CAM_STATISTICS_START] = {
	"histogramMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_SHARPNESS_MAP_MODE - AMD_CAM_STATISTICS_START] = {
	"sharpnessMapMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_HOT_PIXEL_MAP_MODE - AMD_CAM_STATISTICS_START] = {
	"hotPixelMapMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_FACE_IDS - AMD_CAM_STATISTICS_START] = {
	"faceIds", TYPE_INT32},
	[AMD_CAM_STATISTICS_FACE_LANDMARKS - AMD_CAM_STATISTICS_START] = {
	"faceLandmarks", TYPE_INT32},
	[AMD_CAM_STATISTICS_FACE_RECTANGLES - AMD_CAM_STATISTICS_START] = {
	"faceRectangles", TYPE_INT32},
	[AMD_CAM_STATISTICS_FACE_SCORES - AMD_CAM_STATISTICS_START] = {
	"faceScores", TYPE_BYTE},
	[AMD_CAM_STATISTICS_HISTOGRAM - AMD_CAM_STATISTICS_START] = {
	"histogram", TYPE_INT32},
	[AMD_CAM_STATISTICS_SHARPNESS_MAP - AMD_CAM_STATISTICS_START] = {
	"sharpnessMap", TYPE_INT32},
	[AMD_CAM_STATISTICS_LENS_SHADING_CORRECTION_MAP -
	 AMD_CAM_STATISTICS_START] = {"lensShadingCorrectionMap", TYPE_BYTE},
	[AMD_CAM_STATISTICS_LENS_SHADING_MAP - AMD_CAM_STATISTICS_START] = {
	"lensShadingMap", TYPE_INT32},
	[AMD_CAM_STATISTICS_PREDICTED_COLOR_GAINS -
	AMD_CAM_STATISTICS_START] = {
	"predictedColorGains", TYPE_FLOAT},
	[AMD_CAM_STATISTICS_PREDICTED_COLOR_TRANSFORM -
	 AMD_CAM_STATISTICS_START] = {"predictedColorTransform", TYPE_RATIONAL},
	[AMD_CAM_STATISTICS_SCENE_FLICKER - AMD_CAM_STATISTICS_START] = {
	"sceneFlicker", TYPE_BYTE},
	[AMD_CAM_STATISTICS_HOT_PIXEL_MAP - AMD_CAM_STATISTICS_START] = {
	"hotPixelMap", TYPE_INT32},
	[AMD_CAM_STATISTICS_LENS_SHADING_MAP_MODE -
	AMD_CAM_STATISTICS_START] = {
	"lensShadingMapMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_OIS_DATA_MODE - AMD_CAM_STATISTICS_START] = {
	"oisDataMode", TYPE_BYTE},
	[AMD_CAM_STATISTICS_OIS_TIMESTAMPS - AMD_CAM_STATISTICS_START] = {
	"oisTimestamps", TYPE_INT64},
	[AMD_CAM_STATISTICS_OIS_X_SHIFTS - AMD_CAM_STATISTICS_START] = {
	"oisXShifts", TYPE_FLOAT},
	[AMD_CAM_STATISTICS_OIS_Y_SHIFTS - AMD_CAM_STATISTICS_START] = {
	"oisYShifts", TYPE_FLOAT},
};

static struct tag_info amd_cam_statistics_info[AMD_CAM_STATISTICS_INFO_END -
	AMD_CAM_STATISTICS_INFO_START] = {
	[AMD_CAM_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES -
	 AMD_CAM_STATISTICS_INFO_START] = {
	"availableFaceDetectModes", TYPE_BYTE},
	[AMD_CAM_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT -
	 AMD_CAM_STATISTICS_INFO_START] = {"histogramBucketCount", TYPE_INT32},
	[AMD_CAM_STATISTICS_INFO_MAX_FACE_COUNT -
	 AMD_CAM_STATISTICS_INFO_START] = {"maxFaceCount", TYPE_INT32},
	[AMD_CAM_STATISTICS_INFO_MAX_HISTOGRAM_COUNT -
	 AMD_CAM_STATISTICS_INFO_START] = {"maxHistogramCount", TYPE_INT32},
	[AMD_CAM_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE -
	 AMD_CAM_STATISTICS_INFO_START] = {"maxSharpnessMapValue", TYPE_INT32},
	[AMD_CAM_STATISTICS_INFO_SHARPNESS_MAP_SIZE -
	 AMD_CAM_STATISTICS_INFO_START] = {"sharpnessMapSize", TYPE_INT32},
	[AMD_CAM_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES -
	 AMD_CAM_STATISTICS_INFO_START] = {
	"availableHotPixelMapModes", TYPE_BYTE},
	[AMD_CAM_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES -
	 AMD_CAM_STATISTICS_INFO_START] = {
	 "availableLensShadingMapModes", TYPE_BYTE},
	[AMD_CAM_STATISTICS_INFO_AVAILABLE_OIS_DATA_MODES -
	 AMD_CAM_STATISTICS_INFO_START] = {"availableOisDataModes", TYPE_BYTE},
};

static struct tag_info amd_cam_tonemap[AMD_CAM_TONEMAP_END -
				       AMD_CAM_TONEMAP_START] = {
	[AMD_CAM_TONEMAP_CURVE_BLUE - AMD_CAM_TONEMAP_START] = {
	"curveBlue", TYPE_FLOAT},
	[AMD_CAM_TONEMAP_CURVE_GREEN - AMD_CAM_TONEMAP_START] = {
	"curveGreen", TYPE_INT32},
	[AMD_CAM_TONEMAP_CURVE_RED - AMD_CAM_TONEMAP_START] = {
	"curveRed", TYPE_FLOAT},
	[AMD_CAM_TONEMAP_MODE - AMD_CAM_TONEMAP_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_TONEMAP_MAX_CURVE_POINTS - AMD_CAM_TONEMAP_START] = {
	"maxCurvePoints", TYPE_INT32},
	[AMD_CAM_TONEMAP_AVAILABLE_TONE_MAP_MODES - AMD_CAM_TONEMAP_START] = {
	"availableToneMapModes", TYPE_BYTE},
	[AMD_CAM_TONEMAP_GAMMA - AMD_CAM_TONEMAP_START] = {"gamma", TYPE_FLOAT},
	[AMD_CAM_TONEMAP_PRESET_CURVE - AMD_CAM_TONEMAP_START] = {
	"presetCurve", TYPE_BYTE},
};

static struct tag_info amd_cam_led[AMD_CAM_LED_END - AMD_CAM_LED_START] = {
	[AMD_CAM_LED_TRANSMIT - AMD_CAM_LED_START] = {"transmit", TYPE_BYTE},
	[AMD_CAM_LED_AVAILABLE_LEDS - AMD_CAM_LED_START] = {
	"availableLeds", TYPE_BYTE},
};

static struct tag_info amd_cam_info[AMD_CAM_INFO_END - AMD_CAM_INFO_START] = {
	[AMD_CAM_INFO_SUPPORTED_HARDWARE_LEVEL - AMD_CAM_INFO_START] = {
	"supportedHardwareLevel", TYPE_BYTE},
	[AMD_CAM_INFO_VERSION - AMD_CAM_INFO_START] = {"version", TYPE_BYTE},
};

static struct tag_info amd_cam_black_level[AMD_CAM_BLACK_LEVEL_END -
					   AMD_CAM_BLACK_LEVEL_START] = {
	[AMD_CAM_BLACK_LEVEL_LOCK - AMD_CAM_BLACK_LEVEL_START] = {
	"lock", TYPE_BYTE},
};

static struct tag_info amd_cam_sync[AMD_CAM_SYNC_END - AMD_CAM_SYNC_START] = {
	[AMD_CAM_SYNC_FRAME_NUMBER - AMD_CAM_SYNC_START] = {
	"frameNumber", TYPE_INT64},
	[AMD_CAM_SYNC_MAX_LATENCY - AMD_CAM_SYNC_START] = {
	"maxLatency", TYPE_INT32},
};

static struct tag_info amd_cam_reprocess[AMD_CAM_REPROCESS_END -
					 AMD_CAM_REPROCESS_START] = {
	[AMD_CAM_REPROCESS_EFFECTIVE_EXPOSURE_FACTOR -
	 AMD_CAM_REPROCESS_START] = {"effectiveExposureFactor", TYPE_FLOAT},
	[AMD_CAM_REPROCESS_MAX_CAPTURE_STALL - AMD_CAM_REPROCESS_START] = {
	"maxCaptureStall", TYPE_INT32},
};

static struct tag_info amd_cam_depth[AMD_CAM_DEPTH_END - AMD_CAM_DEPTH_START]
	= {
	[AMD_CAM_DEPTH_MAX_DEPTH_SAMPLES - AMD_CAM_DEPTH_START] = {
	 "maxDepthSamples", TYPE_INT32},
	[AMD_CAM_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS -
	 AMD_CAM_DEPTH_START] = {"availableDepthStreamConfigurations",
				 TYPE_INT32},
	[AMD_CAM_DEPTH_AVAILABLE_DEPTH_MIN_FRAME_DURATIONS -
	 AMD_CAM_DEPTH_START] = {"availableDepthMinFrameDurations",
				 TYPE_INT64},
	[AMD_CAM_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS - AMD_CAM_DEPTH_START]
	= {
	 "availableDepthStallDurations", TYPE_INT64},
	[AMD_CAM_DEPTH_DEPTH_IS_EXCLUSIVE - AMD_CAM_DEPTH_START] = {
	 "depthIsExclusive",  TYPE_BYTE},
};

static struct tag_info
amd_cam_logical_multi_camera[AMD_CAM_LOGICAL_MULTI_CAMERA_END -
			      AMD_CAM_LOGICAL_MULTI_CAMERA_START] = {
	[AMD_CAM_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS -
	 AMD_CAM_LOGICAL_MULTI_CAMERA_START] = {"physicalIds", TYPE_BYTE},
	[AMD_CAM_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE -
	 AMD_CAM_LOGICAL_MULTI_CAMERA_START] = {"sensorSyncType", TYPE_BYTE},
};

static struct tag_info
amd_cam_distortion_correction[AMD_CAM_DISTORTION_CORRECTION_END -
			       AMD_CAM_DISTORTION_CORRECTION_START] = {
	[AMD_CAM_DISTORTION_CORRECTION_MODE -
	 AMD_CAM_DISTORTION_CORRECTION_START] = {"mode", TYPE_BYTE},
	[AMD_CAM_DISTORTION_CORRECTION_AVAILABLE_MODES -
	 AMD_CAM_DISTORTION_CORRECTION_START] = {"availableModes", TYPE_BYTE},
};

static struct tag_info
	amd_cam_vendor_control[AMD_CAM_VENDOR_CONTROL_END -
			       AMD_CAM_VENDOR_CONTROL_START] = {
	[AMD_CAM_VENDOR_CONTROL_FORCEAPPLY_MODE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"forceapply_mode", TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_AVAILABLE_FORCEAPPLY_MODES -
	 AMD_CAM_VENDOR_CONTROL_START] = {"forceapply_supported_modes",
					  TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_EFFECT_MODE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"effectmode", TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_AVAILABLE_EFFECT_MODES -
	 AMD_CAM_VENDOR_CONTROL_START] = {"effect_available_modes", TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_APP_EXPOSURE_TIME_UPPER_LIMIT -
	 AMD_CAM_VENDOR_CONTROL_START] = {"app_exposure_time_upper_limit",
					  TYPE_INT64},
	[AMD_CAM_VENDOR_CONTROL_LENS_CONTROL_MODE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"lens_control_mode", TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_AVAILABLE_LENS_CONTROL_MODES -
	 AMD_CAM_VENDOR_CONTROL_START] = {"lens_control_available_modes",
					  TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_ENGINEERING_MODE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"engineering_mode", TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_AVAILABLE_ENGINEERING_MODES -
	 AMD_CAM_VENDOR_CONTROL_START] = {"engineering_available_modes",
					  TYPE_BYTE},
	[AMD_CAM_VENDOR_CONTROL_LENS_SWEEP_DISTANCERANGE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"lens_sweep_distance_range",
					  TYPE_FLOAT},
	[AMD_CAM_VENDOR_CONTROL_LENS_INFO_MIN_INCREMENT -
	 AMD_CAM_VENDOR_CONTROL_START] = {"lens_info_min_increment",
					  TYPE_FLOAT},
	[AMD_CAM_VENDOR_CONTROL_CVIP_TIMESTAMPS -
	 AMD_CAM_VENDOR_CONTROL_START] = {"cvip_timestamps", TYPE_INT64},
	[AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET_RANGE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"ae_roi_touch_target_range",
					  TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET -
	 AMD_CAM_VENDOR_CONTROL_START] = {"ae_roi_touch_target", TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET_WEIGHT_RANGE -
	 AMD_CAM_VENDOR_CONTROL_START] = {"ae_roi_touch_target_weight_range",
					  TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET_WEIGHT -
	 AMD_CAM_VENDOR_CONTROL_START] = {"ae_roi_touch_target_weight",
					  TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_RAW_FRAME_ID -
	 AMD_CAM_VENDOR_CONTROL_START] = {"raw_frame_id", TYPE_INT32},
	[AMD_CAM_VENDOR_CONTROL_CVIP_AF_FRAME_ID -
	 AMD_CAM_VENDOR_CONTROL_START] = {"cvip_af_frame_id", TYPE_INT32},
};

struct tag_info *tag_info[AMD_CAM_SECTION_COUNT] = {
	amd_cam_color_correction,
	amd_cam_control,
	amd_cam_demosaic,
	amd_cam_edge,
	amd_cam_flash,
	amd_cam_flash_info,
	amd_cam_hot_pixel,
	amd_cam_jpeg,
	amd_cam_lens,
	amd_cam_lens_info,
	amd_cam_noise_reduction,
	amd_cam_quirks,
	amd_cam_request,
	amd_cam_scaler,
	amd_cam_sensor,
	amd_cam_sensor_info,
	amd_cam_shading,
	amd_cam_statistics,
	amd_cam_statistics_info,
	amd_cam_tonemap,
	amd_cam_led,
	amd_cam_info,
	amd_cam_black_level,
	amd_cam_sync,
	amd_cam_reprocess,
	amd_cam_depth,
	amd_cam_logical_multi_camera,
	amd_cam_distortion_correction,
};

struct tag_info *vendor_tag_info[VENDOR_SECTION_COUNT] = {
	amd_cam_vendor_control,
};

const size_t amd_cam_metadata_type_size[MAX_NUM_TYPES] = {
	[TYPE_BYTE] = sizeof(u8),
	[TYPE_INT32] = sizeof(s32),
	[TYPE_FLOAT] = sizeof(float),
	[TYPE_INT64] = sizeof(s64),
	[TYPE_DOUBLE] = sizeof(double),
	[TYPE_RATIONAL] = sizeof(struct amd_cam_metadata_rational)
};

const char *amd_cam_metadata_type_names[MAX_NUM_TYPES] = {
	[TYPE_BYTE] = "byte",
	[TYPE_INT32] = "int",
	[TYPE_FLOAT] = "float",
	[TYPE_INT64] = "int64",
	[TYPE_DOUBLE] = "double",
	[TYPE_RATIONAL] = "rational"
};

size_t calc_amd_md_entry_data_size(u32 type, size_t data_count)
{
	size_t data_bytes = 0;

	if (type >= MAX_NUM_TYPES)
		return 0;

	data_bytes = data_count * amd_cam_metadata_type_size[type];

	return data_bytes;
}

static size_t calculate_amd_cam_metadata_entry_data_size(
	u32 type, size_t data_count)
{
	size_t data_bytes = 0;

	if (type >= MAX_NUM_TYPES)
		return 0;

	data_bytes = data_count * amd_cam_metadata_type_size[type];

	return data_bytes <= FOUR_BYTES ? 0 : data_bytes;
}

struct amd_cam_metadata_buffer_entry *amd_cam_get_entries(
		struct amd_cam_metadata *metadata)
{
	return (struct amd_cam_metadata_buffer_entry *)
		((u8 *)metadata + metadata->entries_start);
}

static u8 *amd_cam_get_data(struct amd_cam_metadata *metadata)
{
	return ((u8 *)metadata + metadata->data_start);
}

int amd_cam_find_metadata_entry(struct amd_cam_metadata *src,
		u32 tag, struct amd_cam_metadata_entry *entry)
{
	u32 index;
	struct amd_cam_metadata_buffer_entry *search_entry = NULL;

	if (!src) {
		pr_err("%s: invalid metadata pointer\n", __func__);
		return -EINVAL;
	}

	search_entry = amd_cam_get_entries(src);
	for (index = 0; index < src->entry_count; index++, search_entry++) {
		if (search_entry->tag == tag)
			break;
	}

	if (index == src->entry_count) {
		pr_debug("%s: %s tag not found\n", __func__,
			 amd_cam_get_metadata_tag_name(tag));
		return TAG_NOT_FOUND;
	}

	return amd_cam_get_metadata_entry(src, index, entry);
}

int amd_cam_get_metadata_entry(struct amd_cam_metadata *src,
		size_t index,
		struct amd_cam_metadata_entry *entry)
{
	struct amd_cam_metadata_buffer_entry *buffer_entry = NULL;

	if (!src || !entry) {
		pr_err("%s: invalid buffers\n", __func__);
		return ERROR;
	}

	if (index >= src->entry_count) {
		pr_err
		    ("%s:requested index %zu greater than the entry count %d\n",
		     __func__, index, src->entry_count);
		return ERROR;
	}

	buffer_entry = (amd_cam_get_entries(src) + index);

	entry->index = index;
	entry->tag = buffer_entry->tag;
	entry->type = buffer_entry->type;
	entry->count = buffer_entry->count;

	if ((buffer_entry->count *
	     amd_cam_metadata_type_size[buffer_entry->type]) > FOUR_BYTES) {
		entry->data.u8 =
		    (amd_cam_get_data(src) + buffer_entry->data.offset);
	} else {
		entry->data.u8 = buffer_entry->data.value;
	}

	return OK;
}

int amd_cam_add_metadata_entry(struct amd_cam_metadata *dst,
		u32 tag, u32 type, const void *data, size_t data_count)
{
	size_t data_bytes = 0, data_payload_bytes = 0;
	struct amd_cam_metadata_buffer_entry *entry = NULL;

	if (!dst) {
		pr_err("%s: invalid metadata buffer\n", __func__);
		return ERROR;
	}

	if (dst->entry_count == dst->entry_capacity) {
		pr_err("%s: entry count reaches the max capacity\n", __func__);
		return ERROR;
	}

	if (data_count && !data) {
		pr_err("%s: data pointer is NULL!\n", __func__);
		return ERROR;
	}

	pr_debug("%s: tag:0x%x, type:%d, data_count:%zu\n", __func__, tag, type,
		 data_count);

	data_bytes =
	    calculate_amd_cam_metadata_entry_data_size(type, data_count);

	if ((data_bytes + dst->data_count) > dst->data_capacity) {
		pr_err("%s: data size exceeds the maximum data capacity\n",
		       __func__);
		return ERROR;
	}

	data_payload_bytes = (data_count * amd_cam_metadata_type_size[type]);

	entry = (amd_cam_get_entries(dst) + dst->entry_count);

	memset(entry, 0, sizeof(struct amd_cam_metadata_buffer_entry));
	entry->tag = tag;
	entry->type = type;
	entry->count = data_count;

	if (data_bytes == 0) {
		memcpy(entry->data.value, data, data_payload_bytes);
	} else {
		entry->data.offset = dst->data_count;
		memcpy((amd_cam_get_data(dst) + entry->data.offset),
		       data, data_payload_bytes);
		dst->data_count += data_bytes;
	}
	dst->entry_count++;

	return OK;
}

int amd_cam_update_metadata_entry(struct amd_cam_metadata *dst,
		size_t index,
		const void *data, size_t data_count,
		struct amd_cam_metadata_entry *updated_entry)
{
	size_t data_bytes = 0, data_payload_bytes = 0;
	size_t entry_bytes = 0;
	struct amd_cam_metadata_buffer_entry *entry = NULL;

	if (!dst) {
		pr_err("%s: invalid metadata buffer\n", __func__);
		return ERROR;
	}

	if (index >= dst->entry_count) {
		pr_err("%s: requested tag index(%zu) not found\n", __func__,
		       index);
		return ERROR;
	}

	entry = amd_cam_get_entries(dst) + index;

	data_bytes =
	    calculate_amd_cam_metadata_entry_data_size(entry->type, data_count);
	data_payload_bytes =
	    (data_count * amd_cam_metadata_type_size[entry->type]);

	entry_bytes =
	    calculate_amd_cam_metadata_entry_data_size(entry->type,
						       entry->count);

	if (data_bytes != entry_bytes) {
		if (dst->data_capacity < (dst->data_count + data_bytes)) {
			pr_err
			("%s: no free space for updating the tag index %zu\n",
			     __func__, index);
			return ERROR;
		}
		//data size modified, append the new data at the end
		if (data_bytes != 0) {
			entry->data.offset = dst->data_count;
			memcpy((amd_cam_get_data(dst) + entry->data.offset),
			       data, data_payload_bytes);
			dst->data_count += data_bytes;
		}
	} else if (data_bytes != 0) {
		// data size unchanged, reuse same data location
		memcpy((amd_cam_get_data(dst) + entry->data.offset),
		       data, data_payload_bytes);
	}

	if (data_bytes == 0) {
		// Data fits into entry
		memcpy(entry->data.value, data, data_payload_bytes);
	}

	entry->count = data_count;

	if (updated_entry)
		amd_cam_get_metadata_entry(dst, index, updated_entry);

	return OK;
}

int amd_cam_update_tag(struct amd_cam_metadata *metadata, u32 tag,
		       const void *data, size_t data_count,
		       struct amd_cam_metadata_entry *updated_entry)
{
	int res = 0;
	u32 type;
	struct amd_cam_metadata_entry entry;

	res = amd_cam_get_metadata_tag_type(tag);
	if (res < 0) {
		pr_err("unable to find the type of tag 0x%x\n", tag);
		return res;
	}
	type = (u32)res;
	pr_debug("%s:tag 0x%x name %s type %d\n", __func__, tag,
		 amd_cam_get_metadata_tag_name(tag), type);

	res = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (res == TAG_NOT_FOUND) {
		res = amd_cam_add_metadata_entry(metadata,
						 tag, type, data, data_count);
	} else if (res == OK) {
		res = amd_cam_update_metadata_entry(metadata,
						    entry.index, data,
						    data_count, NULL);
	}

	return res;
}

struct tag_info *amd_cam_get_vendor_tag_info(u32 tag)
{
	u32 tag_section =
	    (tag - AMD_CAM_VENDOR_SECTION_START) >> TAG_SECTION_SHIFT;
	u32 tag_index;

	if (tag_section >= VENDOR_SECTION_COUNT) {
		pr_err("%s: tag 0x%x is after vendor section\n", __func__, tag);
		return NULL;
	}

	if (tag >= amd_cam_metadata_vendor_section_bounds[tag_section][1]) {
		pr_err("%s: tag 0x%x is outside of vendor section\n", __func__,
		       tag);
		return NULL;
	}

	tag_index =
	    tag - amd_cam_metadata_vendor_section_bounds[tag_section][0];
	pr_debug("%s: tag 0x%x, tag_sec:%d, tag_index:%d", __func__, tag,
		 tag_section, tag_index);
	return &vendor_tag_info[tag_section][tag_index];
}

int amd_cam_get_metadata_tag_type(u32 tag)
{
	struct tag_info *vndr_tag_info = NULL;
	u32 tag_section = tag >> TAG_SECTION_SHIFT;
	u32 tag_index;

	if (tag_section >= AMD_CAM_VENDOR_SECTION) {
		vndr_tag_info = amd_cam_get_vendor_tag_info(tag);
		if (!vndr_tag_info) {
			pr_err
			    ("%s: failed to find vendor tag info for tag 0x%x",
			     __func__, tag);
			return ERROR;
		}
		return vndr_tag_info->tag_type;
	}

	if (tag_section >= AMD_CAM_SECTION_COUNT ||
	    tag >= amd_cam_metadata_section_bounds[tag_section][1]) {
		return ERROR;
	}
	tag_index = tag & TAG_INDEX_MASK;

	return tag_info[tag_section][tag_index].tag_type;
}

const char *amd_cam_get_metadata_tag_name(u32 tag)
{
	struct tag_info *vndr_tag_info = NULL;
	u32 tag_section = tag >> TAG_SECTION_SHIFT;
	u32 tag_index;

	if (tag_section >= AMD_CAM_VENDOR_SECTION) {
		vndr_tag_info = amd_cam_get_vendor_tag_info(tag);
		if (!vndr_tag_info) {
			pr_err
			    ("%s: failed to find vendor tag info for tag 0x%x",
			     __func__, tag);
			return NULL;
		}
		return vndr_tag_info->tag_name;
	}

	if (tag_section >= AMD_CAM_SECTION_COUNT ||
	    tag >= amd_cam_metadata_section_bounds[tag_section][1]) {
		return NULL;
	}
	tag_index = tag & TAG_INDEX_MASK;

	return tag_info[tag_section][tag_index].tag_name;
}

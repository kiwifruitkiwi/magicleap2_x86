/*
 *Copyright © 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "amd_metadata.h"
#include "amd_stream.h"

int set_ae_lock(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	u8 ae_lock;
	struct amd_cam_metadata_entry entry;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
		AMD_CAM_CONTROL_AE_LOCK, &entry);
	if (ret) {
		loge("%s: tag AE LOCK not found\n", __func__);
		return ret;
	}
	ae_lock = entry.data.u8[0];

	if (ae_lock == AMD_CAM_CONTROL_AE_LOCK_ON) {
		//LOCK_TYPE_IMMEDIATELY
		LockType_t lock_mode = LOCK_TYPE_CONVERGENCE;

		isp->intf->auto_exposure_lock(isp->intf->context,
				s->cur_cfg.sensor_id, lock_mode);
	} else if (ae_lock == AMD_CAM_CONTROL_AE_LOCK_OFF)
		isp->intf->auto_exposure_unlock(isp->intf->context,
				s->cur_cfg.sensor_id);
	logi("%s: set ae lock(%d) suc!!", __func__, ae_lock);

	logi("exit %s", __func__);
	return 0;
}

int set_ae_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	u8 ae_mode;
	struct amd_cam_metadata_entry entry;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
			AMD_CAM_CONTROL_AE_MODE, &entry);
	if (ret) {
		loge("%s: tag AE MODE not found\n", __func__);
		return ret;
	}
	ae_mode = entry.data.u8[0];
	if (isp->intf->set_exposure_mode) {
		AeMode_t mode;

		if (ae_mode == AMD_CAM_CONTROL_AE_MODE_OFF)
			mode = AE_MODE_MANUAL;
		else
			mode = AE_MODE_AUTO;
		isp->intf->set_exposure_mode(isp->intf->context,
					s->cur_cfg.sensor_id, mode);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_itime(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	int64 etime;//ns
	int32 itime;//ms
	struct amd_cam_metadata_entry entry;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
			AMD_CAM_SENSOR_EXPOSURE_TIME, &entry);
	if (ret) {
		loge("%s: tag EXP TIME not found\n", __func__);
		return ret;
	}
	etime = entry.data.i64[0];
	itime = (int32)(etime / 1000 / 1000);
	if (isp->intf->set_snr_itime) {
		isp->intf->set_snr_itime(isp->intf->context,
				s->cur_cfg.sensor_id, itime);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_again(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	int32 aGain;
	struct amd_cam_metadata_entry entry;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
		AMD_CAM_SENSOR_SENSITIVITY, &entry);
	if (ret) {
		loge("%s: tag SENSOR SENS not found\n", __func__);
		return ret;
	}
	aGain = entry.data.i32[0];

	if (isp->intf->set_snr_ana_gain) {
		isp->intf->set_snr_ana_gain(isp->intf->context,
			s->cur_cfg.sensor_id, (uint32)aGain * 10);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_3a(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	struct amd_cam_metadata_entry entry;
	u8 control_mode;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
		AMD_CAM_CONTROL_MODE, &entry);
	if (ret) {
		loge("%s: CONTROL_MODE tag not found\n", __func__);
		return ret;
	}

	control_mode = entry.data.u8[0];

	if (control_mode == AMD_CAM_CONTROL_MODE_OFF) {
	//Full application control
		logi("%s: control mode off", __func__);

		ret = set_ae_mode(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set ae mode\n", __func__);
			return ret;
		}

		ret = set_ae_lock(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set ae lock\n", __func__);
			return ret;
		}

		ret = set_itime(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set itime\n", __func__);
			return ret;
		}

		ret = set_again(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set again\n", __func__);
			return ret;
		}
	} else if (control_mode == AMD_CAM_CONTROL_MODE_AUTO) {
	//Manual control disabled
		logi("%s: control mode auto", __func__);

		ret = set_ae_mode(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set ae mode\n", __func__);
			return ret;
		}
	} else if (control_mode == AMD_CAM_CONTROL_MODE_USE_SCENE_MODE) {
	//Use a specific scene mode and disable 3A mode controls
		logi("%s: control mode scene", __func__);
		ret = set_scene_mode(isp, s, metadata);
		if (ret) {
			loge("%s: failed to set scene mode\n", __func__);
			return ret;
		}
	} else if (control_mode == AMD_CAM_CONTROL_MODE_OFF_KEEP_STATE) {
	//Lock and use the last 3A values
		logi("%s: control mode keep", __func__);
	} else {
		loge("%s: unknown control mode!", __func__);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_flicker_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	struct amd_cam_metadata_entry entry;
	u8 mode;
	enum anti_banding_mode anti_banding;

	logi("entry %s", __func__);
	ret = amd_cam_find_metadata_entry(metadata,
		AMD_CAM_CONTROL_AE_ANTIBANDING_MODE, &entry);
	if (ret) {
		loge("%s: tag anti banding mode not found\n", __func__);
		return ret;
	}
	mode = entry.data.u8[0];

	if (mode == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_OFF)
		anti_banding = ANTI_BANDING_MODE_DISABLE;
	else if (mode == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_50HZ)
		anti_banding = ANTI_BANDING_MODE_50HZ;
	else if (mode == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_60HZ)
		anti_banding = ANTI_BANDING_MODE_60HZ;
	else if (mode == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_AUTO)
		anti_banding = ANTI_BANDING_MODE_50HZ;
	else
		loge("%s: unknown flicker mode[%d]", __func__, mode);

	if (isp->intf->set_snr_ana_gain) {
		isp->intf->set_flicker(isp->intf->context,
			s->cur_cfg.sensor_id, anti_banding);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_ae_regions(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	struct amd_cam_metadata_entry entry;
	window_t win;
	enum amd_camera_metadata_tag tag;

	logi("entry %s", __func__);

	//android.control.aeRegions 5n i32 (xmin, ymin, xmax, ymax, w[0, 1000])
	//Only use the first one
	tag = AMD_CAM_CONTROL_AE_REGIONS;
	ret = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (ret) {
		loge("%s: tag SENSOR SENS not found\n", __func__);
		return ret;
	}

	win.h_offset = entry.data.i32[0];
	win.v_offset = entry.data.i32[1];
	win.h_size = entry.data.i32[2] - entry.data.i32[0];
	win.v_size = entry.data.i32[3] - entry.data.i32[1];

	if (isp->intf->set_ae_roi) {
		isp->intf->set_ae_roi(isp->intf->context,
			s->cur_cfg.sensor_id, &win);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_ev_compensation(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	enum amd_camera_metadata_tag tag;
	struct amd_cam_metadata_entry entry;
	AeEv_t ev;
	int32 tmp;

	logi("entry %s", __func__);

	//android.control.aeExposureCompensation 1 i32
	tag = AMD_CAM_CONTROL_AE_EXPOSURE_COMPENSATION;
	ret = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (ret) {
		loge("%s: tag SENSOR SENS not found\n", __func__);
		return ret;
	}
	tmp = 100 * entry.data.i32[0];

	//android.control.aeCompensationStep 1 r
	tag = AMD_CAM_CONTROL_AE_COMPENSATION_STEP;
	if (ret) {
		loge("%s: tag SENSOR SENS not found\n", __func__);
		return ret;
	}
	ev.numerator =
		tmp * entry.data.r[0].numerator / entry.data.r[0].denominator;
	ev.denominator = 100;

	if (isp->intf->set_ev_compensation) {
		isp->intf->set_ev_compensation(isp->intf->context,
			s->cur_cfg.sensor_id, &ev);
	}

	logi("exit %s", __func__);
	return 0;
}

int set_scene_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata)
{
	int ret;
	enum amd_camera_metadata_tag tag;
	struct amd_cam_metadata_entry e;
	enum isp_scene_mode mode;

	logi("entry %s", __func__);

	////android.control.sceneMode 1 u8
	tag = AMD_CAM_CONTROL_SCENE_MODE;
	ret = amd_cam_find_metadata_entry(metadata, tag, &e);
	if (ret) {
		loge("%s: tag SENSOR SENS not found\n", __func__);
		return ret;
	}

	if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_ACTION)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_PORTRAIT)
		mode = ISP_SCENE_MODE_PORTRAIT;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_LANDSCAPE)
		mode = ISP_SCENE_MODE_LANDSCAPE;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_NIGHT)
		mode = ISP_SCENE_MODE_NIGHT;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_NIGHT_PORTRAIT)
		mode = ISP_SCENE_MODE_NIGHTPORTRAIT;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_THEATRE)
		mode = ISP_SCENE_MODE_SUNSET;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_BEACH)
		mode = ISP_SCENE_MODE_BEACH;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_SNOW)
		mode = ISP_SCENE_MODE_SNOW;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_SUNSET)
		mode = ISP_SCENE_MODE_SUNSET;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_STEADYPHOTO)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_FIREWORKS)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_SPORTS)
		mode = ISP_SCENE_MODE_SPORT;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_PARTY)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_CANDLELIGHT)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_BARCODE)
		mode = ISP_SCENE_MODE_BARCODE;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_HDR)
		mode = ISP_SCENE_MODE_AUTO;
	else if (e.data.u8[0] == AMD_CAM_CONTROL_SCENE_MODE_DISABLED)
		mode = ISP_SCENE_MODE_AUTO;
	else
		mode = ISP_SCENE_MODE_MAX;

	if (isp->intf->set_scene_mode) {
		isp->intf->set_scene_mode(isp->intf->context,
			s->cur_cfg.sensor_id, mode);
	}

	logi("exit %s", __func__);
	return 0;
}

extern int set_kparam_2_fw(struct cam_stream *s)
{
	int ret;
//	struct amd_cam_metadata_entry entry;
	struct isp_info *isp = v4l2_get_subdevdata(s->isp_sd);
	struct stream_buf *s_buf;
	struct amd_cam_metadata *metadata;

	logi("entry %s", __func__);
	s_buf = container_of(s->vidq.bufs[s->dq_id],
		struct stream_buf, vb);

	metadata = (struct amd_cam_metadata *)s_buf->kparam;

	ret = set_flicker_mode(isp, s, metadata);
	if (ret) {
		loge("%s: failed to set flicker\n", __func__);
		return ret;
	}

	ret = set_3a(isp, s, metadata);
	if (ret) {
		loge("%s: failed to set 3a\n", __func__);
		return ret;
	}

	ret = set_ae_regions(isp, s, metadata);
	if (ret) {
		loge("%s: failed to set regions\n", __func__);
		return ret;
	}

	ret = set_ev_compensation(isp, s, metadata);
	if (ret) {
		loge("%s: failed to set regions\n", __func__);
		return ret;
	}

	//AMD_CAM_CONTROL_AE_TARGET_FPS_RANGE:
		//set frame rate info ???

	//AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER:
		//CMD_ID_AE_PRECAPTURE

	logi("exit %s", __func__);
	return 0;
}


extern int update_kparam(struct cam_stream *s)
{
	MetaInfo_t *meta;
	int64_t itime;//ms
	int32 aGain;
	int64 frame_duration;
	struct stream_buf *s_buf;
	struct amd_cam_metadata *metadata;
	int ret;
	u8 control_mode;
	enum amd_camera_metadata_tag tag;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states states_3a;
	s32 crop_region[4], a_region[5];

	logi("entry %s", __func__);
	s_buf = container_of(s->vidq.bufs[s->dq_id],
		struct stream_buf, vb);
	meta = s_buf->m;

	metadata = (struct amd_cam_metadata *)s_buf->kparam;


	ret = amd_cam_find_metadata_entry(metadata,
		AMD_CAM_CONTROL_MODE, &entry);
	if (ret) {
		loge("3A: CONTROL_MODE tag not found\n");
		return ret;
	}

	control_mode = entry.data.u8[0];

	if (control_mode == AMD_CAM_CONTROL_MODE_OFF) {
		states_3a.prv_control_mode = AMD_CAM_CONTROL_MODE_OFF;
		states_3a.prv_scene_mode =
			AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
		states_3a.prv_ae_mode = AMD_CAM_CONTROL_AE_MODE_OFF;
		states_3a.prv_af_mode = AMD_CAM_CONTROL_AF_MODE_OFF;

		states_3a.ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		states_3a.af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		states_3a.awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;

		amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AE_STATE,
			(const void *)&states_3a.ae_state, 1, NULL);

		amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AF_STATE,
			(const void *)&states_3a.af_state, 1, NULL);

		amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AWB_STATE,
			(const void *)&states_3a.awb_state, 1, NULL);
	}

//test
//	if(meta->ae.mode == AE_MODE_MANUAL) {
		itime = (int64_t)(meta->ae.itime * 1000 * 1000);
		ret = amd_cam_update_tag(metadata,
			AMD_CAM_SENSOR_EXPOSURE_TIME,
			(const void *)&itime, 1, NULL);
		if (ret) {
			loge("%s: update itime failed with %d", __func__, ret);
			return ret;
		}

		aGain = meta->ae.aGain;
		ret = amd_cam_update_tag(metadata, AMD_CAM_SENSOR_SENSITIVITY,
			(const void *)&itime, 1, NULL);
		if (ret) {
			loge("%s: update again failed with %d", __func__, ret);
			return ret;
		}
		//To Do: update the duration
		frame_duration = 1000 * 1000 * 1000 / 15;
		amd_cam_update_tag(metadata,
			AMD_CAM_SENSOR_INFO_MAX_FRAME_DURATION,
			(const void *)&frame_duration, 1, NULL);
		if (ret) {
			loge("%s: update fd failed with %d", __func__, ret);
			return ret;
		}
//	}


	tag = AMD_CAM_SCALER_CROP_REGION;
	ret = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (ret)
		return ret;

	crop_region[0] = entry.data.i32[0];
	crop_region[1] = entry.data.i32[1];
	crop_region[2] = (entry.data.i32[2] + crop_region[0]);
	crop_region[3] = (entry.data.i32[3] + crop_region[1]);

	tag = AMD_CAM_CONTROL_AE_REGIONS;
	ret = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (ret)
		return ret;

	a_region[4] = entry.data.i32[4];
	a_region[0] = meta->ae.regionWindow.h_offset;
	a_region[1] = meta->ae.regionWindow.v_offset;
	a_region[2] = a_region[0] + meta->ae.regionWindow.h_size;
	a_region[3] = a_region[1] + meta->ae.regionWindow.v_size;
	// calculate the intersection of AE/AF/AWB and CROP regions
	if (a_region[0] < crop_region[2] && crop_region[0] < a_region[2] &&
	    a_region[1] < crop_region[3] && crop_region[1] < a_region[3]) {
		s32 inter_sect[5];
		size_t data_count = 5;

		inter_sect[0] = max(a_region[0], crop_region[0]);
		inter_sect[1] = max(a_region[1], crop_region[1]);
		inter_sect[2] = min(a_region[2], crop_region[2]);
		inter_sect[3] = min(a_region[3], crop_region[3]);
		inter_sect[4] = a_region[4];

		tag = AMD_CAM_CONTROL_AE_REGIONS;
		amd_cam_update_tag(metadata, tag,
			(const void *)&inter_sect[0], data_count, NULL);
	}

	logi("exit %s", __func__);
	return 0;
}

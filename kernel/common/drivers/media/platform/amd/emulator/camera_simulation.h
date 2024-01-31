/*
 * Copyright (C) 2020 Advanced Micro Devices.
 */

#ifndef __AMD_SIMULATION_H_
#define __AMD_SIMULATION_H_

#include <linux/cam_api_simulation.h>
#include <linux/types.h>

#define MAX_REQ_DEPTH MAX_BUFFERS_PER_STREAM

struct algo_3a_states {
	u8 prv_control_mode;
	u8 prv_scene_mode;
	u8 prv_ae_mode;
	u8 prv_af_mode;
	u8 ae_state;
	u8 af_state;
	u8 awb_state;
};

int set_properties(struct s_properties *prop);
int ioctl_simu_done_buf(struct simu_cam_buf *done_buf);
int ioctl_simu_streamon(struct file *file,
			struct kernel_stream_indexes *stream_on);
int ioctl_simu_streamoff(struct file *file,
			 struct kernel_stream_indexes *stream_off);
int ioctl_simu_streamdestroy(struct file *file,
			     struct kernel_stream_indexes *stream_destroy);
int ioctl_simu_config(struct file *file,
		      struct kernel_stream_configuration *config);
int ioctl_simu_request(struct kernel_request *request);
int ioctl_simu_result(struct kernel_result *result);
int process_simu_3a(struct amd_cam_metadata *metadata);
int do_simu_ae(struct amd_cam_metadata *metadata);
int do_simu_af(struct amd_cam_metadata *metadata);
int do_simu_awb(struct amd_cam_metadata *metadata);
void update_simu_3a(struct amd_cam_metadata *metadata);
void update_simu_3a_region(u32 tag, struct amd_cam_metadata *metadata);

#endif /* camera_simulation.h */

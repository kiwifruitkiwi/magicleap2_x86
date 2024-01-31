/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef TUNING_DATA_HEADER_H
#define TUNING_DATA_HEADER_H

/*
 *Note: this file will be shared in driver and AIDT.
 */

#define MAX_PROFILE_NO		10
#define PITEMSTEP		4
#define MAX_SCENEMODENUMB	13

enum expo_limit_type {
	EXPO_LIMIT_TYPE_NO,     //no limit
	EXPO_LIMIT_TYPE_LONG,   //long exposure,explicit value is determined by
				//tuning data
	EXPO_LIMIT_TYPE_SHORT   //short exposure,explicit value is determined by
				//tuning data
};

struct _profile_header {
	unsigned int size;		//tuning data length
	unsigned int offset;		//offset to header
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int hdr;
	enum expo_limit_type expo_limit;
};

struct _cproc_header {
	int saturation;
	int hue;
	int contrast;
	int brightness;
};

struct _scene_mode_table {
	int sm_valid[MAX_SCENEMODENUMB];	//0 off; 1 on
	int ae_sp[MAX_SCENEMODENUMB];	// AE set point
	int ae_maxitime[MAX_SCENEMODENUMB];	// AE maximum itime
	int ae_iso[MAX_SCENEMODENUMB];	//AE ISO
	int ae_fps[MAX_SCENEMODENUMB][2];	//AE FPS[min, max]
	//AWB index, -1 means awb; >0 means switch to MWB
	int awb_index[MAX_SCENEMODENUMB];
	int af[MAX_SCENEMODENUMB][2];	//AF [min, max]
	int denoise[MAX_SCENEMODENUMB][7];
	//saturation, brightness, hue and contrast order
	int cproc[MAX_SCENEMODENUMB][4];
	int sharpness[MAX_SCENEMODENUMB][5];
	//-1 means auto, 0 means off; 1 means on; 2 means ignore.
	int flashlight[MAX_SCENEMODENUMB];
};

struct _wb_light_source_index {
	unsigned int index_map[6];
};

//#define DIR_PARAM_V1 // remove the macro after tuning data header updated
#ifndef DIR_PARAM_V1
#define DIR_PARAM_V2
#endif

#ifdef DIR_PARAM_V1
struct {
	unsigned int black_level[4];	// Black level R,G,B,I
	unsigned int max_val;
	unsigned int thres[4];
	float rdir_coeff[10];
	float gdir_coeff[10];
	float bdir_coeff[10];
	float rgb2ir_coeff[3];
};
#endif

#ifdef DIR_PARAM_V2
struct _module_def {
	float black_level[4];	// Black level R,G,B,IR
	// forward compatibility (may abandon in future)
	float max_val;
	// Matches original THRES[4] (RGB_TH[0], RGB_TH[1], RGB_TH[2],
	//			RGB_TH[3],IR_TH, RGB_UTH, OutRGBTH, ??) (TBD)
	float thres[8];
	float dp_th[2];	// Dead pixel thresholds (DPU_TH & DPA_TH) (TBD)
	float ratio[2];		// (CompRatio, CutRatio) (TBD)
	float axc[6];		// (ARC, AGC, ABC, ARFC, AGFC, ABFC) (TBD)
	float rdir_coeff[9];	// Get rid of last member
	float gdir_coeff[9];	// Get rid of last member
	float bdir_coeff[9];	// Get rid of last member
	float rgb2ir_coeff[4];	// Keep
};
#endif

struct _package_td_header {
	//indicate the tuning data is before/after structure update.
	//0x33ffffffffffffff means after tuning data update.
	unsigned int magic_no_high;
	unsigned int magic_no_low;
	unsigned int length;		// file total length.
	unsigned int struct_rev;
	unsigned int date;
	char author[32];
	char cam_name[32];	//module name
	unsigned int pro_no;		//support profile numbers
	struct _profile_header pro[MAX_PROFILE_NO];
	unsigned int crc[7 + MAX_PROFILE_NO];
	//struct _cproc_header cproc_value;
	//saturation, brightness, hue and contrast order

	//add scene mode table here
	//scene mode table structure
	struct _scene_mode_table scene_mode_table;
	struct _wb_light_source_index wb_idx;
	struct _module_def dir_para;

	//if crop_wnd_width or crop_wnd_height is 0, they will be ignored.
	//if crop_wnd_width or crop_wnd_height is bigger than
	//aspect ratio window
	// they will also be ignored
	//otherwise driver will send them down by digital zoom command
	//and make sure they
	//are at the center of the aspect ratio window
	unsigned int crop_wnd_width;
	unsigned int crop_wnd_height;
};

#endif				// TUNING_DATA_HEADER_H

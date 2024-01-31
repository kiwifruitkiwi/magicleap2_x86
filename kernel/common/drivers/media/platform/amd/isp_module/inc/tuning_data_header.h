#ifndef TUNING_DATA_HEADER_H
#define TUNING_DATA_HEADER_H

/*
 *Note: this file will be shared in driver and AIDT.
 */

#define MAX_PROFILE_NO		10
#define PITEMSTEP		4
#define MAX_SCENEMODENUMB	13

typedef struct _profile_header {
	uint32 size;		//tuning data length
	uint32 offset;		//offset to header
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 hdr;
} profile_header;

typedef struct _cproc_header {
	int32 saturation;
	int32 hue;
	int32 contrast;
	int32 brightness;
} cproc_header;

typedef struct _scene_mode_table {
	int32 sm_valid[MAX_SCENEMODENUMB];	//0 off; 1 on
	int32 ae_sp[MAX_SCENEMODENUMB];	// AE set point
	int32 ae_maxitime[MAX_SCENEMODENUMB];	// AE maximum itime
	int32 ae_iso[MAX_SCENEMODENUMB];	//AE ISO
	int32 ae_fps[MAX_SCENEMODENUMB][2];	//AE FPS[min, max]
	//AWB index, -1 means awb; >0 means switch to MWB
	int32 awb_index[MAX_SCENEMODENUMB];
	int32 af[MAX_SCENEMODENUMB][2];	//AF [min, max]
	int32 denoise[MAX_SCENEMODENUMB][7];
	//saturation, brightness, hue and contrast order
	int32 cproc[MAX_SCENEMODENUMB][4];
	int32 sharpness[MAX_SCENEMODENUMB][5];
	//-1 means auto, 0 means off; 1 means on; 2 means ignore.
	int32 flashlight[MAX_SCENEMODENUMB];
} scene_mode_table_header;

typedef struct {
	uint32 index_map[6];
} wb_light_source_index;

//#define DIR_PARAM_V1 // remove the macro after tuning data header updated
#ifndef DIR_PARAM_V1
#define DIR_PARAM_V2
#endif

#ifdef DIR_PARAM_V1
typedef struct {
	uint32 black_level[4];	// Black level R,G,B,I
	uint32 max_val;
	uint32 thres[4];
	float rdir_coeff[10];
	float gdir_coeff[10];
	float bdir_coeff[10];
	float rgb2ir_coeff[3];
} module_def;
#endif

#ifdef DIR_PARAM_V2
typedef struct {
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
} module_def;
#endif

typedef struct _package_td_header {
	//indicate the tuning data is before/after structure update.
	//0x33ffffffffffffff means after tuning data update.
	uint32 magic_no_high;
	uint32 magic_no_low;
	uint32 length;		// file total length.
	uint32 struct_rev;
	uint32 date;
	char author[32];
	char cam_name[32];	//module name
	uint32 pro_no;		//support profile numbers
	profile_header pro[MAX_PROFILE_NO];
	uint32 crc[7 + MAX_PROFILE_NO];
	//cproc_header cproc_value;
	//saturation, brightness, hue and contrast order

	//add scene mode table here
	scene_mode_table_header scene_mode_table;//scene mode table structure
	wb_light_source_index wb_idx;
	module_def dir_para;

	//if crop_wnd_width or crop_wnd_height is 0, they will be ignored.
	//if crop_wnd_width or crop_wnd_height is bigger than
	//aspect ratio window
	// they will also be ignored
	//otherwise driver will send them down by digital zoom command
	//and make sure they
	//are at the center of the aspect ratio window
	uint32 crop_wnd_width;
	uint32 crop_wnd_height;
} package_td_header;

#endif				// TUNING_DATA_HEADER_H

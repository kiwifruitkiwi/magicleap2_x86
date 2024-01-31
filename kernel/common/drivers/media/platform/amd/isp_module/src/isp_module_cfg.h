/**************************************************************************
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
 **************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

//#ifndef TEST_ISP_GFX_IF
//#define TEST_ISP_GFX_IF
//#endif

#ifndef USING_NATIVE_SYS_MALLOC_FUNC
#define USING_NATIVE_SYS_MALLOC_FUNC
#endif

#ifndef HAL_ALTERA_PCI
#define HAL_ALTERA_PCI
#endif

#ifndef HAL_ALTERA_PCI
#define HAL_ALTERA_PCI
#endif

#ifndef SENSOR_DRIVER_ON_HOST
#define SENSOR_DRIVER_ON_HOST
#endif

#ifndef FPGA_DRV_READY
#define FPGA_DRV_READY
#endif

#ifndef ENABLE_SENSOR_TUNING
#define ENABLE_SENSOR_TUNING
#endif
/*
 *#ifndef FPGA_VERSION_ISP1P1
 *#define FPGA_VERSION_ISP1P1
 *#endif
 */
#ifndef HDMI_YUV422_DISPLAY
#define HDMI_YUV422_DISPLAY
#endif

//#ifndef SUPPORT_13M_PICTURE_FOR_FPGA
//#define SUPPORT_13M_PICTURE_FOR_FPGA
//#endif

/*
 *#ifndef DBG
 *#define DBG 1
 *#endif
 */

#define IDLE_DETECT_INTERVAL		10	/*ms */
#define SENSOR_PWR_OFF_DEALY_TIME	500	/*ms */
#define ISP_PWR_OFF_DELAY_TIME		500	/*ms */
#define WORK_ITEM_INTERVAL		5	/*ms */

#ifndef ISP_FW_CTRL_3A
#define ISP_FW_CTRL_3A
#endif

//#ifndef ISP_TUNING_TOOL_SUPPORT
//#define ISP_TUNING_TOOL_SUPPORT
//#endif

//#ifndef DEBUG_AE_ROI
//#define DEBUG_AE_ROI
//#endif

//#ifndef PRINT_FW_LOG
//#define PRINT_FW_LOG
//#endif

//#ifndef USING_PREVIEW_TO_TRIGGER_ZSL
//#define USING_PREVIEW_TO_TRIGGER_ZSL
//#endif

//#ifndef DUMP_PREVIEW_WITH_META
//#define DUMP_PREVIEW_WITH_META
//#endif

//#ifndef DUMP_RGBIR_RAW_OUTPUT
//#define DUMP_RGBIR_RAW_OUTPUT
//#endif

//#ifndef DUMP_PIN_OUTPUT
//#define DUMP_PIN_OUTPUT
//#endif

#ifndef IGNORE_PWR_REQ_FAIL
#define IGNORE_PWR_REQ_FAIL
#endif

//#define CONFIG_ENABLE_CMD_RESP_256_BYTE

//#define USING_PREALLOCATED_FB
//#define USING_PREALLOCATED_FB_FOR_MAP

//#ifdef	USING_PREALLOCATED_FB
//#define USING_ALLOC_AND_MAP_FOR_TMP
//#define USING_WIRTECOMBINE_BUFFER_FOR_TMP
//#define FRONT_CAMERA_SUPPORT_ONLY
//#define USING_FB_FOR_RGBIR_OUTPUT
//#endif

//#define USING_FB_FOR_RGBIR_FRAME_INFO

//#define USING_WIRTECOMBINE_BUFFER_FOR_STAGE2

#define SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER
#ifdef	SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER
//#define SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER_SWRT
#define SET_STAGE_MEM_ACCESS_BY_DAGB_REGISTER_HRT
#endif

//#define SET_P_STAGE_WATERMARK_REGISTER

//don't enable this until fw adds corresponding support otherwise it
//will make two cameras case fail
//#define ENABLE_ISP_DYNAMIC_CLOCK

//if enable this macro, isp module will control isp power and clock by
//operating RSMU register instead of using kmd interface
//#define DO_ISP_PWR_CLK_CTRL_BY_RSMU_REGISTER

//define this macro to support mainline KMD otherwise
//to support BU KMD(16.59.1002.1001)

//define this macro to support new AWB
#define SUPPORT_NEW_AWB

//when this macro is defined, we must use FW built from
//camip/isp/2.0/main/src/fw/stg_2_0_new_awb_winston/
//instead of //camip/isp/2.0/main/src/fw/stg_2_0_new_awb/
//#define HOST_PROCESSING_3A

//#define ENABLE_PERFORMANCE_PROFILE_LOG
//only to define this if wanting to verify ISP FW loadingin KMD
//and this macro should never be defined when built with avstream
//or for formal KMD
//#define TEST_KICKING_OFF_FW_IN_KMD

//#define ENABLE_MULTIPLE_MAP

#endif

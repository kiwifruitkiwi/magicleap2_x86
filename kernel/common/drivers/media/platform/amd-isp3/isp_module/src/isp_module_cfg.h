/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef CONFIG_H
#define CONFIG_H

#define IDLE_DETECT_INTERVAL		10	/*ms */
#define SENSOR_PWR_OFF_DEALY_TIME	500	/*ms */
#define ISP_PWR_OFF_DELAY_TIME		500	/*ms */
#define WORK_ITEM_INTERVAL		5	/*ms */

#ifndef IGNORE_PWR_REQ_FAIL
#define IGNORE_PWR_REQ_FAIL
#endif

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

//only to define this if wanting to verify ISP FW loadingin KMD
//and this macro should never be defined when built with avstream
//or for formal KMD
//#define TEST_KICKING_OFF_FW_IN_KMD

//#define ENABLE_MULTIPLE_MAP

#endif

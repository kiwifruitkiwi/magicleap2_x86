/*
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_ISP_H__
#define __IPC_ISP_H__

/* Registers from ISP_IMC block */
#define mmISP_IMC_FW_EN                                 0x67000
#define mmISP_IMC_CVP_INTR_CTRL_INFO                    0x67040
#define mmISP_IMC_CVP_CAM0_ACK_ID                       0x67044
#define mmISP_IMC_CVP_CAM0_RESP_ID                      0x67048
#define mmISP_IMC_CVP_CAM0_RESP_STATUS                  0x6704C
#define mmISP_IMC_CVP_CAM0_INFO_STRM                    0x67050
#define mmISP_IMC_CVP_CAM0_INFO_RES                     0x67054
#define mmISP_IMC_CVP_CAM0_INFO_FPS                     0x67058
#define mmISP_IMC_CVP_CAM0_INFO_AG0                     0x6705C
#define mmISP_IMC_CVP_CAM0_INFO_DG0                     0x67060
#define mmISP_IMC_CVP_CAM0_INFO_ITIME0                  0x67064
#define mmISP_IMC_CVP_CAM0_INFO_AG1                     0x67068
#define mmISP_IMC_CVP_CAM0_INFO_DG1                     0x6706C
#define mmISP_IMC_CVP_CAM0_INFO_ITIME1                  0x67070
#define mmISP_IMC_CVP_CAM0_INFO_AG2                     0x67074
#define mmISP_IMC_CVP_CAM0_INFO_DG2                     0x67078
#define mmISP_IMC_CVP_CAM0_INFO_ITIME2                  0x6707C
#define mmISP_IMC_CVP_CAM1_ACK_ID                       0x67080
#define mmISP_IMC_CVP_CAM1_RESP_ID                      0x67084
#define mmISP_IMC_CVP_CAM1_RESP_STATUS                  0x67088
#define mmISP_IMC_CVP_CAM1_INFO_STRM                    0x6708C
#define mmISP_IMC_CVP_CAM1_INFO_RES                     0x67090
#define mmISP_IMC_CVP_CAM1_INFO_FPS                     0x67094
#define mmISP_IMC_CVP_CAM1_INFO_AG0                     0x67098
#define mmISP_IMC_CVP_CAM1_INFO_DG0                     0x6709C
#define mmISP_IMC_CVP_CAM1_INFO_ITIME0                  0x670A0
#define mmISP_IMC_CVP_CAM1_INFO_AG1                     0x670A4
#define mmISP_IMC_CVP_CAM1_INFO_DG1                     0x670A8
#define mmISP_IMC_CVP_CAM1_INFO_ITIME1                  0x670AC
#define mmISP_IMC_CVP_CAM1_INFO_AG2                     0x670B0
#define mmISP_IMC_CVP_CAM1_INFO_DG2                     0x670B4
#define mmISP_IMC_CVP_CAM1_INFO_ITIME2                  0x670B8
#define mmISP_IMC_FW_CAM0_ACK_ID                        0x67100
#define mmISP_IMC_FW_CAM0_RESP_ID                       0x67104
#define mmISP_IMC_FW_CAM0_RESP_STATUS                   0x67108
#define mmISP_IMC_FW_CAM1_ACK_ID                        0x6710C
#define mmISP_IMC_FW_CAM1_RESP_ID                       0x67110
#define mmISP_IMC_FW_CAM1_RESP_STATUS                   0x67114
#define mmISP_IMC_FW_DMA_STATUS_ACK_CTRL                0x67118
#define mmISP_IMC_FW_CAM_CTRL_MIS                       0x67140
#define mmISP_IMC_FW_CAM_CTRL_RIS                       0x67144
#define mmISP_IMC_FW_CAM_CTRL_MASK                      0x67148
#define mmISP_IMC_FW_CAM_CTRL_CLR                       0x6714C
#define mmISP_IMC_FW_CAM_CTRL_ISR                       0x67150
#define mmISP_IMC_CVP_INBUF_REL_ACK                     0x67400
#define mmISP_IMC_CVP_INBUF_CAM0_SET_DROP_ID            0x67404
#define mmISP_IMC_CVP_INBUF_CAM1_SET_DROP_ID            0x67408
#define mmISP_IMC_CVP_INBUF_CAM0_ADDR_OFFSET            0x6740C
#define mmISP_IMC_CVP_INBUF_CAM0_EXP12_GAP              0x67410
#define mmISP_IMC_CVP_INBUF_CAM0_FRMSZ_BITDP            0x67414
#define mmISP_IMC_CVP_INBUF_CAM0_EXP0_FRM_SLC_ID        0x67418
#define mmISP_IMC_CVP_INBUF_CAM0_EXP1_FRM_SLC_ID        0x6741C
#define mmISP_IMC_CVP_INBUF_CAM0_EXP2_FRM_SLC_ID        0x67420
#define mmISP_IMC_CVP_INBUF_CAM1_ADDR_OFFSET            0x67428
#define mmISP_IMC_CVP_INBUF_CAM1_EXP12_GAP              0x6742C
#define mmISP_IMC_CVP_INBUF_CAM1_FRMSZ_BITDP            0x67430
#define mmISP_IMC_CVP_INBUF_CAM1_EXP0_FRM_SLC_ID        0x67434
#define mmISP_IMC_CVP_INBUF_CAM1_EXP1_FRM_SLC_ID        0x67438
#define mmISP_IMC_CVP_INBUF_CAM1_EXP2_FRM_SLC_ID        0x6743C
#define mmISP_IMC_CVP_INBUF_CAM2_ADDR_OFFSET            0x67448
#define mmISP_IMC_CVP_INBUF_CAM2_EXP12_GAP              0x6744C
#define mmISP_IMC_CVP_INBUF_CAM2_FRMSZ_BITDP            0x67450
#define mmISP_IMC_CVP_INBUF_CAM2_EXP0_FRM_SLC_ID        0x67454
#define mmISP_IMC_CVP_INBUF_CAM2_EXP1_FRM_SLC_ID        0x67458
#define mmISP_IMC_CVP_INBUF_CAM2_EXP2_FRM_SLC_ID        0x6745C
#define mmISP_IMC_CVP_INBUF_CAM0_PRE_DATA_ADDR_OFFSET   0x67470
#define mmISP_IMC_CVP_INBUF_CAM0_PRE_DATA_FRM_ID        0x67474
#define mmISP_IMC_CVP_INBUF_CAM1_PRE_DATA_ADDR_OFFSET   0x67478
#define mmISP_IMC_CVP_INBUF_CAM1_PRE_DATA_FRM_ID        0x6747C
#define mmISP_IMC_CVP_INBUF_CAM0_POST_DATA_ADDR_OFFSET  0x67480
#define mmISP_IMC_CVP_INBUF_CAM0_POST_DATA_FRM_ID       0x67484
#define mmISP_IMC_CVP_INBUF_CAM1_POST_DATA_ADDR_OFFSET  0x67488
#define mmISP_IMC_CVP_INBUF_CAM1_POST_DATA_FRM_ID       0x6748C
#define mmISP_IMC_FW_INBUF_CTRL                         0x67500
#define mmISP_IMC_FW_INBUF_FRM_ID_OVFL_TH               0x67504
#define mmISP_IMC_FW_INBUF_SLC_NUM                      0x67508
#define mmISP_IMC_FW_INBUF_SET_CAM0_DROP_ID             0x67510
#define mmISP_IMC_FW_INBUF_SET_CAM1_DROP_ID             0x67514
#define mmISP_IMC_FW_INBUF_SET_CAM2_DROP_ID             0x67518
#define mmISP_IMC_FW_INBUF_FRM_ID                       0x67580
#define mmISP_IMC_FW_INBUF_ADDR_OFFSET                  0x67584
#define mmISP_IMC_FW_INBUF_EXP12_GAP                    0x67588
#define mmISP_IMC_FW_INBUF_FRMSZ_BITDP                  0x6758C
#define mmISP_IMC_FW_INBUF_CAM0_DROP_ID                 0x67598
#define mmISP_IMC_FW_INBUF_CAM1_DROP_ID                 0x6759C
#define mmISP_IMC_FW_INBUF_CAM2_DROP_ID                 0x675A0
#define mmISP_IMC_FW_PRE_DATA_ADDR_OFFSET               0x675A4
#define mmISP_IMC_FW_PRE_DATA_FRM_ID                    0x675A8
#define mmISP_IMC_FW_POST_DATA_ADDR_OFFSET              0x675AC
#define mmISP_IMC_FW_POST_DATA_FRM_ID                   0x675B0
#define mmISP_IMC_FW_INBUF_MIS                          0x675C0
#define mmISP_IMC_FW_INBUF_RIS                          0x675C4
#define mmISP_IMC_FW_INBUF_MASK                         0x675C8
#define mmISP_IMC_FW_INBUF_CLR                          0x675CC
#define mmISP_IMC_FW_INBUF_ISR                          0x675D0
#define mmISP_IMC_FW_INBUF_SLC_BUF_FILL_LV              0x675E0
#define mmISP_IMC_FW_INBUF_FRM_FIFO_FILL_LV             0x675E4
#define mmISP_IMC_FW_INBUF_PREPOST_FIFO_FILL_LV         0x675E8
#define mmISP_IMC_FW_INBUF_DBG_RDMA0_SLC_STATUS         0x67600
#define mmISP_IMC_FW_INBUF_DBG_RDMA1_SLC_STATUS         0x67604
#define mmISP_IMC_FW_INBUF_DBG_RDMA2_SLC_STATUS         0x67608
#define mmISP_IMC_CVP_OUTBUF_SLC_ACK                    0x67800
#define mmISP_IMC_CVP_OUTBUF_CAM0_STRM0_REL             0x67804
#define mmISP_IMC_CVP_OUTBUF_CAM0_STRM1_REL             0x67808
#define mmISP_IMC_CVP_OUTBUF_CAM0_STRM2_REL             0x6780C
#define mmISP_IMC_CVP_OUTBUF_CAM0_STRM3_REL             0x67810
#define mmISP_IMC_CVP_OUTBUF_CAM0_IR_REL                0x67814
#define mmISP_IMC_CVP_OUTBUF_CAM0_IRSTAT_REL            0x67818
#define mmISP_IMC_CVP_OUTBUF_CAM0_RGBSTAT_REL           0x6781C
#define mmISP_IMC_CVP_OUTBUF_CAM1_STRM0_REL             0x67820
#define mmISP_IMC_CVP_OUTBUF_CAM1_STRM1_REL             0x67824
#define mmISP_IMC_CVP_OUTBUF_CAM1_STRM2_REL             0x67828
#define mmISP_IMC_CVP_OUTBUF_CAM1_STRM3_REL             0x6782C
#define mmISP_IMC_CVP_OUTBUF_CAM1_IR_REL                0x67830
#define mmISP_IMC_CVP_OUTBUF_CAM1_IRSTAT_REL            0x67834
#define mmISP_IMC_CVP_OUTBUF_CAM1_RGBSTAT_REL           0x67838
#define mmISP_IMC_FW_OUTBUF_CTRL                        0x67900
#define mmISP_IMC_FW_OUTBUF_CAM0_STRM0_BASE_ADDR        0x67904
#define mmISP_IMC_FW_OUTBUF_CAM0_STRM1_BASE_ADDR        0x67908
#define mmISP_IMC_FW_OUTBUF_CAM0_STRM2_BASE_ADDR        0x67910
#define mmISP_IMC_FW_OUTBUF_CAM0_STRM3_BASE_ADDR        0x67914
#define mmISP_IMC_FW_OUTBUF_CAM0_IR_BASE_ADDR           0x67918
#define mmISP_IMC_FW_OUTBUF_CAM1_STRM0_BASE_ADDR        0x6791C
#define mmISP_IMC_FW_OUTBUF_CAM1_STRM1_BASE_ADDR        0x67920
#define mmISP_IMC_FW_OUTBUF_CAM1_STRM2_BASE_ADDR        0x67924
#define mmISP_IMC_FW_OUTBUF_CAM1_STRM3_BASE_ADDR        0x67928
#define mmISP_IMC_FW_OUTBUF_CAM1_IR_BASE_ADDR           0x6792C
#define mmISP_IMC_FW_OUTBUF_CAM0_ACK_INTR_MAP           0x67930
#define mmISP_IMC_FW_OUTBUF_CAM1_ACK_INTR_MAP           0x67934
#define mmISP_IMC_FW_OUTBUF_FRM_BROKEN                  0x67938
#define mmISP_IMC_FW_OUTBUF_TRIG                        0x6793C
#define mmISP_IMC_FW_OUTBUF_CAM0_REL                    0x67980
#define mmISP_IMC_FW_OUTBUF_CAM1_REL                    0x67984
#define mmISP_IMC_FW_OUTBUF_OVFL_MIS                    0x67990
#define mmISP_IMC_FW_OUTBUF_OVFL_RIS                    0x67994
#define mmISP_IMC_FW_OUTBUF_OVFL_MASK                   0x67998
#define mmISP_IMC_FW_OUTBUF_OVFL_CLR                    0x6799C
#define mmISP_IMC_FW_OUTBUF_OVFL_SET                    0x679A0
#define mmISP_IMC_FW_OUTBUF_CVP_ACK_MIS                 0x679B0
#define mmISP_IMC_FW_OUTBUF_CVP_ACK_RIS                 0x679B4
#define mmISP_IMC_FW_OUTBUF_CVP_ACK_MASK                0x679B8
#define mmISP_IMC_FW_OUTBUF_CVP_ACK_CLR                 0x679BC
#define mmISP_IMC_FW_OUTBUF_CVP_ACK_SET                 0x679C0
#define mmISP_IMC_FW_OUTBUF_MIS                         0x679C4
#define mmISP_IMC_FW_OUTBUF_RIS                         0x679C8
#define mmISP_IMC_FW_OUTBUF_MSK                         0x679CC
#define mmISP_IMC_FW_OUTBUF_CLR                         0x679D0
#define mmISP_IMC_FW_OUTBUF_ISR                         0x679D4
#define mmISP_IMC_FW_OUTBUF_CAM0_STM0_FRM_DONE          0x679D8
#define mmISP_IMC_FW_OUTBUF_CAM0_STM1_FRM_DONE          0x679DC
#define mmISP_IMC_FW_OUTBUF_CAM0_STM2_FRM_DONE          0x679E0
#define mmISP_IMC_FW_OUTBUF_CAM0_STM3_FRM_DONE          0x679E4
#define mmISP_IMC_FW_OUTBUF_CAM0_IR_FRM_DONE            0x679E8
#define mmISP_IMC_FW_OUTBUF_CAM1_STM0_FRM_DONE          0x679EC
#define mmISP_IMC_FW_OUTBUF_CAM1_STM1_FRM_DONE          0x679F0
#define mmISP_IMC_FW_OUTBUF_CAM1_STM2_FRM_DONE          0x679F4
#define mmISP_IMC_FW_OUTBUF_CAM1_STM3_FRM_DONE          0x679F8
#define mmISP_IMC_FW_OUTBUF_CAM1_IR_FRM_DONE            0x679FC
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM0_IN            0x67A00
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM0_IN            0x67A04
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM0_OUT           0x67A08
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM0_OUT           0x67A0C
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM1_IN            0x67A10
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM1_IN            0x67A14
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM1_OUT           0x67A18
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM1_OUT           0x67A1C
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM2_IN            0x67A20
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM2_IN            0x67A24
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM2_OUT           0x67A28
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM2_OUT           0x67A2C
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM3_IN            0x67A30
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM3_IN            0x67A34
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STM3_OUT           0x67A38
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STM3_OUT           0x67A3C
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STMIR_IN           0x67A40
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STMIR_IN           0x67A44
#define mmISP_IMC_FW_OUTBUF_DBG_CAM0_STMIR_OUT          0x67A48
#define mmISP_IMC_FW_OUTBUF_DBG_CAM1_STMIR_OUT          0x67A4C

#define mmISP_STATUS                                    0x61000
#define mmISP_CCPU_SCRATCHA                             0x61428
#define mmISP_CCPU_SCRATCHB                             0x6142C

/******************************************************************************/
/* definition for CVP_ISP registers */
#define CVP_IMC_BASE			mmISP_IMC_FW_EN
#define CVP_ISP_OFFSET(x)		((x) - CVP_IMC_BASE)

#define CVP_INT_CTRL_INFO \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INTR_CTRL_INFO)
#define CVP_INT_BUF_RELEASE_ACK \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_REL_ACK)
#define CVP_INT_BUF_NOTIFY_ACK \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_SLC_ACK)

#define CAM0_CVP_CTRL_INFO_ACK_ID \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_ACK_ID)
#define CAM0_CVP_CTRL_INFO_RESP_ID \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_RESP_ID)
#define CAM0_CVP_CTRL_INFO_RESP \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_RESP_STATUS)
#define CAM0_CVP_CTRL_INFO_STRM \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_STRM)
#define CAM0_CVP_CTRL_INFO_RES \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_RES)
#define CAM0_CVP_CTRL_INFO_FPS \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_FPS)
#define CAM0_CVP_CTRL_INFO_AGAIN0 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_AG0)
#define CAM0_CVP_CTRL_INFO_DGAIN0 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_DG0)
#define CAM0_CVP_CTRL_INFO_ITIME0 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_ITIME0)
#define CAM0_CVP_CTRL_INFO_FRAME_DURATION \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_AG1)
#define CAM0_CVP_CTRL_INFO_AF_PARAMETER0 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_DG1)
#define CAM0_CVP_CTRL_INFO_AF_PARAMETER1 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_ITIME1)
#define CAM0_CVP_CTRL_INFO_AF_PARAMETER2 \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_AG2)
#define CAM0_CVP_CTRL_INFO_OUT_SLICE_SYNC \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_DG2)
#define CAM0_CVP_CTRL_INFO_OUT_SLICE_PARAMETER \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_CAM0_INFO_ITIME2)

#define CAM0_CVP_INBUF_EXP0_FRM_SLICE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_EXP0_FRM_SLC_ID)
#define CAM0_CVP_INBUF_EXP0_ADDR_OFFSET \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_ADDR_OFFSET)
#define CAM0_CVP_INBUF_IMG_SIZE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_FRMSZ_BITDP)
#define CAM0_CVP_INBUF_SLICE_INFO \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_EXP12_GAP)

#define CAM0_CVP_PREDAT_FRM_ID \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_PRE_DATA_FRM_ID)
#define CAM0_CVP_PREDAT_ADDR_OFFSET \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_PRE_DATA_ADDR_OFFSET)
#define CAM0_CVP_POSTDAT_FRM_ID \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_POST_DATA_FRM_ID)
#define CAM0_CVP_POSTDAT_ADDR_OFFSET \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_POST_DATA_ADDR_OFFSET)

#define CAM0_CVP_YUV_STRM0_OUTPUT_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_STRM0_REL)
#define CAM0_CVP_YUV_STRM1_OUTPUT_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_STRM1_REL)
#define CAM0_CVP_YUV_STRM2_OUTPUT_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_STRM2_REL)
#define CAM0_CVP_YUV_STRM3_OUTPUT_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_STRM3_REL)
#define CAM0_CVP_IR_OUTPUT_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_IR_REL)
#define CAM0_CVP_IR_STATBUF_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_IRSTAT_REL)
#define CAM0_CVP_RGB_STATBUF_RELEASE \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_OUTBUF_CAM0_RGBSTAT_REL)

#define CAM0_CVP_INBUF_DROP_ID \
	CVP_ISP_OFFSET(mmISP_IMC_CVP_INBUF_CAM0_SET_DROP_ID)

#define CAM_CTRL_INFO_REG_SIZE \
	(mmISP_IMC_CVP_CAM1_ACK_ID - mmISP_IMC_CVP_CAM0_ACK_ID)
#define CAM_INBUFF_PIXEL_REG_SIZE \
	(mmISP_IMC_CVP_INBUF_CAM1_ADDR_OFFSET - \
	mmISP_IMC_CVP_INBUF_CAM0_ADDR_OFFSET)
#define CAM_INBUFF_DATA_REG_SIZE \
	(mmISP_IMC_CVP_INBUF_CAM1_PRE_DATA_ADDR_OFFSET - \
	mmISP_IMC_CVP_INBUF_CAM0_PRE_DATA_ADDR_OFFSET)
#define CAM_OUTBUFF_REG_SIZE \
	(mmISP_IMC_CVP_OUTBUF_CAM1_STRM0_REL - \
	mmISP_IMC_CVP_OUTBUF_CAM0_STRM0_REL)
#define CAM_INBUF_DROP_ID_REG_SIZE \
	(mmISP_IMC_CVP_INBUF_CAM1_SET_DROP_ID - \
	 mmISP_IMC_CVP_INBUF_CAM0_SET_DROP_ID)

#define ISP_OUTPUT_BUFFER_RELEASE_SIZE	4
#define ISP_STATS_BUFFER_RELEASE_SIZE	4

#define ISP_CCPU_SCRATCHA (mmISP_CCPU_SCRATCHA - mmISP_STATUS)
#define ISP_CCPU_SCRATCHB (mmISP_CCPU_SCRATCHB - mmISP_STATUS)

#define CVIP_ISP_EVENT_LOG_SIZE	2048

#define ISP_SIMNOW_MODE		0
#define ISP_HW_MODE		1

enum isp_camera {
	CAM0		= 0,
	CAM1		= 1,
	NUM_OF_CAMERA	= 2,
};

enum isp_stats_type {
	IRSTATS		= 0,
	RGBSTATS	= 1,
	NUM_OF_STATS	= 2,
};

enum isp_stream_type {
	STREAM0		= 0,
	STREAM1		= 1,
	STREAM2		= 2,
	STREAM3		= 3,
	STREAMIR	= 4,
	NUM_OF_STREAM	= 5,
};

enum isp_message_type {
	INPUT_BUFF_RELEASE	= 0,
	PRE_BUFF_RELEASE	= 1,
	POST_BUFF_RELEASE	= 2,
	WDMA_STATUS		= 3,
	NUM_OF_MESSAGE		= 4,
};

/******************************************************************************/
/* register payload definition : CAM Control and Information Payload */
enum reg_req {
	REQ_NOOP		= 0,
	REQ_OP			= 1,
};

enum reg_req_rw {
	REQ_READ		= 0,
	REQ_WRITE		= 1,
};

enum reg_resp {
	RESP_NO			= 0,
	RESP_YES		= 1,
};

enum reg_stream_status {
	STREAM_UNTOUCHED	= 0,
	STREAM_OFF		= 1,
	STREAM_STDBY		= 2,
	STREAM_ON		= 3,
	STREAM_RESERVED		= 7,
};

enum reg_resolution {
	RES_UNTOUCHED		= 0,
	RES_FULL		= 1,
	RES_V2_BINNING		= 2,
	RES_H2V2_BINNING	= 3,
	RES_H4V4_BINNING	= 4,
	RES_4K2K		= 5,
	RES_RESERVED		= 7,
};

enum reg_hdr {
	HDR_UNTOUCHED		= 0,
	HDR_NO			= 1,
	HDR_ZIGZAG		= 2,
	HDR_2FRAME		= 3,
	HDR_3FRAME		= 4,
	HDR_RESERVED		= 7,
};

enum reg_test_pattern {
	TP_UNTOUCHED		= 0,
	TP_NO			= 1,
	TP_1			= 2,
	TP_2			= 3,
	TP_3			= 4,
	TP_RESERVED		= 7,
};

enum reg_bit_depth {
	BD_UNTOUCHED		= 0,
	BD_8BIT			= 1,
	BD_10BIT		= 2,
	BD_12BIT		= 3,
	BD_RESERVED		= 7,
};

enum reg_transition_type {
	TRAN_UNTOUCHED		= 0,
	TRAN_INPUT		= 1,
	TRAN_OUTPUT		= 2,
	TRAN_BOTH		= 3,
	TRAN_SLICE_INFO		= 4,
	TRAN_RESERVED		= 15,
};

struct reg_cam_ctrl_info {
	union {
		u32 value;
		struct {
			u32 pl_num	:8;
			u32:24;
		};
	} pl_num;
	u32	req_id;
	union {
		u32 value;
		struct {
			u32 stream_op		:1;
			u32 stream_rw		:1;
			u32 res_op		:1;
			u32 res_rw		:1;
			u32 fpks_op		:1;
			u32 fpks_rw		:1;
			u32 again0_op		:1;
			u32 again0_rw		:1;
			u32 dgain0_op		:1;
			u32 dgain0_rw		:1;
			u32 itime0_op		:1;
			u32 itime0_rw		:1;
			u32 info_sync_op	:1;
			u32 info_sync_rw	:1;
			u32:2;
			u32 af_lens_rng_op	:1;
			u32 af_lens_rng_rw	:1;
			u32 lens_pos_op		:1;
			u32 lens_pos_rw		:1;
			u32 af_mode_op		:1;
			u32 af_mode_rw		:1;
			u32 frame_duration_op	:1;
			u32 frame_duration_rw	:1;
			u32 prepost_mem_op	:1;
			u32 prepost_mem_rw	:1;
			u32 stats_mem_op	:1;
			u32 stats_mem_rw	:1;
			u32 af_trig_op		:1;
			u32 af_trig_rw		:1;
			u32:1;
			u32 response		:1;
		};
	} req;
	union {
		u32 value;
		struct {
			u32 stream	:3;
			u32 res		:3;
			u32 hdr		:3;
			u32 tp		:3;
			u32 bd		:3;
			u32:17;
		};
	} strm;
	union {
		u32 value;
		struct {
			u32 width	:16;
			u32 height	:16;
		};
	} res;
	u32	fpks; /* frame per kilo seconds */
	u32	again0;
	u32	dgain0;
	u32	itime0;
	u32	frame_duration;
	union {
		u32 value;
		struct {
			u32 lens_pos	:24;
			u32:2;
			u32 af_trigger	:2;
			u32 af_mode	:4;
		};
	} af_p0;
	union {
		u32 value;
		struct {
			u32 far		:24;
			u32:8;
		};
	} af_p1;
	union {
		u32 value;
		struct {
			u32 near	:24;
			u32:8;
		};
	} af_p2;
	union {
		u32 value;
		struct {
			u32 frame	:28;
			u32 tran	:4;
		};
	} oslice_sync;
	union {
		u32 value;
		struct {
			u32 height_s0	:12;
			u32 height_s	:12;
			u32:3;
			u32 slice_num	:5;	/* [1, 16] */
		};
	} oslice_param;
	u32	prepost_hi;
	u32	prepost_lo;
	u32	prepost_size;
	u32	stats_hi;
	u32	stats_lo;
	u32	stats_size;
};

/******************************************************************************/
#define REG_IMC_CAM_SHIFT	16

/* register payload definition : interrupt IMC_CTRL */
enum reg_imc_ctrl_type {
	REG_IMC_CTRL_RESP	= 0,
	REG_IMC_CTRL_REQ_ACK	= 1,
	REG_IMC_CTRL_RESET	= 2,
	REG_IMC_CTRL_RECOVER	= 3,
	REG_IMC_CTRL_WDMA	= 4,
};

/******************************************************************************/
/* register payload definition : interrupt IMC_INBUF */
enum reg_imc_inbuf_type {
	REG_IMC_INBUF_REL_ACK	= 0,
	REG_IMC_PREBUF_REL_ACK	= 1,
	REG_IMC_POSTBUF_REL_ACK	= 2,
};

/******************************************************************************/
/* register payload definition : interrupt IMC_OUTBUF */
enum reg_imc_outbuf_type {
	REG_IMC_STATS_IR	= 0,
	REG_IMC_STATS_RGB	= 1,
	REG_IMC_OUTBUF_STRM0	= 2,
	REG_IMC_OUTBUF_STRM1	= 3,
	REG_IMC_OUTBUF_STRM2	= 4,
	REG_IMC_OUTBUF_STRM3	= 5,
	REG_IMC_OUTBUF_IR	= 6,
};

/******************************************************************************/
/* register payload definition : CAM information Response Payload */
enum reg_resp_type {
	RESP_SUCCEED		= 0,
	RESP_UNSUPPORT		= 1,
	RESP_UNSUPPORT_RES	= 1,
	RESP_UNSUPPORT_HDR	= 2,
	RESP_UNSUPPORT_TP	= 3,
	RESP_UNSUPPORT_WIDTH	= 1,
	RESP_UNSUPPORT_HEIGHT	= 2,
};

enum reg_stream_status_resp {
	STREAM_OFF_RESP		= 0,
	STREAM_STDBY_RESP	= 1,
	STREAM_ON_RESP		= 2,
	STREAM_RESERVED_RESP	= 7,
};

static inline
enum reg_stream_status_resp to_stream_resp(enum reg_stream_status req)
{
	switch (req) {
	case STREAM_OFF:	return STREAM_OFF_RESP;
	case STREAM_STDBY:	return STREAM_STDBY_RESP;
	case STREAM_ON:		return STREAM_ON_RESP;
	default:		return STREAM_RESERVED_RESP;
	}
}

enum reg_resolution_resp {
	RES_FULL_RESP		= 0,
	RES_V2_BINNING_RESP	= 1,
	RES_H2V2_BINNING_RESP	= 2,
	RES_H4V4_BINNING_RESP	= 3,
	RES_4K2K_RESP		= 4,
	RES_RESERVED_RESP	= 7,
};

static inline
enum reg_resolution_resp to_res_resp(enum reg_resolution req)
{
	switch (req) {
	case RES_FULL:		return RES_FULL_RESP;
	case RES_V2_BINNING:	return RES_V2_BINNING_RESP;
	case RES_H2V2_BINNING:	return RES_H2V2_BINNING_RESP;
	case RES_H4V4_BINNING:	return RES_H4V4_BINNING_RESP;
	case RES_4K2K:		return RES_4K2K_RESP;
	default:		return RES_RESERVED_RESP;
	}
}

enum reg_hdr_resp {
	HDR_NO_RESP		= 0,
	HDR_ZIGZAG_RESP		= 1,
	HDR_2FRAME_RESP		= 2,
	HDR_3FRAME_RESP		= 3,
	HDR_RESERVED_RESP	= 7,
};

static inline
enum reg_hdr_resp to_hdr_resp(enum reg_hdr req)
{
	switch (req) {
	case HDR_NO:		return HDR_NO_RESP;
	case HDR_ZIGZAG:	return HDR_ZIGZAG_RESP;
	case HDR_2FRAME:	return HDR_2FRAME_RESP;
	case HDR_3FRAME:	return HDR_3FRAME_RESP;
	default:		return HDR_RESERVED_RESP;
	}
}

enum reg_test_pattern_resp {
	TP_NO_RESP		= 0,
	TP_1_RESP		= 1,
	TP_2_RESP		= 2,
	TP_3_RESP		= 3,
	TP_RESERVED_RESP	= 7,
};

static inline
enum reg_test_pattern_resp to_tp_resp(enum reg_test_pattern req)
{
	switch (req) {
	case TP_NO:	return TP_NO_RESP;
	case TP_1:	return TP_1_RESP;
	case TP_2:	return TP_2_RESP;
	case TP_3:	return TP_3_RESP;
	default:	return TP_RESERVED_RESP;
	}
}

enum reg_bit_depth_resp {
	BD_8BIT_RESP		= 0,
	BD_10BIT_RESP		= 1,
	BD_12BIT_RESP		= 2,
	BD_RESERVED_RESP	= 7,
};

static inline
enum reg_bit_depth_resp to_bd_resp(enum reg_bit_depth req)
{
	switch (req) {
	case BD_8BIT:	return BD_8BIT_RESP;
	case BD_10BIT:	return BD_10BIT_RESP;
	case BD_12BIT:	return BD_12BIT_RESP;
	default:	return BD_RESERVED_RESP;
	}
}

/* enum reg_lens_mode_resp == enum reg_lens_mode */
#define to_lens_resp(x) (x)

/* enum reg_transition_type_resp == enum reg_transition_type */
#define to_tran_resp(x) (x)

struct reg_cam_ctrl_resp {
	u32	resp_id;
	union {
		u32 value;
		struct {
			u32 stream		:2;
			u32 res			:2;
			u32 fpks		:2; /* frame per kilo seconds */
			u32 again0		:2;
			u32 dgain0		:2;
			u32 itime0		:2;
			u32 info_sync		:2;
			u32:2;
			u32 af_lens_rng		:2;
			u32 lens_pos		:2;
			u32 af_mode		:2;
			u32 frame_duration	:2;
			u32 prepost_mem		:2;
			u32 stats_mem		:2;
			u32 af_trig		:2;
			u32:2;
		};
	} resp;
	union {
		u32 value;
		struct {
			u32 stream	:3;
			u32 res		:3;
			u32 hdr		:3;
			u32 tp		:3;
			u32 bd		:3;
			u32:17;
		};
	} strm;
	union {
		u32 value;
		struct {
			u32 width	:16;
			u32 height	:16;
		};
	} res;
	u32	fpks; /* frame per kilo seconds */
	u32	again0;
	u32	dgain0;
	u32	itime0;
	u32	frame_duration;
	union {
		u32 value;
		struct {
			u32 lens_pos	:24;
			u32:2;
			u32 af_trigger	:2;
			u32 af_mode	:4;
		};
	} af_p0;
	union {
		u32 value;
		struct {
			u32 far		:24;
			u32:8;
		};
	} af_p1;
	union {
		u32 value;
		struct {
			u32 near	:24;
			u32:8;
		};
	} af_p2;
	union {
		u32 value;
		struct {
			u32 frame	:28;
			u32 tran	:4;
		};
	} oslice_sync;
	union {
		u32 value;
		struct {
			u32 height_s0	:12;
			u32 height_s	:12;
			u32:3;
			u32 slice_num	:5;	/* [1, 16] */
		};
	} oslice_param;
};

/******************************************************************************/
/* register payload definition : raw CAM buffer : ISP_IMC_CVP_INBUF */
enum reg_bit_width {
	BW_8BIT		= 0,
	BW_10BIT	= 1,
	BW_12BIT	= 2,
	BW_14BIT	= 3,
	BW_16BIT	= 4,
	BW_RESERVED	= 0xf,
};

static inline
enum reg_bit_width to_bit_width(enum reg_bit_depth req)
{
	switch (req) {
	case BD_8BIT:	return BW_8BIT;
	case BD_10BIT:	return BW_10BIT;
	case BD_12BIT:	return BW_12BIT;
	default:	return BW_RESERVED;
	}
}

/******************************************************************************/
/* register payload definition : prepost release, inbuf release, wdma report */
enum reg_frame_status {
	FRAME_PROCESSED		= 0,
	FRAME_DROPPED		= 1,
};

union frame_info {
	u32 value;
	struct {
		u32 frame	:28;
		u32 field	:4;
	};
};

struct reg_prepost_release {
	union frame_info	id;
};

struct reg_inbuf_release {
	union frame_info	id;
};

/******************************************************************************/
/* register payload definition : wdma report */
enum reg_wdma_status {
	WDMA_DISABLED		= 0,
	WDMA_ENABLED		= 1,
};

enum reg_cwr_vid {
	CWR_VID_DEF		= 0,
	CWR_VID_CV_CV		= 1,
	CWR_VID_X86_CV0		= 2,
	CWR_VID_X86_CV		= 3,
	CWR_VID_RESERVED	= 7,
};

enum reg_swr_vid {
	SWR_VID_DEF		= 0,
	SWR_VID_X86_IR		= 1,
	SWR_VID_X86_RAW		= 2,
	SWR_VID_RESERVED	= 7,
};

struct reg_wdma_report {
	union {
		u32 value;
		struct {
			u32 cwr_dma0_status	:1;
			u32 cwr_dma1_status	:1;
			u32 cwr_dma2_status	:1;
			u32 cwr_dma3_status	:1;
			u32 swr_dma_status	:1;
			u32 cwr_dma0_vid	:3;
			u32 cwr_dma1_vid	:3;
			u32 cwr_dma2_vid	:3;
			u32 cwr_dma3_vid	:3;
			u32 swr_dma_vid		:3;
			u32:12;
		};
	};
};

/******************************************************************************/
/* register payload definition : output buffer, stats buffer */
struct reg_outbuf {
	union frame_info id;
	union {
		struct {
			u32	y_addr;
			u32	u_addr;
			u32	v_addr;
		};
		u32 ir_addr;
	};
};

struct reg_stats_buf {
	union frame_info id;
	u32 addr;
};

/******************************************************************************/

/**
 * struct cam_ctrl_node - camera control requests
 * @data:		camera control request
 * @camera_id:		requested camera
 */
struct cam_ctrl_node {
	struct reg_cam_ctrl_info data;
	enum isp_camera camera_id;
};

/**
 * struct out_buff_node - output buffer requests
 * @data:		output buffer request
 * @camera_id:		camera id
 * @stream:		stream id
 */
struct out_buff_node {
	struct reg_outbuf data;
	enum isp_camera camera_id;
	enum isp_stream_type stream;
};

/**
 * struct stats_buff_node - stats buffer requests
 * @data:		stats buffer request
 * @camera_id:		camera id
 * @type:		stats buffer type
 */
struct stats_buff_node {
	struct reg_stats_buf data;
	enum isp_camera camera_id;
	enum isp_stats_type type;
};

struct isp_msg_node {
	union {
		struct reg_inbuf_release inbuf;
		struct reg_prepost_release prepost;
		struct reg_wdma_report wdma;
	};
	enum isp_camera camera_id;
	enum isp_message_type type;
};

/* isp test configuration */
#define TEST_TOUT		(5 * HZ)
#define MAX_INPUT_FRAME		6
#define MAX_OUTPUT_FRAME	1
#define FRAMEID_INVALID		-1
#define FRAMEID_MAX		((1 << 28) - 1)
#define CAM0_INPUT_SIZE		(150 * 1024 * 1024)
#define CAM0_OUTPUT_SIZE	(144 * 1024 * 1024)
#define PREDATA_SIZE		(sizeof(struct predata_t))
#define POSTDATA_SIZE		(sizeof(struct postdata_t))
#define AFROI_SIZE		(sizeof(struct cvipafroi_t))
/* isp fw define (struct _AaaStatData_t) for stats data,
 * size is 114176 (0x1BE00) bytes.
 */
#define STATS_SIZE		0x1BE00
#define ISP_STRIDE_ALIGN_SIZE	256
#define ISP_MAX_SCALER_DEPTH		32
#define ISP_MAX_HW_PIPELINE_DEPTH	56
#define ISP_HW_PIPELINE_MAX_DELAY_LINE \
	(ISP_MAX_HW_PIPELINE_DEPTH + ISP_MAX_SCALER_DEPTH + 2)

/******************************************************************************/
/* predata definitions , from isp fw AIDT */

/**
 * @typedef enum CvipLensState_t CvipLensState_t
 */
/**
 * @enum  CvipLensState_t
 * @brief  The CvipLensState
 */
enum cviplensstate_t {
	/**< enum value: CVIP_LENS_STATE_INVALID*/
	CVIP_LENS_STATE_INVALID      = 0,
	/**< enum value: CVIP_LENS_STATE_SEARCHING*/
	CVIP_LENS_STATE_SEARCHING    = 1,
	/**< enum value: CVIP_LENS_STATE_CONVERGED*/
	CVIP_LENS_STATE_CONVERGED    = 2,
	CVIP_LENS_STATE_MAX,
};

/**
 * @typedef struct _CvipExpData_t _CvipExpData_t
 */
/**
 * @struct _CvipExpData_t
 * @brief  The _CvipExpData_t
 */
struct cvipexpdata_t {
	/**< The real itime setting for sensor, unit is exposure line*/
	u32 itime;
	/**< The real again setting for sensor, register-related value
	 * For (ex. reg 0x204/205 of IMX577), it is the value of
	 *   sensor register ANA_GAIN_GLOBAL
	 * For (ex. reg 0x03508/3509 of OV13858), it is the value of
	 *   sensor register LONG_GAIN
	 * for (ex. reg 0x0204/205 of S573L6XX), it is the value of
	 *   sensor register analogue_gain_code_global
	 */
	u32 again;
	/**< Reserved because we have not idea how convert gain value into real
	 * register setting yet. Values will be 1000 always
	 */
	u32 dgain;
};

struct cviptimestampinpre_t {
	u64 readoutstarttimestampus; /**< readout start timestamp in us */
	u64 centroidtimestampus;     /**< centroid timestamp in us */
	u64 seqWintimestampus;       /**< seq win timestamp in us */
};

#define CVIP_AF_ROI_MAX_NUMBER 3

struct cvipafroiwindow_t {
	u32 xmin;
	u32 ymin;
	u32 xmax;
	u32 ymax;
	u32 weight;
};

struct cvipafroi_t {
	u32 numRoi;
	struct cvipafroiwindow_t roi_af[CVIP_AF_ROI_MAX_NUMBER];
};

#define CVIP_AF_CONTROL_MODE_MASK_OFF 0x0
#define CVIP_AF_CONTROL_MODE_MASK_AUTO 0x1
#define CVIP_AF_CONTROL_MODE_MASK_MACRO 0x2
#define CVIP_AF_CONTROL_MODE_MASK_CONT_VIDEO 0x4
#define CVIP_AF_CONTROL_MODE_MASK_CONT_PICTURE 0x8

enum cvipafmode_t {
	CVIP_AF_CONTROL_MODE_OFF,
	CVIP_AF_CONTROL_MODE_AUTO,
	CVIP_AF_CONTROL_MODE_MACRO,
	CVIP_AF_CONTROL_MODE_CONTINUOUS_VIDEO,
	CVIP_AF_CONTROL_MODE_CONTINUOUS_PICTURE,
};

enum cvipaftrigger_t {
	CVIP_AF_CONTROL_TRIGGER_IDLE,
	CVIP_AF_CONTROL_TRIGGER_START,
	CVIP_AF_CONTROL_TRIGGER_CANCEL,
};

enum cvipafstate_t {
	CVIP_AF_STATE_INACTIVE,
	CVIP_AF_STATE_PASSIVE_SCAN,
	CVIP_AF_STATE_PASSIVE_FOCUSED,
	CVIP_AF_STATE_ACTIVE_SCAN,
	CVIP_AF_STATE_FOCUSED_LOCKED,
	CVIP_AF_STATE_NOT_FOCUSED_LOCKED,
	CVIP_AF_STATE_PASSIVE_UNFOCUSED,
};

enum cvipafscenechange_t {
	CVIP_AF_SCENE_CHANGE_NOT_DETECTED,
	CVIP_AF_SCENE_CHANGE_DETECTED,
};

struct cvipmetaaf_t {
	//Af
	enum cvipafmode_t        mode;
	enum cvipaftrigger_t     trigger;
	enum cvipafstate_t       state;
	enum cvipafscenechange_t scene_change;
	struct cvipafroi_t       cvipafroi;
	u32                      afframeid;

	//lens
	enum cviplensstate_t lensState;
	float                distance;
	float                distancenear;
	float                distancefar;
	float                focusrangenear;
	float                focusrangefar;
};

/**
 * @typedef struct _CvipTimestampInPre_t CvipTimestampInPre_t
 */
/**
 * @struct _CvipTimestampInPre_t
 * @brief  The CvipTimestampInPre
 */
struct predata_t {
	struct cvipexpdata_t         expdata;
	struct cviptimestampinpre_t  timestampinpre;
	struct cvipmetaaf_t          afmetadata;
};

/**
 * @typedef struct _CvipTimestampInPost_t CvipTimestampInPost_t
 */
/**
 * @struct _CvipTimestampInPost_t
 * @brief  The CvipTimestampInPost
 */
struct cviptimestampinpost_t {
	u64 readoutendtimestampus;   /**< readout end timestamp in Us */
};

/**
 * @typedef struct _PostData PostData_t
 */
/**
 * @struct _PostData_t
 * @brief  The PostData
 */
struct postdata_t {
	/**< Timestamp in post data */
	struct cviptimestampinpost_t  timestampinpost;
};

/******************************************************************************/

#define INSLICE0_SHIFT		0
#define INSLICE0_MASK		0xfff
#define INSLICEMID_SHIFT	12
#define INSLICEMID_MASK		0xfff000

/**
 * struct isp_input_frame - isp input frame information
 * @width:			frame width
 * @height:			frame height
 * @bw:				frame bitwidth
 * @exp0_offset:		exp0 offset
 * @slice_info:			raw slice info: height, number
 * @frame_id:			frame id
 * @cur_slice:			slice consumed by ISP
 * @total_slices:		slice per frame
 * @exp12data:			flag for exp12 offset
 * @predata_offset:		predata offset
 * @postdata_offset:		postdata offset
 */
struct isp_input_frame {
	u32 width;
	u32 height;
	u8 bw;
	u32 exp0_offset;
	u32 slice_info;
	u32 frame_id;
	u8 cur_slice;
	u8 total_slices;
	u32 predata_offset;
	u32 postdata_offset;
};

enum _imageformat_t {
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

enum _rawpktfmt_t {
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

#define TYPE_ANDROID	0x76895321
#define TYPE_UBUNTU	0x12359867
#define TYPE_NONE	0

struct isp_drv_to_cvip_ctl {
	// type is TYPE_ANDROID or others
	u32 type;
	enum _rawpktfmt_t raw_fmt;
	u32 raw_slice_num;    /* [1, 16] */
	u32 output_slice_num; /* [1, 16] unused */
	u32 output_width;
	u32 output_height;
	enum _imageformat_t output_fmt;
};

/*
 * each frame consists of N slices, which can be divided into 3 groups:
 * SLICE_FIRST refers to slice index 0,
 * SLICE_MID refers to slice index [1, N-2],
 * SLICE_LAST refers to slice index N-1.
 * slice might have different height between groups
 */
enum slice_idx_type {
	SLICE_FIRST,
	SLICE_MID,
	SLICE_LAST,
	SLICE_MAX
};

/**
 * struct isp_output_frame - isp output frame information
 * @width:			frame width
 * @height:			frame height
 * @pixel_size:			frame pixel size in bytes
 * @y_slice_size:		slice size for Luma
 * @uv_slice_size:		slice size for chruma
 * @ir_slice_size:		slice size for IR
 * @data_addr:			pixel output offset
 * @frame_id:			frame id
 * @cur_slice:			slice produced by ISP
 * @total_slices:		slice per frame
 * @stat_addr:			stats offset
 * @stat_size:			stats size
 * @stat_type:			type of stats data
 */
struct isp_output_frame {
	u32 width;
	u32 height;
	u32 y_slice_size[SLICE_MAX];
	u32 uv_slice_size[SLICE_MAX];
	u32 ir_slice_size[SLICE_MAX];
	u32 y_frame_size;
	u32 uv_frame_size;
	u32 ir_frame_size;
	u32 data_addr;
	u32 frame_id;
	u8 cur_slice;
	u8 total_slices;
	u32 stat_addr;
	u32 stat_size;
	enum isp_stats_type stat_type;
};

struct test_dqbuf_resp_t {
	int lock_frame_id;
	int lock_stats_id;
};

struct terr_base_t {
	int drop_frame;
	int recover_frame;
};

struct slice_sync_info {
	int slice_height[SLICE_MAX];
	int slice_num; /* [1, 16] */
};

/**
 * struct camera - isp camera
 * @input_base:			address ptr for FMR5 input pixel buffer
 * @output_base:		address ptr for FMR8 output buffer
 * @stats_base:			address ptr for stats buffer used by ISP
 * @data_base:			address ptr for Pre/Post buffer
 * @input_buf_offset:		physical address for input buffer
 * @output_buf_offset:		physical address for output buffer
 * @data_buf_offset:		physical address for Pre/Post buffer
 * @stats_buf_offset:		physical address for Stats buffer
 * @input_buf_size:		size of input buffer
 * @output_buf_size:		size of output buffer
 * @data_buf_size:		size of Pre/Post buffer
 * @stats_buf_size:		size of Stats buffer
 * @sensor_frame:		running idx for sensor output frame
 * @isp_input_frame:		running idx for isp processed input frame
 * @isp_predata_frame:		running idx for isp processed predata frame
 * @isp_postdata_frame:		running idx for isp processed postdata frame
 * @isp_output_frame:		running idx for isp processed output frame
 * @isp_stat_frame:		running idx for isp processed stat frame
 * @out_frame_base:		address ptr for copy frame buffer
 * @out_stats_base:		address ptr for copy stats buffer
 * @out_param_base:		address ptr for in/out parameters buffer
 * @dmabuf_frame:		dma-buf of frame data
 * @dmabuf_stats:		dma-buf of statistics data
 * @dmabuf_param:		dma-buf for passing in/out parameters
 * @test_condition:		test condition
 * @test_wq:			workqueue for waiting a testcase
 * @test_dqbuf_resp		store out parameter of dqbuf testcase
 * @input_frames:		input frame ring settings
 * @output_frames:		output frame ring settings
 * @isp_ioimg_setting:		input and output image configuration
 * @cam_ctrl_info:		camera control information
 * @cam_ctrl_resp:		camera control response
 * @isp_data_timer:		kernel timer for sensor output simulation
 * @streamon_work:		work for stream on task
 */
struct camera {
	/* for memory regions */
	u8 *input_base; /* isp input buffer */
	u8 *output_base; /* isp output buffer */
	u8 *stats_base; /* for rgb/ir stats data */
	u8 *data_base; /* for pre/post data */

	/* for user space access */
	phys_addr_t input_buf_offset;
	phys_addr_t output_buf_offset;
	phys_addr_t data_buf_offset;
	phys_addr_t stats_buf_offset;
	size_t input_buf_size;
	size_t output_buf_size;
	size_t data_buf_size;
	size_t stats_buf_size;
	/* running current frame number */
	u32 sensor_frame;
	/* running current frame number consumed by isp */
	u32 isp_input_frame;
	u32 isp_predata_frame;
	u32 isp_postdata_frame;
	u32 isp_output_frame;
	u32 isp_stat_frame;

	/* for copy-out memory regions */
	u8 *out_frame_base; /* for copy frame data */
	u8 *out_stats_base; /* for copy stats data */
	u8 *out_param_base; /* for in/out parameters */
	size_t out_frame_size;
	size_t out_stats_size;
	size_t out_param_size;
	struct dma_buf *dmabuf_frame;
	struct dma_buf *dmabuf_stats;
	struct dma_buf *dmabuf_param;

	int isp_test_id;
	unsigned long test_condition;
	wait_queue_head_t test_wq;
	struct test_dqbuf_resp_t test_dqbuf_resp;
	struct terr_base_t terr_base;

	/* input frame ring */
	struct isp_input_frame input_frames[MAX_INPUT_FRAME];
	/* output frame ring */
	struct isp_output_frame output_frames[MAX_OUTPUT_FRAME];
	struct slice_sync_info inslice;
	struct slice_sync_info outslice;
	struct isp_drv_to_cvip_ctl isp_ioimg_setting;
	struct reg_cam_ctrl_info cam_ctrl_info;
	struct reg_cam_ctrl_resp cam_ctrl_resp;
	/* prepost data */
	struct predata_t pre_data;
	struct postdata_t post_data;

	/* fake sensor */
	struct timer_list isp_data_timer;
	struct work_struct streamon_work;
	bool fake_sensor_initialised;
};

/**
 * struct isp_ipc_device - isp ipc device
 * @ipc:			ipc context
 * @stats_buffs:		stats buffers
 * @out_buffs:			output buffers
 * @cam_ctrls:			camera control buffers
 * @isp_msg_node:		inbuf_release, prepost_release, wdma_status
 * @test_config:		user defined test configuration
 * @base:			base address for CVP_ISP registers
 * @sc_base:			base address for CVP_SCATCH registers
 * @evlog_idx:			CVIP event log idx
 * @camera:			camera struct
 * @d_dir:			debug fs entry
 * @dev:			isp test device
 */
struct isp_ipc_device {
	struct ipc_context *ipc;

	/* for data buffer */
	struct stats_buff_node stats_buffs;

	/* for output yuv and ir buffer */
	struct out_buff_node out_buffs;

	/* for camera control */
	struct cam_ctrl_node cam_ctrls;

	/* for events: inbuf_rel,  prepost_rel, wdma_status */
	struct isp_msg_node msg_nodes[NUM_OF_MESSAGE];

	u32 test_config;

	void __iomem *base;
	void __iomem *sc_base;
	int evlog_idx;

	struct camera camera[NUM_OF_CAMERA];

#ifdef CONFIG_DEBUG_FS
	struct dentry *d_dir;
#endif
	struct device *dev;
};

/* isp message handlers */
void isp_cam0_cntl_cb(struct mbox_client *client, void *message);
void isp_cam1_cntl_cb(struct mbox_client *client, void *message);
void isp_cam0_rgb_cb(struct mbox_client *client, void *message);
void isp_cam0_ir_cb(struct mbox_client *client, void *message);
void isp_cam1_rgb_cb(struct mbox_client *client, void *message);
void isp_cam1_ir_cb(struct mbox_client *client, void *message);
void isp_cam0_isp_input_cb(struct mbox_client *client, void *message);
void isp_cam1_isp_input_cb(struct mbox_client *client, void *message);
void isp_cam0_pre_buff_cb(struct mbox_client *client, void *message);
void isp_cam1_pre_buff_cb(struct mbox_client *client, void *message);
void isp_cam0_post_buff_cb(struct mbox_client *client, void *message);
void isp_cam1_post_buff_cb(struct mbox_client *client, void *message);
void isp_cam0_stream0_cb(struct mbox_client *client, void *message);
void isp_cam0_stream1_cb(struct mbox_client *client, void *message);
void isp_cam0_stream2_cb(struct mbox_client *client, void *message);
void isp_cam0_stream3_cb(struct mbox_client *client, void *message);
void isp_cam0_streamir_cb(struct mbox_client *client, void *message);
void isp_cam1_stream0_cb(struct mbox_client *client, void *message);
void isp_cam1_stream1_cb(struct mbox_client *client, void *message);
void isp_cam1_stream2_cb(struct mbox_client *client, void *message);
void isp_cam1_stream3_cb(struct mbox_client *client, void *message);
void isp_cam1_streamir_cb(struct mbox_client *client, void *message);
void isp_cam0_dma_cb(struct mbox_client *client, void *message);
void isp_cam1_dma_cb(struct mbox_client *client, void *message);

/* raw golden image we have */
/* relative to firmware_class.path /usr/lib/ltp-testsuite/testcases/data/ */
struct isp_img {
	int width;
	int height;
	const char *img_path;
};

#ifdef CONFIG_CVIP_IPC
const char *isp_get_img_path(int width, int height);
int isp_load_image(struct isp_ipc_device *idev, int cam_id,
		   const char *img_path);
void isp_setup_output_rings(struct camera *icam);
void isp_reset_output_rings(struct camera *icam);
int calculate_input_slice_info(struct camera *icam);
int calculate_output_slice_info(struct camera *icam);
#else
static inline
const char *isp_get_img_path(int width, int height)
{
	return NULL;
}

static inline
int isp_load_image(struct isp_ipc_device *idev, int cam_id,
		   const char *img_path)
{
	return -1;
}

static inline
void isp_setup_output_rings(struct camera *icam)
{
}

static inline
void isp_reset_output_rings(struct camera *icam)
{
}

static inline
int calculate_input_slice_info(struct camera *icam)
{
	return -1;
}

static inline
int calculate_output_slice_info(struct camera *icam)
{
	return -1;
}
#endif

#endif /* __IPC_ISP_H__ */

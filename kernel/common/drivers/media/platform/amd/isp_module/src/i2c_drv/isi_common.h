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
 *************************************************************************
 */

#ifndef __ISI_COMMON_H__
#define __ISI_COMMON_H__

#define ISI_INTERFACE_VERSION		6

/*****************************************************************************/
/*capabilities / configuration
 *****************************************************************************/

/**<bus_width */
/**< to expand to a (possibly higher) resolution in marvin,
 *the l_s_bs will be set to zero
 */
#define ISI_BUSWIDTH_8BIT_ZZ		0x00000001U
/**< to expand to a (possibly higher) resolution in marvin,
 *the l_s_bs will be copied from the m_s_bs
 */
#define ISI_BUSWIDTH_8BIT_EX		0x00000002U
/**< /formerly known as ISI_BUSWIDTH_10BIT
 *(at times no marvin derivative was able to process more than 10 bit)
 */
#define ISI_BUSWIDTH_10BIT_EX		0x00000004U
#define ISI_BUSWIDTH_10BIT_ZZ		0x00000008U
#define ISI_BUSWIDTH_12BIT		0x00000010U

#define ISI_BUSWIDTH_10BIT		(ISI_BUSWIDTH_10BIT_EX)

/**< mode, operating mode of the image sensor in
 *terms of output data format and timing data transmission
 */
/**< YUV-data with separate h/v sync lines (ITU-R BT.601) */
#define ISI_MODE_BT601			0x00000001
/**< YUV-data with sync words inside the datastream (ITU-R BT.656) */
#define ISI_MODE_BT656			0x00000002
/*< bayer data with separate h/v sync lines*/
#define ISI_MODE_BAYER			0x00000004
/**< any binary data without line/column-structure,
 *(e.g. already JPEG encoded) h/v sync lines act as data valid signals
 */
#define ISI_MODE_DATA			0x00000008
/**< RAW picture data with separate h/v sync lines */
#define ISI_MODE_PICT			0x00000010
/**< RGB565 data with separate h/v sync lines */
#define ISI_MODE_RGB565			0x00000020
/**< SMIA conform data stream (see smia_mode for details) */
#define ISI_MODE_SMIA			0x00000040
/**< MIPI conform data stream (see mipi_mode for details) */
#define ISI_MODE_MIPI			0x00000080
/**< bayer data with sync words inside the datastream
 *(similar to ITU-R BT.656)
 */
#define ISI_MODE_BAY_BT656		0x00000100
/**< raw picture data with sync words inside the datastream
 *(similar to ITU-R BT.656)
 */
#define ISI_MODE_RAW_BT656		0x00000200

/**< smia_mode */
/*!< compression mode */
#define ISI_SMIA_MODE_COMPRESSED		0x00000001
/*!< 8bit to 10 bit decompression */
#define ISI_SMIA_MODE_RAW_8_TO_10_DECOMP	0x00000002
/*!< 12 bit RAW bayer data */
#define ISI_SMIA_MODE_RAW_12		0x00000004
/*!< 10 bit RAW bayer data */
#define ISI_SMIA_MODE_RAW_10		0x00000008
/*!< 8 bit RAW bayer data */
#define ISI_SMIA_MODE_RAW_8		0x00000010
/*!< 7 bit RAW bayer data */
#define ISI_SMIA_MODE_RAW_7		0x00000020
/*!< 6 bit RAW bayer data */
#define ISI_SMIA_MODE_RAW_6		0x00000040
#define ISI_SMIA_MODE_RGB_888		0x00000080
/*!< RGB 565 display ready data */
#define ISI_SMIA_MODE_RGB_565		0x00000100
/*!< RGB 444 display ready data */
#define ISI_SMIA_MODE_RGB_444		0x00000200
#define ISI_SMIA_MODE_YUV_420		0x00000400	/*!< YUV420 data */
#define ISI_SMIA_MODE_YUV_422		0x00000800	/*!< YUV422 data */
#define ISI_SMIA_OFF			0x80000000/*!< SMIA is disabled */

/**< mipi_mode */
#define ISI_MIPI_MODE_YUV420_8		0x00000001	/*!< YUV 420  8-bit */
#define ISI_MIPI_MODE_YUV420_10		0x00000002	/*!< YUV 420 10-bit */
/*!< legacy YUV 420 8-bit */
#define ISI_MIPI_MODE_LEGACY_YUV420_8	0x00000004
/*!< YUV 420 8-bit (CSPS) */
#define ISI_MIPI_MODE_YUV420_CSPS_8	0x00000008
/*!< YUV 420 10-bit (CSPS) */
#define ISI_MIPI_MODE_YUV420_CSPS_10	0x00000010
#define ISI_MIPI_MODE_YUV422_8		0x00000020	/*!< YUV 422 8-bit */
#define ISI_MIPI_MODE_YUV422_10		0x00000040	/*!< YUV 422 10-bit */
#define ISI_MIPI_MODE_RGB444		0x00000080	/*!< RGB 444 */
#define ISI_MIPI_MODE_RGB555		0x00000100	/*!< RGB 555 */
#define ISI_MIPI_MODE_RGB565		0x00000200	/*!< RGB 565 */
#define ISI_MIPI_MODE_RGB666		0x00000400	/*!< RGB 666 */
#define ISI_MIPI_MODE_RGB888		0x00000800	/*!< RGB 888 */
#define ISI_MIPI_MODE_RAW_6		0x00001000	/*!< RAW_6 */
#define ISI_MIPI_MODE_RAW_7		0x00002000	/*!< RAW_7 */
#define ISI_MIPI_MODE_RAW_8		0x00004000	/*!< RAW_8 */
#define ISI_MIPI_MODE_RAW_10		0x00008000	/*!< RAW_10 */
#define ISI_MIPI_MODE_RAW_12		0x00010000	/*!< RAW_12 */
#define ISI_MIPI_OFF			0x80000000/*!< MIPI is disabled */

/**< field_selection */
/**< sample all field (don't care about fields */
#define ISI_FIELDSEL_BOTH		0x00000001
/**< sample only even fields */
#define ISI_FIELDSEL_EVEN		0x00000002
/**< sample only odd fields */
#define ISI_FIELDSEL_ODD		0x00000004

/**< y_c_seq */
#define ISI_YCSEQ_YCBYCR		0x00000001
#define ISI_YCSEQ_YCRYCB		0x00000002
#define ISI_YCSEQ_CBYCRY		0x00000004
#define ISI_YCSEQ_CRYCBY		0x00000008

/**< conv422 */
#define ISI_CONV422_COSITED		0x00000001
#define ISI_CONV422_INTER		0x00000002
#define ISI_CONV422_NOCOSITED		0x00000004

/**< bayer_patttern */
#define ISI_BPAT_RGRGGBGB		0x00000001
#define ISI_BPAT_GRGRBGBG		0x00000002
#define ISI_BPAT_GBGBRGRG		0x00000004
#define ISI_BPAT_BGBGGRGR		0x00000008

/**< h_polarity */
/**< sync signal pulses high between lines */
#define ISI_HPOL_SYNCPOS		0x00000001
/**< sync signal pulses low between lines */
#define ISI_HPOL_SYNCNEG		0x00000002
/**< reference signal is high as long as sensor puts out line data */
#define ISI_HPOL_REFPOS			0x00000004
/**< reference signal is low as long as sensor puts out line data */
#define ISI_HPOL_REFNEG			0x00000008

/**< v_polarity */
#define ISI_VPOL_POS			0x00000001
#define ISI_VPOL_NEG			0x00000002

/**< edge */
#define ISI_EDGE_RISING			0x00000001
#define ISI_EDGE_FALLING		0x00000002

/**< bls (black level subtraction) */
/**< turns on/off additional black lines at frame start */
#define ISI_BLS_OFF			0x00000001
#define ISI_BLS_TWO_LINES		0x00000002
/**< two lines top and two lines bottom */
#define ISI_BLS_FOUR_LINES		0x00000004
#define ISI_BLS_SIX_LINES		0x00000008	/**< six lines top */

/**< gamma */
/**< turns on/off gamma correction in the sensor ISP */
#define ISI_GAMMA_ON			0x00000001
#define ISI_GAMMA_OFF			0x00000002

/**< color_conv */
/**< turns on/off color conversion matrix in the sensor ISP */
#define ISI_CCONV_ON			0x00000001
#define ISI_CCONV_OFF			0x00000002

/**< resolution */
#define ISI_RES_VGA		0x00000001	/**< 1  640x480*/
#define ISI_RES_2592_1944	0x00000002	/**< 2 2592x1944*/
#define ISI_RES_3264_2448	0x00000004	/**< 3 3264x2448*/
#define ISI_RES_4416_3312	0x00000010	/**< 5 4416x3312*/
#define ISI_RES_TV720P5		0x00010000	/**< 16 1280x720@5*/
#define ISI_RES_TV720P15	0x00020000	/**< 17 1280x720@15*/
#define ISI_RES_TV720P30	0x00040000	/**< 18 1280x720@30*/
#define ISI_RES_TV720P60	0x00080000	/**< 19 1280x720@60*/
#define ISI_RES_TV1080P5	0x00200000	/**< 21 1920x1080@5*/
#define ISI_RES_TV1080P6	0x00400000	/**< 22 1920x1080@6*/
#define ISI_RES_TV1080P10	0x00800000	/**< 23 1920x1080@10 */
#define ISI_RES_TV1080P12	0x01000000	/**< 24 1920x1080@12 */
#define ISI_RES_TV1080P15	0x02000000	/**< 25 1920x1080@15 */
#define ISI_RES_TV1080P20	0x04000000	/**< 26 1920x1080@20 */
#define ISI_RES_TV1080P24	0x08000000	/**< 27 1920x1080@24 */
#define ISI_RES_TV1080P25	0x10000000	/**< 28 1920x1080@25 */
#define ISI_RES_TV1080P30	0x20000000	/**< 29 1920x1080@30 */
#define ISI_RES_TV1080P50	0x40000000	/**< 30 1920x1080@50 */
#define ISI_RES_TV1080P60	0x80000000	/**< 31 1920x1080@60 */

/**< dwn_sz */
/*!< use subsampling to downsize output window */
#define ISI_DWNSZ_SUBSMPL		0x00000001
/*!< use scaling with bayer sampling to downsize output window */
#define ISI_DWNSZ_SCAL_BAY		0x00000002
/*!< use scaling with co-sited sampling to downsize output window */
#define ISI_DWNSZ_SCAL_COS		0x00000004

/**< BLC */
/**< camera black_level_correction on */
#define ISI_BLC_AUTO			0x00000001
/**< camera black_level_correction off */
#define ISI_BLC_OFF			0x00000002

/**< AGC */
/**< camera auto_gain_control on */
#define ISI_AGC_AUTO			0x00000001
/**< camera auto_gain_control off */
#define ISI_AGC_OFF			0x00000002

/**< AWB */
/**< camera auto_white_balance on */
#define ISI_AWB_AUTO			0x00000001
/**< camera auto_white_balance off */
#define ISI_AWB_OFF			0x00000002

/**< AEC */
/**< camera auto_exposure_control on */
#define ISI_AEC_AUTO			0x00000001
/**< camera auto_exposure_control off */
#define ISI_AEC_OFF			0x00000002

/**< DPCC */
/**< camera defect_pixel_correction on */
#define ISI_DPCC_AUTO			0x00000001
/**< camera defect_pixel_correction off */
#define ISI_DPCC_OFF			0x00000002

/**< AFPS */
/**< auto FPS mode not supported; or ISI_RES_XXX bitmask of all resolutions
 *being part of any AFPS series
 */
#define ISI_AFPS_NOTSUPP		0x00000000

/**< ul_cie_profile */
/*according to http://www.hunterlab.com/appnotes/an05_05.pdf, illuminants
 *A, D65 and F2 are most commonly used and should be selected prior to the
 *others if only a subset is to be supported. illuminants B and E are mentioned
 *here: http://www.aim-dtp.net/aim/technology/cie_xyz/cie_xyz.htm.
 */
#define ISI_CIEPROF_A		0x00000001 /*!< incandescent/tungsten,2856K */
#define ISI_CIEPROF_D40		0x00000002
#define ISI_CIEPROF_D45		0x00000004
#define ISI_CIEPROF_D50		0x00000008 /*!< horizon light,5000K */
#define ISI_CIEPROF_D55		0x00000010 /*!< mid morning daylight,5500K */
#define ISI_CIEPROF_D60		0x00000020
/*!< indoor D65 daylight from fluorescent lamp,	6504K */
#define ISI_CIEPROF_D65		0x00000040
#define ISI_CIEPROF_D70		0x00000080
#define ISI_CIEPROF_D75		0x00000100 /*!< overcast daylight, 7500K */
#define ISI_CIEPROF_D80		0x00000200
#define ISI_CIEPROF_D85		0x00000400
#define ISI_CIEPROF_D90		0x00000800
#define ISI_CIEPROF_D95		0x00001000
#define ISI_CIEPROF_D100	0x00002000
#define ISI_CIEPROF_D105	0x00004000
#define ISI_CIEPROF_D110	0x00008000
#define ISI_CIEPROF_D115	0x00010000
#define ISI_CIEPROF_D120	0x00020000
#define ISI_CIEPROF_E		0x00040000 /*!< normalized reference source */
#define ISI_CIEPROF_F1		0x00080000 /*!< daylight fluorescent 6430K*/
/*!< cool white fluorescent CWF		4230K */
#define ISI_CIEPROF_F2		0x00100000
#define ISI_CIEPROF_F3		0x00200000 /*!< white fluorescent 3450K*/
#define ISI_CIEPROF_F4		0x00400000 /*!< warm white fluorescent 2940K*/
#define ISI_CIEPROF_F5		0x00800000 /*!< daylight fluorescent 6350K*/
#define ISI_CIEPROF_F6		0x01000000 /*!< lite white fluorescent 4150K*/
/*!< similar to D65, daylight fluorescent	6500K */
#define ISI_CIEPROF_F7		0x02000000
/*!< similar to D50, sylvania F40DSGN50 fluorescent5000K */
#define ISI_CIEPROF_F8		0x04000000
/*!< cool white deluxe fluorescent 4150K */
#define ISI_CIEPROF_F9		0x08000000
#define ISI_CIEPROF_F10		0x10000000 /*!< TL85, ultralume 50 5000K*/
#define ISI_CIEPROF_F11		0x20000000 /*!< TL84, ultralume 40,SP41 4000K*/
#define ISI_CIEPROF_F12		0x40000000 /*!< TL83, ultralume 30 3000K*/
#define ISI_CIEPROF_HORIZON	0x80000000 /*!< TL83, ultralume 30 */
#endif /*__ISI_COMMON_H__ */

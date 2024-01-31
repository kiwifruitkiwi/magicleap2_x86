/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef DW_COMMON_BITOPS_H
#define DW_COMMON_BITOPS_H

/****h*include.bitops
 *NAME
 *d_w_common_bitops.h -- bit manipulation macros
 *DESCRIPTION
 *this set of macros accesses data using a width/shift parameter.
 *this assumes that constants bfo_x_x_x and bfw_x_x_x are defined,
 *where XXX is the value specified in the parameter.
 *+ bfo_x_x_x is the offset of the lowest bit.
 *+ bfw_x_x_x is the number of bits to be accessed.
 ***/

/****d*include.bitops/DW_BITS
 *DESCRIPTION
 *constant definitions for various bits of a 32-bit word.
 *SOURCE
 */
#define DW_BIT0		0x00000001
#define DW_BIT1		0x00000002
#define DW_BIT2		0x00000004
#define DW_BIT3		0x00000008
#define DW_BIT4		0x00000010
#define DW_BIT5		0x00000020
#define DW_BIT6		0x00000040
#define DW_BIT7		0x00000080
#define DW_BIT8		0x00000100
#define DW_BIT9		0x00000200
#define DW_BIT10	0x00000400
#define DW_BIT11	0x00000800
#define DW_BIT12	0x00001000
#define DW_BIT13	0x00002000
#define DW_BIT14	0x00004000
#define DW_BIT15	0x00008000
#define DW_BIT16	0x00010000
#define DW_BIT17	0x00020000
#define DW_BIT18	0x00040000
#define DW_BIT19	0x00080000
#define DW_BIT20	0x00100000
#define DW_BIT21	0x00200000
#define DW_BIT22	0x00400000
#define DW_BIT23	0x00800000
#define DW_BIT24	0x01000000
#define DW_BIT25	0x02000000
#define DW_BIT26	0x04000000
#define DW_BIT27	0x08000000
#define DW_BIT28	0x10000000
#define DW_BIT29	0x20000000
#define DW_BIT30	0x40000000
#define DW_BIT31	0x80000000
#define DW_BITS_ALL 0xFFFFFFFF
/*****/

	/****d*include.bitops/DW_BIT_WIDTH
	 *DESCRIPTION
	 *returns the width of the specified bit-field.
	 *ARGUMENTS
	 *__bfwsa width/shift pair
	 *SOURCE
	 */
#define DW_BIT_WIDTH(__bfws)((unsigned int) (bfw ## __bfws))
/*****/

	/****d*include.bitops/DW_BIT_OFFSET
	 *DESCRIPTION
	 *returns the offset of the specified bit-field.
	 *ARGUMENTS
	 *__bfwsa width/shift pair
	 *SOURCE
	 */
#define DW_BIT_OFFSET(__bfws) ((unsigned int) (bfo ## __bfws))
/*****/

	/****d*include.bitops/DW_BIT_MASK
	 *DESCRIPTION
	 *returns a mask with the bits to be addressed set and all others
	 *cleared.
	 *ARGUMENTS
	 *__bfws	a width/shift pair
	 *SOURCE
	 */

#define DW_BIT_MASK(__bfws) ((unsigned int) (((bfw ## __bfws) == 32) ?	\
		0xFFFFFFFF : ((1U << (bfw ## __bfws)) - 1)) << (bfo ## __bfws))
/*****/

	/****d*include.bitops/DW_BIT_CLEAR
	 *DESCRIPTION
	 *clear the specified bits.
	 *ARGUMENTS
	 *__datum the word of data to be modified
	 *__bfwsa width/shift pair
	 *SOURCE
	 */
#define DW_BIT_CLEAR(__datum, __bfws)	\
	((__datum) = ((unsigned int) (__datum) & ~DW_BIT_MASK(__bfws)))
/*****/

	/****d*include.bitops/DW_BIT_GET_UNSHIFTED
	 *DESCRIPTION
	 *returns the relevant bits masked from the data word, still at their
	 *original offset.
	 *ARGUMENTS
	 *__datum	the word of data to be accessed
	 *__bfws	a width/shift pair
	 *SOURCE
	 */
#define DW_BIT_GET_UNSHIFTED(__datum, __bfws)	\
	((unsigned int) ((__datum) & DW_BIT_MASK(__bfws)))
/*****/

	/****d*include.bitops/DW_BIT_GET
	 *DESCRIPTION
	 *returns the relevant bits masked from the data word shifted to bit
	 *zero (i.e. access the specified bits from a word of data as an
	 *integer value).
	 *ARGUMENTS
	 *__datum	the word of data to be accessed
	 *__bfws	a width/shift pair
	 *SOURCE
	 */
#define DW_BIT_GET(__datum, __bfws)	\
	((unsigned int) (((__datum) & DW_BIT_MASK(__bfws)) >>	\
	(bfo ## __bfws)))
/*****/

	/****d*include.bitops/DW_BIT_SET
	 *DESCRIPTION
	 *place the specified value into the specified bits of a word of data
	 *(first the data is read, and the non-specified bits are re-written).
	 *ARGUMENTS
	 *__datum	the word of data to be accessed
	 *__bfws	a width/shift pair
	 *__val	the data value to be shifted into the specified bits
	 *SOURCE
	 */
#define DW_BIT_SET(__datum, __bfws, __val)	\
	((__datum) = ((unsigned int) (__datum) & ~DW_BIT_MASK(__bfws)) | \
	(((__val) << (bfo ## __bfws)) & DW_BIT_MASK(__bfws)))
/*****/

	/****d*include.bitops/DW_BIT_SET_NOREAD
	 *DESCRIPTION
	 *place the specified value into the specified bits of a word of data
	 *without reading first - for sensitive interrupt type registers
	 *ARGUMENTS
	 *__datum     the word of data to be accessed
	 *__bfws      a width/shift pair
	 *__val       the data value to be shifted into the specified bits
	 *SOURCE
	 */
#define DW_BIT_SET_NOREAD(__datum, __bfws, __val)	\
	((unsigned int) ((__datum) = (((__val) << (bfo ## __bfws)) &	\
	DW_BIT_MASK(__bfws))))
/*****/

	/****d*include.bitops/DW_BIT_BUILD
	 *DESCRIPTION
	 *shift the specified value into the desired bits.
	 *ARGUMENTS
	 *__bfws	a width/shift pair
	 *__val		the data value to be shifted into the specified bits
	 *SOURCE
	 */
#define DW_BIT_BUILD(__bfws, __val)	\
	((unsigned int) (((__val) << (bfo ## __bfws)) & DW_BIT_MASK(__bfws)))
/*****/

#endif				/*DW_COMMON_BITOPS_H */

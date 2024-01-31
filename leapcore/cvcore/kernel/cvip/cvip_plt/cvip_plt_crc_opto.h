/* SPDX-License-Identifier: GPL-2.0-or-later */
//from: python pycrc.py --model=crc-8 --std=C99 --generate=h --algorithm=tbl
//--symbol-prefix=crc8_
/**
 * file: cvip_plt_crc_opto
 * Functions and types for CRC checks.
 *
 * Generated on Mon Nov 17 16:27:06 2014,
 * by pycrc v0.8.1, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 8
 *    Poly         = 0x07
 *    XorIn        = 0x00
 *    ReflectIn    = False
 *    XorOut       = 0x00
 *    ReflectOut   = False
 *    Algorithm    = table-driven
 * NOTE: References to file names and the prefix for functions and types
 * have been updated for local details in 2022
 *****************************************************************************/
#ifndef __CVIP_PLT_CRC_OPTO__
#define __CVIP_PLT_CRC_OPTO__

//#include <stdint.h>
//#include <stdlib.h>
#include <stddef.h>
#include <linux/string.h>

#ifdef ANDROID
	#include <common/visibility.h>
#else
	#define SO_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The definition of the used algorithm.
 *****************************************************************************/
#define CVIP_PLT_CRC_ALGO_TABLE_DRIVEN 1


/**
 * The type of the CRC values.
 *
 * This type must be big enough to contain at least 8 bits.
 *****************************************************************************/
typedef uint8_t cvip_plt_crc8_t;


/**
 * Calculate the initial crc value.
 *
 * \return     The initial crc value.
 *****************************************************************************/
static inline cvip_plt_crc8_t cvip_plt_crc8_init(void)
{
	return 0xFF;
}


/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
SO_EXPORT cvip_plt_crc8_t cvip_plt_crc8_update(cvip_plt_crc8_t crc,
	const unsigned char *data, size_t data_len);


/**
 * Calculate the final crc value.
 *
 * \param crc  The current crc value.
 * \return     The final crc value.
 *****************************************************************************/
static inline cvip_plt_crc8_t cvip_plt_crc8_finalize(cvip_plt_crc8_t crc)
{
	return crc ^ 0x00;
}


//from: python pycrc.py --model=ccitt  --xor-in=0xFFFF --generate=h
//                      --algorithm=tbl --symbol-prefix=crc16_

typedef uint16_t cvip_plt_crc16_t;


/**
 * Calculate the initial crc value.
 *
 * \return     The initial crc value.
 *****************************************************************************/
static inline cvip_plt_crc16_t cvip_plt_crc16_init(void)
{
	return 0xffff;
}


/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
SO_EXPORT cvip_plt_crc16_t cvip_plt_crc16_update(cvip_plt_crc16_t crc,
	const unsigned char *data, size_t data_len);


/**
 * Calculate the final crc value.
 *
 * \param crc  The current crc value.
 * \return     The final crc value.
 *****************************************************************************/
static inline cvip_plt_crc16_t cvip_plt_crc16_finalize(cvip_plt_crc16_t crc)
{
	return crc ^ 0x0000;
}


#ifdef __cplusplus
}           /* closing brace for extern "C" */
#endif

#endif      /* __CVIP_PLT_CRC_OPTO__ */

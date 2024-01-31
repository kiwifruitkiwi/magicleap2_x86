/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef GSM_JRTC_H
#define GSM_JRTC_H

/**
* \brief Retrieve the current value of the JRTC.
*
* \return   Current tick count of the shared high resolution Real Time Clock.
*/
uint64_t gsm_jrtc_get_ticks(void);

/**
* \brief Retrieve the current value of the JRTC in nanoseconds.
*
* \return   Current value of the shared high resolution Real Time Clock in nanoseconds.
*/
uint64_t gsm_jrtc_get_time_ns(void);

/**
* \brief Retrieve the current value of the JRTC in microseconds.
*
* \return   Current value of the shared high resolution Real Time Clock in microseconds.
*/
uint64_t gsm_jrtc_get_time_us(void);

/**
* \brief Retrieve the current value of the JRTC in milliseconds.
*
* \return   Current value of the shared high resolution Real Time Clock in milliseconds.
*/
uint64_t gsm_jrtc_get_time_ms(void);

/**
* \brief Convert a raw jrtc (ticks) value to nanoseconds
*
* \return   The given tick value converted to nanoseconds
*/
uint64_t gsm_jrtc_to_ns(uint64_t jrtc);

/**
* \brief Convert a raw jrtc (ticks) value to microseconds
*
* \return   The given tick value converted to microseconds
*/
uint64_t gsm_jrtc_to_us(uint64_t jrtc);

/**
* \brief Convert a raw jrtc (ticks) value to milliseconds
*
* \return   The given tick value converted to milliseconds
*/
uint64_t gsm_jrtc_to_ms(uint64_t jrtc);

/**
* \brief Convert nanoseconds to a raw jrtc (ticks) value
*
* \return   The given nanoseconds converted to tick value
*/
uint64_t gsm_ns_to_jrtc(uint64_t ns);

/**
* \brief Convert microseconds to a raw jrtc (ticks) value
*
* \return   The given microseconds converted to tick value
*/
uint64_t gsm_us_to_jrtc(uint64_t us);

/**
* \brief Convert milliseconds to a raw jrtc (ticks) value
*
* \return   The given milliseconds converted to tick value
*/
uint64_t gsm_ms_to_jrtc(uint64_t ms);

/**
* \brief Check if the JRTC is operating within specification.
*
* \return   True if JRTC is operating within specification,
*           False otherwise.
*/
bool gsm_jrtc_check_is_valid(void);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_JRTC_H */

// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020
// Magic Leap, Inc. (COMPANY)
//
#include <linux/types.h>
#include <linux/module.h>
#include <linux/preempt.h>
#include <linux/io.h>
#include "gsm.h"
#include "gsm_cvip.h"

#define JRTC_NANOSECONDS_PER_SECOND (1000000000U)
#define JRTC_MICROSECONDS_PER_SECOND    (1000000U)
#define JRTC_MILLISECONDS_PER_SECOND    (1000U)

// Data points: 25911, 25915, 25913, 25911, 25915; lower by 10% cushion
#define JRTC_MIN_RFRC_AT_850        (23320)
// Data points: 25965, 25964, 25964, 25969, 25964; raise by 10% cushion
#define JRTC_MAX_RFRC_AT_850        (28566)

// Data points: 18290, 18293, 18291, 18290, 18294; lower by 10% cushion
#define JRTC_MIN_RFRC_AT_600        (16461)
// Data points: 18328, 18327, 18328, 18333, 18327; raise by 10% cushion
#define JRTC_MAX_RFRC_AT_600        (20161)

// Data points: 12193, 12195, 12193, 12194, 12196; lower by 10% cushion
#define JRTC_MIN_RFRC_AT_400        (10974)
// Data points: 12219, 12218, 12218, 12218, 12218; raise by 10% cushion
#define JRTC_MAX_RFRC_AT_400        (13441)

// gsm_gac_get_ticks conceptually returns a fixed-point number of 32768Hz clock ticks in 48.16 format.
// As an integer, that is 32768 * 65536 ticks per second.
#define JRTC_TICKS_PER_SEC (32768UL * 65536UL)

#define GSM_JRTC_POISON 0xABCDEF00

static inline int32_t MAX(int32_t a, int32_t b) {
    return ((a) > (b) ? a : b);
}
static inline int32_t MIN(int32_t a, int32_t b) {
    return ((a) < (b) ? a : b);
}

/**
* Structure definition for JRTC Counter 3 value
*/
union jrtc_counter3_val {
    uint64_t raw64[2];
    struct {
        uint16_t rfrc_hist[4];
        uint16_t rfrc;
        uint16_t rtcc[3];
    };
};

static inline bool __gsm_jrtc_check_is_valid(__uint128_t raw128)
{
	int i;
	int invalid;
	// The rfrc will vary depending on the clock speed of the NIC-400.
	int num_rfrc_in_400_range;
	int num_rfrc_in_600_range;
	int num_rfrc_in_850_range;
	union jrtc_counter3_val regval;

	regval.raw64[0] = raw128 & 0xffffffffffffffffU;
	regval.raw64[1] = raw128 >> 64;

	// Ensure sane denominators (rfrc_hist within bounds)
	num_rfrc_in_400_range = 0;
	num_rfrc_in_600_range = 0;
	num_rfrc_in_850_range = 0;

	for (i = 0; i < 4; i++) {
		if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_850 &&
			regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_850) {
			num_rfrc_in_850_range++;
		}
		if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_600 &&
			regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_600) {
			num_rfrc_in_600_range++;
		}
		if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_400 &&
			regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_400) {
			num_rfrc_in_400_range++;
		}
	}

	// Assume that only one nic clock change can happen in 4 slow clock ticks.
	// Therefore, all 4 samples should fall into one of two bins.
	invalid = 1;
	if (num_rfrc_in_850_range + num_rfrc_in_600_range == 4 && num_rfrc_in_400_range == 0) {
		invalid = 0;
	}
	if (num_rfrc_in_850_range + num_rfrc_in_400_range == 4 && num_rfrc_in_600_range == 0) {
		invalid = 0;
	}
	if (num_rfrc_in_600_range + num_rfrc_in_400_range == 4 && num_rfrc_in_850_range == 0) {
		invalid = 0;
	}

	return invalid ? false : true;
}

static uint64_t _gsm_jrtc_get_ticks_a2(void)
{
	int i;
	uint64_t rtcc;
	uint32_t fract;
	uint16_t maxdenom;
	union jrtc_counter3_val regval;
	__uint128_t raw128;

	raw128 = gsm_reg_read_128(GSMI_JRTC_COUNTER3);

	regval.raw64[0] = raw128 & 0xffffffffffffffffU;
	regval.raw64[1] = raw128 >> 64;

	rtcc = regval.raw64[1] >> 16;

	maxdenom = 1;

	for (i = 0; i < 4; i++)
		maxdenom = MAX(maxdenom, regval.rfrc_hist[i]);

	fract = MIN(65535ul, ((uint32_t)regval.rfrc * 65536ul) / maxdenom);
	return (uint64_t)(rtcc * 65536ull + fract);
}

static uint64_t _gsm_jrtc_get_ticks_a1(void)
{
	int invalid;
	int i;
	uint64_t rtcc;
	uint32_t fract;
	uint16_t maxdenom;
        // The rfrc will vary depending on the clock speed of the NIC-400.
	int num_rfrc_in_400_range;
	int num_rfrc_in_600_range;
	int num_rfrc_in_850_range;
	union jrtc_counter3_val regval;
	__uint128_t raw128;

	// TODO(dlaw): There appears to be a bug that causes us to read a random GSM location
	// instead of the actual register value, apparently if there is a gsm write happening
	// at the same time.  Once we have A2, we should remove the while
	// loop.  I (jsandler) have opted to do an infinite loop, because there is
	// no valid failure mode here, and thus no way to force the calling context
	// to catch the failure.  Since it isn't expected that everyone sanity check
	// their timestamps, it seems reasonable to force a failure in the unlikely
	// event that we NEVER get rfrc within bounds.
	while (1) {
		raw128 = gsm_reg_read_128(GSMI_JRTC_COUNTER3);

		regval.raw64[0] = raw128 & 0xffffffffffffffffU;
		regval.raw64[1] = raw128 >> 64;

		rtcc = regval.raw64[1] >> 16;

		// Ensure sane denominators (rfrc_hist within bounds)

		maxdenom = 1;
		num_rfrc_in_400_range = 0;
		num_rfrc_in_600_range = 0;
		num_rfrc_in_850_range = 0;

		for (i = 0; i < 4; i++) {
			if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_850 &&
                                regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_850) {
                                num_rfrc_in_850_range++;
			}
			if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_600 &&
                                regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_600) {
                                num_rfrc_in_600_range++;
			}
			if (regval.rfrc_hist[i] > JRTC_MIN_RFRC_AT_400 &&
                                regval.rfrc_hist[i] < JRTC_MAX_RFRC_AT_400) {
                                num_rfrc_in_400_range++;
			}
			maxdenom = MAX(maxdenom, regval.rfrc_hist[i]);
		}

		// Assume that only one nic clock change can happen in 4 slow clock ticks.
		// Therefore, all 4 samples should fall into one of two bins.
		invalid = 1;
		if (num_rfrc_in_850_range + num_rfrc_in_600_range == 4 && num_rfrc_in_400_range == 0) {
			invalid = 0;
		}
		if (num_rfrc_in_850_range + num_rfrc_in_400_range == 4 && num_rfrc_in_600_range == 0) {
			invalid = 0;
		}
		if (num_rfrc_in_600_range + num_rfrc_in_400_range == 4 && num_rfrc_in_850_range == 0) {
			invalid = 0;
		}


		if (invalid == 1) {
			// None of our history fields should be out of bounds.  If this happened, we hit
			// the register latch issue.  Go back to the start of the while loop
			// and try again, unless in non task context, then
			// return 0.
			if (in_task()) {
				continue;
			} else {
				return 0;
			}
		}

		// If we got this far, we have resonable confidence that we did not hit the
		// register latch issue.  Let's go with what we have.
		fract = MIN(65535ul, ((uint32_t)regval.rfrc * 65536ul) / maxdenom);
		return (uint64_t)(rtcc * 65536ull + fract);
	}
}

#if defined(__i386__) || defined(__amd64__)
static uint64_t _gsm_jrtc_get_ticks_check_hw(void);

/* assume a1 and check if hw id is passed from cvip */
static uint64_t (*_gsm_jrtc_get_ticks)(void) = _gsm_jrtc_get_ticks_check_hw;

static uint64_t _gsm_jrtc_get_ticks_check_hw(void)
{
	u32 hw_id = GSM_READ_32(GSMI_READ_COMMAND_RAW, GSM_RESERVED_HW_ID_OFFSET);

	if ((hw_id & 0xFFFFFF00) == GSM_JRTC_POISON) {
		if ((hw_id & 0xFF) == 0xA2) {
			_gsm_jrtc_get_ticks = _gsm_jrtc_get_ticks_a2;
			return _gsm_jrtc_get_ticks_a2();
		} else {
			_gsm_jrtc_get_ticks = _gsm_jrtc_get_ticks_a1;
		}
	}

	return _gsm_jrtc_get_ticks_a1();
}
#else
/* assume a1 hw unless boot cfg is checked */
static uint64_t (*_gsm_jrtc_get_ticks)(void)  = _gsm_jrtc_get_ticks_a1;

#define BOOT_CONFIG_ADDR        (0x0000a000U)
#define BOOT_CONFIG_SIZE        (sizeof(uint32_t))

static int is_hw_mero_a2(int board_id)
{
	if (board_id == 19 || board_id < 18)
		return 0;
	return 1;
}

void gsm_jrtc_init(void)
{
	void *boot_config_base;
	u32 hw_id = GSM_JRTC_POISON | 0xA1;

	boot_config_base = memremap(BOOT_CONFIG_ADDR, BOOT_CONFIG_SIZE, MEMREMAP_WB);
	if (boot_config_base == NULL) {
		pr_err("%s: memremap failed (%x, %lu)\n",
				__func__, BOOT_CONFIG_ADDR, BOOT_CONFIG_SIZE);
		return;
	}
	if (is_hw_mero_a2(*((char *)boot_config_base + 1) & 0x3F)) {
		_gsm_jrtc_get_ticks  = _gsm_jrtc_get_ticks_a2;
		hw_id = GSM_JRTC_POISON | 0xA2;
	}

	GSM_WRITE_32(GSMI_WRITE_COMMAND_RAW, GSM_RESERVED_HW_ID_OFFSET, hw_id);

	memunmap(boot_config_base);
}
#endif

/**
* Retrieve the current value of JRTC in RTC ticks.
*/
uint64_t gsm_jrtc_get_ticks(void)
{
	return _gsm_jrtc_get_ticks();
}
EXPORT_SYMBOL(gsm_jrtc_get_ticks);

/**
* Retrieve the current value of JRTC in nanoseconds.  Do the multiply and divide in 128 bits
* in order to not lose precision.
*/
uint64_t gsm_jrtc_get_time_ns(void)
{
	uint64_t ns;
	__uint128_t math;

	math = gsm_jrtc_get_ticks();
	math = (math * JRTC_NANOSECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	ns = math;
	return ns;
}
EXPORT_SYMBOL(gsm_jrtc_get_time_ns);

/**
* Retrieve the current value of JRTC in microseconds.  Do the division without loss of precision.
*/
uint64_t gsm_jrtc_get_time_us(void)
{
	uint64_t ns;
	__uint128_t math;

	math = gsm_jrtc_get_ticks();
	math = (math * JRTC_MICROSECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	ns = math;
	return ns;
}
EXPORT_SYMBOL(gsm_jrtc_get_time_us);

/**
* Retrieve the current value of JRTC in milliseconds.  Do the division without loss of precision.
*/
uint64_t gsm_jrtc_get_time_ms(void)
{
	uint64_t ns;
	__uint128_t math;

	math = gsm_jrtc_get_ticks();
	math = (math * JRTC_MILLISECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	ns = math;
	return ns;
}
EXPORT_SYMBOL(gsm_jrtc_get_time_ms);

uint64_t gsm_jrtc_to_ns(uint64_t jrtc)
{
	uint64_t ns;
	__uint128_t math;

	math = jrtc;
	math = (math * JRTC_NANOSECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	ns = math;
	return ns;
}
EXPORT_SYMBOL(gsm_jrtc_to_ns);

uint64_t gsm_jrtc_to_us(uint64_t jrtc)
{
	uint64_t us;
	__uint128_t math;

	math = jrtc;
	math = (math * JRTC_MICROSECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	us = math;
	return us;
}
EXPORT_SYMBOL(gsm_jrtc_to_us);

uint64_t gsm_jrtc_to_ms(uint64_t jrtc)
{
	uint64_t ms;
	__uint128_t math;

	math = jrtc;
	math = (math * JRTC_MILLISECONDS_PER_SECOND) / JRTC_TICKS_PER_SEC;
	ms = math;
	return ms;
}
EXPORT_SYMBOL(gsm_jrtc_to_ms);

bool gsm_jrtc_check_is_valid(void)
{
	extern uint8_t gsm_core_is_cvip_up(void);
	int attempts;
	__uint128_t jrtc_cnt3;

	/*
	 * If gsm core is not initialized, we assume the
	 * jrtc is (will be) within spec.
	 */
	if (!gsm_core_is_cvip_up())
		return true;

	/* We'll allow four attempts to be invalid */
	for (attempts = 0; attempts < 4; ++attempts) {
		jrtc_cnt3 = gsm_reg_read_128(GSMI_JRTC_COUNTER3);
		if (__gsm_jrtc_check_is_valid(jrtc_cnt3))
			return true;
	}
	return false;
}
EXPORT_SYMBOL(gsm_jrtc_check_is_valid);

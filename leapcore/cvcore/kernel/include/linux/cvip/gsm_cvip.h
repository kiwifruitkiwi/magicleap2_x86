/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __GSM_CVIP_H__
#define __GSM_CVIP_H__

// master_membar (CVIP GSM)
#define CVCORE_GSM_PADDR             (0xC0000000U)
#define CVCORE_GSM_SIZE              (0x800000U)

/*!
 * CVIP GSM - JRTC (access available to x86)
 * JRTC address is 0xC0000800 (page aligned to 0xC0000000)
 * JRTC size is 0x40 (page aligned to 0x1000)
 */
#define CVCORE_GSM_REG_SIF_PADDR     (0xC0000000U)
#define CVCORE_GSM_REG_SIF_VADDR     (0x00500000U)
#define CVCORE_GSM_REG_SIF_SIZE      (0x1000U)

#define CVCORE_GSM_JRTC_SIF_OFFSET   (0x800U)
#define CVCORE_GSM_JRTC_SIF_SIZE     (0x40U)

/*!
 * This is the virtual start address for GSM memory
 * that is accessible to the x86 and can be mapped
 * through the /dev/gsm_mem driver.
 *
 * The start address, 0x00520000, doesn't actually map
 * to anything, but was picked to make offset calculations
 * easier.
 *
 * Example:
 * The address for GSM_RESERVED_REGISTRY_QPAD in gsm.h
 * is 0x02000.
 * The resulting virtual address calculation for the
 * qpad registry is:
 *      0x00520000 + 0x02000 = 0x00522000
 *
 * Under apu_s_intf_x86 in ml-mero-cvip-mortable-common.dtsi
 * is an entry for the input address 0x00522000 that maps to
 * the physical address 0xc0082000 in gsm memory.
 */
#define CVCORE_GSM_MEM_SIF_VADDR     (0x00520000U)

#define GSMI_READ_COMMAND_REGISTER                   (0)
#define GSMI_READ_COMMAND_RAW                        (1)
#define GSMI_READ_COMMAND_FIND_AND_SET_32            (2)
#define GSMI_READ_COMMAND_FIND_AND_SET_128           (3)
#define GSMI_READ_COMMAND_ATOMIC_INCREMENT           (4)
#define GSMI_READ_COMMAND_ATOMIC_DECREMENT           (5)
#define GSMI_READ_COMMAND_TEST_AND_SET_AXI           (6)
#define GSMI_READ_COMMAND_RING_BUFFER                (7)  // Not implemented yet
#define GSMI_READ_COMMAND_NIBBLE_TEST_AND_SET_BASE   (8)

#define GSMI_WRITE_COMMAND_REGISTER                  (0)
#define GSMI_WRITE_COMMAND_RAW                       (1)
#define GSMI_WRITE_COMMAND_ATOMIC_BITSET             (2)
#define GSMI_WRITE_COMMAND_ATOMIC_BITCLEAR           (3)
#define GSMI_WRITE_COMMAND_ATOMIC_XOR                (4)
#define GSMI_WRITE_COMMAND_ATOMIC_ADD                (5)
#define GSMI_WRITE_COMMAND_ATOMIC_SUBTRACT           (6)
// Command 7 does not exist
#define GSMI_WRITE_COMMAND_NIBBLE_WRITE_BASE         (8)

// Interesting GSM registers.
#define GSMI_REGISTER_GAC                            (0x20)

// GSM JRTC registers.
#define GSMI_JRTC_CONTROL                           (0x800)
#define GSMI_JRTC_COUNTER1                          (0x810)
#define GSMI_JRTC_COUNTER2                          (0x820)
#define GSMI_JRTC_COUNTER3                          (0x830)

extern uint8_t *gsm_backing;
#define GSM_BASE (gsm_backing)

#if defined(__i386__) || defined(__amd64__)
#define GSM_REGION_SHIFT (13)
#else
#define GSM_REGION_SHIFT (19)
#endif
#define GSM_REGION_MASK ((0x1U << GSM_REGION_SHIFT) - 1)
#define GSM_REGION_COUNT (16)
#define GSM_X86_REGION_SIZE  (0x2000)

// The first 512KB is register memory and the second 512KB is shared GSM memory
#define GSM_SHARED_MEM_BASE (GSM_BASE + GSM_MEMORY_SIZE_BYTES)

#define GSM_ADDRESS_8(command, offset)               (volatile uint8_t *) ((uintptr_t)GSM_BASE + (((command) << GSM_REGION_SHIFT) | ((uintptr_t)(offset) & GSM_REGION_MASK)))
#define GSM_ADDRESS_32(command, offset)              (volatile uint32_t *) ((uintptr_t)GSM_BASE + (((command) << GSM_REGION_SHIFT) | ((uintptr_t)(offset) & GSM_REGION_MASK)))
#define GSM_ADDRESS_64(command, offset)              (volatile uint64_t *) ((uintptr_t)GSM_BASE + (((command) << GSM_REGION_SHIFT) | ((uintptr_t)(offset) & GSM_REGION_MASK)))
#define GSM_ADDRESS_128(command, offset)             (volatile __uint128_t *) ((uintptr_t)GSM_BASE + (((command) << GSM_REGION_SHIFT) | ((uintptr_t)(offset) & GSM_REGION_MASK)))
#define GSM_READ_8(command, offset)                  (*GSM_ADDRESS_8((command), (offset)))
#define GSM_READ_32(command, offset)                 (*GSM_ADDRESS_32((command), (offset)))
#define GSM_READ_64(command, offset)                 (*GSM_ADDRESS_64((command), (offset)))
#define GSM_READ_128(command, offset)                (*GSM_ADDRESS_128((command), (offset)))
#define GSM_WRITE_32(command, offset, val)           (*GSM_ADDRESS_32((command), (offset))) = (val)
#define GSM_WRITE_64(command, offset, val)           (*GSM_ADDRESS_64((command), (offset))) = (val)
#define GSM_WRITE_128(command, offset, val)          (*GSM_ADDRESS_128((command), (offset))) = (val)

// These are in lower case to match matheval function names, to allow for common code
#define gsm_raw_read_8(offset)                       GSM_READ_8(GSMI_READ_COMMAND_RAW, (offset))
#define gsm_raw_read_32(offset)                      GSM_READ_32(GSMI_READ_COMMAND_RAW, (offset))
#define gsm_raw_read_64(offset)                      GSM_READ_64(GSMI_READ_COMMAND_RAW, (offset))
#define gsm_raw_read_128(offset)                     GSM_READ_128(GSMI_READ_COMMAND_RAW, (offset))
#define gsm_raw_write_32(offset, val)                GSM_WRITE_32(GSMI_WRITE_COMMAND_RAW, (offset), (val))
#define gsm_raw_write_64(offset, val)                GSM_WRITE_64(GSMI_WRITE_COMMAND_RAW, (offset), (val))
#define gsm_raw_write_128(offset, val)               GSM_WRITE_128(GSMI_WRITE_COMMAND_RAW, (offset), (val))

// These two inline functions likely need to be optimized to do a single 128bit access.
// JTate suggests using neon intrinsics for ARM.  We need to figure that out, and find a
// DSP friendly implementation as well.  Not that this may mean we need to compile all ARM
// programs that use this with an additional option - something like "-mfpu=neon".
// For now, this works.
static inline void gsm_raw_read_128_ptr(uint32_t offset, uint64_t *val) {
	val[0] = gsm_raw_read_64(offset);
	val[1] = gsm_raw_read_64((offset + 8));
}

static inline void gsm_raw_write_128_ptr(uint32_t offset, uint64_t *val) {
	gsm_raw_write_64(offset, val[0]);
	gsm_raw_write_64((offset + 8), val[1]);
}

#define gsm_reg_read_8(offset)                       GSM_READ_8(GSMI_READ_COMMAND_REGISTER, (offset))
#define gsm_reg_read_32(offset)                      GSM_READ_32(GSMI_READ_COMMAND_REGISTER, (offset))
#define gsm_reg_read_64(offset)                      GSM_READ_64(GSMI_READ_COMMAND_REGISTER, (offset))
#define gsm_reg_read_128(offset)                     GSM_READ_128(GSMI_READ_COMMAND_REGISTER, (offset))
#define gsm_reg_write_32(offset, val)                GSM_WRITE_32(GSMI_WRITE_COMMAND_REGISTER, (offset), (val))
#define gsm_reg_write_64(offset, val)                GSM_WRITE_64(GSMI_WRITE_COMMAND_REGISTER, (offset), (val))
#define gsm_reg_write_128(offset, val)               GSM_WRITE_128(GSMI_WRITE_COMMAND_REGISTER, (offset), (val))

uint8_t gsm_core_is_cvip_up(void);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_CVIP_H */

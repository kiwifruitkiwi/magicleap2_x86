/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __GSM_H__
#define __GSM_H__

#define CVCORE_GSM_ALWAYS_INLINE  static inline __attribute__ ((always_inline, unused))

#define GSM_MEMORY_SIZE_BYTES                           (512 * 1024)
#define GSM_TOTAL_SIZE_BYTES                            (8 * 1024 * 1024)

#define GSM_IPC_NUM_MAILBOXES                           (32)
#define GSM_IPC_GLOBAL_EXT_INFO_SIZE                    (32)
#define GSM_IPC_MAILBOX_EXT_INFO_SIZE                   (16)

// GSM memory area shared with the x86 via /dev/gsm_mem
#define GSM_X86_DEV_GSM_MEM_BASE                        (0x02000U)
#define GSM_X86_DEV_GSM_MEM_SIZE                        (32 * 1024) // 32KB

#define GSM_RESERVED_MLNET_CONTROL_OFFSET               (0x0102CU)
#define GSM_RESERVED_MLNET_CONTROL_SIZE                 (4)

// GSM IPC Global State Info for X86
#define GSM_IPC3_GLOBAL_EXT_INFO_SIZE                   (GSM_IPC_GLOBAL_EXT_INFO_SIZE)
#define GSM_RESERVED_IPC_IPC3_GLOBAL_EXT_INFO_START     (0x01460U)
#define GSM_IPC3_MAILBOX_EXT_INFO_SIZE                  (GSM_IPC_MAILBOX_EXT_INFO_SIZE * GSM_IPC_NUM_MAILBOXES)
#define GSM_RESERVED_IPC_IPC3_MAILBOX_EXT_INFO_START    (0x1480U)

#define GSM_RESERVED_HW_ID_OFFSET                       (0x1680U)
#define GSM_RESERVED_HW_ID_SIZE                         (4)

// GSM 8 x86 shared pinlocks
#define GSM_RESERVED_PREDEF_CVCORE_LOCKS_X86            (0x1684U)
#define GSM_RESERVED_PREDEF_CVCORE_LOCKS_X86_SIZE       (4)
#define GSM_PREDEF_LCLK_STATE_LOCK_NIBBLE               (0)
#define GSM_PREDEF_FCLK_STATE_LOCK_NIBBLE               (1)
#define GSM_PREDEF_UNUSED_LOCK_2_NIBBLE                 (2)
#define GSM_PREDEF_UNUSED_LOCK_3_NIBBLE                 (3)
#define GSM_PREDEF_UNUSED_LOCK_4_NIBBLE                 (4)
#define GSM_PREDEF_UNUSED_LOCK_5_NIBBLE                 (5)
#define GSM_PREDEF_UNUSED_LOCK_6_NIBBLE                 (6)
#define GSM_PREDEF_UNUSED_LOCK_7_NIBBLE                 (7)

// GSM lclk state
#define GSM_RESERVED_LCLK_STATE                         (0x1688U)
#define GSM_RESERVED_LCLK_STATE_SIZE                    (8)

// GSM fclk state
#define GSM_RESERVED_FCLK_STATE                         (0x1690U)
#define GSM_RESERVED_FCLK_STATE_SIZE                    (8)

// qpad registry memory
#define GSM_RESERVED_REGISTRY_QPAD                      (0x02000U)
#define GSM_QPAD_REGISTRY_SIZE                          (8 * 1024) // 8KB
// zpad registry memory
#define GSM_RESERVED_REGISTRY_ZPAD                      (0x04000U)
#define GSM_ZPAD_REGISTRY_SIZE                          (16 * 1024) // 16KB

// CVIP spinlock
#define GSM_RESERVED_CVIP_MUTEX_BITMAP_BASE             (0x0A088U)
#define GSM_MAX_NUM_CVIP_MUTEX                          (512)

#define GSM_RESERVED_CVIP_MUTEX_NIBBLE_BASE             (0x0A0C8U)

// GSM spinlock
#define GSM_RESERVED_CVIP_SPINLOCK_BITMAP_BASE          (0x0A1C8U)
#define GSM_MAX_NUM_CVIP_SPINLOCK                       (512)

#define GSM_RESERVED_CVIP_SPINLOCK_NIBBLE_BASE          (0x0A208U)

#define GSM_RESERVED_TEST_MEMORY                        (0x0A7E0U)
#define GSM_TEST_MEMORY_SIZE                            (GSM_MSG_NUM_CORE_IDS * 4)

// Statically reserved spinlocks
#define GSM_RESERVED_PREDEF_CVCORE_LOCKS_1              (0x0B830U)

#define GSM_RESERVED_PREDEF_CVCORE_LOCKS_2              (0x0B834U)

#define GSM_PREDEF_CVCORE_LOCKS_SIZE                    (8)

// GSM based integer (atomic int) memory start
#define GSM_RESERVED_CVIP_INTEGER_BITMAP_BASE           ((GSM_RESERVED_PREDEF_CVCORE_LOCKS_1 + GSM_PREDEF_CVCORE_LOCKS_SIZE + 4) & ~0x3)
#define GSM_MAX_NUM_CVIP_INTEGERS                       (256)

// TODO(KIrick): Remove this temporary test space for work queue job sharing without shregion
#define GSM_RESERVED_WORK_QUEUE_TEST_SPACE              (0x0C030U)
#define GSM_WORK_QUEUE_TEST_SIZE                        (0x400)

// State information for IPC2. IPC3 is shared with x86 and therefore its state information is
// located in the x86<->cvip shared gsm memory region
#define GSM_IPC2_GLOBAL_EXT_INFO_SIZE                   (GSM_IPC_GLOBAL_EXT_INFO_SIZE)
#define GSM_RESERVED_IPC_IPC2_GLOBAL_EXT_INFO_START     (0x0C000U)
#define GSM_IPC2_MAILBOX_EXT_INFO_SIZE                  (GSM_IPC_MAILBOX_EXT_INFO_SIZE * GSM_IPC_NUM_MAILBOXES)
#define GSM_RESERVED_IPC_IPC2_MAILBOX_EXT_INFO_START    (0x0C020U)

// GSM PL330_DMAC channel allocator word
#define GSM_RESERVED_DMAC_CHANNEL_ALLOC_BITMAP_ADDR     (0x0C440)
#define GSM_DMAC_CHANNEL_ALLOC_BITMAP_SIZE              (4)
#define GSM_DMAC_CHANNEL_ALLOC_INIT_MASK                (0xFFFFFF00)

// GSM PL330_DMAC patchset descriptor buffer allocator word
#define GSM_RESERVED_PATCHSET_BUF_ALLOC_BITMAP_ADDR     (0x0C648U)
#define GSM_DMAC_PATCHSET_ALLOC_BITMAP_SIZE             (4)
#define GSM_DMAC_PATCHSET_ALLOC_INIT_MASK               (0xFFFFFFFC)

// GSM DSP KPI buffer
#define GSM_RESERVED_DSP_KPI_MEM_BASE                   (0x40000U)
#define GSM_RESERVED_DSP_KPI_SIZE                       (64 * 1024)

// GSM based trace context start
#define GSM_MAX_NUM_CVCORE_TRACERS                      (256)

#define GSM_RESERVED_CVCORE_TRACE_BITMAP_BASE           (0x50000U)
#define GSM_CVCORE_TRACE_BITMAP_TABLE_SIZE              ((GSM_MAX_NUM_CVCORE_TRACERS / 32) * 4)

#define GSM_RESERVED_CVCORE_TRACE_ENABLE_BITMAP_BASE    (GSM_RESERVED_CVCORE_TRACE_BITMAP_BASE + GSM_CVCORE_TRACE_BITMAP_TABLE_SIZE)
#define GSM_CVCORE_TRACE_ENABLE_BITMAP_TABLE_SIZE       ((GSM_MAX_NUM_CVCORE_TRACERS / 32) * 4)

#define GSM_CVCORE_TRACE_DESC_SIZE                      (16)
#define GSM_CVCORE_TRACE_DESC_AREA_SIZE                 (GSM_MAX_NUM_CVCORE_TRACERS * GSM_CVCORE_TRACE_DESC_SIZE)
#define GSM_RESERVED_CVCORE_TRACE_DESC_BASE             (GSM_RESERVED_CVCORE_TRACE_ENABLE_BITMAP_BASE + GSM_CVCORE_TRACE_ENABLE_BITMAP_TABLE_SIZE)

// GSM based early trace context start
#define GSM_MAX_NUM_EARLY_TRACERS						(32)
#define GSM_NUM_EARLY_TRACERS							(2)
#define GSM_NUM_EARLY_TRACERS_BITMASK					((1 << GSM_NUM_EARLY_TRACERS) - 1)
#define GSM_RESERVED_EARLY_TRACE_BITMAP_BASE			(0x52000U)

#define GSM_EARLY_TRACE_BITMAP_TABLE_SIZE				((GSM_MAX_NUM_EARLY_TRACERS / 32) * 4)
#define GSM_RESERVED_EARLY_TRACE_ENABLE_BITMAP_BASE		(GSM_RESERVED_EARLY_TRACE_BITMAP_BASE + GSM_EARLY_TRACE_BITMAP_TABLE_SIZE)
#define GSM_EARLY_TRACE_ENABLE_BITMAP_TABLE_SIZE		((GSM_MAX_NUM_EARLY_TRACERS / 32) * 4)
#define GSM_EARLY_TRACE_DESC_SIZE						(16)
#define GSM_EARLY_TRACE_DESC_AREA_SIZE					(GSM_NUM_EARLY_TRACERS * GSM_EARLY_TRACE_DESC_SIZE)
#define GSM_RESERVED_EARLY_TRACE_DESC_BASE				(GSM_RESERVED_EARLY_TRACE_ENABLE_BITMAP_BASE + GSM_EARLY_TRACE_ENABLE_BITMAP_TABLE_SIZE)

// GSM DSP clock frequency config start
#define GSM_RESERVED_DSP_CLK_CONFIG                     (0x0C430U)
#define GSM_RESERVED_DSP_CLK_CONFIG_SIZE                (12) // Must be a multiple of 4 for init

#define GSM_MSG_NUM_CORE_IDS                            (9)  // 8 DSPs plus 1 for ARM
#define GSM_MSG_MAX_CORE_ID                             (GSM_MSG_NUM_CORE_IDS - 1)  // core_id is zero-based

// GSM Early Trace Buffer
#define GSM_RESERVED_EARLY_TRACE_BUF_BASE               (0x5DC00)
#define GSM_EARLY_TRACE_MAX_BUF_SIZE				    (4 * 1024) // 4 KB
#define GSM_EARLY_TRACE_BUF_AREA_SIZE                   (GSM_NUM_EARLY_TRACERS * GSM_EARLY_TRACE_MAX_BUF_SIZE)

// GSM based soft semaphore memory start
#define GSM_RESERVED_GSM_SEM_BASE                       ((GSM_RESERVED_TEST_MEMORY + GSM_TEST_MEMORY_SIZE) + (0x10 - ((GSM_RESERVED_TEST_MEMORY + GSM_TEST_MEMORY_SIZE) & 0xF)))

#define GSM_MAX_NUM_CVIP_GSM_SEM                        (256)
#define GSM_CTRL_WORD_SIZE                              (32)
#define GSM_SEM_CTRL_WORD_COUNT                         (GSM_MAX_NUM_CVIP_GSM_SEM / GSM_CTRL_WORD_SIZE) // divide 32
#define GSM_SEM_START                                   (GSM_RESERVED_GSM_SEM_BASE + (GSM_SEM_CTRL_WORD_COUNT * 4))
#define GSM_SEM_SIZE                                    (16)
#define GSM_SEM_TOTAL_SIZE                              (GSM_SEM_SIZE * GSM_MAX_NUM_CVIP_GSM_SEM) + (GSM_SEM_CTRL_WORD_COUNT * 4)

#define GSM_RESERVED_GSM_SEM_TEST_MEMORY                (0x5FC00U)

// DSP panic recovery KPI
#define GSM_RESERVED_DSP_PANIC_RECOVERY_STATS           (0x5FE00U)
#define GSM_DSP_PANIC_RECOVERY_STATS_SIZE               (256)

extern uint8_t *gsm_backing;

// GSM spinlock defines
#define SPINLOCK_CTRL_BITS_PER_WORD                     (32)
#define SPINLOCK_NIBBLES_PER_WORD                       (8)
#define SPINLOCK_CTRL_WORDS                             (GSM_MAX_NUM_CVIP_SPINLOCK / SPINLOCK_CTRL_BITS_PER_WORD)
#define SPINLOCK_NIBBLE_WORDS                           (GSM_MAX_NUM_CVIP_SPINLOCK / SPINLOCK_NIBBLES_PER_WORD)
#define SPINLOCK_NIBBLE_WORD_SIZE                       (sizeof(uint32_t))
#define SPINLOCK_CTRL_WORD_SIZE                         (sizeof(uint32_t))

// CVIP spinlock defines, see cvip_spinlock_core_internal.h
#define TAS_CTRL_BITS_PER_WORD                          (32)
#define TAS_NIBBLES_PER_WORD                            (8)
#define TAS_NIBBLES_PER_BYTE                            (2)
#define TAS_CTRL_WORDS                                  (GSM_MAX_NUM_CVIP_MUTEX / TAS_CTRL_BITS_PER_WORD)
#define TAS_NIBBLE_WORDS                                (GSM_MAX_NUM_CVIP_MUTEX / TAS_NIBBLES_PER_WORD)
#define TAS_CTRL_WORD_SIZE                              (sizeof(uint32_t))
#define TAS_NIBBLE_WORD_SIZE                            (sizeof(uint32_t))

// CVIP integer defines, see cvip_integer_core_internal.h
#define CVIP_INTEGER_NUM_CTRL_BITS_PER_DATA_WORD        (1)
#define CVIP_INTEGER_NUM_BITS_PER_CTRL_WORD             (32)
#define CVIP_INTEGER_CTRL_WORDS                                                 \
    ((GSM_MAX_NUM_CVIP_INTEGERS / CVIP_INTEGER_NUM_CTRL_BITS_PER_DATA_WORD) /   \
    CVIP_INTEGER_NUM_BITS_PER_CTRL_WORD)
#define CVIP_INTEGER_DATA_WORDS                         (GSM_MAX_NUM_CVIP_INTEGER)
#define CVIP_INTEGER_CTRL_WORD_SIZE                     (sizeof(uint32_t))
#define CVIP_INTEGER_DATA_WORD_SIZE                     (sizeof(uint32_t))

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_H */

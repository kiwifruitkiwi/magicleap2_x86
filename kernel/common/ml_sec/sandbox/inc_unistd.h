//#ifndef __MLSEC_SBOX_INC_UNISTD_H
//#define __MLSEC_SBOX_INC_UNISTD_H

#ifdef CONFIG_ARM64
#include <asm/unistd_ext.h>
#else // CONFIG_X86_64


#ifndef __SYSCALL_64
#include <asm/asm-offsets.h>  // for __NR_syscall_max
struct pt_regs;
#endif

// generated to out/target/product/acamas/obj/kernel/arch/x86/include/generated/uapi/asm/unistd_64.h
//  that's generated from  kernel/arch/x86/entry/syscalls/syscall_64.tbl
//#include <asm/unistd_64.h>
//#define __NR_rseq 334 // hack
//#define __X_NR_syscalls (__NR_rseq + 1) // TBD __NR_syscall_max this is a hack, maybe to it with a script?
#define __X_NR_syscalls NR_syscalls
#define __X_SYSCALL_NR_TO_SERIAL(nr)	(nr)

// /data/ml2/kernel/arch/x86/include/uapi/asm/unistd.h
//#include <uapi/asm/unistd.h>
#ifndef __SYSCALL_64
#define __SYSCALL_64(nr, sym, qual) extern /*asmlinkage*/ long sym(const struct pt_regs *);
#endif
#include <asm/syscalls_64.h>
#undef __SYSCALL_64

#endif

//#endif //__MLSEC_SBOX_INC_UNISTD_H

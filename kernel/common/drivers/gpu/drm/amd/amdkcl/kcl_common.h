#ifndef AMDKCL_COMMON_H
#define AMDKCL_COMMON_H

/*
 * kallsyms_lookup_name has been exported in version 2.6.33
 */
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/bug.h>

#ifndef HAVE_KALLSYMS_LOOKUP_NAME
extern unsigned long (*_kcl_kallsyms_lookup_name)(const char *name);
#endif
static inline unsigned long kcl_kallsyms_lookup_name(const char *name)
{
#ifndef HAVE_KALLSYMS_LOOKUP_NAME
	return _kcl_kallsyms_lookup_name(name);
#else
	return kallsyms_lookup_name(name);
#endif
}

static inline void *amdkcl_fp_setup(const char *symbol, void *fp_stup)
{
	unsigned long addr;
	void *fp = NULL;

	addr = kcl_kallsyms_lookup_name(symbol);
	if (addr == 0) {
		fp = fp_stup;
		if (fp != NULL)
			printk_once(KERN_WARNING "Warning: fail to get symbol %s, replace it with kcl stub\n", symbol);
		else {
			printk_once(KERN_ERR "Error: fail to get symbol %s\n", symbol);
			BUG();
		}
	} else {
		fp = (void *)addr;
	}

	return fp;
}
#endif

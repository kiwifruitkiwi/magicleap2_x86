/*
 * Copyright (C) 2010 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __CVIP_SYSCAHCE_DEBUG_FS
#define __CVIP_SYSCAHCE_DEBUG_FS

#ifdef CONFIG_DEBUG_FS
int syscache_debugfs_create(void __iomem **base);
void syscache_debugfs_remove(void);
#else
static inline int syscache_debugfs_create(void __iomem **base)
{
	return 0;
}

static inline void syscache_debugfs_remove(void)
{
}
#endif
#endif


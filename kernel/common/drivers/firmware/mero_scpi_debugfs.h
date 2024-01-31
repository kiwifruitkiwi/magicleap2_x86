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

#ifndef __MERO_SCPI_DEBUGFS_H__
#define __MERO_SCPI_DEBUGFS_H__

#if IS_REACHABLE(CONFIG_DEBUG_FS)
int mero_scpi_debugfs_init(struct scpi_drvinfo *scpi_info);
void mero_scpi_debugfs_exit(void);
#else
static inline int mero_scpi_debugfs_init(struct scpi_drvinfo *scpi_info)
{
	return 0;
}

static inline void mero_scpi_debugfs_exit(void) { }
#endif

#endif

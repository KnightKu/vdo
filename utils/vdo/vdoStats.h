/*
 * Copyright Red Hat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 */

#ifndef VDO_STATS_H
#define VDO_STATS_H

#include "types.h"

/**
 * Read vdo statistics from a buffer
 *
 * @param buf     pointer to the buffer
 * @param stats   pointer to the statistics
 *
 * @return VDO_SUCCESS or an error
 */
int read_vdo_stats(char *buf, struct vdo_statistics *stats);

/**
 * Write vdo statistics to stdout
 *
 * @param stats   pointer to the statistics
 *
 * @return VDO_SUCCESS or an error
 */
int vdo_write_stats(struct vdo_statistics *stats);

#endif  /* VDO_STATS_H */

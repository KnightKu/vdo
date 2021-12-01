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

#ifndef COMMON_H
#define COMMON_H

#include "string-utils.h"
#include "type-defs.h"
#include "uds.h"

enum {
	KILOBYTE = 1024,
	MEGABYTE = KILOBYTE * KILOBYTE,
	GIGABYTE = KILOBYTE * MEGABYTE
};

struct uds_chunk_data;

struct uds_chunk_record {
	struct uds_chunk_name name;
	struct uds_chunk_data data;
};

#endif /* COMMON_H */

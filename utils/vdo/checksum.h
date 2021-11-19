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

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <zlib.h>

/**
 * A CRC-32 checksum
 **/
typedef uint32_t crc32_checksum_t;

static const crc32_checksum_t VDO_INITIAL_CHECKSUM = 0xffffffff;

enum {
	/* The size of a CRC-32 checksum */
	VDO_CHECKSUM_SIZE = sizeof(crc32_checksum_t),
};

/**
 * A function to update a running CRC-32 checksum.
 *
 * @param crc     The current value of the crc
 * @param buffer  The data to add to the checksum
 * @param length  The length of the data
 *
 * @return The updated value of the checksum
 **/
static inline crc32_checksum_t vdo_update_crc32(crc32_checksum_t crc,
						const byte *buffer,
						size_t length)
{
	return crc32(crc, buffer, length);
}

#endif /* CHECKSUM_H */

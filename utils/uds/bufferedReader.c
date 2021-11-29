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

#include "bufferedReader.h"

#include "compiler.h"
#include "ioFactory.h"
#include "logger.h"
#include "memoryAlloc.h"
#include "numeric.h"

/*
 * Define sector_t.  The kernel really wants us to use it.  The code becomes
 * ugly if we need to #ifdef every usage of sector_t.  Note that the of #define
 * means that even if a user mode include typedefs sector_t, it will not affect
 * this module.
 */
#define sector_t uint64_t

struct buffered_reader {
	/* Region to read from */
	struct io_region *br_region;
	/* Number of the current block */
	uint64_t br_block_number;
	/* Start of the buffer */
	byte *br_start;
	/* End of the data read from the buffer */
	byte *br_pointer;
};


/**********************************************************************/
int make_buffered_reader(struct io_region *region,
			 struct buffered_reader **reader_ptr)
{
	byte *data;
	int result = UDS_ALLOCATE_IO_ALIGNED(
		UDS_BLOCK_SIZE, byte, "buffer writer buffer", &data);
	if (result != UDS_SUCCESS) {
		return result;
	}

	struct buffered_reader *reader = NULL;
	result =
		UDS_ALLOCATE(1, struct buffered_reader, "buffered reader",
			     &reader);
	if (result != UDS_SUCCESS) {
		UDS_FREE(data);
		return result;
	}

	*reader = (struct buffered_reader){
		.br_region = region,
		.br_block_number = 0,
		.br_start = data,
		.br_pointer = NULL,
	};

	get_io_region(region);
	*reader_ptr = reader;
	return UDS_SUCCESS;
}

/**********************************************************************/
void free_buffered_reader(struct buffered_reader *br)
{
	if (br == NULL) {
		return;
	}
	put_io_region(br->br_region);
	UDS_FREE(br->br_start);
	UDS_FREE(br);
}

/**********************************************************************/
static int
position_reader(struct buffered_reader *br, sector_t block_number, off_t offset)
{
	if ((br->br_pointer == NULL) || (block_number != br->br_block_number)) {
		int result = read_from_region(br->br_region,
					      block_number * UDS_BLOCK_SIZE,
					      br->br_start,
					      UDS_BLOCK_SIZE,
					      NULL);
		if (result != UDS_SUCCESS) {
			uds_log_warning_strerror(
				result,
				"%s got read_from_region error",
				__func__);
			return result;
		}
	}
	br->br_block_number = block_number;
	br->br_pointer = br->br_start + offset;
	return UDS_SUCCESS;
}

/**********************************************************************/
static size_t bytes_remaining_in_read_buffer(struct buffered_reader *br)
{
	return (br->br_pointer == NULL ?
			0 :
			br->br_start + UDS_BLOCK_SIZE - br->br_pointer);
}

/**********************************************************************/
static int reset_reader(struct buffered_reader *br)
{
	sector_t block_number;

	if (bytes_remaining_in_read_buffer(br) > 0) {
		return UDS_SUCCESS;
	}

	block_number = br->br_block_number;
	if (br->br_pointer != NULL) {
		++block_number;
	}

	return position_reader(br, block_number, 0);
}

/**********************************************************************/
int read_from_buffered_reader(struct buffered_reader *br,
			      void *data,
			      size_t length)
{
	byte *dp = data;
	int result = UDS_SUCCESS;
	size_t avail, chunk;
	while (length > 0) {
		result = reset_reader(br);
		if (result != UDS_SUCCESS) {
			break;
		}

		avail = bytes_remaining_in_read_buffer(br);
		chunk = min(length, avail);
		memcpy(dp, br->br_pointer, chunk);
		length -= chunk;
		dp += chunk;
		br->br_pointer += chunk;
	}

	if (((result == UDS_OUT_OF_RANGE) || (result == UDS_END_OF_FILE)) &&
	    (dp - (byte *) data > 0)) {
		result = UDS_SHORT_READ;
	}
	return result;
}

/**********************************************************************/
int verify_buffered_data(struct buffered_reader *br,
			 const void *value,
			 size_t length)
{
	int result = UDS_SUCCESS;
	size_t avail,chunk;
	const byte *vp = value;
	sector_t starting_block_number = br->br_block_number;
	int starting_offset = br->br_pointer - br->br_start;
	while (length > 0) {
		result = reset_reader(br);
		if (result != UDS_SUCCESS) {
			result = UDS_CORRUPT_FILE;
			break;
		}

		avail = bytes_remaining_in_read_buffer(br);
		chunk = min(length, avail);
		if (memcmp(vp, br->br_pointer, chunk) != 0) {
			result = UDS_CORRUPT_FILE;
			break;
		}
		length -= chunk;
		vp += chunk;
		br->br_pointer += chunk;
	}

	if (result != UDS_SUCCESS) {
		position_reader(br, starting_block_number, starting_offset);
	}

	return result;
}

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

#ifndef BUFFERED_WRITER_H
#define BUFFERED_WRITER_H 1

#include "common.h"

struct io_region;

struct buffered_writer;

/**
 * Make a new buffered writer.
 *
 * @param region        The IO region to write to.
 * @param writer_ptr    The new buffered writer goes here.
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check make_buffered_writer(struct io_region *region,
				      struct buffered_writer **writer_ptr);

/**
 * Free a buffered writer, without flushing.
 *
 * @param [in] buffer   The buffered writer object.
 **/
void free_buffered_writer(struct buffered_writer *buffer);

/**
 * Append data to buffer, writing as needed.
 *
 * @param buffer        The buffered writer object.
 * @param data          The data to write.
 * @param len           The length of the data written.
 *
 * @return              UDS_SUCCESS or an error code.
 *                      The error may reflect previous attempts to write
 *                      or flush the buffer.  Once a write or flush error
 *                      occurs it is sticky.
 **/
int __must_check write_to_buffered_writer(struct buffered_writer *buffer,
					  const void *data,
					  size_t len);

/**
 * Zero data in the buffer, writing as needed.
 *
 * @param bw            The buffered writer object.
 * @param len           The number of zero bytes to write.
 *
 * @return              UDS_SUCCESS or an error code.
 *                      The error may reflect previous attempts to write
 *                      or flush the buffer.  Once a write or flush error
 *                      occurs it is sticky.
 **/
int __must_check write_zeros_to_buffered_writer(struct buffered_writer *bw,
						size_t len);

/**
 * Flush any partial data from the buffer.
 *
 * @param buffer        The buffered writer object.
 *
 * @return              UDS_SUCCESS or an error code.
 *                      The error may reflect previous attempts to write
 *                      or flush the buffer.  Once a write or flush error
 *                      occurs it is sticky.
 **/
int __must_check flush_buffered_writer(struct buffered_writer *buffer);

#endif /* BUFFERED_WRITER_H */

/*
 * Copyright (c) 2020 Red Hat, Inc.
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
 *
 * $Id: //eng/linux-vdo/src/c++/vdo/base/vio.h#18 $
 */

#ifndef VIO_H
#define VIO_H

#include <stdarg.h>

#include "completion.h"
#include "trace.h"
#include "types.h"
#include "vdo.h"

/**
 * A representation of a single block which may be passed between the VDO base
 * and the physical layer.
 **/
struct vio {
	/* The completion for this vio */
	struct vdo_completion completion;

	/* The functions to call when this vio's operation is complete */
	vdo_action *callback;
	vdo_action *error_handler;

	/* The vdo handling this vio */
	struct vdo *vdo;

	/* The address on the underlying device of the block to be read/written
	 */
	physical_block_number_t physical;

	/* The type of request this vio is servicing */
	vio_operation operation;

	/* The queueing priority of the vio operation */
	vio_priority priority;

	/* The vio type is used for statistics and instrumentation. */
	vio_type type;

	/* Used for logging and debugging */
	struct trace *trace;
};

/**
 * Convert a generic vdo_completion to a vio.
 *
 * @param completion  The completion to convert
 *
 * @return The completion as a vio
 **/
static inline struct vio *as_vio(struct vdo_completion *completion)
{
	assert_completion_type(completion->type, VIO_COMPLETION);
	return container_of(completion, struct vio, completion);
}

/**
 * Convert a vio to a generic completion.
 *
 * @param vio The vio to convert
 *
 * @return The vio as a completion
 **/
static inline struct vdo_completion *vio_as_completion(struct vio *vio)
{
	return &vio->completion;
}

/**
 * Create a vio.
 *
 * @param [in]  layer      The physical layer
 * @param [in]  vio_type   The type of vio to create
 * @param [in]  priority   The relative priority to assign to the vio
 * @param [in]  parent     The parent of the vio
 * @param [in]  data       The buffer
 * @param [out] vio_ptr    A pointer to hold the new vio
 *
 * @return VDO_SUCCESS or an error
 **/
static inline int create_vio(PhysicalLayer *layer,
			     vio_type vio_type,
			     vio_priority priority,
			     void *parent,
			     char *data,
			     struct vio **vio_ptr)
{
	return layer->createMetadataVIO(layer, vio_type, priority, parent,
					data, vio_ptr);
}

/**
 * Destroy a vio. The pointer to the vio will be nulled out.
 *
 * @param vio_ptr  A pointer to the vio to destroy
 **/
void free_vio(struct vio **vio_ptr);

/**
 * Initialize a vio.
 *
 * @param vio       The vio to initialize
 * @param type      The vio type
 * @param priority  The relative priority of the vio
 * @param parent    The parent (the extent completion) to assign to the vio
 *                  completion
 * @param vdo       The vdo for this vio
 * @param layer     The layer for this vio
 **/
void initialize_vio(struct vio *vio,
		    vio_type type,
		    vio_priority priority,
		    struct vdo_completion *parent,
		    struct vdo *vdo,
		    PhysicalLayer *layer);

/**
 * The very last step in processing a vio. Set the vio's completion's callback
 * and error handler from the fields set in the vio itself on launch and then
 * actually complete the vio's completion.
 *
 * @param completion  The vio
 **/
void vio_done_callback(struct vdo_completion *completion);

/**
 * Get the name of a vio's operation.
 *
 * @param vio  The vio
 *
 * @return The name of the vio's operation (read, write, or read-modify-write)
 **/
const char *get_vio_read_write_flavor(const struct vio *vio);

/**
 * Update per-vio error stats and log the error.
 *
 * @param vio     The vio which got an error
 * @param format  The format of the message to log (a printf style format)
 **/
void update_vio_error_stats(struct vio *vio, const char *format, ...)
	__attribute__((format(printf, 2, 3)));

/**
 * Add a trace record for the current source location.
 *
 * @param vio      The vio structure to be updated
 * @param location The source-location descriptor to be recorded
 **/
static inline void vio_add_trace_record(struct vio *vio,
					const struct trace_location *location)
{
	if (unlikely(vio->trace != NULL)) {
		add_trace_record(vio->trace, location);
	}
}

/**
 * Check whether a vio is servicing an external data request.
 *
 * @param vio  The vio to check
 **/
static inline bool is_data_vio(struct vio *vio)
{
	return is_data_vio_type(vio->type);
}

/**
 * Check whether a vio is for compressed block writes
 *
 * @param vio  The vio to check
 **/
static inline bool is_compressed_write_vio(struct vio *vio)
{
	return is_compressed_write_vio_type(vio->type);
}

/**
 * Check whether a vio is for metadata
 *
 * @param vio  The vio to check
 **/
static inline bool is_metadata_vio(struct vio *vio)
{
	return is_metadata_vio_type(vio->type);
}

/**
 * Check whether a vio is a read.
 *
 * @param vio  The vio
 *
 * @return <code>true</code> if the vio is a read
 **/
static inline bool is_read_vio(const struct vio *vio)
{
	return ((vio->operation & VIO_READ_WRITE_MASK) == VIO_READ);
}

/**
 * Check whether a vio is a read-modify-write.
 *
 * @param vio  The vio
 *
 * @return <code>true</code> if the vio is a read-modify-write
 **/
static inline bool is_read_modify_write_vio(const struct vio *vio)
{
	return ((vio->operation & VIO_READ_WRITE_MASK) ==
		VIO_READ_MODIFY_WRITE);
}

/**
 * Check whether a vio is a write.
 *
 * @param vio  The vio
 *
 * @return <code>true</code> if the vio is a write
 **/
static inline bool is_write_vio(const struct vio *vio)
{
	return ((vio->operation & VIO_READ_WRITE_MASK) == VIO_WRITE);
}

/**
 * Check whether a vio requires a flush before doing its I/O.
 *
 * @param vio  The vio
 *
 * @return <code>true</code> if the vio requires a flush before
 **/
static inline bool vio_requires_flush_before(const struct vio *vio)
{
	return ((vio->operation & VIO_FLUSH_BEFORE) == VIO_FLUSH_BEFORE);
}

/**
 * Check whether a vio requires a flush after doing its I/O.
 *
 * @param vio  The vio
 *
 * @return <code>true</code> if the vio requires a flush after
 **/
static inline bool vio_requires_flush_after(const struct vio *vio)
{
	return ((vio->operation & VIO_FLUSH_AFTER) == VIO_FLUSH_AFTER);
}

/**
 * Launch a metadata vio.
 *
 * @param vio            The vio to launch
 * @param physical       The physical block number to read or write
 * @param callback       The function to call when the vio completes its I/O
 * @param error_handler  The handler for write errors
 * @param operation      The operation to perform (read or write)
 **/
void launch_metadata_vio(struct vio *vio,
			 physical_block_number_t physical,
			 vdo_action *callback,
			 vdo_action *error_handler,
			 vio_operation operation);

/**
 * Launch a metadata read vio.
 *
 * @param vio            The vio to launch
 * @param physical       The physical block number to read
 * @param callback       The function to call when the vio completes its read
 * @param error_handler  The handler for write errors
 **/
static inline void launch_read_metadata_vio(struct vio *vio,
					    physical_block_number_t physical,
					    vdo_action *callback,
					    vdo_action *error_handler)
{
	launch_metadata_vio(vio, physical, callback, error_handler, VIO_READ);
}

/**
 * Launch a metadata write vio.
 *
 * @param vio            The vio to launch
 * @param physical       The physical block number to write
 * @param callback       The function to call when the vio completes its write
 * @param error_handler  The handler for write errors
 **/
static inline void launch_write_metadata_vio(struct vio *vio,
					     physical_block_number_t physical,
					     vdo_action *callback,
					     vdo_action *error_handler)
{
	launch_metadata_vio(vio, physical, callback, error_handler, VIO_WRITE);
}

/**
 * Launch a metadata write vio optionally flushing the layer before and/or
 * after the write operation.
 *
 * @param vio           The vio to launch
 * @param physical      The physical block number to write
 * @param callback      The function to call when the vio completes its
 *                      operation
 * @param error_handler The handler for flush or write errors
 * @param flush_before  Whether or not to flush before writing
 * @param flush_after   Whether or not to flush after writing
 **/
static inline void
launch_write_metadata_vio_with_flush(struct vio *vio,
				     physical_block_number_t physical,
				     vdo_action *callback,
				     vdo_action *error_handler,
				     bool flush_before,
				     bool flush_after)
{
	launch_metadata_vio(vio,
			    physical,
			    callback,
			    error_handler,
			    (VIO_WRITE | (flush_before ? VIO_FLUSH_BEFORE : 0) |
			     (flush_after ? VIO_FLUSH_AFTER : 0)));
}

/**
 * Issue a flush to the layer. Currently expected to be used only in
 * async mode.
 *
 * @param vio            The vio to notify when the flush is complete
 * @param callback       The function to call when the flush is complete
 * @param error_handler  The handler for flush errors
 **/
void launch_flush(struct vio *vio,
		  vdo_action *callback,
		  vdo_action *error_handler);

#endif // VIO_H
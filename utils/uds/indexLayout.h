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

#ifndef INDEX_LAYOUT_H
#define INDEX_LAYOUT_H

#include "buffer.h"
#include "config.h"
#include "indexState.h"
#include "ioFactory.h"
#include "uds.h"

struct index_layout;

/**
 * Construct an index layout.
 *
 * @param config      The configuration required for a new layout.
 * @param new_layout  Whether this is a new layout.
 * @param layout_ptr  Where to store the new index layout
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check make_uds_index_layout(struct configuration *config,
				       bool new_layout,
				       struct index_layout **layout_ptr);

/**
 * Free an index layout.
 *
 * @param layout  The layout to free
 **/
void free_uds_index_layout(struct index_layout *layout);

/**********************************************************************/
int __must_check cancel_uds_index_save(struct index_layout *layout,
				       unsigned int save_slot);

/**********************************************************************/
int __must_check commit_uds_index_save(struct index_layout *layout,
				       unsigned int save_slot);

/**********************************************************************/
int __must_check discard_uds_index_saves(struct index_layout *layout);

/**
 * Find the latest index save slot.
 *
 * @param [in]  layout          The single file layout.
 * @param [out] num_zones_ptr   Where to store the actual number of zones
 *                                that were saved.
 * @param [out] slot_ptr        Where to store the slot number we found.
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check find_latest_uds_index_save_slot(struct index_layout *layout,
						 unsigned int *num_zones_ptr,
						 unsigned int *slot_ptr);

/**
 * Open a buffered reader for a specified state, kind, and zone.
 *
 * @param layout      The index layout
 * @param slot        The save slot
 * @param kind        The kind of index save region to open.
 * @param zone        The zone number for the region.
 * @param reader_ptr  Where to store the buffered reader.
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check
open_uds_index_buffered_reader(struct index_layout *layout,
			       unsigned int slot,
			       enum region_kind kind,
			       unsigned int zone,
			       struct buffered_reader **reader_ptr);

/**
 * Open a buffered writer for a specified state, kind, and zone.
 *
 * @param layout      The index layout
 * @param slot        The save slot
 * @param kind        The kind of index save region to open.
 * @param zone        The zone number for the region.
 * @param writer_ptr  Where to store the buffered writer.
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check
open_uds_index_buffered_writer(struct index_layout *layout,
			       unsigned int slot,
			       enum region_kind kind,
			       unsigned int zone,
			       struct buffered_writer **writer_ptr);

/**
 * Obtain the nonce to be used to store or validate the loading of volume index
 * pages.
 *
 * @param [in]  layout   The index layout.
 *
 * @return The nonce to use.
 **/
uint64_t __must_check get_uds_volume_nonce(struct index_layout *layout);

/**
 * Obtain an IO region for the specified index volume.
 *
 * @param [in]  layout      The index layout.
 * @param [out] region_ptr  Where to put the new region.
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check open_uds_volume_region(struct index_layout *layout,
					struct io_region **region_ptr);

/**
 * Determine which index save slot to use for a new index save.
 *
 * Also allocates the volume index regions and the openChapter region.
 *
 * @param [in]  layout          The index layout.
 * @param [in]  num_zones       Actual number of zones currently in use.
 * @param [out] save_slot_ptr   Where to store the save slot number.
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check setup_uds_index_save_slot(struct index_layout *layout,
					   unsigned int num_zones,
					   unsigned int *save_slot_ptr);

/**
 * Get the index state buffer
 *
 * @param layout  the index layout
 * @param slot    the save slot
 *
 * @return UDS_SUCCESS or an error code
 **/
struct buffer *__must_check
get_uds_index_state_buffer(struct index_layout *layout, unsigned int slot);

#endif /* INDEX_LAYOUT_H */

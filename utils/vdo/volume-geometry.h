/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright Red Hat
 */

#ifndef VOLUME_GEOMETRY_H
#define VOLUME_GEOMETRY_H


#include <uuid/uuid.h>

#include "uds.h"

#include "physicalLayer.h"
#include "types.h"

enum {
	VDO_GEOMETRY_BLOCK_LOCATION = 0,
};

struct index_config {
	uint32_t mem;
	uint32_t unused;
	bool sparse;
} __packed;

enum volume_region_id {
	VDO_INDEX_REGION = 0,
	VDO_DATA_REGION = 1,
	VDO_VOLUME_REGION_COUNT,
};

struct volume_region {
	/** The ID of the region */
	enum volume_region_id id;
	/**
	 * The absolute starting offset on the device. The region continues
	 * until the next region begins.
	 */
	physical_block_number_t start_block;
} __packed;

struct volume_geometry {
	/** The release version number of this volume */
	release_version_number_t release_version;
	/** The nonce of this volume */
	nonce_t nonce;
	/** The uuid of this volume */
	uuid_t uuid;
	/** The block offset to be applied to bios */
	block_count_t bio_offset;
	/** The regions in ID order */
	struct volume_region regions[VDO_VOLUME_REGION_COUNT];
	/** The index config */
	struct index_config index_config;
} __packed;

/** This volume geometry struct is used for sizing only */
struct volume_geometry_4_0 {
	/** The release version number of this volume */
	release_version_number_t release_version;
	/** The nonce of this volume */
	nonce_t nonce;
	/** The uuid of this volume */
	uuid_t uuid;
	/** The regions in ID order */
	struct volume_region regions[VDO_VOLUME_REGION_COUNT];
	/** The index config */
	struct index_config index_config;
} __packed;

/**
 * Get the start of the index region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return The start of the index region
 **/
static inline physical_block_number_t __must_check
vdo_get_index_region_start(struct volume_geometry geometry)
{
	return geometry.regions[VDO_INDEX_REGION].start_block;
}

/**
 * Get the start of the data region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return The start of the data region
 **/
static inline physical_block_number_t __must_check
vdo_get_data_region_start(struct volume_geometry geometry)
{
	return geometry.regions[VDO_DATA_REGION].start_block;
}

/**
 * Get the size of the index region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return the size of the index region
 **/
static inline physical_block_number_t __must_check
vdo_get_index_region_size(struct volume_geometry geometry)
{
	return vdo_get_data_region_start(geometry) -
		vdo_get_index_region_start(geometry);
}

int __must_check
vdo_load_volume_geometry(PhysicalLayer *layer,
			 struct volume_geometry *geometry);

int __must_check
vdo_initialize_volume_geometry(nonce_t nonce,
			       uuid_t *uuid,
			       const struct index_config *index_config,
			       struct volume_geometry *geometry);

int __must_check vdo_clear_volume_geometry(PhysicalLayer *layer);

int __must_check
vdo_write_volume_geometry(PhysicalLayer *layer,
			  struct volume_geometry *geometry);

int __must_check
vdo_write_volume_geometry_with_version(PhysicalLayer *layer,
				       struct volume_geometry *geometry,
				       uint32_t version);

int __must_check vdo_compute_index_blocks(const struct index_config *index_config,
					  block_count_t *index_blocks_ptr);

#endif /* VOLUME_GEOMETRY_H */

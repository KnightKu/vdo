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
 *
 * $Id: //eng/vdo-releases/aluminum/src/c++/vdo/base/volumeGeometry.h#7 $
 */

#ifndef VOLUME_GEOMETRY_H
#define VOLUME_GEOMETRY_H

#include "uds.h"

#include "types.h"

struct indexConfig {
  uint32_t mem;
  uint32_t checkpointFrequency;
  bool     sparse;
} __attribute__((packed));

typedef enum {
  INDEX_REGION = 0,
  DATA_REGION  = 1,
  VOLUME_REGION_COUNT,
} VolumeRegionID;

typedef struct {
  /** The ID of the region */
  VolumeRegionID      id;
  /**
   * The absolute starting offset on the device. The region continues until
   * the next region begins.
   */
  PhysicalBlockNumber startBlock;
} __attribute__((packed)) VolumeRegion;

/** A binary UUID is 16 bytes. */
typedef unsigned char UUID[16];

typedef struct {
  /** The release version number of this volume */
  ReleaseVersionNumber releaseVersion;
  /** The nonce of this volume */
  Nonce                nonce;
  /** The UUID of this volume */
  UUID                 uuid;
  /** The block offset to be applied to bios */
  BlockCount           bioOffset;
  /** The regions in ID order */
  VolumeRegion         regions[VOLUME_REGION_COUNT];
  /** The index config */
  IndexConfig          indexConfig;
} __attribute__((packed)) VolumeGeometry;

/** This volume geometry struct is used for sizing only */
typedef struct {
  /** The release version number of this volume */
  ReleaseVersionNumber releaseVersion;
  /** The nonce of this volume */
  Nonce                nonce;
  /** The UUID of this volume */
  UUID                 uuid;
  /** The regions in ID order */
  VolumeRegion         regions[VOLUME_REGION_COUNT];
  /** The index config */
  IndexConfig          indexConfig;
} __attribute__((packed)) VolumeGeometry_4_0;

/**
 * Get the start of the index region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return The start of the index region
 **/
__attribute__((warn_unused_result))
static inline PhysicalBlockNumber getIndexRegionStart(VolumeGeometry geometry)
{
  return geometry.regions[INDEX_REGION].startBlock;
}

/**
 * Get the start of the data region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return The start of the data region
 **/
__attribute__((warn_unused_result))
static inline PhysicalBlockNumber getDataRegionStart(VolumeGeometry geometry)
{
  return geometry.regions[DATA_REGION].startBlock;
}

/**
 * Get the size of the index region from a geometry.
 *
 * @param geometry  The geometry
 *
 * @return the size of the index region
 **/
__attribute__((warn_unused_result))
static inline PhysicalBlockNumber getIndexRegionSize(VolumeGeometry geometry)
{
  return getDataRegionStart(geometry) - getIndexRegionStart(geometry);
}

/**
 * Read the volume geometry from a layer.
 *
 * @param layer     The layer to read and parse the geometry from
 * @param geometry  The geometry to be loaded
 **/
int loadVolumeGeometry(PhysicalLayer *layer, VolumeGeometry *geometry)
  __attribute__((warn_unused_result));

/**
 * Attempt to read the volume geometry from a layer at a specified block.
 *
 * @param layer       The layer to read and parse the geometry from
 * @param blockNumber The block at which to read
 * @param geometry    The geometry to be loaded
 **/
int loadVolumeGeometryAtBlock(PhysicalLayer        *layer,
                              PhysicalBlockNumber   blockNumber,
                              VolumeGeometry       *geometry)
  __attribute__((warn_unused_result));

/**
 * Initialize a VolumeGeometry for a VDO.
 *
 * @param nonce        The nonce for the VDO
 * @param uuid         The uuid for the VDO
 * @param indexConfig  The index config of the VDO
 * @param geometry     The geometry being initialized
 *
 * @return VDO_SUCCESS or an error
 **/
int initializeVolumeGeometry(Nonce           nonce,
                             UUID            uuid,
                             IndexConfig    *indexConfig,
                             VolumeGeometry *geometry)
  __attribute__((warn_unused_result));

/**
 * Zero out the geometry on a layer.
 *
 * @param layer  The layer to clear
 *
 * @return VDO_SUCCESS or an error
 **/
int clearVolumeGeometry(PhysicalLayer *layer)
  __attribute__((warn_unused_result));

/**
 * Write a geometry block for a VDO.
 *
 * @param layer     The layer on which to write
 * @param geometry  The VolumeGeometry to be written
 *
 * @return VDO_SUCCESS or an error
 **/
int writeVolumeGeometry(PhysicalLayer *layer, VolumeGeometry *geometry)
  __attribute__((warn_unused_result));

/**
 * Write a specific version of geometry block for a VDO.
 *
 * @param layer     The layer on which to write
 * @param geometry  The VolumeGeometry to be written
 * @param version   The version of VolumeGeometry to write
 *
 * @return VDO_SUCCESS or an error
 **/
int writeVolumeGeometryWithVersion(PhysicalLayer  *layer,
                                   VolumeGeometry *geometry,
                                   uint32_t        version)
  __attribute__((warn_unused_result));

/**
 * Convert an index config to a UDS configuration, which can be used by UDS.
 *
 * @param [in]  indexConfig   The index config to convert
 * @param [out] udsConfigPtr  A pointer to return the UDS configuration
 *
 * @return VDO_SUCCESS or an error
 **/
int indexConfigToUdsConfiguration(IndexConfig      *indexConfig,
                                  UdsConfiguration *udsConfigPtr)
  __attribute__((warn_unused_result));

/**
 * Modify the uds_parameters to match the requested index config.
 *
 * @param indexConfig  The index config to convert
 * @param userParams   The uds_parameters to modify
 **/
void indexConfigToUdsParameters(IndexConfig           *indexConfig,
                                struct uds_parameters *userParams);

/**
 * Compute the index size in blocks from the IndexConfig.
 *
 * @param [in]  indexConfig     The index config
 * @param [out] indexBlocksPtr  A pointer to return the index size in blocks
 *
 * @return VDO_SUCCESS or an error
 **/
int computeIndexBlocks(IndexConfig *indexConfig, BlockCount *indexBlocksPtr)
  __attribute__((warn_unused_result));

/**
 * Set load config fields from a volume geometry.
 *
 * @param [in]  geometry    The geometry to use
 * @param [out] loadConfig  The load config to set
 **/
static inline void setLoadConfigFromGeometry(VolumeGeometry *geometry,
                                             VDOLoadConfig  *loadConfig)
{
  loadConfig->firstBlockOffset = getDataRegionStart(*geometry);
  loadConfig->releaseVersion   = geometry->releaseVersion;
  loadConfig->nonce            = geometry->nonce;
}

#endif // VOLUME_GEOMETRY_H

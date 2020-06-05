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
 * $Id: //eng/linux-vdo/src/c++/vdo/user/vdoVolumeUtils.c#19 $
 */

#include "vdoVolumeUtils.h"

#include <err.h>

#include "constants.h"
#include "fixedLayout.h"
#include "slab.h"
#include "slabDepotInternals.h"
#include "slabSummaryInternals.h"
#include "types.h"
#include "vdoComponentStates.h"
#include "vdoDecode.h"
#include "vdoInternal.h"
#include "vdoLayout.h"

#include "fileLayer.h"

/**********************************************************************/
static int __must_check
decode_vdo(struct vdo *vdo, bool validate_config)
{
	struct vdo_component_states states;
	int result = start_vdo_decode(vdo, validate_config, &states);
	if (result != VDO_SUCCESS) {
		return result;
	}

	result = decode_vdo_layout(states.layout, &vdo->layout);
	if (result != VDO_SUCCESS) {
		return result;
	}
	return finish_vdo_decode(vdo, &states);
}

/**********************************************************************/
int load_vdo_superblock(PhysicalLayer *layer,
                        struct volume_geometry *geometry,
                        bool validate_config,
                        struct vdo **vdo_ptr)
{
	struct vdo *vdo;
	int result = make_vdo(layer, &vdo);
	if (result != VDO_SUCCESS) {
		return result;
	}

	setLoadConfigFromGeometry(geometry, &vdo->load_config);
	result = load_super_block(layer, get_first_block_offset(vdo),
				  &vdo->super_block);
	if (result != VDO_SUCCESS) {
		free_vdo(&vdo);
		return result;
	}

	result = decode_vdo(vdo, validate_config);
	if (result != VDO_SUCCESS) {
		free_vdo(&vdo);
		return result;
	}

	*vdo_ptr = vdo;
	return VDO_SUCCESS;
}

/**********************************************************************/
int load_vdo(PhysicalLayer *layer,
	     bool validate_config,
	     struct vdo **vdo_ptr)
{
	struct volume_geometry geometry;
	int result = load_volume_geometry(layer, &geometry);
	if (result != VDO_SUCCESS) {
		return result;
	}

	return load_vdo_superblock(layer, &geometry, validate_config, vdo_ptr);
}

/**
 * Load a VDO from a file.
 *
 * @param [in]  filename        The file name
 * @param [in]  readOnly        Whether the layer should be read-only.
 * @param [in]  validateConfig  Whether the VDO should validate its config
 * @param [out] vdoPtr          A pointer to hold the VDO
 *
 * @return VDO_SUCCESS or an error code
 **/
static int __must_check loadVDOFromFile(const char *filename,
					bool readOnly,
					bool validateConfig,
					struct vdo **vdoPtr)
{
  int result = ASSERT(validateConfig || readOnly,
                      "Cannot make a writable VDO"
                      " without validating its config");
  if (result != UDS_SUCCESS) {
    return result;
  }

  PhysicalLayer *layer;
  if (readOnly) {
    result = makeReadOnlyFileLayer(filename, &layer);
  } else {
    result = makeFileLayer(filename, 0, &layer);
  }

  if (result != VDO_SUCCESS) {
    char errBuf[ERRBUF_SIZE];
    warnx("Failed to make FileLayer from '%s' with %s",
          filename, stringError(result, errBuf, ERRBUF_SIZE));
    return result;
  }

  // Create the VDO.
  struct vdo *vdo;
  result = load_vdo(layer, validateConfig, &vdo);
  if (result != VDO_SUCCESS) {
    layer->destroy(&layer);
    char errBuf[ERRBUF_SIZE];
    warnx("allocateVDO failed for '%s' with %s",
          filename, stringError(result, errBuf, ERRBUF_SIZE));
    return result;
  }

  *vdoPtr = vdo;
  return VDO_SUCCESS;
}

/**********************************************************************/
int makeVDOFromFile(const char *filename, bool readOnly, struct vdo **vdoPtr)
{
  return loadVDOFromFile(filename, readOnly, true, vdoPtr);
}

/**********************************************************************/
int readVDOWithoutValidation(const char *filename, struct vdo **vdoPtr)
{
  return loadVDOFromFile(filename, true, false, vdoPtr);
}

/**********************************************************************/
void freeVDOFromFile(struct vdo **vdoPtr)
{
  if (*vdoPtr == NULL) {
    return;
  }

  PhysicalLayer *layer = (*vdoPtr)->layer;
  free_vdo(vdoPtr);
  layer->destroy(&layer);
}

/**********************************************************************/
int loadSlabSummarySync(struct vdo *vdo, struct slab_summary **summaryPtr)
{
  struct partition *slabSummaryPartition
    = get_vdo_partition(vdo->layout, SLAB_SUMMARY_PARTITION);
  struct slab_depot *depot = vdo->depot;
  struct thread_config *threadConfig;
  int result = make_one_thread_config(&threadConfig);
  if (result != VDO_SUCCESS) {
    warnx("Could not create thread config");
    return result;
  }

  struct slab_summary *summary = NULL;
  result = make_slab_summary(vdo->layer, slabSummaryPartition, threadConfig,
                             depot->slab_size_shift,
                             depot->slab_config.data_blocks,
                             NULL, &summary);
  if (result != VDO_SUCCESS) {
    warnx("Could not create in-memory slab summary");
  }
  free_thread_config(&threadConfig);
  if (result != VDO_SUCCESS) {
    return result;
  }

  physical_block_number_t origin
    = get_fixed_layout_partition_offset(slabSummaryPartition);
  result = vdo->layer->reader(vdo->layer, origin,
                              get_slab_summary_size(VDO_BLOCK_SIZE),
                              (char *) summary->entries, NULL);
  if (result != VDO_SUCCESS) {
    warnx("Could not read summary data");
    return result;
  }

  summary->zones_to_combine = depot->old_zone_count;
  combine_zones(summary);
  *summaryPtr = summary;
  return VDO_SUCCESS;
}

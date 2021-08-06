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
 * $Id: //eng/uds-releases/krusty/src/uds/indexZone.c#33 $
 */

#include "indexZone.h"

#include "errors.h"
#include "index.h"
#include "indexCheckpoint.h"
#include "indexRouter.h"
#include "logger.h"
#include "memoryAlloc.h"
#include "permassert.h"
#include "request.h"
#include "sparseCache.h"
#include "uds.h"

/**********************************************************************/
int make_index_zone(struct index *index, unsigned int zone_number)
{
	struct index_zone *zone;
	int result = UDS_ALLOCATE(1, struct index_zone, "index zone", &zone);
	if (result != UDS_SUCCESS) {
		return result;
	}

	result = make_open_chapter(index->volume->geometry,
				   index->zone_count,
				   &zone->open_chapter);
	if (result != UDS_SUCCESS) {
		free_index_zone(zone);
		return result;
	}

	result = make_open_chapter(index->volume->geometry,
				   index->zone_count,
				   &zone->writing_chapter);
	if (result != UDS_SUCCESS) {
		free_index_zone(zone);
		return result;
	}

	zone->index = index;
	zone->id = zone_number;
	index->zones[zone_number] = zone;

	return UDS_SUCCESS;
}

/**********************************************************************/
void free_index_zone(struct index_zone *zone)
{
	if (zone == NULL) {
		return;
	}

	free_open_chapter(zone->open_chapter);
	free_open_chapter(zone->writing_chapter);
	UDS_FREE(zone);
}

/**********************************************************************/
bool is_zone_chapter_sparse(const struct index_zone *zone,
			    uint64_t virtual_chapter)
{
	return is_chapter_sparse(zone->index->volume->geometry,
				 zone->oldest_virtual_chapter,
				 zone->newest_virtual_chapter,
				 virtual_chapter);
}

/**********************************************************************/
void set_active_chapters(struct index_zone *zone)
{
	zone->oldest_virtual_chapter = zone->index->oldest_virtual_chapter;
	zone->newest_virtual_chapter = zone->index->newest_virtual_chapter;
}

/**
 * Swap the open and writing chapters after blocking until there are no active
 * chapter writers on the index.
 *
 * @param zone  The zone swapping chapters
 *
 * @return UDS_SUCCESS or a return code
 **/
static int swap_open_chapter(struct index_zone *zone)
{
	struct open_chapter_zone *temp_chapter;
	// Wait for any currently writing chapter to complete
	int result = finish_previous_chapter(zone->index->chapter_writer,
					     zone->newest_virtual_chapter);
	if (result != UDS_SUCCESS) {
		return result;
	}

	// Swap the writing and open chapters
	temp_chapter = zone->open_chapter;
	zone->open_chapter = zone->writing_chapter;
	zone->writing_chapter = temp_chapter;
	return UDS_SUCCESS;
}

/**
 * Advance to a new open chapter, and forget the oldest chapter in the
 * index if necessary.
 *
 * @param zone                 The zone containing the chapter to reap
 *
 * @return UDS_SUCCESS or an error code
 **/
static int reap_oldest_chapter(struct index_zone *zone)
{
	struct index *index = zone->index;
	unsigned int chapters_per_volume =
		index->volume->geometry->chapters_per_volume;
	int result =
		ASSERT(((zone->newest_virtual_chapter -
			 zone->oldest_virtual_chapter) <= chapters_per_volume),
		       "newest (%llu) and oldest (%llu) virtual chapters less than or equal to chapters per volume (%u)",
		       (unsigned long long) zone->newest_virtual_chapter,
		       (unsigned long long) zone->oldest_virtual_chapter,
		       chapters_per_volume);
	if (result != UDS_SUCCESS) {
		return result;
	}

	set_volume_index_zone_open_chapter(index->volume_index, zone->id,
					   zone->newest_virtual_chapter);
	return UDS_SUCCESS;
}

/**
 * Handle notification that some other zone has closed its open chapter. If
 * the chapter that was closed is still the open chapter for this zone,
 * close it now in order to minimize skew.
 *
 * @param zone             The zone receiving the notification
 * @param virtual_chapter  The closed virtual chapter
 *
 * @return UDS_SUCCESS or an error code
 **/
static int handle_chapter_closed(struct index_zone *zone,
				 uint64_t virtual_chapter)
{
	if (zone->newest_virtual_chapter == virtual_chapter) {
		return open_next_chapter(zone, NULL);
	}

	return UDS_SUCCESS;
}

/**********************************************************************/
int dispatch_index_zone_control_request(struct uds_request *request)
{
	struct uds_zone_message *message = &request->zone_message;
	struct index_zone *zone = message->index->zones[request->zone_number];

	switch (message->type) {
	case UDS_MESSAGE_SPARSE_CACHE_BARRIER:
		return update_sparse_cache(zone, message->virtual_chapter);

	case UDS_MESSAGE_ANNOUNCE_CHAPTER_CLOSED:
		return handle_chapter_closed(zone, message->virtual_chapter);

	default:
		uds_log_error("invalid message type: %d", message->type);
		return UDS_INVALID_ARGUMENT;
	}
}

/**
 * Announce the closure of the current open chapter to the other zones.
 *
 * @param request       The request which caused the chapter to close
 *                      (may be NULL)
 * @param zone          The zone which first closed the chapter
 * @param closed_chapter The chapter which was closed
 *
 * @return UDS_SUCCESS or an error code
 **/
static int announce_chapter_closed(struct uds_request *request,
				   struct index_zone *zone,
				   uint64_t closed_chapter)
{
	struct index_router *router =
		((request != NULL) ? request->router : NULL);

	struct uds_zone_message zone_message = {
		.type = UDS_MESSAGE_ANNOUNCE_CHAPTER_CLOSED,
		.index = zone->index,
		.virtual_chapter = closed_chapter,
	};

	unsigned int i;
	for (i = 0; i < zone->index->zone_count; i++) {
		int result;
		if (zone->id == i) {
			continue;
		}
		if (router != NULL) {
			result = launch_zone_message(zone_message, i, router);
		} else {
			// We're in a test which doesn't have zone queues, so
			// we can just call the message function directly.
			result = handle_chapter_closed(zone->index->zones[i],
						       closed_chapter);
		}
		if (result != UDS_SUCCESS) {
			return result;
		}
	}

	return UDS_SUCCESS;
}

/**********************************************************************/
int open_next_chapter(struct index_zone *zone, struct uds_request *request)
{
	uint64_t closed_chapter, victim;
	int result;
	unsigned int finished_zones;
	unsigned int expired_chapters;
	uds_log_debug("closing chapter %llu of zone %u after %u entries (%u short)",
		      (unsigned long long) zone->newest_virtual_chapter,
		      zone->id,
		      zone->open_chapter->size,
		      zone->open_chapter->capacity - zone->open_chapter->size);

	result = swap_open_chapter(zone);
	if (result != UDS_SUCCESS) {
		return result;
	}

	closed_chapter = zone->newest_virtual_chapter++;
	result = reap_oldest_chapter(zone);
	if (result != UDS_SUCCESS) {
		return uds_log_error_strerror(result,
					      "reap_oldest_chapter failed");
	}

	reset_open_chapter(zone->open_chapter);

	// begin, continue, or finish the checkpoint processing
	// moved above start_closing_chapter because some of the
	// checkpoint processing now done by the chapter writer thread
	result = process_checkpointing(zone->index, zone->id,
				       zone->newest_virtual_chapter);
	if (result != UDS_SUCCESS) {
		return result;
	}

	finished_zones =
		start_closing_chapter(zone->index->chapter_writer, zone->id,
				      zone->writing_chapter);
	if ((finished_zones == 1) && (zone->index->zone_count > 1)) {
		// This is the first zone of a multi-zone index to close this
		// chapter, so inform the other zones in order to control zone
		// skew.
		result =
			announce_chapter_closed(request, zone, closed_chapter);
		if (result != UDS_SUCCESS) {
			return result;
		}
	}

	victim = zone->oldest_virtual_chapter;
	expired_chapters = chapters_to_expire(zone->index->volume->geometry,
					      zone->newest_virtual_chapter);
	zone->oldest_virtual_chapter += expired_chapters;

	if (finished_zones < zone->index->zone_count) {
		// We are not the last zone to close the chapter, so we're done
		return UDS_SUCCESS;
	}

	/*
	 * We are the last zone to close the chapter, so clean up the cache.
	 * That it is safe to let the last thread out of the previous chapter
	 * to do this relies on the fact that although the new open chapter
	 * shadows the oldest chapter in the cache, until we write the new open
	 * chapter to disk, we'll never look for it in the cache.
	 */
	while ((expired_chapters-- > 0) && (result == UDS_SUCCESS)) {
		result = forget_chapter(zone->index->volume, victim++,
					INVALIDATION_EXPIRE);
	}

        return result;
}

/**********************************************************************/
enum uds_index_region compute_index_region(const struct index_zone *zone,
					   uint64_t virtual_chapter)
{
	if (virtual_chapter == zone->newest_virtual_chapter) {
		return UDS_LOCATION_IN_OPEN_CHAPTER;
	}
	if (is_zone_chapter_sparse(zone, virtual_chapter)) {
		return UDS_LOCATION_IN_SPARSE;
	}
	return UDS_LOCATION_IN_DENSE;
}

/**********************************************************************/
int get_record_from_zone(struct index_zone *zone,
			 struct uds_request *request,
			 bool *found,
			 uint64_t virtual_chapter)
{
	struct volume *volume;
	if (virtual_chapter == zone->newest_virtual_chapter) {
		search_open_chapter(zone->open_chapter,
				    &request->chunk_name,
				    &request->old_metadata,
				    found);
		return UDS_SUCCESS;
	}

	if ((zone->newest_virtual_chapter > 0) &&
	    (virtual_chapter == (zone->newest_virtual_chapter - 1)) &&
	    (zone->writing_chapter->size > 0)) {
		// Only search the writing chapter if it is full, else look on
		// disk.
		search_open_chapter(zone->writing_chapter,
				    &request->chunk_name,
				    &request->old_metadata,
				    found);
		return UDS_SUCCESS;
	}

	// We have determined the location previously.
	if (request->location != UDS_LOCATION_UNKNOWN) {
		*found = (request->location != UDS_LOCATION_UNAVAILABLE);
		return UDS_SUCCESS;
	}

	volume = zone->index->volume;
	if (is_zone_chapter_sparse(zone, virtual_chapter) &&
	    sparse_cache_contains(volume->sparse_cache,
				  virtual_chapter,
				  request->zone_number)) {
		// The named chunk, if it exists, is in a sparse chapter that
		// is cached, so just run the chunk through the sparse chapter
		// cache search.
		return search_sparse_cache_in_zone(zone, request,
						   virtual_chapter, found);
	}

	return search_volume_page_cache(volume,
					request, &request->chunk_name,
					virtual_chapter,
					&request->old_metadata, found);
}

/**********************************************************************/
int put_record_in_zone(struct index_zone *zone,
		       struct uds_request *request,
		       const struct uds_chunk_data *metadata)
{
	unsigned int remaining;
	int result = put_open_chapter(zone->open_chapter, &request->chunk_name,
				      metadata, &remaining);
	if (result != UDS_SUCCESS) {
		return result;
	}

	if (remaining == 0) {
		return open_next_chapter(zone, request);
	}

	return UDS_SUCCESS;
}

/**********************************************************************/
int search_sparse_cache_in_zone(struct index_zone *zone,
				struct uds_request *request,
				uint64_t virtual_chapter,
				bool *found)
{
	struct volume *volume;
	int record_page_number;
	unsigned int chapter;
	int result = search_sparse_cache(zone,
					 &request->chunk_name,
					 &virtual_chapter,
					 &record_page_number);
	if ((result != UDS_SUCCESS) || (virtual_chapter == UINT64_MAX)) {
		return result;
	}

	volume = zone->index->volume;
	// XXX map to physical chapter and validate. It would be nice to just
	// pass the virtual in to the slow lane, since it's tracking
	// invalidations.
	chapter = map_to_physical_chapter(volume->geometry, virtual_chapter);

	return search_cached_record_page(volume,
					 request, &request->chunk_name,
					 chapter, record_page_number,
					 &request->old_metadata, found);
}

/*
 * %Copyright%
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
 * $Id: //eng/uds-releases/lisa/src/uds/indexSession.c#1 $
 */

#include "indexSession.h"

#include "indexCheckpoint.h"
#include "logger.h"
#include "memoryAlloc.h"
#include "requestQueue.h"
#include "timeUtils.h"

/**********************************************************************/
static void collect_stats(const struct uds_index_session *index_session,
			  struct uds_index_stats *stats)
{
	const struct session_stats *session_stats = &index_session->stats;

	stats->current_time =
		ktime_to_seconds(current_time_ns(CLOCK_REALTIME));
	stats->posts_found = READ_ONCE(session_stats->posts_found);
	stats->in_memory_posts_found =
		READ_ONCE(session_stats->posts_found_open_chapter);
	stats->dense_posts_found = READ_ONCE(session_stats->posts_found_dense);
	stats->sparse_posts_found =
		READ_ONCE(session_stats->posts_found_sparse);
	stats->posts_not_found = READ_ONCE(session_stats->posts_not_found);
	stats->updates_found = READ_ONCE(session_stats->updates_found);
	stats->updates_not_found = READ_ONCE(session_stats->updates_not_found);
	stats->deletions_found = READ_ONCE(session_stats->deletions_found);
	stats->deletions_not_found =
		READ_ONCE(session_stats->deletions_not_found);
	stats->queries_found = READ_ONCE(session_stats->queries_found);
	stats->queries_not_found = READ_ONCE(session_stats->queries_not_found);
	stats->requests = READ_ONCE(session_stats->requests);
}

/**********************************************************************/
static void handle_callbacks(struct uds_request *request)
{
	if (request->status == UDS_SUCCESS) {
		// Measure the turnaround time of this request and include that
		// time, along with the rest of the request, in the context's
		// stat counters.
		update_request_context_stats(request);
	}

	if (request->callback != NULL) {
		// The request has specified its own callback and does not
		// expect to be freed.
		struct uds_index_session *index_session = request->session;
		request->found =
			(request->location != UDS_LOCATION_UNAVAILABLE);
		request->callback((struct uds_request *) request);
		// We do this release after the callback because of the
		// contract of the uds_flush_index_session method.
		release_index_session(index_session);
	}
}

/**********************************************************************/
int check_index_session(struct uds_index_session *index_session)
{
	unsigned int state;
	uds_lock_mutex(&index_session->request_mutex);
	state = index_session->state;
	uds_unlock_mutex(&index_session->request_mutex);

	if (state == IS_FLAG_LOADED) {
		return UDS_SUCCESS;
	} else if (state & IS_FLAG_DISABLED) {
		return UDS_DISABLED;
	} else if ((state & IS_FLAG_LOADING) || (state & IS_FLAG_SUSPENDED) ||
		   (state & IS_FLAG_WAITING)) {
		return -EBUSY;
	}

	return UDS_NO_INDEX;
}

/**********************************************************************/
int get_index_session(struct uds_index_session *index_session)
{
	int result;
	uds_lock_mutex(&index_session->request_mutex);
	index_session->request_count++;
	uds_unlock_mutex(&index_session->request_mutex);

	result = check_index_session(index_session);
	if (result != UDS_SUCCESS) {
		release_index_session(index_session);
		return result;
	}
	return UDS_SUCCESS;
}

/**********************************************************************/
void release_index_session(struct uds_index_session *index_session)
{
	uds_lock_mutex(&index_session->request_mutex);
	if (--index_session->request_count == 0) {
		uds_broadcast_cond(&index_session->request_cond);
	}
	uds_unlock_mutex(&index_session->request_mutex);
}

/**********************************************************************/
int start_loading_index_session(struct uds_index_session *index_session)
{
	int result;
	uds_lock_mutex(&index_session->request_mutex);
	if (index_session->state & IS_FLAG_SUSPENDED) {
		uds_log_info("Index session is suspended");
		result = -EBUSY;
	} else if (index_session->state != 0) {
		uds_log_info("Index is already loaded");
		result = -EBUSY;
	} else {
		index_session->state |= IS_FLAG_LOADING;
		result = UDS_SUCCESS;
	}
	uds_unlock_mutex(&index_session->request_mutex);
	return result;
}

/**********************************************************************/
void finish_loading_index_session(struct uds_index_session *index_session,
				  int result)
{
	uds_lock_mutex(&index_session->request_mutex);
	index_session->state &= ~IS_FLAG_LOADING;
	if (result == UDS_SUCCESS) {
		index_session->state |= IS_FLAG_LOADED;
	}
	uds_broadcast_cond(&index_session->request_cond);
	uds_unlock_mutex(&index_session->request_mutex);
}

/**********************************************************************/
void disable_index_session(struct uds_index_session *index_session)
{
	uds_lock_mutex(&index_session->request_mutex);
	index_session->state |= IS_FLAG_DISABLED;
	uds_unlock_mutex(&index_session->request_mutex);
}

/**********************************************************************/
int make_empty_index_session(struct uds_index_session **index_session_ptr)
{
	struct uds_index_session *session;
	int result = UDS_ALLOCATE(1, struct uds_index_session, __func__, &session);
	if (result != UDS_SUCCESS) {
		return result;
	}

	result = uds_init_mutex(&session->request_mutex);
	if (result != UDS_SUCCESS) {
		UDS_FREE(session);
		return result;
	}

	result = uds_init_cond(&session->request_cond);
	if (result != UDS_SUCCESS) {
		uds_destroy_mutex(&session->request_mutex);
		UDS_FREE(session);
		return result;
	}

	result = uds_init_mutex(&session->load_context.mutex);
	if (result != UDS_SUCCESS) {
		uds_destroy_cond(&session->request_cond);
		uds_destroy_mutex(&session->request_mutex);
		UDS_FREE(session);
		return result;
	}

	result = uds_init_cond(&session->load_context.cond);
	if (result != UDS_SUCCESS) {
		uds_destroy_mutex(&session->load_context.mutex);
		uds_destroy_cond(&session->request_cond);
		uds_destroy_mutex(&session->request_mutex);
		UDS_FREE(session);
		return result;
	}

	result = make_uds_request_queue("callbackW", &handle_callbacks,
					&session->callback_queue);
	if (result != UDS_SUCCESS) {
		uds_destroy_cond(&session->load_context.cond);
		uds_destroy_mutex(&session->load_context.mutex);
		uds_destroy_cond(&session->request_cond);
		uds_destroy_mutex(&session->request_mutex);
		UDS_FREE(session);
		return result;
	}

	*index_session_ptr = session;
	return UDS_SUCCESS;
}

/**********************************************************************/
int uds_suspend_index_session(struct uds_index_session *session, bool save)
{
	int result;
	bool flush_index = false;
	bool save_index = false;
	bool suspend_index = false;
	uds_lock_mutex(&session->request_mutex);
	// Wait for any pending close operation to complete.
	while (session->state & IS_FLAG_CLOSING) {
		uds_wait_cond(&session->request_cond, &session->request_mutex);
	}
	if ((session->state & IS_FLAG_WAITING) ||
	    (session->state & IS_FLAG_DESTROYING)) {
		uds_log_info("Index session is already changing state");
		result = -EBUSY;
	} else if (session->state & IS_FLAG_SUSPENDED) {
		result = UDS_SUCCESS;
	} else if (session->state & IS_FLAG_LOADING) {
		session->state |= IS_FLAG_WAITING;
		suspend_index = true;
		result = UDS_SUCCESS;
	} else if (!(session->state & IS_FLAG_LOADED)) {
		if (session->index != NULL) { 
			flush_index = true;
			session->state |= IS_FLAG_WAITING;
		} else {
			session->state |= IS_FLAG_SUSPENDED;
			uds_broadcast_cond(&session->request_cond);
		}
		result = UDS_SUCCESS;
	} else {
		save_index = save;
		if (save_index) {
			session->state |= IS_FLAG_WAITING;
		} else if (session->index != NULL) { 
			flush_index = true;
			session->state |= IS_FLAG_WAITING;
		} else {
			session->state |= IS_FLAG_SUSPENDED;
			uds_broadcast_cond(&session->request_cond);
		}
		result = UDS_SUCCESS;
	}
	uds_unlock_mutex(&session->request_mutex);

	if (!save_index && !suspend_index && !flush_index) {
		return uds_map_to_system_error(result);
	}

	if (flush_index) {
		result = uds_flush_index_session(session);
		uds_lock_mutex(&session->request_mutex);
		session->state &= ~IS_FLAG_WAITING;
		session->state |= IS_FLAG_SUSPENDED;
		uds_broadcast_cond(&session->request_cond);
		uds_unlock_mutex(&session->request_mutex);
		return uds_map_to_system_error(result);
	}

	if (save_index) {
		result = uds_save_index(session);
		uds_lock_mutex(&session->request_mutex);
		session->state &= ~IS_FLAG_WAITING;
		session->state |= IS_FLAG_SUSPENDED;
		uds_broadcast_cond(&session->request_cond);
		uds_unlock_mutex(&session->request_mutex);
		return uds_map_to_system_error(result);
	}

	uds_lock_mutex(&session->load_context.mutex);
	switch (session->load_context.status) {
	case INDEX_OPENING:
		session->load_context.status = INDEX_SUSPENDING;

		// Wait until the index indicates that it is not replaying.
		while ((session->load_context.status != INDEX_SUSPENDED) &&
		       (session->load_context.status != INDEX_READY)) {
			uds_wait_cond(&session->load_context.cond,
				      &session->load_context.mutex);
		}
		break;

	case INDEX_READY:
		// Index load does not need to be suspended.
		break;

	case INDEX_SUSPENDED:
	case INDEX_SUSPENDING:
	case INDEX_FREEING:
	default:
		// These cases should not happen.
		ASSERT_LOG_ONLY(false,
				"Bad load context state %u",
				session->load_context.status);
		break;
	}
	uds_unlock_mutex(&session->load_context.mutex);

	uds_lock_mutex(&session->request_mutex);
	session->state &= ~IS_FLAG_WAITING;
	session->state |= IS_FLAG_SUSPENDED;
	uds_broadcast_cond(&session->request_cond);
	uds_unlock_mutex(&session->request_mutex);
	return UDS_SUCCESS;
}

/**********************************************************************/
int uds_resume_index_session(struct uds_index_session *session)
{
	uds_lock_mutex(&session->request_mutex);
	if (session->state & IS_FLAG_WAITING) {
		uds_unlock_mutex(&session->request_mutex);
		uds_log_info("Index session is already changing state");
		return -EBUSY;
	}

	/* If not suspended, just succeed */
	if (!(session->state & IS_FLAG_SUSPENDED)) {
		uds_unlock_mutex(&session->request_mutex);
		return UDS_SUCCESS;
	}

	if (!(session->state & IS_FLAG_LOADING)) {
		session->state &= ~IS_FLAG_SUSPENDED;
		uds_unlock_mutex(&session->request_mutex);
		return UDS_SUCCESS;
	}

	session->state |= IS_FLAG_WAITING;
	uds_unlock_mutex(&session->request_mutex);

	uds_lock_mutex(&session->load_context.mutex);
	switch (session->load_context.status) {
	case INDEX_SUSPENDED:
		session->load_context.status = INDEX_OPENING;
		// Notify the index to start replaying again.
		uds_broadcast_cond(&session->load_context.cond);
		break;

	case INDEX_READY:
		// There is no index rebuild to resume.
		break;

	case INDEX_OPENING:
	case INDEX_SUSPENDING:
	case INDEX_FREEING:
	default:
		// These cases should not happen; do nothing.
		ASSERT_LOG_ONLY(false,
				"Bad load context state %u",
				session->load_context.status);
		break;
	}
	uds_unlock_mutex(&session->load_context.mutex);

	uds_lock_mutex(&session->request_mutex);
	session->state &= ~IS_FLAG_WAITING;
	session->state &= ~IS_FLAG_SUSPENDED;
	uds_broadcast_cond(&session->request_cond);
	uds_unlock_mutex(&session->request_mutex);
	return UDS_SUCCESS;
}

/**********************************************************************/
static void
wait_for_no_requests_in_progress(struct uds_index_session *index_session)
{
	uds_lock_mutex(&index_session->request_mutex);
	while (index_session->request_count > 0) {
		uds_wait_cond(&index_session->request_cond,
			      &index_session->request_mutex);
	}
	uds_unlock_mutex(&index_session->request_mutex);
}

/**********************************************************************/
int save_and_free_index(struct uds_index_session *index_session)
{
	int result = UDS_SUCCESS;
	bool suspended;
	struct uds_index *index = index_session->index;
	if (index != NULL) {
		uds_lock_mutex(&index_session->request_mutex);
		suspended = (index_session->state & IS_FLAG_SUSPENDED);
		uds_unlock_mutex(&index_session->request_mutex);
		if (!suspended) {
			result = save_index(index);
			if (result != UDS_SUCCESS) {
				uds_log_warning_strerror(result,
							 "ignoring error from save_index");
			}
		}
		free_index(index);
		index_session->index = NULL;

		// Reset all index state that happens to be in the index
		// session, so it doesn't affect any future index.
		uds_lock_mutex(&index_session->load_context.mutex);
		index_session->load_context.status = INDEX_OPENING;
		uds_unlock_mutex(&index_session->load_context.mutex);

		uds_lock_mutex(&index_session->request_mutex);
		// Only the suspend bit will remain relevant.
		index_session->state &= IS_FLAG_SUSPENDED;
		uds_unlock_mutex(&index_session->request_mutex);
	}

	uds_log_debug("Closed index");
	return result;
}

/**********************************************************************/
int uds_close_index(struct uds_index_session *index_session)
{
	int result = UDS_SUCCESS;
	uds_lock_mutex(&index_session->request_mutex);

	// Wait for any pending suspend, resume or close operations to
	// complete.
	while ((index_session->state & IS_FLAG_WAITING) ||
	       (index_session->state & IS_FLAG_CLOSING)) {
		uds_wait_cond(&index_session->request_cond,
			      &index_session->request_mutex);
	}

	if (index_session->state & IS_FLAG_SUSPENDED) {
		uds_log_info("Index session is suspended");
		result = -EBUSY;
	} else if ((index_session->state & IS_FLAG_DESTROYING) ||
		   !(index_session->state & IS_FLAG_LOADED)) {
		// The index doesn't exist, hasn't finished loading, or is
		// being destroyed.
		result = UDS_NO_INDEX;
	} else {
		index_session->state |= IS_FLAG_CLOSING;
	}
	uds_unlock_mutex(&index_session->request_mutex);
	if (result != UDS_SUCCESS) {
		return uds_map_to_system_error(result);
	}

	uds_log_debug("Closing index");
	wait_for_no_requests_in_progress(index_session);
	result = save_and_free_index(index_session);

	uds_lock_mutex(&index_session->request_mutex);
	index_session->state &= ~IS_FLAG_CLOSING;
	uds_broadcast_cond(&index_session->request_cond);
	uds_unlock_mutex(&index_session->request_mutex);
	return uds_map_to_system_error(result);
}

/**********************************************************************/
int uds_destroy_index_session(struct uds_index_session *index_session)
{
	int result;
	bool load_pending = false;
	uds_log_debug("Destroying index session");

	uds_lock_mutex(&index_session->request_mutex);

	// Wait for any pending suspend, resume, or close operations to
	// complete.
	while ((index_session->state & IS_FLAG_WAITING) ||
	       (index_session->state & IS_FLAG_CLOSING)) {
		uds_wait_cond(&index_session->request_cond,
			      &index_session->request_mutex);
	}

	if (index_session->state & IS_FLAG_DESTROYING) {
		uds_unlock_mutex(&index_session->request_mutex);
		uds_log_info("Index session is already closing");
		return -EBUSY;
	}

	index_session->state |= IS_FLAG_DESTROYING;
	load_pending = ((index_session->state & IS_FLAG_LOADING) &&
			(index_session->state & IS_FLAG_SUSPENDED));
	uds_unlock_mutex(&index_session->request_mutex);

	if (load_pending) {
		// Tell the index to terminate the rebuild.
		uds_lock_mutex(&index_session->load_context.mutex);
		if (index_session->load_context.status == INDEX_SUSPENDED) {
			index_session->load_context.status = INDEX_FREEING;
			uds_broadcast_cond(&index_session->load_context.cond);
		}
		uds_unlock_mutex(&index_session->load_context.mutex);

		// Wait until the load exits before proceeding.
		uds_lock_mutex(&index_session->request_mutex);
		while (index_session->state & IS_FLAG_LOADING) {
			uds_wait_cond(&index_session->request_cond,
				      &index_session->request_mutex);
		}
		uds_unlock_mutex(&index_session->request_mutex);
	}

	wait_for_no_requests_in_progress(index_session);
	result = save_and_free_index(index_session);
	uds_request_queue_finish(index_session->callback_queue);
	index_session->callback_queue = NULL;
	uds_destroy_cond(&index_session->load_context.cond);
	uds_destroy_mutex(&index_session->load_context.mutex);
	uds_destroy_cond(&index_session->request_cond);
	uds_destroy_mutex(&index_session->request_mutex);
	uds_log_debug("Destroyed index session");
	UDS_FREE(index_session);
	return uds_map_to_system_error(result);
}

/**********************************************************************/
int uds_flush_index_session(struct uds_index_session *index_session)
{
	wait_for_no_requests_in_progress(index_session);
	// Wait until any open chapter writes are complete
	wait_for_idle_index(index_session->index);
	return UDS_SUCCESS;
}

/**********************************************************************/
int uds_save_index(struct uds_index_session *index_session)
{
	wait_for_no_requests_in_progress(index_session);
	// save_index waits for open chapter writes to complete
	return save_index(index_session->index);
}

/**********************************************************************/
int uds_set_checkpoint_frequency(struct uds_index_session *index_session,
				 unsigned int frequency)
{
	set_index_checkpoint_frequency(index_session->index->checkpoint,
				       frequency);
	return UDS_SUCCESS;
}

/**********************************************************************/
int uds_get_index_configuration(struct uds_index_session *index_session,
				struct uds_configuration **conf)
{
	int result;
	if (conf == NULL) {
		uds_log_error("received a NULL config pointer");
		return -EINVAL;
	}
	result = UDS_ALLOCATE(1, struct uds_configuration, __func__, conf);
	if (result == UDS_SUCCESS) {
		**conf = index_session->user_config;
	}
	return uds_map_to_system_error(result);
}

/**********************************************************************/
int uds_get_index_stats(struct uds_index_session *index_session,
			struct uds_index_stats *stats)
{
	if (stats == NULL) {
		uds_log_error("received a NULL index stats pointer");
		return -EINVAL;
	}

	collect_stats(index_session, stats);
	if (index_session->index != NULL) {
		get_index_stats(index_session->index, stats);
	} else {
          stats->entries_indexed = 0;
          stats->memory_used = 0;
          stats->collisions = 0;
          stats->entries_discarded = 0;
	}

	return UDS_SUCCESS;
}

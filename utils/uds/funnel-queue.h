/* SPDX-License-Identifier: GPL-2.0-only */
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

#ifndef FUNNEL_QUEUE_H
#define FUNNEL_QUEUE_H

#include <linux/atomic.h>

#include "compiler.h"
#include "cpu.h"
#include "type-defs.h"

/**
 * A funnel queue is a simple lock-free (almost) queue that accepts entries
 * from multiple threads (multi-producer) and delivers them to a single thread
 * (single-consumer). "Funnel" is an attempt to evoke the image of requests
 * from more than one producer being "funneled down" to a single consumer.
 *
 * This is an unsynchronized but thread-safe data structure when used as
 * intended. There is no mechanism to ensure that only one thread is consuming
 * from the queue, so if that is done mistakenly, it will not be trapped, and
 * the resulting behavior is undefined. Clients must not directly access or
 * manipulate the internals, which are only exposed for the purpose of
 * allowing the very simple enqueue operation to be in-lined.
 *
 * The implementation requires that a funnel_queue_entry structure (a link
 * pointer) be embedded in the queue entries, and pointers to those structures
 * are used exclusively by the queue. No macros are defined to template the
 * queue, so the offset of the funnel_queue_entry in the records placed in the
 * queue must all have a fixed offset so the client can derive their structure
 * pointer from the entry pointer returned by funnel_queue_poll().
 *
 * Callers are wholly responsible for allocating and freeing the entries.
 * Entries may be freed as soon as they are returned since this queue is not
 * susceptible to the "ABA problem" present in many lock-free data structures.
 * The queue is dynamically allocated to ensure cache-line alignment, but no
 * other dynamic allocation is used.
 *
 * The algorithm is not actually 100% lock-free. There is a single point in
 * funnel_queue_put() at which a pre-empted producer will prevent the consumers
 * from seeing items added to the queue by later producers, and only if the
 * queue is short enough or the consumer fast enough for it to reach what was
 * the end of the queue at the time of the pre-empt.
 *
 * The consumer function, funnel_queue_poll(), will return NULL when the queue
 * is empty. To wait for data to consume, spin (if safe) or combine the queue
 * with an event_count to signal the presence of new entries.
 **/

/**
 * The queue link structure that must be embedded in client entries.
 **/
struct funnel_queue_entry {
	/* The next (newer) entry in the queue. */
	struct funnel_queue_entry *volatile next;
};

/**
 * The dynamically allocated queue structure, which is aligned to a cache line
 * boundary when allocated. This should be consider opaque; it is exposed here
 * so funnel_queue_put() can be in-lined.
 **/
struct __attribute__((aligned(CACHE_LINE_BYTES))) funnel_queue {
	/*
	 * The producers' end of the queue--an atomically exchanged pointer
	 * that will never be NULL.
	 */
	struct funnel_queue_entry *volatile newest;

	/*
	 * The consumer's end of the queue. Owned by the consumer and never
	 * NULL.
	 */
	struct funnel_queue_entry *oldest
		__attribute__((aligned(CACHE_LINE_BYTES)));

	/*
	 * A re-usable dummy entry used to provide the non-NULL invariants
	 * above.
	 */
	struct funnel_queue_entry stub;
};

/**
 * Construct and initialize a new, empty queue.
 *
 * @param queue_ptr  a pointer in which to store the queue
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check make_funnel_queue(struct funnel_queue **queue_ptr);

/**
 * Free a queue.
 *
 * This will not free any entries in the queue. The caller must ensure that
 * either the queue will be empty or that any entries in the queue will not be
 * leaked by dropping the references from queue.
 *
 * @param queue  the queue to free
 **/
void free_funnel_queue(struct funnel_queue *queue);

/**
 * Put an entry on the end of the queue.
 *
 * The entry pointer must be to the struct funnel_queue_entry embedded in the
 * caller's data structure. The caller must be able to derive the address of
 * the start of their data structure from the pointer that passed in here, so
 * every entry in the queue must have the struct funnel_queue_entry at the same
 * offset within the client's structure.
 *
 * @param queue  the queue on which to place the entry
 * @param entry  the entry to be added to the queue
 **/
static INLINE void funnel_queue_put(struct funnel_queue *queue,
				    struct funnel_queue_entry *entry)
{
	struct funnel_queue_entry *previous;
	/*
	 * Barrier requirements: All stores relating to the entry ("next"
	 * pointer, containing data structure fields) must happen before the
	 * previous->next store making it visible to the consumer. Also, the
	 * entry's "next" field initialization to NULL must happen before any
	 * other producer threads can see the entry (the xchg) and try to
	 * update the "next" field.
	 *
	 * xchg implements a full barrier.
	 */
	entry->next = NULL;
	/*
	 * The xchg macro in the PPC kernel calls a function that takes a void*
	 * argument, triggering a warning about dropping the volatile
	 * qualifier.
	 */
#pragma GCC diagnostic push
#if __GNUC__ >= 5
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
	previous = xchg(&queue->newest, entry);
#pragma GCC diagnostic pop
	/*
	 * Pre-empts between these two statements hide the rest of the queue
	 * from the consumer, preventing consumption until the following
	 * assignment runs.
	 */
	previous->next = entry;
}

/**
 * Poll a queue, removing the oldest entry if the queue is not empty. This
 * function must only be called from a single consumer thread.
 *
 * @param queue  the queue from which to remove an entry
 *
 * @return the oldest entry in the queue, or NULL if the queue is empty.
 **/
struct funnel_queue_entry *__must_check
funnel_queue_poll(struct funnel_queue *queue);

/**
 * Check whether the funnel queue is empty or not. This function must only be
 * called from a single consumer thread, as with funnel_queue_poll.
 *
 * If the queue is in a transition state with one or more entries being added
 * such that the list view is incomplete, it may not be possible to retrieve an
 * entry with the funnel_queue_poll() function. In such states this function
 * will report an empty indication.
 *
 * @param queue  the queue which to check for entries.
 *
 * @return true iff queue contains no entry which can be retrieved
 **/
bool __must_check is_funnel_queue_empty(struct funnel_queue *queue);

/**
 * Check whether the funnel queue is idle or not. This function must only be
 * called from a single consumer thread, as with funnel_queue_poll.
 *
 * If the queue has entries available to be retrieved, it is not idle. If the
 * queue is in a transition state with one or more entries being added such
 * that the list view is incomplete, it may not be possible to retrieve an
 * entry with the funnel_queue_poll() function, but the queue will not be
 * considered idle.
 *
 * @param queue  the queue which to check for entries.
 *
 * @return true iff queue contains no entry which can be retrieved nor is
 *              known to be having an entry added
 **/
bool __must_check is_funnel_queue_idle(struct funnel_queue *queue);

#endif /* FUNNEL_QUEUE_H */

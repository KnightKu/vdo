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

#include "permassert.h"
#include "uds-threads.h"

/**********************************************************************/
int uds_init_cond(struct cond_var *cond)
{
	int result = pthread_cond_init(&cond->condition, NULL);
	return ASSERT_WITH_ERROR_CODE((result == 0), result,
				      "pthread_cond_init error");
}

/**********************************************************************/
int uds_signal_cond(struct cond_var *cond)
{
	int result = pthread_cond_signal(&cond->condition);
	return ASSERT_WITH_ERROR_CODE((result == 0), result,
				      "pthread_cond_signal error");
}

/**********************************************************************/
int uds_broadcast_cond(struct cond_var *cond)
{
	int result = pthread_cond_broadcast(&cond->condition);
	return ASSERT_WITH_ERROR_CODE((result == 0), result,
				      "pthread_cond_broadcast error");
}

/**********************************************************************/
int uds_wait_cond(struct cond_var *cond, struct mutex *mutex)
{
	int result = pthread_cond_wait(&cond->condition, &mutex->mutex);
	return ASSERT_WITH_ERROR_CODE((result == 0), result,
				      "pthread_cond_wait error");
}

/**********************************************************************/
int uds_timed_wait_cond(struct cond_var *cond,
			struct mutex *mutex,
			ktime_t timeout)
{
	struct timespec ts = future_time(timeout);
	return pthread_cond_timedwait(&cond->condition, &mutex->mutex, &ts);
}

/**********************************************************************/
int uds_destroy_cond(struct cond_var *cond)
{
	int result = pthread_cond_destroy(&cond->condition);
	return ASSERT_WITH_ERROR_CODE((result == 0), result,
				      "pthread_cond_destroy error");
}

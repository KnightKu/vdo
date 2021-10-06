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
 * $Id: //eng/uds-releases/lisa/src/uds/logger.h#2 $
 */

#ifndef LOGGER_H
#define LOGGER_H 1

#include <stdarg.h>
#include "minisyslog.h"

#define UDS_LOG_EMERG LOG_EMERG
#define UDS_LOG_ALERT LOG_ALERT
#define UDS_LOG_CRIT LOG_CRIT
#define UDS_LOG_ERR LOG_ERR
#define UDS_LOG_WARNING LOG_WARNING
#define UDS_LOG_NOTICE LOG_NOTICE
#define UDS_LOG_INFO LOG_INFO
#define UDS_LOG_DEBUG LOG_DEBUG

#define UDS_LOGGING_MODULE_NAME "vdo"

/*
 * Apply a rate limiter to a log method call.
 *
 * @param log_fn  A method that does logging, which is not invoked if we are
 *                running in the kernel and the ratelimiter detects that we
 *                are calling it frequently.
 */
#define uds_log_ratelimit(log_fn, ...) log_fn(__VA_ARGS__)

/**
 * @file
 *
 * All of the log<Level>() functions will preserve the callers value of errno.
 **/

/**
 * Initialize the user space logger using optional environment
 * variables to set the default log level and log file. Can be called
 * more than once, but only the first call affects logging by user
 * space programs. For testing purposes, when the logging environment
 * needs to be changed, see reinit_uds_logger. The kernel module uses
 * kernel logging facilities and therefore doesn't need an open_uds_logger
 * method.
 **/
void open_uds_logger(void);


/**
 * Get the current logging level.
 *
 * @return  the current logging priority level.
 **/
int get_uds_log_level(void);

/**
 * Set the current logging level.
 *
 * @param new_log_level  the new value for the logging priority level.
 **/
void set_uds_log_level(int new_log_level);

/**
 * Return the integer logging priority represented by a name.
 *
 * @param string  the name of the logging priority (case insensitive).
 *
 * @return the integer priority named by string, or UDS_LOG_INFO if not
 *          recognized.
 **/
int uds_log_string_to_priority(const char *string);

/**
 * Return the printable name of a logging priority.
 *
 * @return the priority name
 **/
const char *uds_log_priority_to_string(int priority);

/**
 * Log a message embedded within another message.
 *
 * @param priority      the priority at which to log the message
 * @param module        the name of the module doing the logging
 * @param prefix        optional string prefix to message, may be NULL
 * @param fmt1          format of message first part (required)
 * @param args1         arguments for message first part (required)
 * @param fmt2          format of message second part
 **/
void uds_log_embedded_message(int priority,
			      const char *module,
			      const char *prefix,
			      const char *fmt1,
			      va_list args1,
			      const char *fmt2,
			      ...)
	__printf(4, 0) __printf(6, 7);

/**
 * Log a message pack consisting of multiple variable sections.
 *
 * @param priority      the priority at which to log the message
 * @param module        the name of the module doing the logging
 * @param prefix        optional string prefix to message, may be NULL
 * @param fmt1          format of message first part (required)
 * @param args1         arguments for message first part
 * @param fmt2          format of message second part (required)
 * @param args2         arguments for message second part
 **/
void uds_log_message_pack(int priority,
			  const char *module,
			  const char *prefix,
			  const char *fmt1,
			  va_list args1,
			  const char *fmt2,
			  va_list args2)
	__printf(4, 0) __printf(6, 0);

/**
 * Log a stack backtrace.
 *
 * @param  priority The priority at which to log the backtrace
 **/
void uds_log_backtrace(int priority);

/**
 * Log a message with an error from an error code.
 *
 * @param priority  The priority of the logging entry
 * @param errnum    Int value of errno or a UDS_* value
 *
 * @return errnum
 **/
#define uds_log_strerror(priority, errnum, ...)     \
	__uds_log_strerror(priority,                \
			   errnum,                  \
			   UDS_LOGGING_MODULE_NAME, \
			   __VA_ARGS__)

/**
 * Log a message with an error from an error code.
 *
 * @param priority  The priority of the logging entry
 * @param errnum    Int value of errno or a UDS_* value
 * @param module    The name of the module doing the logging
 * @param format    The format of the message (a printf style format)
 *
 * @return errnum
 **/
int __uds_log_strerror(int priority,
		       int errnum,
		       const char *module,
		       const char *format,
		       ...)
	__printf(4, 5);

/**
 * Log a message with an error from an error code.
 *
 * @param priority  The priority of the logging entry
 * @param errnum    Int value of errno or a UDS_* value
 * @param module    The name of the module doing the logging
 * @param format    The format of the message (a printf style format)
 * @param args	    The list of arguments with format.
 *
 * @return errnum
 **/
int uds_vlog_strerror(int priority,
		      int errnum,
		      const char *module,
		      const char *format,
		      va_list args)
	__printf(4, 0);

/**
 * Log an error prefixed with the string associated with the errnum.
 *
 * @param errnum Int value of errno or a UDS_* value.
 *
 * @return errnum
 **/
#define uds_log_error_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_ERR, errnum, __VA_ARGS__);

/**********************************************************************/
#define uds_log_debug_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_DEBUG, errnum, __VA_ARGS__);

/**********************************************************************/
#define uds_log_info_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_INFO, errnum, __VA_ARGS__);

/**********************************************************************/
#define uds_log_notice_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_NOTICE, errnum, __VA_ARGS__);

/**********************************************************************/
#define uds_log_warning_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_WARNING, errnum, __VA_ARGS__);

/**********************************************************************/
#define uds_log_fatal_strerror(errnum, ...) \
	uds_log_strerror(UDS_LOG_CRIT, errnum, __VA_ARGS__);

/**
 * Log a message.
 *
 * @param priority  The syslog priority value for the message
 * @param format    The format of the message (a printf style format)
 **/
void uds_log_message(int priority, const char *format, ...)
	__printf(2, 3);

/**
 * Log a debug message. Takes printf-style arguments.
 **/
#define uds_log_debug(...) uds_log_message(UDS_LOG_DEBUG, __VA_ARGS__)

/**
 * Log an informational message. Takes printf-style arguments.
 **/
#define uds_log_info(...) uds_log_message(UDS_LOG_INFO, __VA_ARGS__)

/**
 * Log a normal (but notable) condition. Takes printf-style arguments.
 **/
#define uds_log_notice(...) uds_log_message(UDS_LOG_NOTICE, __VA_ARGS__)

/**
 * Log a warning. Takes printf-style arguments.
 **/
#define uds_log_warning(...) uds_log_message(UDS_LOG_WARNING, __VA_ARGS__)

/**
 * Log an error. Takes printf-style arguments.
 **/
#define uds_log_error(...) uds_log_message(UDS_LOG_ERR, __VA_ARGS__)

/**
 * Log a fatal error. Takes printf-style arguments.
 **/
#define uds_log_fatal(...) uds_log_message(UDS_LOG_CRIT, __VA_ARGS__)

/**
 * Sleep or delay a short time (likely a few milliseconds) in an attempt allow
 * the log buffers to be written out in case they might be overrun. This is
 * unnecessary in user-space (and is a no-op there), but is needed when
 * quickly issuing a lot of log output in the Linux kernel, as when dumping a
 * large number of data structures.
 **/
void uds_pause_for_logger(void);


#endif /* LOGGER_H */

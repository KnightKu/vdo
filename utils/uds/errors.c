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
 * $Id: //eng/uds-releases/krusty/src/uds/errors.c#18 $
 */

#include "errors.h"

#include "common.h"
#include "permassert.h"
#include "stringUtils.h"


static const struct error_info successful = { "UDS_SUCCESS", "Success" };


static const struct error_info error_list[] = {
	{ "UDS_UNUSED_CODE_0", "Unused error code 0" },
	{ "UDS_UNUSED_CODE_1", "Unused error code 1" },
	{ "UDS_EMODULE_LOAD", "Could not load modules" },
	{ "UDS_UNUSED_CODE_3", "Unused error code 3" },
	{ "UDS_UNUSED_CODE_4", "Unused error code 4" },
	{ "UDS_DISABLED", "UDS library context is disabled" },
	{ "UDS_CORRUPT_COMPONENT", "Corrupt saved component" },
	{ "UDS_UNKNOWN_ERROR", "Unknown error" },
	{ "UDS_UNUSED_CODE_8", "Unused error code 8" },
	{ "UDS_UNUSED_CODE_9", "Unused error code 9" },
	{ "UDS_UNSUPPORTED_VERSION", "Unsupported version" },
	{ "UDS_UNUSED_CODE_11", "Unused error code 11" },
	{ "UDS_CORRUPT_DATA", "Index data in memory is corrupt" },
	{ "UDS_SHORT_READ", "Could not read requested number of bytes" },
	{ "UDS_UNUSED_CODE_14", "Unused error code 14" },
	{ "UDS_RESOURCE_LIMIT_EXCEEDED", "Internal resource limits exceeded" },
	{ "UDS_VOLUME_OVERFLOW", "Memory overflow due to storage failure" },
	{ "UDS_UNUSED_CODE_17", "Unused error code 17" },
	{ "UDS_UNUSED_CODE_18", "Unused error code 18" },
	{ "UDS_UNUSED_CODE_19", "Unused error code 19" },
	{ "UDS_UNUSED_CODE_20", "Unused error code 20" },
	{ "UDS_UNUSED_CODE_21", "Unused error code 21" },
	{ "UDS_UNUSED_CODE_22", "Unused error code 22" },
	{ "UDS_UNUSED_CODE_23", "Unused error code 23" },
	{ "UDS_UNUSED_CODE_24", "Unused error code 24" },
	{ "UDS_UNUSED_CODE_25", "Unused error code 25" },
	{ "UDS_UNUSED_CODE_26", "Unused error code 26" },
	{ "UDS_UNUSED_CODE_27", "Unused error code 27" },
	{ "UDS_UNUSED_CODE_28", "Unused error code 28" },
	{ "UDS_UNUSED_CODE_29", "Unused error code 29" },
	{ "UDS_UNUSED_CODE_30", "Unused error code 30" },
	{ "UDS_UNUSED_CODE_31", "Unused error code 31" },
	{ "UDS_UNUSED_CODE_32", "Unused error code 32" },
	{ "UDS_UNUSED_CODE_33", "Unused error code 33" },
	{ "UDS_UNUSED_CODE_34", "Unused error code 34" },
	{ "UDS_UNUSED_CODE_35", "Unused error code 35" },
	{ "UDS_UNUSED_CODE_36", "Unused error code 36" },
	{ "UDS_NO_INDEX", "No index found" },
	{ "UDS_UNUSED_CODE_38", "Unused error code 38" },
	{ "UDS_UNUSED_CODE_39", "Unused error code 39" },
	{ "UDS_UNUSED_CODE_40", "Unused error code 40" },
	{ "UDS_UNUSED_CODE_41", "Unused error code 41" },
	{ "UDS_UNUSED_CODE_42", "Unused error code 42" },
	{ "UDS_UNUSED_CODE_43", "Unused error code 43" },
	{ "UDS_END_OF_FILE", "Unexpected end of file" },
	{ "UDS_INDEX_NOT_SAVED_CLEANLY", "Index not saved cleanly" },
};

static const struct error_info internal_error_list[] = {
	{ "UDS_INTERNAL_UNUSED_0", "Unused internal error 0" },
	{ "UDS_OVERFLOW", "Index overflow" },
	{ "UDS_INTERNAL_UNUSED_2", "Unused internal error 2" },
	{ "UDS_INVALID_ARGUMENT",
	  "Invalid argument passed to internal routine" },
	{ "UDS_BAD_STATE", "UDS data structures are in an invalid state" },
	{ "UDS_DUPLICATE_NAME",
	  "Attempt to enter the same name into a delta index twice" },
	{ "UDS_UNEXPECTED_RESULT", "Unexpected result from internal routine" },
	{ "UDS_INTERNAL_UNUSED_7", "Unused internal error 7" },
	{ "UDS_ASSERTION_FAILED", "Assertion failed" },
	{ "UDS_INTERNAL_UNUSED_9", "Unused internal error 9" },
	{ "UDS_QUEUED", "Request queued" },
	{ "UDS_INTERNAL_UNUSED_11", "Unused internal error 11" },
	{ "UDS_INTERNAL_UNUSED_12", "Unused internal error 12" },
	{ "UDS_BUFFER_ERROR", "Buffer error" },
	{ "UDS_INTERNAL_UNUSED_14", "Unused internal error 14" },
	{ "UDS_INTERNAL_UNUSED_15", "Unused internal error 15" },
	{ "UDS_NO_DIRECTORY", "Expected directory is missing" },
	{ "UDS_CHECKPOINT_INCOMPLETE", "Checkpoint not completed" },
	{ "UDS_INTERNAL_UNUSED_18", "Unused internal error 18" },
	{ "UDS_INTERNAL_UNUSED_19", "Unused internal error 19" },
	{ "UDS_ALREADY_REGISTERED", "Error range already registered" },
	{ "UDS_BAD_IO_DIRECTION", "Bad I/O direction" },
	{ "UDS_INCORRECT_ALIGNMENT", "Offset not at block alignment" },
	{ "UDS_OUT_OF_RANGE", "Cannot access data outside specified limits" },
};

struct error_block {
	const char *name;
	int base;
	int last;
	int max;
	const struct error_info *infos;
};

enum {
	MAX_ERROR_BLOCKS = 6 // needed for testing
};

static struct error_information {
	int allocated;
	int count;
	struct error_block blocks[MAX_ERROR_BLOCKS];
} registered_errors = {
	.allocated = MAX_ERROR_BLOCKS,
	.count = 2,
	.blocks = { {
			    .name = "UDS Error",
			    .base = UDS_ERROR_CODE_BASE,
			    .last = UDS_ERROR_CODE_LAST,
			    .max = UDS_ERROR_CODE_BLOCK_END,
			    .infos = error_list,
		    },
		    {
			    .name = "UDS Internal Error",
			    .base = UDS_INTERNAL_ERROR_CODE_BASE,
			    .last = UDS_INTERNAL_ERROR_CODE_LAST,
			    .max = UDS_INTERNAL_ERROR_CODE_BLOCK_END,
			    .infos = internal_error_list,
		    } }
};

/**
 * Fetch the error info (if any) for the error number.
 *
 * @param errnum        the error number
 * @param info_ptr      the place to store the info for this error (if known),
 *                      otherwise set to NULL
 *
 * @return              the name of the error block (if known), NULL othersise
 **/
static const char *get_error_info(int errnum,
				  const struct error_info **info_ptr)
{
	struct error_block *block;

	if (errnum == UDS_SUCCESS) {
		if (info_ptr != NULL) {
			*info_ptr = &successful;
		}
		return NULL;
	}

	for (block = registered_errors.blocks;
	     block < registered_errors.blocks + registered_errors.count;
	     ++block) {
		if ((errnum >= block->base) && (errnum < block->last)) {
			if (info_ptr != NULL) {
				*info_ptr =
					block->infos + (errnum - block->base);
			}
			return block->name;
		} else if ((errnum >= block->last) && (errnum < block->max)) {
			if (info_ptr != NULL) {
				*info_ptr = NULL;
			}
			return block->name;
		}
	}
	if (info_ptr != NULL) {
		*info_ptr = NULL;
	}
	return NULL;
}

/**
 * Return string describing a system error message
 *
 * @param errnum  System error number
 * @param buf     Buffer that can be used to contain the return value
 * @param buflen  Length of the buffer
 *
 * @return The error string, which may be a string constant or may be
 *         returned in the buf argument
 **/
static INLINE const char *
system_string_error(int errnum, char *buf, size_t buflen)
{
	return strerror_r(errnum, buf, buflen);
}

/**********************************************************************/
const char *string_error(int errnum, char *buf, size_t buflen)
{
	char *buffer = buf;
	char *buf_end = buf + buflen;
	const struct error_info *info = NULL;
	const char *block_name;

	if (buf == NULL) {
		return NULL;
	}

	if (errnum < 0) {
		errnum = -errnum;
	}

	block_name = get_error_info(errnum, &info);

	if (block_name != NULL) {
		if (info != NULL) {
			buffer = uds_append_to_buffer(buffer,
						      buf_end,
						      "%s: %s",
						      block_name,
						      info->message);
		} else {
			buffer = uds_append_to_buffer(buffer,
						      buf_end,
						      "Unknown %s %d",
						      block_name,
						      errnum);
		}
	} else if (info != NULL) {
		buffer = uds_append_to_buffer(buffer, buf_end, "%s",
					      info->message);
	} else {
		const char *tmp =
			system_string_error(errnum, buffer, buf_end - buffer);
		if (tmp != buffer) {
			buffer = uds_append_to_buffer(buffer, buf_end, "%s",
						      tmp);
		} else {
			buffer += strlen(tmp);
		}
	}
	return buf;
}

/**********************************************************************/
const char *string_error_name(int errnum, char *buf, size_t buflen)
{

	char *buffer = buf;
	char *buf_end = buf + buflen;
	const struct error_info *info = NULL;
	const char *block_name;

	if (errnum < 0) {
		errnum = -errnum;
	}
	block_name = get_error_info(errnum, &info);
	if (block_name != NULL) {
		if (info != NULL) {
			buffer = uds_append_to_buffer(buffer, buf_end, "%s",
						      info->name);
		} else {
			buffer = uds_append_to_buffer(buffer, buf_end, "%s %d",
						      block_name, errnum);
		}
	} else if (info != NULL) {
		buffer = uds_append_to_buffer(buffer, buf_end, "%s",
					      info->name);
	} else {
		const char *tmp =
			system_string_error(errnum, buffer, buf_end - buffer);
		if (tmp != buffer) {
			buffer = uds_append_to_buffer(buffer, buf_end, "%s",
						      tmp);
		} else {
			buffer += strlen(tmp);
		}
	}
	return buf;
}

/**********************************************************************/
int register_error_block(const char *block_name,
			 int first_error,
			 int last_reserved_error,
			 const struct error_info *infos,
			 size_t info_size)
{
	struct error_block *block;
	int result = ASSERT(first_error < last_reserved_error,
			    "bad error block range");
	if (result != UDS_SUCCESS) {
		return result;
	}

	if (registered_errors.count == registered_errors.allocated) {
		// could reallocate and grow, but should never happen
		return UDS_OVERFLOW;
	}

	for (block = registered_errors.blocks;
	     block < registered_errors.blocks + registered_errors.count;
	     ++block) {
		if (strcmp(block_name, block->name) == 0) {
			return UDS_DUPLICATE_NAME;
		}
		// check for overlap in error ranges
		if ((first_error < block->max) &&
		    (last_reserved_error > block->base)) {
			return UDS_ALREADY_REGISTERED;
		}
	}

	registered_errors.blocks[registered_errors.count++] =
		(struct error_block){ .name = block_name,
				      .base = first_error,
				      .last = first_error +
					      (info_size /
					       sizeof(struct error_info)),
				      .max = last_reserved_error,
				      .infos = infos };

	return UDS_SUCCESS;
}

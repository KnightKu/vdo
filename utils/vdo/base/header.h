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
 * $Id: //eng/linux-vdo/src/c++/vdo/base/header.h#5 $
 */

#ifndef HEADER_H
#define HEADER_H

#include "buffer.h"
#include "numeric.h"

#include "types.h"

/**
 * An in-memory representation of a version number for versioned structures on
 * disk.
 *
 * A version number consists of two portions, a major version and a
 * minor version. Any format change which does not require an explicit
 * upgrade step from the previous version should increment the minor
 * version. Any format change which either requires an explicit
 * upgrade step, or is wholly incompatible (i.e. can not be upgraded
 * to), should increment the major version, and set the minor version
 * to 0.
 **/
struct version_number {
	uint32_t major_version;
	uint32_t minor_version;
} __attribute__((packed));

/**
 * A packed, machine-independent, on-disk representation of a version_number.
 * Both fields are stored in little-endian byte order.
 **/
struct packed_version_number {
	byte major_version[4];
	byte minor_version[4];
} __attribute__((packed));

/**
 * The registry of component ids for use in headers
 **/
typedef enum {
	SUPER_BLOCK = 0,
	FIXED_LAYOUT = 1,
	RECOVERY_JOURNAL = 2,
	SLAB_DEPOT = 3,
	BLOCK_MAP = 4,
	GEOMETRY_BLOCK = 5,
} component_id;

/**
 * The header for versioned data stored on disk.
 **/
struct header {
	component_id id; // The component this is a header for
	struct version_number version; // The version of the data format
	size_t size; // The size of the data following this header
} __attribute__((packed));

enum {
	ENCODED_HEADER_SIZE = sizeof(struct header),
};

/**
 * Check whether two version numbers are the same.
 *
 * @param version_a The first version
 * @param version_b The second version
 *
 * @return <code>true</code> if the two versions are the same
 **/
static inline bool are_same_version(struct version_number version_a,
				    struct version_number version_b)
{
	return ((version_a.major_version == version_b.major_version)
		&& (version_a.minor_version == version_b.minor_version));
}

/**
 * Check whether an actual version is upgradable to an expected version.
 * An actual version is upgradable if its major number is expected but
 * its minor number differs, and the expected version's minor number
 * is greater than the actual version's minor number.
 *
 * @param expected_version The expected version
 * @param actual_version   The version being validated
 *
 * @return <code>true</code> if the actual version is upgradable
 **/
static inline bool is_upgradable_version(struct version_number expected_version,
					 struct version_number actual_version)
{
	return ((expected_version.major_version == actual_version.major_version)
		&& (expected_version.minor_version > actual_version.minor_version));
}

/**
 * Check whether a version matches an expected version. Logs an error
 * describing a mismatch.
 *
 * @param expected_version  The expected version
 * @param actual_version    The version being validated
 * @param component_name    The name of the component or the calling function
 *                          (for error logging)
 *
 * @return VDO_SUCCESS             if the versions are the same
 *         VDO_UNSUPPORTED_VERSION if the versions don't match
 **/
int validate_version(struct version_number expected_version,
		     struct version_number actual_version,
		     const char *component_name)
	__attribute__((warn_unused_result));

/**
 * Check whether a header matches expectations. Logs an error describing the
 * first mismatch found.
 *
 * @param expected_header  The expected header
 * @param actual_header    The header being validated
 * @param exact_size       If true, the size fields of the two headers must be
 *                         the same, otherwise actualSize >= expectedSize is OK
 * @param component_name   The name of the component or the calling function
 *                         (for error logging)
 *
 * @return VDO_SUCCESS             if the header meets expectations
 *         VDO_INCORRECT_COMPONENT if the component ids don't match
 *         VDO_UNSUPPORTED_VERSION if the versions or sizes don't match
 **/
int validate_header(const struct header *expected_header,
		    const struct header *actual_header, bool exact_size,
		    const char *component_name)
	__attribute__((warn_unused_result));

/**
 * Encode a header into a buffer.
 *
 * @param header  The header to encode
 * @param buffer  The buffer in which to encode the header
 *
 * @return UDS_SUCCESS or an error
 **/
int encode_header(const struct header *header, struct buffer *buffer)
	__attribute__((warn_unused_result));

/**
 * Encode a version number into a buffer.
 *
 * @param version  The version to encode
 * @param buffer   The buffer in which to encode the version
 *
 * @return UDS_SUCCESS or an error
 **/
int encode_version_number(struct version_number version, struct buffer *buffer)
	__attribute__((warn_unused_result));

/**
 * Decode a header from a buffer.
 *
 * @param [in]  buffer  The buffer from which to decode the header
 * @param [out] header  The header to decode
 *
 * @return UDS_SUCCESS or an error
 **/
int decode_header(struct buffer *buffer, struct header *header)
	__attribute__((warn_unused_result));

/**
 * Decode a version number from a buffer.
 *
 * @param buffer   The buffer from which to decode the version
 * @param version  The version structure to decode into
 *
 * @return UDS_SUCCESS or an error
 **/
int decode_version_number(struct buffer *buffer,
			  struct version_number *version)
	__attribute__((warn_unused_result));

/**
 * Convert a version_number to its packed on-disk representation.
 *
 * @param version  The version number to convert
 *
 * @return the platform-independent representation of the version
 **/
static inline struct packed_version_number
pack_version_number(struct version_number version)
{
	struct packed_version_number packed;
	storeUInt32LE(packed.major_version, version.major_version);
	storeUInt32LE(packed.minor_version, version.minor_version);
	return packed;
}

/**
 * Convert a packed_version_number to its native in-memory representation.
 *
 * @param version  The version number to convert
 *
 * @return the platform-independent representation of the version
 **/
static inline struct version_number
unpack_version_number(struct packed_version_number version)
{
	return (struct version_number) {
		.major_version = getUInt32LE(version.major_version),
		.minor_version = getUInt32LE(version.minor_version),
	};
}

#endif // HEADER_H

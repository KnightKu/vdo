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
 * $Id: //eng/uds-releases/lisa/src/uds/config.h#9 $
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "bufferedReader.h"
#include "bufferedWriter.h"
#include "geometry.h"
#include "nonce.h"
#include "uds.h"

enum {
	DEFAULT_VOLUME_INDEX_MEAN_DELTA = 4096,
	DEFAULT_CACHE_CHAPTERS = 7,
	DEFAULT_SPARSE_SAMPLE_RATE = 32,
	MAX_ZONES = 16,
};

/**
 * A set of configuration parameters for the indexer.
 **/
struct configuration {
	/** String describing the storage device */
	const char *name;

	/** The maximum allowable size of the index */
	size_t size;

	/** The offset where the index should start */
	off_t offset;

	/* Parameters for the volume */

	/* The volume layout */
	struct geometry *geometry;

	/** Index owner's nonce */
	uds_nonce_t nonce;

	/* The number of threads used to process index requests */
	unsigned int zone_count;

	/* The number of threads used to read volume pages */
	unsigned int read_threads;

	/*
	 * Size of the page cache and sparse chapter index cache, in
	 * chapters
	 */
	unsigned int cache_chapters;

	/** Parameters for the volume index */

	/* The mean delta for the volume index */
	unsigned int volume_index_mean_delta;

	/* Sampling rate for sparse indexing */
	unsigned int sparse_sample_rate;
};

/**
 * On-disk structure of data for a 8.02 index.
 **/
struct uds_configuration_8_02 {
	/** Smaller (16), Small (64) or large (256) indices */
	unsigned int record_pages_per_chapter;
	/** Total number of chapters per volume */
	unsigned int chapters_per_volume;
	/** Number of sparse chapters per volume */
	unsigned int sparse_chapters_per_volume;
	/** Size of the page cache, in chapters */
	unsigned int cache_chapters;
	/** Unused field */
	unsigned int unused;
	/** The volume index mean delta to use */
	unsigned int volume_index_mean_delta;
	/** Size of a page, used for both record pages and index pages */
	unsigned int bytes_per_page;
	/** Sampling rate for sparse indexing */
	unsigned int sparse_sample_rate;
	/** Index Owner's nonce */
	uds_nonce_t nonce;
	/** Virtual chapter remapped from physical chapter 0 */
	uint64_t remapped_virtual;
	/** New physical chapter which remapped chapter was moved to */
	uint64_t remapped_physical;
};

/**
 * On-disk structure of data for a 6.02 index.
 **/
struct uds_configuration_6_02 {
	/** Smaller (16), Small (64) or large (256) indices */
	unsigned int record_pages_per_chapter;
	/** Total number of chapters per volume */
	unsigned int chapters_per_volume;
	/** Number of sparse chapters per volume */
	unsigned int sparse_chapters_per_volume;
	/** Size of the page cache, in chapters */
	unsigned int cache_chapters;
	/** Unused field */
	unsigned int unused;
	/** The volume index mean delta to use */
	unsigned int volume_index_mean_delta;
	/** Size of a page, used for both record pages and index pages */
	unsigned int bytes_per_page;
	/** Sampling rate for sparse indexing */
	unsigned int sparse_sample_rate;
	/** Index Owner's nonce */
	uds_nonce_t nonce;
};

/**
 * Construct a new index configuration.
 *
 * @param params      The user parameters
 * @param config_ptr  The new index configuration
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check make_configuration(const struct uds_parameters *params,
				    struct configuration **config_ptr);

/**
 * Clean up the configuration struct.
 **/
void free_configuration(struct configuration *config);

/**
 * Read the index configuration from stable storage, and validate it against
 * the provided configuration.
 *
 * @param [in]     reader  A buffered reader
 * @param [in,out] config  The index configuration
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check validate_config_contents(struct buffered_reader *reader,
					  struct configuration *config);

/**
 * Write the index configuration information to stable storage. If
 * the superblock version is < 4 write the 6.02 version; otherwise
 * write the 8.02 version, indicating the configuration is for an
 * index that has been reduced by one chapter.
 * 
 * @param writer   A buffered writer
 * @param config   The index configuration
 * @param version  The index superblock version
 *
 * @return UDS_SUCCESS or an error code.
 **/
int __must_check write_config_contents(struct buffered_writer *writer,
				       struct configuration *config,
				       uint32_t version);

/**
 * Log an index configuration.
 *
 * @param conf  The configuration
 **/
void log_uds_configuration(struct configuration *conf);

#endif /* CONFIG_H */

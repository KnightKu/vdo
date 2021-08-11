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
 * $Id: //eng/uds-releases/lisa/src/uds/regionIdentifiers.h#1 $
 */

#ifndef REGION_IDENTIFIERS_H
#define REGION_IDENTIFIERS_H

enum region_type {
	RH_TYPE_FREE = 0, // unused
	RH_TYPE_SUPER = 1,
	RH_TYPE_SAVE = 2,
	RH_TYPE_CHECKPOINT = 3,
	RH_TYPE_UNSAVED = 4,
};

enum region_kind {
	RL_KIND_SCRATCH = 0, // uninitialized or scrapped
	RL_KIND_HEADER = 1, // for self-referential items
	RL_KIND_CONFIG = 100,
	RL_KIND_INDEX = 101,
	RL_KIND_SEAL = 102,
	RL_KIND_VOLUME = 201,
	RL_KIND_SAVE = 202,
	RL_KIND_INDEX_PAGE_MAP = 301,
	RL_KIND_VOLUME_INDEX = 302,
	RL_KIND_OPEN_CHAPTER = 303,
	RL_KIND_INDEX_STATE = 401, // not saved as region
};

enum {
	RL_SOLE_INSTANCE = 65535,
};

#endif // REGION_IDENTIFIERS_H

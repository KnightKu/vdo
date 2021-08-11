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
 * $Id: //eng/uds-releases/lisa/src/uds/zone.h#1 $
 */

#ifndef ZONE_H
#define ZONE_H

#include "compiler.h"
#include "uds.h"

enum {
	MAX_ZONES = 16,
};

/**
 * Return the number of zones.
 *
 * @param user_params  the index session parameters.  If NULL, the default
 *                     session parameters will be used.
 *
 * @return the number of zones
 **/
unsigned int __must_check
get_zone_count(const struct uds_parameters *user_params);

#endif /* ZONE_H */

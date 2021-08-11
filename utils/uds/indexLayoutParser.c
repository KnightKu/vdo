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
 * $Id: //eng/uds-releases/lisa/src/uds/indexLayoutParser.c#1 $
 */

#include "indexLayoutParser.h"

#include "errors.h"
#include "logger.h"
#include "permassert.h"
#include "stringUtils.h"
#include "typeDefs.h"
#include "uds.h"

/**********************************************************************/
static int __must_check set_parameter_value(struct layout_parameter *lp,
					    char *data)
{
	if ((lp->type & LP_TYPE_MASK) == LP_UINT64) {
		int result = uds_parse_uint64(data, lp->value.num);
		if (result != UDS_SUCCESS) {
			uds_log_error("bad numeric value %s", data);
			return -EINVAL;
		}
	} else if ((lp->type & LP_TYPE_MASK) == LP_STRING) {
		*lp->value.str = data;
	} else {
		uds_log_error("unknown layout parameter type code %x",
			      (lp->type & LP_TYPE_MASK));
		return -EINVAL;
	}
	return UDS_SUCCESS;
}

/**********************************************************************/
int parse_layout_string(char *info, struct layout_parameter *params)
{
	if (!strchr(info, '=')) {
		struct layout_parameter *lp;
		for (lp = params; lp->type != LP_NULL; ++lp) {
			if (lp->type & LP_DEFAULT) {
				int result = set_parameter_value(lp, info);
				if (result != UDS_SUCCESS) {
					return result;
				}
				break;
			}
		}
	} else {
		char *data = NULL;
		char *token;
		for (token = uds_next_token(info, " ", &data); token;
		     token = uds_next_token(NULL, " ", &data)) {
			int result;
			char *equal = strchr(token, '=');
			struct layout_parameter *lp;
			for (lp = params; lp->type != LP_NULL; ++lp) {
				if (!equal && (lp->type & LP_DEFAULT)) {
					break;
				} else if (strncmp(token,
						   lp->name,
						   equal - token) == 0 &&
					   strlen(lp->name) ==
						   (size_t)(equal - token)) {
					break;
				}
			}
			if (lp->type == LP_NULL) {
				uds_log_error("unknown index parameter %s",
					      token);
				return -EINVAL;
			}
			if (lp->seen) {
				uds_log_error("duplicate index parameter %s",
					      token);
				return -EINVAL;
			}
			lp->seen = true;
			result = set_parameter_value(
				lp, equal ? equal + 1 : token);
			if (result != UDS_SUCCESS) {
				return result;
			}
		}
	}
	return UDS_SUCCESS;
}

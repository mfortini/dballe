/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBA_FORMATTER_H
#define DBA_FORMATTER_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <dballe/core/error.h>

/**
 * Describe a level layer.
 *
 * @param ltype  The level type
 * @param l1 The l1 value for the level
 * @retval buf
 *   The formatted layer description, on a newly allocated string.  It is the
 *   responsibility of the caller to deallocate the string.
 * @returns
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_formatter_describe_level(int ltype, int l1, char** buf);

/**
 * Describe a level or a layer.
 *
 * @param ltype1  The type of the first level
 * @param l1 The value of the first level
 * @param ltype2  The type of the second level
 * @param l2 The value of the second level
 * @retval buf
 *   The formatted layer description, on a newly allocated string.  It is the
 *   responsibility of the caller to deallocate the string.
 * @returns
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_formatter_describe_level_or_layer(int ltype1, int l1, int ltype2, int l2, char** buf);

/**
 * Describe a time range.
 *
 * @param ptype  The time range type
 * @param p1 The p1 value for the time range
 * @param p2 The p2 value for the time range
 * @retval buf
 *   The formatted time range description, on a newly allocated string.  It is
 *   the responsibility of the caller to deallocate the string.
 * @returns
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_formatter_describe_trange(int ptype, int p1, int p2, char** buf);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif

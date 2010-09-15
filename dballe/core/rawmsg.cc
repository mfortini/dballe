/*
 * dballe/rawmsg - annotated raw buffer
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "rawmsg.h"
#include "file.h"

#include <stdlib.h> /* malloc */
#include <string.h> /* memcpy */

namespace dballe {

const char* encoding_name(Encoding enc)
{
	switch (enc)
	{
		case BUFR: return "BUFR";
		case CREX: return "CREX";
		case AOF: return "AOF";
		default: return "(unknown)";
	}
}

Rawmsg::Rawmsg()
	: file(0), offset(0), index(0), encoding(BUFR)
{
}

Rawmsg::~Rawmsg()
{
}

std::string Rawmsg::filename() const throw ()
{
	if (!file) return "(memory)";
	return file->name();
}

void Rawmsg::clear() throw ()
{
	file = 0;
	offset = 0;
	index = 0;
	encoding = BUFR;
	std::string::clear();
}

}

/* vim:set ts=4 sw=4: */
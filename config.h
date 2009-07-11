/** \file
 * Key/value parser until we have a proper config
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _config_h_
#define _config_h_

#define MAX_NAME_LEN		32
#define MAX_VALUE_LEN		32


struct config
{
	struct config *		next;
	char			name[ MAX_NAME_LEN ];
	char			value[ MAX_VALUE_LEN ];
};

extern struct config * global_config;

extern struct config *
config_parse(
	FILE *			file
);


extern char *
config_value(
	struct config *		config,
	const char *		name
);

extern int
config_int(
	struct config *		config,
	const char *		name,
	int			def
);


extern struct config *
config_parse_file(
	const char *		filename
);


#endif

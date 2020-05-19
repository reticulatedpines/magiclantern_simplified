/*
 * Copyright (C) 2013 Magic Lantern Team
 *
 * This file is part of Magic Lantern.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __STUB_H__
#define __STUB_H__

#define NSTUB(addr,name) \
	.global name; \
	.extern name; \
	name = addr

#endif /* __STUB_H__ */

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

/* for 32-bit ARM functions */
#define ARM32_FN(addr,name) \
	.global name; \
    .type name, %function; \
	.extern name; \
	name = (addr) & ~3

/* for 16/32-bit Thumb functions */
#define THUMB_FN(addr,name) \
	.global name; \
	.extern name; \
	name = (addr) | 1

/* for data structures */
#define DATA_PTR(addr,name) \
	.global name; \
	.extern name; \
	name = addr

/* deprecated */
/* FIXME: how to print a warning when compiling old code? */
#define NSTUB(addr,name) \
	.global name; \
	.extern name; \
	name = addr

#endif /* __STUB_H__ */

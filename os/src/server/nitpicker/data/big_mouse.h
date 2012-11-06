/*
 * \brief  Mouse cursor pixel data
 * \author Norman Feske
 * \date   2006-08-09
 */

/*
 * Copyright (C) 2006-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

struct mouse_cursor {
	unsigned short w, h, pixels[16][16];
};


static struct mouse_cursor big_mouse = {
	16,16,
	{
		{0x738E,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x94B2,0x7BCF,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x630C,0xC638,0xC638,0x6B4D,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x738E,0x8C71,0xFFFF,0xB5B6,0x6B4D,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x630C,0x4A49,0x630C,0xB5B6,0xFFFF,0xB5B6,0x630C,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x528A,0x528A,0x630C,0x9492,0xB5B6,0xFFFF,0xB5B6,0x630C,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x528A,0x39C7,0x4208,0x630C,0x7BCF,0xB5B6,0xFFFF,0xFFFF,0xB5B6,0x630C,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x4208,0x39C7,0x4208,0x630C,0x7BCF,0xB5B6,0xDEFB,0xFFFF,0xFFFF,0xB5B6,0x630C,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x4A49,0x1082,0x39C7,0x4208,0x5ACB,0x7BCF,0x8C71,0xAD75,0x630C,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x4208,0x1082,0x39C7,0x5ACB,0x630C,0xB5B6,0x0000,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x4A49,0x1082,0x1082,0x39C7,0x4A49,0x630C,0xAD75,0x0000,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x4208,0x1082,0x4A49,0x0000,0x4A49,0x630C,0x8C71,0x0000,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x4A49,0x39C7,0x4A49,0x0000,0x0000,0x4A49,0x630C,0x8C71,0x0000,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x4A49,0x0000,0x0000,0x0000,0x0000,0x4A49,0x630C,0x8C71,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x4A49,0x0000,0x0000,0x0000,0x0000,0x0000,0x4A49,0x4A49,0x0000},
		{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
	}
};

static struct mouse_cursor no_mouse = {
	1,1,
	{
		{0}
	}
};

/*
 * \brief  OMAP4 common IO registers taken from u-boot, linux and TRM
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-01-15
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _OMAP4_IO_COMMON_H_
#define _OMAP4_IO_COMMON_H_

namespace Omap4 {

enum {
	OMAP44XX_L4_CORE_BASE = 0x4A000000,
};

/* CONTROL */
enum {
	CTRL_BASE = OMAP44XX_L4_CORE_BASE + 0x2000,
	CONTROL_PADCONF_CORE = OMAP44XX_L4_CORE_BASE + 0x100000,
	CONTROL_PADCONF_CORE_SIZE = 0x1000,
	CONTROL_PADCONF_WKUP = OMAP44XX_L4_CORE_BASE + 0x31E000,
	CONTROL_PADCONF_WKUP_SIZE = 0x1000,
};

} //namespace Omap4

#endif /* _OMAP4_IO_COMMON_H_ */

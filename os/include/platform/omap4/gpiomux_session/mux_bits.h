/*
 * \brief  OMAP4 GPIO MUX register definitions
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-01-15
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * based on mux.h from linux kernel which is
 * Copyright (C) 2009 Nokia
 * Copyright (C) 2009-2010 Texas Instruments
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__MUX_BITS_H_
#define _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__MUX_BITS_H_

namespace Omap4 {
	namespace GpioMux {

		enum {
			/* 34xx mux mode options for each pin. See TRM for options */
			MUX_MODE0      = 0,
			MUX_MODE1      = 1,
			MUX_MODE2      = 2,
			MUX_MODE3      = 3,
			MUX_MODE4      = 4,
			MUX_MODE5      = 5,
			MUX_MODE6      = 6,
			MUX_MODE7      = 7,

			/* 24xx/34xx mux bit defines */
			PULL_ENA			= (1 << 3),
			PULL_UP			= (1 << 4),
			ALTELECTRICALSEL	= (1 << 5),

			/* 34xx specific mux bit defines */
			INPUT_EN			= (1 << 8),
			OFF_EN				= (1 << 9),
			OFFOUT_EN			= (1 << 10),
			OFFOUT_VAL			= (1 << 11),
			OFF_PULL_EN		= (1 << 12),
			OFF_PULL_UP		= (1 << 13),
			WAKEUP_EN			= (1 << 14),

			/* 44xx specific mux bit defines */
			WAKEUP_EVENT		= (1 << 15),

			/* Active pin states */
			PIN_OUTPUT			= 0,
			PIN_INPUT			= INPUT_EN,
			PIN_INPUT_PULLUP	= (PULL_ENA | INPUT_EN \
								| PULL_UP),
			PIN_INPUT_PULLDOWN	= (PULL_ENA | INPUT_EN),

			/* Off mode states */
			PIN_OFF_NONE			= 0,
			PIN_OFF_OUTPUT_HIGH 	= (OFF_EN | OFFOUT_EN \
								| OFFOUT_VAL),
			PIN_OFF_OUTPUT_LOW		= (OFF_EN | OFFOUT_EN),
			PIN_OFF_INPUT_PULLUP	= (OFF_EN | OFF_PULL_EN \
								| OFF_PULL_UP),
			PIN_OFF_INPUT_PULLDOWN	= (OFF_EN | OFF_PULL_EN),
			PIN_OFF_WAKEUPENABLE	= WAKEUP_EN,
		};
	} //namespace GpioMux
} //namespace Omap4

#endif //_INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__MUX_BITS_H_

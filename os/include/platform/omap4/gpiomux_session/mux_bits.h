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

/* 34xx mux mode options for each pin. See TRM for options */
#define OMAP_MUX_MODE0      0
#define OMAP_MUX_MODE1      1
#define OMAP_MUX_MODE2      2
#define OMAP_MUX_MODE3      3
#define OMAP_MUX_MODE4      4
#define OMAP_MUX_MODE5      5
#define OMAP_MUX_MODE6      6
#define OMAP_MUX_MODE7      7

/* 24xx/34xx mux bit defines */
#define OMAP_PULL_ENA			(1 << 3)
#define OMAP_PULL_UP			(1 << 4)
#define OMAP_ALTELECTRICALSEL		(1 << 5)

/* 34xx specific mux bit defines */
#define OMAP_INPUT_EN			(1 << 8)
#define OMAP_OFF_EN			(1 << 9)
#define OMAP_OFFOUT_EN			(1 << 10)
#define OMAP_OFFOUT_VAL			(1 << 11)
#define OMAP_OFF_PULL_EN		(1 << 12)
#define OMAP_OFF_PULL_UP		(1 << 13)
#define OMAP_WAKEUP_EN			(1 << 14)

/* 44xx specific mux bit defines */
#define OMAP_WAKEUP_EVENT		(1 << 15)

/* Active pin states */
#define OMAP_PIN_OUTPUT			0
#define OMAP_PIN_INPUT			OMAP_INPUT_EN
#define OMAP_PIN_INPUT_PULLUP		(OMAP_PULL_ENA | OMAP_INPUT_EN \
						| OMAP_PULL_UP)
#define OMAP_PIN_INPUT_PULLDOWN		(OMAP_PULL_ENA | OMAP_INPUT_EN)

/* Off mode states */
#define OMAP_PIN_OFF_NONE		0
#define OMAP_PIN_OFF_OUTPUT_HIGH	(OMAP_OFF_EN | OMAP_OFFOUT_EN \
						| OMAP_OFFOUT_VAL)
#define OMAP_PIN_OFF_OUTPUT_LOW		(OMAP_OFF_EN | OMAP_OFFOUT_EN)
#define OMAP_PIN_OFF_INPUT_PULLUP	(OMAP_OFF_EN | OMAP_OFF_PULL_EN \
						| OMAP_OFF_PULL_UP)
#define OMAP_PIN_OFF_INPUT_PULLDOWN	(OMAP_OFF_EN | OMAP_OFF_PULL_EN)
#define OMAP_PIN_OFF_WAKEUPENABLE	OMAP_WAKEUP_EN

#endif //_INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__MUX_BITS_H_

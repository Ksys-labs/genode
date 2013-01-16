/*
 * \brief  Tests for GPIO MUX interface on omap4
 * \author Alexander Tarasikov
 * \date   2013-01-15
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <platform/omap4/gpiomux_session/connection.h>
#include <platform/omap4/gpiomux_session/mux_bits.h>
#include <base/printf.h>

using namespace Genode;

int main()
{
	PINF("+++ Test OMAP4 GPIO MUX +++");

	static GpioMux::Connection mux;
	mux.init_signal("i2c1_scl", OMAP_PIN_INPUT_PULLUP);

	PINF("--- Test OMAP4 GPIO MUX ---");

	return 0;
}


/*
 * \brief  GPIO MUX driver for OMAP4
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

#ifndef _DRIVER_H_
#define _DRIVER_H_

/* Genode includes */
#include <io_mem_session/connection.h>
#include <util/mmio.h>
#include <platform/omap4/io_common.h>

/* local includes */
#include "mux44xx_io.h"

namespace GpioMux {
	using namespace Genode;
	using namespace Omap4;
	class Driver;
}

static int verbose = 1;

class GpioMux::Driver
{
	private:
		Io_mem_connection _io_core;
		Io_mem_connection _io_wkup;
		void *_core_base;
		void *_wkup_base;
	
		struct omap_mux *_mux_core;
		struct omap_mux *_mux_wkup;
		struct omap_mux *_mux_subset;
		
		bool mux_has_pad(struct omap_mux *mux, const char *name);
		void config_pad(struct omap_mux *mux, int value);
		struct omap_mux *find_mux_by_gpio(int gpio);
		struct omap_mux *find_mux_by_name(const char *name);

	public:
		enum revision {
			OMAP4_ES10,
			OMAP4_ES2X,
		};

		Driver(enum revision);
		bool init_gpio(int gpio, int value);
		bool init_signal(char *name, int value);
};

GpioMux::Driver::Driver(enum revision revision) :
	_io_core(CONTROL_PADCONF_CORE, CONTROL_PADCONF_CORE_SIZE),
	_io_wkup(CONTROL_PADCONF_WKUP, CONTROL_PADCONF_WKUP_SIZE),
	_core_base(env()->rm_session()->attach(_io_core.dataspace())),
	_wkup_base(env()->rm_session()->attach(_io_wkup.dataspace())),
	_mux_core(omap4_core_muxmodes),
	_mux_wkup(omap4_wkup_muxmodes),
	_mux_subset(NULL)
{
	switch (revision) {
		case OMAP4_ES10:
			break;
		case OMAP4_ES2X:
			_mux_subset = omap4_es2_core_subset;
			break;
		default:
			_mux_core = NULL;
			_mux_wkup = NULL;
			PERR("unknown revision %d", revision);
			break;
	}
}

bool GpioMux::Driver::mux_has_pad(struct omap_mux *mux, const char *name) {
	if (!mux || !name) {
		return false;
	}

	for (int i = 0; i < OMAP_MUX_NR_MODES; i++) {
		if (!mux->muxnames[i]) {
			continue;
		}
		if (!Genode::strcmp(mux->muxnames[i], name)) {
			return true;
		}
	}

	return false;
}

struct GpioMux::omap_mux *GpioMux::Driver::find_mux_by_gpio(int gpio) {
	struct omap_mux *core = _mux_core;
	struct omap_mux *subset = _mux_subset;

	if (verbose) {
		PDBG("find_mux_by_gpio %d", gpio);
	}

	while (subset && subset->reg_offset != NO_GPIO) {
		if (subset->gpio == gpio) {
			return subset;
		}
		subset++;
	}

	while (core && core->reg_offset != NO_GPIO) {
		if (core->gpio == gpio) {
			return core;
		}
		core++;
	}

	return NULL;
}

struct GpioMux::omap_mux *GpioMux::Driver::find_mux_by_name(const char *name) {
	struct omap_mux *core = _mux_core;
	struct omap_mux *subset = _mux_subset;
	
	if (verbose) {
		PDBG("find_mux_by_name %s", name);
	}

	while (subset && subset->reg_offset != NO_GPIO) {
		if (mux_has_pad(subset, name)) {
			return subset;
		}
		subset++;
	}

	while (core && core->reg_offset != NO_GPIO) {
		if (mux_has_pad(core, name)) {
			return core;
		}
		core++;
	}

	return NULL;
}

void GpioMux::Driver::config_pad(struct GpioMux::omap_mux *mux, int value) {
	uint16_t *cfg_reg = (uint16_t*)(((uint8_t*)_core_base) + mux->reg_offset);

	if (verbose) {
		PDBG("gpio config [%x] %x -> %x", mux->reg_offset, *cfg_reg, value);
	}

	*cfg_reg = value;
}

bool GpioMux::Driver::init_gpio(int gpio, int value) {
	if (verbose) {
		PDBG("init_gpio %d %x", gpio, value);
	}

	struct omap_mux *mux = find_mux_by_gpio(gpio);
	if (!mux) {
		PERR("failed to find mux for gpio %d", gpio);
		return false;
	}
	
	config_pad(mux, value);
	return true;
}

bool GpioMux::Driver::init_signal(char *name, int value) {
	if (verbose) {
		PDBG("init_signal %s %x", name, value);
	}

	struct omap_mux *mux = find_mux_by_name(name);
	if (!mux) {
		PERR("failed to find mux for signal %s", name);
		return false;
	}

	config_pad(mux, value);
	return true;
}

#endif /* _DRIVER_H_ */

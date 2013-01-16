/*
 * \brief  GPIO MUX implementation for OMAP4
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
#include <platform/omap4/gpiomux_session/gpiomux_session.h>
#include <cap_session/connection.h>
#include <dataspace/client.h>
#include <base/printf.h>
#include <base/sleep.h>
#include <root/component.h>
#include <os/attached_ram_dataspace.h>
#include <os/static_root.h>

/* local includes */
#include "driver.h"

namespace GpioMux {
	using namespace Genode;
	class Session_component;
}

class GpioMux::Session_component : public Genode::Rpc_object<GpioMux::Session>
{
	private:
		Driver &_driver;
		Attached_ram_dataspace _io_buffer;

	public:
		Session_component(Driver &driver) :
			_driver(driver),
			_io_buffer(env()->ram_session(), GpioMux::Session::IO_BUFFER_SIZE)
		{}
		
		/************************************
		 ** GpioMux::Session interface **
		 ************************************/

		bool init_gpio(int gpio, int value) {
			return _driver.init_gpio(gpio, value);
		}

		bool init_signal(char *name, int value) {
			char *buf = _io_buffer.local_addr<char>();
			return _driver.init_signal(buf, value);
		}

		Dataspace_capability dataspace() {
			return _io_buffer.cap();
		}
};

int main(int, char**)
{
	using namespace GpioMux;

	PINF("OMAP4 GPIO MUX driver");

	Driver driver(Driver::OMAP4_ES2X);

	/*
	 * Initialize server entry point
	 */
	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "gpiomux_ep");

	/*
	 * Let the entry point serve the mux session and root interfaces
	 */
	static Session_component          mux_session(driver);
	static Static_root<GpioMux::Session> mux_root(ep.manage(&mux_session));

	/*
	 * Announce service
	 */
	env()->parent()->announce(ep.manage(&mux_root));
	Genode::sleep_forever();
	return 0;
}


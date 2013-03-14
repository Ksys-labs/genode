/*
 * \brief  Terminal multiplexing service
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2013-02-26
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <cap_session/connection.h>
#include <base/printf.h>
#include <base/rpc_server.h>
#include <base/sleep.h>

/* local includes */
#include "terminal_root.h"
#include "mux.h"

int main(int argc, char **argv)
{
	using namespace Genode;
	
	PINF("--- Terminalmux started ---\n");

	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, Terminal::STACK_SIZE, "terminal_ep");
	
	try {
		static Terminal::Connection terminal;
		
		static Mux                  mux(&terminal);

		static Terminal::Root       terminal_root(&ep, env()->heap(), &mux);
		env()->parent()->announce(ep.manage(&terminal_root));
		
	} catch (Parent::Service_denied) {
		PERR("Could not connect to uplink Terminal");
		return 1;
	}
	
	sleep_forever();
	return 0;
}
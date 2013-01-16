/*
 * \brief  OMAP4 GPIO MUX session interface type
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

#ifndef _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__GPIOMUX_SESSION_H_
#define _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__GPIOMUX_SESSION_H_

#include <base/signal.h>
#include <session/session.h>
#include <dataspace/capability.h>

namespace GpioMux {

	using namespace Genode;

	struct Session : Genode::Session
	{
		enum { IO_BUFFER_SIZE = 4096 };
		static const char *service_name() { return "GpioMux"; }

		virtual ~Session() { }

		virtual bool init_gpio(int gpio, int value) = 0;
		virtual bool init_signal(char *name, int value) = 0;
		virtual Genode::Dataspace_capability dataspace(void) = 0;

		/*******************
		 ** RPC interface **
		 *******************/

		GENODE_RPC(Rpc_init_gpio, bool, init_gpio, int, int);
		GENODE_RPC(Rpc_init_signal, bool, init_signal, char*, int);
		GENODE_RPC(Rpc_dataspace, Genode::Dataspace_capability, dataspace);

		GENODE_RPC_INTERFACE(
			Rpc_init_gpio,
			Rpc_init_signal,
			Rpc_dataspace
		);
	};
}

#endif /* _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__GPIOMUX_SESSION_H_ */

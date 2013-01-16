/*
 * \brief  OMAP4 GPIO MUX session connection
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

#ifndef _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CONNECTION_H_
#define _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CONNECTION_H_

#include <platform/omap4/gpiomux_session/client.h>
#include <base/connection.h>

namespace GpioMux {

	struct Connection : Genode::Connection<Session>, Session_client
	{
		Connection()
		:
			Genode::Connection<Session>(session("ram_quota=4K")),
			Session_client(cap())
		{ }
	};
}

#endif /* _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CONNECTION_H_ */

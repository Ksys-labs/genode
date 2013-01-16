/*
 * \brief  OMAP4 GPIO MUX session interface
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-01-15
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CLIENT_H_
#define _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CLIENT_H_

#include <platform/omap4/gpiomux_session/capability.h>
#include <base/rpc_client.h>
#include <dataspace/capability.h>
#include <base/env.h>

namespace GpioMux {

	struct Session_client : Genode::Rpc_client<Session>
	{
	private:
		struct Io_buffer
		{
			Genode::Dataspace_capability ds_cap;
			char						*base;
			Genode::size_t			   size;

			Io_buffer(Genode::Dataspace_capability ds_cap)
			:
				ds_cap(ds_cap),
				base(Genode::env()->rm_session()->attach(ds_cap)),
				size(ds_cap.call<Genode::Dataspace::Rpc_size>())
			{ }

			~Io_buffer()
			{
				Genode::env()->rm_session()->detach(base);
			}
		};
		Io_buffer _io_buffer;
	public:
		explicit Session_client(Session_capability session) :
			Genode::Rpc_client<Session>(session),
			_io_buffer(call<Rpc_dataspace>())
		{ }

		bool init_gpio(int gpio, int value) {
			return call<Rpc_init_gpio>(gpio, value);
		}
		
		bool init_signal(char *name, int value) {
			size_t len = 1 + Genode::strlen(name);
			if (len >= _io_buffer.size) {
				PERR("len (%d) exceeds the buffer size (%d)",
					len, _io_buffer.size);
				return -1;
			}

			Genode::memcpy(_io_buffer.base, name, len);
			return call<Rpc_init_signal>(_io_buffer.base, value);
		}

		Dataspace_capability dataspace(void) {
			return _io_buffer.ds_cap;
		}
	};
}

#endif /* _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CLIENT_H_ */

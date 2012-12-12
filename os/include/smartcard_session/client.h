/*
 * \brief  Client-side Smartcard session interface
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__SMARTCARD_SESSION_H__CLIENT_H_
#define _INCLUDE__SMARTCARD_SESSION_H__CLIENT_H_

#include <util/string.h>
#include <base/env.h>
#include <base/rpc_client.h>

#include <smartcard_session/smartcard_session.h>

namespace Smartcard {

	class Session_client : public Genode::Rpc_client<Session>
	{
		struct Io_buffer
		{
			Genode::Dataspace_capability ds_cap;
			char                        *base;
			Genode::size_t               size;

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
		explicit Session_client(Genode::Capability<Session> cap)
		:
			Genode::Rpc_client<Session>(cap),
			_io_buffer(call<Rpc_dataspace>())
		{ }

		ReaderStatus reader_status() const {
			call<Rpc_reader_status>();
			return *reinterpret_cast<ReaderStatus*>(_io_buffer.base);
		}

		bool activate_card()   { return call<Rpc_activate_card>(); }
		bool deactivate_card() { return call<Rpc_deactivate_card>(); }

		bool is_card_present() { return call<Rpc_is_card_present>(); }


		bool transmit(void const *send_buf, Genode::size_t send_length,
							  void *recv_buf, Genode::size_t *recv_length)
		{
			char const * const src           = (char const *)send_buf;

			Genode::memcpy(_io_buffer.base, src, send_length);

			/* tell server to pick up new I/O buffer content */
			Genode::size_t     num_bytes = call<Rpc_transmit>(send_length);

			Genode::memcpy(recv_buf, _io_buffer.base, num_bytes);

			*recv_length = num_bytes;

			return (num_bytes > 0);
		}


	};
}

#endif /* _INCLUDE__SMARTCARD_SESSION_H__CLIENT_H_ */
/*
 * \brief  Smartcard component
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SMARTCARD_COMPONENT_H_
#define _SMARTCARD_COMPONENT_H_

/* Genode includes */
#include <base/rpc_server.h>
#include <os/attached_ram_dataspace.h>
#include <root/component.h>
#include <smartcard_session/smartcard_session.h>

/* local includes */
#include "smartcard_driver.h"

namespace Smartcard {

	using namespace Genode;

	class Session_component : public Rpc_object<Smartcard::Session, Session_component>
	{
		private:

			/*
			 * XXX Do not use hard-coded value, better make it dependent
			 *     on the RAM quota donated by the client.
			 */
			enum { IO_BUFFER_SIZE = 4096 };

			Genode::Attached_ram_dataspace _io_buffer;

			Smartcard::Driver         &_driver;

		public:

			/**
			 * Constructor
			 */
			Session_component(Driver &driver)
			:
				_io_buffer(Genode::env()->ram_session(), IO_BUFFER_SIZE),
				_driver(driver)
			{ }


			/********************************
			 ** Smartcard session interface **
			 ********************************/

			void _reader_status()
			{
				void *io_buf = _io_buffer.local_addr<void>();
				_driver.reader_status(reinterpret_cast<ReaderStatus*>(io_buf));
			}

			bool is_card_present() { return _driver.is_card_present(); }

			bool activate_card() { return _driver.activate_card(); }
			bool deactivate_card() { return _driver.deactivate_card(); }

			Genode::Dataspace_capability _dataspace() { return _io_buffer.cap(); }

			Genode::size_t _transmit(Genode::size_t num_bytes)
			{
				char *io_buf = _io_buffer.local_addr<char>();
				return _driver.transmit(io_buf, num_bytes, io_buf);
			}


			ReaderStatus reader_status() const {
				return ReaderStatus();
			}

			bool transmit(void const *cmd_buf, Genode::size_t cmd_length,
			             void *resp_buf, Genode::size_t *resp_length)
			{ return false; }
	};

	typedef Root_component<Session_component, Single_client> Root_component;


	class Root : public Root_component
	{
	private:

		Driver &_driver;

	protected:

		Session_component *_create_session(const char *args)
		{
			return new (md_alloc())
					Session_component(_driver);
		}

	public:

		/**
		 * Constructor
		 */
		Root(Rpc_entrypoint *ep, Allocator *md_alloc, Driver &driver)
		:
			Root_component(ep, md_alloc), _driver(driver)
		{ }
	};
}

#endif /* _TERMINAL_COMPONENT_H_ */

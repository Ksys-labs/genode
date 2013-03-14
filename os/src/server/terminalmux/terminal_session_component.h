/*
 * \brief  Terminal session component for Terminalmux
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

#ifndef _TERMINAL_SESSION_COMPONENT_H_
#define _TERMINAL_SESSION_COMPONENT_H_

/* Genode includes */
#include <base/rpc_server.h>
#include <base/lock.h>
#include <os/attached_ram_dataspace.h>
#include <os/ring_buffer.h>
#include <terminal_session/terminal_session.h>

/* local includes */
#include "channel_node.h"

class Mux;

namespace Terminal {

	using namespace Genode;

	enum { STACK_SIZE = sizeof(addr_t)*1024 };
	enum { BUFFER_SIZE = 4096 };

	class Session_component : public Rpc_object<Terminal::Session,
	                                            Session_component>
	{
		private:
			typedef Ring_buffer<unsigned char, BUFFER_SIZE+1> Local_buffer;

			Mux                      *_mux;
			int                       _channel;
			Channel_node              _channel_node;

			Attached_ram_dataspace    _io_buffer;

			Local_buffer              _buffer;
			size_t                    _read_num_bytes_avail;
			Lock                      _read_avail_lock;

			Signal_context_capability _read_avail_sigh;

		public:

			/**
			 * Constructor
			 */
			Session_component(Mux *mux, int channel);
			
			void char_avail(unsigned char *buf, size_t num_bytes);

			/********************************
			 ** Terminal session interface **
			 ********************************/
			Size size()  { return Size(0, 0); }

			bool avail() { return (_read_num_bytes_avail > 0); }
			Genode::Dataspace_capability _dataspace() { return _io_buffer.cap(); }

			Genode::size_t _read(Genode::size_t dst_len);

			void _write(Genode::size_t num_bytes);


			void connected_sigh(Genode::Signal_context_capability sigh);

			void read_avail_sigh(Genode::Signal_context_capability sigh);

			Genode::size_t read(void *, Genode::size_t)        { return 0; }
			Genode::size_t write(void const *, Genode::size_t) { return 0; }
	};

}

#endif /* _TERMINAL_SESSION_COMPONENT_H_ */

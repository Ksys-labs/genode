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


/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/signal.h>

/* local includes */
#include "terminal_session_component.h"
#include "mux.h"

using namespace Genode;

namespace Terminal {

Session_component::Session_component(Mux *mux, int channel)
: _mux(mux),
  _channel(channel),
  _channel_node(channel, this),
  _io_buffer(env()->ram_session(), BUFFER_SIZE),
  _read_num_bytes_avail(0)
{
	_mux->register_channel(&_channel_node);
}

void Session_component::char_avail(unsigned char *buf, size_t num_bytes)
{
	size_t num_bytes_written = 0;
	size_t src_index = 0;
	while (num_bytes_written < num_bytes)
	{
		try {
			_buffer.add(buf[src_index]);
			++src_index;
			++num_bytes_written;
		} catch(Local_buffer::Overflow) {
			_read_num_bytes_avail += num_bytes_written;
			num_bytes_written = 0;
			num_bytes -= num_bytes_written;
			if (_read_avail_sigh.valid())
				Signal_transmitter(_read_avail_sigh).submit();
			
			_read_avail_lock.lock();
		}
	}
	_read_num_bytes_avail += num_bytes_written;
	if (_read_avail_sigh.valid())
		Signal_transmitter(_read_avail_sigh).submit();
}

size_t Session_component::_read(size_t dst_len)
{
	size_t         num_bytes_read;
	unsigned char *dst = _io_buffer.local_addr<unsigned char>();

	for (num_bytes_read = 0;
	     (num_bytes_read < dst_len) && !_buffer.empty();
	     num_bytes_read++)
		dst[num_bytes_read] = _buffer.get();

	_read_num_bytes_avail -= num_bytes_read;

	_read_avail_lock.unlock();

	return num_bytes_read;
}


void Session_component::_write(size_t num_bytes)
{
	unsigned char *src = _io_buffer.local_addr<unsigned char>();
	
	size_t num_bytes_written = 0;
	while (num_bytes > 0)
	{
		int written = _mux->write(_channel, src + num_bytes_written, num_bytes);
		num_bytes_written += written;
		num_bytes -= written;
	}
}


void Session_component::connected_sigh(Signal_context_capability sigh)
{
	/*
	 * Immediately reflect connection-established signal to the
	 * client because the session is ready to use immediately after
	 * creation.
	 */
	Signal_transmitter(sigh).submit();
}


void Session_component::read_avail_sigh(Signal_context_capability sigh)
{
	_read_avail_sigh = sigh;
}

}

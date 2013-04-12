/*
 * \brief  Terminal multiplexer and demultiplexer
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

#include "mux.h"
#include "rx_thread.h"

Mux::Mux(Terminal::Connection *terminal)
:   _terminal(terminal),
	_io_buffer(Genode::env()->ram_session(), Terminal::BUFFER_SIZE),
	_io_buf(_io_buffer.local_addr<unsigned char>()),
	_tx_channel(-1),
	_rx_channel(-1)
{
	static Genode::Signal_receiver sig_rec;
	static Genode::Signal_context  sig_ctx;
	_terminal->read_avail_sigh(sig_rec.manage(&sig_ctx));

	static Rx_thread            rx_thread(this, &sig_rec);
};
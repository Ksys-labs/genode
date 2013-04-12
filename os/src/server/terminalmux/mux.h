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

#ifndef _MUX_H_
#define _MUX_H_

/* Genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <base/lock.h>
#include <util/avl_tree.h>
#include <terminal_session/connection.h>

/* local includes */
#include "channel_node.h"
#include "terminal_session_component.h"

class Mux 
{
	enum { verbose = 0 };
	
	private:
		Terminal::Connection           *_terminal;
		Genode::Lock                    lock;

		Genode::Attached_ram_dataspace  _io_buffer;
		unsigned char                  *_io_buf;
		
		int                             _tx_channel;
		int                             _rx_channel;
		
		Genode::Avl_tree<Channel_node>  _channel_tree;
		
		enum {
			CH_SWITCH_ESCAPE = 0x18,
			CHAN0_MUX_CHAR   = '0',
		};
		
		void tx_switch(int channel)
		{
			if (channel == _tx_channel)
				return;
			
			_io_buf[0] = CH_SWITCH_ESCAPE;
			_io_buf[1] = CHAN0_MUX_CHAR + channel;
			
			_terminal->write(_io_buf, 2);
			
			_tx_channel = channel;
		}
		
	public:
		Mux(Terminal::Connection *terminal);
		~Mux() {};
		
		void register_channel(Channel_node *node)
		{
			Genode::Lock::Guard _guard(lock);
			_channel_tree.insert( node );
		}
		
		Genode::size_t write(int channel, unsigned char *data, Genode::size_t num_bytes)
		{
			Genode::Lock::Guard _guard(lock);
			
			if (verbose)
				PDBG("channel%d: write(%d) %s", channel, num_bytes, data);
			
			tx_switch(channel);
			
			// check escape characters in data
			Genode::size_t proc_bytes = 0;
			Genode::size_t src_index = 0;
			while ((src_index < num_bytes) && (proc_bytes < Terminal::BUFFER_SIZE-1) )
			{
				if (data[src_index] == CH_SWITCH_ESCAPE)
					_io_buf[proc_bytes++] = CH_SWITCH_ESCAPE;
				
				_io_buf[proc_bytes++] = data[src_index++];
			}

			if (_terminal->write(_io_buf, proc_bytes) != proc_bytes)
				PWRN("Lost data");
			
			return num_bytes;
		}
		
		void char_avail()
		{
			Genode::Lock::Guard _guard(lock);
			
			static bool esc_flag = false;

			Genode::size_t num_bytes = _terminal->read(_io_buf, Terminal::BUFFER_SIZE);
			Genode::size_t proc_bytes = 0;
			
			for(Genode::size_t num = 0; num < num_bytes; num++)
			{
				if (esc_flag)
				{
					esc_flag = false;
					if (_io_buf[num] >= CHAN0_MUX_CHAR)
					{
						int rx_channel = _io_buf[num] - CHAN0_MUX_CHAR;
						if (rx_channel != _rx_channel)
						{
							_rx_channel = rx_channel;
							if (verbose)
								PDBG("switch rx_channel to channel %d", _rx_channel);
						}
						continue;
					} else if (_io_buf[num] != CH_SWITCH_ESCAPE) {
						PWRN("bad command");
					}
				} else if  (_io_buf[num] == CH_SWITCH_ESCAPE) {
					esc_flag = true;
					continue;
				}

				_io_buf[proc_bytes++] = _io_buf[num];
			}
			
			if (proc_bytes > 0 && _rx_channel != -1)
			{
				_io_buf[proc_bytes] = 0;
				
				if (verbose)
					PDBG("channel%d: read(%d) %s", _rx_channel, proc_bytes, _io_buf);
				
				Channel_node *node = _channel_tree.first();
				if (node)
					node = node->find_by_channel(_rx_channel);
				if (node)
					node->transmit(_io_buf, proc_bytes);
			}
		}
};

#endif // _MUX_H_

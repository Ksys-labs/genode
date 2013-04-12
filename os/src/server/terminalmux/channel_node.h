/*
 * \brief  Channel-node.
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2013-02-26
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2010-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CHANNEL_NODE_H_
#define _CHANNEL_NODE_H_

#include <base/stdint.h>
#include <util/avl_tree.h>

namespace Terminal {
	class Session_component;
}

class Channel_node: public Genode::Avl_node<Channel_node>
{
	private:
		unsigned int                 _channel;
		Terminal::Session_component *_session;
		
	public:
		Channel_node(unsigned int channel, Terminal::Session_component *session)
		: _channel(channel), _session(session) {}
		
		unsigned int                 channel() { return _channel; }
		Terminal::Session_component *session() { return _session; }
		
		bool higher(Channel_node *c) {
			return c->_channel > _channel;
		}
		
		Channel_node *find_by_channel(unsigned int channel)
		{
			using namespace Genode;
			if (channel == _channel)
				return this;

			bool side = (channel > _channel);
			Channel_node *c = Avl_node<Channel_node>::child(side);
			return c ? c->find_by_channel(channel) : 0;
		}
		
		void transmit(unsigned char *addr, Genode::size_t size);
		
};


#endif /* _CHANNEL_NODE_H_ */

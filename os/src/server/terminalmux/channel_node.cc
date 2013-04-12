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

#include "channel_node.h"
#include "terminal_session_component.h"

void Channel_node::transmit(unsigned char *addr, Genode::size_t size)
{
	_session->char_avail(addr, size);
}
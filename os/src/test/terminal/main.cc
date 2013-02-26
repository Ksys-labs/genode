/*
 * \brief  Test for UART driver
 * \author Christian Helmuth
 * \date   2011-05-30
 */

/*
 * Copyright (C) 2011-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/snprintf.h>
#include <timer_session/connection.h>
#include <terminal_session/connection.h>
#include <os/config.h>


using namespace Genode;

int main()
{
	printf("--- Terminal test started ---\n");
	
	long channel_num = 0;
	
	try {
		Genode::Xml_node channel_node = Genode::config()->xml_node().sub_node("channel");
		
		channel_node.attribute("num").value(&channel_num);
		
		PINF("Channel%ld", channel_num);
	}
	catch (...) {
		PDBG("No channel config");
	}
	
	static Timer::Connection timer;
	static Terminal::Connection  uart;

	for (unsigned i = 0; ; ++i) {

		static char buf[100];
		int n = snprintf(buf, sizeof(buf), "Channel%ld test message %d\n\r", channel_num, i);
		uart.write(buf, n);

		timer.msleep(2000);
	}

	return 0;
}

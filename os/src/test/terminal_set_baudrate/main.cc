/*
 * \brief  Test for UART driver
 * \author Christian Helmuth
 * \date   2011-05-30
 */

/*
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/snprintf.h>
#include <timer_session/connection.h>
#include <uart_session/connection.h>


using namespace Genode;

int main()
{
	printf("--- UART test started ---\n");

	static Timer::Connection    timer;
	static Uart::Connection     terminal;

	unsigned baudrate = 115200;

	for (unsigned i = 0; ; ++i, baudrate = (i % 2) ? 115200: 57600 ) {

		static char buf[100];

		terminal.baud_rate(baudrate);

		int n = snprintf(buf, sizeof(buf), "UART test message %d (baudrate=%d)\r\n", i, baudrate);
		terminal.write(buf, n);

		timer.msleep(2000);
	}

	return 0;
}


/*
 * \brief  Test smartcard driver
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2009-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/printf.h>
#include <base/signal.h>
#include <smartcard_session/connection.h>
#include <timer_session/connection.h>

using namespace Genode;

static void printhex(unsigned char *data, unsigned length)
{
	char atr_str[1024];
	if (length > 256) length = 256;
	for (unsigned i = 0; i < length; ++i)
	{
		atr_str[i*3] = '\0';
		snprintf(atr_str + i * 3, 4, "%02x ", data[i]);
	}
	printf("%s\n", atr_str);
}

int main(int, char **)
{
	static Smartcard::Connection smartcard;
	static Timer::Connection     timer;

	PDBG("Wait card...");
	while (!smartcard.is_card_present())
		timer.msleep(1000);

	PDBG("Card connected");
	smartcard.activate_card();

	timer.msleep(500);

	Smartcard::ReaderStatus status = smartcard.reader_status();

	printf("ATR(%d): ", status.atr().length);
	printhex(status.atr().data, status.atr().length);

	unsigned char apdu_cmd[] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned char apdu_resp[256];
	unsigned resp_length;

	printf("APDU command(%d): ", sizeof(apdu_cmd));
	printhex(apdu_cmd, sizeof(apdu_cmd));

	smartcard.transmit(apdu_cmd, sizeof(apdu_cmd), apdu_resp, &resp_length);

	printf("APDU response(%d): ", resp_length);
	printhex(apdu_resp, resp_length);


	return 0;
}

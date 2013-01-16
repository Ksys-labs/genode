/*
 * \brief  Melfas MMS Touchscreen driver
 * \author Alexander Tarasikov
 * \date   2012-12-25
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */

#include <i2c_session/connection.h>
#include <timer_session/connection.h>
#include <base/printf.h>

using namespace Genode;

#define MMS_TS_ADDR 0x48

#define MAX_FINGERS		10
#define MAX_WIDTH		30
#define MAX_PRESSURE		255

/* Registers */
#define MMS_MODE_CONTROL	0x01
#define MMS_XYRES_HI		0x02
#define MMS_XRES_LO		0x03
#define MMS_YRES_LO		0x04

#define MMS_INPUT_EVENT_PKT_SZ	0x0F
#define MMS_INPUT_EVENT0	0x10
#define 	FINGER_EVENT_SZ	6

#define MMS_TSP_REVISION	0xF0
#define MMS_HW_REVISION		0xF1
#define MMS_COMPAT_GROUP	0xF2
#define MMS_FW_VERSION		0xF3

enum {
	ISP_MODE_FLASH_ERASE	= 0x59F3,
	ISP_MODE_FLASH_WRITE	= 0x62CD,
	ISP_MODE_FLASH_READ	= 0x6AC9,
};

/* each address addresses 4-byte words */
#define ISP_MAX_FW_SIZE		(0x1F00 * 4)
#define ISP_IC_INFO_ADDR	0x1F00

static void mms_ts_interrupt(void) {
}

int main()
{
	printf("--- MMS Touchscreen ---\n");
	static I2C::Connection i2c;
	static Timer::Connection timer;

	return 0;

	int tries = 0;
	Genode::uint8_t fw_version = -1;

	do {
		if (!i2c.read(MMS_TS_ADDR, MMS_FW_VERSION, &fw_version, 1)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_FW_VERSION);
			break;
		}
	} while ((fw_version & 0x80) && tries < 3);

	PINF("MMS TS: FW Version %x", fw_version);

	Genode::uint8_t buf[FINGER_EVENT_SZ * MAX_FINGERS];

	do {
		buf[0] = 0;
		if (!i2c.write(MMS_TS_ADDR, 0, buf, 1)) {
			PERR("Failed to wake up MMS_TS");
		}

		if (!i2c.read(MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ, buf, 1)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ);
		}
		int count = buf[0];

		if (!i2c.read(MMS_TS_ADDR, MMS_INPUT_EVENT0, buf, count)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ);
		}

		for (int i = 0; i < count; i+= FINGER_EVENT_SZ) {
			Genode::uint8_t *tmp = buf + i;
			int x = tmp[2] | ((tmp[1] & 0xf) << 8);
			int y = tmp[3] | (((tmp[1] >> 4) & 0xf) << 8);
		
			PINF("Touch <%d; %d>", x, y);
		}
		
		timer.msleep(200);
	} while (1);

	PINF("MMS_TS: done");

	return 0;
}


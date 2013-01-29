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

#include <base/printf.h>
#include <base/env.h>
#include <base/rpc_server.h>
#include <root/component.h>
#include <cap_session/connection.h>
#include <input/component.h>
#include <input/keycodes.h>
#include <input/event.h>
#include <os/ring_buffer.h>

#include <i2c_session/connection.h>
#include <gpio_session/connection.h>
#include <timer_session/connection.h>

#include <platform/omap4/gpiomux_session/connection.h>
#include <platform/omap4/gpiomux_session/mux_bits.h>

using namespace Genode;

/*****************************************
 ** Implementation of the input service **
 *****************************************/

class Event_queue : public Ring_buffer<Input::Event, 10> { };

static Event_queue ev_queue;

namespace Input {
	void event_handling(bool enable) { }
	bool event_pending() { return !ev_queue.empty(); }
	Event get_event() { return ev_queue.get(); }
}
	
static I2C::Connection *i2c;
static Timer::Connection *timer;
static Gpio::Connection *gpio;
static GpioMux::Connection *mux;

/* tuna specific stuff */
#define GPIO_TOUCH_EN		19
#define GPIO_TOUCH_IRQ		46

/* touch is on i2c3 */
#define GPIO_TOUCH_SCL	130
#define GPIO_TOUCH_SDA	131

static void gpio_out(int g, int val) {
	gpio->direction_output(g, val);
	gpio->dataout(g, val);
}

static void tuna_melfas_mux_fw_flash(bool to_gpios) {
	if (to_gpios) {
		mux->init_gpio(GPIO_TOUCH_IRQ,
			Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
		gpio_out(GPIO_TOUCH_IRQ, 0);

		mux->init_gpio(GPIO_TOUCH_SCL,
			Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
		gpio_out(GPIO_TOUCH_SCL, 0);
		
		mux->init_gpio(GPIO_TOUCH_SDA,
			Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
		gpio_out(GPIO_TOUCH_SDA, 0);
	}
	else {
		mux->init_gpio(GPIO_TOUCH_IRQ,
			Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE3);
		gpio_out(GPIO_TOUCH_IRQ, 1);
		gpio->direction_input(GPIO_TOUCH_IRQ);

		mux->init_gpio(GPIO_TOUCH_SCL,
			Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE0);
		gpio_out(GPIO_TOUCH_SCL, 1);
		gpio->direction_input(GPIO_TOUCH_SCL);
		
		mux->init_gpio(GPIO_TOUCH_SDA,
			Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE0);
		gpio_out(GPIO_TOUCH_SDA, 1);
		gpio->direction_input(GPIO_TOUCH_SDA);
	}
}

static void tuna_melfas_init(void) {
	mux->init_gpio(GPIO_TOUCH_IRQ,
		Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE3);
	gpio->direction_input(GPIO_TOUCH_IRQ);

	mux->init_gpio(GPIO_TOUCH_EN,
		Omap4::GpioMux::PIN_OUTPUT | Omap4::GpioMux::MUX_MODE3);
	gpio_out(GPIO_TOUCH_EN, 1);

	tuna_melfas_mux_fw_flash(false);
	timer->msleep(200);
}

static void hw_reboot(bool bootloader) {
	gpio_out(GPIO_TOUCH_EN, 0);
	gpio_out(GPIO_TOUCH_SDA, bootloader ? 0 : 1);
	gpio_out(GPIO_TOUCH_SCL, bootloader ? 0 : 1);
	gpio_out(GPIO_TOUCH_IRQ, 0);
	timer->msleep(30);
	gpio_out(GPIO_TOUCH_EN, 1);
	timer->msleep(30);

	if (bootloader) {
		gpio_out(GPIO_TOUCH_SCL, 0);
		gpio_out(GPIO_TOUCH_SDA, 1);
	} else {
		gpio_out(GPIO_TOUCH_IRQ, 1);
		gpio->direction_input(GPIO_TOUCH_IRQ);
		gpio->direction_input(GPIO_TOUCH_SCL);
		gpio->direction_input(GPIO_TOUCH_SDA);
	}
	timer->msleep(40);
}

static void mms_pwr_on_reset() {
	tuna_melfas_mux_fw_flash(true);
	
	gpio_out(GPIO_TOUCH_EN, 0);
	gpio_out(GPIO_TOUCH_SDA, 1);
	gpio_out(GPIO_TOUCH_SCL, 1);
	gpio_out(GPIO_TOUCH_IRQ, 1);
	timer->msleep(50);
	gpio_out(GPIO_TOUCH_EN, 1);
	timer->msleep(50);
	
	tuna_melfas_mux_fw_flash(false);
	
	timer->msleep(250);
}

/* generic stuff */

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
	PINF("--- MMS Touchscreen ---\n");
	static I2C::Connection _i2c;
	i2c = &_i2c;
	PINF("I2C");
	static Timer::Connection _timer;
	timer = &_timer;
	PINF("TIMER");
	static Gpio::Connection _gpio;
	gpio = &_gpio;
	PINF("GPIO");
	static GpioMux::Connection _mux;
	mux = &_mux;
	PINF("GPIO MUX");


	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "mms_ts_ep");
	static Input::Root input_root(&ep, env()->heap());
	env()->parent()->announce(ep.manage(&input_root));

	tuna_melfas_init();
	tuna_melfas_mux_fw_flash(true);
	hw_reboot(0);
	mms_pwr_on_reset();
	PINF("reset done");

	int tries = 0;
	Genode::uint8_t fw_version = -1;
	Genode::uint8_t buf[FINGER_EVENT_SZ * MAX_FINGERS];
		
	buf[0] = 0;
	if (!i2c->write(MMS_TS_ADDR, 0, buf, 1)) {
		PERR("Failed to wake up MMS_TS");
	}
	timer->msleep(10);

	do {
		if (!i2c->read(MMS_TS_ADDR, MMS_FW_VERSION, &fw_version, 1)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_FW_VERSION);
			break;
		}
		PINF("MMS TS: FW Version %x", fw_version);
		timer->msleep(100);
	} while ((fw_version & 0x80) && tries++ < 3);

	#define _REG(name) {name, #name}
	struct {
		Genode::uint8_t reg;
		char *name;
	} regs[] = {
		_REG(MMS_TSP_REVISION),
		_REG(MMS_HW_REVISION),
		_REG(MMS_COMPAT_GROUP),
		_REG(MMS_XRES_LO),
		_REG(MMS_YRES_LO),
	};
	for (int i = 0; i < sizeof(regs)/sizeof(regs[0]);i++){
		Genode::uint8_t tr = -1;
		i2c->read(MMS_TS_ADDR, regs[i].reg, &tr, 1);
		PINF("MMS %s : %x", regs[i].name, tr);
	}

	bool finger_down = false;
	
	do {
		if (!i2c->read(MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ, buf, 1)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ);
		}
		int count = buf[0];

		if (!count || count > MAX_FINGERS * FINGER_EVENT_SZ) {
			continue;
		}
		
		Genode::memset(buf, 0, sizeof(buf));
		if (!i2c->read(MMS_TS_ADDR, MMS_INPUT_EVENT0, buf, count)) {
			PERR("Failed to read i2c %x %x", MMS_TS_ADDR, MMS_INPUT_EVENT_PKT_SZ);
		}
			
		for (int i = 0; i < count; i+= FINGER_EVENT_SZ) {
			Genode::uint8_t *tmp = buf + i;
			int id = (tmp[0] & 0xf) - 1;
			int x = tmp[2] | ((tmp[1] & 0xf) << 8);
			int y = tmp[3] | (((tmp[1] >> 4) & 0xf) << 8);
			int w = tmp[4];
			int p = tmp[5];
			
			bool up = ((tmp[0] & 0x80) == 0);
			
			Input::Event ev(Input::Event::MOTION,
				0, x, y, 0, 0);
			ev_queue.add(ev);
			
			/* emulate mouse with the first finger */
			if (id == 0) {
				if (up) {
					if (finger_down) {
						finger_down = false;
						Input::Event em(Input::Event::RELEASE,
							Input::BTN_LEFT, x, y, 0, 0);
						ev_queue.add(em);
					}
				}
				else {
					if (!finger_down) {
						finger_down = true;
						Input::Event em(Input::Event::PRESS,
							Input::BTN_LEFT, x, y, 0, 0);
						ev_queue.add(em);
					}
				}
			}
			
			PINF("Touch %d <%d; %d> (%d %d)", id, x, y, p, w);
		}
		
		timer->msleep(10);
	} while (1);

	PINF("MMS_TS: done");

	return 0;
}


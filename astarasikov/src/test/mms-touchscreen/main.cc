/*
 * \brief  Tuna board code for Melfas MMS Touchscreen driver
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

#include "mms_ts.h"

using namespace Genode;

class TunaTouchscreen : public Input::MMSTouchScreen {
protected:
	Gpio::Connection *_gpio;
	GpioMux::Connection *_mux;

	enum {
		/* tuna specific stuff */
		GPIO_TOUCH_EN = 19,
		GPIO_TOUCH_IRQ = 46,

		/* touch is on i2c3 */
		GPIO_TOUCH_SCL = 130,
		GPIO_TOUCH_SDA = 131,

		MMS_TS_ADDR = 0x48,
	};
	
	void gpio_out(int g, int val) {
		_gpio->direction_output(g, val);
		_gpio->dataout(g, val);
	}

public:
	TunaTouchscreen(
		I2C::Connection *i2c,
		Timer::Connection *timer,
		Input::Event_queue *ev_queue,
		Gpio::Connection *gpio,
		GpioMux::Connection *mux) :
		MMSTouchScreen(i2c, timer, ev_queue, MMS_TS_ADDR), _gpio(gpio), _mux(mux)
		{}
	~TunaTouchscreen() {
	}
	
	virtual void platform_mux_fw_flash(bool to_gpios) {
		if (to_gpios) {
			_mux->init_gpio(GPIO_TOUCH_IRQ,
				Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
			gpio_out(GPIO_TOUCH_IRQ, 0);

			_mux->init_gpio(GPIO_TOUCH_SCL,
				Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
			gpio_out(GPIO_TOUCH_SCL, 0);
			
			_mux->init_gpio(GPIO_TOUCH_SDA,
				Omap4::GpioMux::PIN_INPUT | Omap4::GpioMux::MUX_MODE3);
			gpio_out(GPIO_TOUCH_SDA, 0);
		}
		else {
			_mux->init_gpio(GPIO_TOUCH_IRQ,
				Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE3);
			gpio_out(GPIO_TOUCH_IRQ, 1);
			_gpio->direction_input(GPIO_TOUCH_IRQ);

			_mux->init_gpio(GPIO_TOUCH_SCL,
				Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE0);
			gpio_out(GPIO_TOUCH_SCL, 1);
			_gpio->direction_input(GPIO_TOUCH_SCL);
			
			_mux->init_gpio(GPIO_TOUCH_SDA,
				Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE0);
			gpio_out(GPIO_TOUCH_SDA, 1);
			_gpio->direction_input(GPIO_TOUCH_SDA);
		}
	}

	virtual void platform_init(void) {
		_mux->init_gpio(GPIO_TOUCH_IRQ,
			Omap4::GpioMux::PIN_INPUT_PULLUP | Omap4::GpioMux::MUX_MODE3);
		_gpio->direction_input(GPIO_TOUCH_IRQ);

		_mux->init_gpio(GPIO_TOUCH_EN,
			Omap4::GpioMux::PIN_OUTPUT | Omap4::GpioMux::MUX_MODE3);
		gpio_out(GPIO_TOUCH_EN, 1);

		platform_mux_fw_flash(false);
		_timer->msleep(200);
	}

	virtual void platform_hw_reboot(bool bootloader) {
		gpio_out(GPIO_TOUCH_EN, 0);
		gpio_out(GPIO_TOUCH_SDA, bootloader ? 0 : 1);
		gpio_out(GPIO_TOUCH_SCL, bootloader ? 0 : 1);
		gpio_out(GPIO_TOUCH_IRQ, 0);
		_timer->msleep(30);
		gpio_out(GPIO_TOUCH_EN, 1);
		_timer->msleep(30);

		if (bootloader) {
			gpio_out(GPIO_TOUCH_SCL, 0);
			gpio_out(GPIO_TOUCH_SDA, 1);
		} else {
			gpio_out(GPIO_TOUCH_IRQ, 1);
			_gpio->direction_input(GPIO_TOUCH_IRQ);
			_gpio->direction_input(GPIO_TOUCH_SCL);
			_gpio->direction_input(GPIO_TOUCH_SDA);
		}
		_timer->msleep(40);
	}

	virtual void platform_pwr_on_reset(void) {
		platform_mux_fw_flash(true);
		
		gpio_out(GPIO_TOUCH_EN, 0);
		gpio_out(GPIO_TOUCH_SDA, 1);
		gpio_out(GPIO_TOUCH_SCL, 1);
		gpio_out(GPIO_TOUCH_IRQ, 1);
		_timer->msleep(50);
		gpio_out(GPIO_TOUCH_EN, 1);
		_timer->msleep(50);
		
		platform_mux_fw_flash(false);
		
		_timer->msleep(250);
	}
};

/*****************************************
 ** Implementation of the input service **
 *****************************************/
static Input::Event_queue ev_queue;

namespace Input {
	void event_handling(bool enable) { }
	bool event_pending() { return !ev_queue.empty(); }
	Event get_event() { return ev_queue.get(); }
}
	
int main()
{
	PINF("--- MMS Touchscreen ---\n");
	static I2C::Connection _i2c;
	PINF("I2C");
	static Timer::Connection _timer;
	PINF("TIMER");
	static Gpio::Connection _gpio;
	PINF("GPIO");
	static GpioMux::Connection _mux;

	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "mms_ts_ep");
	static Input::Root input_root(&ep, env()->heap());
	env()->parent()->announce(ep.manage(&input_root));

	TunaTouchscreen touchscreen(&_i2c, &_timer, &ev_queue, &_gpio, &_mux);
	touchscreen.init();

	do {
		touchscreen.event();
		_timer.msleep(10);
	} while (1);

	PINF("MMS_TS: done");

	return 0;
}


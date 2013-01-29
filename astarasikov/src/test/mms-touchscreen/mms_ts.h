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

#ifndef _MMS_TS__H_
#define _MMS_TS__H_

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
#include <timer_session/connection.h>

using namespace Genode;

/*****************************************
 ** Implementation of the input service **
 *****************************************/

namespace Input {

class Event_queue : public Ring_buffer<Input::Event, 10> { };

class MMSTouchScreen {
protected:
	I2C::Connection *_i2c;
	Timer::Connection *_timer;
	Input::Event_queue *_ev_queue;
	Genode::uint8_t _address;

	enum {
		MAX_FINGERS		= 10,
		MAX_WIDTH		= 30,
		MAX_PRESSURE	= 255,

		/* Registers */
		MMS_MODE_CONTROL	= 0x01,
		MMS_XYRES_HI		= 0x02,
		MMS_XRES_LO			= 0x03,
		MMS_YRES_LO			= 0x04,
		
		MMS_INPUT_EVENT_PKT_SZ	= 0x0F,
		MMS_INPUT_EVENT0	= 0x10,
		FINGER_EVENT_SZ		= 6,
		
		MMS_TSP_REVISION	= 0xF0,
		MMS_HW_REVISION		= 0xF1,
		MMS_COMPAT_GROUP	= 0xF2,
		MMS_FW_VERSION		= 0xF3,
	};

	Genode::uint8_t _buf[FINGER_EVENT_SZ * MAX_FINGERS];
	bool finger_down;

public:
	MMSTouchScreen(
		I2C::Connection *i2c,
		Timer::Connection *timer,
		Input::Event_queue *ev_queue,
		Genode::uint8_t address
	)
	: _i2c(i2c), _timer(timer), _ev_queue(ev_queue), _address(address) {}
	virtual ~MMSTouchScreen() {}

	virtual void platform_init(void) {}
	virtual void platform_mux_fw_flash(bool to_gpios) {}
	virtual void platform_hw_reboot(bool bootloader) {}
	virtual void platform_pwr_on_reset(void) {}

	void init(void) {
		platform_init();
		platform_mux_fw_flash(true);
		platform_hw_reboot(false);
		platform_pwr_on_reset();
		PINF("reset done");

		_buf[0] = 0;
		if (!_i2c->write(_address, 0, _buf, 1)) {
			PERR("Failed to wake up MMS_TS");
		}
		_timer->msleep(10);
	
		Genode::uint8_t fw_version = -1;
		int tries = 0;
		do {
			if (!_i2c->read(_address, MMS_FW_VERSION, &fw_version, 1)) {
				PERR("Failed to read i2c %x %x", _address, MMS_FW_VERSION);
				break;
			}
			PINF("MMS TS: FW Version %x", fw_version);
			_timer->msleep(100);
		} while ((fw_version & 0x80) && tries++ < 3);
	}

	void event(void) {
		if (!_i2c->read(_address, MMS_INPUT_EVENT_PKT_SZ, _buf, 1)) {
			PERR("Failed to read i2c %x %x",
				_address, MMS_INPUT_EVENT_PKT_SZ);
			return;
		}
		Genode::uint8_t count = _buf[0];
		if (!count || count > MAX_FINGERS * FINGER_EVENT_SZ) {
			return;
		}
		
		Genode::memset(_buf, 0, sizeof(_buf));
		if (!_i2c->read(_address, MMS_INPUT_EVENT0, _buf, count)) {
			PERR("Failed to read i2c %x %x", _address, MMS_INPUT_EVENT_PKT_SZ);
			return;
		}
			
		for (int i = 0; i < count; i+= FINGER_EVENT_SZ) {
			Genode::uint8_t *tmp = _buf + i;
			int id = (tmp[0] & 0xf) - 1;
			int x = tmp[2] | ((tmp[1] & 0xf) << 8);
			int y = tmp[3] | (((tmp[1] >> 4) & 0xf) << 8);
			int w = tmp[4];
			int p = tmp[5];
			
			bool up = ((tmp[0] & 0x80) == 0);
			
			Input::Event ev(Input::Event::MOTION,
				0, x, y, 0, 0);
			_ev_queue->add(ev);
			
			/* emulate mouse with the first finger */
			if (id == 0) {
				if (up) {
					if (finger_down) {
						finger_down = false;
						Input::Event em(Input::Event::RELEASE,
							Input::BTN_LEFT, x, y, 0, 0);
						_ev_queue->add(em);
					}
				}
				else {
					if (!finger_down) {
						finger_down = true;
						Input::Event em(Input::Event::PRESS,
							Input::BTN_LEFT, x, y, 0, 0);
						_ev_queue->add(em);
					}
				}
			}
			PINF("Touch %d <%d; %d> (%d %d)", id, x, y, p, w);
		}
	}
};

} //namespace Input

#endif //_MMS_TS__H_

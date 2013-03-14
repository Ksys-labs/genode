/*
 * \brief  Interface to TSC2046
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-08-01
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _TSC2046_H_
#define _TSC2046_H_

#include <base/env.h>
#include <base/printf.h>
#include <util/misc_math.h>
#include <input/keycodes.h>
#include <timer_session/connection.h>

#include <gpio_session/connection.h>
#include <base/signal.h>
#include <spi_session/connection.h>


#include "event_queue.h"
#include "input_driver.h"

using namespace Genode;

static int verbose = 0;

class Driver : public Input_driver
{
public:
	enum {
		CHIPSEE_TS_GPIO = 39,
	};

private:
	enum {
		LEFT                = 0,
		NUM_BUTTONS,
	};

	enum {
		Y_MAX               = 0x0fff,
		TS_YMIN             = 0x00d9,
		TS_YMAX             = 0x0ec6,

		X_PLATE_OHMS        = 180,
		PRESSURE_MAX        = 255,
		DEBOUNCE_MAX        = 10,
		DEBOUNCE_TOL        = 4,
		DEBOUNCE_REP        = 1,
	};

	enum {
		// MCSPI_MODULCTRL
		SINGLE              = (1 << 0),

		// MCSPI_CHxCTRL
		EN                  = (1 << 0),

		// MCSPI_CHxSTAT
		EOT                 = (1 << 2),

		// MCSPI_CHxCONF
		CLKD_32             = (0x05 << 2),
		CLKD_1024           = (0x09 << 2),
		WL_8BIT             = (0x07 << 7),
		WL_16BIT            = (0x0f << 7),
		EPOL_LOW            = (1 << 6),
		DPE0                = (1 << 16),
		FORCE               = (1 << 20),
	};

	enum {
		// Bit positions for ADS7846E Control byte
		ADS_START           = (1 << 7),     // Start bit, always 1
		ADS_8_BIT           = (1 << 3),     // 0 = 12-bit conversion, 1 = 8-bits
		ADS_12_BIT          = (0 << 3),
		ADS_SER             = (1 << 2),     // 0 = Differential, 1 = Single ended
		ADS_DFR             = (0 << 2),

		ADS_A2A1A0_d_y      = (0x01 << 4),  // Y position measurement
		ADS_A2A1A0_d_z1     = (0x03 << 4),  // pressure measurement 1
		ADS_A2A1A0_d_z2     = (0x04 << 4),  // pressure measurement 2
		ADS_A2A1A0_d_x      = (0x05 << 4),  // X position measurement

		ADS_PD10_PDOWN      = (0 << 0),     // low power mode + penirq
		ADS_PD10_ADC_ON     = (1 << 0),     // ADC on
		ADS_PD10_REF_ON     = (2 << 0),     // vREF on + penirq
		ADS_PD10_ALL_ON     = (3 << 0),     // ADC + vREF on
	};


private:

	Event_queue                         &_ev_queue;
	Timer::Connection                   _timer;

	Gpio::Connection                    _gpio;
	Signal_receiver                     _sig_rec;
	Signal_context                      _sig_ctx;

	Spi::Connection                     _spi;

	bool                                _pen_down;

	int                                 _empty_cnt;

	int                                 _abs_x;
	int                                 _abs_y;

	uint16_t _tsc2046_wr(Genode::uint16_t d)
	{
		uint8_t    s0, s1;

		_spi.write(0, 0, d);
		s0 = _spi.read(0, 0);
		s1 = _spi.read(0, 0);

		if (verbose)
			PDBG("%02x - %02x%02x %04x", (unsigned char)d, (unsigned char)s0, (unsigned char)s1, ((((uint16_t)s0 << 8) + s1) >> 3) & 0xfff);

		return ((((uint16_t)s0 << 8) + s1) >> 3) & 0xfff;
	}

	void _init()
	{
		if (verbose)
			PDBG("Init touchscreen\n");

		// Configure GPIO
		_gpio.direction_input(CHIPSEE_TS_GPIO);

		_gpio.debouncing_time(CHIPSEE_TS_GPIO, 31*100);
		_gpio.debounce_enable(CHIPSEE_TS_GPIO, 1);

		_gpio.falling_detect(CHIPSEE_TS_GPIO, 1);
		_gpio.irq_enable(CHIPSEE_TS_GPIO, 1);

		_gpio.irq_sigh(_sig_rec.manage(&_sig_ctx), CHIPSEE_TS_GPIO);

		// Configure SPI
		_spi.set_module_ctrl(0, SINGLE);
		_spi.set_channel_config(0, 0, FORCE | DPE0 | (0x05 << 2) | WL_8BIT | EPOL_LOW);

		_tsc2046_wr(ADS_START);
	}

	int _read_filtered(unsigned char cmd)
	{
		unsigned short cnt = 0;
		unsigned short rep = 0;
		unsigned short last = 0;
		unsigned short val = 0;

		while (1)
		{
			val = _tsc2046_wr(cmd);

			if (!cnt || ( abs(last - val) > DEBOUNCE_TOL ))
			{
				rep = 0;
				if (cnt < DEBOUNCE_MAX) {
					last = val;
					cnt++;
					// repeat
				} else {
					// ignore
					return -1;
				}
			} else {
				if (++rep > DEBOUNCE_REP)
				{
					// ok
					break;
				} else {
					// repeat
				}
			}
		}

		return val;
	}

	void _tsc_get_pen_position(int *x, int *y, int *Rt)
	{
		int             ts_y, ts_x, ts_z1, ts_z2;
		unsigned int    ts_rt = 0;

		*x = -1; *y = -1; *Rt = 0;

		// read y
		ts_y = _read_filtered( ADS_START | ADS_PD10_ADC_ON | ADS_PD10_REF_ON | ADS_A2A1A0_d_y);
		if (ts_y < 0)
			return;

		// read x
		ts_x = _read_filtered( ADS_START | ADS_PD10_ADC_ON | ADS_PD10_REF_ON | ADS_A2A1A0_d_x);
		if (ts_x < 0)
			return;

		// read z1
		ts_z1 = _read_filtered( ADS_START | ADS_PD10_ADC_ON | ADS_PD10_REF_ON | ADS_A2A1A0_d_z1);
		if (ts_z1 < 0)
			return;

		// read z2
		ts_z2 = _read_filtered( ADS_START | ADS_PD10_ADC_ON | ADS_PD10_REF_ON | ADS_A2A1A0_d_z2);
		if (ts_z2 < 0)
			return;

		if ( (ts_z1 > 0) && (ts_x > 0)) {
			// compute touch pressure resistance
			ts_rt = ts_z2;
			ts_rt -= ts_z1;
			ts_rt *= ts_x;
			ts_rt *= X_PLATE_OHMS;
			ts_rt /= ts_z1;
			ts_rt = (ts_rt + 2047) >> 12;
		}
		else {
			ts_rt = 0;
		}

		if (verbose)
			PDBG("Info: (ts_x,ts_y,ts_z1,ts_z2,ts_rt)=(%04x, %04x, %04x, %04x, %u)\n", ts_x, ts_y, ts_z1, ts_z2, ts_rt);

		if ( (ts_rt != 0) && (ts_rt < PRESSURE_MAX) )
		{
			*x = ts_x;
			*y = ts_y;
			*Rt = ts_rt;
		}
	}

	void _create_motion_event()
	{
		int x = 0;
		int y = 0;
		int Rt = 0;

		_tsc_get_pen_position(&x, &y, &Rt);

  		if (x==-1 || y==-1)
		{
			// ignore
			_empty_cnt++;
 			return;
		}

		_empty_cnt = 0;

 		if (verbose)
			PDBG ("create_motion_event ts_x=%d, ts_y=%d Rt=%d\n", x, y, Rt);

		// from linux driver
		_abs_x = x;
		_abs_y = Y_MAX - ((Y_MAX * (y - TS_YMIN)) / (TS_YMAX - TS_YMIN));

		_ev_queue.add(Input::Event(Input::Event::MOTION, 0, _abs_x, _abs_y, 0, 0));

		// in first generate key press
		if ( _pen_down)
		{
			if (verbose)
				printf("post %s, BTN_LEFT x=%d y=%d\n", "PRESS", _abs_x, _abs_y);

			_ev_queue.add(Input::Event(Input::Event::PRESS, Input::BTN_LEFT, _abs_x, _abs_y, 0, 0));

			_pen_down = false;
		}
	}

public:

	Driver(Event_queue &ev_queue, int irq_number)
	:
		_ev_queue(ev_queue)
	{
		_init();
	}

	Signal_receiver *get_signal_receiver() { return &_sig_rec; }

	void handle_event()
	{
		_gpio.irq_enable(CHIPSEE_TS_GPIO, 0);

		if ( !_gpio.datain(CHIPSEE_TS_GPIO) )
		{
			if (verbose)
				Genode::printf("handle_event\n");

			_pen_down = true;

			_empty_cnt = 0;

			while(1)
			{
				_create_motion_event();

				// check pen up
				if (_empty_cnt > 5)
				{
					//PDBG("check pin");
					_tsc2046_wr( ADS_START );
					_timer.msleep(5);
					if( _gpio.datain(CHIPSEE_TS_GPIO) )
					{
						//PDBG("break");
						break;
					}
				}
			}

			if (!_pen_down)
			{
				if (verbose)
					Genode::printf("post %s, BTN_LEFT x=%d y=%d\n", "RELEASE", _abs_x, _abs_y);

				_ev_queue.add(Input::Event(Input::Event::RELEASE, Input::BTN_LEFT, _abs_x, _abs_y, 0, 0));
			}
		}

		_gpio.irq_enable(CHIPSEE_TS_GPIO, 1);
	}
};

#endif /* _TSC2046_H_ */

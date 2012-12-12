/*
 * \brief  TDA8029 driver
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

/* Genode includes */
#include <base/printf.h>
#include <base/signal.h>
#include <timer_session/connection.h>
#include <uart_session/connection.h>
#include <smartcard_session/smartcard_session.h>

/* local includes */
#include "smartcard_driver.h"
#include "alpar.h"

using namespace Genode;

class Driver: public Smartcard::Driver
{
	enum { verbose = 1 };
	
	Smartcard::ReaderStatus _status;
	unsigned                _timeout;

	Timer::Connection       _timer;
	Uart::Connection        _uart;
	
public:

	Driver();

	void reader_status(Smartcard::ReaderStatus *status);

	bool is_card_present();
	bool activate_card();
	bool deactivate_card();

	size_t transmit(const void *cmd_buf, Genode::size_t cmd_length, void *resp_buf);

private:
	bool open_reader();
	bool card_command(const void *cmd_buf, unsigned cmd_length, void *resp_buf, unsigned *resp_len);
	
	void wait_card();
	
	/* alpar cmds */
	bool send_num_mask(char *firmware_str);
	bool set_serial_baud_rate(unsigned char baud);
	bool check_presence_card();
	bool power_up_iso();
	bool power_off();

	/* internal functions */
	void timeout(unsigned msec) { _timeout = msec; }

	void transmit(const AlparProtocol& cmd);
	AlparProtocol recv();
	void read_uart(uint8_t *buf, unsigned length);
	void flush_uart();
};

/* public */
Driver::Driver()
: _timeout(1000)
{
	if ( !open_reader() ) {
		PERR("Open reader error");
	}
}

void Driver::reader_status(Smartcard::ReaderStatus *status)
{
	_status._card_present = is_card_present();
	memcpy(status, &_status, sizeof(_status));
}

bool Driver::is_card_present()
{
	try {
		return check_presence_card();
	}
	catch(const ReaderException& e)
	{
		PERR("Error: %s", e.what());
	}
	return false;
}

bool Driver::activate_card()
{
	try {
		return power_up_iso();
	}
	catch(const ReaderException& e)
	{
		PERR("Error: %s", e.what());
	}
	return false;
}

bool Driver::deactivate_card()
{
	try {
		power_off();
		return true;
	}
	catch(const ReaderException& e)
	{
		PERR("Error: %s", e.what());
	}
	return false;
}

size_t Driver::transmit(const void *cmd_buf, Genode::size_t cmd_length, void *resp_buf)
{
	unsigned received;
	card_command(cmd_buf, cmd_length, resp_buf, &received);

	return received;
}

/* private */
bool Driver::open_reader()
{
	char firmware[256];
	unsigned baud = 38400;

	_uart.baud_rate(baud);
	_timer.msleep(200);

	if (verbose)
		PDBG("Open serial port with baud %d.", baud);

	timeout(500);

	if ( !send_num_mask(firmware) )
	{
		baud = 115200;
		_uart.baud_rate(baud);
		_timer.msleep(200);
		if (verbose)
			PDBG("Reopen serial port with baud %d.", baud);
		_timer.msleep(1000);
		
		if (!send_num_mask(firmware)) return false;
	}

	if (baud != 115200)
	{
		if ( !set_serial_baud_rate(AlparProtocol::BAUDRATE_115200) )
			return false;

		baud = 115200;
		_uart.baud_rate(baud);
		_timer.msleep(200);

		if (verbose)
			PDBG("Reopen serial port with baud %d.", baud);
	}

	if (verbose) {
		snprintf(_status._reader_name, 255, "TDA8029 (%s)", firmware);
		PDBG("%s", _status._reader_name);
	}

	timeout(2500);
	
	_status._reader_present = true;

	return true;
}

void Driver::wait_card()
{
// 	Genode::Signal_receiver sig_rec;
// 	Genode::Signal_context  sig_ctx;
// 
// 	_uart.read_avail_sigh(sig_rec.manage(&sig_ctx));
// 
// 	while(!_status._card_present)
// 	{
// 		sig_rec.wait_for_signal();
// 
// 		AlparProtocol response = recv();
// 		if (response.length() == 1)
// 		{
// 			if (response.command() == 0xa0)
// 			{
// 				_status._card_present = response.data()[0] == 0x01;
// 			}
// 		}
// 	}
}

bool Driver::card_command(const void *cmd, unsigned cmd_len, void *resp, unsigned *resp_len)
{
	try {
		transmit( AlparProtocol(AlparProtocol::CARD_COMMAND, (uint8_t*)cmd, cmd_len) );
		AlparProtocol response = recv();

		memcpy(resp, response.data(), response.length());
		*resp_len = response.length();
		return true;
	}
	catch(const ReaderException& e)
	{
		*resp_len = 0;
		PERR("Error: %s", e.what());
	}
	return false;
}

bool Driver::send_num_mask(char *firmware_str)
{
	try {
		transmit( AlparProtocol(AlparProtocol::SEND_NUM_MASK) );
		AlparProtocol response = recv();

		memcpy(firmware_str, response.data(), response.length());
		firmware_str[response.length()]=0;
	}
	catch(const ReaderException& e)
	{
		return false;
	}

	return true;
}

bool Driver::set_serial_baud_rate(unsigned char baud)
{
	try {
		transmit( AlparProtocol(AlparProtocol::SET_SERIAL_BAUD_RATE, &baud, 1) );
		recv();
	}
	catch(const ReaderException& e)
	{
		return false;
	}
	return true;
}

bool Driver::check_presence_card()
{
	transmit( AlparProtocol(AlparProtocol::CARD_PRESENCE) );
	AlparProtocol response = recv();

	return (response.length() == 1) && (response.data()[0] == 1);
}

bool Driver::power_up_iso()
{
	transmit( AlparProtocol(AlparProtocol::POWER_UP_ISO) );
	AlparProtocol response = recv();

	if(response.data()[0] == 0xc0) {
		if (verbose)
			PDBG("Card absent\n");
		_status._card_present = false;
		_status._error_code = AlparProtocol::ERR_PROTOCOL | response.data()[0];
		_status._atr.length = 0;
		return false;
	}

	memcpy(_status._atr.data, response.data(), response.length());
	_status._atr.length = response.length();

	return true;
}

bool Driver::power_off()
{
	_status._card_present = false;
	_status._error_code = 0;
	_status._atr.length = 0;
	
	transmit( AlparProtocol(AlparProtocol::POWER_OFF) );
	recv();

	return true;
}

void Driver::transmit(const AlparProtocol& cmd)
{
	_status._error_code = 0;
	flush_uart();
	if (verbose) {
		PDBG("Cmd: ");
		for(int i = 0; i != cmd.buffer_length(); ++i)
			Genode::printf("%02x ", cmd.buffer()[i]);
		Genode::printf("\n");
	}
	_uart.write(cmd.buffer(), cmd.buffer_length());
}

AlparProtocol Driver::recv()
{
	AlparProtocol response;
	uint16_t length;

	read_uart(response.buffer(), 4);

	length = ( response.buffer()[1] << 8 ) | ( response.buffer()[2] );

	read_uart(response.buffer() + 4, length + 1);

	// check error
	try {
		response.parse_buffer();
	}
	catch (AlparNac) {
		_status._error_code = AlparProtocol::ERR_PROTOCOL | response.errno();
		PERR("NAC");
	}
	return response;
}

void Driver::read_uart(uint8_t *buf, unsigned length)
{
	unsigned ms_count = 0;
	unsigned readed = 0;
	unsigned pos = 0;

	while(length) {
		if (!_uart.avail()) {
			_timer.msleep(1);
			if (++ms_count >= _timeout)
			{
				if (verbose)
					PDBG("Timeout");
				throw Timeout();
			}
		} else {
			readed = _uart.read(buf + pos, length);
			ms_count = 0;
			if (verbose) {
				PDBG("read: ");
				for(int i = 0; i != readed; ++i)
					Genode::printf("%02x ", (buf + pos)[i]);
				Genode::printf("\n");
			}
			pos += readed;
			length -= readed;
		}
	}
}

void Driver::flush_uart()
{
	while(_uart.avail()) {
		unsigned char dummy;
		_uart.read(&dummy, 1);
	}
}

#endif // _DRIVER_H_

/*
 * \brief  Alpar protocol implementation
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

#ifndef _ALPAR_H_
#define _ALPAR_H_

#include <base/printf.h>


/* local includes */
#include "readerexceptions.h"

using Genode::uint8_t ;
using Genode::uint16_t;
using Genode::uint32_t;
using Genode::memcpy;
using Genode::snprintf;

class AlparProtocol
{
	enum { verbose = 0 };

	enum {
		MAX_PACKET_DATA = 506,
		READ_BUFFER_SIZE = MAX_PACKET_DATA + 5,
	};
	
public:
	enum {
		BAUDRATE_115200         = 0x06,
	};

	enum Commands {
		CARD_COMMAND            = 0x00,
		CARD_PRESENCE           = 0x09,
		SEND_NUM_MASK           = 0x0a,
		SET_SERIAL_BAUD_RATE    = 0x0d,
		POWER_OFF               = 0x4d,
		POWER_UP_ISO            = 0x69,
	};

	enum {
		ALPAR_ACK               = 0x60,
		ALPAR_NAK               = 0xe0,
	};
	
	enum {
		ERR_PROTOCOL            = (1<<24),
	};

	AlparProtocol() {}

	AlparProtocol(uint8_t command, uint8_t *data = 0, uint16_t length = 0)
	: _command(command), _data(data), _length(length), _errno(0)
	{
		if (_length > MAX_PACKET_DATA)
			throw DataTooLong();

		_buffer[0] = ALPAR_ACK;
		_buffer[1] = (_length & 0xFF00) >> 8 ;
		_buffer[2] = _length & 0x00FF;
		_buffer[3] = _command;

		if(_length)
			memcpy(_buffer + 4, _data, _length);

		_buffer[_length + 4] = lrc(_length + 4);
	}

	void parse_buffer()
	{
		if (verbose)
			PDBG("%s", _buffer[0] == ALPAR_ACK ? "Successfull Ack(0x60)" :
					_buffer[0] == ALPAR_NAK ? "Error Nak(0xE0)" :
					"Unknown header signature!!\n"
			);

		if((_buffer[0] != ALPAR_ACK) && (_buffer[0] != ALPAR_NAK) )
			throw UnknownHeaderSignature();

		_command = _buffer[4];
		_length = ( _buffer[1] << 8 ) | ( _buffer[2] );

		if (_length > MAX_PACKET_DATA)
			throw DataTooLong();


		if (_length > 0) {
			_data = _buffer + 4;
		} else {
			_data = 0;
		}

		if ( lrc(_length + 5) )
			throw BadLRC();

		if ( _buffer[0] != ALPAR_ACK )
		{
			PERR("Error: %s", error_str(_buffer[5]));
			_errno = _buffer[5];
			throw AlparNac();
		}
		_errno = 0;
	}

	uint8_t *buffer() { return _buffer; }

 	const uint8_t *buffer() const { return _buffer; }

	uint16_t buffer_length() const { return _length + 5; }

	uint8_t *data() const { return _data; }
	uint16_t length() const { return _length; }
	uint16_t command() const { return _command; }
	uint16_t ack() const { return _buffer[0]; }
	uint8_t  errno() const { return _errno; }

private:
	uint8_t lrc(uint32_t length) {
		uint8_t lrc = _buffer[0];

		for(unsigned i = 1; i < length; i++)
			lrc = lrc ^ _buffer[i];

		return lrc;
	}

	const char* error_str(uint8_t status)
	{
		_errno = status;
		
		switch(status)
		{
			case 0x08: return "Length of the data buffer too short";
			case 0x0a: return "3 consecutive errors from the card in T=1 protocol";

			case 0x20: return "Wrong APDU";
			case 0x21: return "Too short APDU";
			case 0x22: return "Card mute now (during T=1 exchange)";
			case 0x24: return "Bad NAD";
			case 0x25: return "Bad LRC";
			case 0x26: return "Resynchronized";
			case 0x27: return "Chain aborted";
			case 0x28: return "Bad PCB";
			case 0x29: return "Overflow from card";

			case 0x30: return "Non negotiable mode (TA2 present)";
			case 0x31: return "Protocol is neither T=0 nor T=1 (negotiate command)";
			case 0x32: return "T=1 is not accepted (negotiate command)";
			case 0x33: return "PPS answer is different from PPS request";
			case 0x34: return "Error on PCK (negotiate command)";
			case 0x35: return "Bad parameter in command";
			case 0x38: return "TB3 absent";
			case 0x39: return "PPS not accepted (no answer from card)";
			case 0x3b: return "Early answer of the card during the activation";

			case 0x55: return "Unknown command";

			case 0x80: return "Card mute (after power on)";
			case 0x81: return "Time out (waiting time exceeded)";
			case 0x83: return "5 parity errors in reception";
			case 0x84: return "5 parity errors in transmission";
			case 0x86: return "Bad FiDi";
			case 0x88: return "ATR duration greater than 19200 etus (E.M.V.)";
			case 0x89: return "CWI not supported (E.M.V.)";
			case 0x8a: return "BWI not supported (E.M.V.)";
			case 0x8b: return "WI (Work waiting time) not supported (E.M.V.)";
			case 0x8c: return "TC3 not accepted (E.M.V.)";
			case 0x8d: return "Parity error during ATR";

			case 0x90: return "3 consecutive parity errors in T=1 protocol";
			case 0x91: return "SW1 different from 6X or 9X";
			case 0x92: return "Specific mode byte TA2 with b5 byte=1";
			case 0x93: return "TB1 absent during a cold reset (E.M.V.)";
			case 0x94: return "TB1 different from 00 during a cold reset (E.M.V.)";
			case 0x95: return "IFSC<10H or IFSC=FFH";
			case 0x96: return "Wrong TDi";
			case 0x97: return "TB2 is present in the ATR (E.M.V.)";
			case 0x98: return "TC1 is not compatible with CWT";
			case 0x9b: return "Not T=1 card";

			case 0xa0: return "Procedure byte error";
			case 0xa1: return "Card deactivated due to a hardware problem";

			case 0xc0: return "Card absent";
			case 0xc3: return "Checksum error";
			case 0xc4: return "TS is neither 3B nor 3F";
			case 0xc6: return "ATR not supported";
			case 0xc7: return "VPP is not supported";

			case 0xe1: return "Card clock frequency not accepted (after a set_clock_card command)";
			case 0xe2: return "UART overflow";
			case 0xe3: return "Supply voltage drop-off";
			case 0xe4: return "Temperature alarm";
			case 0xe9: return "Framing error";

			case 0xf0: return "Serial LRC error";
			case 0xff: return "Serial time out";

			default:   return "Unknown error";
		}
	}

private:
	uint8_t   _command;
	uint8_t  *_data;
	uint16_t  _length;
	uint8_t   _errno;
	uint8_t   _buffer[READ_BUFFER_SIZE];
};


#endif /* _ALPAR_H_ */

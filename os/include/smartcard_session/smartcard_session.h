/*
 * \brief  Smartcard session interface
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

#ifndef _INCLUDE__SMARTCARD_SESSION__SMARTCARD_SESSION_H_
#define _INCLUDE__SMARTCARD_SESSION__SMARTCARD_SESSION_H_

/* Genode includes */
#include <session/session.h>
#include <base/rpc.h>
#include <dataspace/capability.h>

namespace Smartcard {

	struct ReaderStatus
	{
		char     _reader_name[256];
		struct ATR {
			Genode::size_t  length;
			Genode::uint8_t data[64];
		} _atr;
		bool     _reader_present;
		bool     _card_present;
		unsigned _error_code;

		ReaderStatus()
		: _reader_present(false), _card_present(false), _error_code(0) 
		{ 
			_atr.length = 0; 
		}

		const char* reader_name() const { return _reader_name;    };
		bool     reader_present() const { return _reader_present; };
		bool     card_present()   const { return _card_present;   };
		ATR      atr()            const { return _atr;            };
		unsigned error_code()     const { return _error_code;     };
	};

	struct Session : Genode::Session
	{
		static const char   *service_name() { return "Smartcard"; }

		virtual ReaderStatus reader_status() const = 0;

		virtual bool         is_card_present() = 0;

		virtual bool         activate_card() = 0;
		virtual bool         deactivate_card() = 0;

		virtual bool         transmit(void const *cmd_buf, Genode::size_t cmd_length,
		                              void *resp_buf, Genode::size_t *resp_length) = 0;

		/*******************
		 ** RPC interface **
		 *******************/
		GENODE_RPC(Rpc_is_card_present,   bool,                         is_card_present);
		GENODE_RPC(Rpc_activate_card,     bool,                         activate_card);
		GENODE_RPC(Rpc_deactivate_card,   bool,                         deactivate_card);
		GENODE_RPC(Rpc_dataspace,         Genode::Dataspace_capability, _dataspace);
		GENODE_RPC(Rpc_reader_status,     void,                         _reader_status);
		GENODE_RPC(Rpc_transmit,          Genode::size_t,               _transmit, Genode::size_t);



		GENODE_RPC_INTERFACE(
			Rpc_is_card_present,
			Rpc_activate_card,
			Rpc_deactivate_card,
			Rpc_dataspace,
			Rpc_reader_status,
			Rpc_transmit
		);
	};
}

#endif /* _INCLUDE__SMARTCARD_SESSION__SMARTCARD_SESSION_H_ */

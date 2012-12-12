/*
 * \brief  Smartcard driver interface
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2012-10-13
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _SMARTCARD_DRIVER_H_
#define _SMARTCARD_DRIVER_H_

#include <smartcard_session/smartcard_session.h>

namespace Smartcard {

	struct Driver
	{
		virtual bool activate_card() = 0;
		virtual bool deactivate_card() = 0;

		virtual bool is_card_present() = 0;

		virtual void reader_status(ReaderStatus *status) = 0;

		virtual Genode::size_t transmit(const void *cmd_buf, Genode::size_t cmd_length, void *resp_buf) = 0;
	};
}

#endif /* _SMARTCARD_DRIVER_H_ */

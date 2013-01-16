/*
 * \brief  Timer delayer interface common header
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-01-12
 *
 * Many drivers need to poll for register state based within
 * a limited time period. This class provides the necessary implementation
 * for using the wait_for<register>(state, delayer) call
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__OS__TIMER_DELAYER_H_
#define _INCLUDE__OS__TIMER_DELAYER_H_

#include <util/mmio.h>
#include <timer_session/connection.h>

namespace Genode {

struct Timer_delayer : Timer::Connection, Mmio::Delayer {
	/**
	 * Implementation of 'Delayer' interface
	 */
	void usleep(unsigned us) {
		/* polling */
		if (us == 0)
			return;

		unsigned ms = us / 1000;
		if (ms == 0)
			ms = 1;

		Timer::Connection::msleep(ms);
	}
};

} //namespace Genode

#endif /* _INCLUDE__OS__TIMER_DELAYER_H_ */

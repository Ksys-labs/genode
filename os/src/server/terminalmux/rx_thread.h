/*
 * \brief  Receive thread from terminal
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2013-02-26
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _RX_THREAD_H_
#define _RX_THREAD_H_

#include <base/signal.h>
#include <base/thread.h>

#include "mux.h"

class Rx_thread : public Genode::Thread<8192>
{
	private:
		Mux                     *_mux;
		Genode::Signal_receiver *_sig_rec;

	protected:
		void entry()
		{
			while (true) {
				_sig_rec->wait_for_signal();
				_mux->char_avail();
			}
		}

	public:
		Rx_thread(Mux *mux, Genode::Signal_receiver *sig_rec)
		: Genode::Thread<8192>("rx-thread"), _mux(mux), _sig_rec(sig_rec)
		{
			start();
		}
};


#endif /* _RX_THREAD_H_ */

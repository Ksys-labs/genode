/*
 * \brief  Terminal root for Terminalmux
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

#ifndef _TERMINAL_ROOT_H_
#define _TERMINAL_ROOT_H_

/* Genode includes */
#include <cap_session/cap_session.h>
#include <root/component.h>
#include <os/session_policy.h>

/* local includes */
#include "terminal_session_component.h"

namespace Terminal {

	using namespace Genode;

	class Root : public Root_component<Session_component>
	{
		private:
			Mux                    *_mux;
			char                    _label[256];

		protected:
			Session_component *_create_session(const char *args)
			{
				static long channel = 0;

				try {
					Genode::Arg_string::find_arg(args, "label").string(_label, sizeof(_label), "");

					Session_policy policy(args);

					try {
						policy.attribute("channel").value(&channel);
					} catch(...) {}
					
					PINF("Terminalmux: use channel %ld for %s", channel, _label);

					return new (md_alloc()) Session_component(_mux, channel++);

				} catch (Session_policy::No_policy_defined) {
					PERR("Invalid session request, no matching policy");
					throw Root::Unavailable();
				}
			}
			
		public:
			/**
			 * Constructor
			 */
			Root(Rpc_entrypoint       *session_ep,
			     Allocator            *md_alloc,
			     Mux                  *mux)
			: Root_component<Session_component>(session_ep, md_alloc),
			  _mux(mux)
			{}
	};
}

#endif /* _TERMINAL_ROOT_H_ */

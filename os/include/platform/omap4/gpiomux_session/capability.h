/*
 * \brief  OMAP4 GPIO MUX capability type
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-01-15
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CAPABILITY_H_
#define _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CAPABILITY_H_

#include <base/capability.h>
#include <platform/omap4/gpiomux_session/gpiomux_session.h>

namespace GpioMux { typedef Genode::Capability<Session> Session_capability; }

#endif /* _INCLUDE__PLATFORM__OMAP4__GPIOMUX_SESSION__CAPABILITY_H_ */

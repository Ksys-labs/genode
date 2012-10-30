/*
 * \brief  Parts of kernel support that are identical for all Cortex A9 systems
 * \author Martin Stein
 * \date   2012-04-23
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CORE__INCLUDE__CORTEX_A9__KERNEL_SUPPORT_H_
#define _CORE__INCLUDE__CORTEX_A9__KERNEL_SUPPORT_H_

/* Core includes */
#include <cortex_a9/cpu/core.h>
#include <pic/pl390_base.h>

/**
 * CPU driver
 */
class Cpu : public Genode::Cortex_a9 { };

namespace Kernel
{
	/* import Genode types */
	typedef Genode::Cortex_a9 Cortex_a9;
	typedef Genode::Pl390_base Pl390_base;

	/**
	 * Kernel interrupt-controller
	 */
	class Pic : public Pl390_base
	{
		public:

			/**
			 * Constructor
			 */
			Pic() : Pl390_base(Cortex_a9::PL390_DISTRIBUTOR_MMIO_BASE,
			                   Cortex_a9::PL390_CPU_MMIO_BASE)
			{
				/* disable device */
				_distr.write<Distr::Icddcr::Enable>(0);
				_cpu.write<Cpu::Iccicr::Enable>(0);
				mask();

				/* supported priority range */
				unsigned const min_prio = _distr.min_priority();
				unsigned const max_prio = _distr.max_priority();

				/* configure every shared peripheral interrupt */
				for (unsigned i=MIN_SPI; i <= _max_interrupt; i++)
				{
					_distr.write<Distr::Icdicr::Edge_triggered>(0, i);
					_distr.write<Distr::Icdipr::Priority>(max_prio, i);
					_distr.write<Distr::Icdiptr::Cpu_targets>(Distr::Icdiptr::Cpu_targets::ALL, i);
				}

				/* disable the priority filter */
				_cpu.write<Cpu::Iccpmr::Priority>(min_prio);

				/* disable preemption of interrupt handling by interrupts */
				_cpu.write<Cpu::Iccbpr::Binary_point>(
					Cpu::Iccbpr::Binary_point::NO_PREEMPTION);

				/* enable device */
				_distr.write<Distr::Icddcr::Enable>(1);
				_cpu.write<Cpu::Iccicr::Enable>(1);
			}
	};

	/**
	 * Kernel timer
	 */
	class Timer : public Cortex_a9::Private_timer { };
}

#endif /* _CORE__INCLUDE__CORTEX_A9__KERNEL_SUPPORT_H_ */


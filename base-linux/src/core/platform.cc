/*
 * \brief   Linux platform interface implementation
 * \author  Norman Feske
 * \date    2006-06-13
 */

/*
 * Copyright (C) 2006-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/lock.h>
#include <linux_dataspace/client.h>

/* local includes */
#include "platform.h"
#include "core_env.h"
#include "server_socket_pair.h"

/* Linux includes */
#include <core_linux_syscalls.h>


using namespace Genode;


/**
 * Memory pool used for for core-local meta data
 */
static char _core_mem[80*1024*1024];


/*
 * Basic semaphore implementation based on the 'pipe' syscall.
 *
 * This alternative implementation is needed to be able to wake up the
 * blocked main thread from a signal handler executed by the same thread.
 */
class Pipe_semaphore
{
	private:

		int  _pipefd[2];

	public:

		Pipe_semaphore()
		{
			lx_pipe(_pipefd);
		}

		void down()
		{
			char dummy;
			while(lx_read(_pipefd[0], &dummy, 1) != 1);
		}

		void up()
		{
			char dummy;
			while (lx_write(_pipefd[1], &dummy, 1) != 1);
		}
};


static Pipe_semaphore _wait_for_exit_sem;  /* wakeup of '_wait_for_exit' */
static bool           _do_exit = false;    /* exit condition */


static void sigint_handler(int signum)
{
	_do_exit = true;
	_wait_for_exit_sem.up();
}


static void sigchld_handler(int signnum)
{
	_wait_for_exit_sem.up();
}


Platform::Platform()
: _core_mem_alloc(0)
{
	/* catch control-c */
	lx_sigaction(LX_SIGINT, sigint_handler);

	/* catch SIGCHLD */
	lx_sigaction(LX_SIGCHLD, sigchld_handler);

	/* create resource directory under /tmp */
	lx_mkdir(resource_path(), S_IRWXU);

	_core_mem_alloc.add_range((addr_t)_core_mem, sizeof(_core_mem));

	/*
	 * Occupy the socket handle that will be used to propagate the parent
	 * capability new processes. Otherwise, there may be the chance that the
	 * parent capability as supplied by the process creator will be assigned to
	 * this handle, which would result in a 'dup2' syscall taking
	 * PARENT_SOCKET_HANDLE as both source and target descriptor.
	 */
	lx_dup2(0, PARENT_SOCKET_HANDLE);
}


void Platform::wait_for_exit()
{
	for (;;) {

		/*
		 * Block until a signal occurs.
		 */
		_wait_for_exit_sem.down();

		/*
		 * Each time, the '_wait_for_exit_sem' gets unlocked, we could have
		 * received either a SIGINT or SIGCHLD. If a SIGINT was received, the
		 * '_exit' condition will be set.
		 */
		if (_do_exit)
			return;

		/*
		 * Reflect SIGCHLD as exception signal to the signal context of the CPU
		 * session of the process. Because multiple children could have been
		 * terminated, we iterate until 'pollpid' (wrapper around 'wait4')
		 * returns -1.
		 */
		for (;;) {
			int const pid = lx_pollpid();

			if (pid <= 0)
				break;

			Platform_thread::submit_exception(pid);
		}
	}
}


void Core_parent::exit(int exit_value)
{
	lx_exit_group(exit_value);
}


/*****************************
 ** Support for IPC library **
 *****************************/

namespace Genode {

	Native_connection_state server_socket_pair()
	{
		return create_server_socket_pair(Thread_base::myself()->tid().tid);
	}

	void destroy_server_socket_pair(Native_connection_state const &ncs)
	{
		/*
		 * As entrypoints in core are never destructed, this function is only
		 * called on IPC-client destruction. In this case, it's a no-op in core
		 * as well as in Genode processes.
		 */
		if (ncs.server_sd != -1 || ncs.client_sd != -1)
			PERR("%s called for IPC server which should never happen", __func__);
	}
}


/****************************************************
 ** Support for Platform_env_base::Rm_session_mmap **
 ****************************************************/

Genode::size_t
Platform_env_base::Rm_session_mmap::_dataspace_size(Capability<Dataspace> ds_cap)
{
	if (!ds_cap.valid())
		return Dataspace_capability::deref(ds_cap)->size();

	/* use RPC if called from a different thread */
	if (!core_env()->entrypoint()->is_myself()) {
		/* release Rm_session_mmap::_lock during RPC */
		_lock.unlock();
		Genode::size_t size = Dataspace_client(ds_cap).size();
		_lock.lock();
		return size;
	}

	/* use local function call if called from the entrypoint */
	Object_pool<Rpc_object_base>::Guard
		ds_rpc(core_env()->entrypoint()->lookup_and_lock(ds_cap));
	Dataspace * ds = dynamic_cast<Dataspace *>(&*ds_rpc);
	return ds ? ds->size() : 0;
}


int Platform_env_base::Rm_session_mmap::_dataspace_fd(Capability<Dataspace> ds_cap)
{
	if (!core_env()->entrypoint()->is_myself()) {
		/* release Rm_session_mmap::_lock during RPC */
		_lock.unlock();
		int socket = Linux_dataspace_client(ds_cap).fd().dst().socket;
		_lock.lock();
		return socket;
	}

	Capability<Linux_dataspace> lx_ds_cap = static_cap_cast<Linux_dataspace>(ds_cap);

	Object_pool<Rpc_object_base>::Guard
		ds_rpc(core_env()->entrypoint()->lookup_and_lock(lx_ds_cap));
	Linux_dataspace * ds = dynamic_cast<Linux_dataspace *>(&*ds_rpc);

	return ds ? ds->fd().dst().socket : -1;
}


bool Platform_env_base::Rm_session_mmap::_dataspace_writable(Dataspace_capability ds_cap)
{
	if (!core_env()->entrypoint()->is_myself()) {
		/* release Rm_session_mmap::_lock during RPC */
		_lock.unlock();
		bool writable = Dataspace_client(ds_cap).writable();
		_lock.lock();
		return writable;
	}

	Object_pool<Rpc_object_base>::Guard
		ds_rpc(core_env()->entrypoint()->lookup_and_lock(ds_cap));
	Dataspace * ds = dynamic_cast<Dataspace *>(&*ds_rpc);

	return ds ? ds->writable() : false;
}

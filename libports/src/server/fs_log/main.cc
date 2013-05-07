/*
 * \brief  LOG service that writes to a file
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-05-07
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/env.h>
#include <base/rpc_server.h>
#include <base/sleep.h>
#include <root/component.h>
#include <util/string.h>

#include <cap_session/connection.h>
#include <log_session/log_session.h>

namespace Unix {
#include <fcntl.h>
#include <unistd.h>
}

#define DEFAULT_FILENAME "/genode_log.txt"

namespace Genode {

	class FsLog_component : public Rpc_object<Log_session>
	{
		public:

			enum { LABEL_LEN = 64 };

		private:

			char                  _label[LABEL_LEN];
			size_t _label_len;
			int _fd;

		public:

			/**
			 * Constructor
			 */
			FsLog_component(const char *label, int fd) : _fd(fd) {
				snprintf(_label, LABEL_LEN, "[%s] ", label);
				_label_len = strlen(_label);
			}


			/*****************
			 ** Log session **
			 *****************/

			/**
			 * Write a log-message to the file.
			 */
			size_t write(String const &string_buf)
			{
				if (!(string_buf.is_valid_string())) {
					PERR("corrupted string");
					return 0;
				}

				char const *string = string_buf.string();
				int len = strlen(string);

				Unix::write(_fd, _label, _label_len);
				Unix::write(_fd, string, len);

				return len;
			}
	};


	class FsLog_root : public Root_component<FsLog_component>
	{
		protected:
			int _fd;

			/**
			 * Root component interface
			 */
			FsLog_component *_create_session(const char *args)
			{
				size_t ram_quota =
					Arg_string::find_arg(args, "ram_quota"  ).ulong_value(0);

				/* delete ram quota by the memory needed for the session */
				size_t session_size = max((size_t)4096, sizeof(FsLog_component));
				if (ram_quota < session_size) {
					PERR("out of memory");
					throw Root::Quota_exceeded();
				}

				char label_buf[FsLog_component::LABEL_LEN];
				
				Arg label_arg = Arg_string::find_arg(args, "label");
				label_arg.string(label_buf, sizeof(label_buf), "");

				return new (md_alloc()) FsLog_component(label_buf, _fd);
			}

		public:

			/**
			 * Constructor
			 *
			 * \param session_ep  entry point for managing cpu session objects
			 * \param md_alloc    meta-data allocator to be used by root component
			 */
			FsLog_root(Rpc_entrypoint *session_ep, Allocator *md_alloc, int fd)
			: Root_component<FsLog_component>(session_ep, md_alloc), _fd(fd) {}
	};
}


int main(int argc, char **argv)
{
	using namespace Genode;

	/*
	 * Initialize server entry point
	 */
	enum { STACK_SIZE = 4096 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "fslog_ep");

	int fd = Unix::open(DEFAULT_FILENAME, O_RDWR|O_CREAT);
	if (fd < 0) {
		PERR("unable to open %s", DEFAULT_FILENAME);
		throw Root::Unavailable();
	}

	static FsLog_root fslog_root(&ep, env()->heap(), fd);

	/*
	 * Announce services
	 */
	env()->parent()->announce(ep.manage(&fslog_root));

	/**
	 * Got to sleep forever
	 */
	sleep_forever();
	return 0;
}

#ifndef VGS_PROCTOOLS_POLLER_H
#define VGS_PROCTOOLS_POLLER_H

#include <sys/select.h>

namespace proctools
{

	class process;

	class poller
	{
	public:
		poller();

		void add(const process* process);
		bool is_set(const process* process) const;
		bool poll(int timeout);

	private:
		fd_set m_readfds;
		int m_maxfd;
	};

};

#endif // VGS_PROCTOOLS_POLLER_H

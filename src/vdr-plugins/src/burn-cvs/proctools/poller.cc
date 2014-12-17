#include "poller.h"
#include "process.h"
#include "logger.h"
#include "format.h"
#include <algorithm>
#include <sstream>

using std::max;
using std::string;

namespace proctools
{

	poller::poller():
			m_maxfd(-1)
	{
		FD_ZERO(&m_readfds);
	}

	void poller::add(const process* process)
	{
		FD_SET(process->pipe(), &m_readfds);
		m_maxfd = max(process->pipe(), m_maxfd);
	}

	bool poller::is_set(const process* process) const
	{
		return FD_ISSET(process->pipe(), &m_readfds);
	}

	bool poller::poll(int timeout)
	{
		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		if (select(m_maxfd + 1, &m_readfds, NULL, NULL, &tv) == -1) {
			logger::error("failure while tracking processes");
			return false;
		}
		return true;
	}

};

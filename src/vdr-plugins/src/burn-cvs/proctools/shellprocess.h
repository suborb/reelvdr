#ifndef VGS_PROCTOOLS_SHELLPROCESS_H
#define VGS_PROCTOOLS_SHELLPROCESS_H

#include "process.h"
#include "shellescape.h"
#include <string>
#include <sys/types.h>

namespace proctools
{

	// intended to start SCRIPTS that process the environment
	class shellprocess: public process
	{
	public:
		shellprocess(const std::string& name, const shellescape& command);

	protected:
		virtual std::string info(pid_t pid);
		virtual int run();

	private:
		std::string m_command;
	};

};

#endif // VGS_PROCTOOLS_SHELLPROCESS_H

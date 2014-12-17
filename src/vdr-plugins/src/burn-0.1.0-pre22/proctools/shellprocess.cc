#include "shellprocess.h"
#include "format.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
	      

using std::string;

namespace proctools
{

   shellprocess::shellprocess(const string& name, const shellescape& command, int priority):
   process( name ),
   m_command( str( command ) ),
   m_priority( priority )
   {
   }

   string shellprocess::info(pid_t pid)
   {
      return format("sh -c '{0}' (pid = {1})") % m_command % pid;
   }

   int shellprocess::run()
   {
      setpriority(PRIO_PGRP, 0, m_priority);	    
      execl("/bin/sh", "sh", "-c", m_command.c_str(), NULL);
      return(127);
   }

};

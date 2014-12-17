#include "process.h"
#include "chain.h"
#include "logger.h"
#include "format.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace proctools
{

   using namespace std;

   process::process(const string& name):
   m_parent(NULL),
   m_name(name),
   m_pid(-1),
   m_pipe(-1),
   m_returnStatus(none),
   m_returnCode(-1)
   {
   }

   process::~process()
   {
      if( m_pid != -1 )
         wait_for_exit(false);
   }

   bool process::start_if_idle()
   {
      if( m_pid != -1 )
         return true;

      int pipes[2];
      if( ::pipe(pipes) == -1 ) {
         logger::error("couldn't start process group (failed to pipe): ");
         return false;
      }

      if( (m_pid = fork()) == -1 ) {
         logger::error("couldn't start process group (failed to fork): ");
         return false;
      }

      if( m_pid == 0 ) {
         /* Child process */
         if( m_workdir != "" )
            chdir(m_workdir.c_str());

         close(STDOUT_FILENO); dup(pipes[1]);
         close(STDERR_FILENO); dup(pipes[1]);
         close(STDIN_FILENO);

         /* Close all dup'ed file descriptors */
         int max_fd = getdtablesize();
         for( int i = STDERR_FILENO + 1; i < max_fd; ++i )
            close(i);

         m_environment.set();
         setpgrp();
//         setpriority(PRIO_PGRP, 0, m_priority);
         setlocale(LC_ALL, "C");
         _exit(run());
      }

      logger::info(format("starting {0}") % info(m_pid));

      m_pipe = pipes[0];
      close(pipes[1]);
      return true;
   }

   bool process::read_pipe()
   {
      char buffer[BUFSIZ];
      int result;

      if( (result = read(m_pipe, buffer, sizeof(buffer))) == -1 ) {
         if( errno != EAGAIN && errno != EINTR ) {
            logger::error("failure reading process");
            return false;
         }
         return true; // nothing read, but try again anyway
      }

      if( result == 0 ) {
         wait_for_exit();
         return false;
      }

      buffer[result] = '\0';
      m_linebuf += buffer;

      string::size_type pos;
      while( (pos = m_linebuf.find_first_of("\015\012")) != string::npos ) {
         logger::text(format("[{0}] {1}")
                      % name() % m_linebuf.substr(0, pos));
         if( m_parent != NULL )
            m_parent->process_line(m_linebuf.substr(0, pos));
         m_linebuf.erase(0, pos + 1);
      }
      return true;
   }

   bool process::wait_for_exit(bool wait)
   {
      int pid = m_pid;
      m_pid = -1;

      close(m_pipe);

      int result, status;
      if( (result = waitpid(pid, &status, wait ? 0 : WNOHANG)) == 0 ) {
         kill(-pid, SIGTERM);
         usleep(wait ? 1000000 : 50000);

         if( (result = waitpid(pid, &status, WNOHANG)) == 0 ) {
            logger::error(
                         format("process {0} (pid = {1}) not ending, killing it...")
                         % m_name % pid, false);
            kill(-pid, SIGKILL);
            waitpid(pid, &status, 0);
            m_returnCode = 1;
            m_returnStatus = crash;
            return false;
         }
      }

      if( result == -1 ) {
         logger::error(
                      format("process {0} (pid = {1}) lost (this shouldn't happen!)")
                      % m_name % pid);
         m_returnCode = 0;
         m_returnStatus = crash;
         return false;
      }

      if( WIFSIGNALED(status) ) {
         logger::error(
                      format("process {0} (pid = {1}) crashed (signal {2})")
                      % m_name % pid % WTERMSIG(status), false);
         m_returnCode = WTERMSIG(status);
         m_returnStatus = crash;
         return false;
      }

      m_returnCode = WEXITSTATUS(status);
      m_returnStatus = m_returnCode == 0 ? ok : error;
      logger::debug(
                   format("process {0} (pid = {1}) exited gracefully (exit code {2})")
                   % m_name % pid % m_returnCode);
      return m_returnCode == 0;
   }

   template<>
   void process::put_environment(const string& name, const string& value)
   {
      m_environment.put(name, value);
   }

};

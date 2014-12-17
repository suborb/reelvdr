#include "chain-vdr.h"
#include "chain-dvd.h"
#include "chain-archive.h"
#include "logger-vdr.h"
#include "setup.h"
#include "burn.h"
#include "menuburn.h"
#include "proctools/format.h"
#include "proctools/logger.h"
#include "proctools/shellprocess.h"
#include "proctools/shellescape.h"
#include <memory>
#include <vdr/config.h>
#include <sys/time.h>
#include <sys/resource.h>

namespace vdr_burn
{

	using namespace std;
	using proctools::chain;
	using proctools::logger;
	using proctools::format;
	using proctools::process;
	using proctools::shellescape;

	chain_vdr::chain_vdr(const string& name, job& job_):
			chain(name),
#if VDRVERSNUM >= 10300
			cThread("burn: subprocess watcher"),
#endif
			m_job( job_ ),
			m_paths( create_temp_path( BurnParameters.TempPath ), create_temp_path( BurnParameters.DataPath ) ),
			m_progress(0),
			m_burning(false),
			m_burnProgress(0),
			m_canceled(false)
	{
		job_.set_paths(m_paths);
		logger_vdr::set_logfile( get_log_path() );
		start_time = time(NULL);
	}

	chain_vdr::~chain_vdr()
	{
		stop();
		if (return_status() != process::ok)
			execute( shellescape( "rm" ) + "-rf" + m_job.get_iso_path() );
		execute( shellescape( "rm" ) + "-rf" + m_paths.temp );
		execute( shellescape( "rm" ) + "-rf" + m_paths.data );
	}

	chain_vdr* chain_vdr::create_chain(job& job)
	{
		switch (job.get_disk_type()) {
		case disktype_dvd_menu:
		case disktype_dvd_nomenu: return new chain_dvd(job);

		case disktype_archive:    return new chain_archive(job);
		}

		logger::error( "chain_vdr::create_chain requested for unknown disktype" );
		BURN_THROW_DEBUG( "chain_vdr::create_chain requested for unknown disktype" );
		return 0;
	}

	void chain_vdr::stop()
	{
		logger::debug("stopping chain");
		m_canceled = true;
		chain::stop();
		Cancel(-1);
	}

	void chain_vdr::add_process(proctools::process* proc)
	{
		proc->set_workdir(m_paths.data);
		chain::add_process(proc);
	}

	string chain_vdr::create_temp_path( const string& pathPrefix )
	{
		static const string pathTemplate( "{0}/vdr-burn.{1}.XXXXXX" );

		string tempPath( format( pathTemplate ) % pathPrefix % string_replace( m_job.get_title(), '/', '~' ) );
		string cleanPath( clean_path_name( tempPath ) );
		protected_array<char> path( new char[ cleanPath.length() + 1 ] );
		strcpy( path.get(), cleanPath.c_str() );
		mkdtemp( path.get() );
		return path.get();
	}

	void chain_vdr::all_done()
	{
		logger_vdr::close_logfile();

		if (global_setup().PreserveLogFiles) {
			string target = BurnParameters.IsoPath.empty() ? BurnParameters.TempPath : BurnParameters.IsoPath;
			execute( shellescape( "vdrburn-copylog.sh" ) + get_log_path() +
					 str( format( "{0}/vdrburn-{1}.log" ) % target % m_job.get_title() ) );
		}
	}

	void chain_vdr::Action()
	{
#if VDRVERSNUM < 10300
		logger::info(format("subprocess watcher thread started (pid={0})")
					 % getpid());
		setpriority(PRIO_PROCESS, 0, 19);
#else
		SetPriority(19);
#endif

		run();

#if VDRVERSNUM < 10300
		logger::info(format("subprocess watcher thread stopped (pid={0})")
					 % getpid());
#endif
	}
};

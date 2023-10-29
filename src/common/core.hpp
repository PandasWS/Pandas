﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CORE_HPP
#define CORE_HPP

#include <config/pandas.hpp>

#include <string>
#include <vector>

#include "timer.hpp"

#ifdef _WIN32
#ifndef Pandas_Console_Translate
	#include "winapi.hpp" // Console close event handling
#endif // Pandas_Console_Translate
#endif


#ifdef Pandas_Google_Breakpad
#include "crashdump.hpp"
#endif // Pandas_Google_Breakpad

/* so that developers with --enable-debug can raise signals from any section of the code they'd like */
#ifdef DEBUG
	#include <signal.h>
#endif

#if defined(BUILDBOT)
	extern int buildbotflag;
#endif

#define UNKNOWN_VERSION '\x02'

extern char *SERVER_NAME;
extern char db_path[12]; /// relative path for db from servers
extern char conf_path[12]; /// relative path for conf from servers

extern int parse_console(const char* buf);
const char *get_svn_revision(void);
const char *get_git_hash(void);

namespace rathena{
	namespace server_core{
		enum class e_core_status{
			NOT_STARTED,
			CORE_INITIALIZING,
			CORE_INITIALIZED,
			SERVER_INITIALIZING,
			SERVER_INITIALIZED,
			RUNNING,
			STOPPING,
			SERVER_FINALIZING,
			SERVER_FINALIZED,
			CORE_FINALIZING,
			CORE_FINALIZED,
			STOPPED,
		};

		enum class e_core_type{
			LOGIN,
			CHARACTER,
			MAP,
			TOOL,
			WEB
		};

		class Core{
			private:
				e_core_status m_status;
				e_core_type m_type;
				bool m_run_once;
				bool m_crashed;

			protected:
				virtual bool initialize( int argc, char* argv[] );
				virtual void handle_main( t_tick next );
				virtual void finalize();
				virtual void handle_crash();
				virtual void handle_shutdown();
				void set_status( e_core_status status );

			public:
				Core( e_core_type type ){
					this->m_status = e_core_status::NOT_STARTED;
					this->m_run_once = false;
					this->m_crashed = false;
					this->m_type = type;
				}

				e_core_status get_status();
				e_core_type get_type();
				bool is_running();
				// TODO: refactor to protected
				void set_run_once( bool run_once );
				void signal_crash();
				void signal_shutdown();
				int start( int argc, char* argv[] );
		};
	}
}

extern rathena::server_core::Core* global_core;

template <typename T> int main_core( int argc, char *argv[] ){
	T server = {};

	global_core = &server;

	return server.start( argc, argv );
}

#endif /* CORE_HPP */

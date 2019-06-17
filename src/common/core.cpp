﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "core.hpp"

#ifndef MINICORE
#include "ers.hpp"
#include "socket.hpp"
#include "timer.hpp"
#include "sql.hpp"
#endif
#include <stdlib.h>
#include <signal.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include "winapi.hpp" // Console close event handling
#include <direct.h> // _chdir
#endif

#include "cbasetypes.hpp"
#include "malloc.hpp"
#include "mmo.hpp"
#include "showmsg.hpp"
#include "strlib.hpp"

#ifdef Pandas_Crash_Report
#include "minidump.hpp"

// 当程序崩溃时生成的转储文件类型
// 这里的设置会被 login_athena.conf、char-athena.conf、map-athena.conf 中的选项调整
//
// 设置为 1 则表示生成 FullDump, 虽然保存信息完整, 但体积较大 (默认值)
// 设置为 0 则表示生成 MiniDump, 虽然体积较小, 但是保存的信息也比较少
int create_fulldump = 0;
#endif // Pandas_Crash_Report

/// Called when a terminate signal is received.
void (*shutdown_callback)(void) = NULL;

#if defined(BUILDBOT)
	int buildbotflag = 0;
#endif

int runflag = CORE_ST_RUN;
char db_path[12] = "db"; /// relative path for db from server
char conf_path[12] = "conf"; /// relative path for conf from server

char *SERVER_NAME = NULL;
char SERVER_TYPE = ATHENA_SERVER_NONE;

#ifndef MINICORE	// minimalist Core
// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifdef WIN32	// windows don't have SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef POSIX
#define compat_signal(signo, func) signal(signo, func)
#else
sigfunc *compat_signal(int signo, sigfunc *func) {
	struct sigaction sact, oact;

	sact.sa_handler = func;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
#ifdef SA_INTERRUPT
	sact.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &sact, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif

/*======================================
 *	CORE : Console events for Windows
 *--------------------------------------*/
#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD c_event) {
    switch(c_event) {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		if( shutdown_callback != NULL )
			shutdown_callback();
		else
			runflag = CORE_ST_STOP;// auto-shutdown
        break;
	default:
		return FALSE;
    }
    return TRUE;
}

static void cevents_init() {
	if (SetConsoleCtrlHandler(console_handler,TRUE)==FALSE)
		ShowWarning ("Unable to install the console handler!\n");
}
#endif

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------*/
static void sig_proc(int sn) {
	static int is_called = 0;

	switch (sn) {
	case SIGINT:
	case SIGTERM:
		if (++is_called > 3)
			exit(EXIT_SUCCESS);
		if( shutdown_callback != NULL )
			shutdown_callback();
		else
			runflag = CORE_ST_STOP;// auto-shutdown
		break;
	case SIGSEGV:
	case SIGFPE:
		do_abort();
		// Pass the signal to the system's default handler
		compat_signal(sn, SIG_DFL);
#ifndef Pandas_Crash_Report
		// 启用崩溃转储文件生成机制的时候
		// 这里就不再需要 rAthena 自己进行错误处理了, 否则会捕获不到异常 [Sola丶小克]
		raise(sn);
#endif // Pandas_Crash_Report
		break;
#ifndef _WIN32
	case SIGXFSZ:
		// ignore and allow it to set errno to EFBIG
		ShowWarning ("Max file size reached!\n");
		//run_flag = 0;	// should we quit?
		break;
	case SIGPIPE:
		//ShowInfo ("Broken pipe found... closing socket\n");	// set to eof in socket.cpp
		break;	// does nothing here
#endif
	}
}

void signals_init (void) {
	compat_signal(SIGTERM, sig_proc);
	compat_signal(SIGINT, sig_proc);
#ifndef _DEBUG // need unhandled exceptions to debug on Windows
	compat_signal(SIGSEGV, sig_proc);
	compat_signal(SIGFPE, sig_proc);
#endif
#ifndef _WIN32
	compat_signal(SIGILL, SIG_DFL);
	compat_signal(SIGXFSZ, sig_proc);
	compat_signal(SIGPIPE, sig_proc);
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL);
#endif
}
#endif

const char* get_svn_revision(void) {
#ifdef SVNVERSION
	return EXPAND_AND_QUOTE(SVNVERSION);
#else// not SVNVERSION
	static char svn_version_buffer[16] = "";
	FILE *fp;

	if( svn_version_buffer[0] != '\0' )
		return svn_version_buffer;

	// subversion 1.7 uses a sqlite3 database
	// FIXME this is hackish at best...
	// - ignores database file structure
	// - assumes the data in NODES.dav_cache column ends with "!svn/ver/<revision>/<path>)"
	// - since it's a cache column, the data might not even exist
	if( (fp = fopen(".svn" PATHSEP_STR "wc.db", "rb")) != NULL || (fp = fopen(".." PATHSEP_STR ".svn" PATHSEP_STR "wc.db", "rb")) != NULL )
	{
	#ifndef SVNNODEPATH
		//not sure how to handle branches, so i'll leave this overridable define until a better solution comes up
		#define SVNNODEPATH trunk
	#endif
		const char* prefix = "!svn/ver/";
		const char* postfix = "/" EXPAND_AND_QUOTE(SVNNODEPATH) ")"; // there should exist only 1 entry like this
		size_t prefix_len = strlen(prefix);
		size_t postfix_len = strlen(postfix);
		size_t i,j,len;
		char* buffer;

		// read file to buffer
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		buffer = (char*)aMalloc(len + 1);
		fseek(fp, 0, SEEK_SET);
		len = fread(buffer, 1, len, fp);
		buffer[len] = '\0';
		fclose(fp);

		// parse buffer
		for( i = prefix_len + 1; i + postfix_len <= len; ++i ) {
			if( buffer[i] != postfix[0] || memcmp(buffer + i, postfix, postfix_len) != 0 )
				continue; // postfix missmatch
			for( j = i; j > 0; --j ) {// skip digits
				if( !ISDIGIT(buffer[j - 1]) )
					break;
			}
			if( memcmp(buffer + j - prefix_len, prefix, prefix_len) != 0 )
				continue; // prefix missmatch
			// done
			snprintf(svn_version_buffer, sizeof(svn_version_buffer), "%d", atoi(buffer + j));
			break;
		}
		aFree(buffer);

		if( svn_version_buffer[0] != '\0' )
			return svn_version_buffer;
	}

	// subversion 1.6 and older?
	if ((fp = fopen(".svn/entries", "r")) != NULL)
	{
		char line[1024];
		int rev;
		// Check the version
		if (fgets(line, sizeof(line), fp))
		{
			if(!ISDIGIT(line[0]))
			{
				// XML File format
				while (fgets(line,sizeof(line),fp))
					if (strstr(line,"revision=")) break;
				if (sscanf(line," %*[^\"]\"%11d%*[^\n]", &rev) == 1) {
					snprintf(svn_version_buffer, sizeof(svn_version_buffer), "%d", rev);
				}
			}
			else
			{
				// Bin File format
				if ( fgets(line, sizeof(line), fp) == NULL ) { printf("Can't get bin name\n"); } // Get the name
				if ( fgets(line, sizeof(line), fp) == NULL ) { printf("Can't get entries kind\n"); } // Get the entries kind
				if(fgets(line, sizeof(line), fp)) // Get the rev numver
				{
					snprintf(svn_version_buffer, sizeof(svn_version_buffer), "%d", atoi(line));
				}
			}
		}
		fclose(fp);

		if( svn_version_buffer[0] != '\0' )
			return svn_version_buffer;
	}

	// fallback
	svn_version_buffer[0] = UNKNOWN_VERSION;
	return svn_version_buffer;
#endif
}

// Grabs the hash from the last time the user updated their working copy (last pull)
const char *get_git_hash (void) {
	static char GitHash[41] = ""; //Sha(40) + 1
	FILE *fp;

	if( GitHash[0] != '\0' )
		return GitHash;

	if( (fp = fopen(".git/refs/remotes/origin/master", "r")) != NULL || // Already pulled once
		(fp = fopen(".git/refs/heads/master", "r")) != NULL ) { // Cloned only
		char line[64];
		char *rev = (char*)malloc(sizeof(char) * 50);

		if( fgets(line, sizeof(line), fp) && sscanf(line, "%40s", rev) )
			snprintf(GitHash, sizeof(GitHash), "%s", rev);

		free(rev);
		fclose(fp);
	} else {
		GitHash[0] = UNKNOWN_VERSION;
	}

	if ( !(*GitHash) ) {
		GitHash[0] = UNKNOWN_VERSION;
	}

	return GitHash;
}

/*======================================
 *	CORE : Display title
 *  ASCII By CalciumKid 1/12/2011
 *--------------------------------------*/
static void display_title(void) {
#ifndef Pandas_Show_Version
	const char* svn = get_svn_revision();
	const char* git = get_git_hash();
#endif // Pandas_Show_Version

#ifndef Pandas_Show_Logo
	ShowMessage("\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "                                                                 " CL_PASS"" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "       " CL_BT_WHITE "            rAthena Development Team presents                  " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "                 ___   __  __                                    " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "           _____/   | / /_/ /_  ___  ____  ____ _                " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "          / ___/ /| |/ __/ __ \\/ _ \\/ __ \\/ __ `/                " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "         / /  / ___ / /_/ / / /  __/ / / / /_/ /                 " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "        /_/  /_/  |_\\__/_/ /_/\\___/_/ /_/\\__,_/                  " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "                                                                 " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "       " CL_GREEN "              http://rathena.org/board/                        " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_PASS "     " CL_BOLD "                                                                 " CL_PASS "" CL_CLL "" CL_NORMAL "\n");
#else
	ShowMessage("\n");
	ShowMessage("" CL_BG_RED "     " CL_BOLD "                                                                      " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "                       Pandas Dev Team Presents                   " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "                ____                    _                         " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "               |  _ \\  __ _  _ __    __| |  __ _  ___            " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "               | |_) |/ _` || '_ \\  / _` | / _` |/ __|           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "               |  __/| (_| || | | || (_| || (_| |\\__ \\          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "               |_|    \\__,_||_| |_| \\__,_| \\__,_||___/         " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "                                                                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_GREEN "                          https://pandas.ws/                         " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BOLD "                                                                      " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
#ifdef Pandas_Disclaimer
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "          Pandas is only for learning and research purposes.      " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BT_WHITE "                 Please don't use it for commercial.              " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED "     " CL_BOLD "                                                                      " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
#endif // Pandas_Disclaimer
	ShowMessage("\n");
#endif // Pandas_Show_Logo

#ifndef Pandas_Show_Version
	if( svn[0] != UNKNOWN_VERSION )
		ShowInfo("SVN Revision: '" CL_WHITE "%s" CL_RESET "'\n", svn);
	else if( git[0] != UNKNOWN_VERSION )
		ShowInfo("Git Hash: '" CL_WHITE "%s" CL_RESET "'\n", git);
#else
	// 显示 Pandas 的版本号 [Sola丶小克]
	ShowInfo("Pandas Version: " CL_WHITE "%s" CL_RESET "\n", GetPandasVersion().c_str());
#endif // Pandas_Show_Version

#ifdef Pandas_Disclaimer
	ShowInfo("This program is completely free! You don't need to pay for it.\n");
#endif // Pandas_Disclaimer
}

// Warning if executed as superuser (root)
void usercheck(void)
{
#if !defined(BUILDBOT)
#ifdef _WIN32
	if (IsCurrentUserLocalAdministrator()) {
		ShowWarning("You are running rAthena with admin privileges, it is not necessary.\n");
	}
#else
	if (geteuid() == 0) {
		ShowWarning ("You are running rAthena with root privileges, it is not necessary.\n");
	}
#endif
#endif
}

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------*/
int main (int argc, char **argv)
{
#ifdef Pandas_Crash_Report
	SetUnhandledExceptionFilter(Pandas_UnhandledExceptionFilter);
#endif // Pandas_Crash_Report

	{// initialize program arguments
		char *p1;
		if((p1 = strrchr(argv[0], '/')) != NULL ||  (p1 = strrchr(argv[0], '\\')) != NULL ){
			char *pwd = NULL; //path working directory
			int n=0;
			SERVER_NAME = ++p1;
			n = p1-argv[0]; //calc dir name len

#ifdef Pandas_LGTM_Optimization
			// 对通过参数传入的工作路径进行长度限制判断 (暂定为 1kb 的长度)
			n = (n > 1024 ? 1024 : n);
#endif // Pandas_LGTM_Optimization

			pwd = safestrncpy((char*)malloc(n + 1), argv[0], n);
			if(chdir(pwd) != 0)
				ShowError("Couldn't change working directory to %s for %s, runtime will probably fail",pwd,SERVER_NAME);
			free(pwd);
		}else{
			// On Windows the .bat files have the executeable names as parameters without any path seperator [Lemongrass]
			SERVER_NAME = argv[0];
		}
	}

	malloc_init();// needed for Show* in display_title() [FlavioJS]

#ifdef MINICORE // minimalist Core
	display_title();
	usercheck();
	do_init(argc,argv);
	do_final();
#else// not MINICORE
	set_server_type();
	display_title();
	usercheck();

	Sql_Init();
	db_init();
	signals_init();

#ifdef _WIN32
	cevents_init();
#endif

	timer_init();
	socket_init();

	do_init(argc,argv);

	// Main runtime cycle
	while (runflag != CORE_ST_STOP) { 
		t_tick next = do_timer(gettick_nocache());
		do_sockets(next);
	}

	do_final();

	timer_final();
	socket_final();
	db_final();
	ers_final();
#endif

	malloc_final();

#if defined(BUILDBOT)
	if( buildbotflag ){
		exit(EXIT_FAILURE);
	}
#endif

	return 0;
}

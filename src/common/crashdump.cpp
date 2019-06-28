// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "crashdump.hpp"

#include <string> // std::string, std::wstring

#include "assistant.hpp" // makePath, ws2s
#include "showmsg.hpp"

#ifdef _WIN32
#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"
#else
#include "client/linux/handler/exception_handler.h"
#endif // _WIN32

// 当程序崩溃时, 将转储文件保存在什么位置
std::wstring g_dumpSaveDirectory = L"log/dumps";
google_breakpad::ExceptionHandler* g_pExceptionHandler = NULL;

#ifdef _WIN32

//************************************
// Method:		breakpad_callback
// Description:	当转储文件生成成功后, 会触发此回调函数
// Parameter:	const wchar_t * dump_path
// Parameter:	const wchar_t * minidump_id
// Parameter:	void * context
// Parameter:	EXCEPTION_POINTERS * exinfo
// Parameter:	MDRawAssertionInfo * assertion
// Parameter:	bool succeeded
// Returns:		bool
//************************************
bool breakpad_callback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context,
	EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded)
{
	// 若回调过来的时候发现崩溃转储文件生成失败了, 那么这里什么都不做.
	// 返回 FALSE 的话, breakpad 内部会尝试调用标准的错误处理方法, 试图抢救一下...
	if (!succeeded) {
		return succeeded;
	}

	std::wstring filepath;
	filepath = stdStringFormat(filepath, L"%s\\%s.dmp", dump_path, minidump_id);
	ensurePathSep(filepath);

	ShowMessage("\n");
	ShowMessage("" CL_BG_RED CL_BOLD     "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                           Pandas Dev Team Apologetic                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                    ____                   _                _                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                   / ___| _ __  __ _  ___ | |__    ___   __| |                 " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                  | |    | '__|/ _` |/ __|| '_ \\  / _ \\ / _` |               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                  | |___ | |  | (_| |\\__ \\| | | ||  __/| (_| |               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                   \\____||_|   \\__,_||___/|_| |_| \\___| \\__,_|             " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_GREEN    "                               https://pandas.ws/                              " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BOLD     "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " The program has stopped working. We are very apologetic about this.           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " We created a crash dump file and written into the following location:         " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_YELLOW   " %s "                                                                            CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n", wstring2string(filepath).c_str());
	ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " We will trying to send the dump file to the developer,                        " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " the sending process may take a few seconds, please be patient.                " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " The program will shutdown automatically after sent.                           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");

	ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending...                                                                    " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");

	// 发送崩溃转储文件给服务端进行记录

	ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BT_WHITE " Thank you for your cooperation. We will resolve this issue ASAP.              " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	ShowMessage("" CL_BG_RED CL_BOLD     "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");

	return succeeded;
}

//************************************
// Method:		breakpad_initialize
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_initialize() {
	makeDirectories(wstring2string(ensurePathSep(g_dumpSaveDirectory)));

	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		g_dumpSaveDirectory, 0, breakpad_callback, 0,
		google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, L"", 0
	);
}

#else

// 在 Linux 环境下 Breakpad 的使用方法与 Windows 有差异

bool breakpad_callback(const google_breakpad::MinidumpDescriptor& descriptor,
	void* context, bool succeeded)
{
	std::string filepath = descriptor.path();
	ensurePathSep(filepath);

	printf("Dump path: %s\n", filepath.c_str());
	return succeeded;
}

//************************************
// Method:		breakpad_initialize
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_initialize() {
	makeDirectories(wstring2string(ensurePathSep(g_dumpSaveDirectory)));
	google_breakpad::MinidumpDescriptor descriptor(wstring2string(g_dumpSaveDirectory).c_str());

	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		descriptor, NULL, breakpad_callback, NULL, true, -1
	);
}

#endif // _WIN32

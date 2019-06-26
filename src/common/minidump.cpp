// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "minidump.hpp"

#include <string> // std::string, std::wstring
#include "assistant.hpp" // makePath, ws2s

#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"

// 当程序崩溃时, 将转储文件保存在什么位置
std::wstring g_dumpSaveDirectory = L"log\\dumps";

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
	return succeeded;
}

//************************************
// Method:		breakpad_setup
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_setup() {
	makeDirectories(wstring2string(g_dumpSaveDirectory));
	google_breakpad::ExceptionHandler *pHandler = new google_breakpad::ExceptionHandler(
		g_dumpSaveDirectory, 0, breakpad_callback, 0,
		google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, L"", 0
	);
}

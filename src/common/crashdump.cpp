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

// 设置 crash.checkpoint 检查点记录文件保存在什么位置
std::wstring g_crashCheckPointFilepath = g_dumpSaveDirectory + L"/crash.checkpoint";

// 用于上报崩溃转储文件的数据接口地址
std::wstring g_crashDumpUploadInterface = L"http://crashrpt.pandas.ws/upload";

// 保存程序崩溃时负责转储工作的 ExceptionHandler 指针
google_breakpad::ExceptionHandler* g_pExceptionHandler = NULL;

// 用于标记本程序的 Breakpad 是否已经完成了初始化
bool g_BreakpadInitialized = false;

//************************************
// Method:		display_crashtips
// Description:	当程序崩溃时, 用于显示一些提示信息到终端里
// Parameter:	std::string dumpfilepath
// Parameter:	bool bottom
// Returns:		void
//************************************
void display_crashtips(std::string dumpfilepath, bool bottom) {
	if (!bottom) {
		ShowMessage("\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " The program has stopped working. We are very apologetic about this.           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " We created a crash dump file and written into the following location:         " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_YELLOW   " %s "                                                                            CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n", dumpfilepath.c_str());
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " We will trying to send the dump file to the developer,                        " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " the sending process may take a few seconds, please be patient.                " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " The program will shutdown automatically after sent.                           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	}
	else {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " Thank you for your cooperation. We will resolve this issue ASAP.              " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE "                                                                               " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("\n");
	}
}

//************************************
// Method:		breakpad_status
// Description:	确认当前 Breakpad 的状态是否已初始化成功
// Returns:		void
//************************************
void breakpad_status() {
	if (g_BreakpadInitialized)
		ShowStatus("Google Breakpad initialised: " CL_WHITE "%s/" CL_RESET "\n", wstring2string(g_dumpSaveDirectory).c_str());
	else
		ShowWarning("Google Breakpad initialization failed!\n");
}

#ifdef _WIN32

//************************************
// Method:		breakpad_callback
// Description:	当转储文件的生成过程结束后, 会触发此回调函数
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
	if (!succeeded) return succeeded;

	// 崩溃转储文件的本地保存路径
	std::wstring filepath;
	filepath = stdStringFormat(filepath, L"%s/%s.dmp", dump_path, minidump_id);

	// 将崩溃转储文件存入一个 std::map 中等待发送请求使用
	std::map<std::wstring, std::wstring> files;
	files.insert(std::make_pair(L"dumpfile", filepath));

	// 收集其他可能需要的信息, 用于服务端分析或归类使用
	std::map<std::wstring, std::wstring> data;
	data.insert(std::make_pair(L"platfrom", L"windows"));

	display_crashtips(wstring2string(filepath), false);
	ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending the crash dump file, please wait a second...                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");

	// 创建 CrashReportSender 对象并利用其提交转储文件给服务器
	google_breakpad::CrashReportSender sender(g_crashCheckPointFilepath);

	// 根据发送的结果给与提示信息
	if (sender.SendCrashReport(g_crashDumpUploadInterface, data, files, 0) ==
		google_breakpad::ReportResult::RESULT_SUCCEEDED)
		ShowMessage("" CL_BG_RED CL_BT_GREEN " SUCCESS : The crash dump file has been uploaded successfull.                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	else
		ShowMessage("" CL_BG_RED CL_BT_CYAN  " FAILED: The crash dump file failed to upload, please report to developer.     " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");

	display_crashtips(wstring2string(filepath), true);

	return succeeded;
}

//************************************
// Method:		breakpad_initialize
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_initialize() {
	if (!makeDirectories(wstring2string(g_dumpSaveDirectory))) return;

	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		g_dumpSaveDirectory, 0, breakpad_callback, 0,
		google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, L"", 0
	);

	g_BreakpadInitialized = true;
}

#else

// 在 Linux 环境下 Breakpad 的使用方法与 Windows 有差异

//************************************
// Method:		breakpad_callback
// Description:	当转储文件的生成过程结束后, 会触发此回调函数
// Parameter:	const google_breakpad::MinidumpDescriptor & descriptor
// Parameter:	void * context
// Parameter:	bool succeeded
// Returns:		bool
//************************************
bool breakpad_callback(const google_breakpad::MinidumpDescriptor& descriptor,
	void* context, bool succeeded)
{
	// 若回调过来的时候发现崩溃转储文件生成失败了, 那么这里什么都不做.
	// 返回 FALSE 的话, breakpad 内部会尝试调用标准的错误处理方法, 试图抢救一下...
	if (!succeeded) return succeeded;

	std::string filepath = descriptor.path();

	display_crashtips(filepath, false);
	
	ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending the crash dump file, please wait a second...                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	// 发送崩溃转储文件给服务端进行记录

	display_crashtips(filepath, true);

	return succeeded;
}

//************************************
// Method:		breakpad_initialize
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_initialize() {
	if (!makeDirectories(wstring2string(g_dumpSaveDirectory))) return;

	google_breakpad::MinidumpDescriptor descriptor(wstring2string(g_dumpSaveDirectory).c_str());

	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		descriptor, NULL, breakpad_callback, NULL, true, -1
	);

	g_BreakpadInitialized = true;
}

#endif // _WIN32

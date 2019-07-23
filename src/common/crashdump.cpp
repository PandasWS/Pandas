// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "crashdump.hpp"

#include <string> // std::string, std::wstring

#include "cryptopp.hpp"
#include "assistant.hpp" // makeDirectories, wstring2string
#include "showmsg.hpp"

#ifdef _WIN32
#include "client/windows/handler/exception_handler.h"
#include "client/windows/sender/crash_report_sender.h"
#else
#include "common/linux/http_upload.h"
#include "client/linux/handler/exception_handler.h"
#endif // _WIN32

// 目前仅支持对 PandasWS 官方编译的 Release 版本进行崩溃转储文件分析.
// 只有认识的程序的符号文件在分析服务器上, 分析才有意义.
//
// 为了避免服务端分析不认识的转储文件, 导致分析资源的浪费, 这里采用了一个简单的验证机制,
// 来让服务端识别是否为认识的程序所产生的崩溃.
// 
// 当程序崩溃后往 g_crashDumpUploadInterface 接口发送崩溃转储文件时,
// 会根据此处定义的公钥来构建一个 token 来发给服务端, 服务端采用私钥进行简单的身份验证.
// 
// 待未来整个崩溃分析机制成熟完善, 会考虑将分析能力开放给其他需要的合作伙伴.

#ifndef CRASHRPT_APPID
	#define CRASHRPT_APPID L""
#endif // CRASHRPT_APPID

#ifndef CRASHRPT_PUBLICKEY
	#define CRASHRPT_PUBLICKEY L""
#endif // CRASHRPT_PUBLICKEY

// 当程序崩溃时, 将转储文件保存在什么位置
std::wstring g_dumpSaveDirectory = L"log/dumps";

// 设置 crash.checkpoint 检查点记录文件保存在什么位置
std::wstring g_crashCheckPointFilepath = g_dumpSaveDirectory + L"/crash.checkpoint";

// 用于上报崩溃转储文件的数据接口地址
std::wstring g_crashDumpUploadInterface = strFormat(L"http://crashrpt.pandas.ws/upload?appid=%s", CRASHRPT_APPID);

// 保存程序崩溃时负责转储工作的 ExceptionHandler 指针
google_breakpad::ExceptionHandler* g_pExceptionHandler = NULL;

// 保存用于上报转储文件时所使用的令牌
std::wstring g_crashDumpUploadToken = L"";

// 用于标记本程序的 Breakpad 是否已经完成了初始化
bool g_BreakpadInitialized = false;

// 用于标记本程序是否具备上报崩溃转储文件的条件 (设置了 AppID 且已经成功构建 Token)
bool g_CrashDumpUploadAllowed = false;

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
		if (g_CrashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " We will trying to send the dump file to the developer,                        " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " the sending process may take a few seconds, please be patient.                " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " The program will shutdown automatically after sent.                           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		}
	}
	else {
		if (g_CrashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " Thank you for your cooperation. We will resolve this issue ASAP.              " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		}
		else {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " This is a modified version, please analysis the crashdump by yourself.        " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		}
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
	std::wstring filepath = strFormat(L"%s/%s.dmp", dump_path, minidump_id);

	// 将崩溃转储文件存入一个 std::map 中等待发送请求使用
	std::map<std::wstring, std::wstring> files;
	files.insert(std::make_pair(L"dumpfile", filepath));

	// 收集其他可能需要的信息, 用于服务端分析或归类使用
	std::map<std::wstring, std::wstring> data;
	data.insert(std::make_pair(L"token", g_crashDumpUploadToken));
	data.insert(std::make_pair(L"platform", L"windows"));

	// 创建 CrashReportSender 对象并利用其提交转储文件给服务器
	google_breakpad::CrashReportSender sender(g_crashCheckPointFilepath);

	display_crashtips(wstring2string(filepath), false);
	if (g_CrashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending the crash dump file, please wait a second...                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	if (sender.SendCrashReport(g_crashDumpUploadInterface, data, files, 0) ==
		google_breakpad::ReportResult::RESULT_SUCCEEDED)
		ShowMessage("" CL_BG_RED CL_BT_GREEN " SUCCESS : The crash dump file has been uploaded successfull.                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	else
		ShowMessage("" CL_BG_RED CL_BT_CYAN  " FAILED: The crash dump file failed to upload, please report to developer.     " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	}
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

	// 提前创建用于上报转储文件时所使用的令牌
	// 因为当程序崩溃的时候, 回调函数中的代码应该尽量少的进行任何多余操作
	if (std::wstring(CRASHRPT_APPID).length() > 0 &&
		std::wstring(CRASHRPT_PUBLICKEY).length() > 0) {
		std::wstring tokeninfo = strFormat(
			L"PANDASWS|%s|%d", CRASHRPT_APPID, time(NULL)
		);
		g_crashDumpUploadToken = string2wstring(crypto_RSAEncryptString(
			wstring2string(std::wstring(CRASHRPT_PUBLICKEY)),
			wstring2string(tokeninfo)
		));
		g_CrashDumpUploadAllowed = true;
	}

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

	// 提前创建用于上报转储文件时所使用的令牌
	// 因为当程序崩溃的时候, 回调函数中的代码应该尽量少的进行任何多余操作
	if (std::wstring(CRASHRPT_APPID).length() > 0 &&
		std::wstring(CRASHRPT_PUBLICKEY).length() > 0) {
		std::wstring tokeninfo = strFormat(
			L"PANDASWS|%s|%d", CRASHRPT_APPID, time(NULL)
		);
		g_crashDumpUploadToken = string2wstring(crypto_RSAEncryptString(
			wstring2string(std::wstring(CRASHRPT_PUBLICKEY)),
			wstring2string(tokeninfo)
		));
		g_CrashDumpUploadAllowed = true;
	}

	g_BreakpadInitialized = true;
}

#endif // _WIN32

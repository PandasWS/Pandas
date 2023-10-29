// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "crashdump.hpp"

#include <string> // std::string, std::wstring
#include <fmt/core.h>

#include "cryptopp.hpp"
#include "assistant.hpp" // boost, makeDirectories, wstring2string
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

// 当程序崩溃时, 将转储文件保存在什么位置
std::string g_dumpSaveDirectory = "dumps";

// 设置检查点记录文件的保存位置
std::string g_crashCheckPointFilepath = g_dumpSaveDirectory + "/crashdump.report";

// 用于上报崩溃转储文件的数据接口地址
std::string g_crashDumpUploadInterface = "https://crashrpt.pandas.ws/upload";

// 保存程序崩溃时负责转储工作的 ExceptionHandler 指针
google_breakpad::ExceptionHandler* g_pExceptionHandler = NULL;

// 用于标记本程序的 Breakpad 是否已经完成了初始化
bool g_breakpadInitialized = false;

// 用于标记本程序是否具备上报崩溃转储文件的条件 (设置了 AppID 和 PublicKey)
bool g_crashDumpUploadAllowed = false;

// 表示本次崩溃是由 @crashtest 刻意引发的, 上报转储文件时携带相关标记
bool g_crashByTestCommand = false;

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
		if (g_crashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " We will trying to send the dump file to the developer,                        " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " the sending process may take a few seconds, please be patient.                " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " The program will shutdown automatically after sent.                           " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		ShowMessage("" CL_BG_RED CL_BT_WHITE " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
		}
	}
	else {
		if (g_crashDumpUploadAllowed) {
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
// Method:      breakpad_status
// Description: 显示当前 Breakpad 的初始化状态
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/08/24 18:55
//************************************
void breakpad_status() {
	if (g_breakpadInitialized)
		ShowInfo("Server crashdump file will be saved to: " CL_WHITE "'%s/'" CL_RESET "\n", g_dumpSaveDirectory.c_str());
	else
		ShowWarning("Google Breakpad initialization failed!\n");
}

//************************************
// Method:      params_dosign
// Description: 用于对即将发送的参数进行签名, 以便服务端验证身份
// Parameter:   std::map<std::string
// Parameter:   std::string> & params
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/08/24 18:54
//************************************
void params_dosign(std::map<std::string, std::string> & params)
{
	std::string sign, token;
	std::string timestamp = fmt::format("{}", time(NULL));

	// 加入一个当前服务器的时间戳
	params.insert(std::make_pair("timestamp", timestamp));

	// 根据当前的参数列表构造一个用于加密的字符串
	std::map<std::string, std::string>::iterator iter;
	for (iter = params.begin(); iter != params.end(); iter++) {
		sign = sign + iter->first + "|";
		sign = sign + iter->second + "|";
	}

	// 使用公钥对 sign 变量的值进行签名, 将密文放到 token 字段发送
	if (std::string(CRASHRPT_PUBLICKEY).length()) {
		token = crypto_RSAEncryptString(std::string(CRASHRPT_PUBLICKEY), sign);
		params.insert(std::make_pair("token", token));
	}
}

#ifdef _WIN32

//************************************
// Method:      breakpad_filter
// Description: 在转储文件生成之前, 会触发此回调函数
// Parameter:   void * context
// Parameter:   EXCEPTION_POINTERS * exinfo
// Parameter:   MDRawAssertionInfo * assertion
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/08/31 20:22
//************************************
bool breakpad_filter(void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion)
{
	// 在此时立刻创建用于保存转储文件的目录, 创建失败转储文件将不会被生成
	return makeDirectories(g_dumpSaveDirectory);
}

//************************************
// Method:      breakpad_map2w
// Description: 将 std::string 的 std::map 转换成 std::wstring 
// Parameter:   std::map<std::string, std::string> strmap
// Parameter:   std::map<std::wstring, std::wstring> & wstrmap
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/11/05 12:56
//************************************
inline void breakpad_map2w(std::map<std::string, std::string> strmap, std::map<std::wstring, std::wstring>& wstrmap) {
	wstrmap.clear();
	std::map<std::string, std::string>::iterator iter;
	for (iter = strmap.begin(); iter != strmap.end(); iter++) {
		wstrmap.insert(std::make_pair(strToWideStr(iter->first), strToWideStr(iter->second)));
	}
}

//************************************
// Method:      breakpad_callback
// Description: 当转储文件已经成功生成后, 会触发此回调函数
// Parameter:   const wchar_t * dump_path
// Parameter:   const wchar_t * minidump_id
// Parameter:   void * context
// Parameter:   EXCEPTION_POINTERS * exinfo
// Parameter:   MDRawAssertionInfo * assertion
// Parameter:   bool succeeded
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/08/31 20:21
//************************************
bool breakpad_callback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context,
	EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded)
{
	// 若回调过来的时候发现崩溃转储文件生成失败了, 那么这里什么都不做.
	// 返回 FALSE 的话, breakpad 内部会尝试调用标准的错误处理方法, 试图抢救一下...
	if (!succeeded) return succeeded;

	// 崩溃转储文件的本地保存路径
	std::string filepath = fmt::format(
		"{}/{}.dmp", wideStrToStr(dump_path), wideStrToStr(minidump_id)
	);

	// 将崩溃转储文件存入一个 std::map 中等待发送请求使用
	std::map<std::string, std::string> sfiles;
	sfiles.insert(std::make_pair("dumpfile", filepath));


	// 收集其他可能需要的信息, 用于服务端分析或归类使用
	std::map<std::string, std::string> sparams;
	sparams.insert(std::make_pair("appid", CRASHRPT_APPID));
	sparams.insert(std::make_pair("platform", "windows"));
	sparams.insert(std::make_pair("version", getPandasVersion(false)));
	sparams.insert(std::make_pair("branch", GIT_BRANCH));
	sparams.insert(std::make_pair("hash", GIT_HASH));
	sparams.insert(std::make_pair("testing", g_crashByTestCommand ? "1" : "0"));
	params_dosign(sparams);

	// 创建 CrashReportSender 对象并利用其提交转储文件给服务器
	google_breakpad::CrashReportSender sender(strToWideStr(g_crashCheckPointFilepath));

	// 由于 Windows 下的 SendCrashReport 仅支持 wstring
	// 所以需要将上面基于 std::string 的 std::map 转换成基于 std::wstring 的 std::map
	std::map<std::wstring, std::wstring> wparams;
	breakpad_map2w(sparams, wparams);

	std::map<std::wstring, std::wstring> wfiles;
	breakpad_map2w(sfiles, wfiles);

	// 开始上报转储文件
	display_crashtips(filepath, false);
	if (g_crashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending the crash dump file, please wait a second...                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	if (sender.SendCrashReport(strToWideStr(g_crashDumpUploadInterface), wparams, wfiles, 0) == google_breakpad::ReportResult::RESULT_SUCCEEDED)
		ShowMessage("" CL_BG_RED CL_BT_GREEN " SUCCESS : The crash dump file has been uploaded successfull.                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	else
		ShowMessage("" CL_BG_RED CL_BT_CYAN  " FAILED: The crash dump file failed to upload, please report to developer.     " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	}
	display_crashtips(filepath, true);

	return succeeded;
}

#else

// 在 Linux 环境下 Breakpad 的使用方法与 Windows 有差异

//************************************
// Method:      breakpad_filter
// Description: 在转储文件生成之前, 会触发此回调函数
// Parameter:   void * context
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/08/31 20:22
//************************************
bool breakpad_filter(void* context)
{
	// 在此时立刻创建用于保存转储文件的目录, 创建失败转储文件将不会被生成
	return makeDirectories(g_dumpSaveDirectory);
}

//************************************
// Method:      breakpad_callback
// Description: 当转储文件已经成功生成后, 会触发此回调函数
// Parameter:   const google_breakpad::MinidumpDescriptor & descriptor
// Parameter:   void * context
// Parameter:   bool succeeded
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/08/31 20:22
//************************************
bool breakpad_callback(const google_breakpad::MinidumpDescriptor& descriptor,
	void* context, bool succeeded)
{
	// 若回调过来的时候发现崩溃转储文件生成失败了, 那么这里什么都不做.
	// 返回 FALSE 的话, breakpad 内部会尝试调用标准的错误处理方法, 试图抢救一下...
	if (!succeeded) return succeeded;

	// 崩溃转储文件的本地保存路径
	std::string filepath = descriptor.path();

	// 将崩溃转储文件存入一个 std::map 中等待发送请求使用
	std::map<std::string, std::string> files;
	files.insert(std::make_pair("dumpfile", filepath));

	// 收集其他可能需要的信息, 用于服务端分析或归类使用
	std::map<std::string, std::string> params;
	params.insert(std::make_pair("appid", CRASHRPT_APPID));
	params.insert(std::make_pair("platform", "linux"));
	params.insert(std::make_pair("version", getPandasVersion(false)));
	params.insert(std::make_pair("branch", GIT_BRANCH));
	params.insert(std::make_pair("hash", GIT_HASH));
	params.insert(std::make_pair("testing", g_crashByTestCommand ? "1" : "0"));
	params_dosign(params);

	std::string response, error;
	bool success = google_breakpad::HTTPUpload::SendRequest(
		g_crashDumpUploadInterface,
		params, files, "", "", "", &response, NULL, &error
	);

	display_crashtips(filepath, false);
	if (g_crashDumpUploadAllowed) {
		ShowMessage("" CL_BG_RED CL_BT_WHITE " Sending the crash dump file, please wait a second...                          " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	if (success)
		ShowMessage("" CL_BG_RED CL_BT_GREEN " SUCCESS : The crash dump file has been uploaded successfull.                  " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	else
		ShowMessage("" CL_BG_RED CL_BT_CYAN  " FAILED: The crash dump file failed to upload, please report to developer.     " CL_BT_WHITE "" CL_CLL "" CL_NORMAL "\n");
	}
	display_crashtips(filepath, true);

	return succeeded;
}

#endif // _WIN32

//************************************
// Method:		breakpad_initialize
// Description:	设置 Google Breakpad 并初始化
// Returns:		void
//************************************
void breakpad_initialize() {
#ifdef _WIN32
	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		strToWideStr(g_dumpSaveDirectory), breakpad_filter, breakpad_callback, nullptr,
		google_breakpad::ExceptionHandler::HANDLER_ALL,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs), L"", 0
	);
#else
	// 在 Linux 环境下需要先初始化一个 MinidumpDescriptor 对象用来描述转储文件的保存位置
	google_breakpad::MinidumpDescriptor descriptor(g_dumpSaveDirectory.c_str());

	// 此处不能放弃对 g_pExceptionHandler 的赋值,
	// 否则整个 google_breakpad::ExceptionHandler 对象会在本函数结束的时候被释放掉
	g_pExceptionHandler = new google_breakpad::ExceptionHandler(
		descriptor, breakpad_filter, breakpad_callback, nullptr, true, -1
	);
#endif // _WIN32

	// 若初始化 g_pExceptionHandler 失败, 那么标记整个 Breakpad 机制初始化失败
	g_breakpadInitialized = (g_pExceptionHandler != nullptr);

	// 必须配置了 APPID 和 PUBLICKEY 才能够上报转储文件到分析服务器
	g_crashDumpUploadAllowed = (
		std::string(CRASHRPT_APPID).length() && std::string(CRASHRPT_PUBLICKEY).length()
	);
}

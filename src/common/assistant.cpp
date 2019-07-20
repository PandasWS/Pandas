// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "assistant.hpp"

#include <stdlib.h>
#include <locale.h> // setlocale
#include <sys/stat.h> // _stat
#include <errno.h> // errno, ENOENT, EEXIST
#include <wchar.h> // vswprintf

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32

#include "strlib.hpp"
#include "db.hpp"
#include "showmsg.hpp"


//************************************
// Method:		isDirectoryExistss
// Description:	判断目录是否存在 (跨平台支持)
// Parameter:	const std::string & path
// Returns:		bool
//************************************
bool isDirectoryExists(const std::string& path) {
#ifdef _WIN32
	struct _stat info;
	if (_stat(path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & _S_IFDIR) != 0;
#else 
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
#endif // _WIN32
}

//************************************
// Method:		makeDirectories
// Description:	创建多层目录 (跨平台支持)
// Parameter:	const std::string & path
// Returns:		bool
//************************************
bool makeDirectories(const std::string& path) {
#ifdef _WIN32
	int ret = _mkdir(path.c_str());
#else
	mode_t mode = 0755;
	int ret = mkdir(path.c_str(), mode);
#endif // _WIN32

	if (ret == 0)
		return true;

	switch (errno)
	{
	case ENOENT:
		// parent didn't exist, try to create it
	{
		size_t pos = path.find_last_of('/');
		if (pos == std::string::npos)
#ifdef _WIN32
			pos = path.find_last_of('\\');
		if (pos == std::string::npos)
#endif // _WIN32
			return false;
		if (!makeDirectories(path.substr(0, pos)))
			return false;
	}
	// now, try to create again
#ifdef _WIN32
	return 0 == _mkdir(path.c_str());
#else 
	return 0 == mkdir(path.c_str(), mode);
#endif // _WIN32

	case EEXIST:
		// done!
		return isDirectoryExists(path);

	default:
		return false;
	}
}

//************************************
// Method:		stdStringReplaceAll
// Description:	用于对 std::string 进行全部替换操作
// Parameter:	std::string & str
// Parameter:	const std::string & from
// Parameter:	const std::string & to
// Returns:		void
//************************************
void stdStringReplaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//************************************
// Method:		stdStringReplaceAll
// Description:	用于对 std::wstring 进行全部替换操作
// Parameter:	std::wstring & str
// Parameter:	const std::wstring & from
// Parameter:	const std::wstring & to
// Returns:		void
//************************************
void stdStringReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//************************************
// Method:		ensurePathSep
// Description:	将给带的路径中存在的 / 或 \ 转换成当前系统匹配的路径分隔符
// Parameter:	std::string path
// Returns:		std::string
//************************************
std::string ensurePathSep(std::string& path) {
#ifdef _WIN32
	char pathsep[] = "\\";
#else
	char pathsep[] = "/";
#endif // _WIN32
	stdStringReplaceAll(path, "/", pathsep);
	stdStringReplaceAll(path, "\\", pathsep);
	return path;
}

//************************************
// Method:		ensurePathSep
// Description:	将给带的路径中存在的 / 或 \ 转换成当前系统匹配的路径分隔符
// Parameter:	std::wstring path
// Returns:		std::wstring
//************************************
std::wstring ensurePathSep(std::wstring& path) {
#ifdef _WIN32
	wchar_t pathsep[] = L"\\";
#else
	wchar_t pathsep[] = L"/";
#endif // _WIN32
	stdStringReplaceAll(path, L"/", pathsep);
	stdStringReplaceAll(path, L"\\", pathsep);
	return path;
}

//************************************
// Method:		string2wstring
// Description:	将 std::string 转换成 std::wstring
//              https://blog.csdn.net/CYYTU/article/details/78616132
// Parameter:	const std::string & s
// Returns:		std::wstring
//************************************
std::wstring string2wstring(const std::string& s) {
#ifdef _WIN32
	setlocale(LC_ALL, "chs");
#else  
	setlocale(LC_ALL, "zh_CN.gbk");
#endif // _WIN32
	const char* _Source = s.c_str();
	size_t _Dsize = s.size() + 1;
	wchar_t *_Dest = new wchar_t[_Dsize];
	wmemset(_Dest, 0, _Dsize);
	mbstowcs(_Dest, _Source, _Dsize);
	std::wstring result = _Dest;
	delete[]_Dest;
	setlocale(LC_ALL, "C");
	return result;
}

//************************************
// Method:		wstring2string
// Description:	将 std::wstring 转换成 std::string
//              https://blog.csdn.net/CYYTU/article/details/78616132
// Parameter:	const std::wstring & ws
// Returns:		std::string
//************************************
std::string wstring2string(const std::wstring& ws) {
	std::string curLocale = setlocale(LC_ALL, NULL);
#ifdef _WIN32
	setlocale(LC_ALL, "chs");
#else  
	setlocale(LC_ALL, "zh_CN.gbk");
#endif // _WIN32
	const wchar_t* _Source = ws.c_str();
	size_t _Dsize = 2 * ws.size() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);
	std::string result = _Dest;
	delete[]_Dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::string 的格式化
// Parameter:	std::string & _str
// Parameter:	const char * _Format
// Parameter:	...
// Returns:		std::string &
//************************************
std::string & strFormat(std::string & _str, const char * _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vsnprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	char* buf = (char*)aMalloc(count * sizeof(char));
	vsnprintf(buf, count, _Format, marker);
	_str = std::string(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::string 的格式化
// Parameter:	const char * _Format
// Parameter:	...
// Returns:		std::string
//************************************
std::string strFormat(const char* _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vsnprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	char* buf = (char*)aMalloc(count * sizeof(char));
	vsnprintf(buf, count, _Format, marker);
	std::string _str = std::string(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::wstring 的格式化
// Parameter:	std::wstring & _str
// Parameter:	const wchar_t * _Format
// Parameter:	...
// Returns:		std::wstring &
//************************************
std::wstring & strFormat(std::wstring & _str, const wchar_t * _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vswprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	wchar_t* buf = (wchar_t*)aMalloc(count * sizeof(wchar_t));
	vswprintf(buf, count, _Format, marker);
	_str = std::wstring(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::wstring 的格式化
// Parameter:	const wchar_t * _Format
// Parameter:	...
// Returns:		std::wstring
//************************************
std::wstring strFormat(const wchar_t* _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vswprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	wchar_t* buf = (wchar_t*)aMalloc(count * sizeof(wchar_t));
	vswprintf(buf, count, _Format, marker);
	std::wstring _str = std::wstring(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		getPandasVersion
// Description:	用于获取 Pandas 的主程序版本号
// Returns:		std::string
//************************************
std::string getPandasVersion() {
#ifdef _WIN32
	std::string pandasVersion = strFormat(
		"v%s", getFileVersion("", true).c_str()
	);
	return pandasVersion;
#else
	return std::string(Pandas_Version);
#endif // _WIN32
}

#ifndef MINICORE
//************************************
// Method:		detectCodepage
// Description:	为指定的 sql_handle 设定合适的编码
// Parameter:	Sql * sql_handle
// Parameter:	const char * connect_name
// Parameter:	const char * codepage
// Returns:		void
//************************************
void detectCodepage(Sql* sql_handle, const char* connect_name, const char* codepage) {
	char* buf = NULL;
	char finally_codepage[32] = { 0 };
	bool bShowInfomation = (connect_name != NULL);

	if (!sql_handle || !codepage || strlen(codepage) <= 0) return;

	if (strcmpi(codepage, "auto") != 0) {
		if (SQL_ERROR == Sql_SetEncoding(sql_handle, codepage))
			Sql_ShowDebug(sql_handle);
		return;
	}

	if (SQL_ERROR == Sql_Query(sql_handle, "SHOW VARIABLES LIKE 'character_set_server';")) {
		Sql_ShowDebug(sql_handle);
		return;
	}

	if (SQL_ERROR == Sql_NextRow(sql_handle) ||
		SQL_ERROR == Sql_GetData(sql_handle, 1, &buf, NULL)) {
		Sql_ShowDebug(sql_handle);
		return;
	}

	Sql_FreeResult(sql_handle);
	if (buf == NULL) return;

	if (bShowInfomation)
		ShowInfo("Detected the " CL_WHITE "%s" CL_RESET " database server character set is " CL_WHITE "%s" CL_RESET ".\n", connect_name, buf);

	// 以下的判断策略, 有待进一步研究和测试
	// 目的是能够在 Linux 和 Windows 上都能尽可能合理的处理 GBK 和 BIG5 编码的服务器
	if (stricmp(buf, "utf8") == 0 || stricmp(buf, "utf8mb4") == 0) {
#ifdef _WIN32
		LCID lcid = GetSystemDefaultLCID();
		if (lcid == 0x0804) {
			// 若是简体中文系统，那么内码表设置为GBK
			// http://blog.csdn.net/shen_001/article/details/6364326
			safestrncpy(finally_codepage, "gbk", sizeof(finally_codepage));
		}
		else if (lcid == 0x0404) {
			// 若是繁体中文系统，那么内码表设置为BIG5
			safestrncpy(finally_codepage, "big5", sizeof(finally_codepage));
		}
#else
		char* lang = getenv("LANG");
		if (lang != NULL) {
			size_t i = 0;
			const char* localtable[] = { "zh_cn.utf-8", "zh_cn.gb2312", "zh_cn.gbk" };

			ARR_FIND(0, ARRAYLENGTH(localtable), i, stricmp(lang, localtable[i]) == 0);
			if (i != ARRAYLENGTH(localtable)) {
				safestrncpy(finally_codepage, "gbk", sizeof(finally_codepage));
			}
		}
#endif // _WIN32
	}
	else {
		size_t i = 0;
		const char* passthrough[] = { "latin1", "gbk", "big5" };

		ARR_FIND(0, ARRAYLENGTH(passthrough), i, stricmp(buf, passthrough[i]) == 0);
		if (i != ARRAYLENGTH(passthrough)) {
			safestrncpy(finally_codepage, buf, sizeof(finally_codepage));
		}
	}

	// 从这里开始, 不再需要使用 buf 的值, 进行一些清理操作

	if (bShowInfomation) {
		if (strlen(finally_codepage) > 0) {
			ShowInfo("Will connect to " CL_WHITE "%s" CL_RESET " database using " CL_WHITE "%s" CL_RESET ".\n", connect_name, finally_codepage);
		}
		else {
			ShowInfo("Will connect to " CL_WHITE "%s" CL_RESET " database using " CL_WHITE "%s" CL_RESET ".\n", connect_name, "default");
		}
	}

	if (strlen(finally_codepage) > 0) {
		if (SQL_ERROR == Sql_SetEncoding(sql_handle, finally_codepage))
			Sql_ShowDebug(sql_handle);
	}
}
#endif // MINICORE

#ifdef _WIN32
//************************************
// Method:		getFileVersion
// Description:	获取指定文件的文件版本号
// Parameter:	std::string filename
// Parameter:	bool bWithoutBuildNum
// Returns:		std::string
//************************************
std::string getFileVersion(std::string filename, bool bWithoutBuildNum) {
	char szModulePath[MAX_PATH] = { 0 };

	if (filename.empty()) {
		if (GetModuleFileName(NULL, szModulePath, MAX_PATH) == 0) {
			ShowWarning("getFileVersion: Could not get module file name, defaulting to '%s'\n", Pandas_Version);
			return std::string(Pandas_Version);
		}
		filename = std::string(szModulePath);
	}

	DWORD dwInfoSize = 0, dwHandle = 0;

	dwInfoSize = GetFileVersionInfoSize(filename.c_str(), &dwHandle);
	if (dwInfoSize == 0) {
		ShowWarning("getFileVersion: Could not get version info size, defaulting to '%s'\n", Pandas_Version);
		return std::string(Pandas_Version);
	}

	void* pVersionInfo = new char[dwInfoSize];
	if (GetFileVersionInfo(filename.c_str(), dwHandle, dwInfoSize, pVersionInfo)) {
		void* lpBuffer = NULL;
		UINT nItemLength = 0;
		if (VerQueryValue(pVersionInfo, "\\", &lpBuffer, &nItemLength)) {
			VS_FIXEDFILEINFO *pFileInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			std::string sFileVersion;
			strFormat(sFileVersion, "%d.%d.%d",
				pFileInfo->dwFileVersionMS >> 16,
				pFileInfo->dwProductVersionMS & 0xFFFF,
				pFileInfo->dwProductVersionLS >> 16
			);

			if (!bWithoutBuildNum) {
				strFormat(sFileVersion, "%s.%d",
					sFileVersion.c_str(), pFileInfo->dwProductVersionLS & 0xFFFF
				);
			}
			delete[] pVersionInfo;
			return sFileVersion;
		}
	}

	delete[] pVersionInfo;
	ShowWarning("getFileVersion: Could not get file version, defaulting to '%s'\n", Pandas_Version);
	return std::string(Pandas_Version);
}
#endif // _WIN32

//************************************
// Method:		safety_localtime
// Description:	能够兼容 Windows 和 Linux 的线程安全 localtime 函数
// Parameter:	const time_t * time
// Parameter:	struct tm * result
// Returns:		struct tm *
//************************************
struct tm *safety_localtime(const time_t *time, struct tm *result) {
#ifdef _WIN32
	return (localtime_s(result, time) == S_OK ? result : nullptr);
#else
	return localtime_r(time, result);
#endif // _WIN32
}

//************************************
// Method:		safety_gmtime
// Description:	能够兼容 Windows 和 Linux 的线程安全 gmtime 函数
// Parameter:	const time_t * time
// Parameter:	struct tm * result
// Returns:		struct tm *
//************************************
struct tm *safety_gmtime(const time_t *time, struct tm *result) {
#ifdef _WIN32
	return (gmtime_s(result, time) == S_OK ? result : nullptr);
#else
	return gmtime_r(time, result);
#endif // _WIN32
}

//************************************
// Method:		safty_localtime_define
// Description:	用于覆盖 localtime 的的替换函数, 用于修正 LGTM 警告
// Parameter:	const time_t * time
// Returns:		std::shared_ptr<struct tm>
//************************************
std::shared_ptr<struct tm> safty_localtime_define(const time_t *time) {
	struct tm *_ttm_result = new struct tm();
	return std::shared_ptr<struct tm>(safety_localtime(time, _ttm_result));
}

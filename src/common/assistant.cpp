// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "assistant.hpp"

#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#include "strlib.hpp"
#include "db.hpp"
#include "showmsg.hpp"

//************************************
// Method:		std_string_format
// Description:	用于进行 std::string 的格式化
// Parameter:	std::string & _str
// Parameter:	const char * _Format
// Parameter:	...
// Returns:		std::string &
//************************************
std::string & std_string_format(std::string & _str, const char * _Format, ...) {
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
// Method:		safety_localtime
// Description:	能够兼容 Windows 和 Linux 的线程安全 localtime 函数
// Parameter:	const time_t * time
// Parameter:	struct tm * result
// Returns:		struct tm *
//************************************
struct tm *safety_localtime(const time_t *time, struct tm *result) {
#ifdef WIN32
	return (localtime_s(result, time) == S_OK ? result : nullptr);
#else
	return localtime_r(time, result);
#endif // WIN32
}

//************************************
// Method:		safety_gmtime
// Description:	能够兼容 Windows 和 Linux 的线程安全 gmtime 函数
// Parameter:	const time_t * time
// Parameter:	struct tm * result
// Returns:		struct tm *
//************************************
struct tm *safety_gmtime(const time_t *time, struct tm *result) {
#ifdef WIN32
	return (gmtime_s(result, time) == S_OK ? result : nullptr);
#else
	return gmtime_r(time, result);
#endif // WIN32
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

#ifndef MINICORE
//************************************
// Method:		smart_codepage
// Description:	为指定的 sql_handle 设定合适的编码
// Parameter:	Sql * sql_handle
// Parameter:	const char * connect_name
// Parameter:	const char * codepage
// Returns:		void
//************************************
void smart_codepage(Sql* sql_handle, const char* connect_name, const char* codepage) {
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

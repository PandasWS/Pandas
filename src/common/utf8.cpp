﻿// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma warning(disable : 26812)

#include "utf8.hpp"

#include "../common/strlib.hpp"
#include "../common/performance.hpp"
#include "../common/assistant.hpp"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <errno.h>
	#include <string.h>

	#include <stdio.h>
	#include <locale.h>
	#include <langinfo.h>

	#include "../common/iconv.hpp"
#endif // _WIN32

#include <unordered_map>

namespace PandasUtf8 {

// 当无法通过 PandasUtf8::systemLanguage 获得契合的字符编码时
// 将会使用这里定义的默认编码 (主要是在 Linux 平台上使用)
#define DEFAULT_ENCODING "GBK"

// 设定两个全局变量用于保存系统的语言和控制台的编码
// 这两个东西通常程序启动后就不会再发生任何变化, 为了避免频繁检测, 缓存起来比较值得
enum e_console_encoding consoleEncoding = getConsoleEncoding();
enum e_system_language systemLanguage = getSystemLanguage();

// 用于保存 FILE 指针和文件编码模式的缓存
std::unordered_map<FILE*, e_file_charsetmode> __fpmodecache;

// 此处定义的缓冲区大小可参考 showmsg.cpp 中 SBUF_SIZE 的定义
// 按照 rAthena 的建议, 此处的 STRBUF_SIZE 不会设置低于 SBUF_SIZE 设定的值
#define STRBUF_SIZE 2054

//************************************
// Method:      setModeMapping
// Description: 保存 FILE 指针和文件编码模式的关联到缓存
// Parameter:   FILE * _fp
// Parameter:   e_file_charsetmode _mode
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/16 01:22
//************************************
void setModeMapping(FILE* _fp, e_file_charsetmode _mode) {
	auto it = __fpmodecache.find(_fp);
	if (it != __fpmodecache.end()) {
		it->second = _mode;
		return;
	}
	__fpmodecache[_fp] = _mode;
}

//************************************
// Method:      getModeMapping
// Description: 根据 FILE 指针获取缓存中的文件编码模式
// Parameter:   FILE * _fp
// Returns:     e_file_charsetmode
// Author:      Sola丶小克(CairoLee)  2020/02/16 01:23
//************************************
e_file_charsetmode getModeMapping(FILE* _fp) {
	auto it = __fpmodecache.find(_fp);
	if (it != __fpmodecache.end()) {
		return it->second;
	}
	return FILE_CHARSETMODE_UNKNOW;
}

//************************************
// Method:      clearModeMapping
// Description: 清空指定 FILE 指针在缓存中的数据
// Parameter:   FILE * _fp
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/16 01:23
//************************************
void clearModeMapping(FILE* _fp) {
	__fpmodecache.erase(_fp);
}

//************************************
// Method:      getConsoleEncoding
// Description: 获取当前操作系统终端控制台的默认编码
// Returns:     enum e_console_encoding
// Author:      Sola丶小克(CairoLee)  2020/01/24 15:51
//************************************
enum e_console_encoding getConsoleEncoding() {
#ifdef _WIN32
	// 关于 GetACP 的编码对应表可以在以下文档中查询:
	// https://docs.microsoft.com/zh-cn/windows/win32/intl/code-page-identifiers
	UINT nCodepage = GetACP();

	switch (nCodepage) {
	case 936:	// GBK
		return CONSOLE_ENCODING_GBK;
	case 950:	// BIG5
		return CONSOLE_ENCODING_BIG5;
	case 1252:	// LATIN1
		return CONSOLE_ENCODING_LATIN1;
	case 65001:	// UTF-8
		return CONSOLE_ENCODING_UTF8;
	default:
		ShowWarning("%s: Unsupport default ANSI codepage: %d, defaulting to latin1\n", __func__, nCodepage);
		return CONSOLE_ENCODING_LATIN1;
	}
#else
	setlocale(LC_ALL, "");
	char* szLanginfo = nl_langinfo(CODESET);

	if (boost::icontains(szLanginfo, "UTF-8"))
		return CONSOLE_ENCODING_UTF8;
	else if (boost::icontains(szLanginfo, "GBK"))
		return CONSOLE_ENCODING_GBK;
	else if (boost::icontains(szLanginfo, "GB18030"))
		return CONSOLE_ENCODING_GBK;
	else if (boost::icontains(szLanginfo, "Big5HKSCS"))
		return CONSOLE_ENCODING_BIG5;
	else if (boost::icontains(szLanginfo, "Big5"))
		return CONSOLE_ENCODING_BIG5;
	else if (boost::icontains(szLanginfo, "ANSI_X3.4-1968"))
		return CONSOLE_ENCODING_LATIN1;
	else {
		printf("%s: Unsupport codeset: %s, defaulting to latin1\n", __func__, szLanginfo);
	}

	return CONSOLE_ENCODING_LATIN1;
#endif // _WIN32
}

//************************************
// Method:      getSystemLanguage
// Description: 获取当前操作系统的默认展现语言
// Returns:     enum e_system_language
// Author:      Sola丶小克(CairoLee)  2020/01/24 21:59
//************************************
enum e_system_language getSystemLanguage() {
#ifdef _WIN32
	// GetUserDefaultUILanguage 获取到的编码对照表:
	// https://www.voidtools.com/support/everything/language_ids/
	switch (GetUserDefaultUILanguage()) {
	case 0x0409:	// English (United States)
		return SYSTEM_LANGUAGE_ENG;
	case 0x0804:	// Chinese (PRC) 
		return SYSTEM_LANGUAGE_CHS;
	case 0x0404:	// Chinese (Taiwan Region)
	case 0x0c04:	// Chinese (Hong Kong SAR, PRC) 
		return SYSTEM_LANGUAGE_CHT;
	default:
		return SYSTEM_LANGUAGE_ENG;
	}
#else
	setlocale(LC_ALL, "");
	char* szLocale = setlocale(LC_CTYPE, NULL);

	if (boost::istarts_with(szLocale, "zh_CN"))
		return SYSTEM_LANGUAGE_CHS;
	else if (boost::istarts_with(szLocale, "zh_TW"))
		return SYSTEM_LANGUAGE_CHT;
	else if (boost::istarts_with(szLocale, "en_US"))
		return SYSTEM_LANGUAGE_ENG;
	else if (boost::iequals(szLocale, "C"))
		return SYSTEM_LANGUAGE_ENG;
	else {
		printf("%s: Unsupport locale: %s, defaulting to english\n", __func__, szLocale);
	}

	return SYSTEM_LANGUAGE_ENG;
#endif // _WIN32
}

//************************************
// Method:      getDefaultCodepage
// Description: 获取无法根据系统语言获取到对应的编码时采用的默认编码
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/02/08 15:53
//************************************
std::string getDefaultCodepage() {
	return std::string(DEFAULT_ENCODING);
}

#ifdef _WIN32

//************************************
// Method:      UnicodeEncode
// Description: 
// Parameter:   const std::string & strANSI
// Parameter:   unsigned int nCodepage
// Returns:     std::wstring
// Author:      Sola丶小克(CairoLee)  2020/01/31 14:00
//************************************
std::wstring UnicodeEncode(const std::string& strANSI, unsigned int nCodepage) {
	int unicodeLen = MultiByteToWideChar(nCodepage, 0, strANSI.c_str(), -1, NULL, 0);
	wchar_t* strUnicode = new wchar_t[unicodeLen];
	wmemset(strUnicode, 0, unicodeLen);
	MultiByteToWideChar(nCodepage, 0, strANSI.c_str(), -1, strUnicode, unicodeLen);
	return std::wstring(strUnicode);
}

//************************************
// Method:      UnicodeDecode
// Description: 
// Parameter:   const std::wstring & strUnicode
// Parameter:   unsigned int nCodepage
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/31 14:00
//************************************
std::string UnicodeDecode(const std::wstring& strUnicode, unsigned int nCodepage) {
	int ansiLen = WideCharToMultiByte(nCodepage, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);
	char* strAnsi = new char[ansiLen];
	memset(strAnsi, 0, ansiLen);
	WideCharToMultiByte(nCodepage, 0, strUnicode.c_str(), -1, strAnsi, ansiLen, NULL, NULL);
	return std::string(strAnsi);
}

#else

//************************************
// Method:      iconvConvert
// Description: 在 Linux 平台上使用 iconv 库进行字符编码转换
// Parameter:   const std::string & val
// Parameter:   const std::string & in_enc
// Parameter:   const std::string out_enc
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/02/02 23:42
//************************************
std::string iconvConvert(const std::string& val, const std::string& in_enc, const std::string& out_enc) {
	if (in_enc == out_enc) return val;
	iconvpp::converter conv(out_enc, in_enc, true);
	std::string result;
	conv.convert(val, result);
	return result;
}

//************************************
// Method:      consoleConvert
// Description: 在 Linux 环境下对输出到控制台的文本进行编码转换
// Parameter:   const std::string & mes
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/02/05 16:42
//************************************
std::string consoleConvert(const std::string& mes) {
#ifdef BUILDBOT
	// 若当前程序编译运行在持续集成环境
	// 那么不进行任何终端编码的转换操作, 让它持续处于英文状态
	return mes;
#endif // BUILDBOT

	// 在 Linux 环境下我们目前只接受终端编码为 UTF8 的情况
	// 如果当前的终端编码不为 UTF8 则停止进行任何转换的具体工作, 维持英文状态
	if (PandasUtf8::consoleEncoding != CONSOLE_ENCODING_UTF8) {
		return mes;
	}

	std::string _from;

	switch (PandasUtf8::systemLanguage) {
		case SYSTEM_LANGUAGE_CHT: _from = "BIG5"; break;
		case SYSTEM_LANGUAGE_CHS: _from = "GBK"; break;
		default: return mes; break;
	}

	return PandasUtf8::iconvConvert(mes, _from, "UTF-8");
}

//************************************
// Method:      vfprintf
// Description: 用于对 vfprintf 函数进行劫持和编码转换处理
// Parameter:   FILE * file
// Parameter:   const char * fmt
// Parameter:   va_list args
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2020/02/05 16:13
//************************************
int vfprintf(FILE* file, const char* fmt, va_list args) {
	va_list apcopy;
	va_copy(apcopy, args);

	char sbuf[STRBUF_SIZE] = { 0 };
	int len = vsnprintf(sbuf, STRBUF_SIZE, fmt, apcopy);
	std::string strBuf;

	if (len >= 0 && len < STRBUF_SIZE) {
		strBuf = std::string(sbuf);
	}
	else {
		StringBuf* sbuf = StringBuf_Malloc();
		StringBuf_Vprintf(sbuf, fmt, args);
		strBuf = std::string(StringBuf_Value(sbuf));
		StringBuf_Free(sbuf);
		printf("%s: dynamic buffer used, increase the static buffer size to %d or more.\n", __func__, len + 1);
	}

	va_end(apcopy);

	// 进行字符串编码的转码加工处理
	strBuf = PandasUtf8::consoleConvert(strBuf);

	// 将处理完的字符串输出到指定的地方去 (显示到终端)
	return fprintf(file, "%s", strBuf.c_str());
}

#endif // _WIN32

//************************************
// Method:      utf8ToAnsi
// Description: 将 UTF8 字符串转换为 ANSI 字符串
// Parameter:   const std::string & strUtf8 须为 UTF-8 编码的字符串
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/24 00:26
//************************************
std::string utf8ToAnsi(const std::string& strUtf8) {
#ifdef _WIN32
	std::wstring strUnicode = PandasUtf8::UnicodeEncode(strUtf8, CP_UTF8);
	return PandasUtf8::UnicodeDecode(strUnicode, CP_ACP);
#else
	std::string toCharset;
	switch (PandasUtf8::systemLanguage) {
	case SYSTEM_LANGUAGE_CHS: toCharset = "GBK"; break;
	case SYSTEM_LANGUAGE_CHT: toCharset = "BIG5"; break;
	default: toCharset = PandasUtf8::getDefaultCodepage(); break;
	}
	return PandasUtf8::iconvConvert(strUtf8, "UTF-8", toCharset);
#endif // _WIN32
}

//************************************
// Method:      ansiToUtf8
// Description: 将 ANSI 字符串转换为 UTF8 字符串
// Parameter:   const std::string & strAnsi 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/24 00:26
//************************************
std::string ansiToUtf8(const std::string& strAnsi) {
#ifdef _WIN32
	std::wstring strUnicode = PandasUtf8::UnicodeEncode(strAnsi, CP_ACP);
	return PandasUtf8::UnicodeDecode(strUnicode, CP_UTF8);
#else
	std::string fromCharset;
	switch (PandasUtf8::systemLanguage) {
	case SYSTEM_LANGUAGE_CHS: fromCharset = "GBK"; break;
	case SYSTEM_LANGUAGE_CHT: fromCharset = "BIG5"; break;
	default: fromCharset = PandasUtf8::getDefaultCodepage(); break;
	}
	return PandasUtf8::iconvConvert(strAnsi, fromCharset, "UTF-8");
#endif // _WIN32
}

//************************************
// Method:      fmode
// Description: 尝试获取某个文件指针的文本编码
// Parameter:   _In_ FILE * _Stream
// Returns:     enum e_file_charsetmode
// Author:      Sola丶小克(CairoLee)  2020/01/21 09:37
//************************************
enum e_file_charsetmode fmode(FILE* _Stream) {
	// 优先从缓存中读取
	e_file_charsetmode cached_charsetmode = PandasUtf8::getModeMapping(_Stream);
	if (cached_charsetmode != FILE_CHARSETMODE_UNKNOW) {
		return cached_charsetmode;
	}

	size_t extracted = 0;
	unsigned char buf[3] = { 0 };
	enum e_file_charsetmode charset_mode = FILE_CHARSETMODE_UNKNOW;

	// 若传递的 FILE 指针无效则直接返回编码不可知
	if (_Stream == nullptr) {
		return charset_mode;
	}

	// 记录目前指针所在的位置
	long curpos = ftell(_Stream);

	// 指针移动到开头, 读取前 3 个字节
	fseek(_Stream, 0, SEEK_SET);
	extracted = ::fread(buf, sizeof(unsigned char), 3, _Stream);

	// 根据读取到的前几个字节来判断文本的编码类型
	if (extracted == 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
		// UTF8-BOM
		charset_mode = FILE_CHARSETMODE_UTF8_BOM;
	}
	else if (extracted >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
		// UCS-2 LE
		charset_mode = FILE_CHARSETMODE_UCS2_LE;
	}
	else if (extracted >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) {
		// UCS-2 BE
		charset_mode = FILE_CHARSETMODE_UCS2_BE;
	}
	else {
		// 若无法根据上面的前几个字节判断出编码, 那么默认为 ANSI 编码 (GBK\BIG5)
		charset_mode = FILE_CHARSETMODE_ANSI;
	}

	// 将指针设置回原来的位置, 避免影响后续的读写流程
	fseek(_Stream, curpos, SEEK_SET);

	// 将当前分析到的 charset_mode 保存到缓存
	PandasUtf8::setModeMapping(_Stream, charset_mode);

	return charset_mode;
}

//************************************
// Method:      fmode
// Description: 尝试获取某个文件指针的文本编码
// Parameter:   std::ifstream & ifs
// Returns:     enum e_file_charsetmode
// Author:      Sola丶小克(CairoLee)  2020/01/27 21:38
//************************************
enum e_file_charsetmode fmode(std::ifstream& ifs) {
	unsigned char buf[3] = { 0 };
	enum e_file_charsetmode charset_mode = FILE_CHARSETMODE_UNKNOW;

	// 记录目前指针所在的位置
	long curpos = (long)ifs.tellg();

	// 指针移动到开头, 读取前 3 个字节
	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)buf, 3);
	long extracted = (long)ifs.gcount();

	// 根据读取到的前几个字节来判断文本的编码类型
	if (extracted == 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
		// UTF8-BOM
		charset_mode = FILE_CHARSETMODE_UTF8_BOM;
	}
	else if (extracted >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
		// UCS-2 LE
		charset_mode = FILE_CHARSETMODE_UCS2_LE;
	}
	else if (extracted >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) {
		// UCS-2 BE
		charset_mode = FILE_CHARSETMODE_UCS2_BE;
	}
	else {
		// 若无法根据上面的前几个字节判断出编码, 那么默认为 ANSI 编码 (GBK\BIG5)
		charset_mode = FILE_CHARSETMODE_ANSI;
	}

	// 将指针设置回原来的位置, 避免影响后续的读写流程
	ifs.seekg(curpos, std::ios::beg);
	return charset_mode;
}

//************************************
// Method:      fopen
// Description: 能够在原有的打开模式中追加二进制模式的 fopen 函数
// Parameter:   const char * _FileName
// Parameter:   const char * _Mode
// Returns:     FILE*
// Author:      Sola丶小克(CairoLee)  2020/01/24 01:27
//************************************
FILE* fopen(const char* _FileName, const char* _Mode) {
	// 若当前打开文件的模式已经是二进制, 那么直接调用 fopen 并返回
	if (strchr(_Mode, 'b')) {
		return ::fopen(_FileName, _Mode);
	}

	// 若文件的打开模式不以二进制模式打开, 那么补充对应的 _Mode 标记
	std::string sMode(_Mode);
	sMode += "b";
	return ::fopen(_FileName, sMode.c_str());
}

//************************************
// Method:      fclose
// Description: 进行了一些清理工作的 fclose 方法
// Parameter:   FILE * _fp
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2020/02/16 01:05
//************************************
int fclose(FILE* _fp) {
	PandasUtf8::clearModeMapping(_fp);
	return ::fclose(_fp);
}

char* fgets(char* _Buffer, int _MaxCount, FILE* _Stream) {
	// 若不是 UTF8-BOM, 那么直接透传 fgets 调用
	if (PandasUtf8::fmode(_Stream) != FILE_CHARSETMODE_UTF8_BOM) {
		return ::fgets(_Buffer, _MaxCount, _Stream);
	}

	// 若指针在文件的前 3 个字节, 那么将指针移动到前 3 个字节的后面,
	// 避免后续进行 fgets 的时候读取到前 3 个字节, 同时将当前位置记录到 curpos
	long curpos = ftell(_Stream);
	if (curpos <= 3) fseek(_Stream, 3, SEEK_SET);

	// 读取 _MaxCount 长度的内容并保存到 buffer 中
	char* buffer = new char[_MaxCount];
	char* fgets_result = ::fgets(buffer, _MaxCount, _Stream);

	if (!fgets_result) {
		delete[] buffer;
		return fgets_result;
	}
	
	std::string line(buffer);
	delete[] buffer;

	// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
	std::string ansi = PandasUtf8::utf8ToAnsi(line);
	memset(_Buffer, 0, _MaxCount);

	// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
	// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存
	// 不过就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
	if (ansi.size() > (size_t)_MaxCount) {
		ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", __func__, sizeof(_Buffer), ansi.size());
		fseek(_Stream, curpos, SEEK_SET);
		return ::fgets(_Buffer, _MaxCount, _Stream);
	}

	// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
	memcpy(_Buffer, ansi.c_str(), ansi.size());
	return _Buffer;
}

size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
	// 若不是 UTF8-BOM 或者 _ElementSize 不等于 1, 那么直接透传 fread 调用
	if (PandasUtf8::fmode(_Stream) != FILE_CHARSETMODE_UTF8_BOM || _ElementSize != 1) {
		return ::fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}

	size_t extracted = 0;
	long curpos = ftell(_Stream);
	size_t elementlen = (_ElementSize * _ElementCount) + 1;
	char* buffer = new char[elementlen];

	// 若指针在文件的前 3 个字节, 那么将指针移动到前 3 个字节后面,
	// 避免后续进行 fread 的时候读取到前 3 个字节
	if (curpos <= 3) {
		fseek(_Stream, 3, SEEK_SET);

		// 需要重新分配缓冲区大小, 以及调整 _ElementCount 的大小
		delete[] buffer;
		if (_ElementCount >= 3) _ElementCount -= 3;
		if (elementlen >= 3) elementlen -= 3;
		buffer = new char[elementlen];
	}

	// 将缓冲区的内容全部重置为 0x00
	memset(buffer, 0, elementlen);

	// 读取特定长度的内容并保存到 buffer 中
	extracted = ::fread(buffer, _ElementSize, _ElementCount, _Stream);

	if (!extracted) {
		delete[] buffer;
		return extracted;
	}

	// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
	std::string ansi = PandasUtf8::utf8ToAnsi(std::string(buffer));
	memset(_Buffer, 0, _ElementSize * _ElementCount);
	delete[] buffer;

	// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
	// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存
	// 不过就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
	if (ansi.size() > elementlen) {
		ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", __func__, sizeof(_Buffer), ansi.size());
		fseek(_Stream, curpos, SEEK_SET);
		// 之前修正过 _ElementCount 的大小, 现在这里需要改回去
		if (curpos <= 3) _ElementCount += 3;
		return ::fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}

	// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
	memcpy(_Buffer, ansi.c_str(), ansi.size());
	return extracted;
}

} // namespace PandasUtf8
// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma warning(disable : 26812)

#include "utf8.hpp"

#include "../common/strlib.hpp"
#include "../common/performance.hpp"
#include "../common/assistant.hpp"

#undef fopen
#undef fgets
#undef _fgets
#undef fread
#undef _fread
#undef fclose

#ifdef _WIN32
	#include <windows.h>
#else
	#include <errno.h>
	#include <string.h>

	#include <stdio.h>
	#include <locale.h>
	#include <langinfo.h>

	#include <iconv.h>
#endif // _WIN32

#include <unordered_map>

#include "../../3rdparty/uchardet/src/uchardet.h"

namespace PandasUtf8 {
// -------------------------------------------------------------------
// 概念定义
// -------------------------------------------------------------------
// 系统语言: (System Language, 与 e_pandas_language 对应)
// 	   系统语言是指操作系统展现给用户的语言, 决定着熊猫模拟器用什么样的终端汉化信息.
// 	   本文件中的 getSystemLanguage 函数用来获取当前操作系统的系统语言.
// 
//     在 Windows 中在控制面板可以修改, 使用 GetUserDefaultUILanguage 读取
//     在 Linux 中使用 setlocale(LC_CTYPE, NULL) 读取, 既 zh_CN.UTF-8 中的前半段: zh_CN
//
// 字符编码: (Character Encoding, 与 e_pandas_encoding 对应)
//     字符编码是指当前程序默认使用的编码, 比如 BIG5, GBK 等.
// 	   本文件中的 getSystemEncoding 函数用来获取当前操作系统的字符编码.
//
//     在 Windows 中使用 GetACP 来获取页码(Codepage), 使用页码进行字符编码的判断
//     在 Linux 中使用 setlocale(LC_CTYPE, NULL) 读取, 既 zh_CN.UTF-8 中的后半段: UTF-8
//
// 字符页码: (Character Codepage)
//     页码的概念仅在 Windows 系统上存在, 针对非 Unicode 程序系统会用页码来理解程序中的字符串.
//     而 Athena 系列模拟器都是非 Unicode 程序, 因此该值在 Windows 平台是关键的.
//
//     在本文件中我们将会使用 Codepage 来判断出 Encoding, 一切操作以 Encoding 作为基准,
//     在一些必要场景会进行 Encoding 和 Codepage 之间的相互转换.
//
// 特殊情况:
//     在 Win10 系统开始, Codepage 会被实验性的设置为 CP_UTF8 即 65001.
// 	   但由于客户端能呈现的更多是 BIG5 或者 GBK 这样的编码, 因此当 GetACP 得到的是 CP_UTF8 时,
// 	   我们会根据系统语言来确定理想的字符编码.
// -------------------------------------------------------------------

// 此处定义的缓冲区大小可参考 showmsg.cpp 中 SBUF_SIZE 的定义
// 按照 rAthena 的建议, 此处的 STRBUF_SIZE 不会设置低于 SBUF_SIZE 设定的值
#define STRBUF_SIZE 1024 * 4

// 当无法通过获得契合的字符编码时, 将会使用这里定义的默认编码 (Linux 平台上使用)
#define UNSUPPORT_DEFAULT_CODEPAGE "GBK"
#define UNSUPPORT_DEFAULT_ENCODING PANDAS_ENCODING_GBK

// 设定两个全局变量用于保存系统的语言和控制台的编码
// 这两个东西通常程序启动后就不会再发生任何变化, 为了避免频繁检测缓存起来比较值得
enum e_pandas_language systemLanguage = getSystemLanguage();
enum e_pandas_encoding systemEncoding = getSystemEncoding();

//************************************
// Method:      getSystemLanguage
// Description: 获取当前操作系统的默认展现语言
// Returns:     enum e_pandas_language
// Author:      Sola丶小克(CairoLee)  2020/01/24 21:59
//************************************
enum e_pandas_language getSystemLanguage() {
#ifdef _WIN32
	// GetUserDefaultUILanguage 获取到的编码对照表:
	// https://www.voidtools.com/support/everything/language_ids/
	switch (GetUserDefaultUILanguage()) {
	case 0x0409: return PANDAS_LANGUAGE_ENG;	// English (United States)
	case 0x0804: return PANDAS_LANGUAGE_CHS;	// Chinese (PRC) 
	case 0x0404: return PANDAS_LANGUAGE_CHT;	// Chinese (Taiwan Region)
	case 0x0c04: return PANDAS_LANGUAGE_CHT;	// Chinese (Hong Kong SAR, PRC) 
	case 0x041e: return PANDAS_LANGUAGE_THA;	// Thai (Thailand)
	default: return PANDAS_LANGUAGE_ENG;
	}
#else
	setlocale(LC_ALL, "");
	char* szLocale = setlocale(LC_CTYPE, NULL);

	if (boost::istarts_with(szLocale, "zh_CN"))
		return PANDAS_LANGUAGE_CHS;
	else if (boost::istarts_with(szLocale, "zh_TW"))
		return PANDAS_LANGUAGE_CHT;
	else if (boost::istarts_with(szLocale, "zh_HK"))
		return PANDAS_LANGUAGE_CHT;
	else if (boost::istarts_with(szLocale, "en_US"))
		return PANDAS_LANGUAGE_ENG;
	else if (boost::istarts_with(szLocale, "C."))
		return PANDAS_LANGUAGE_ENG;
	else if (boost::istarts_with(szLocale, "th_TH"))
		return PANDAS_LANGUAGE_THA;
	else {
		printf("%s: Unsupport locale: %s, defaulting to english\n", __func__, szLocale);
	}

	return PANDAS_LANGUAGE_ENG;
#endif // _WIN32
}

//************************************
// Method:      getSystemEncoding
// Description: 获取当前操作系统终端控制台的默认编码
// Returns:     enum e_pandas_encoding
// Author:      Sola丶小克(CairoLee)  2020/01/24 15:51
//************************************
enum e_pandas_encoding getSystemEncoding(bool bIgnoreUtf8) {
#ifdef _WIN32
	// 关于 GetACP 的编码对应表可以在以下文档中查询:
	// https://docs.microsoft.com/zh-cn/windows/win32/intl/code-page-identifiers
	UINT nCodepage = GetACP();

	switch (nCodepage) {
	case 874:	// TIS-620
		return PANDAS_ENCODING_TIS620;
	case 936:	// GBK
		return PANDAS_ENCODING_GBK;
	case 950:	// BIG5
		return PANDAS_ENCODING_BIG5;
	case 1252:	// LATIN1
		return PANDAS_ENCODING_LATIN1;
	case 65001:	// UTF-8
		if (!bIgnoreUtf8) {
			return PANDAS_ENCODING_UTF8;
		}
		switch (systemLanguage) {
		case PANDAS_LANGUAGE_ENG: return PANDAS_ENCODING_LATIN1;
		case PANDAS_LANGUAGE_CHS: return PANDAS_ENCODING_GBK;
		case PANDAS_LANGUAGE_CHT: return PANDAS_ENCODING_BIG5;
		case PANDAS_LANGUAGE_THA: return PANDAS_ENCODING_TIS620;
		default:
			return PANDAS_ENCODING_LATIN1;
		}
	default:
		ShowWarning("%s: Unsupport default ANSI codepage: %d, defaulting to latin1\n", __func__, nCodepage);
		return PANDAS_ENCODING_LATIN1;
	}
#else
	setlocale(LC_ALL, "");
	char* szLanginfo = nl_langinfo(CODESET);

	if (boost::icontains(szLanginfo, "UTF-8")) {
		if (!bIgnoreUtf8) {
			return PANDAS_ENCODING_UTF8;
		}
		switch (systemLanguage) {
		case PANDAS_LANGUAGE_ENG: return PANDAS_ENCODING_LATIN1;
		case PANDAS_LANGUAGE_CHS: return PANDAS_ENCODING_GBK;
		case PANDAS_LANGUAGE_CHT: return PANDAS_ENCODING_BIG5;
		case PANDAS_LANGUAGE_THA: return PANDAS_ENCODING_TIS620;
		default:
			return PANDAS_ENCODING_LATIN1;
		}
	}
	else if (boost::icontains(szLanginfo, "GBK"))
		return PANDAS_ENCODING_GBK;
	else if (boost::icontains(szLanginfo, "GB18030"))
		return PANDAS_ENCODING_GBK;
	else if (boost::icontains(szLanginfo, "Big5HKSCS"))
		return PANDAS_ENCODING_BIG5;
	else if (boost::icontains(szLanginfo, "Big5"))
		return PANDAS_ENCODING_BIG5;
	else if (boost::icontains(szLanginfo, "ANSI_X3.4-1968"))
		return PANDAS_ENCODING_LATIN1;
	else {
		printf("%s: Unsupport codeset: %s, defaulting to latin1\n", __func__, szLanginfo);
	}

	return PANDAS_ENCODING_LATIN1;
#endif // _WIN32
}

//************************************
// Method:      getEncodingByLanguage
// Description: 根据指定的系统语言获取与之匹配的字符编码
// Access:      public 
// Parameter:   e_pandas_language lang
// Returns:     enum e_pandas_encoding
// Author:      Sola丶小克(CairoLee)  2021/10/28 17:41
//************************************ 
enum e_pandas_encoding getEncodingByLanguage(e_pandas_language lang) {
	switch (lang) {
	case PANDAS_LANGUAGE_CHS: return PANDAS_ENCODING_GBK;
	case PANDAS_LANGUAGE_CHT: return PANDAS_ENCODING_BIG5;
	case PANDAS_LANGUAGE_THA: return PANDAS_ENCODING_TIS620;
	default: return UNSUPPORT_DEFAULT_ENCODING;
	}
}

//************************************
// Method:      getEncodingByString
// Description: 提供一个编码字符串来将他转换成一个 e_pandas_encoding 枚举值
// Access:      public 
// Parameter:   const std::string & strEncoding
// Returns:     enum e_pandas_encoding
// Author:      Sola丶小克(CairoLee)  2021/10/28 18:56
//************************************ 
enum e_pandas_encoding getEncodingByString(const std::string& strEncoding) {
	if (boost::icontains(strEncoding, "UTF-8"))
		return PANDAS_ENCODING_UTF8;
	if (boost::icontains(strEncoding, "GBK"))
		return PANDAS_ENCODING_GBK;
	if (boost::icontains(strEncoding, "BIG5"))
		return PANDAS_ENCODING_BIG5;
	if (boost::icontains(strEncoding, "LATIN1"))
		return PANDAS_ENCODING_LATIN1;
	if (boost::icontains(strEncoding, "TIS-620"))
		return PANDAS_ENCODING_TIS620;
	return PANDAS_ENCODING_UNKNOW;
}

#ifdef _WIN32
//************************************
// Method:      convertEncodingToCodepage
// Description: 根据 Encoding 来获取与之对应的字符内码
// Access:      public 
// Parameter:   enum e_pandas_encoding encoding
// Returns:     unsigned int
// Author:      Sola丶小克(CairoLee)  2021/10/28 16:48
//************************************ 
unsigned int convertEncodingToCodepage(enum e_pandas_encoding encoding) {
	switch (encoding) {
	case PANDAS_ENCODING_TIS620: return 874;
	case PANDAS_ENCODING_GBK: return 936;
	case PANDAS_ENCODING_BIG5: return 950;
	case PANDAS_ENCODING_LATIN1: return 1252;
	case PANDAS_ENCODING_UTF8: return 65001;
	default:
		ShowWarning("%s: Unsupport encoding: e_pandas_encoding[%d], defaulting to latin1\n", __func__, (int)encoding);
		return 1252;
	}
}
#else
//************************************
// Method:      convertEncodingToCodepage
// Description: 根据 Encoding 来获取与之对应的编码字符串
// Access:      public 
// Parameter:   enum e_pandas_encoding encoding
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/10/28 17:23
//************************************ 
std::string convertEncodingToCodepage(enum e_pandas_encoding encoding) {
	switch (encoding) {
	case PANDAS_ENCODING_GBK: return "GBK";
	case PANDAS_ENCODING_BIG5: return "BIG5";
	case PANDAS_ENCODING_LATIN1: return "LATIN1";
	case PANDAS_ENCODING_UTF8: return "UTF-8";
	case PANDAS_ENCODING_TIS620: return "TIS-620";
	default:
		ShowWarning("%s: Unsupport encoding: e_pandas_encoding[%d], defaulting to %s\n", __func__, UNSUPPORT_DEFAULT_CODEPAGE);
		return UNSUPPORT_DEFAULT_CODEPAGE;
	}
}
#endif // _WIN32

#ifdef _WIN32
//************************************
// Method:      setupConsoleOutputCP
// Description: 根据系统语言设置我们预期的终端输出编码
//              避免 Win10 将非 Unicode 文本编码调为 Beta:UTF8 的影响
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/4 22:52
//************************************
bool setupConsoleOutputCP() {
	e_pandas_encoding eEncoding = getSystemEncoding(true);
	unsigned int nCodepage = convertEncodingToCodepage(eEncoding);
	return (!SetConsoleOutputCP(nCodepage) || !SetConsoleCP(nCodepage));
}
#endif // _WIN32

//************************************
// Method:      UnicodeEncode
// Description: 将给定的 ANSI 字符串编码成 Unicode 字符串
// Parameter:   const std::string & strANSI
// Parameter:   unsigned int nCodepage
// Returns:     std::wstring
// Author:      Sola丶小克(CairoLee)  2020/01/31 14:00
//************************************
std::wstring UnicodeEncode(const std::string& strANSI, e_pandas_encoding strEncoding) {
#ifdef _WIN32
	unsigned int nCodepage = convertEncodingToCodepage(strEncoding);
	int unicodeLen = MultiByteToWideChar(nCodepage, 0, strANSI.c_str(), -1, NULL, 0);
	wchar_t* strUnicode = new wchar_t[unicodeLen];
	wmemset(strUnicode, 0, unicodeLen);
	MultiByteToWideChar(nCodepage, 0, strANSI.c_str(), -1, strUnicode, unicodeLen);
	std::wstring encoded(strUnicode);
	delete[] strUnicode;
	return std::move(encoded);
#else
	std::string encoding = convertEncodingToCodepage(strEncoding);
	iconv_t descr_in = iconv_open("WCHAR_T//IGNORE", encoding.c_str());

	if ((iconv_t)-1 == descr_in) {
		return L"";
	}

	const char* instr = strANSI.c_str();
	size_t instr_len = (strANSI.length() + 1) * sizeof(char);

	wchar_t* result_buf = new wchar_t[instr_len * sizeof(wchar_t)];
	wchar_t* result_buf_out = result_buf;
	size_t result_buf_len = instr_len * sizeof(wchar_t);
	memset(result_buf, 0, result_buf_len);

	size_t iconv_result = iconv(descr_in,
		(char**)&instr, &instr_len,
		(char**)&result_buf, &result_buf_len
	);

	std::wstring w_content(result_buf_out);
	delete[] result_buf_out;
	iconv_close(descr_in);

	return std::move(w_content);
#endif // _WIN32
}

//************************************
// Method:      UnicodeDecode
// Description: 将给定的 Unicode 字符串解码成 ANSI 字符串
// Access:      public 
// Parameter:   const std::wstring & strUnicode
// Parameter:   e_pandas_encoding strEncoding
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/10/29 21:47
//************************************ 
std::string UnicodeDecode(const std::wstring& strUnicode, e_pandas_encoding strEncoding) {
#ifdef _WIN32
	unsigned int nCodepage = convertEncodingToCodepage(strEncoding);
	int ansiLen = WideCharToMultiByte(nCodepage, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);
	char* strAnsi = new char[ansiLen];
	memset(strAnsi, 0, ansiLen);
	WideCharToMultiByte(nCodepage, 0, strUnicode.c_str(), -1, strAnsi, ansiLen, NULL, NULL);
	std::string decoded(strAnsi);
	delete[] strAnsi;
	return std::move(decoded);
#else
	std::string encoding = convertEncodingToCodepage(strEncoding);
	encoding += "//IGNORE";
	iconv_t descr_out = iconv_open(encoding.c_str(), "WCHAR_T");

	if ((iconv_t)-1 == descr_out) {
		return "";
	}

	const char* instr = (const char*)strUnicode.c_str();
	size_t instr_len = (strUnicode.length() + 1) * sizeof(wchar_t);

	char* result_buf = new char[instr_len];
	char* result_buf_out = result_buf;
	size_t result_buf_len = instr_len;
	memset(result_buf, 0, result_buf_len);

	size_t iconv_result = iconv(descr_out,
		(char**)&instr, &instr_len,
		(char**)&result_buf, &result_buf_len
	);

	std::string s_content(result_buf_out);
	delete[] result_buf_out;
	iconv_close(descr_out);

	return std::move(s_content);
#endif // _WIN32
}

//************************************
// Method:      splashUnicodeToBIG5
// Description: 当其他字符串从 Unicode 转换成 BIG5 编码的字符串时,
//              若发现字符低位为 0x5C 的字符则在后面补一个反斜杠, 用于解决 BIG5 冲码问题 
// Parameter:   const std::wstring & strUnicode
// Returns:     std::string 输出的将会是繁体中文的 ANSI 字符串
// Author:      Sola丶小克(CairoLee)  2020/8/2 21:32
//************************************
std::string splashUnicodeToBIG5(const std::wstring& strUnicode) {
	std::string strAnsi;

	for (wchar_t uniChar : strUnicode) {
		// 遍历每一个多字节字符串, 将他们转换成 std::wstring
		std::wstring uniStr;
		uniStr.push_back(uniChar);

		// 将 uniStr 单独转换成 Ansi 字符
		std::string ansiChar = UnicodeDecode(uniStr, PANDAS_ENCODING_BIG5);

		// 如若 ansiChar 等于两个字节, 且字符的低位等于 0x5C,
		// 那么输出时末尾多来个反斜杠
		if (ansiChar.size() == 2 && ansiChar.c_str()[1] == 0x5C) {
			strAnsi += ansiChar + "\\";
			continue;
		}

		strAnsi += ansiChar;
	}

	return strAnsi;
}

//************************************
// Method:      unsplashUnicodeToUtf8
// Description: 当 BIG5 繁体中文字符串从 Unicode 转换成 Utf8 字符串时,
//              自动移除用来解决 BIG5 编码冲码的 0x5C 反斜杠
// Access:      public 
// Parameter:   const std::wstring & strUnicode 从 BIG5 编码用 UnicodeEncode 函数转换成的 Unicode 字符串
// Returns:     std::string 输出的字符串将会是 Utf8 编码的字符串
// Author:      Sola丶小克(CairoLee)  2021/10/22 18:54
//************************************ 
std::string unsplashUnicodeToUtf8(const std::wstring& strUnicode) {
	std::string strUtf8;
	bool bNeedSkipNextSplash = false;

	for (wchar_t uniChar : strUnicode) {
		if (uniChar == 0x5C && bNeedSkipNextSplash) {
			bNeedSkipNextSplash = false;
			continue;
		}

		// 无论上面是否处理成功都将标记位重置回默认
		bNeedSkipNextSplash = false;

		// 遍历每一个多字节字符串, 将他们转换成 std::wstring
		std::wstring uniStr;
		uniStr.push_back(uniChar);

		// 将 uniStr 单独转换成 Ansi 字符
		std::string ansiChar = UnicodeDecode(uniStr, PANDAS_ENCODING_BIG5);

		// 如若 ansiChar 等于两个字节, 且字符的低位等于 0x5C,
		// 那么跳过下一个紧挨着的反斜杠
		if (ansiChar.size() == 2 && ansiChar.c_str()[1] == 0x5C) {
			bNeedSkipNextSplash = true;
		}

		std::string utf8Char = UnicodeDecode(uniStr, PANDAS_ENCODING_UTF8);
		strUtf8 += utf8Char;
	}

	return strUtf8;
}

//************************************
// Method:      splashForUtf8
// Description: 当 Utf8 字符串中包含的 BIG5 字符低位为 0x5C 时, 自动追加反斜杠
//              在正常情况下这毫无意义, 不过游戏客户端比较呆, 没办法.
// Access:      public 
// Parameter:   const std::string & strUtf8 兼容 BIG5 编码的 Utf8 字符串
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/10/25 22:33
//************************************ 
std::string splashForUtf8(const std::string& strUtf8) {
	std::string strResult;
	std::wstring strUnicode = UnicodeEncode(strUtf8, PANDAS_ENCODING_UTF8);

	for (wchar_t uniChar : strUnicode) {
		// 遍历每一个多字节字符串, 将他们转换成 std::wstring
		std::wstring uniStr;
		uniStr.push_back(uniChar);

		// 将 uniStr 单独转换成 Ansi 字符
		std::string ansiChar = UnicodeDecode(uniStr, PANDAS_ENCODING_BIG5);
		std::string utf8Char = UnicodeDecode(uniStr, PANDAS_ENCODING_UTF8);

		// 如若 ansiChar 等于两个字节, 且字符的低位等于 0x5C,
		// 那么输出时末尾多来个反斜杠
		if (ansiChar.size() == 2 && ansiChar.c_str()[1] == 0x5C) {
			strResult += utf8Char + "\\";
			continue;
		}

		strResult += utf8Char;
	}

	return strResult;
}

//************************************
// Method:      isUtf8Content
// Description: 判断给定的缓冲区是不是 UTF-8 编码的内容
// Parameter:   const char * buffer
// Parameter:   size_t len
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2022/08/02 22:09
//************************************
bool isUtf8Content(const char* buffer, size_t len) {
	uchardet_t handle = uchardet_new();
	
	int retval = uchardet_handle_data(handle, buffer, len);
	if (retval != 0) {
		uchardet_data_end(handle);
		uchardet_delete(handle);
		return false;
	}
	uchardet_data_end(handle);

	bool bIsUtf8Content = false;
	const char* charset = uchardet_get_charset(handle);

	if (charset && !stricmp(charset, "UTF-8")) {
		bIsUtf8Content = true;
	}

	uchardet_delete(handle);
	return bIsUtf8Content;
}

#ifndef _WIN32
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
	if (systemEncoding != PANDAS_ENCODING_UTF8) {
		return mes;
	}

	e_pandas_encoding fromEncoding = getEncodingByLanguage();
	if (fromEncoding != PANDAS_ENCODING_GBK && fromEncoding != PANDAS_ENCODING_BIG5) {
		return mes;
	}

	return ansiToUtf8(mes, fromEncoding);
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
	strBuf = consoleConvert(strBuf);

	// 将处理完的字符串输出到指定的地方去 (显示到终端)
	return fprintf(file, "%s", strBuf.c_str());
}

#endif // _WIN32

//************************************
// Method:      utf8ToAnsi
// Description: 将 UTF8 字符串转换为 ANSI 字符串 (ANSI 字符将自适应当前系统语言对应的编码)
// Parameter:   const std::string & strUtf8 须为 UTF-8 编码的字符串
// Parameter:   int flag
//				0x1	若携带此标记则表示需要先移除掉 Utf8 编码下为 0x5C 追加的反斜杠
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/24 00:26
//************************************
std::string utf8ToAnsi(const std::string& strUtf8, int flag) {
#ifdef _WIN32
	e_pandas_encoding toEncoding = getSystemEncoding(true);
	return utf8ToAnsi(strUtf8, toEncoding, flag);
#else
	e_pandas_encoding toEncoding = getEncodingByLanguage();
	return utf8ToAnsi(strUtf8, toEncoding, flag);
#endif // _WIN32
}

//************************************
// Method:      utf8ToAnsi
// Description: 将 UTF8 字符串转换为 ANSI 字符串 (ANSI 字符将使用指定的编码)
// Access:      public 
// Parameter:   const std::string & strUtf8
// Parameter:   const std::string & toEncoding
// Parameter:   int flag
//				0x1	表示需要先移除掉 Utf8 编码下为 0x5C 追加的反斜杠
// 				0x2 表示需要在转换成 BIG5 编码的时候为低位为 0x5C 的字符追加反斜杠
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/09/30 20:57
//************************************ 
std::string utf8ToAnsi(const std::string& strUtf8, e_pandas_encoding toEncoding, int flag) {
	if (toEncoding == PANDAS_ENCODING_UNKNOW)
		return utf8ToAnsi(strUtf8, flag);

	std::wstring strUnicode = UnicodeEncode(strUtf8, PANDAS_ENCODING_UTF8);

	if (toEncoding == PANDAS_ENCODING_BIG5) {
		if (flag & 0x1) {
			// 由于传入的 Utf8 字符可能已经针对 BIG5 的情况在 0x5C 后面追加了反斜杠
			// 在开始我们自己追加反斜杠的操作之前, 首先需要先移除掉 Utf8 编码下为 0x5C 追加的反斜杠
			std::string strUnsplashUtf8 = unsplashUnicodeToUtf8(strUnicode);
			strUnicode = UnicodeEncode(strUnsplashUtf8, PANDAS_ENCODING_UTF8);
		}

		if (flag & 0x2) {
			// 若指定的目标 Codepage 是繁体中文 (BIG5),
			// 那么需要在字符的低位字节为 0x5C 的情况下, 自动追加反斜杠
			return splashUnicodeToBIG5(strUnicode);
		}
	}

	// 若不是繁体中文 (BIG5) 则不存在此问题, 将 Unicode 转换成 ANSI 字符即可
	return UnicodeDecode(strUnicode, toEncoding);
}

//************************************
// Method:      utf8ToAnsi
// Description: 将 UTF8 字符串转换为 ANSI 字符串 (ANSI 字符将使用指定的编码)
// Access:      public 
// Parameter:   const std::string & strUtf8
// Parameter:   const std::string & strToEncoding
// Parameter:   int flag
//				0x1	若携带此标记则表示需要先移除掉 Utf8 编码下为 0x5C 追加的反斜杠
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/10/28 18:54
//************************************
std::string utf8ToAnsi(const std::string& strUtf8, const std::string& strToEncoding, int flag) {
	e_pandas_encoding toEncoding = getEncodingByString(strToEncoding);
	if (toEncoding == PANDAS_ENCODING_UNKNOW) {
		return strUtf8;
	}
	return utf8ToAnsi(strUtf8, toEncoding, flag);
}

//************************************
// Method:      ansiToUtf8
// Description: 将 ANSI 字符串转换为 UTF8 字符串 (ANSI 字符将自适应当前系统语言对应的编码)
// Parameter:   const std::string & strAnsi 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/24 00:26
//************************************
std::string ansiToUtf8(const std::string& strAnsi) {
#ifdef _WIN32
	e_pandas_encoding fromEncoding = getSystemEncoding(true);
	return ansiToUtf8(strAnsi, fromEncoding);
#else
	e_pandas_encoding fromEncoding = getEncodingByLanguage();
	return ansiToUtf8(strAnsi, fromEncoding);
#endif // _WIN32
}

//************************************
// Method:      ansiToUtf8
// Description: 将 ANSI 字符串转换为 UTF8 字符串 (ANSI 字符将使用指定的编码)
// Access:      public 
// Parameter:   const std::string & strAnsi
// Parameter:   const std::string & fromEncoding
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/09/30 20:57
//************************************ 
std::string ansiToUtf8(const std::string& strAnsi, e_pandas_encoding fromEncoding) {
	if (fromEncoding == PANDAS_ENCODING_UNKNOW)
		return ansiToUtf8(strAnsi);

	std::wstring strUnicode = UnicodeEncode(strAnsi, fromEncoding);

	if (fromEncoding == PANDAS_ENCODING_BIG5) {
		// 若当前指定的目标 Codepage 是繁体中文 (BIG5),
		// 那么需要在字符的低位字节为 0x5C 的情况下, 自动移除紧接着的反斜杠
		return unsplashUnicodeToUtf8(strUnicode);
	}

	// 若不是繁体中文 (BIG5) 则不存在此问题, 将 Unicode 转换成 Utf8 字符即可
	return UnicodeDecode(strUnicode, PANDAS_ENCODING_UTF8);
}

//************************************
// Method:      ansiToUtf8
// Description: 将 ANSI 字符串转换为 UTF8 字符串 (ANSI 字符将使用指定的编码)
// Access:      public 
// Parameter:   const std::string & strAnsi
// Parameter:   const std::string & strFromEncoding
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/10/28 18:54
//************************************ 
std::string ansiToUtf8(const std::string& strAnsi, const std::string& strFromEncoding) {
	e_pandas_encoding fromEncoding = getEncodingByString(strFromEncoding);
	if (fromEncoding == PANDAS_ENCODING_UNKNOW) {
		return strAnsi;
	}
	return ansiToUtf8(strAnsi, fromEncoding);
}

//////////////////////////////////////////////////////////////////////////

// 用于保存 FILE 指针和文件编码模式的缓存
std::unordered_map<FILE*, e_file_charsetmode> __fpmodecache;

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
// Method:      get_charsetmode
// Description: 提供一个缓冲区, 尝试获取其编码模式
// Access:      public 
// Parameter:   unsigned char * buf
// Parameter:   size_t len 缓冲区的长度
// Returns:     enum e_file_charsetmode
// Author:      Sola丶小克(CairoLee)  2021/02/06 11:29
//************************************ 
enum e_file_charsetmode get_charsetmode(unsigned char* buf, size_t len) {
	enum e_file_charsetmode charset_mode = FILE_CHARSETMODE_UNKNOW;
	if (len >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
		// UTF8-BOM
		charset_mode = FILE_CHARSETMODE_UTF8_BOM;
	}
	else if (len >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
		// UCS-2 LE
		charset_mode = FILE_CHARSETMODE_UCS2_LE;
	}
	else if (len >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) {
		// UCS-2 BE
		charset_mode = FILE_CHARSETMODE_UCS2_BE;
	}
	else if (len > 0 && isUtf8Content((const char*)buf, len)) {
		// UTF8 without BOM
		charset_mode = FILE_CHARSETMODE_UTF8;
	}
	else {
		// 若无法根据上面的前几个字节判断出编码, 那么默认为 ANSI 编码 (GBK\BIG5)
		charset_mode = FILE_CHARSETMODE_ANSI;
	}
	return charset_mode;
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
	enum e_file_charsetmode cached_charsetmode = getModeMapping(_Stream);
	if (cached_charsetmode != FILE_CHARSETMODE_UNKNOW) {
		return cached_charsetmode;
	}

	// 若传递的 FILE 指针无效则直接返回编码不可知
	if (_Stream == nullptr) {
		return FILE_CHARSETMODE_UNKNOW;
	}

	// 记录目前游标所在的位置
	long curpos = ftell(_Stream);

	// 将游标移动到文件末尾
	fseek(_Stream, 0, SEEK_END);

	// 读取文件的大小
	size_t size = ftell(_Stream);

	// 申请容量足够的缓冲区, 用于存放一会儿要读取的内容
	unsigned char* buf = (unsigned char*)aMalloc(size + 1);

	// 指针移动到开头, 准备读取整个文件的内容
	fseek(_Stream, 0, SEEK_SET);

	// 读取整个文件的内容
	size_t extracted = 0;
	extracted = ::fread(buf, sizeof(unsigned char), size, _Stream);

	// 根据读取到的数据来判断文本的编码类型
	enum e_file_charsetmode charset_mode = get_charsetmode(buf, extracted);

	// 释放掉缓冲区
	if (buf) aFree(buf);

	// 将指针设置回原来的位置, 避免影响后续的读写流程
	fseek(_Stream, curpos, SEEK_SET);

	// 将当前分析到的 charset_mode 保存到缓存
	setModeMapping(_Stream, charset_mode);

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
	// 记录目前游标所在的位置
	long curpos = (long)ifs.tellg();

	// 将游标移动到文件末尾
	ifs.seekg(0, std::ios::end);

	// 读取文件的大小
	long size = (long)ifs.tellg();

	// 指针移动到开头, 读取前 3 个字节
	ifs.seekg(0, std::ios::beg);

	// 申请容量足够的缓冲区, 用于存放一会儿要读取的内容
	unsigned char* buf = (unsigned char*)aMalloc(size + 1);

	// 读取整个文件的内容
	ifs.read((char*)buf, size);

	// 获取读取成功的字节个数
	long extracted = (long)ifs.gcount();

	// 根据读取到的数据来判断文本的编码类型
	enum e_file_charsetmode charset_mode = get_charsetmode(buf, extracted);

	// 释放掉缓冲区
	if (buf) aFree(buf);

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
	std::string strMode(_Mode);
	std::string strFilename(_FileName);
	
	if (strchr(_Mode, 'b') || strchr(_Mode, 'w') || strchr(_Mode, 'a')) {
		// 若当前打开文件的模式已经是二进制, 那么无需修改打开模式
		// 若当前打开文件的模式包含 Write 或者是 Append 模式, 那么也无需修改打开模式
		strMode = _Mode;
	}
	else {
		// 若文件的打开模式不以二进制模式打开, 那么补充对应的 _Mode 标记
		strMode += "b";
	}

#ifndef _WIN32
	// 在 Linux 环境下并且终端编码为 UTF8 的情况下,
	// 将需要打开的文件路径直接转码成 utf8 编码再进行读取, 否则会找不到文件
	if (systemEncoding == PANDAS_ENCODING_UTF8) {
		strFilename = consoleConvert(_FileName).c_str();
	}
#endif // _WIN32

	return ::fopen(strFilename.c_str(), strMode.c_str());
}

//************************************
// Method:      fclose
// Description: 进行了一些清理工作的 fclose 函数
// Parameter:   FILE * _fp
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2020/02/16 01:05
//************************************
int fclose(FILE* _fp) {
	clearModeMapping(_fp);
	return ::fclose(_fp);
}

//************************************
// Method:      fgets
// Description: 能够将 utf8 内容转换成普通 ansi 编码的 fgets 函数
// Parameter:   char * _Buffer
// Parameter:   int _MaxCount
// Parameter:   FILE * _Stream
// Parameter:   int flag
// Returns:     char*
// Author:      Sola丶小克(CairoLee)  2022/07/30 22:05
//************************************
char* fgets(char* _Buffer, int _MaxCount, FILE* _Stream, int flag/* = 0*/) {
	e_file_charsetmode mode = PandasUtf8::fmode(_Stream);

	// 若不是 UTF8-BOM 或者 UTF8, 那么直接透传 fgets 调用
	if (mode != FILE_CHARSETMODE_UTF8_BOM && mode != FILE_CHARSETMODE_UTF8) {
		return ::fgets(_Buffer, _MaxCount, _Stream);
	}

	// 使用 UTF8-BOM 编码时, 若指针在文件的前 3 个字节, 那么将指针移动到前 3 个字节的后面,
	// 避免后续进行 fgets 的时候读取到前 3 个字节, 同时将当前位置记录到 curpos
	long curpos = ftell(_Stream);
	if (mode == FILE_CHARSETMODE_UTF8_BOM && curpos < 3) fseek(_Stream, 3, SEEK_SET);

	// 读取 _MaxCount 长度的内容并保存到 buffer 中
	char* buffer = new char[_MaxCount];
	char* fgets_result = ::fgets(buffer, _MaxCount, _Stream);

	if (!fgets_result) {
		delete[] buffer;
		return fgets_result;
	}
	
	std::string line(buffer);

	// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
	std::string strAnsi = utf8ToAnsi(line, flag);
	memset(_Buffer, 0, _MaxCount);

	if (strAnsi.size() > (size_t)_MaxCount) {
		// 如果转换后的字符串长度大于 _Buffer 的容量, 那么放弃转换并报错
		ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", __func__, sizeof(_Buffer), strAnsi.size());
		memcpy(_Buffer, buffer, _MaxCount);
	}
	else {
		// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
		memcpy(_Buffer, strAnsi.c_str(), strAnsi.size());
	}

	delete[] buffer;
	return _Buffer;
}

//************************************
// Method:      _fgets
// Description: 防止 utf8_defines.hpp 中宏定义重复的 fgets 透传函数
// Parameter:   char * _Buffer
// Parameter:   int _MaxCount
// Parameter:   FILE * _Stream
// Parameter:   int flag
// Returns:     char*
// Author:      Sola丶小克(CairoLee)  2022/07/30 22:06
//************************************
char* _fgets(char* _Buffer, int _MaxCount, FILE* _Stream, int flag/* = 0*/) {
	return fgets(_Buffer, _MaxCount, _Stream, flag);
}

//************************************
// Method:      fread
// Description: 能够将 utf8 内容转换成普通 ansi 编码的 fread 函数
// Parameter:   void * _Buffer
// Parameter:   size_t _ElementSize
// Parameter:   size_t _ElementCount
// Parameter:   FILE * _Stream
// Parameter:   int flag
// Returns:     size_t
// Author:      Sola丶小克(CairoLee)  2022/07/30 22:05
//************************************
size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream, int flag/* = 0*/) {
	e_file_charsetmode mode = PandasUtf8::fmode(_Stream);

	// 若不是 UTF8-BOM 或者 UTF8, 那么直接透传 fread 调用
	if (mode != FILE_CHARSETMODE_UTF8_BOM && mode != FILE_CHARSETMODE_UTF8) {
		return ::fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}

	// 若 _ElementSize 不等于 1, 那么直接透传 fread 调用
	if (_ElementSize != 1) {
		return ::fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}

	size_t extracted = 0;
	long curpos = ftell(_Stream);
	size_t elementlen = (_ElementSize * _ElementCount) + 1;
	char* buffer = new char[elementlen];

	// 使用 UTF8-BOM 编码时, 若指针在文件的前 3 个字节, 那么将指针移动到前 3 个字节后面,
	// 避免后续进行 fread 的时候读取到前 3 个字节的 BOM
	if (mode == FILE_CHARSETMODE_UTF8_BOM && curpos < 3) {
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

	// 这里需要手动处理一下确保零结尾
	// 在 Linux 系统上虽然前面已经将缓冲区填充为 0
	// 但最终返回的内容依然会有一些内存中的残留数据跟着回来
	buffer[extracted] = '\0';
	
	if (!extracted) {
		delete[] buffer;
		return extracted;
	}

	// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
	std::string strAnsi = utf8ToAnsi(std::string(buffer), flag);
	memset(_Buffer, 0, _ElementSize * _ElementCount);

	if (strAnsi.size() > elementlen) {
		// 如果转换后的字符串长度大于 _Buffer 的容量, 那么放弃转换并报错
		ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", __func__, _ElementCount, strAnsi.size());
		memcpy(_Buffer, buffer, elementlen);
	}
	else {
		// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
		memcpy(_Buffer, strAnsi.c_str(), strAnsi.size());

		// 转换成功后 extracted 应该更新为 strAnsi 字符串的大小
		// 因为外部调用者可能会依赖返回值对字符串进行截断
		extracted = strAnsi.size();
	}

	delete[] buffer;
	return extracted;
}

//************************************
// Method:      _fread
// Description: 防止 utf8_defines.hpp 中宏定义重复的 fread 透传函数
// Parameter:   void * _Buffer
// Parameter:   size_t _ElementSize
// Parameter:   size_t _ElementCount
// Parameter:   FILE * _Stream
// Parameter:   int flag
// Returns:     size_t
// Author:      Sola丶小克(CairoLee)  2022/07/30 22:06
//************************************
size_t _fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream, int flag/* = 0*/) {
	return fread(_Buffer, _ElementSize, _ElementCount, _Stream, flag);
}

} // namespace PandasUtf8

// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_UTF8_HPP_
#define _RATHENA_CN_UTF8_HPP_

#include "showmsg.hpp"
#include "cbasetypes.hpp"

#include "../config/pandas.hpp"

#include <stdio.h>
#include <string>	// std::string
#include <fstream>	// std::ifstream

enum e_file_charsetmode : uint8 {
	FILE_CHARSETMODE_UNKNOW = 0,
	FILE_CHARSETMODE_ANSI,
	FILE_CHARSETMODE_UTF8_BOM,
	FILE_CHARSETMODE_UCS2_LE,
	FILE_CHARSETMODE_UCS2_BE
};

enum e_console_encoding : uint8 {
	CONSOLE_ENCODING_UNKNOW = 0,
	CONSOLE_ENCODING_LATIN1,
	CONSOLE_ENCODING_UTF8,
	CONSOLE_ENCODING_GB2312,
	CONSOLE_ENCODING_BIG5
};

enum e_system_language : uint8 {
	SYSTEM_LANGUAGE_UNKNOW = 0,
	SYSTEM_LANGUAGE_ENG,
	SYSTEM_LANGUAGE_CHS,
	SYSTEM_LANGUAGE_CHT
};

class PandasUtf8
{
public:
	static enum e_console_encoding consoleEncoding;
	static enum e_system_language systemLanguage;

	static enum e_console_encoding getConsoleEncoding();
	static enum e_system_language getSystemLanguage();

	static enum e_file_charsetmode fmode(FILE* _Stream);
	static enum e_file_charsetmode fmode(std::ifstream& ifs);

	static FILE* fopen(const char* _FileName, const char* _Mode);
	static char* fgets(char* _Buffer, int _MaxCount, FILE* _Stream);
	static size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream);

	static std::string utf8ToAnsi(const std::string& strUtf8);
	static std::string ansiToUtf8(const std::string& strAnsi);

	static std::string getDefaultCodepage();

#ifdef _WIN32
	static std::wstring UnicodeEncode(const std::string& strANSI, unsigned int nCodepage);
	static std::string UnicodeDecode(const std::wstring& strUnicode, unsigned int nCodepage);
#else
	static std::string iconvConvert(const std::string& val, const std::string& from_charset, const std::string& to_charset);
	static std::string consoleConvert(const std::string& mes);
	static int vfprintf(FILE* file, const char* fmt, va_list args);
#endif // _WIN32
};

#endif // _RATHENA_CN_UTF8_HPP_

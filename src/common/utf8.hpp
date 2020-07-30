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

namespace PandasUtf8 {

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
	CONSOLE_ENCODING_GBK,
	CONSOLE_ENCODING_BIG5
};

enum e_system_language : uint8 {
	SYSTEM_LANGUAGE_UNKNOW = 0,
	SYSTEM_LANGUAGE_ENG,
	SYSTEM_LANGUAGE_CHS,
	SYSTEM_LANGUAGE_CHT
};

// 引用全局变量声明, 变量定义在 utf8.cpp 中
extern enum e_console_encoding consoleEncoding;
extern enum e_system_language systemLanguage;

void setModeMapping(FILE* _fp, e_file_charsetmode _mode);
e_file_charsetmode getModeMapping(FILE* _fp);
void clearModeMapping(FILE* _fp);

enum e_console_encoding getConsoleEncoding();
enum e_system_language getSystemLanguage();

// 对一系列的文件操作函数进行额外的加工操作
// 最终通过 src/common/utf8_defines.hpp 进行整个工程的函数替换操作
enum e_file_charsetmode fmode(FILE* _Stream);
enum e_file_charsetmode fmode(std::ifstream& ifs);
FILE* fopen(const char* _FileName, const char* _Mode);
char* fgets(char* _Buffer, int _MaxCount, FILE* _Stream);
size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream);
int fclose(FILE* _fp);

std::string utf8ToAnsi(const std::string& strUtf8);
std::string ansiToUtf8(const std::string& strAnsi);

std::string getDefaultCodepage();

#ifdef _WIN32
	std::wstring UnicodeEncode(const std::string& strANSI, unsigned int nCodepage);
	std::string UnicodeDecode(const std::wstring& strUnicode, unsigned int nCodepage);
#else
	std::string iconvConvert(const std::string& val, const std::string& from_charset, const std::string& to_charset);
	std::string consoleConvert(const std::string& mes);
	int vfprintf(FILE* file, const char* fmt, va_list args);
#endif // _WIN32

} // namespace PandasUtf8

#endif // _RATHENA_CN_UTF8_HPP_

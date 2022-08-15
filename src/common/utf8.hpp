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
	FILE_CHARSETMODE_UTF8,
	FILE_CHARSETMODE_UCS2_LE,
	FILE_CHARSETMODE_UCS2_BE,
	FILE_CHARSETMODE_TIS620
};

enum e_pandas_encoding : uint8 {
	PANDAS_ENCODING_UNKNOW = 0,
	PANDAS_ENCODING_LATIN1,
	PANDAS_ENCODING_UTF8,
	PANDAS_ENCODING_GBK,
	PANDAS_ENCODING_BIG5,
	PANDAS_ENCODING_TIS620
};

enum e_pandas_language : uint8 {
	PANDAS_LANGUAGE_UNKNOW = 0,
	PANDAS_LANGUAGE_ENG,
	PANDAS_LANGUAGE_CHS,
	PANDAS_LANGUAGE_CHT,
	PANDAS_LANGUAGE_THA
};

// 引用全局变量声明, 变量定义在 utf8.cpp 中
extern enum e_pandas_encoding systemEncoding;
extern enum e_pandas_language systemLanguage;

enum e_pandas_language getSystemLanguage();
enum e_pandas_encoding getSystemEncoding(bool bIgnoreUtf8 = false);
enum e_pandas_encoding getEncodingByLanguage(e_pandas_language lang = systemLanguage);

std::string utf8ToAnsi(const std::string& strUtf8, int flag = 0);
std::string utf8ToAnsi(const std::string& strUtf8, e_pandas_encoding toCharset, int flag = 0);
std::string utf8ToAnsi(const std::string& strUtf8, const std::string& strToEncoding, int flag = 0);

std::string ansiToUtf8(const std::string& strAnsi);
std::string ansiToUtf8(const std::string& strAnsi, e_pandas_encoding fromEncoding);
std::string ansiToUtf8(const std::string& strAnsi, const std::string& strFromEncoding);

std::string splashForUtf8(const std::string& strUtf8);

#ifdef _WIN32
	bool setupConsoleOutputCP();
#else
	std::string consoleConvert(const std::string& mes);
	int vfprintf(FILE* file, const char* fmt, va_list args);
#endif // _WIN32

//////////////////////////////////////////////////////////////////////////

void setModeMapping(FILE* _fp, e_file_charsetmode _mode);
e_file_charsetmode getModeMapping(FILE* _fp);
void clearModeMapping(FILE* _fp);

// 对一系列的文件操作函数进行额外的加工操作
// 最终通过 src/common/utf8_defines.hpp 进行整个工程的函数替换操作
enum e_file_charsetmode fmode(FILE* _Stream);
enum e_file_charsetmode fmode(std::ifstream& ifs);
FILE* fopen(const char* _FileName, const char* _Mode);
char* fgets(char* _Buffer, int _MaxCount, FILE* _Stream, int flag = 0);
char* _fgets(char* _Buffer, int _MaxCount, FILE* _Stream, int flag = 0);
size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream, int flag = 0);
size_t _fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream, int flag = 0);
int fclose(FILE* _fp);

} // namespace PandasUtf8

#endif // _RATHENA_CN_UTF8_HPP_

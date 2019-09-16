// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string> // std::string
#include <time.h> // struct tm
#include <memory> // std::shared_ptr

void deployImportDirectories();

bool getExecuteFilepath(std::string& outFilepath);
bool getExecuteFileDirectory(std::string& outFileDirectory);

bool deleteDirectory(std::string szDirPath);
bool isDirectoryExists(const std::string& path);
bool makeDirectories(const std::string& path);
bool copyDirectory(std::string fromPath, std::string toPath);

bool copyFile(std::string fromPath, std::string toPath);

bool strEndWith(std::string fullstring, std::string ending);
bool strEndWith(std::wstring fullstring, std::wstring ending);

void strReplace(std::string& str, const std::string& from, const std::string& to);
void strReplace(std::wstring& str, const std::wstring& from, const std::wstring& to);

void standardizePathSep(std::string& path);
void standardizePathSep(std::wstring& path);

void ensurePathEndwithSep(std::string& path, std::string sep);
void ensurePathEndwithSep(std::wstring& path, std::wstring sep);

std::wstring string2wstring(const std::string& s);
std::string wstring2string(const std::wstring& ws);

std::string & strFormat(std::string & _str, const char * _Format, ...);
std::string strFormat(const char* _Format, ...);

std::wstring & strFormat(std::wstring & _str, const wchar_t * _Format, ...);
std::wstring strFormat(const wchar_t* _Format, ...);

std::string getPandasVersion(bool without_vmark = false);

#ifndef MINICORE
// 若 MINICORE 宏定义启用的话, 则表示目前链接这个 hpp 的工程是一个辅助工具 (csv2yaml)
// 这些辅助工具引入的是 common-minicore 工程, 而这个 common-minicore 工程没有将 sql.hpp 等文件 include 进去,
// 所以 assistant.hpp 在 minicore 模式下使用的话, 会出现链接错误
//
// 目前为了解决由此引发的链接问题, 这里将部分 common-minicore 没有 include 的头文件,
// 以及利用到这些头文件的函数单独剥离, 并使用 MINICORE 宏定义条件包起来.
#include "sql.hpp"

void detectCodepage(Sql* sql_handle, const char* connect_name, const char* codepage);
#endif // MINICORE

#ifdef _WIN32
std::string getFileVersion(std::string filename, bool bWithoutBuildNum);
#endif // _WIN32

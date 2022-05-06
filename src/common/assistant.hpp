// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string> // std::string
#include <memory> // std::shared_ptr
#include <vector> // std::vector

// 使 Boost::locale 使用 unique_ptr 而不是已被声明废弃的 auto_ptr
// 定义这个开关是 Boost 的推荐解决方案, 主要为了关闭在 GCC 编译器下的警告提示
#define BOOST_LOCALE_HIDE_AUTO_PTR

#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

void deployImportDirectories();

bool getExecuteFilepath(std::string& outFilepath);
bool getExecuteFileDirectory(std::string& outFileDirectory);

bool isDirectoryExists(const std::string& path);
bool makeDirectories(const std::string& path);
bool ensureDirectories(const std::string& filepath);
bool deleteDirectory(std::string szDirPath);
bool copyDirectory(const boost::filesystem::path& from, const boost::filesystem::path& to);

bool isFileExists(const std::string& path);
bool copyFile(const std::string& fromPath, const std::string& toPath);
bool deleteFile(const std::string& path);

bool strEndWith(std::string fullstring, std::string ending);
bool strEndWith(std::wstring fullstring, std::wstring ending);

void strReplace(std::string& str, const std::string& from, const std::string& to);
void strReplace(std::wstring& str, const std::wstring& from, const std::wstring& to);
void strReplace(char* str, const char* from, const char* to);

bool strContain(std::vector<std::string> needle, std::string& str);
bool strContain(std::string needle, std::string& str);

std::string strTrim(const std::string& s);

std::vector<std::string> strExplode(std::string const& s, char delim);

void standardizePathSep(std::string& path);
void standardizePathSep(std::wstring& path);

void ensurePathEndwithSep(std::string& path, std::string sep);
void ensurePathEndwithSep(std::wstring& path, std::wstring sep);

std::wstring strToWideStr(const std::string& s);
std::string wideStrToStr(const std::wstring& ws);

std::string formatVersion(std::string ver, bool bPrefix, bool bSuffix, int ver_type);
bool isCommercialVersion();
std::string getPandasVersion(bool bPrefix = true, bool bSuffix = true);

bool isDoubleByteCharacter(unsigned char high, unsigned char low);
bool isEscapeSequence(const char* start_p);

void isaAvailableHotfix();

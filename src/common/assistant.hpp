// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string> // std::string
#include <time.h> // struct tm
#include <memory> // std::shared_ptr
#include <vector> // std::vector

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

bool strContain(std::vector<std::string> contain, std::string str, bool bCaseSensitive = false);
bool strContain(std::string contain, std::string str, bool bCaseSensitive = false);

std::string strLeftTrim(const std::string& s);
std::string strRightTrim(const std::string& s);
std::string strTrim(const std::string& s);

std::vector<std::string> strExplode(std::string const& s, char delim);

std::string strLower(const std::string& s);
std::string strUpper(const std::string& s);

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

std::string getPandasVersion(bool bPrefix = true, bool bSuffix = true);

std::string getSystemLanguage();

#ifdef _WIN32
std::string getFileVersion(std::string filename);
#endif // _WIN32

// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "assistant.hpp"

#include <stdlib.h>
#include <locale.h> // setlocale
#include <sys/stat.h> // _stat
#include <errno.h> // errno, ENOENT, EEXIST
#include <wchar.h> // vswprintf
#include <sstream> // std::istringstream
#include <iostream>
#include <fstream>
#include <cctype> // std::tolower std::toupper std::isspace
#include <stdio.h> // __isa_available

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> // DIR remove
#include <unistd.h> // rmdir
#include <linux/limits.h> // PATH_MAX
#endif // _WIN32

#include <boost/regex.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "strlib.hpp"
#include "db.hpp"
#include "showmsg.hpp"
#include "utils.hpp" // check_filepath

#include "processmutex.hpp"

extern "C" int __isa_available;

//************************************
// Method:      systemPause
// Description: 暂停并要求用户按任意键继续
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2022/05/07 16:43
//************************************
void systemPause() {
#ifdef _WIN32
	system("pause");
#else
	int _unused = system("read");
#endif // _WIN32
}

//************************************
// Method:      isRegexMatched
// Description: 
// Parameter:   const std::string& patterns
// Parameter:   const std::string& content
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/10/13 15:59
//************************************
bool isRegexMatched(const std::string& content, const std::string& patterns) {
	try
	{
		boost::regex re(patterns, boost::regex::icase);
		boost::smatch match_result;

		if (!boost::regex_search(content, match_result, re)) {
			return false;
		}
	}
	catch (const boost::regex_error& e)
	{
		ShowWarning("%s throw regex_error : %s\n", __func__, e.what());
		return false;
	}
	return true;
}

//************************************
// Method:      regexExtract
// Description: 使用正则表达式提取指定分组的内容
// Parameter:   const std::string & patterns
// Parameter:   const std::string & content
// Parameter:   size_t extract_group	分组编号
// Parameter:   bool icase				匹配时是否区分大小写
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2022/05/04 16:53
//************************************ 
std::string regexExtract(const std::string& content, const std::string& patterns, size_t extract_group, bool icase) {
	try
	{
		boost::regex re(patterns, icase ? boost::regex::icase : boost::regex::normal);
		boost::smatch match_result;

		if (!boost::regex_search(content, match_result, re)) {
			return "";
		}

		if (extract_group >= match_result.size()) {
			return "";
		}

		return match_result[extract_group];
	}
	catch (const boost::regex_error& e)
	{
		ShowWarning("%s throw regex_error : %s\n", __func__, e.what());
		return "";
	}

	return "";
}

//************************************
// Method:      deployImportDirectory
// Description: 将指定的 import-tmpl 部署到对应的 import 位置
// Parameter:   std::string fromImportDir
// Parameter:   std::string toImportDir
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:34
//************************************
bool deployImportDirectory(std::string fromImportDir, std::string toImportDir) {

	std::string workDirectory;
	if (!getExecuteFileDirectory(workDirectory)) {
		return false;
	}

	std::string fromDirectory = workDirectory + fromImportDir;
	std::string toDirectory = workDirectory + toImportDir;

	ensurePathEndwithSep(fromDirectory, PATH_SEPARATOR);
	ensurePathEndwithSep(toDirectory, PATH_SEPARATOR);

	standardizePathSep(fromDirectory);
	standardizePathSep(toDirectory);

	if (!isDirectoryExists(toDirectory)) {
		if (!copyDirectory(fromDirectory, toDirectory)) {
			ShowWarning("Could not copy %s to %s.\n", fromImportDir.c_str(), toImportDir.c_str());
			return false;
		}
	}

#ifdef _WIN32
	WIN32_FIND_DATA fdFileData;
	HANDLE hSearch = NULL;
	std::string szPathWildcard = fromDirectory + "*.*";

	hSearch = FindFirstFile(szPathWildcard.c_str(), &fdFileData);
	if (hSearch == INVALID_HANDLE_VALUE) return false;

	do {
		if (!lstrcmp(fdFileData.cFileName, "..")) continue;
		if (!lstrcmp(fdFileData.cFileName, ".")) continue;

		std::string fromFullPath = fromDirectory + fdFileData.cFileName;
		std::string toFullPath = toDirectory + fdFileData.cFileName;

		if (check_filepath(fromFullPath.c_str()) != 2) continue;
		if (check_filepath(toFullPath.c_str()) != 3) continue;

		std::string displayTargetPath = toImportDir;
		ensurePathEndwithSep(displayTargetPath, PATH_SEPARATOR);
		displayTargetPath = displayTargetPath + fdFileData.cFileName;
		standardizePathSep(displayTargetPath);

		if (!(fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (!copyFile(fromFullPath.c_str(), toFullPath.c_str())) {
				ShowWarning("Deploy %s is failed.\n", displayTargetPath.c_str());
			}
			else {
				ShowInfo("Deploy %s is successful.\n", displayTargetPath.c_str());
			}
		}
	} while (FindNextFile(hSearch, &fdFileData));

	FindClose(hSearch);
#else
	DIR* dirHandle = opendir(fromDirectory.c_str());
	if (!dirHandle) return false;

	struct dirent* dt = NULL;
	while ((dt = readdir(dirHandle))) {
		if (strcmp(dt->d_name, "..") == 0) continue;
		if (strcmp(dt->d_name, ".") == 0) continue;

		std::string fromFullPath = fromDirectory + dt->d_name;
		std::string toFullPath = toDirectory + dt->d_name;

		// 注意: Linux 下 check_filepath 返回值和 Windows 含义不同
		if (check_filepath(fromFullPath.c_str()) != 2) continue;
		if (check_filepath(toFullPath.c_str()) != 0) continue;

		std::string displayTargetPath = toImportDir;
		ensurePathEndwithSep(displayTargetPath, PATH_SEPARATOR);
		displayTargetPath = displayTargetPath + dt->d_name;
		standardizePathSep(displayTargetPath);

		struct stat st = { 0 };
		stat(fromFullPath.c_str(), &st);
		if (!S_ISDIR(st.st_mode)) {
			if (!copyFile(fromFullPath.c_str(), toFullPath.c_str())) {
				ShowWarning("Deploy %s is failed.\n", displayTargetPath.c_str());
			}
			else {
				ShowInfo("Deploy %s is successful.\n", displayTargetPath.c_str());
			}
		}
	}
	closedir(dirHandle);
#endif // _WIN32

	// 若不存在 src 目录 (用户的生产环境), 那么删除部署的来源目录 (import-tmpl)
	if (!isDirectoryExists(std::string(workDirectory + "src"))) {
		deleteDirectory(fromDirectory);
	}

	return true;
}

//************************************
// Method:      deployImportDirectories
// Description: 部署全部已知的 import 文件夹, 确保它们都存在
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:33
//************************************
void deployImportDirectories() {
	ProcessMutex mutex = ProcessMutex("PANDAS_IMPORT_DEPLOY_MUTEX");

	mutex.lock();

	const struct _import_data {
		std::string import_from;
		std::string import_to;
	} import_data[] = {
		{ "conf/import-tmpl", "conf/import" },
		{ "conf/msg_conf/import-tmpl", "conf/msg_conf/import" },
		{ "db/import-tmpl", "db/import" }
	};

	for (size_t i = 0; i < ARRAYLENGTH(import_data); i++) {
		deployImportDirectory(import_data[i].import_from, import_data[i].import_to);
	}

	mutex.unlock();
}

//************************************
// Method:      getExecuteFilepath
// Description: 获取当前进程的文件路径 (跨平台支持)
// Parameter:   std::string & outFilepath
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/14 23:55
//************************************
bool getExecuteFilepath(std::string& outFilepath) {
#ifdef _WIN32
	DWORD dwError = 0;
	DWORD dwResult = 0;
	DWORD dwSize = MAX_PATH;

	SetLastError(dwError);
	while (dwSize <= 10240) {
		outFilepath.resize(dwSize);

		dwResult = GetModuleFileName(NULL, &outFilepath[0], dwSize);
		dwError = GetLastError();

		if (0 == dwResult) {
			outFilepath = "";
			return false;
		}

		if (ERROR_INSUFFICIENT_BUFFER == dwError) {
			dwSize *= 2;
			SetLastError(dwError);
			continue;
		}

		outFilepath.resize(dwResult);
		return true;
	}

	return false;
#else
	outFilepath.resize(PATH_MAX);
	if (readlink("/proc/self/exe", &outFilepath[0], PATH_MAX) >= 0) {
		return true;
	}
	
	return false;
#endif // _WIN32
}

//************************************
// Method:      getExecuteFileDirectory
// Description: 获取当前进程的所在目录 (返回的路径结尾包含路径分隔符)
// Parameter:   std::string & outFileDirectory
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/14 23:55
//************************************
bool getExecuteFileDirectory(std::string& outFileDirectory) {
	std::string filePath;
	if (!getExecuteFilepath(filePath)) {
		outFileDirectory = "";
		return false;
	}

	std::size_t dwPos = filePath.rfind(PATH_SEPARATOR);
	if (dwPos == std::string::npos) {
		outFileDirectory = "";
		return false;
	}

	outFileDirectory = filePath.substr(0, dwPos);
	ensurePathEndwithSep(outFileDirectory, PATH_SEPARATOR);
	return true;
}

//************************************
// Method:      isDirectoryExists
// Description: 判断目录是否存在 (跨平台支持)
// Parameter:   const std::string & path
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/4/5 18:55
//************************************
bool isDirectoryExists(const std::string& path) {
	try
	{
		boost::filesystem::path dirpath(path);
		dirpath = dirpath.generic_path();
		return boost::filesystem::is_directory(dirpath);
	}
	catch (const boost::filesystem::filesystem_error &e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}

//************************************
// Method:      makeDirectories
// Description: 创建多层目录 (跨平台支持)
// Parameter:   const std::string & dirpath 目录路径
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/4/5 18:55
//************************************
bool makeDirectories(const std::string& dirpath) {
	try
	{
		boost::filesystem::path path(dirpath);
		path = path.generic_path();
		if (isDirectoryExists(dirpath) || isFileExists(dirpath)) return true;
		return boost::filesystem::create_directories(path);
	}
	catch (const boost::filesystem::filesystem_error &e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}

//************************************
// Method:      ensureDirectories
// Description: 确保给定的文件路径其对应的目录存在 (跨平台支持)
// Parameter:   const std::string & filepath 文件路径
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/1/12 00:02
//************************************
bool ensureDirectories(const std::string& filepath) {
	try
	{
		boost::filesystem::path path(filepath);
		path = path.generic_path().parent_path();
		return makeDirectories(path.string());
	}
	catch (const boost::filesystem::filesystem_error& e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}

//************************************
// Method:      deleteDirectory
// Description: 删除指定的目录及其全部内容 (跨平台支持)
// Parameter:   std::string path
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/15 21:54
//************************************
bool deleteDirectory(std::string path) {
	try
	{
		boost::filesystem::path dirpath(path);
		dirpath = dirpath.generic_path();
		boost::filesystem::remove_all(dirpath);
		return true;
	}
	catch (const boost::filesystem::filesystem_error &e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}

//************************************
// Method:      copyDirectory
// Description: 将指定的目录, 复制到另外一个位置 (跨平台支持)
// Parameter:   std::string fromPath
// Parameter:   std::string toPath
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/16 08:21
//************************************
bool copyDirectory(const boost::filesystem::path &from, const boost::filesystem::path &to) {
	try
	{
		if (boost::filesystem::exists(to)) {
			throw std::runtime_error("The path "+ to.generic_string() + " is already exists.");
		}

		if (boost::filesystem::is_directory(from)) {
			boost::filesystem::create_directories(to);
			for (boost::filesystem::directory_entry& item : boost::filesystem::directory_iterator(from)) {
				copyDirectory(item.path(), to / item.path().filename());
			}
		}
		else if (boost::filesystem::is_regular_file(from)) {
			boost::filesystem::copy(from, to);
		}
		else {
			throw std::runtime_error("The path " + to.generic_string() + " is not directory or file.");
		}
		return true;
	}
	catch (const std::runtime_error &e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}


//************************************
// Method:      isFileExists
// Description: 判断文件是否存在 (跨平台支持)
// Parameter:   const std::string & path
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/4/5 18:55
//************************************
bool isFileExists(const std::string& path) {
	try
	{
		boost::filesystem::path filepath(path);
		filepath = filepath.generic_path();
		return boost::filesystem::is_regular_file(filepath);
	}
	catch (const boost::filesystem::filesystem_error& e)
	{
		ShowWarning("%s: %s\n", __func__, e.what());
		return false;
	}
}

//************************************
// Method:      copyFile
// Description: 复制文件到另外一个位置, 若存在则覆盖 (跨平台支持)
// Parameter:   const std::string & from
// Parameter:   const std::string & to
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/11/07 12:28
//************************************
bool copyFile(const std::string& from, const std::string& to) {
	try
	{
		boost::filesystem::path frompath(from);
		frompath = frompath.generic_path();

		boost::filesystem::path topath(to);
		topath = topath.generic_path();

		boost::filesystem::copy_file(
			frompath, topath,
			boost::filesystem::copy_option::overwrite_if_exists
		);
		return true;
	}
	catch (const boost::filesystem::filesystem_error&)
	{
		return false;
	}
}

//************************************
// Method:      deleteFile
// Description: 删除指定的文件
// Access:      public 
// Parameter:   const std::string & path
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2022/03/12 19:02
//************************************ 
bool deleteFile(const std::string& path) {
	try
	{
		boost::filesystem::path filepath(path);
		filepath = filepath.generic_path();
		return boost::filesystem::remove(filepath);
	}
	catch (const boost::filesystem::filesystem_error&)
	{
		return false;
	}
}

//************************************
// Method:      strReplace
// Description: 用于对 std::string 进行全部替换操作
// Parameter:   std::string & str
// Parameter:   const std::string & from
// Parameter:   const std::string & to
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 18:55
//************************************
void strReplace(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//************************************
// Method:      strReplace
// Description: 用于对 std::wstring 进行全部替换操作
// Parameter:   std::wstring & str
// Parameter:   const std::wstring & from
// Parameter:   const std::wstring & to
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 18:55
//************************************
void strReplace(std::wstring& str, const std::wstring& from, const std::wstring& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//************************************
// Method:      strReplace
// Description: 用于对 char[] 字符串数组进行全部替换操作
// Access:      public 
// Parameter:   char * str
// Parameter:   const char * from
// Parameter:   const char * to
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/08/07 16:10
//************************************ 
void strReplace(char* str, const char* from, const char* to) {
	int len = strlen(str);
	int from_len = strlen(from), to_len = strlen(to);
	for (char* p = str; p = strstr(p, from); ++p) {
		if (from_len != to_len) // shift end as needed 
			memmove(p + to_len, p + from_len,
				len - (p - str) + to_len);
		memcpy(p, to, to_len);
	}
}

//************************************
// Method:      strContain
// Description: 判断 str 中是否包含 needle 数组中的任何一个字符串 (不区分大小写)
// Parameter:   std::vector<std::string> needle
// Parameter:   const std::string& str
// Returns:     bool 只要包含任何一个则返回 true, 都不包含则返回 false
// Author:      Sola丶小克(CairoLee)  2019/10/13 23:46
//************************************
bool strContain(std::vector<std::string> needle, const std::string& str) {
	for (auto it : needle) {
		if (strContain(it, str)) return true;
	}
	return false;
}

//************************************
// Method:      strContain
// Description: 判断 str 中是否包含 needle 字符串 (不区分大小写)
// Parameter:   std::string needle
// Parameter:   const std::string& str
// Returns:     bool 包含则返回 true, 不包含则返回 false
// Author:      Sola丶小克(CairoLee)  2019/10/13 23:46
//************************************
bool strContain(std::string needle, const std::string& str) {
	auto it = std::search(
		str.begin(), str.end(),
		needle.begin(), needle.end(),
		[](char ch1, char ch2) {
			return std::toupper(ch1) == std::toupper(ch2);
		}
	);
	return (it != str.end());
}

//************************************
// Method:      strTrim
// Description: 移除给定 std::string 字符串左右两侧的空白字符
// Parameter:   const std::string & s
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/10/13 23:46
//************************************
std::string strTrim(const std::string& s) {
	return boost::trim_copy(s);
}

//************************************
// Method:      strExplode
// Description: 将给定的 s 字符串按 delim 字符进行切割
// Parameter:   std::string const & s
// Parameter:   char delim 注意这只是一个字符, 而不是一个字符串
// Returns:     std::vector<std::string> 返回切割后的内容
// Author:      Sola丶小克(CairoLee)  2019/10/13 23:46
//************************************
std::vector<std::string> strExplode(std::string const& s, char delim) {
	std::vector<std::string> result;
	std::istringstream iss(s);
	for (std::string token; std::getline(iss, token, delim); ) {
		result.push_back(std::move(token));
	}
	if (s.back() == delim) {
		result.push_back("");
	}
	return result;
}

//************************************
// Method:      strIsNumber
// Description: 检查指定的字符串是不是一个数字
// Access:      public 
// Parameter:   const std::string & str
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2022/04/16 18:34
//************************************ 
bool strIsNumber(const std::string& str) {
	if (!str.length()) {
		return false;
	}

	for (char const& c : str) {
		if (std::isdigit(c) == 0) {
			return false;
		}
	}
	return true;
}

//************************************
// Method:      strEndWith
// Description: 用于判断 std::string 是否以另外一个 std::string 结尾
// Parameter:   std::string fullstring
// Parameter:   std::string ending
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/15 21:55
//************************************
bool strEndWith(std::string fullstring, std::string ending) {
	if (fullstring.length() >= ending.length()) {
		return (0 == fullstring.compare(
			fullstring.length() - ending.length(), ending.length(), ending
		));
	}
	return false;
}

//************************************
// Method:      strEndWith
// Description: 用于判断 std::wstring 是否以另外一个 std::wstring 结尾
// Parameter:   std::wstring fullstring
// Parameter:   std::wstring ending
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/15 21:56
//************************************
bool strEndWith(std::wstring fullstring, std::wstring ending) {
	if (fullstring.length() >= ending.length()) {
		return (0 == fullstring.compare(
			fullstring.length() - ending.length(), ending.length(), ending
		));
	}
	return false;
}

//************************************
// Method:      standardizePathSep
// Description:	将给带的路径中存在的 / 或 \ 转换成当前系统匹配的路径分隔符
//              只有涉及输出体验或代码在 WIN 平台上硬编码对 \ 的判断时, 才需要使用它
//              无论是什么平台的 API 函数, 应该都认可使用 / 作为目录分隔符
// Parameter:   std::string & path
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 08:24
//************************************
void standardizePathSep(std::string& path) {
	strReplace(path, "/", PATH_SEPARATOR);
	strReplace(path, "\\", PATH_SEPARATOR);
}

//************************************
// Method:      standardizePathSep
// Description:	将给带的路径中存在的 / 或 \ 转换成当前系统匹配的路径分隔符
//              只有涉及输出体验或代码在 WIN 平台上硬编码对 \ 的判断时, 才需要使用它
//              无论是什么平台的 API 函数, 应该都认可使用 / 作为目录分隔符
// Parameter:   std::wstring & path
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 08:24
//************************************
void standardizePathSep(std::wstring& path) {
	strReplace(path, L"/", WIDE_PATH_SEPARATOR);
	strReplace(path, L"\\", WIDE_PATH_SEPARATOR);
}

//************************************
// Method:      ensurePathEndwithSep
// Description: 确保指定的 std::string 路径使用 sep 结尾
// Parameter:   std::string & path
// Parameter:   const std::string & sep
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:34
//************************************
void ensurePathEndwithSep(std::string& path, const std::string& sep) {
	if (!(strEndWith(path, "\\") || strEndWith(path, "/"))) {
		path.append(sep);
	}
}

//************************************
// Method:      ensurePathEndwithSep
// Description: 确保指定的 std::wstring 路径使用 sep 结尾
// Parameter:   std::wstring & path
// Parameter:   const std::wstring & sep
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:35
//************************************
void ensurePathEndwithSep(std::wstring& path, const std::wstring& sep) {
	if (!(strEndWith(path, L"\\") || strEndWith(path, L"/"))) {
		path.append(sep);
	}
}

//************************************
// Method:      strToWideStr
// Description: 将 std::string 转换成 std::wstring
// Parameter:   const std::string & s
// Returns:     std::wstring
// Author:      Sola丶小克(CairoLee)  2019/11/05 15:32
//************************************
std::wstring strToWideStr(const std::string& s) {
	return boost::locale::conv::utf_to_utf<wchar_t>(s);
}

//************************************
// Method:      wideStrToStr
// Description: 将 std::wstring 转换成 std::string
// Parameter:   const std::wstring & ws
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/11/05 15:32
//************************************
std::string wideStrToStr(const std::wstring& ws) {
	return boost::locale::conv::utf_to_utf<char>(ws);
}

//************************************
// Method:      formatVersion
// Description: 对版本号进行格式化的处理函数
// Parameter:   std::string ver 四段式版本号
// Parameter:   bool bPrefix 是否携带 v 前缀
// Parameter:   bool bSuffix 是否根据最后一段补充后缀
// Parameter:   int ver_type 版本类型 (0 - 社区版, 1 - 专业版)
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/11/04 23:20
//************************************
std::string formatVersion(std::string ver, bool bPrefix, bool bSuffix, int ver_type) {
	std::vector<std::string> split;
	boost::split(split, ver, boost::is_any_of("."));

	if (ver_type == 0) {
		std::string suffix = split[split.size() - 1] == "1" ? "-dev" : "";
		return ver = boost::str(
			boost::format("%1%%2%.%3%.%4%%5%") %
			(bPrefix ? "v" : "") % split[0] % split[1] % split[2] % (bSuffix ? suffix : "")
		);
	}
	else {
		std::string suffix = split[split.size() - 1];
		suffix = (suffix != "0" ? " Rev." + suffix : "");
		return ver = boost::str(
			boost::format("%1%%2%.%3%.%4%%5%") %
			(bPrefix ? "v" : "") % split[0] % split[1] % split[2] % (bSuffix ? suffix : "")
		);
	}
}

//************************************
// Method:      isCommercialVersion
// Description: 判断当前是否为专业版 (商业版)
// Access:      public 
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2022/01/28 01:16
//************************************ 
bool isCommercialVersion() {
#ifdef Pandas_Commercial_Version
	return true;
#else
	return false;
#endif // Pandas_Commercial_Version
}

//************************************
// Method:      getDefinedVersion
// Description: 读取宏定义中所预设的版本字符串
// Access:      public 
// Returns:     const char*
// Author:      Sola丶小克(CairoLee)  2022/02/08 17:00
//************************************ 
inline const char* getDefinedVersion() {
#ifdef Pandas_Commercial_Version
	return Pandas_Commercial_Version;
#else
	return Pandas_Version;
#endif // Pandas_Commercial_Version
}

#ifdef Pandas_Version
//************************************
// Method:      getPandasVersion
// Description: 用于获取 Pandas 的主程序版本号
// Parameter:   bool bPrefix
// Parameter:   bool bSuffix
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/11/04 23:20
//************************************
std::string getPandasVersion(bool bPrefix, bool bSuffix) {
#ifdef _WIN32
	std::string szDefaultVersion = formatVersion(getDefinedVersion(), bPrefix, bSuffix, isCommercialVersion() ? 1 : 0);

	// 获取当前的文件名
	char szModulePath[MAX_PATH] = { 0 };
	if (GetModuleFileName(NULL, szModulePath, MAX_PATH) == 0) {
		ShowWarning("%s: Could not get module file name, defaulting to '%s'\n", __func__, szDefaultVersion.c_str());
		return szDefaultVersion;
	}
	std::string filename = std::string(szModulePath);

	// 获取文件版本信息的结构体大小
	DWORD dwInfoSize = 0, dwHandle = 0;
	dwInfoSize = GetFileVersionInfoSize(filename.c_str(), &dwHandle);
	if (dwInfoSize == 0) {
		ShowWarning("%s: Could not get version info size, defaulting to '%s'\n", __func__, szDefaultVersion.c_str());
		return szDefaultVersion;
	}

	// 获取文件的版本号, 预期格式为: x.x.x.x
	void* pVersionInfo = new char[dwInfoSize];
	dwHandle = 0; // 根据 GetFileVersionInfoA 标准, 这里的 dwHandle 应确保为 0
	if (GetFileVersionInfo(filename.c_str(), dwHandle, dwInfoSize, pVersionInfo)) {
		void* lpBuffer = NULL;
		UINT nItemLength = 0;
		if (VerQueryValue(pVersionInfo, "\\", &lpBuffer, &nItemLength)) {
			VS_FIXEDFILEINFO *pFileInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			std::string szVersionFormat = (isCommercialVersion() ? "%1$04d.%2$02d.%3$02d.%4%" : "%1%.%2%.%3%.%4%");

			std::string sFileVersion = boost::str(boost::format(szVersionFormat) %
				HIWORD(pFileInfo->dwFileVersionMS) % LOWORD(pFileInfo->dwProductVersionMS) % 
				HIWORD(pFileInfo->dwProductVersionLS) % LOWORD(pFileInfo->dwProductVersionLS)
			);

			delete[] pVersionInfo;

			return formatVersion(sFileVersion, bPrefix, bSuffix, isCommercialVersion() ? 1 : 0);
		}
	}

	delete[] pVersionInfo;
	ShowWarning("%s: Could not get file version, defaulting to '%s'\n", __func__, szDefaultVersion.c_str());
	return szDefaultVersion;
#else
	return formatVersion(getDefinedVersion(), bPrefix, bSuffix, isCommercialVersion() ? 1 : 0);
#endif // _WIN32
}
#endif // Pandas_Version

//************************************
// Method:      isGBKCharacter
// Description: 判断一个给定的高低位组合是否在 GBK 的双字节汉字编码区间内
// Parameter:   unsigned char high
// Parameter:   unsigned char low
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/12/02 23:17
//************************************
bool isGBKCharacter(unsigned char high, unsigned char low) {
	// 判断基于 GBK 编码的双字节字符规则
	// https://www.qqxiuzi.cn/zh/hanzi-gbk-bianma.php
	// 由于 GBK 兼容 GB2312, 是 GB2312 的超集, 所以这里不再单独对 GB2312 区间做判断
	// 
	// GBK 亦采用双字节表示, 总体编码范围为 0x8140-0xFEFE.
	// 高位字节在 0x81-0xFE 之间, 低位字节在 0x40-0xFE 之间, 剔除 xx7F 一条线

	// 先判断低位是否不为 0x7f 且在 0x40-0xFE 区间内
	if (low != 0 && low != 0x7f && low >= 0x40 && low <= 0xfe) {
		// 若低位成立, 再判断高位是否在 0x81-0xFE 区间内
		if (high != 0 && high >= 0x81 && high <= 0xFE) {
			// 若都成立, 则表示在 GBK 编码情况下, 当前 p 指向的反斜杠是一个独立字符
			// 而不是某个 GBK 编码中双字节汉字中的一部分
			return true;
		}
	}

	return false;
}

//************************************
// Method:      isBIG5Character
// Description: 判断一个给定的高低位组合是否在 BIG5 的双字节汉字编码区间内
// Parameter:   unsigned char high
// Parameter:   unsigned char low
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/12/02 23:17
//************************************
bool isBIG5Character(unsigned char high, unsigned char low) {
	// 先判断基于 BIG5 编码的双字节字符规则
	// https://www.qqxiuzi.cn/zh/hanzi-big5-bianma.php
	// BIG5采用双字节编码, 使用两个字节来表示一个字符
	// 高位字节使用了0x81-0xFE, 低位字节使用了0x40-0x7E, 及0xA1-0xFE
	// 
	// 这里判断 p[-1] 上一个字节是否在上述的低位区间,
	// 若是则再进一步判断 p[-2] 是否在高位区间, 都成立, 那么说明当前的反斜杠是独立字符

	// 先判断低位是否在 0x40-0x7E, 及 0xA1-0xFE 区间内
	if (low != 0 && ((low >= 0x40 && low <= 0x7e) || (low >= 0xa1 && low <= 0xfe))) {
		// 若低位成立, 再判断高位是否在 0x81-0xFE 区间内
		if (high != 0 && high >= 0x81 && high <= 0xFE) {
			// 若都成立, 则表示在 BIG5 编码情况下, 当前 p 指向的反斜杠是一个独立字符
			// 而不是某个 BIG5 编码中双字节汉字中的一部分
			//
			// 注意: 部分 GBK 字符的编码区间和 BIG5 是重叠的, 部分 GBK 的双字节字符可能因此从这里返回
			// 暂时还没发现有判断脚本文件的编码来确认应该走哪个编码判断分支的必要
			return true;
		}
	}

	return false;
}

//************************************
// Method:      isDoubleByteCharacter
// Description: 判断一个给定的高低位组合是否为一个合法的双字节字符
// Parameter:   unsigned char high
// Parameter:   unsigned char low
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/12/04 23:45
//************************************
bool isDoubleByteCharacter(unsigned char high, unsigned char low) {
	return (isGBKCharacter(high, low) || isBIG5Character(high, low));
}

//************************************
// Method:      isEscapeSequence
// Description: 给定的 p 指针所指向的 p[0] 字节以及 p[1] 是否构成一个转义序列
// Parameter:   const char * start_p
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/12/04 23:52
//************************************
bool isEscapeSequence(const char* start_p) {
	char buf[8] = { 0 };
	size_t len = skip_escaped_c(start_p) - start_p;
	if (len != 2) return false;
	size_t n = sv_unescape_c(buf, start_p, len);
	return (n == 1);
}

//************************************
// Method:      convertRFC2822toTimeStamp
// Description: 将符合 RFC2822 标准的时间字符串转换成时间戳
// Access:      public 
// Parameter:   std::string strRFC822Date
//				例如: Sun, 23 Jan 2022 05:03:50 GMT
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2022/01/23 13:17
//************************************ 
int convertRFC2822toTimeStamp(std::string strRFC822Date)
{
	boost::posix_time::ptime pt;
	boost::posix_time::time_input_facet* tif(
		new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S GMT")
	);

	std::stringstream ss;
	ss.imbue(std::locale(std::locale::classic(), tif));
	ss.str(strRFC822Date);
	ss >> pt;

	boost::posix_time::ptime time_t_begin(boost::gregorian::date(1970, 1, 1));
	boost::posix_time::time_duration timestamp = pt - time_t_begin;
	return (int)timestamp.total_seconds();
}

//************************************
// Method:      isaAvailableHotfix
// Description: 修正在支持 AVX512 指令的设备上
//              使用 std::unordered_map::reserve 会提示 Illegal instruction 的问题
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/11/17 23:28
//************************************
void isaAvailableHotfix() {
#if (_MSC_VER == 1923)	// Visual Studio 2019 version 16.3
	if (__isa_available > 5) {
		__isa_available = 5;
	}
#endif // (_MSC_VER == 1923)
}

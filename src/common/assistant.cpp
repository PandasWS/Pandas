// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "assistant.hpp"

#include <stdlib.h>
#include <locale.h> // setlocale
#include <sys/stat.h> // _stat
#include <errno.h> // errno, ENOENT, EEXIST
#include <wchar.h> // vswprintf

#include <iostream>
#include <fstream>

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

#include "strlib.hpp"
#include "db.hpp"
#include "showmsg.hpp"
#include "utils.hpp" // check_filepath

#ifdef _WIN32
	const std::string pathSep = "\\";
#else
	const std::string pathSep = "/";
#endif // _WIN32

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
	ensurePathEndwithSep(workDirectory, pathSep);

	std::string fromDirectory = workDirectory + fromImportDir;
	std::string toDirectory = workDirectory + toImportDir;

	ensurePathEndwithSep(fromDirectory, pathSep);
	ensurePathEndwithSep(toDirectory, pathSep);

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
		ensurePathEndwithSep(displayTargetPath, pathSep);
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
		ensurePathEndwithSep(displayTargetPath, pathSep);
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
}

//************************************
// Method:      getExecuteFilepath
// Description: 获取当前进程的文件路径 (暂时不支持 Windows 以外的系统平台)
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
// Description: 获取当前进程的所在目录
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

	std::size_t dwPos = filePath.rfind(pathSep);
	if (dwPos == std::string::npos) {
		outFileDirectory = "";
		return false;
	}

	outFileDirectory = filePath.substr(0, dwPos);
	return true;
}

//************************************
// Method:		isDirectoryExistss
// Description:	判断目录是否存在 (跨平台支持)
// Parameter:	const std::string & path
// Returns:		bool
//************************************
bool isDirectoryExists(const std::string& path) {
#ifdef _WIN32
	struct _stat info = { 0 };
	if (_stat(path.c_str(), &info) != 0) {
		return false;
	}
	return (info.st_mode & _S_IFDIR) != 0;
#else 
	struct stat info = { 0 };
	if (stat(path.c_str(), &info) != 0) {
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
#endif // _WIN32
}

//************************************
// Method:		makeDirectories
// Description:	创建多层目录 (跨平台支持)
// Parameter:	const std::string & path
// Returns:		bool
//************************************
bool makeDirectories(const std::string& path) {
#ifdef _WIN32
	int ret = _mkdir(path.c_str());
#else
	mode_t mode = 0755;
	int ret = mkdir(path.c_str(), mode);
#endif // _WIN32

	if (ret == 0)
		return true;

	switch (errno)
	{
	case ENOENT:
		// parent didn't exist, try to create it
	{
		size_t pos = path.find_last_of('/');
		if (pos == std::string::npos)
#ifdef _WIN32
			pos = path.find_last_of('\\');
		if (pos == std::string::npos)
#endif // _WIN32
			return false;
		if (!makeDirectories(path.substr(0, pos)))
			return false;
	}
	// now, try to create again
#ifdef _WIN32
	return 0 == _mkdir(path.c_str());
#else 
	return 0 == mkdir(path.c_str(), mode);
#endif // _WIN32

	case EEXIST:
		// done!
		return isDirectoryExists(path);

	default:
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
	ensurePathEndwithSep(path, pathSep);
#ifdef _WIN32
	WIN32_FIND_DATA fdFileData;
	HANDLE hSearch = NULL;
	std::string szPathWildcard, szFullPath;

	szPathWildcard = path + "*.*";
	hSearch = FindFirstFile(szPathWildcard.c_str(), &fdFileData);
	if (hSearch == INVALID_HANDLE_VALUE) return true;

	do {
		if (!lstrcmp(fdFileData.cFileName, "..")) continue;
		if (!lstrcmp(fdFileData.cFileName, ".")) continue;

		szFullPath = path + fdFileData.cFileName;
		if (fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			deleteDirectory(szFullPath);
		}
		else {
			DeleteFile(szFullPath.c_str());
		}
	} while (FindNextFile(hSearch, &fdFileData));
	FindClose(hSearch);

	return (RemoveDirectory(path.c_str()) != 0);
#else
	DIR* dirHandle = opendir(path.c_str());
	if (!dirHandle) return false;

	struct dirent* dt = NULL;
	while ((dt = readdir(dirHandle))) {
		if (strcmp(dt->d_name, "..") == 0) continue;
		if (strcmp(dt->d_name, ".") == 0) continue;

		struct stat st = { 0 };
		std::string enum_path = path + dt->d_name;
		stat(enum_path.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			deleteDirectory(enum_path);
		else
			remove(enum_path.c_str());
	}
	closedir(dirHandle);
	return (rmdir(path.c_str()) == 0 ? true : false);
#endif // _WIN32
}

//************************************
// Method:      copyDirectory
// Description: 将指定的目录, 复制到另外一个位置 (跨平台支持)
// Parameter:   std::string fromPath
// Parameter:   std::string toPath
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/16 08:21
//************************************
bool copyDirectory(std::string fromPath, std::string toPath) {
	ensurePathEndwithSep(fromPath, pathSep);
	ensurePathEndwithSep(toPath, pathSep);

	if (!isDirectoryExists(fromPath) || fromPath == toPath) {
		return false;
	}

	if (!isDirectoryExists(toPath) && !makeDirectories(toPath)) {
		return false;
	}

#ifdef _WIN32
	WIN32_FIND_DATA fdFileData;
	HANDLE hSearch = NULL;
	std::string szPathWildcard, szFullPath, szDstPath;

	szPathWildcard = fromPath + "*.*";
	hSearch = FindFirstFile(szPathWildcard.c_str(), &fdFileData);
	if (hSearch == INVALID_HANDLE_VALUE) return true;

	do {
		if (!lstrcmp(fdFileData.cFileName, "..")) continue;
		if (!lstrcmp(fdFileData.cFileName, ".")) continue;

		szFullPath = fromPath + fdFileData.cFileName;
		szDstPath = toPath + fdFileData.cFileName;

		if (fdFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!copyDirectory(szFullPath, szDstPath)) {
				FindClose(hSearch);
				return false;
			}
		}
		else {
			if (!CopyFile(szFullPath.c_str(), szDstPath.c_str(), FALSE)) {
				FindClose(hSearch);
				return false;
			}
		}
	} while (FindNextFile(hSearch, &fdFileData));
	FindClose(hSearch);
#else
	DIR* dirHandle = opendir(fromPath.c_str());
	if (!dirHandle) {
		return false;
	}

	struct dirent* dt = NULL;
	while ((dt = readdir(dirHandle))) {
		if (strcmp(dt->d_name, "..") == 0) continue;
		if (strcmp(dt->d_name, ".") == 0) continue;

		struct stat st = { 0 };
		std::string enum_frompath = fromPath + dt->d_name;
		std::string enum_topath = toPath + dt->d_name;

		stat(enum_frompath.c_str(), &st);
		if (S_ISDIR(st.st_mode)) {
			if (!copyDirectory(enum_frompath, enum_topath)) {
				closedir(dirHandle);
				return false;
			}
		}
		else {
			if (!copyFile(enum_frompath, enum_topath)) {
				closedir(dirHandle);
				return false;
			}
		}
	}
	closedir(dirHandle);
#endif // _WIN32

	return true;
}


//************************************
// Method:      copyFile
// Description: 复制文件到另外一个位置 (跨平台支持)
// Parameter:   std::string fromPath
// Parameter:   std::string toPath
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/09/16 12:47
//************************************
bool copyFile(std::string fromPath, std::string toPath) {
	if (check_filepath(fromPath.c_str()) != 2) {
		return false;
	}
#ifdef _WIN32
	return (CopyFile(fromPath.c_str(), toPath.c_str(), false) != 0);
#else
	try {
		std::ifstream fromStream(fromPath.c_str(), std::ios::binary);
		std::remove(toPath.c_str());
		std::ofstream toStream(toPath.c_str(), std::ios::binary);
		
		if (!toStream) {
			ShowWarning("Could not create output file: %s\n", toPath.c_str());
			return false;
		}
		
		toStream << fromStream.rdbuf();
		return true;
	}
	catch (...) {
		return false;
	}
#endif // _WIN32
}


//************************************
// Method:		strReplace
// Description:	用于对 std::string 进行全部替换操作
// Parameter:	std::string & str
// Parameter:	const std::string & from
// Parameter:	const std::string & to
// Returns:		void
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
// Method:		strReplace
// Description:	用于对 std::wstring 进行全部替换操作
// Parameter:	std::wstring & str
// Parameter:	const std::wstring & from
// Parameter:	const std::wstring & to
// Returns:		void
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
#ifdef _WIN32
	char pathsep[] = "\\";
#else
	char pathsep[] = "/";
#endif // _WIN32
	strReplace(path, "/", pathsep);
	strReplace(path, "\\", pathsep);
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
#ifdef _WIN32
	wchar_t pathsep[] = L"\\";
#else
	wchar_t pathsep[] = L"/";
#endif // _WIN32
	strReplace(path, L"/", pathsep);
	strReplace(path, L"\\", pathsep);
}

//************************************
// Method:      ensurePathEndwithSep
// Description: 确保指定的 std::string 路径使用 sep 结尾
// Parameter:   std::string & path
// Parameter:   std::string sep
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:34
//************************************
void ensurePathEndwithSep(std::string& path, std::string sep) {
	if (!(strEndWith(path, "\\") || strEndWith(path, "/"))) {
		path.append(sep);
	}
}

//************************************
// Method:      ensurePathEndwithSep
// Description: 确保指定的 std::wstring 路径使用 sep 结尾
// Parameter:   std::wstring & path
// Parameter:   std::wstring sep
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/09/16 07:35
//************************************
void ensurePathEndwithSep(std::wstring& path, std::wstring sep) {
	if (!(strEndWith(path, L"\\") || strEndWith(path, L"/"))) {
		path.append(sep);
	}
}

//************************************
// Method:		string2wstring
// Description:	将 std::string 转换成 std::wstring
//              https://blog.csdn.net/CYYTU/article/details/78616132
// Parameter:	const std::string & s
// Returns:		std::wstring
//************************************
std::wstring string2wstring(const std::string& s) {
#ifdef _WIN32
	setlocale(LC_ALL, "chs");
#else  
	setlocale(LC_ALL, "zh_CN.gbk");
#endif // _WIN32
	const char* _Source = s.c_str();
	size_t _Dsize = s.size() + 1;
	wchar_t *_Dest = new wchar_t[_Dsize];
	wmemset(_Dest, 0, _Dsize);
	mbstowcs(_Dest, _Source, _Dsize);
	std::wstring result = _Dest;
	delete[]_Dest;
	setlocale(LC_ALL, "C");
	return result;
}

//************************************
// Method:		wstring2string
// Description:	将 std::wstring 转换成 std::string
//              https://blog.csdn.net/CYYTU/article/details/78616132
// Parameter:	const std::wstring & ws
// Returns:		std::string
//************************************
std::string wstring2string(const std::wstring& ws) {
	std::string curLocale = setlocale(LC_ALL, NULL);
#ifdef _WIN32
	setlocale(LC_ALL, "chs");
#else  
	setlocale(LC_ALL, "zh_CN.gbk");
#endif // _WIN32
	const wchar_t* _Source = ws.c_str();
	size_t _Dsize = 2 * ws.size() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);
	std::string result = _Dest;
	delete[]_Dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::string 的格式化
// Parameter:	std::string & _str
// Parameter:	const char * _Format
// Parameter:	...
// Returns:		std::string &
//************************************
std::string & strFormat(std::string & _str, const char * _Format, ...) {
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
// Method:		strFormat
// Description:	用于进行 std::string 的格式化
// Parameter:	const char * _Format
// Parameter:	...
// Returns:		std::string
//************************************
std::string strFormat(const char* _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vsnprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	char* buf = (char*)aMalloc(count * sizeof(char));
	vsnprintf(buf, count, _Format, marker);
	std::string _str = std::string(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::wstring 的格式化
// Parameter:	std::wstring & _str
// Parameter:	const wchar_t * _Format
// Parameter:	...
// Returns:		std::wstring &
//************************************
std::wstring & strFormat(std::wstring & _str, const wchar_t * _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vswprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	wchar_t* buf = (wchar_t*)aMalloc(count * sizeof(wchar_t));
	vswprintf(buf, count, _Format, marker);
	_str = std::wstring(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		strFormat
// Description:	用于进行 std::wstring 的格式化
// Parameter:	const wchar_t * _Format
// Parameter:	...
// Returns:		std::wstring
//************************************
std::wstring strFormat(const wchar_t* _Format, ...) {
	va_list marker;

	va_start(marker, _Format);
	size_t count = vswprintf(NULL, 0, _Format, marker) + 1;
	va_end(marker);

	va_start(marker, _Format);
	wchar_t* buf = (wchar_t*)aMalloc(count * sizeof(wchar_t));
	vswprintf(buf, count, _Format, marker);
	std::wstring _str = std::wstring(buf, count);
	aFree(buf);
	va_end(marker);

	return _str;
}

//************************************
// Method:		getPandasVersion
// Description:	用于获取 Pandas 的主程序版本号
// Parameter:	bool without_vmark
// Returns:		std::string
//************************************
std::string getPandasVersion(bool without_vmark) {
#ifdef _WIN32
	return strFormat(
		(without_vmark ? "%s" : "v%s"), getFileVersion("").c_str()
	);
#else
	return strFormat(
		(without_vmark ? "%s" : "v%s"), std::string(Pandas_Version).c_str()
	);
#endif // _WIN32
}

//************************************
// Method:      getSystemLanguage
// Description: 获取当前系统的语言 (跨平台支持)
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2019/09/20 18:44
//************************************
std::string getSystemLanguage() {
#ifdef _WIN32
	LCID localeID = GetUserDefaultLCID();
	if (0x0804 == localeID) return "zh-cn";	// 简体中文
	if (0x0404 == localeID) return "zh-tw";	// 繁体中文
	return "unknow";	// 其他语言都判定为未知
#else
	char* lang = getenv("LANG");
	if (lang == nullptr) return "unknow";
	if (stricmp(lang, "zh_cn") > 0) return "zh-cn";	// 简体中文
	if (stricmp(lang, "zh_tw") > 0) return "zh-tw";	// 繁体中文
	return "unknow";	// 其他语言都判定为未知
#endif // _WIN32
}

#ifdef _WIN32
//************************************
// Method:		getFileVersion
// Description:	获取指定文件的文件版本号
// Parameter:	std::string filename
// Parameter:	bool bWithoutBuildNum
// Returns:		std::string
//************************************
std::string getFileVersion(std::string filename) {
	char szModulePath[MAX_PATH] = { 0 };

	if (filename.empty()) {
		if (GetModuleFileName(NULL, szModulePath, MAX_PATH) == 0) {
			ShowWarning("getFileVersion: Could not get module file name, defaulting to '%s'\n", Pandas_Version);
			return std::string(Pandas_Version);
		}
		filename = std::string(szModulePath);
	}

	DWORD dwInfoSize = 0, dwHandle = 0;

	dwInfoSize = GetFileVersionInfoSize(filename.c_str(), &dwHandle);
	if (dwInfoSize == 0) {
		ShowWarning("getFileVersion: Could not get version info size, defaulting to '%s'\n", Pandas_Version);
		return std::string(Pandas_Version);
	}

	void* pVersionInfo = new char[dwInfoSize];
	if (GetFileVersionInfo(filename.c_str(), dwHandle, dwInfoSize, pVersionInfo)) {
		void* lpBuffer = NULL;
		UINT nItemLength = 0;
		if (VerQueryValue(pVersionInfo, "\\", &lpBuffer, &nItemLength)) {
			VS_FIXEDFILEINFO *pFileInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			std::string sFileVersion;
			strFormat(sFileVersion, "%d.%d.%d",
				HIWORD(pFileInfo->dwFileVersionMS),
				LOWORD(pFileInfo->dwProductVersionMS),
				HIWORD(pFileInfo->dwProductVersionLS)
			);

			// 若从资源中读取版本号, 那么当第四段的版本号值为 1 时则代表这是一个开发者的版本
			// 返回的版本号字符上会自动追加上 -dev 后缀, 以便区分.
			if (LOWORD(pFileInfo->dwProductVersionLS) == 1) {
				strFormat(sFileVersion, "%s-dev", sFileVersion.c_str());
			}

			delete[] pVersionInfo;
			return sFileVersion;
		}
	}

	delete[] pVersionInfo;
	ShowWarning("getFileVersion: Could not get file version, defaulting to '%s'\n", Pandas_Version);
	return std::string(Pandas_Version);
}
#endif // _WIN32

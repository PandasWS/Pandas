// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "utf8.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef _WIN32

//************************************
// Method:		utf8_u2g
// Description:	将 UTF8 字符串转换为 ANSI 字符串
// Parameter:	const std::string & strUtf8	须为 UTF-8 编码的字符串
// Returns:		std::string
//************************************
std::string utf8_u2g(const std::string& strUtf8) {
	// UTF-8 转 Unicode
	int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
	wchar_t *strUnicode = new wchar_t[len];
	wmemset(strUnicode, 0, len);
	MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, strUnicode, len);

	// Unicode 转 GBK
	len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	char *strGbk = new char[len];
	memset(strGbk, 0, len);
	WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strGbk, len, NULL, NULL);

	std::string strTemp(strGbk);
	delete[] strUnicode;
	delete[] strGbk;
	strUnicode = nullptr;
	strGbk = nullptr;
	return strTemp;
}

//************************************
// Method:		utf8_g2u
// Description:	将 ANSI 字符串转换为 UTF8 字符串
// Parameter:	const std::string & strGbk 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
// Returns:		std::string
//************************************
std::string utf8_g2u(const std::string& strGbk) {
	// GBK 转 Unicode
	int len = MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, NULL, 0);
	wchar_t *strUnicode = new wchar_t[len];
	wmemset(strUnicode, 0, len);
	MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, strUnicode, len);

	// Unicode 转 UTF-8
	len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	char *strUtf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, strUtf8, len, NULL, NULL);

	std::string strTemp(strUtf8);
	delete[] strUnicode;
	delete[] strUtf8;
	strUnicode = nullptr;
	strUtf8 = nullptr;
	return strTemp;
}

#else

//************************************
// Method:		utf8_u2g
// Description:	将 UTF8 字符串转换为 ANSI 字符串
// Parameter:	const std::string & strUtf8 须为 UTF-8 编码的字符串
// Returns:		std::string
//************************************
std::string utf8_u2g(const std::string& strUtf8) {
	iconv_t c_pt = nullptr;
	char *str_input = nullptr, *p_str_input = nullptr;
	char *str_output = nullptr, *p_str_output = nullptr;
	size_t str_input_len = 0, str_output_len = 0;
	std::string strResult;

	if ((c_pt = iconv_open("GBK", "UTF-8")) == (iconv_t)-1) {
		ShowFatalError("utf8_u2g: %s was failed: %s\n", "iconv_open", strerror(errno));
		exit(EXIT_FAILURE);
	}

	str_input_len = strUtf8.size();
	str_input = new char[str_input_len + 1];
	memcpy(str_input, strUtf8.c_str(), str_input_len);
	p_str_input = str_input;	// 必须这样赋值一下, 不然在 Linux 下会提示段错误(Segmentation fault)

	str_output_len = str_input_len + 1;
	str_output = new char[str_output_len];
	memset(str_output, 0, str_output_len);
	p_str_output = str_output;	// 必须这样赋值一下, 不然在 Linux 下会提示段错误(Segmentation fault)

	if (iconv(c_pt, (char **)&p_str_input, &str_input_len, (char **)&p_str_output, &str_output_len) == (size_t)-1) {
		ShowFatalError("utf8_u2g: %s was failed: %s\n", "iconv", strerror(errno));
		ShowFatalError("utf8_u2g: the strUtf8 param value: %s", str_input);
		exit(EXIT_FAILURE);
	}

	strResult = str_output;
	iconv_close(c_pt);
	delete[] str_output;
	delete[] str_input;

	return strResult;
}

//************************************
// Method:		utf8_g2u
// Description:	将 ANSI 字符串转换为 UTF8 字符串
// Parameter:	const std::string & strGbk 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
// Returns:		std::string
//************************************
std::string utf8_g2u(const std::string& strGbk) {
	ShowFatalError("utf8_g2u: is not implement yet.\n");
	return std::string("");
}

#endif // _WIN32

//************************************
// Method:		utf8_isbom
// Description:	判断 FILE 对应的文件是否为 UTF8-BOM 编码
// Parameter:	FILE * _Stream
// Returns:		bool
//************************************
bool utf8_isbom(FILE *_Stream) {
	long curpos = 0;
	unsigned char buf[3] = { 0 };
	
	// 记录目前指针所在的位置
	curpos = ftell(_Stream);
	
	// 指针移动到开头, 读取前3个字节
	fseek(_Stream, 0, SEEK_SET);
	if (fread(buf, sizeof(unsigned char), 3, _Stream) == 3) {
		fseek(_Stream, curpos, SEEK_SET);
		return (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF);
	}
	fseek(_Stream, curpos, SEEK_SET);
	return false;
}

//************************************
// Method:		utf8_fopen
// Description:	能够在原有的 mode 中加入 "b" 的 fopen 函数
//              此文件的 utf8_isbom 只有在 "b" 模式下, 才能进行正确的 fseek 操作
// Parameter:	const char * _FileName
// Parameter:	const char * _Mode
// Returns:		FILE*
//************************************
FILE* utf8_fopen(const char* _FileName, const char* _Mode) {
	std::string szMode(_Mode);
	std::string::size_type i = szMode.find("b", 0);
	if (i == std::string::npos) {
		szMode += "b";
	}
	return fopen(_FileName, szMode.c_str());
}

//************************************
// Method:		utf8_fgets
// Description:	能够兼容读取 UTF8-BOM 编码文件的 fgets 函数
// Parameter:	char * _Buffer
// Parameter:	int _MaxCount
// Parameter:	FILE * _Stream
// Returns:		char*
//************************************
char* utf8_fgets(char *_Buffer, int _MaxCount, FILE *_Stream) {
	if (utf8_isbom(_Stream) == false) {
		// 若不是 UTF8-BOM, 那么直接透传 fgets 调用
		return fgets(_Buffer, _MaxCount, _Stream);
	}
	else {
		long curpos = 0;
		char *result = nullptr;
		char *buffer = new char[_MaxCount];
		std::string ansi_str, origin_str;

		// 若指针在文件的前3个字节, 那么将指针移动到前3个字节后面,
		// 避免后续进行 fgets 的时候读取到前3个字节
		curpos = ftell(_Stream);
		if (curpos <= 3) {
			fseek(_Stream, 3, SEEK_SET);
		}

		// 读取 _MaxCount 长度的内容并保存到 buf 中
		result = fgets(buffer, _MaxCount, _Stream);
		if (result) {
			origin_str = std::string(buffer);
			delete[] buffer;
			buffer = nullptr;

			// 若为注释行则直接返回空即可, fgets 每次调用读取一行文本
			// 所以在这里若发现是 // 开头的行则直接返回空, 应该不影响现有程序逻辑
			if (origin_str.rfind("//", 0) == 0) {
				memset(_Buffer, 0, _MaxCount);
				return result;
			}

			// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
			ansi_str = utf8_u2g(origin_str);
			memset(_Buffer, 0, _MaxCount);

			if (ansi_str.size() <= (size_t)_MaxCount) {
				// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
				memcpy(_Buffer, ansi_str.c_str(), ansi_str.size());
			}
			else {
				// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
				// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存

				// 不过退一万步, 目前的实现方法由于 _Buffer 是静态数组
				// 所以这里就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
				ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", "utf8_fgets", sizeof(_Buffer), ansi_str.size());
				fseek(_Stream, curpos, SEEK_SET);
				return fgets(_Buffer, _MaxCount, _Stream);
			}
		}

		delete[] buffer;
		buffer = nullptr;

		return result;
	}
}

//************************************
// Method:		utf8_fread
// Description:	能够兼容读取 UTF8-BOM 编码文件的 fread 函数
// Parameter:	void * _Buffer
// Parameter:	size_t _ElementSize
// Parameter:	size_t _ElementCount
// Parameter:	FILE * _Stream
// Returns:		size_t
//************************************
size_t utf8_fread(void *_Buffer, size_t _ElementSize, size_t _ElementCount, FILE *_Stream) {
	if (utf8_isbom(_Stream) == false || _ElementSize != 1) {
		// 若不是 UTF8-BOM 或者 _ElementSize 不等于 1, 那么直接透传 fread 调用
		return fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}
	else {
		long curpos = ftell(_Stream);
		size_t len = (_ElementSize * _ElementCount) + 1;
		size_t result = 0;
		char *buffer = new char[len];
		std::string ansi_str;

		// 若指针在文件的前3个字节, 那么将指针移动到前3个字节后面,
		// 避免后续进行 fread 的时候读取到前3个字节
		if (curpos <= 3) {
			fseek(_Stream, 3, SEEK_SET);

			// 重新分配缓冲区大小, 以及调整 _ElementCount 的大小
			delete[] buffer;
			if (_ElementCount >= 3) _ElementCount -= 3;
			if (len >= 3) len -=3;
			buffer = new char[len];
		}

		memset(buffer, 0, len);

		// 读取特定长度的内容并保存到 buf 中
		result = fread(buffer, _ElementSize, _ElementCount, _Stream);
		if (result) {
			// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
			ansi_str = utf8_u2g(std::string(buffer));
			memset(_Buffer, 0, len);
			delete[] buffer;
			buffer = nullptr;

			if (ansi_str.size() <= len) {
				// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
				memcpy(_Buffer, ansi_str.c_str(), ansi_str.size());
			}
			else {
				// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
				// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存

				// 不过退一万步, 目前的实现方法由于 _Buffer 是静态数组
				// 所以这里就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
				ShowWarning("%s: _Buffer size is only %lu but we need %lu, Could not realloc...\n", "utf8_fread", sizeof(_Buffer), ansi_str.size());

				fseek(_Stream, curpos, SEEK_SET);
				if (curpos <= 3) _ElementCount += 3;	// 之前修正过 _ElementCount 的大小, 现在这里需要改回去
				return fread(_Buffer, _ElementSize, _ElementCount, _Stream);
			}
		}

		delete[] buffer;
		buffer = nullptr;

		return result;
	}
}

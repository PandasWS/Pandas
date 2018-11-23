// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "utf8.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif

#ifdef _WIN32

// 将 UTF8 字符串转换为 ANSI 字符串
// 此处传入的 strUtf8 参数, 须为 UTF-8 编码的字符串
std::string Utf8ToGbk(const std::string& strUtf8)
{
	// UTF-8 转 Unicode
	int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
	wchar_t * strUnicode = new wchar_t[len];
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
	strUnicode = NULL;
	strGbk = NULL;
	return strTemp;
}

// 将 ANSI 字符串转换为 UTF8 字符串
// 此处传入的 strGbk 参数, 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
std::string GbkToUtf8(const std::string& strGbk)
{
	// GBK 转 Unicode
	int len = MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, NULL, 0);
	wchar_t *strUnicode = new wchar_t[len];
	wmemset(strUnicode, 0, len);
	MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, strUnicode, len);

	// Unicode 转 UTF-8
	len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	char * strUtf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, strUtf8, len, NULL, NULL);

	std::string strTemp(strUtf8);
	delete[] strUnicode;
	delete[] strUtf8;
	strUnicode = NULL;
	strUtf8 = NULL;
	return strTemp;
}

#else

// 将 UTF8 字符串转换为 ANSI 字符串
// 此处传入的 strUtf8 参数, 须为 UTF-8 编码的字符串
std::string Utf8ToGbk(const std::string& strUtf8) {
	iconv_t c_pt;
	char *str_input, *p_str_input, *str_output, *p_str_output;
	size_t str_input_len, str_output_len;
	std::string strResult("Utf8ToGbk Failed");

	if ((c_pt = iconv_open("GBK", "UTF-8")) == (iconv_t)-1) {
		ShowFatalError("Utf8ToGbk: %s return false. From %s to %s\n", "iconv_open", "GBK", "UTF-8");
		return strResult;
	}

	str_input_len = strUtf8.size();
	str_input = aStrdup(strUtf8.c_str());
	p_str_input = str_input;	// 必须这样赋值一下, 不然在 Linux 下会提示段错误(Segmentation fault)

	str_output_len = str_input_len * 2 + 1;
	str_output = (char *)aMalloc(sizeof(char) * str_output_len);
	memset(str_output, 0, str_output_len);
	p_str_output = str_output;	// 必须这样赋值一下, 不然在 Linux 下会提示段错误(Segmentation fault)

	if (iconv(c_pt, (char **)&p_str_input, &str_input_len, (char **)&p_str_output, &str_output_len) == (size_t)-1) {
		ShowFatalError("Utf8ToGbk: %s return false. From %s to %s\n", "iconv", "GBK", "UTF-8");
		return strResult;
	}

	iconv_close(c_pt);
	strResult = str_output;
	aFree(str_output);
	aFree(str_input);

	return strResult;
}

// 将 ANSI 字符串转换为 UTF8 字符串
// 此处传入的 strGbk 参数, 须为 GBK 或 BIG5 等 ANSI 类编码的字符串
std::string GbkToUtf8(const std::string& strGbk) {
	ShowFatalError("GbkToUtf8: is not implement yet.\n");
	return std::string("");
}

#endif // _WIN32

// 判断 FILE 对应的文件是否为 UTF8-BOM 编码
bool isUTF8withBOM(FILE *_Stream) {
	long curpos = 0;
	unsigned char buf[3] = { 0 };
	
	// 记录目前指针所在的位置
	curpos = ftell(_Stream);
	
	// 指针移动到开头, 读取前3个字节, 然后指针归位
	fseek(_Stream, 0, SEEK_SET);
	fread(buf, sizeof(unsigned char), 3, _Stream);
	fseek(_Stream, curpos, SEEK_SET);
	
	// 判断是否为 UTF8-BOM
	return (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF);
}

// 能够兼容读取 UTF8-BOM 编码文件的 fgets 函数
char* fgets_ex(char *_Buffer, int _MaxCount, FILE *_Stream) {
	if (isUTF8withBOM(_Stream) == false) {
		// 若不是 UTF8-BOM, 那么直接透传 fgets 调用
		return fgets(_Buffer, _MaxCount, _Stream);
	}
	else {
		long curpos = 0;
		char* result = NULL;
		char* buf = new char[_MaxCount];
		std::string ansi_str;

		// 若指针在文件的前3个字节, 那么将指针移动到前3个字节后面,
		// 避免后续进行 fgets 的时候读取到前3个字节
		curpos = ftell(_Stream);
		if (curpos <= 3) {
			fseek(_Stream, 3, SEEK_SET);
		}

		// 读取 _MaxCount 长度的内容并保存到 buf 中
		result = fgets(buf, _MaxCount, _Stream);
		if (result) {
			// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
			ansi_str = Utf8ToGbk(std::string(buf));
			memset(_Buffer, 0, _MaxCount);
			delete[] buf;
			buf = NULL;

			if (ansi_str.size() <= (size_t)_MaxCount) {
				// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
				memcpy(_Buffer, ansi_str.c_str(), ansi_str.size());
			}
			else {
				// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
				// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存

				// 不过退一万步, 目前的实现方法由于 _Buffer 是静态数组
				// 所以这里就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
				ShowWarning("fgets_ex: _Buffer size only %d but we need %d, Could not realloc...\n", sizeof(_Buffer), ansi_str.size());
				fseek(_Stream, curpos, SEEK_SET);
				return fgets(_Buffer, _MaxCount, _Stream);
			}
		}

		return result;
	}
}

// 能够兼容读取 UTF8-BOM 编码文件的 fread 函数
size_t fread_ex(void *_Buffer, size_t _ElementSize, size_t _ElementCount, FILE *_Stream) {
	if (isUTF8withBOM(_Stream) == false || _ElementSize != 1) {
		// 若不是 UTF8-BOM 或者 _ElementSize 不等于 1, 那么直接透传 fread 调用
		return fread(_Buffer, _ElementSize, _ElementCount, _Stream);
	}
	else {
		long curpos = ftell(_Stream);
		size_t len = (_ElementSize * _ElementCount) + 1;
		size_t result = 0;
		char* buf = new char[len];
		std::string ansi_str;

		// 若指针在文件的前3个字节, 那么将指针移动到前3个字节后面,
		// 避免后续进行 fread 的时候读取到前3个字节
		if (curpos <= 3) {
			fseek(_Stream, 3, SEEK_SET);

			// 重新分配缓冲区大小, 以及调整 _ElementCount 的大小
			delete[] buf;
			if (_ElementCount >= 3) _ElementCount -= 3;
			if (len >= 3) len -=3;
			buf = new char[len];
		}

		memset(buf, 0, len);

		// 读取特定长度的内容并保存到 buf 中
		result = fread(buf, _ElementSize, _ElementCount, _Stream);
		if (result) {
			// 将 UTF8 编码的字符转换成 ANSI 多字节字符集 (GBK 或者 BIG5)
			ansi_str = Utf8ToGbk(std::string(buf));
			memset(_Buffer, 0, len);
			delete[] buf;
			buf = NULL;

			if (ansi_str.size() <= len) {
				// 外部函数定义的 _Buffer 容量足够, 直接进行赋值
				memcpy(_Buffer, ansi_str.c_str(), ansi_str.size());
			}
			else {
				// 按道理来说, 此函数主要负责将 UTF8-BOM 编码的字符转成 ANSI
				// 转换结束按道理来说需要的空间会更小, 无需拓展空间, 所以理论上基本无需分配额外内存

				// 不过退一万步, 目前的实现方法由于 _Buffer 是静态数组
				// 所以这里就算 _Buffer 的空间不足, 其实也无法进行 realloc 操作...
				ShowWarning("fread_ex: _Buffer size only %d but we need %d, Could not realloc...\n", sizeof(_Buffer), ansi_str.size());

				fseek(_Stream, curpos, SEEK_SET);
				if (curpos <= 3) _ElementCount += 3;	// 之前修正过 _ElementCount 的大小, 现在这里需要改回去
				return fread(_Buffer, _ElementSize, _ElementCount, _Stream);
			}
		}

		return result;
	}
}

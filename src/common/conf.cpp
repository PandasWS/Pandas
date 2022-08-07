// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "conf.hpp"

#include "showmsg.hpp" // ShowError

#include "utf8.hpp"
#include <iostream>
#include <sstream>

#ifndef Pandas_Support_UTF8BOM_Files
int conf_read_file(config_t *config, const char *config_filename)
#else
// 重命名该函数以便重载一个签名完全一致的函数接管其处理逻辑
int conf_read_file_internal(config_t* config, const char* config_filename)
#endif // Pandas_Support_UTF8BOM_Files
{
	config_init(config);
	if (!config_read_file(config, config_filename)) {
		ShowError("%s:%d - %s\n", config_error_file(config),
		          config_error_line(config), config_error_text(config));
		config_destroy(config);
		return 1;
	}
	return 0;
}

#ifdef Pandas_Support_UTF8BOM_Files
//************************************
// Method:      conf_read_file
// Description: 能够对 UTF8-BOM 自动转码的 libconfig 读取函数
// Access:      public 
// Parameter:   config_t * config
// Parameter:   const char * config_filename
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2021/02/06 11:55
//************************************ 
int conf_read_file(config_t* config, const char* config_filename)
{
	std::string strFilename(config_filename);

#ifndef _WIN32
	// 在 Linux 环境下并且终端编码为 UTF8 的情况下,
	// 将需要检查的文件路径直接转码成 utf8 编码再进行读取, 否则会找不到文件
	if (PandasUtf8::systemEncoding == PandasUtf8::PANDAS_ENCODING_UTF8) {
		strFilename = PandasUtf8::consoleConvert(config_filename);
	}
#endif // _WIN32

	std::ifstream fin(strFilename.c_str(), std::ios::binary);
	if (!fin) {
		ShowWarning("conf_read_file: file is not found: %s\n", config_filename);
		return 1;
	}

	PandasUtf8::e_file_charsetmode mode = PandasUtf8::fmode(fin);
	if (mode != PandasUtf8::FILE_CHARSETMODE_UTF8_BOM && mode != PandasUtf8::FILE_CHARSETMODE_UTF8) {
		// 如果文件不是 UTF8-BOM 或者 UTF8 编码, 那么直接透传到原先的 conf_read_file 函数处理
		fin.close();
		return conf_read_file_internal(config, config_filename);
	}

	if (mode == PandasUtf8::FILE_CHARSETMODE_UTF8_BOM) {
		// 先跳过最开始的标记位, UTF8-BOM 是三个字节
		fin.seekg(3, std::ios::beg);
	}

	// 先读取来源文件的全部数据并保存到内存中
	std::stringstream buffer;
	buffer << fin.rdbuf();
	std::string contents(buffer.str());
	fin.close();

	// 执行对应的编码转换
	contents = PandasUtf8::utf8ToAnsi(contents);

	config_init(config);
	if (!config_read_string(config, contents.c_str())) {
		ShowError("%s:%d - %s\n", config_error_file(config),
			config_error_line(config), config_error_text(config));
		config_destroy(config);
		return 1;
	}
	return 0;
}
#endif // Pandas_Support_UTF8BOM_Files

//
// Functions to copy settings from libconfig/contrib
//
static void config_setting_copy_simple(config_setting_t *parent, const config_setting_t *src);
static void config_setting_copy_elem(config_setting_t *parent, const config_setting_t *src);
static void config_setting_copy_aggregate(config_setting_t *parent, const config_setting_t *src);
int config_setting_copy(config_setting_t *parent, const config_setting_t *src);

void config_setting_copy_simple(config_setting_t *parent, const config_setting_t *src)
{
	if (config_setting_is_aggregate(src)) {
		config_setting_copy_aggregate(parent, src);
	}
	else {
		config_setting_t *set = config_setting_add(parent, config_setting_name(src), config_setting_type(src));

		if (set == NULL)
			return;

		if (CONFIG_TYPE_INT == config_setting_type(src)) {
			config_setting_set_int(set, config_setting_get_int(src));
			config_setting_set_format(set, src->format);
		} else if (CONFIG_TYPE_INT64 == config_setting_type(src)) {
			config_setting_set_int64(set, config_setting_get_int64(src));
			config_setting_set_format(set, src->format);
		} else if (CONFIG_TYPE_FLOAT == config_setting_type(src)) {
			config_setting_set_float(set, config_setting_get_float(src));
		} else if (CONFIG_TYPE_STRING == config_setting_type(src)) {
			config_setting_set_string(set, config_setting_get_string(src));
		} else if (CONFIG_TYPE_BOOL == config_setting_type(src)) {
			config_setting_set_bool(set, config_setting_get_bool(src));
		}
	}
}

void config_setting_copy_elem(config_setting_t *parent, const config_setting_t *src)
{
	config_setting_t *set = NULL;

	if (config_setting_is_aggregate(src))
		config_setting_copy_aggregate(parent, src);
	else if (CONFIG_TYPE_INT == config_setting_type(src)) {
		set = config_setting_set_int_elem(parent, -1, config_setting_get_int(src));
		config_setting_set_format(set, src->format);
	} else if (CONFIG_TYPE_INT64 == config_setting_type(src)) {
		set = config_setting_set_int64_elem(parent, -1, config_setting_get_int64(src));
		config_setting_set_format(set, src->format);   
	} else if (CONFIG_TYPE_FLOAT == config_setting_type(src)) {
		config_setting_set_float_elem(parent, -1, config_setting_get_float(src));
	} else if (CONFIG_TYPE_STRING == config_setting_type(src)) {
		config_setting_set_string_elem(parent, -1, config_setting_get_string(src));
	} else if (CONFIG_TYPE_BOOL == config_setting_type(src)) {
		config_setting_set_bool_elem(parent, -1, config_setting_get_bool(src));
	}
}

void config_setting_copy_aggregate(config_setting_t *parent, const config_setting_t *src)
{
	config_setting_t *newAgg;
	int i, n;

	newAgg = config_setting_add(parent, config_setting_name(src), config_setting_type(src));

	if (newAgg == NULL)
		return;

	n = config_setting_length(src);
	
	for (i = 0; i < n; i++) {
		if (config_setting_is_group(src)) {
			config_setting_copy_simple(newAgg, config_setting_get_elem(src, i));            
		} else {
			config_setting_copy_elem(newAgg, config_setting_get_elem(src, i));
		}
	}
}

int config_setting_copy(config_setting_t *parent, const config_setting_t *src)
{
	if (!config_setting_is_group(parent) && !config_setting_is_list(parent))
		return CONFIG_FALSE;

	if (config_setting_is_aggregate(src)) {
		config_setting_copy_aggregate(parent, src);
	} else {
		config_setting_copy_simple(parent, src);
	}
	return CONFIG_TRUE;
}

// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "../config/pandas.hpp"	// 引入主要的宏定义文件 [Sola丶小克]

#ifdef Pandas

	#include "../common/utf8.hpp"

#endif // Pandas

#ifdef Pandas_Support_Read_UTF8BOM_Configure

	#define fopen(FNAME, MODE) PandasUtf8::fopen(FNAME, MODE)
	#define fgets(BUFFER, MAXCOUNT, STREAM) PandasUtf8::fgets(BUFFER, MAXCOUNT, STREAM)
	#define fread(BUFFER, ELESIZE, ELECOUNT, STREAM) PandasUtf8::fread(BUFFER, ELESIZE, ELECOUNT, STREAM)

#endif // Pandas_Support_Read_UTF8BOM_Configure

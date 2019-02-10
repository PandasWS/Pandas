// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CONFIG_CUSTOM_DEFINES_CORE_HPP
#define CONFIG_CUSTOM_DEFINES_CORE_HPP

/**
 * rAthena configuration file (http://rathena.org)
 * For detailed guidance on these check http://rathena.org/wiki/SRC/config/
 **/

#include "../config/rathena.hpp"	// 引入 rAthenaCN 的宏定义文件 [Sola丶小克]

#ifdef rAthenaCN

	#include <string>

	#include "../common/assistant.hpp"
	#include "../common/utf8.hpp"

#endif // rAthenaCN


#ifdef rAthenaCN_Support_Read_UTF8BOM_Configure

	#define UTF8FOPEN(FILENAME, MODE) utf8_fopen(FILENAME, MODE)
	#define UTF8FGETS(BUFFER, MAXCOUNT, STREAM) utf8_fgets(BUFFER, MAXCOUNT, STREAM)
	#define UTF8FREAD(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM) utf8_fread(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM)

#else

	#define UTF8FOPEN(FILENAME, MODE) fopen(FILENAME, MODE)
	#define UTF8FGETS(BUFFER, MAXCOUNT, STREAM) fgets(BUFFER, MAXCOUNT, STREAM)
	#define UTF8FREAD(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM) fread(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM)

#endif // rAthenaCN_Support_Read_UTF8BOM_Configure

#endif /* CONFIG_CUSTOM_DEFINES_CORE_HPP */

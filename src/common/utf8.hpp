// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_UTF8_HPP_
#define _RATHENA_CN_UTF8_HPP_

#include "showmsg.hpp"
#include "../config/pandas.hpp"

#include <stdio.h>
#include <string>	// std::string

std::string utf8_u2g(const std::string& strUtf8);
std::string utf8_g2u(const std::string& strGbk);
bool utf8_isbom(FILE *_Stream);

FILE* utf8_fopen(const char* _FileName, const char* _Mode);
char* utf8_fgets(char *_Buffer, int _MaxCount, FILE *_Stream);
size_t utf8_fread(void *_Buffer, size_t _ElementSize, size_t _ElementCount, FILE *_Stream);

#endif // _RATHENA_CN_UTF8_HPP_

// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_UTF8_HPP_
#define _RATHENA_CN_UTF8_HPP_

#include "showmsg.hpp"
#include "../config/rathena.hpp"

#include <stdio.h>
#include <string>	// std::string

/************************************************************************/
/* 以下为可外导出的函数定义, 需要的话可直接调用
/************************************************************************/

std::string Utf8ToGbk(const std::string& strUtf8);
std::string GbkToUtf8(const std::string& strGbk);

bool isUTF8withBOM(FILE *_Stream);

FILE* fopen_ex(const char* _FileName, const char* _Mode);
char* fgets_ex(char *_Buffer, int _MaxCount, FILE *_Stream);
size_t fread_ex(void *_Buffer, size_t _ElementSize, size_t _ElementCount, FILE *_Stream);

#endif // _RATHENA_CN_UTF8_HPP_

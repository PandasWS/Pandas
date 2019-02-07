// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_UTF8_HPP_
#define _RATHENA_CN_UTF8_HPP_

#include "showmsg.hpp"
#include "../config/rathena.hpp"

#include <stdio.h>
#include <string>	// std::string

/************************************************************************/
/* 以下为一些宏定义, 便于在项目中替换使用
/************************************************************************/

#ifdef rAthenaCN_Support_Read_UTF8BOM_Configure
#define UTF8FOPEN(FILENAME, MODE) fopen_ex(FILENAME, MODE)
#define UTF8FGETS(BUFFER, MAXCOUNT, STREAM) fgets_ex(BUFFER, MAXCOUNT, STREAM)
#define UTF8FREAD(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM) fread_ex(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM)
#else
#define UTF8FOPEN(FILENAME, MODE) fopen(FILENAME, MODE)
#define UTF8FGETS(BUFFER, MAXCOUNT, STREAM) fgets(BUFFER, MAXCOUNT, STREAM)
#define UTF8FREAD(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM) fread(BUFFER, ELEMENTSIZE, ELEMENTCOUNT, STREAM)
#endif // rAthenaCN_Support_Read_UTF8BOM_Configure

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

// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "string"

#include <time.h> // struct tm
#include <memory> // std::shared_ptr

std::string & std_string_format(std::string & _str, const char * _Format, ...);
struct tm *safety_localtime(const time_t *time, struct tm *result);
struct tm *safety_gmtime(const time_t *time, struct tm *result);

// 直接使用 localtime 会被 LGTM 给予安全警告, 这里进行一次封装
// 实际上 localtime 最终会调用 localtime_r 或 localtime_s, 以此获得线程安全以及修正 LGTM 的警告
std::shared_ptr<struct tm> safty_localtime_define(const time_t *time);
#define localtime(_ttick) safty_localtime_define(_ttick).get()

#ifndef MINICORE
// 若 MINICORE 宏定义启用的话, 则表示目前链接这个 hpp 的工程是一个辅助工具 (csv2yaml)
// 这些辅助工具引入的是 common-minicore 工程, 而这个 common-minicore 工程没有将 sql.hpp 等文件 include 进去,
// 所以 assistant.hpp 在 minicore 模式下使用的话, 会出现链接错误
//
// 目前为了解决由此引发的链接问题, 这里将部分 common-minicore 没有 include 的头文件,
// 以及利用到这些头文件的函数单独剥离, 并使用 MINICORE 宏定义条件包起来.
#include "sql.hpp"

void smart_codepage(Sql* sql_handle, const char* connect_name, const char* codepage);
#endif // MINICORE

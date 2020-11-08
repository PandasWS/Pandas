// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include "cbasetypes.hpp"

void performance_create(std::string name);
void performance_start(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_stop(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
int64 performance_get_milliseconds(std::string name);
uint64 performance_get_totalcount(std::string name);
void performance_report(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_destory(std::string name);

void performance_create_and_start(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_stop_and_report(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);

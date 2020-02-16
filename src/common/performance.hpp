// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>

void performance_init(std::string name);
void performance_start(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_stop(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_report(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);

void performance_begin(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);
void performance_end(std::string name, const char* debug_file = __FILE__, const unsigned long debug_line = __LINE__);

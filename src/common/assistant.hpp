// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_ASSISTANT_HPP_
#define _RATHENA_CN_ASSISTANT_HPP_

#include "sql.hpp"
#include "string"

std::string & std_string_format(std::string & _str, const char * _Format, ...);
void smart_codepage(Sql* sql_handle, const char* connect_name, const char* codepage);

#endif // _RATHENA_CN_ASSISTANT_HPP_

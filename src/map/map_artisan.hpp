// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_MAP_ARTISAN_HPP_
#define _RATHENA_CN_MAP_ARTISAN_HPP_

#include <string>	// std::string

bool regexGroupVal(std::string patterns, std::string content, int groupid, std::string & val);
bool hasPet(const char* script, unsigned int & pet_mobid);

#endif // _RATHENA_CN_MAP_ARTISAN_HPP_

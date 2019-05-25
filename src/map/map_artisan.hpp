// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder
#pragma once

#include <string> // std::string

#include "../config/pandas.hpp"

bool regexGroupVal(std::string patterns, std::string content, int groupid, std::string & val);
bool regexMatch(std::string patterns, std::string content);

bool hasPet(const char* _script, unsigned int & pet_mobid);
bool hasCallfunc(const char* _script);

#ifdef Pandas_MapFlag_NoMail
	bool mapflag_nomail_helper(struct map_session_data *sd);
#endif // Pandas_MapFlag_NoMail

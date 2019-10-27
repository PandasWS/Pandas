// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder
#pragma once

#include <string> // std::string
#include <vector> // std::vector

#include "../config/pandas.hpp"
#include "../common/cbasetypes.hpp" // uint32

bool isRegexMatched(std::string patterns, std::string content);
bool hasCatchPet(const char* _script, std::vector<uint32>& pet_mobid);
bool hasCallfunc(const char* _script);

#ifdef Pandas_MapFlag_NoMail
	bool mapflag_nomail_helper(struct map_session_data *sd);
#endif // Pandas_MapFlag_NoMail

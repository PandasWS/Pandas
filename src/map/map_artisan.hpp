// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string> // std::string
#include <vector> // std::vector

#include <config/pandas.hpp>
#include <common/cbasetypes.hpp> // uint32

bool hasCatchPet(const std::string& script, std::vector<uint32>& pet_mobid);
bool hasCallfunc(const std::string& script);


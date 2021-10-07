// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "http.hpp"
#include "../common/mmo.hpp"

struct s_recruitment {
	uint32 account_id;
	uint32 char_id;
	char world_name[32] = { 0 };
	char char_name[NAME_LENGTH] = { 0 };
	char memo[32] = { 0 };
	uint32 min_level = 0;
	uint32 max_level = 0;
	uint8 tanker = 1;
	uint8 healer = 1;
	uint8 dealer = 1;
	uint8 assist = 1;
	uint8 adventure_type = 0;
};

HANDLER_FUNC(party_recruitment_add);
HANDLER_FUNC(party_recruitment_del);
HANDLER_FUNC(party_recruitment_get);
HANDLER_FUNC(party_recruitment_list);
HANDLER_FUNC(party_recruitment_search);

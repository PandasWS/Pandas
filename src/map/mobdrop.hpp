// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include <unordered_map>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"

struct s_mobitem_fixed_ratio_item {
	uint32 nameid;
	uint32 fixed_ratio;
	std::vector<uint32> monsters;
};

class MobItemFixedRatioDB : public TypesafeYamlDatabase<uint32, s_mobitem_fixed_ratio_item> {
public:
	MobItemFixedRatioDB() : TypesafeYamlDatabase("MOBITEM_FIXED_RATIO_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
};

extern MobItemFixedRatioDB mobitem_fixedratio_db;

uint32 mob_fixed_drop_adjust(uint32 nameid, uint32 mobid, uint32 rate);

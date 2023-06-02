// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include <unordered_map>

#include <common/cbasetypes.hpp>
#include <common/database.hpp>

struct s_mobitem_fixed_ratio_item {
	uint32 nameid;
	uint32 fixed_ratio;
	bool strict_fixed;
	std::vector<uint32> monsters;
};

class MobItemFixedRatioDB : public TypesafeYamlDatabase<uint32, s_mobitem_fixed_ratio_item> {
public:
	MobItemFixedRatioDB() : TypesafeYamlDatabase("MOBITEM_FIXED_RATIO_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const ryml::NodeRef& node) override;
};

extern MobItemFixedRatioDB mobitem_fixedratio_db;

uint32 mobdrop_fixed_droprate_adjust(uint32 nameid, uint32 mobid, uint32 rate);
bool mobdrop_strict_droprate(uint32 nameid, uint32 mobid);

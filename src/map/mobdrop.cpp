// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "mobdrop.hpp"

#include <algorithm>	// std::find

#include "itemdb.hpp"

MobItemFixedRatioDB mobitem_fixedratio_db;

const std::string MobItemFixedRatioDB::getDefaultLocation() {
	return std::string(db_path) + "/mob_item_ratio_fixed.yml";
}

uint64 MobItemFixedRatioDB::parseBodyNode(const YAML::Node &node) {
	uint32 nameid = 0;

	if (!this->asUInt32(node, "ItemID", nameid)) {
		return 0;
	}

	if (!itemdb_exists(nameid)) {
		this->invalidWarning(node, "Unknown item ID %hu in MobItem Fixed Ratio Database.\n", nameid);
		return 0;
	}

	auto mobitem_fixed_ratio_item = this->find(nameid);
	bool exists = mobitem_fixed_ratio_item != nullptr;

	if (!exists) {
		if (!this->nodeExists(node, "FixedRatio")) {
			this->invalidWarning(node, "Node \"FixedRatio\" is missing.\n");
			return 0;
		}

		mobitem_fixed_ratio_item = std::make_shared<s_mobitem_fixed_ratio_item>();

		mobitem_fixed_ratio_item->nameid = nameid;
	}

	if (this->nodeExists(node, "FixedRatio")) {
		uint32 fixed_ratio = 0;

		if (!this->asUInt32(node, "FixedRatio", fixed_ratio)) {
			return 0;
		}

		mobitem_fixed_ratio_item->fixed_ratio = fixed_ratio;
	}

	if (this->nodeExists(node, "ForMonster")) {
		for (const YAML::Node& subNode : node["ForMonster"]) {
			uint32 for_monster_id = 0;

			if (!this->asUInt32(subNode, "MobID", for_monster_id)) {
				return 0;
			}

			mobitem_fixed_ratio_item->monsters.push_back(for_monster_id);
		}
	}

	if (!exists) {
		this->put(mobitem_fixed_ratio_item->nameid, mobitem_fixed_ratio_item);
	}

	return 1;
}

//************************************
// Method:		mob_fixed_drop_adjust
// Description:	
// Parameter:	uint32 nameid
// Parameter:	uint32 mobid
// Parameter:	uint32 rate
// Returns:		uint32
//************************************
uint32 mob_fixed_drop_adjust(uint32 nameid, uint32 mobid, uint32 rate) {
	std::shared_ptr<s_mobitem_fixed_ratio_item> data = mobitem_fixedratio_db.find(nameid);
	if (!data) return rate;

	if (!data->monsters.empty()) {
		std::vector<uint32>::iterator iter;
		iter = std::find(data->monsters.begin(), data->monsters.end(), mobid);
		if (iter == data->monsters.end()) return rate;
	}

	return data->fixed_ratio;
}

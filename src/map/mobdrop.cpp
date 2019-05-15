// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "mobdrop.hpp"

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

	auto mobitem_fixed_ratio_table = this->find(nameid);
	bool exists = mobitem_fixed_ratio_table != nullptr;

	if (!exists) {
		if (!this->nodeExists(node, "FixedRatio")) {
			this->invalidWarning(node, "Node \"FixedRatio\" is missing.\n");
			return 0;
		}

		mobitem_fixed_ratio_table = std::make_shared<s_mobitem_fixed_ratio_table>();

		mobitem_fixed_ratio_table->nameid = nameid;
	}

	if (this->nodeExists(node, "FixedRatio")) {
		uint32 fixed_ratio = 0;

		if (!this->asUInt32(node, "FixedRatio", fixed_ratio)) {
			return 0;
		}

		mobitem_fixed_ratio_table->fixed_ratio = fixed_ratio;
	}

	if (this->nodeExists(node, "ForMonster")) {
		for (const YAML::Node& subNode : node["ForMonster"]) {
			uint32 for_monster_id = 0;

			if (!this->asUInt32(subNode, "MobID", for_monster_id)) {
				return 0;
			}

			mobitem_fixed_ratio_table->monsters.push_back(for_monster_id);
		}
	}

	if (!exists) {
		this->put(mobitem_fixed_ratio_table->nameid, mobitem_fixed_ratio_table);
	}

	return 1;
}

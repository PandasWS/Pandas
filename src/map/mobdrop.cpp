// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "mobdrop.hpp"

#include <algorithm>	// std::find

#include "itemdb.hpp"

MobItemFixedRatioDB mobitem_fixedratio_db;

const std::string MobItemFixedRatioDB::getDefaultLocation() {
	return std::string(db_path) + "/mob_item_ratio_fixed.yml";
}

uint64 MobItemFixedRatioDB::parseBodyNode(const ryml::NodeRef& node) {
	uint32 nameid = 0;

	if (!this->asUInt32(node, "ItemID", nameid)) {
		return 0;
	}

	if (!item_db.exists(nameid)) {
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

	if (this->nodeExists(node, "StrictFixed")) {
		bool strict_fixed = false;

		if (!this->asBool(node, "StrictFixed", strict_fixed)) {
			return 0;
		}

		mobitem_fixed_ratio_item->strict_fixed = strict_fixed;
	}

	if (this->nodeExists(node, "ForMonster")) {
		for (const ryml::NodeRef& subNode : node["ForMonster"]) {
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
// Method:		mobdrop_fixed_droprate_adjust
// Description:	根据魔物编号和掉落的道具编号, 查询他们的固定基础掉率
// Parameter:	uint32 nameid	掉落的道具编号
// Parameter:	uint32 mobid	魔物编号
// Parameter:	uint32 rate		当前原始概率
// Returns:		uint32 有配置则返回配置的概率, 无配置则返回当前原始概率
//************************************
uint32 mobdrop_fixed_droprate_adjust(uint32 nameid, uint32 mobid, uint32 rate) {
	std::shared_ptr<s_mobitem_fixed_ratio_item> data = mobitem_fixedratio_db.find(nameid);
	if (!data) return rate;

	if (!data->monsters.empty()) {
		std::vector<uint32>::iterator iter;
		iter = std::find(data->monsters.begin(), data->monsters.end(), mobid);
		if (iter == data->monsters.end()) return rate;
	}

	return data->fixed_ratio;
}

//************************************
// Method:		mobdrop_strict_droprate
// Description:	判断某魔物掉落某道具时, 是否需要执行严格固定掉率
// Parameter:	uint32 nameid
// Parameter:	uint32 mobid
// Returns:		bool 返回 true 表示需要严格固定
//************************************
bool mobdrop_strict_droprate(uint32 nameid, uint32 mobid) {
	std::shared_ptr<s_mobitem_fixed_ratio_item> data = mobitem_fixedratio_db.find(nameid);
	if (!data) return false;

	// 若指定了只对某些魔物有效的话, 那么判断 mobid 是否在列表中
	if (!data->monsters.empty()) {
		std::vector<uint32>::iterator iter;
		iter = std::find(data->monsters.begin(), data->monsters.end(), mobid);
		if (iter == data->monsters.end()) return false;
	}

	return data->strict_fixed;
}

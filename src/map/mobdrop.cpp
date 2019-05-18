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

	if (this->nodeExists(node, "IgnoreLevelPenalty")) {
		bool ignore_level_penalty = false;

		if (!this->asBool(node, "IgnoreLevelPenalty", ignore_level_penalty)) {
			return 0;
		}

		mobitem_fixed_ratio_item->ignore_level_penalty = ignore_level_penalty;
	}

	if (this->nodeExists(node, "IgnoreVipIncrease")) {
		bool ignore_vip_increase = false;

		if (!this->asBool(node, "IgnoreVipIncrease", ignore_vip_increase)) {
			return 0;
		}

		mobitem_fixed_ratio_item->ignore_vip_increase = ignore_vip_increase;
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
// Method:		mobdrop_allow_modifier_sub
// Description:	内部函数, 判断是否允许"等级惩罚"和"VIP加成"机制给道具掉率带来影响
// Parameter:	uint32 nameid
// Parameter:	uint32 mobid
// Parameter:	uint8 modifier	1 = 进行等级惩罚判断, 2 = 进行 VIP 掉率判断
// Returns:		bool 返回 true 表示允许
//************************************
bool mobdrop_allow_modifier_sub(uint32 nameid, uint32 mobid, uint8 modifier) {
	std::shared_ptr<s_mobitem_fixed_ratio_item> data = mobitem_fixedratio_db.find(nameid);
	if (!data) return true;

	// 若指定了只对某些魔物有效的话, 那么判断 mobid 是否在列表中
	// 若不指定的魔物列表中, 那么默认允许掉率被"等级惩罚"或"VIP加成"机制带来的影响
	if (!data->monsters.empty()) {
		std::vector<uint32>::iterator iter;
		iter = std::find(data->monsters.begin(), data->monsters.end(), mobid);
		if (iter == data->monsters.end()) return true;
	}

	switch (modifier) {
	case 1:	return !data->ignore_level_penalty;		// Level Penalty
	case 2:	return !data->ignore_vip_increase;		// Vip Increase
	}

	return true;
}

//************************************
// Method:		mobdrop_allow_lv
// Description:	判断某个道具被某个魔物掉落时, 是否允许等级惩罚调整
// Parameter:	uint32 nameid
// Parameter:	uint32 mobid
// Returns:		bool 返回 true 表示允许
//************************************
bool mobdrop_allow_lv(uint32 nameid, uint32 mobid) {
	return mobdrop_allow_modifier_sub(nameid, mobid, 1);	// Level Penalty
}

//************************************
// Method:		mobdrop_allow_vip
// Description:	判断某个道具被某个魔物掉落时, 是否允许 VIP 掉率加成
// Parameter:	uint32 nameid
// Parameter:	uint32 mobid
// Returns:		bool 返回 true 表示允许
//************************************
bool mobdrop_allow_vip(uint32 nameid, uint32 mobid) {
	return mobdrop_allow_modifier_sub(nameid, mobid, 2);	// Vip Increase
}

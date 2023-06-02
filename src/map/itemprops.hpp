// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include <unordered_map>

#include "itemdb.hpp"

#include <common/cbasetypes.hpp>
#include <common/database.hpp>

#ifdef Pandas_Item_Properties

#define ITEM_PROPERTIES_HASFLAG(id, attr, flag) (id && (id->pandas.properties.attr & flag) == flag)

enum e_item_noview : uint32 {
	ITEM_NOVIEW_UNKNOW		= 0x0000,
	ITEM_NOVIEW_WHEN_I_SEE	= 0x0001,	// 当我看自己的装备时, 隐藏道具外观
	ITEM_NOVIEW_WHEN_T_SEE	= 0x0002	// 当其他人看我的装备时, 隐藏道具外观
};

enum e_item_annouce : uint32 {
	ITEM_ANNOUCE_NONE							= 0x0000,	// 没有指定的特殊公告策略
	ITEM_ANNOUCE_DROP_TO_GROUND					= 0x0001,	// 掉落到地面时触发公告
	ITEM_ANNOUCE_DROP_TO_INVENTORY_FOR_MVP		= 0x0002,	// 作为 MVP 奖励道具直接放到背包时触发公告
	ITEM_ANNOUCE_STEAL_TO_INVENTORY				= 0x0004	// 使用偷窃技能直接将道具偷到背包时触发公告
};

enum e_item_property : uint32 {
	ITEM_PRO_AVOID_CONSUME_FOR_USE		= 0x0001,	// 表示避免物品被玩家主动使用而消耗
	ITEM_PRO_AVOID_CONSUME_FOR_SKILL	= 0x0002,	// 避免物品被作为发动技能的必要道具而消耗
	ITEM_PRO_IS_AMULET_ITEM				= 0x0004,	// 表示此物品是一个护身符类型的道具
	ITEM_PRO_EXECUTE_MOBDROP_EXPRESS	= 0x0008	// 此物品能触发 OnMobDropItemExpress 实时事件
};

struct s_item_properties {
	uint32 nameid;
	uint32 special;
	uint32 noview;
	uint32 annouce;
};

class ItemProperties : public TypesafeYamlDatabase<uint32, s_item_properties> {
public:
	ItemProperties() : TypesafeYamlDatabase("ITEM_PROPERTIES_DB", 2) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const ryml::NodeRef& node) override;

	void parsePropertiesToItemDB(ItemDatabase& item_db);
	std::shared_ptr<s_item_properties> getProperty(uint32 nameid);
};

extern ItemProperties item_properties_db;

#endif // Pandas_Item_Properties

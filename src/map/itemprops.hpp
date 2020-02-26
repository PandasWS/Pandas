// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include <unordered_map>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"

enum e_item_noview : uint32 {
	ITEM_NOVIEW_UNKNOW		= 0x0000,
	ITEM_NOVIEW_WHEN_I_SEE	= 0x0001,	// 当我看自己的装备时, 隐藏道具外观
	ITEM_NOVIEW_WHEN_T_SEE	= 0x0002	// 当其他人看我的装备时, 隐藏道具外观
};

struct s_item_properties {
	uint32 nameid;
	uint32 property;
	uint32 noview;
};

class ItemProperties : public TypesafeYamlDatabase<uint32, s_item_properties> {
public:
	ItemProperties() : TypesafeYamlDatabase("ITEM_PROPERTIES_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
};

extern ItemProperties item_properties_db;

std::shared_ptr<s_item_properties> itemdb_get_property(uint32 nameid);

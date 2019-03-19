// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>
#include <unordered_map>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"

struct s_item_properties_table {
	uint32 nameid;
	uint32 property;
};

class ItemProperties : public TypesafeYamlDatabase<uint32, s_item_properties_table> {
public:
	ItemProperties() : TypesafeYamlDatabase("ITEM_PROPERTIES_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
};

extern ItemProperties item_properties_db;

uint32 itemdb_property(uint16 nameid);

﻿// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "itemprops.hpp"

#include "itemdb.hpp"

ItemProperties item_properties_db;

const std::string ItemProperties::getDefaultLocation() {
	return std::string(db_path) + "/item_properties.yml";
}

uint64 ItemProperties::parseBodyNode(const YAML::Node &node) {
	uint32 nameid = 0;

	if (!this->asUInt32(node, "ItemID", nameid)) {
		return 0;
	}

	if (!itemdb_exists(nameid)) {
		this->invalidWarning(node, "Unknown item ID %hu in Item Properties Database.\n", nameid);
		return 0;
	}

	auto item_properties_item = this->find(nameid);
	bool exists = item_properties_item != nullptr;

	if (!exists) {
		if (!this->nodeExists(node, "Property")) {
			this->invalidWarning(node, "Node \"Property\" is missing.\n");
			return 0;
		}

		item_properties_item = std::make_shared<s_item_properties_item>();

		item_properties_item->nameid = nameid;
	}

	if (this->nodeExists(node, "Property")) {
		uint32 property = 0;

		if (!this->asUInt32(node, "Property", property)) {
			return 0;
		}

		item_properties_item->property = property;
	}

	if (!exists) {
		this->put(item_properties_item->nameid, item_properties_item);
	}

	return 1;
}

//************************************
// Method:		itemdb_get_property
// Description:	获取一个道具编号的特殊属性掩码
// Parameter:	uint32 nameid
// Returns:		uint32
//************************************
uint32 itemdb_get_property(uint32 nameid) {
	std::shared_ptr<s_item_properties_item> data = item_properties_db.find(nameid);
	return (data ? data->property : 0);
}

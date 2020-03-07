// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "itemprops.hpp"

#include "itemdb.hpp"

ItemProperties item_properties_db;

//************************************
// Method:      getDefaultLocation
// Description: 获取 YAML 数据文件的具体路径
// Returns:     const std::string
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:33
//************************************
const std::string ItemProperties::getDefaultLocation() {
	return std::string(db_path) + "/item_properties.yml";
}

//************************************
// Method:      parseBodyNode
// Description: 解析 Body 节点的主要处理函数
// Parameter:   const YAML::Node & node
// Returns:     uint64
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:33
//************************************
uint64 ItemProperties::parseBodyNode(const YAML::Node &node) {
	uint32 nameid = 0;

	if (!this->asUInt32(node, "ItemID", nameid)) {
		return 0;
	}

	if (!itemdb_exists(nameid)) {
		this->invalidWarning(node, "Unknown item ID %hu in Item Properties Database.\n", nameid);
		return 0;
	}

	auto properties_item = this->find(nameid);
	bool exists = properties_item != nullptr;

	if (!exists) {
		properties_item = std::make_shared<s_item_properties>();
		properties_item->nameid = nameid;
	}

	if (this->nodeExists(node, "Property")) {
		uint32 property = 0;

		if (!this->asUInt32(node, "Property", property)) {
			return 0;
		}

		properties_item->property = property;
	}

	if (this->nodeExists(node, "ControlViewID")) {
		const YAML::Node& noviewNode = node["ControlViewID"];

		#define GETYAML_NODE_BOOL(ynode, var, mask) {\
			if (this->nodeExists(ynode, var)) {\
				bool option = false;\
				if (!this->asBool(ynode, var, option)) {\
					return 0;\
				}\
				if (option)\
					properties_item->noview |= mask;\
				else\
					properties_item->noview &= ~mask;\
			}\
		}

		GETYAML_NODE_BOOL(noviewNode, "InvisibleWhenISee", ITEM_NOVIEW_WHEN_I_SEE);
		GETYAML_NODE_BOOL(noviewNode, "InvisibleWhenTheySee", ITEM_NOVIEW_WHEN_T_SEE);

		#undef GETYAML_NODE_BOOL
	}

	if (this->nodeExists(node, "AnnouceRules")) {
		uint32 annouceRules = 0;

		if (!this->asUInt32(node, "AnnouceRules", annouceRules)) {
			return 0;
		}

		properties_item->annouce = annouceRules;
	}

	if (!exists) {
		this->put(properties_item->nameid, properties_item);
	}

	return 1;
}

//************************************
// Method:		itemdb_get_property
// Description:	获取一个道具编号的特殊属性
// Parameter:	uint32 nameid
// Returns:		std::shared_ptr<s_item_properties>
//************************************
std::shared_ptr<s_item_properties> itemdb_get_property(uint32 nameid) {
	return item_properties_db.find(nameid);
}

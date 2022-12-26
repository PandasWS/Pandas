// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "itemprops.hpp"

#ifdef Pandas_Item_Properties

ItemProperties item_properties_db;

#define GETYAML_NODE_BOOL(ynode, var, targetfield, mask) {\
	if (this->nodeExists(ynode, var)) {\
		bool option = false;\
		if (!this->asBool(ynode, var, option)) {\
			return 0;\
		}\
		if (option)\
			targetfield |= mask;\
		else\
			targetfield &= ~mask;\
	}\
}

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
// Parameter:   const ryml::NodeRef & node
// Returns:     uint64
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:33
//************************************
uint64 ItemProperties::parseBodyNode(const ryml::NodeRef& node) {
	uint32 nameid = 0;

	if (!this->asUInt32(node, "ItemID", nameid)) {
		return 0;
	}

	if (!item_db.exists(nameid)) {
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
		const ryml::NodeRef& propertyNode = node["Property"];
		GETYAML_NODE_BOOL(propertyNode, "AvoidConsumeForUse", properties_item->special, ITEM_PRO_AVOID_CONSUME_FOR_USE);
		GETYAML_NODE_BOOL(propertyNode, "AvoidConsumeForSkill", properties_item->special, ITEM_PRO_AVOID_CONSUME_FOR_SKILL);
		GETYAML_NODE_BOOL(propertyNode, "IsAmuletItem", properties_item->special, ITEM_PRO_IS_AMULET_ITEM);
		GETYAML_NODE_BOOL(propertyNode, "ExecuteMobDropExpress", properties_item->special, ITEM_PRO_EXECUTE_MOBDROP_EXPRESS);
	}

	if (this->nodeExists(node, "ControlViewID")) {
		const ryml::NodeRef& noviewNode = node["ControlViewID"];
		GETYAML_NODE_BOOL(noviewNode, "InvisibleWhenISee", properties_item->noview, ITEM_NOVIEW_WHEN_I_SEE);
		GETYAML_NODE_BOOL(noviewNode, "InvisibleWhenTheySee", properties_item->noview, ITEM_NOVIEW_WHEN_T_SEE);
	}

	if (this->nodeExists(node, "AnnouceRules")) {
		const ryml::NodeRef& annouceNode = node["AnnouceRules"];
		GETYAML_NODE_BOOL(annouceNode, "DropToGround", properties_item->annouce, ITEM_ANNOUCE_DROP_TO_GROUND);
		GETYAML_NODE_BOOL(annouceNode, "DropToInventoryForMVP", properties_item->annouce, ITEM_ANNOUCE_DROP_TO_INVENTORY_FOR_MVP);
		GETYAML_NODE_BOOL(annouceNode, "StealToInventory", properties_item->annouce, ITEM_ANNOUCE_STEAL_TO_INVENTORY);
	}

	if (!exists) {
		this->put(properties_item->nameid, properties_item);
	}

	return 1;
}

//************************************
// Method:      parsePropertiesToItemDB
// Description: 为 ItemDatabase 中的道具设置特殊属性
// Access:      public 
// Parameter:   ItemDatabase & item_db
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/01/02 15:56
//************************************
void ItemProperties::parsePropertiesToItemDB(ItemDatabase& item_db) {
	for (const auto& it : item_db) {
		auto value = this->getProperty(it.first);
		auto item = it.second;

		if (value) {
			item->pandas.properties.special_mask = value->special;
			item->pandas.properties.noview_mask = value->noview;
			item->pandas.properties.annouce_mask = value->annouce;
		}

#ifdef Pandas_Item_Amulet_System
		// 若为护身符道具, 则直接改写它的物品类型为 IT_AMULET
		if (ITEM_PROPERTIES_HASFLAG(item, special_mask, ITEM_PRO_IS_AMULET_ITEM)) {
			item->type = IT_AMULET;
		}
#endif // Pandas_Item_Amulet_System
	}
}

//************************************
// Method:      getProperty
// Description: 获取一个道具编号的特殊属性
// Access:      public 
// Parameter:   uint32 nameid
// Returns:     std::shared_ptr<s_item_properties>
// Author:      Sola丶小克(CairoLee)  2021/01/02 15:58
//************************************
std::shared_ptr<s_item_properties> ItemProperties::getProperty(uint32 nameid) {
	return item_properties_db.find(nameid);
}

#endif // Pandas_Item_Properties

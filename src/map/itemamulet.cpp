// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "itemamulet.hpp"

#include "pc.hpp"
#include "itemdb.hpp"
#include "itemprops.hpp"

#include "../common/nullpo.hpp"
#include "../common/utils.hpp"

#ifdef Pandas_Item_Amulet_System

extern short current_equip_item_index;

//************************************
// Method:		amulet_is
// Description:	给定一个道具编号确认它是否为一个护身符类型的道具
// Parameter:	t_itemid nameid
// Returns:		bool 返回 true 则表示该道具为护身符
//************************************
bool amulet_is(t_itemid nameid) {
#ifdef Pandas_Item_Amulet_System
	struct item_data* item = itemdb_search(nameid);
	return ITEM_PROPERTIES_HASFLAG(item, special_mask, ITEM_PRO_IS_AMULET_ITEM);
#else
	return false;
#endif // Pandas_Item_Amulet_System
}

//************************************
// Method:		amulet_pandas_type
// Description:	给定一个道具编号返回它的道具类型, 此函数用于 clif 给客户端发数据前使用
// Parameter:	t_itemid nameid
// Returns:		int
//************************************
int amulet_pandas_type(t_itemid nameid) {
	return (amulet_is(nameid) ? IT_ETC : itemdb_search(nameid)->type);
}

//************************************
// Method:		amulet_is_firstone
// Description:	给定的 item 是否为该角色身上同类型护身符中的第一个
// Parameter:	struct map_session_data * sd
// Parameter:	struct item * item
// Parameter:	int amount
// Returns:		bool 返回 true 表示这是同类护身符中的第一个
//************************************
bool amulet_is_firstone(struct map_session_data *sd, struct item *item, int amount) {
 	nullpo_retr(false, sd);
 	nullpo_retr(false, item);

	if (!sd || !item)
		return false;

	amount = cap_value(amount, 0, MAX_AMOUNT);
	if (item && item->nameid == 0 || amount <= 0)
		return false;

	if (!amulet_is(item->nameid))
		return false;

	struct item_data *id = itemdb_search(item->nameid);
	if (!itemdb_isstackable2(id) || item->expire_time != 0)
		return false;

	uint32 i = MAX_INVENTORY;
	bool is_firstone = true;
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->inventory.u.items_inventory[i].nameid == item->nameid &&
			sd->inventory.u.items_inventory[i].bound == item->bound &&
			sd->inventory.u.items_inventory[i].expire_time == 0 &&
			sd->inventory.u.items_inventory[i].unique_id == item->unique_id &&
			memcmp(&sd->inventory.u.items_inventory[i].card, &item->card, sizeof(item->card)) == 0) {
			is_firstone = false;
		}
	}

	return is_firstone;
}

//************************************
// Method:		amulet_is_lastone
// Description: 判断删除掉指定数量的护身符道具后, 角色身上就不存在其他同类护身符了
// Parameter:	struct map_session_data * sd
// Parameter:	int n
// Parameter:	int amount
// Returns:		bool 返回 true 表示这次删除后，角色身上就不存在同类的护身符道具了
//************************************
bool amulet_is_lastone(struct map_session_data *sd, int n, int amount) {
	nullpo_retr(false, sd);

	amount = cap_value(amount, 0, MAX_AMOUNT);
	if (!sd || !sd->inventory_data[n] || amount <= 0)
		return false;

	struct item_data *item = sd->inventory_data[n];
	if (!amulet_is(item->nameid))
		return false;

	return ((sd->inventory.u.items_inventory[n].amount - amount) <= 0);
}

//************************************
// Method:		amulet_apply_additem
// Description:	添加新的护身符道具时, 根据需要重算角色的能力
// Parameter:	struct map_session_data * sd
// Parameter:	int n
// Parameter:	bool is_firstone
// Returns:		void
//************************************
void amulet_apply_additem(struct map_session_data *sd, int n, bool is_firstone) {
	nullpo_retv(sd);

	if (!sd || !sd->inventory_data[n])
		return;

	struct item_data *item = sd->inventory_data[n];
	if (!amulet_is(item->nameid))
		return;

	if (is_firstone && item->equip_script) {
		// 如果这是背包中出现的第一个同种类护身符，那么触发"穿戴脚本"
		run_script(item->equip_script, 0, sd->bl.id, 0);
		status_calc_pc(sd, SCO_NONE);
	}
	else if (item->script) {
		// 否则如果有"使用脚本"的话, 也需要重算一下角色能力
		status_calc_pc(sd, SCO_NONE);
	}
}

//************************************
// Method:		amulet_apply_delitem
// Description:	删除老的护身符道具时, 根据需要重算角色的能力
// Parameter:	struct map_session_data * sd
// Parameter:	int n
// Parameter:	bool is_lastone
// Returns:		void
//************************************
void amulet_apply_delitem(struct map_session_data *sd, int n, bool is_lastone) {
	nullpo_retv(sd);

	if (!sd || !sd->inventory_data[n])
		return;

	struct item_data *item = sd->inventory_data[n];
	if (!amulet_is(item->nameid))
		return;

	if (is_lastone && item->unequip_script) {
		// 如果移除的护身符是身上同种类护身符的最后一个，那么触发"卸装脚本"
		short save_current_equip_item_index = current_equip_item_index;
		current_equip_item_index = n;
		run_script(item->unequip_script, 0, sd->bl.id, 0);
		current_equip_item_index = save_current_equip_item_index;
		status_calc_pc(sd, SCO_NONE);
	}
	else if (item->script){
		// 否则如果有"使用脚本"的话, 也需要重算一下角色能力
		status_calc_pc(sd, SCO_NONE);
	}
}

//************************************
// Method:      amulet_status_calc
// Description: 重新应用角色身上全部护身符道具的脚本效果
// Parameter:   struct map_session_data * sd
// Parameter:   uint8 opt
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2022/04/04 18:03
//************************************ 
void amulet_status_calc(struct map_session_data *sd, uint8 opt) {
	nullpo_retv(sd);

	if (!sd || sd->pandas.amulet_calculating)
		return;

	sd->pandas.amulet_calculating = true;

	short save_current_equip_item_index = current_equip_item_index;

	for (uint32 i = 0; i < MAX_INVENTORY; i++) {
		if (!sd || !sd->inventory_data[i])
			continue;

		if (!amulet_is(sd->inventory_data[i]->nameid))
			continue;

		std::shared_ptr<item_data> id = item_db.find(sd->inventory_data[i]->nameid);
		if (id && itemdb_isNoEquip(id.get(), sd->bl.m))
			continue;

		current_equip_item_index = i;

		if (opt&SCO_FIRST && sd->inventory_data[i]->equip_script) {
			run_script(sd->inventory_data[i]->equip_script, 0, sd->bl.id, 0);
		}

		if (sd->inventory_data[i]->script) {
			// 这件护身符在身上有多少个, 就执行多少次"使用脚本"
			for (uint16 k = 0; k < sd->inventory.u.items_inventory[i].amount; k++) {
				run_script(sd->inventory_data[i]->script, 0, sd->bl.id, 0);
				if (!sd->pandas.amulet_calculating) {
					current_equip_item_index = save_current_equip_item_index;
					return;
				}
			}
		}
	}

	current_equip_item_index = save_current_equip_item_index;
	sd->pandas.amulet_calculating = false;
}

#endif // Pandas_Item_Amulet_System

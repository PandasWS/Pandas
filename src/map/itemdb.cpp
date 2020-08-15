﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "itemdb.hpp"

#include <stdlib.h>

#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/strlib.hpp"
#include "../common/utils.hpp"
#include "../common/utf8_defines.hpp"  // PandasWS

#include "battle.hpp" // struct battle_config
#include "cashshop.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "log.hpp"
#include "mob.hpp"
#include "pc.hpp"
#include "status.hpp"

static DBMap *itemdb; /// Item DB
static DBMap *itemdb_combo; /// Item Combo DB
static DBMap *itemdb_group; /// Item Group DB
static DBMap *itemdb_randomopt; /// Random option DB
static DBMap *itemdb_randomopt_group; /// Random option group DB

struct item_data *dummy_item; /// This is the default dummy item used for non-existant items. [Skotlex]

struct s_roulette_db rd;

#ifdef Pandas_Speedup_Itemdb_SearchName

typedef std::vector<struct item_data*> speedup_cache_item;
typedef std::shared_ptr<speedup_cache_item> shared_speedup_cache_item;
typedef std::map<std::string, shared_speedup_cache_item> speedup_cache_db;

speedup_cache_db itemdb_speedup_name;
speedup_cache_db itemdb_speedup_jname;

//************************************
// Method:      itemdb_speedup_clear
// Description: 重置并清空用于加速 itemdb 操作的缓存数据
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/14 00:39
//************************************
void itemdb_speedup_clear() {
	itemdb_speedup_name.clear();
	itemdb_speedup_jname.clear();
}

//************************************
// Method:      itemdb_speedup_cache_name
// Description: 缓存某个道具的名称, 以便加速检索效率
// Parameter:   speedup_cache_db & _map
// Parameter:   std::string key
// Parameter:   struct item_data * id
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/14 00:39
//************************************
void itemdb_speedup_cache_name(speedup_cache_db& _map, std::string key, struct item_data* id) {
	auto item = _map.find(key);
	if (item == _map.end()) {
		shared_speedup_cache_item vec = std::make_shared<speedup_cache_item>();
		vec->push_back(id);
		_map[key] = vec;
	}
	else {
		shared_speedup_cache_item vec = item->second;
		for (auto subitem = vec->begin(); subitem != vec->end(); subitem++) {
			if ((*subitem)->nameid == id->nameid) {
				(*subitem) = id;
				return;
			}
		}
		vec->push_back(id);
	}
}

//************************************
// Method:      itemdb_speedup_search_name
// Description: 在某个数据库中搜索特定道具名称的对应物品
// Parameter:   speedup_cache_db & _map
// Parameter:   std::string name
// Returns:     struct item_data*
// Author:      Sola丶小克(CairoLee)  2020/02/14 00:45
//************************************
struct item_data* itemdb_speedup_search_name(speedup_cache_db& _map, std::string name) {
	auto it = _map.find(name);
	if (it != _map.end()) {
		shared_speedup_cache_item sublist = it->second;
		if (!sublist->empty()) {
			return sublist->back();
		}
	}
	return NULL;
}

//************************************
// Method:      itemdb_speedup_item
// Description: 建立该物品相关的缓存, 以便加速其他检索操作
// Parameter:   struct item_data * id
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/14 00:46
//************************************
void itemdb_speedup_item(struct item_data* id) {
	itemdb_speedup_cache_name(itemdb_speedup_name, id->name, id);
	itemdb_speedup_cache_name(itemdb_speedup_jname, id->jname, id);
}

#endif // Pandas_Speedup_Itemdb_SearchName

/**
* Check if combo exists
* @param combo_id
* @return NULL if not exist, or struct item_combo*
*/
struct item_combo *itemdb_combo_exists(unsigned short combo_id) {
	return (struct item_combo *)uidb_get(itemdb_combo, combo_id);
}

/**
* Check if item group exists
* @param group_id
* @return NULL if not exist, or s_item_group_db *
*/
struct s_item_group_db *itemdb_group_exists(unsigned short group_id) {
	return (struct s_item_group_db *)uidb_get(itemdb_group, group_id);
}

/**
 * Check if an item exists in a group
 * @param group_id: Item Group ID
 * @param nameid: Item to check for in group
 * @return True if item is in group, else false
 */
bool itemdb_group_item_exists(unsigned short group_id, t_itemid nameid)
{
	struct s_item_group_db *group = (struct s_item_group_db *)uidb_get(itemdb_group, group_id);
	unsigned short i, j;

	if (!group)
		return false;

	for (i = 0; i < MAX_ITEMGROUP_RANDGROUP; i++) {
		for (j = 0; j < group->random[i].data_qty; j++)
			if (group->random[i].data[j].nameid == nameid)
				return true;
	}
	return false;
}

/**
 * Check if an item exists from a group in a player's inventory
 * @param group_id: Item Group ID
 * @return Item's index if found or -1 otherwise
 */
int16 itemdb_group_item_exists_pc(struct map_session_data *sd, unsigned short group_id)
{
	struct s_item_group_db *group = (struct s_item_group_db *)uidb_get(itemdb_group, group_id);

	if (!group)
		return -1;

	for (int i = 0; i < MAX_ITEMGROUP_RANDGROUP; i++) {
		for (int j = 0; j < group->random[i].data_qty; j++) {
			int16 item_position = pc_search_inventory(sd, group->random[i].data[j].nameid);

			if (item_position != -1)
				return item_position;
		}
	}

	return -1;
}

/**
 * Search for item name
 * name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
 * @see DBApply
 */
static int itemdb_searchname_sub(DBKey key, DBData *data, va_list ap)
{
	struct item_data *item = (struct item_data *)db_data2ptr(data), **dst, **dst2;
	char *str;
	str = va_arg(ap,char *);
	dst = va_arg(ap,struct item_data **);
	dst2 = va_arg(ap,struct item_data **);

	//Absolute priority to Aegis code name.
	if (dst != NULL && strcmpi(item->name, str) == 0)
		*dst = item;

	//Second priority to Client displayed name.
	if (dst2 != NULL && strcmpi(item->jname, str) == 0)
		*dst2 = item;
	return 0;
}

/*==========================================
 * Return item data from item name. (lookup)
 * @param str Item Name
 * @param aegis_only
 * @return item data
 *------------------------------------------*/
static struct item_data* itemdb_searchname1(const char *str, bool aegis_only)
{
	struct item_data *item = NULL, * item2 = NULL;

#ifndef Pandas_Speedup_Itemdb_SearchName
	if( !aegis_only )
		itemdb->foreach(itemdb, itemdb_searchname_sub, str, &item, &item2);
	else
		itemdb->foreach(itemdb, itemdb_searchname_sub, str, &item, NULL);
#else
	item = itemdb_speedup_search_name(itemdb_speedup_name, str);
	if (!aegis_only) {
		item2 = itemdb_speedup_search_name(itemdb_speedup_jname, str);
	}
#endif // Pandas_Speedup_Itemdb_SearchName

	return ((item) ? item : item2);
}

struct item_data* itemdb_searchname(const char *str)
{
	return itemdb_searchname1(str, false);
}

struct item_data* itemdb_search_aegisname( const char *str ){
	return itemdb_searchname1( str, true );
}

/**
 * @see DBMatcher
 */
static int itemdb_searchname_array_sub(DBKey key, DBData data, va_list ap)
{
	struct item_data *item = (struct item_data *)db_data2ptr(&data);
	char *str = va_arg(ap,char *);

	if (stristr(item->jname,str))
		return 0;
	if (stristr(item->name,str))
		return 0;
	return strcmpi(item->jname,str);
}

/*==========================================
 * Founds up to N matches. Returns number of matches [Skotlex]
 * @param *data
 * @param size
 * @param str
 * @return Number of matches item
 *------------------------------------------*/
int itemdb_searchname_array(struct item_data** data, int size, const char *str)
{
	DBData *db_data[MAX_SEARCH];
	int i, count = 0, db_count;

	db_count = itemdb->getall(itemdb, (DBData**)&db_data, size, itemdb_searchname_array_sub, str);
	for (i = 0; i < db_count && count < size; i++)
		data[count++] = (struct item_data*)db_data2ptr(db_data[i]);

	return count;
}

/**
* Return a random group entry from Item Group
* @param group_id
* @param sub_group: 0 is 'must' item group, random groups start from 1 to MAX_ITEMGROUP_RANDGROUP+1
* @return Item group entry or NULL on fail
*/
struct s_item_group_entry *itemdb_get_randgroupitem(uint16 group_id, uint8 sub_group) {
	struct s_item_group_db *group = (struct s_item_group_db *) uidb_get(itemdb_group, group_id);
	struct s_item_group_entry *list = NULL;
	uint16 qty = 0;

	if (!group) {
		ShowError("itemdb_get_randgroupitem: Invalid group id %d\n", group_id);
		return NULL;
	}
	if (sub_group > MAX_ITEMGROUP_RANDGROUP+1) {
		ShowError("itemdb_get_randgroupitem: Invalid sub_group %d\n", sub_group);
		return NULL;
	}
	if (sub_group == 0) {
		list = group->must;
		qty = group->must_qty;
	}
	else {
		list = group->random[sub_group-1].data;
		qty = group->random[sub_group-1].data_qty;
	}
	if (!qty) {
		ShowError("itemdb_get_randgroupitem: No item entries for group id %d and sub group %d\n", group_id, sub_group);
		return NULL;
	}
	return &list[rnd()%qty];
}

/**
* Return a random Item ID from from Item Group
* @param group_id
* @param sub_group: 0 is 'must' item group, random groups start from 1 to MAX_ITEMGROUP_RANDGROUP+1
* @return Item ID or UNKNOWN_ITEM_ID on fail
*/
t_itemid itemdb_searchrandomid(uint16 group_id, uint8 sub_group) {
	struct s_item_group_entry *entry = itemdb_get_randgroupitem(group_id, sub_group);
	return entry ? entry->nameid : UNKNOWN_ITEM_ID;
}

/** [Cydh]
* Gives item(s) to the player based on item group
* @param sd: Player that obtains item from item group
* @param group_id: The group ID of item that obtained by player
* @param *group: struct s_item_group from itemgroup_db[group_id].random[idx] or itemgroup_db[group_id].must[sub_group][idx]
*/
static void itemdb_pc_get_itemgroup_sub(struct map_session_data *sd, bool identify, struct s_item_group_entry *data) {
	uint16 i, get_amt = 0;
	struct item tmp;

	nullpo_retv(data);

	memset(&tmp, 0, sizeof(tmp));

	tmp.nameid = data->nameid;
	tmp.bound = data->bound;
	tmp.identify = identify ? identify : itemdb_isidentified(data->nameid);
#ifdef Pandas_BattleConfig_Force_Identified
	tmp.identify = (battle_config.force_identified & 1 ? 1 : tmp.identify);
#endif // Pandas_BattleConfig_Force_Identified
	tmp.expire_time = (data->duration) ? (unsigned int)(time(NULL) + data->duration*60) : 0;
	if (data->isNamed) {
		tmp.card[0] = itemdb_isequip(data->nameid) ? CARD0_FORGE : CARD0_CREATE;
		tmp.card[1] = 0;
		tmp.card[2] = GetWord(sd->status.char_id, 0);
		tmp.card[3] = GetWord(sd->status.char_id, 1);
	}

	if (!itemdb_isstackable(data->nameid))
		get_amt = 1;
	else
		get_amt = data->amount;

	// Do loop for non-stackable item
	for (i = 0; i < data->amount; i += get_amt) {
		char flag = 0;
		tmp.unique_id = data->GUID ? pc_generate_unique_id(sd) : 0; // Generate GUID
		if ((flag = pc_additem(sd, &tmp, get_amt, LOG_TYPE_SCRIPT))) {
			clif_additem(sd, 0, 0, flag);
			if (pc_candrop(sd, &tmp))
				map_addflooritem(&tmp, tmp.amount, sd->bl.m, sd->bl.x,sd->bl.y, 0, 0, 0, 0, 0);
		}
		else if (!flag && data->isAnnounced)
			intif_broadcast_obtain_special_item(sd, data->nameid, sd->itemid, ITEMOBTAIN_TYPE_BOXITEM);
	}
}

/** [Cydh]
* Find item(s) that will be obtained by player based on Item Group
* @param group_id: The group ID that will be gained by player
* @param nameid: The item that trigger this item group
* @return val: 0:success, 1:no sd, 2:invalid item group
*/
char itemdb_pc_get_itemgroup(uint16 group_id, bool identify, struct map_session_data *sd) {
	uint16 i = 0;
	struct s_item_group_db *group;

	nullpo_retr(1,sd);
	
	if (!(group = (struct s_item_group_db *) uidb_get(itemdb_group, group_id))) {
		ShowError("itemdb_pc_get_itemgroup: Invalid group id '%d' specified.\n",group_id);
		return 2;
	}
	
	// Get the 'must' item(s)
	if (group->must_qty) {
		for (i = 0; i < group->must_qty; i++)
			if (&group->must[i])
				itemdb_pc_get_itemgroup_sub(sd, identify, &group->must[i]);
	}

	// Get the 'random' item each random group
	for (i = 0; i < MAX_ITEMGROUP_RANDGROUP; i++) {
		uint16 rand;
		if (!(&group->random[i]) || !group->random[i].data_qty) //Skip empty random group
			continue;
		rand = rnd()%group->random[i].data_qty;
		if (!(&group->random[i].data[rand]) || !group->random[i].data[rand].nameid)
			continue;
		itemdb_pc_get_itemgroup_sub(sd, identify, &group->random[i].data[rand]);
	}

	return 0;
}

/** Searches for the item_data. Use this to check if item exists or not.
* @param nameid
* @return *item_data if item is exist, or NULL if not
*/
struct item_data* itemdb_exists(t_itemid nameid) {
	return ((struct item_data*)uidb_get(itemdb,nameid));
}

/// Returns name type of ammunition [Cydh]
const char *itemdb_typename_ammo (enum e_item_ammo ammo) {
	switch (ammo) {
		case AMMO_ARROW:			return "Arrow";
		case AMMO_THROWABLE_DAGGER:	return "Throwable Dagger";
		case AMMO_BULLET:			return "Bullet";
		case AMMO_SHELL:			return "Shell";
		case AMMO_GRENADE:			return "Grenade";
		case AMMO_SHURIKEN:			return "Shuriken";
		case AMMO_KUNAI:			return "Kunai";
		case AMMO_CANNONBALL:		return "Cannonball";
		case AMMO_THROWABLE_ITEM:	return "Throwable Item/Sling Item";
	}
	return "Ammunition";
}

/// Returns human readable name for given item type.
/// @param type Type id to retrieve name for ( IT_* ).
const char* itemdb_typename(enum item_types type)
{
	switch(type)
	{
		case IT_HEALING:        return "Potion/Food";
		case IT_USABLE:         return "Usable";
		case IT_ETC:            return "Etc.";
		case IT_WEAPON:         return "Weapon";
		case IT_ARMOR:          return "Armor";
		case IT_CARD:           return "Card";
		case IT_PETEGG:         return "Pet Egg";
		case IT_PETARMOR:       return "Pet Accessory";
		case IT_AMMO:           return "Arrow/Ammunition";
		case IT_DELAYCONSUME:   return "Delay-Consume Usable";
		case IT_SHADOWGEAR:     return "Shadow Equipment";
		case IT_CASH:           return "Cash Usable";
#ifdef Pandas_Item_Amulet_System
		case IT_AMULET:         return "Amulet";
#endif // Pandas_Item_Amulet_System
	}
	return "Unknown Type";
}

/**
 * Converts the jobmask from the format in itemdb to the format used by the map server.
 * @param bclass: Pointer to store itemdb format
 * @param jobmask: Job Mask to convert
 * @author: Skotlex
 */
static void itemdb_jobid2mapid(uint64 *bclass, uint64 jobmask)
{
	int i;

	bclass[0] = bclass[1] = bclass[2] = 0;

	//Base classes
	if (jobmask & 1ULL<<JOB_NOVICE) {
		//Both Novice/Super-Novice are counted with the same ID
		bclass[0] |= 1ULL<<MAPID_NOVICE;
		bclass[1] |= 1ULL<<MAPID_NOVICE;
	}
	for (i = JOB_NOVICE + 1; i <= JOB_THIEF; i++) {
		if (jobmask & 1ULL <<i)
			bclass[0] |= 1ULL<<(MAPID_NOVICE + i);
	}
	//2-1 classes
	if (jobmask & 1ULL<<JOB_KNIGHT)
		bclass[1] |= 1ULL<<MAPID_SWORDMAN;
	if (jobmask & 1ULL<<JOB_PRIEST)
		bclass[1] |= 1ULL<<MAPID_ACOLYTE;
	if (jobmask & 1ULL<<JOB_WIZARD)
		bclass[1] |= 1ULL<<MAPID_MAGE;
	if (jobmask & 1ULL<<JOB_BLACKSMITH)
		bclass[1] |= 1ULL<<MAPID_MERCHANT;
	if (jobmask & 1ULL<<JOB_HUNTER)
		bclass[1] |= 1ULL<<MAPID_ARCHER;
	if (jobmask & 1ULL<<JOB_ASSASSIN)
		bclass[1] |= 1ULL<<MAPID_THIEF;
	//2-2 classes
	if (jobmask & 1ULL<<JOB_CRUSADER)
		bclass[2] |= 1ULL<<MAPID_SWORDMAN;
	if (jobmask & 1ULL<<JOB_MONK)
		bclass[2] |= 1ULL<<MAPID_ACOLYTE;
	if (jobmask & 1ULL<<JOB_SAGE)
		bclass[2] |= 1ULL<<MAPID_MAGE;
	if (jobmask & 1ULL<<JOB_ALCHEMIST)
		bclass[2] |= 1ULL<<MAPID_MERCHANT;
	if (jobmask & 1ULL<<JOB_BARD)
		bclass[2] |= 1ULL<<MAPID_ARCHER;
//	Bard/Dancer share the same slot now.
//	if (jobmask & 1ULL<<JOB_DANCER)
//		bclass[2] |= 1ULL<<MAPID_ARCHER;
	if (jobmask & 1ULL<<JOB_ROGUE)
		bclass[2] |= 1ULL<<MAPID_THIEF;
	//Special classes that don't fit above.
	if (jobmask & 1ULL<<21) //Taekwon
		bclass[0] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<22) //Star Gladiator
		bclass[1] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<23) //Soul Linker
		bclass[2] |= 1ULL<<MAPID_TAEKWON;
	if (jobmask & 1ULL<<JOB_GUNSLINGER) { // Rebellion job can equip Gunslinger equips.
		bclass[0] |= 1ULL<<MAPID_GUNSLINGER;
		bclass[1] |= 1ULL<<MAPID_GUNSLINGER;
	}
	if (jobmask & 1ULL<<JOB_NINJA) { //Kagerou/Oboro jobs can equip Ninja equips. [Rytech]
		bclass[0] |= 1ULL<<MAPID_NINJA;
		bclass[1] |= 1ULL<<MAPID_NINJA;
	}
	if (jobmask & 1ULL<<26) //Bongun/Munak
		bclass[0] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<27) //Death Knight
		bclass[1] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<28) //Dark Collector
		bclass[2] |= 1ULL<<MAPID_GANGSI;
	if (jobmask & 1ULL<<29) //Kagerou / Oboro
		bclass[1] |= 1ULL<<MAPID_NINJA;
	if (jobmask & 1ULL<<30) //Rebellion
		bclass[1] |= 1ULL<<MAPID_GUNSLINGER;
	if (jobmask & 1ULL<<31) //Summoner
		bclass[0] |= 1ULL<<MAPID_SUMMONER;
}

/**
* Create dummy item_data as dummy_item and dummy item group entry as dummy_itemgroup
*/
static void itemdb_create_dummy(void) {
	CREATE(dummy_item, struct item_data, 1);

	memset(dummy_item, 0, sizeof(struct item_data));
	dummy_item->nameid = ITEMID_DUMMY;
	dummy_item->weight = 1;
	dummy_item->value_sell = 1;
	dummy_item->type = IT_ETC; //Etc item
	safestrncpy(dummy_item->name, "UNKNOWN_ITEM", sizeof(dummy_item->name));
	safestrncpy(dummy_item->jname, "Unknown Item", sizeof(dummy_item->jname));
	dummy_item->view_id = UNKNOWN_ITEM_ID;
}

/**
* Create new item data
* @param nameid
*/
static struct item_data *itemdb_create_item(t_itemid nameid) {
	struct item_data *id;
	CREATE(id, struct item_data, 1);
	memset(id, 0, sizeof(struct item_data));
	id->nameid = nameid;
	id->type = IT_ETC; //Etc item
	uidb_put(itemdb, nameid, id);
	return id;
}

/*==========================================
 * Loads an item from the db. If not found, it will return the dummy item.
 * @param nameid
 * @return *item_data or *dummy_item if item not found
 *------------------------------------------*/
struct item_data* itemdb_search(t_itemid nameid) {
	struct item_data* id = NULL;
	if (nameid == dummy_item->nameid)
		id = dummy_item;
	else if (!(id = (struct item_data*)uidb_get(itemdb, nameid))) {
		ShowWarning("itemdb_search: Item ID %u does not exists in the item_db. Using dummy data.\n", nameid);
		id = dummy_item;
	}
	return id;
}

/** Checks if item is equip type or not
* @param id Item data
* @return True if item is equip, false otherwise
*/
bool itemdb_isequip2(struct item_data *id) {
	nullpo_ret(id);
	switch (id->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_AMMO:
		case IT_SHADOWGEAR:
			return true;
		default:
			return false;
	}
}

/** Checks if item is stackable or not
* @param id Item data
* @return True if item is stackable, false otherwise
*/
bool itemdb_isstackable2(struct item_data *id)
{
	nullpo_ret(id);
	return id->isStackable();
}


/*==========================================
 * Trade Restriction functions [Skotlex]
 *------------------------------------------*/
bool itemdb_isdropable_sub(struct item_data *item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&1) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_cantrade_sub(struct item_data* item, int gmlv, int gmlv2) {
	return (item && (!(item->flag.trade_restriction&2) || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

bool itemdb_canpartnertrade_sub(struct item_data* item, int gmlv, int gmlv2) {
	return (item && (item->flag.trade_restriction&4 || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

bool itemdb_cansell_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&8) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_cancartstore_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&16) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_canstore_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&32) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_canguildstore_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&64) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_canmail_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&128) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_canauction_sub(struct item_data* item, int gmlv, int unused) {
	return (item && (!(item->flag.trade_restriction&256) || gmlv >= item->gm_lv_trade_override));
}

bool itemdb_isrestricted(struct item* item, int gmlv, int gmlv2, bool (*func)(struct item_data*, int, int))
{
	struct item_data* item_data = itemdb_search(item->nameid);
	int i;

	if (!func(item_data, gmlv, gmlv2))
		return false;

	if(item_data->slot == 0 || itemdb_isspecial(item->card[0]))
		return true;

	for(i = 0; i < item_data->slot; i++) {
		if (!item->card[i]) continue;
		if (!func(itemdb_search(item->card[i]), gmlv, gmlv2))
			return false;
	}
	return true;
}

bool itemdb_ishatched_egg(struct item* item) {
	if (item && item->card[0] == CARD0_PET && item->attribute == 1)
		return true;
	return false;
}

/** Specifies if item-type should drop unidentified.
* @param nameid ID of item
*/
char itemdb_isidentified(t_itemid nameid) {
	int type=itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETARMOR:
		case IT_SHADOWGEAR:
			return 0;
		default:
			return 1;
	}
}

/** Search by name for the override flags available items (Give item another sprite)
* Structure: <nameid>,<sprite>
*/
static bool itemdb_read_itemavail(char* str[], int columns, int current) {
	t_itemid nameid, sprite;
	struct item_data *id;

	nameid = strtoul(str[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_itemavail: Invalid item id %u.\n", nameid);
		return false;
	}

	sprite = strtoul(str[1], nullptr, 10);

	if( sprite > 0 )
	{
		id->flag.available = 1;
		id->view_id = sprite;
	}
	else
	{
		id->flag.available = 0;
	}

	return true;
}

static int itemdb_group_free(DBKey key, DBData *data, va_list ap);
static int itemdb_group_free2(DBKey key, DBData *data);

static bool itemdb_read_group(char* str[], int columns, int current) {
	int group_id = -1;
	unsigned int j, prob = 1;
	uint8 rand_group = 1;
	struct s_item_group_random *random = NULL;
	struct s_item_group_db *group = NULL;
	struct s_item_group_entry entry;

	memset(&entry, 0, sizeof(entry));
	entry.amount = 1;
	entry.bound = BOUND_NONE;
	
	str[0] = trim(str[0]);
	if( ISDIGIT(str[0][0]) ){
		group_id = atoi(str[0]);
	}else{
		int64 group_tmp;

		// Try to parse group id as constant
		if (!script_get_constant(str[0], &group_tmp)) {
			ShowError("itemdb_read_group: Unknown group constant \"%s\".\n", str[0]);
			return false;
		}
		group_id = static_cast<int>(group_tmp);
	}

	// Check the group id
	if( group_id < 0 ){
		ShowWarning( "itemdb_read_group: Invalid group ID '%s'\n", str[0] );
		return false;
	}

	// Remove from DB
	if( strcmpi( str[1], "clear" ) == 0 ){
		DBData data;

		if( itemdb_group->remove( itemdb_group, db_ui2key(group_id), &data ) ){
			itemdb_group_free2(db_ui2key(group_id), &data);
			ShowNotice( "itemdb_read_group: Item Group '%s' has been cleared.\n", str[0] );
			return true;
		}else{
			ShowWarning( "itemdb_read_group: Item Group '%s' has not been cleared, because it did not exist.\n", str[0] );
			return false;
		}
	}

	if( columns < 3 ){
		ShowError("itemdb_read_group: Insufficient columns (found %d, need at least 3).\n", columns);
		return false;
	}

	// Checking sub group
	prob = atoi(str[2]);

	if( columns > 4 ){
		rand_group = atoi(str[4]);

		if( rand_group < 0 || rand_group > MAX_ITEMGROUP_RANDGROUP ){
			ShowWarning( "itemdb_read_group: Invalid sub group '%d' for group '%s'\n", rand_group, str[0] );
			return false;
		}
	}else{
		rand_group = 1;
	}

	if( rand_group != 0 && prob < 1 ){
		ShowWarning( "itemdb_read_group: Random item must have a probability. Group '%s'\n", str[0] );
		return false;
	}

	// Check item
	str[1] = trim(str[1]);

	// Check if the item can be found by id
	if( ( entry.nameid = strtoul(str[1], nullptr, 10) ) <= 0 || !itemdb_exists( entry.nameid ) ){
		// Otherwise look it up by name
		struct item_data *id = itemdb_searchname(str[1]);

		if( id ){
			// Found the item with a name lookup
			entry.nameid = id->nameid;
		}else{
			ShowWarning( "itemdb_read_group: Non-existant item '%s'\n", str[1] );
			return false;
		}
	}

	if( columns > 3 ) entry.amount = cap_value(atoi(str[3]),1,MAX_AMOUNT);
	if( columns > 5 ) entry.isAnnounced= atoi(str[5]) > 0;
	if( columns > 6 ) entry.duration = cap_value(atoi(str[6]),0,UINT16_MAX);
	if( columns > 7 ) entry.GUID = atoi(str[7]) > 0;
	if( columns > 8 ) entry.bound = cap_value(atoi(str[8]),BOUND_NONE,BOUND_MAX-1);
	if( columns > 9 ) entry.isNamed = atoi(str[9]) > 0;
	
	if (!(group = (struct s_item_group_db *) uidb_get(itemdb_group, group_id))) {
		CREATE(group, struct s_item_group_db, 1);
		group->id = group_id;
		uidb_put(itemdb_group, group->id, group);
	}

	// Must item (rand_group == 0), place it here
	if (!rand_group) {
		RECREATE(group->must, struct s_item_group_entry, group->must_qty+1);
		group->must[group->must_qty++] = entry;
		
		// If 'must' item isn't set as random item, skip the next process
		if (!prob) {
			return true;
		}
		rand_group = 0;
	}
	else
		rand_group -= 1;

	random = &group->random[rand_group];
	
	RECREATE(random->data, struct s_item_group_entry, random->data_qty+prob);

	// Put the entry to its rand_group
	for (j = random->data_qty; j < random->data_qty+prob; j++)
		random->data[j] = entry;
	
	random->data_qty += prob;
	return true;
}

/** Read item forbidden by mapflag (can't equip item)
* Structure: <nameid>,<mode>
*/
static bool itemdb_read_noequip(char* str[], int columns, int current) {
	t_itemid nameid;
	int flag;
	struct item_data *id;

	nameid = strtoul(str[0], nullptr, 10);
	flag = atoi(str[1]);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_noequip: Invalid item id %u.\n", nameid);
		return false;
	}

	if (flag >= 0)
		id->flag.no_equip |= flag;
	else
		id->flag.no_equip &= ~abs(flag);

	return true;
}

/** Reads item trade restrictions [Skotlex]
* Structure: <nameid>,<mask>,<gm level>
*/
static bool itemdb_read_itemtrade(char* str[], int columns, int current) {
	t_itemid nameid;
	unsigned short flag, gmlv;
	struct item_data *id;

	nameid = strtoul(str[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		//ShowWarning("itemdb_read_itemtrade: Invalid item id %d.\n", nameid);
		//return false;
		// FIXME: item_trade.txt contains items, which are commented in item database.
		return true;
	}

	flag = atoi(str[1]);
	gmlv = atoi(str[2]);

	if( flag > 511 ) {//Check range
		ShowWarning("itemdb_read_itemtrade: Invalid trading mask %hu for item id %u.\n", flag, nameid);
		return false;
	}
	if( gmlv < 1 )
	{
		ShowWarning("itemdb_read_itemtrade: Invalid override GM level %hu for item id %u.\n", gmlv, nameid);
		return false;
	}

	id->flag.trade_restriction = flag;
	id->gm_lv_trade_override = gmlv;

	return true;
}

/** Reads item delay amounts [Paradox924X]
* Structure: <nameid>,<delay>{,<delay sc group>}
*/
static bool itemdb_read_itemdelay(char* str[], int columns, int current) {
	t_itemid nameid;
	int delay;
	struct item_data *id;

	nameid = strtoul(str[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_itemdelay: Invalid item id %u.\n", nameid);
		return false;
	}

	delay = atoi(str[1]);

	if( delay < 0 )
	{
		ShowWarning("itemdb_read_itemdelay: Invalid delay %d for item id %u.\n", delay, nameid);
		return false;
	}

	id->delay = delay;

	if (columns == 2)
		id->delay_sc = SC_NONE;
	else if( ISDIGIT(str[2][0]) )
		id->delay_sc = atoi(str[2]);
	else{ // Try read sc group id from const db
		int64 constant;

		if( !script_get_constant(trim(str[2]), &constant) ){
			ShowWarning("itemdb_read_itemdelay: Invalid sc group \"%s\" for item id %u.\n", str[2], nameid);
			return false;
		}

		id->delay_sc = (short)constant;
	}

	return true;
}

/** Reads item stacking restrictions
* Structure: <item id>,<stack limit amount>,<type>
*/
static bool itemdb_read_stack(char* fields[], int columns, int current) {
	t_itemid nameid;
	unsigned short amount;
	unsigned int type;
	struct item_data* id;

	nameid = strtoul(fields[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_stack: Unknown item id '%u'.\n", nameid);
		return false;
	}

	if( !itemdb_isstackable2(id) )
	{
		ShowWarning("itemdb_read_stack: Item id '%u' is not stackable.\n", nameid);
		return false;
	}

	amount = (unsigned short)strtoul(fields[1], NULL, 10);
	type = strtoul(fields[2], NULL, 10);

	if( !amount )
	{// ignore
		return true;
	}

	id->stack.amount       = amount;
	id->stack.inventory    = (type&1)!=0;
	id->stack.cart         = (type&2)!=0;
	id->stack.storage      = (type&4)!=0;
	id->stack.guildstorage = (type&8)!=0;

	return true;
}

/** Reads items allowed to be sold in buying stores
* <nameid>
*/
static bool itemdb_read_buyingstore(char* fields[], int columns, int current) {
	t_itemid nameid;
	struct item_data* id;

	nameid = strtoul(fields[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_buyingstore: Invalid item id %u.\n", nameid);
		return false;
	}

	if( !itemdb_isstackable2(id) )
	{
		ShowWarning("itemdb_read_buyingstore: Non-stackable item id %u cannot be enabled for buying store.\n", nameid);
		return false;
	}

	id->flag.buyingstore = true;

	return true;
}

/** Item usage restriction (item_nouse.txt)
* <nameid>,<flag>,<override>
*/
static bool itemdb_read_nouse(char* fields[], int columns, int current) {
	t_itemid nameid;
	unsigned short flag, override;
	struct item_data* id;

	nameid = strtoul(fields[0], nullptr, 10);

	if( ( id = itemdb_exists(nameid) ) == NULL ) {
		ShowWarning("itemdb_read_nouse: Invalid item id %u.\n", nameid);
		return false;
	}

	flag = atoi(fields[1]);
	override = atoi(fields[2]);

	id->item_usage.flag = flag;
	id->item_usage.override = override;

	return true;
}

/** Misc Item flags
* <item_id>,<flag>
* &1 - As dead branch item
* &2 - As item container
* &4 - GUID item, cannot be stacked even same or stackable item
*/
static bool itemdb_read_flag(char* fields[], int columns, int current) {
	t_itemid nameid = strtoul(fields[0], nullptr, 10);
	uint16 flag;
	bool set;
	struct item_data *id;

	if (!(id = itemdb_exists(nameid))) {
		ShowError("itemdb_read_flag: Invalid item id %u\n", nameid);
		return true;
	}
	
	flag = abs(atoi(fields[1]));
	set = atoi(fields[1]) > 0;

	if (flag&1) id->flag.dead_branch = set ? 1 : 0;
	if (flag&2) id->flag.group = set ? 1 : 0;
	if (flag&4 && itemdb_isstackable2(id)) id->flag.guid = set ? 1 : 0;
	if (flag&8) id->flag.bindOnEquip = true;
	if (flag&16) id->flag.broadcast = 1;
	if (flag&32) id->flag.delay_consume = 2;

	if( flag & 64 ){
		id->flag.dropEffect = 1;
	}else if( flag & 128 ){
		id->flag.dropEffect = 2;
	}else if( flag & 256 ){
		id->flag.dropEffect = 3;
	}else if( flag & 512 ){
		id->flag.dropEffect = 4;
	}else if( flag & 1024 ){
		id->flag.dropEffect = 5;
	}else if( flag & 2048 ){
		id->flag.dropEffect = 6;
	}

	return true;
}

/**
 * @return: amount of retrieved entries.
 **/
static int itemdb_combo_split_atoi (char *str, int *val) {
	int i;

	for (i=0; i<MAX_ITEMS_PER_COMBO; i++) {
		if (!str) break;

		val[i] = atoi(str);

		str = strchr(str,':');

		if (str)
			*str++=0;
	}

	if( i == 0 ) //No data found.
		return 0;

	return i;
}

/**
 * <combo{:combo{:combo:{..}}}>,<{ script }>
 **/
static void itemdb_read_combos(const char* basedir, bool silent) {
	uint32 lines = 0, count = 0;
	char line[1024];

	char path[256];
	FILE* fp;

	sprintf(path, "%s/%s", basedir, "item_combo_db.txt");

	if ((fp = fopen(path, "r")) == NULL) {
		if(silent==0) ShowError("itemdb_read_combos: File not found \"%s\".\n", path);
		return;
	}

	// process rows one by one
	while(fgets(line, sizeof(line), fp)) {
		char *str[2], *p;

		lines++;

		if (line[0] == '/' && line[1] == '/')
			continue;

		memset(str, 0, sizeof(str));

		p = line;

		p = trim(p);

		if (*p == '\0')
			continue;// empty line

		if (!strchr(p,','))
		{
			/* is there even a single column? */
			ShowError("itemdb_read_combos: Insufficient columns in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		}

		str[0] = p;
		p = strchr(p,',');
		*p = '\0';
		p++;

		str[1] = p;
		p = strchr(p,',');
		p++;

		if (str[1][0] != '{') {
			ShowError("itemdb_read_combos(#1): Invalid format (Script column) in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		}
		/* no ending key anywhere (missing \}\) */
		if ( str[1][strlen(str[1])-1] != '}' ) {
			ShowError("itemdb_read_combos(#2): Invalid format (Script column) in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		} else {
			int items[MAX_ITEMS_PER_COMBO];
			int v = 0, retcount = 0;
			struct item_data * id = NULL;
			int idx = 0;
			if((retcount = itemdb_combo_split_atoi(str[0], items)) < 2) {
				ShowError("itemdb_read_combos: line %d of \"%s\" doesn't have enough items to make for a combo (min:2), skipping.\n", lines, path);
				continue;
			}
			/* validate */
			for(v = 0; v < retcount; v++) {
				if( !itemdb_exists(items[v]) ) {
					ShowError("itemdb_read_combos: line %d of \"%s\" contains unknown item ID %d, skipping.\n", lines, path,items[v]);
					break;
				}
			}
			/* failed at some item */
			if( v < retcount )
				continue;
			id = itemdb_exists(items[0]);
			idx = id->combos_count;
			/* first entry, create */
			if( id->combos == NULL ) {
				CREATE(id->combos, struct item_combo*, 1);
				id->combos_count = 1;
			} else {
				RECREATE(id->combos, struct item_combo*, ++id->combos_count);
			}
			CREATE(id->combos[idx],struct item_combo,1);
			id->combos[idx]->nameid = (t_itemid *)aMalloc(retcount * sizeof(t_itemid));
			id->combos[idx]->count = retcount;
			id->combos[idx]->script = parse_script(str[1], path, lines, 0);
			id->combos[idx]->id = count;
			id->combos[idx]->isRef = false;
			/* populate ->nameid field */
			for( v = 0; v < retcount; v++ ) {
				id->combos[idx]->nameid[v] = items[v];
			}

			/* populate the children to refer to this combo */
			for( v = 1; v < retcount; v++ ) {
				struct item_data * it;
				int index;
				it = itemdb_exists(items[v]);
				index = it->combos_count;
				if( it->combos == NULL ) {
					CREATE(it->combos, struct item_combo*, 1);
					it->combos_count = 1;
				} else {
					RECREATE(it->combos, struct item_combo*, ++it->combos_count);
				}
				CREATE(it->combos[index],struct item_combo,1);
				/* we copy previously alloc'd pointers and just set it to reference */
				memcpy(it->combos[index],id->combos[idx],sizeof(struct item_combo));
				/* we flag this way to ensure we don't double-dealloc same data */
				it->combos[index]->isRef = true;
			}
			uidb_put(itemdb_combo,id->combos[idx]->id,id->combos[idx]);
		}
		count++;
	}
	fclose(fp);

	ShowStatus("Done reading '" CL_WHITE "%u" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n",count,path);

	return;
}

#ifdef Pandas_Crashfix_RouletteData_UnInit
//************************************
// Method:      itemdb_dummy_roulette_db
// Description: 禁用大乐透时的配置数据默认填充函数
// Parameter:   void
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/11 22:39
//************************************
bool itemdb_dummy_roulette_db(void) {
	// 由于 大乐透 功能可以在战斗配置选项中通过 feature.roulette 来禁用和启用
	// 如果 GM 先将 feature.roulette 设置为 off, 进入游戏后再修改 feature.roulette 为 on 并使用
	// @reloadbattleconf 来重新加载配置文件的话
	// 在进行过上述操作后, 点击大乐透按钮会导致地图服务器崩溃.
	//
	// 为了修复上面这个问题, 改造了一下大乐透数据的加载逻辑, 当没启用 feature.roulette 的时候
	// 会默认为 大乐透的数据变量 rd 填充上默认的 苹果
	// 这样再进行上述操作的时候, 就不会再出现崩溃的问题了, 虽然看起来显得繁琐

	int i, j;

	for (j = 0; j < MAX_ROULETTE_LEVEL; j++) {
		int limit = MAX_ROULETTE_COLUMNS - j;

		rd.items[j] = limit;
		RECREATE(rd.nameid[j], t_itemid, rd.items[j]);
		RECREATE(rd.qty[j], unsigned short, rd.items[j]);
		RECREATE(rd.flag[j], int, rd.items[j]);

		for (i = 0; i < rd.items[j]; i++) {
			rd.nameid[j][i] = ITEMID_APPLE;
			rd.qty[j][i] = 1;
			rd.flag[j][i] = 0;
		}
	}

	return true;
}
#endif // Pandas_Crashfix_RouletteData_UnInit

/**
 * Process Roulette items
 */
bool itemdb_parse_roulette_db(void)
{
	int i, j;
	uint32 count = 0;

	// retrieve all rows from the item database
	if (SQL_ERROR == Sql_Query(mmysql_handle, "SELECT * FROM `%s`", roulette_table)) {
		Sql_ShowDebug(mmysql_handle);
		return false;
	}

	for (i = 0; i < MAX_ROULETTE_LEVEL; i++)
		rd.items[i] = 0;

	for (i = 0; i < MAX_ROULETTE_LEVEL; i++) {
		int k, limit = MAX_ROULETTE_COLUMNS - i;

		for (k = 0; k < limit && SQL_SUCCESS == Sql_NextRow(mmysql_handle); k++) {
			char* data;
			t_itemid item_id;
			unsigned short amount;
			int level, flag;

			Sql_GetData(mmysql_handle, 1, &data, NULL); level = atoi(data);
			Sql_GetData(mmysql_handle, 2, &data, NULL); item_id = strtoul(data, nullptr, 10);
			Sql_GetData(mmysql_handle, 3, &data, NULL); amount = atoi(data);
			Sql_GetData(mmysql_handle, 4, &data, NULL); flag = atoi(data);

			if (!itemdb_exists(item_id)) {
				ShowWarning("itemdb_parse_roulette_db: Unknown item ID '%u' in level '%d'\n", item_id, level);
				continue;
			}
			if (amount < 1 || amount > MAX_AMOUNT){
				ShowWarning("itemdb_parse_roulette_db: Unsupported amount '%hu' for item ID '%u' in level '%d'\n", amount, item_id, level);
				continue;
			}
			if (flag < 0 || flag > 1) {
				ShowWarning("itemdb_parse_roulette_db: Unsupported flag '%d' for item ID '%u' in level '%d'\n", flag, item_id, level);
				continue;
			}

			j = rd.items[i];
			RECREATE(rd.nameid[i], t_itemid, ++rd.items[i]);
			RECREATE(rd.qty[i], unsigned short, rd.items[i]);
			RECREATE(rd.flag[i], int, rd.items[i]);

			rd.nameid[i][j] = item_id;
			rd.qty[i][j] = amount;
			rd.flag[i][j] = flag;

			++count;
		}
	}

	// free the query result
	Sql_FreeResult(mmysql_handle);

	for (i = 0; i < MAX_ROULETTE_LEVEL; i++) {
		int limit = MAX_ROULETTE_COLUMNS - i;

		if (rd.items[i] == limit)
			continue;

		if (rd.items[i] > limit) {
			ShowWarning("itemdb_parse_roulette_db: level %d has %d items, only %d supported, capping...\n", i + 1, rd.items[i], limit);
			rd.items[i] = limit;
			continue;
		}

		/** this scenario = rd.items[i] < limit **/
		ShowWarning("itemdb_parse_roulette_db: Level %d has %d items, %d are required. Filling with Apples...\n", i + 1, rd.items[i], limit);

		rd.items[i] = limit;
		RECREATE(rd.nameid[i], t_itemid, rd.items[i]);
		RECREATE(rd.qty[i], unsigned short, rd.items[i]);
		RECREATE(rd.flag[i], int, rd.items[i]);

		for (j = 0; j < MAX_ROULETTE_COLUMNS - i; j++) {
			if (rd.qty[i][j])
				continue;

			rd.nameid[i][j] = ITEMID_APPLE;
			rd.qty[i][j] = 1;
			rd.flag[i][j] = 0;
		}
	}

	ShowStatus("Done reading '" CL_WHITE "%u" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", count, roulette_table);

	return true;
}

/**
 * Free Roulette items
 */
static void itemdb_roulette_free(void) {
	int i;

	for (i = 0; i < MAX_ROULETTE_LEVEL; i++) {
		if (rd.nameid[i])
			aFree(rd.nameid[i]);
		if (rd.qty[i])
			aFree(rd.qty[i]);
		if (rd.flag[i])
			aFree(rd.flag[i]);
		rd.nameid[i] = NULL;
		rd.qty[i] = NULL;
		rd.flag[i] = NULL;
		rd.items[i] = 0;
	}
}

/*======================================
 * Applies gender restrictions according to settings. [Skotlex]
 *======================================*/
static char itemdb_gendercheck(struct item_data *id)
{
	if (id->nameid == WEDDING_RING_M) //Grom Ring
		return 1;
	if (id->nameid == WEDDING_RING_F) //Bride Ring
		return 0;
	if (id->look == W_MUSICAL && id->type == IT_WEAPON) //Musical instruments are always male-only
		return 1;
	if (id->look == W_WHIP && id->type == IT_WEAPON) //Whips are always female-only
		return 0;

	return (battle_config.ignore_items_gender) ? 2 : id->sex;
}

/**
 * [RRInd]
 * For backwards compatibility, in Renewal mode, MATK from weapons comes from the atk slot
 * We use a ':' delimiter which, if not found, assumes the weapon does not provide any matk.
 **/
static void itemdb_re_split_atoi(char *str, int *val1, int *val2) {
	int i, val[2];

	for (i=0; i<2; i++) {
		if (!str) break;
		val[i] = atoi(str);
		str = strchr(str,':');
		if (str)
			*str++=0;
	}
	if( i == 0 ) {
		*val1 = *val2 = 0;
		return;//no data found
	}
	if( i == 1 ) {//Single Value
		*val1 = val[0];
		*val2 = 0;
		return;
	}
	//We assume we have 2 values.
	*val1 = val[0];
	*val2 = val[1];
	return;
}

/**
* Processes one itemdb entry
*/
static bool itemdb_parse_dbrow(char** str, const char* source, int line, int scriptopt) {
	/*
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		| 00 |      01      |       02      |  03  |     04    |     05     |   06   |   07   |    08   |   09  |   10  |     11     |      12     |       13      |        14       |      15      |      16     |     17     |  18  |   19   |      20      |        21      |
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		| id | name_english | name_japanese | type | price_buy | price_sell | weight | attack | defence | range | slots | equip_jobs | equip_upper | equip_genders | equip_locations | weapon_level | equip_level | refineable | view | script | equip_script | unequip_script |
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
	*/
	t_itemid nameid;
	struct item_data* id;

	nameid = strtoul(str[0], nullptr, 10);

	if( nameid == 0 || nameid == dummy_item->nameid )
	{
		ShowWarning("itemdb_parse_dbrow: Invalid id %d in line %d of \"%s\", skipping.\n", nameid, line, source);
		return false;
	}

	//ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Job Upper,Gender,Loc,wLV,eLV,refineable,View
	if (!(id = itemdb_exists(nameid))) {
		// Checks if the Itemname is already taken by another id
		if( itemdb_searchname1(str[1], true) != NULL )
			ShowWarning("itemdb_parse_dbrow: Duplicate item name for \"%s\"\n", str[1]);

		// Adds a new Item ID
		id = itemdb_create_item(nameid);
	}

	safestrncpy(id->name, str[1], sizeof(id->name));
	safestrncpy(id->jname, str[2], sizeof(id->jname));

#ifdef Pandas_Speedup_Itemdb_SearchName
	itemdb_speedup_item(id);
#endif // Pandas_Speedup_Itemdb_SearchName

	id->type = atoi(str[3]);

	if( id->type < 0 || id->type == IT_UNKNOWN || id->type == IT_UNKNOWN2 || ( id->type > IT_SHADOWGEAR && id->type < IT_CASH ) || id->type >= IT_MAX )
	{// catch invalid item types
		ShowWarning("itemdb_parse_dbrow: Invalid item type %d for item %u. IT_ETC will be used.\n", id->type, nameid);
		id->type = IT_ETC;
	}

	if (id->type == IT_DELAYCONSUME)
	{	//Items that are consumed only after target confirmation
		id->type = IT_USABLE;
		id->flag.delay_consume = 1;
	} else //In case of an itemdb reload and the item type changed.
		id->flag.delay_consume = 0;

	//When a particular price is not given, we should base it off the other one
	//(it is important to make a distinction between 'no price' and 0z)
	if ( str[4][0] )
		id->value_buy = atoi(str[4]);
	else
		id->value_buy = atoi(str[5]) * 2;

	if ( str[5][0] )
		id->value_sell = atoi(str[5]);
	else
		id->value_sell = id->value_buy / 2;
	/*
	if ( !str[4][0] && !str[5][0])
	{
		ShowWarning("itemdb_parse_dbrow: No buying/selling price defined for item %u (%s), using 20/10z\n", nameid, id->jname);
		id->value_buy = 20;
		id->value_sell = 10;
	} else
	*/
	if (id->value_buy/124. < id->value_sell/75.)
		ShowWarning("itemdb_parse_dbrow: Buying/Selling [%d/%d] price of item %u (%s) allows Zeny making exploit  through buying/selling at discounted/overcharged prices!\n",
			id->value_buy, id->value_sell, nameid, id->jname);

	id->weight = atoi(str[6]);
#ifdef RENEWAL
	itemdb_re_split_atoi(str[7],&id->atk,&id->matk);
#else
	id->atk = atoi(str[7]);
#endif
	id->def = atoi(str[8]);
	id->range = atoi(str[9]);
	id->slot = atoi(str[10]);

	if (id->slot > MAX_SLOTS)
	{
		ShowWarning("itemdb_parse_dbrow: Item %u (%s) specifies %d slots, but the server only supports up to %d. Using %d slots.\n", nameid, id->jname, id->slot, MAX_SLOTS, MAX_SLOTS);
		id->slot = MAX_SLOTS;
	}

	itemdb_jobid2mapid(id->class_base, (uint64)strtoull(str[11],NULL,0));
	id->class_upper = atoi(str[12]);
	id->sex	= atoi(str[13]);
	id->equip = atoi(str[14]);

	if (!id->equip && itemdb_isequip2(id))
	{
		ShowWarning("Item %u (%s) is an equipment with no equip-field! Making it an etc item.\n", nameid, id->jname);
		id->type = IT_ETC;
	}

#ifndef Pandas_Shadowgear_Support_Card
	if( id->type != IT_SHADOWGEAR && id->equip&EQP_SHADOW_GEAR )
	{
		ShowWarning("Item %u (%s) have invalid equipment slot! Making it an etc item.\n", nameid, id->jname);
		id->type = IT_ETC;
	}
#endif // Pandas_Shadowgear_Support_Card

	id->wlv = cap_value(atoi(str[15]), REFINE_TYPE_ARMOR, REFINE_TYPE_MAX);
	itemdb_re_split_atoi(str[16],&id->elv,&id->elvmax);
	id->flag.no_refine = atoi(str[17]) ? 0 : 1; //FIXME: verify this
	id->look = atoi(str[18]);

	id->flag.available = 1;
	id->view_id = 0;
	id->sex = itemdb_gendercheck(id); //Apply gender filtering.

	if (id->script) {
		script_free_code(id->script);
		id->script = NULL;
	}
	if (id->equip_script) {
		script_free_code(id->equip_script);
		id->equip_script = NULL;
	}
	if (id->unequip_script) {
		script_free_code(id->unequip_script);
		id->unequip_script = NULL;
	}

	if (*str[19])
		id->script = parse_script(str[19], source, line, scriptopt);
	if (*str[20])
		id->equip_script = parse_script(str[20], source, line, scriptopt);
	if (*str[21])
		id->unequip_script = parse_script(str[21], source, line, scriptopt);

#ifdef Pandas_Struct_Item_Data_Taming_Mobid
	// 判断该道具的脚本是否调用了 pet 或 mpet 指令 [Sola丶小克]
	// 若确实有相关的调用, 则记录下此道具支持捕捉的魔物编号
	if (id->script != NULL) {
		if (!hasCatchPet(str[19], id->taming_mobid)) {
			id->taming_mobid.clear();
		}
	}
#endif // Pandas_Struct_Item_Data_Taming_Mobid

#ifdef Pandas_Struct_Item_Data_Has_CallFunc
	// 判断该道具的脚本是不是有 callfunc "xxxx"; 若有则记录一下 [Sola丶小克]
	if (id->script != NULL) {
		id->has_callfunc = (hasCallfunc(str[19]) ? 1 : 0);
	}
#endif // Pandas_Struct_Item_Data_Has_CallFunc

	if (!id->nameid) {
		id->nameid = nameid;
		uidb_put(itemdb, nameid, id);
	}
	return true;
}

/**
* Read item from item db
* item_db2 overwriting item_db
*/
static int itemdb_readdb(void){
	const char* filename[] = {
		DBPATH"item_db.txt",
		DBIMPORT"/item_db.txt" 
	};

	int fi;

	for( fi = 0; fi < ARRAYLENGTH(filename); ++fi ) {
		uint32 lines = 0, count = 0;
		char line[1024];

		char path[256];
		FILE* fp;

		sprintf(path, "%s/%s", db_path, filename[fi]);
		fp = fopen(path, "r");
		if( fp == NULL ) {
			ShowWarning("itemdb_readdb: File not found \"%s\", skipping.\n", path);
			continue;
		}

		// process rows one by one
		while(fgets(line, sizeof(line), fp))
		{
			char *str[32], *p;
			int i;
			lines++;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str, 0, sizeof(str));

			p = strstr(line,"//");

			if( p != nullptr ){
				*p = '\0';
			}

			p = line;
			while( ISSPACE(*p) )
				++p;
			if( *p == '\0' )
				continue;// empty line
			for( i = 0; i < 19; ++i )
			{
				str[i] = p;
				p = strchr(p,',');
				if( p == NULL )
					break;// comma not found
				*p = '\0';
				++p;
			}

			if( p == NULL )
			{
				ShowError("itemdb_readdb: Insufficient columns in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}

			// Script
			if( *p != '{' )
			{
				ShowError("itemdb_readdb: Invalid format (Script column) in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}
			str[19] = p + 1;
			p = strstr(p+1,"},");
			if( p == NULL )
			{
				ShowError("itemdb_readdb: Invalid format (Script column) in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}
			*p = '\0';
			p += 2;

			// OnEquip_Script
			if( *p != '{' )
			{
				ShowError("itemdb_readdb: Invalid format (OnEquip_Script column) in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}
			str[20] = p + 1;
			p = strstr(p+1,"},");
			if( p == NULL )
			{
				ShowError("itemdb_readdb: Invalid format (OnEquip_Script column) in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}
			*p = '\0';
			p += 2;

			// OnUnequip_Script (last column)
			if( *p != '{' )
			{
				ShowError("itemdb_readdb: Invalid format (OnUnequip_Script column) in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
				continue;
			}
			str[21] = p;
			p = &str[21][strlen(str[21]) - 2];

			if ( *p != '}' ) {
				/* lets count to ensure it's not something silly e.g. a extra space at line ending */
				int v, lcurly = 0, rcurly = 0;

				for( v = 0; v < strlen(str[21]); v++ ) {
					if( str[21][v] == '{' )
						lcurly++;
					else if (str[21][v] == '}') {
						rcurly++;
						p = &str[21][v];
					}
				}

				if( lcurly != rcurly ) {
					ShowError("itemdb_readdb: Mismatching curly braces in line %d of \"%s\" (item with id %d), skipping.\n", lines, path, atoi(str[0]));
					continue;
				}
			}
			str[21] = str[21] + 1;  //skip the first left curly
			*p = '\0';              //null the last right curly

			if (!itemdb_parse_dbrow(str, path, lines, SCRIPT_IGNORE_EXTERNAL_BRACKETS))
				continue;

			count++;
		}

		fclose(fp);

		ShowStatus("Done reading '" CL_WHITE "%u" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", count, path);
	}

	return 0;
}

/**
* Read item_db table
*/
static int itemdb_read_sqldb(void) {

	const char* item_db_name[] = {
		item_table,
		item2_table
	};
	int fi;

	for( fi = 0; fi < ARRAYLENGTH(item_db_name); ++fi ) {
		uint32 lines = 0, count = 0;

		// retrieve all rows from the item database
		if( SQL_ERROR == Sql_Query(mmysql_handle, "SELECT * FROM `%s`", item_db_name[fi]) ) {
			Sql_ShowDebug(mmysql_handle);
			continue;
		}

		// process rows one by one
		while( SQL_SUCCESS == Sql_NextRow(mmysql_handle) ) {// wrap the result into a TXT-compatible format
			char* str[22];
			char dummy[256] = "";
			int i;
			++lines;
			for( i = 0; i < 22; ++i ) {
				Sql_GetData(mmysql_handle, i, &str[i], NULL);
				if( str[i] == NULL )
					str[i] = dummy; // get rid of NULL columns
			}

			if (!itemdb_parse_dbrow(str, item_db_name[fi], lines, SCRIPT_IGNORE_EXTERNAL_BRACKETS))
				continue;
			++count;
		}

		// free the query result
		Sql_FreeResult(mmysql_handle);

		ShowStatus("Done reading '" CL_WHITE "%u" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", count, item_db_name[fi]);
	}

	return 0;
}

/** Check if the item is restricted by item_noequip.txt
* @param id Item that will be checked
* @param m Map ID
* @return true: can't be used; false: can be used
*/
bool itemdb_isNoEquip(struct item_data *id, uint16 m) {
	if (!id->flag.no_equip)
		return false;
	
	struct map_data *mapdata = map_getmapdata(m);

	if ((id->flag.no_equip&1 && !mapdata_flag_vs2(mapdata)) || // Normal
		(id->flag.no_equip&2 && mapdata->flag[MF_PVP]) || // PVP
		(id->flag.no_equip&4 && mapdata_flag_gvg2_no_te(mapdata)) || // GVG
		(id->flag.no_equip&8 && mapdata->flag[MF_BATTLEGROUND]) || // Battleground
		(id->flag.no_equip&16 && mapdata_flag_gvg2_te(mapdata)) || // WOE:TE
		(id->flag.no_equip&(mapdata->zone) && mapdata->flag[MF_RESTRICTED]) // Zone restriction
		)
		return true;
	return false;
}

/**
* Retrieves random option data
*/
struct s_random_opt_data* itemdb_randomopt_exists(short id) {
	return ((struct s_random_opt_data*)uidb_get(itemdb_randomopt, id));
}

/** Random option
* <ID>,<{Script}>
*/
static bool itemdb_read_randomopt(const char* basedir, bool silent) {
	uint32 lines = 0, count = 0;
	char line[1024];

	char path[256];
	FILE* fp;

	sprintf(path, "%s/%s", basedir, "item_randomopt_db.txt");

	if ((fp = fopen(path, "r")) == NULL) {
		if (silent == 0) ShowError("itemdb_read_randomopt: File not found \"%s\".\n", path);
		return false;
	}

	while (fgets(line, sizeof(line), fp)) {
		char *str[2], *p;

		lines++;

		if (line[0] == '/' && line[1] == '/') // Ignore comments
			continue;

		memset(str, 0, sizeof(str));

		p = line;

		p = trim(p);

		if (*p == '\0')
			continue;// empty line

		if (!strchr(p, ','))
		{
			ShowError("itemdb_read_randomopt: Insufficient columns in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		}

		str[0] = p;
		p = strchr(p, ',');
		*p = '\0';
		p++;

		str[1] = p;

		if (str[1][0] != '{') {
			ShowError("itemdb_read_randomopt(#1): Invalid format (Script column) in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		}
		/* no ending key anywhere (missing \}\) */
		if (str[1][strlen(str[1]) - 1] != '}') {
			ShowError("itemdb_read_randomopt(#2): Invalid format (Script column) in line %d of \"%s\", skipping.\n", lines, path);
			continue;
		}
		else {
			int id = -1;
			struct s_random_opt_data *data;
			struct script_code *code;

			str[0] = trim(str[0]);
			if (ISDIGIT(str[0][0])) {
				id = atoi(str[0]);
			}
			else {
				int64 id_tmp;

				if (!script_get_constant(str[0], &id_tmp)) {
					ShowError("itemdb_read_randopt: Unknown random option constant \"%s\".\n", str[0]);
					continue;
				}
				id = static_cast<int>(id_tmp);
			}

			if (id < 0) {
				ShowError("itemdb_read_randomopt: Invalid Random Option ID '%s' in line %d of \"%s\", skipping.\n", str[0], lines, path);
				continue;
			}

			if ((data = itemdb_randomopt_exists(id)) == NULL) {
				CREATE(data, struct s_random_opt_data, 1);
				uidb_put(itemdb_randomopt, id, data);
			}
			data->id = id;
			if ((code = parse_script(str[1], path, lines, 0)) == NULL) {
				ShowWarning("itemdb_read_randomopt: Invalid script on option ID #%d.\n", id);
				continue;
			}
			if (data->script) {
				script_free_code(data->script);
				data->script = NULL;
			}
			data->script = code;
		}
		count++;
	}
	fclose(fp);

	ShowStatus("Done reading '" CL_WHITE "%u" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", count, path);

	return true;
}

/**
 * Clear Item Random Option Group from memory
 * @author [Cydh]
 **/
static int itemdb_randomopt_group_free(DBKey key, DBData *data, va_list ap) {
	struct s_random_opt_group *g = (struct s_random_opt_group *)db_data2ptr(data);
	if (!g)
		return 0;
	if (g->entries)
		aFree(g->entries);
	g->entries = NULL;
	aFree(g);
	return 1;
}

/**
 * Get Item Random Option Group from itemdb_randomopt_group MapDB
 * @param id Random Option Group
 * @return Random Option Group data or NULL if not found
 * @author [Cydh]
 **/
struct s_random_opt_group *itemdb_randomopt_group_exists(int id) {
	return (struct s_random_opt_group *)uidb_get(itemdb_randomopt_group, id);
}

/**
 * Read Item Random Option Group from db file
 * @author [Cydh]
 **/
static bool itemdb_read_randomopt_group(char* str[], int columns, int current) {
	int64 id_tmp;
	int id = 0;
	int i;
	unsigned short rate = (unsigned short)strtoul(str[1], NULL, 10);
	struct s_random_opt_group *g = NULL;

	if (!script_get_constant(str[0], &id_tmp)) {
		ShowError("itemdb_read_randomopt_group: Invalid ID for Random Option Group '%s'.\n", str[0]);
		return false;
	}

	id = static_cast<int>(id_tmp);

	if ((columns-2)%3 != 0) {
		ShowError("itemdb_read_randomopt_group: Invalid column entries '%d'.\n", columns);
		return false;
	}

	if (!(g = (struct s_random_opt_group *)uidb_get(itemdb_randomopt_group, id))) {
		CREATE(g, struct s_random_opt_group, 1);
		g->id = id;
		g->total = 0;
		g->entries = NULL;
		uidb_put(itemdb_randomopt_group, g->id, g);
	}

	RECREATE(g->entries, struct s_random_opt_group_entry, g->total + rate);

	for (i = g->total; i < (g->total + rate); i++) {
		int j, k;
		memset(&g->entries[i].option, 0, sizeof(g->entries[i].option));
		for (j = 0, k = 2; k < columns && j < MAX_ITEM_RDM_OPT; k+=3) {
			int64 randid_tmp;
			int randid = 0;

			if (!script_get_constant(str[k], &randid_tmp) || ((randid = static_cast<int>(randid_tmp)) && !itemdb_randomopt_exists(randid))) {
				ShowError("itemdb_read_randomopt_group: Invalid random group id '%s' in column %d!\n", str[k], k+1);
				continue;
			}
			g->entries[i].option[j].id = randid;
			g->entries[i].option[j].value = (short)strtoul(str[k+1], NULL, 10);
			g->entries[i].option[j].param = (char)strtoul(str[k+2], NULL, 10);
			j++;
		}
	}
	g->total += rate;
	return true;
}

/**
* Read all item-related databases
*/
static void itemdb_read(void) {
	int i;
	const char* dbsubpath[] = {
		"",
		"/" DBIMPORT,
	};

	if (db_use_sqldbs)
		itemdb_read_sqldb();
	else
		itemdb_readdb();
	
	for(i=0; i<ARRAYLENGTH(dbsubpath); i++){
		uint8 n1 = (uint8)(strlen(db_path)+strlen(dbsubpath[i])+1);
		uint8 n2 = (uint8)(strlen(db_path)+strlen(DBPATH)+strlen(dbsubpath[i])+1);
		char* dbsubpath1 = (char*)aMalloc(n1+1);
		char* dbsubpath2 = (char*)aMalloc(n2+1);
		

		if(i==0) {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n2,"%s/%s%s",db_path,DBPATH,dbsubpath[i]);
		}
		else {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n1,"%s%s",db_path,dbsubpath[i]);
		}
		
		sv_readdb(dbsubpath1, "item_avail.txt",         ',', 2, 2, -1, &itemdb_read_itemavail, i > 0);
		sv_readdb(dbsubpath2, "item_stack.txt",         ',', 3, 3, -1, &itemdb_read_stack, i > 0);
		sv_readdb(dbsubpath1, "item_nouse.txt",         ',', 3, 3, -1, &itemdb_read_nouse, i > 0);
		sv_readdb(dbsubpath2, "item_group_db.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath2, "item_bluebox.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath2, "item_violetbox.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath2, "item_cardalbum.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath1, "item_findingore.txt",	',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath2, "item_giftbox.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
		sv_readdb(dbsubpath2, "item_misc.txt",			',', 2, 10, -1, &itemdb_read_group, i > 0);
#ifdef RENEWAL
		sv_readdb(dbsubpath2, "item_package.txt",		',', 2, 10, -1, &itemdb_read_group, i > 0);
#endif
		itemdb_read_combos(dbsubpath2,i > 0); //TODO change this to sv_read ? id#script ?
		itemdb_read_randomopt(dbsubpath2, i > 0);
		sv_readdb(dbsubpath2, "item_noequip.txt",       ',', 2, 2, -1, &itemdb_read_noequip, i > 0);
		sv_readdb(dbsubpath2, "item_trade.txt",         ',', 3, 3, -1, &itemdb_read_itemtrade, i > 0);
		sv_readdb(dbsubpath2, "item_delay.txt",         ',', 2, 3, -1, &itemdb_read_itemdelay, i > 0);
		sv_readdb(dbsubpath2, "item_buyingstore.txt",   ',', 1, 1, -1, &itemdb_read_buyingstore, i > 0);
		sv_readdb(dbsubpath2, "item_flag.txt",          ',', 2, 2, -1, &itemdb_read_flag, i > 0);
		sv_readdb(dbsubpath2, "item_randomopt_group.txt", ',', 5, 2+5*MAX_ITEM_RDM_OPT, -1, &itemdb_read_randomopt_group, i > 0);
		aFree(dbsubpath1);
		aFree(dbsubpath2);
	}
}

/*==========================================
 * Initialize / Finalize
 *------------------------------------------*/

/**
* Destroys the item_data.
*/
static void destroy_item_data(struct item_data* self) {
	if( self == NULL )
		return;
	// free scripts
	if( self->script )
		script_free_code(self->script);
	if( self->equip_script )
		script_free_code(self->equip_script);
	if( self->unequip_script )
		script_free_code(self->unequip_script);
	if( self->combos_count ) {
		int i;
		for( i = 0; i < self->combos_count; i++ ) {
			if( !self->combos[i]->isRef ) {
				aFree(self->combos[i]->nameid);
				if (self->combos[i]->script)
					script_free_code(self->combos[i]->script);
			}
			aFree(self->combos[i]);
		}
		aFree(self->combos);
	}
#if defined(DEBUG)
	// trash item
	memset(self, 0xDD, sizeof(struct item_data));
#endif
	// free self
	aFree(self);
}

/**
 * @see DBApply
 */
static int itemdb_final_sub(DBKey key, DBData *data, va_list ap)
{
	struct item_data *id = (struct item_data *)db_data2ptr(data);

	destroy_item_data(id);
	return 0;
}

/** NOTE:
* In some OSs, like Raspbian, we aren't allowed to pass 0 in va_list.
* So, itemdb_group_free2 is useful in some cases.
* NB : We keeping that funciton cause that signature is needed for some iterator..
*/
static int itemdb_group_free(DBKey key, DBData *data, va_list ap) {
	return itemdb_group_free2(key,data);
}

/** (ARM)
* Adaptation of itemdb_group_free. This function enables to compile rAthena on Raspbian OS.
*/
static inline int itemdb_group_free2(DBKey key, DBData *data) {
	struct s_item_group_db *group = (struct s_item_group_db *)db_data2ptr(data);
	uint8 j;
	if (!group)
		return 0;
	if (group->must_qty)
		aFree(group->must);
	group->must_qty = 0;
	for (j = 0; j < MAX_ITEMGROUP_RANDGROUP; j++) {
		if (!group->random[j].data_qty || !(&group->random[j]))
			continue;
		aFree(group->random[j].data);
		group->random[j].data_qty = 0;
	}
	aFree(group);
	return 0;
}

static int itemdb_randomopt_free(DBKey key, DBData *data, va_list ap) {
	struct s_random_opt_data *opt = (struct s_random_opt_data *)db_data2ptr(data);
	if (!opt)
		return 0;
	if (opt->script)
		script_free_code(opt->script);
	opt->script = NULL;
	aFree(opt);
	return 1;
}

bool item_data::isStackable()
{
	switch (this->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETEGG:
		case IT_PETARMOR:
		case IT_SHADOWGEAR:
			return false;
	}
	return true;
}

int item_data::inventorySlotNeeded(int quantity)
{
	return (this->flag.guid || !this->isStackable()) ? quantity : 1;
}

#ifdef Pandas_Struct_Item_Data_Properties
static int itemdb_property_parse(DBKey key, DBData *data, va_list ap) {
	struct item_data *item = (struct item_data *)db_data2ptr(data);
	if (item == nullptr) return 0;

	auto it = itemdb_get_property(item->nameid);
	if (it != nullptr) {
		item->properties.no_consume_of_player = ((it->property & 1) ? 1 : 0);
		item->properties.no_consume_of_skills = ((it->property & 2) ? 1 : 0);
		item->properties.is_amulet = ((it->property & 4) ? 1 : 0);
		item->properties.noview_mask = it->noview;
		item->properties.annouce_mask = it->annouce;
	}

#ifdef Pandas_Item_Amulet_System
	// 若为护身符道具, 则直接改写它的物品类型为 IT_AMULET
	if (item->properties.is_amulet)
		item->type = IT_AMULET;
#endif // Pandas_Item_Amulet_System

	return 0;
}
#endif // Pandas_Struct_Item_Data_Properties

/**
* Reload Item DB
*/
void itemdb_reload(void) {
	struct s_mapiterator* iter;
	struct map_session_data* sd;

	itemdb_group->clear(itemdb_group, itemdb_group_free);
	itemdb_randomopt->clear(itemdb_randomopt, itemdb_randomopt_free);
	itemdb_randomopt_group->clear(itemdb_randomopt_group, itemdb_randomopt_group_free);
	itemdb->clear(itemdb, itemdb_final_sub);
	db_clear(itemdb_combo);

#ifdef Pandas_Database_ItemProperties
	item_properties_db.clear();
#endif // Pandas_Database_ItemProperties

#ifdef Pandas_Speedup_Itemdb_SearchName
	itemdb_speedup_clear();
#endif // Pandas_Speedup_Itemdb_SearchName

	if (battle_config.feature_roulette)
		itemdb_roulette_free();

	// read new data
	itemdb_read();
	cashshop_reloaddb();

#ifdef Pandas_Database_ItemProperties
	// 加载 item_properties.yml 必须在 itemdb_read 之后进行
	// 因此加载过程中需要判断物品编号是否有效, 这需要依赖 itemdb_read 的执行结果
	item_properties_db.load();
	#ifdef Pandas_Struct_Item_Data_Properties
		itemdb->foreach(itemdb, itemdb_property_parse);
	#endif // Pandas_Struct_Item_Data_Properties
#endif // Pandas_Database_ItemProperties

	if (battle_config.feature_roulette)
		itemdb_parse_roulette_db();

#ifdef Pandas_Crashfix_RouletteData_UnInit
	// 若没有启用大乐透功能, 那么也初始化一组默认的配置数据
	if (!battle_config.feature_roulette)
		itemdb_dummy_roulette_db();
#endif // Pandas_Crashfix_RouletteData_UnInit

	mob_reload_itemmob_data();

	// readjust itemdb pointer cache for each player
	iter = mapit_geteachpc();
	for( sd = (struct map_session_data*)mapit_first(iter); mapit_exists(iter); sd = (struct map_session_data*)mapit_next(iter) ) {
		memset(sd->item_delay, 0, sizeof(sd->item_delay));  // reset item delays

		if( sd->combos.count ) { // clear combo bonuses
			aFree(sd->combos.bonus);
			aFree(sd->combos.id);
			aFree(sd->combos.pos);
			sd->combos.bonus = nullptr;
			sd->combos.id = nullptr;
			sd->combos.pos = nullptr;
			sd->combos.count = 0;
		}

		pc_setinventorydata(sd);
		pc_check_available_item(sd, ITMCHK_ALL); // Check for invalid(ated) items.
		pc_load_combo(sd); // Check to see if new combos are available

#ifdef Pandas_Database_ItemProperties
		// 当启用了道具特殊属性数据库的话, 重载物品信息后
		// 重新给每个玩家发放背包信息, 以便一些相关的设置能够直接生效
		clif_inventorylist(sd);
#endif // Pandas_Database_ItemProperties

		status_calc_pc(sd, SCO_FORCE); // 
	}
	mapit_free(iter);
}

/**
* Finalizing Item DB
*/
void do_final_itemdb(void) {
	db_destroy(itemdb_combo);
	itemdb_group->destroy(itemdb_group, itemdb_group_free);
	itemdb_randomopt->destroy(itemdb_randomopt, itemdb_randomopt_free);
	itemdb_randomopt_group->destroy(itemdb_randomopt_group, itemdb_randomopt_group_free);
	itemdb->destroy(itemdb, itemdb_final_sub);
	destroy_item_data(dummy_item);

#ifdef Pandas_Database_ItemProperties
	item_properties_db.clear();
#endif // Pandas_Database_ItemProperties

	if (battle_config.feature_roulette)
		itemdb_roulette_free();
}

/**
* Initializing Item DB
*/
void do_init_itemdb(void) {
	itemdb = uidb_alloc(DB_OPT_BASE);
	itemdb_combo = uidb_alloc(DB_OPT_BASE);
	itemdb_group = uidb_alloc(DB_OPT_BASE);
	itemdb_randomopt = uidb_alloc(DB_OPT_BASE);
	itemdb_randomopt_group = uidb_alloc(DB_OPT_BASE);
	itemdb_create_dummy();
	itemdb_read();

#ifdef Pandas_Database_ItemProperties
	// 加载 item_properties.yml 必须在 itemdb_read 之后进行
	// 因此加载过程中需要判断物品编号是否有效, 这需要依赖 itemdb_read 的执行结果
	item_properties_db.load();
	#ifdef Pandas_Struct_Item_Data_Properties
		itemdb->foreach(itemdb, itemdb_property_parse);
	#endif // Pandas_Struct_Item_Data_Properties
#endif // Pandas_Database_ItemProperties

	if (battle_config.feature_roulette)
		itemdb_parse_roulette_db();

#ifdef Pandas_Crashfix_RouletteData_UnInit
	// 若没有启用大乐透功能, 那么也初始化一组默认的配置数据
	if (!battle_config.feature_roulette)
		itemdb_dummy_roulette_db();
#endif // Pandas_Crashfix_RouletteData_UnInit
}

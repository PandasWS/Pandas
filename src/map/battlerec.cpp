// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "battlerec.hpp"

#include "mob.hpp"
#include "pet.hpp"
#include "homunculus.hpp"
#include "mercenary.hpp"
#include "elemental.hpp"
#include "status.hpp"

#include <common/utilities.hpp>
#include <common/nullpo.hpp>

//************************************
// Method:      batrec_key
// Description: 获取指定 block_list 并返回战斗记录字典的键
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     int32
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:44
//************************************ 
inline int32 batrec_key(struct block_list* bl) {
	// 若是玩家则使用角色编号作为键
	// 否则使用 BlockID (AKA: GameID) 作为键
	if (bl && bl->type == BL_PC) {
		return ((TBL_PC*)bl)->status.char_id;
	}
	return (bl ? bl->id : 0);
}

//************************************
// Method:      batrec_key
// Description: 获取指定 BlockID 并返回战斗记录字典的键
// Access:      public 
// Parameter:   uint32 block_id
// Returns:     int32
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:44
//************************************ 
inline int32 batrec_key(uint32 block_id) {
	struct block_list* bl = nullptr;
	bl = map_id2bl(block_id);
	return batrec_key(bl);
}

//************************************
// Method:      batrec_cmp_asc
// Description: 战斗记录进行升序排序的处理函数
// Access:      public 
// Parameter:   std::pair<uint32
// Parameter:   s_batrec_item_ptr> & l
// Parameter:   std::pair<uint32
// Parameter:   s_batrec_item_ptr> & r
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:44
//************************************ 
bool batrec_cmp_asc(std::pair<uint32, s_batrec_item_ptr>& l,
	std::pair<uint32, s_batrec_item_ptr>& r)
{
	if (l.second->damage != r.second->damage) {
		return l.second->damage < r.second->damage;
	}
	return l.first < r.first;
};

//************************************
// Method:      batrec_cmp_desc
// Description: 战斗记录进行降序排序的处理函数
// Access:      public 
// Parameter:   std::pair<uint32
// Parameter:   s_batrec_item_ptr> & l
// Parameter:   std::pair<uint32
// Parameter:   s_batrec_item_ptr> & r
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:44
//************************************ 
bool batrec_cmp_desc(std::pair<uint32, s_batrec_item_ptr>& l,
	std::pair<uint32, s_batrec_item_ptr>& r)
{
	if (l.second->damage != r.second->damage) {
		return l.second->damage > r.second->damage;
	}
	return l.first > r.first;
};

//************************************
// Method:      batrec_new
// Description: 创建指定单位的战斗记录字典
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:44
//************************************ 
void batrec_new(struct block_list* bl) {
	nullpo_retv(bl);

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) return;

	if (!batrec_support(bl)) {
		batrec_free(bl);
		ucd->batrec.dorecord = false;
		return;
	}

	if (!ucd->batrec.dmg_receive) {
		ucd->batrec.dmg_receive = new batrec_map;
	}
	if (!ucd->batrec.dmg_cause) {
		ucd->batrec.dmg_cause = new batrec_map;
	}

	ucd->batrec.dmg_receive->clear();
	ucd->batrec.dmg_cause->clear();

	ucd->batrec.dorecord = (
		(battle_config.batrec_autoenabled_unit & bl->type) == bl->type
	);
}

//************************************
// Method:      batrec_free
// Description: 释放指定单位的战斗记录字典
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/03/21 17:39
//************************************ 
void batrec_free(struct block_list* bl) {
	nullpo_retv(bl);

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) return;

	if (ucd->batrec.dmg_receive) {
		delete ucd->batrec.dmg_receive;
		ucd->batrec.dmg_receive = nullptr;
	}

	if (ucd->batrec.dmg_cause) {
		delete ucd->batrec.dmg_cause;
		ucd->batrec.dmg_cause = nullptr;
	}
}

//************************************
// Method:      batrec_sortout
// Description: 将指定单位的指定战斗记录中已经不存在(或下线)的交互目标记录移除
// Access:      public 
// Parameter:   struct block_list * bl
// Parameter:   e_batrec_type type
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
void batrec_sortout(struct block_list* bl, e_batrec_type type) {
	nullpo_retv(bl);

	batrec_map* rec = nullptr;
	if (!(rec = batrec_getmap(bl, type)))
		return;

	map_session_data* sd = nullptr;
	struct block_list* tbl = nullptr;

	auto iter = rec->begin();
	while (iter != rec->end())
	{
		tbl = nullptr;
		sd = map_charid2sd(iter->first);

		if (sd) tbl = &sd->bl;
		if (!tbl) tbl = map_id2bl(iter->first);
		if (!tbl) iter = rec->erase(iter);	// 单位不存在则移除
	}
}

//************************************
// Method:      batrec_sortout
// Description: 将指定单位的全部战斗记录中已经不存在(或下线)的交互目标记录移除
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/02/14 19:20
//************************************ 
void batrec_sortout(struct block_list* bl) {
	nullpo_retv(bl);

	batrec_sortout(bl, BRT_DMG_RECEIVE);
	batrec_sortout(bl, BRT_DMG_CAUSE);
}

//************************************
// Method:      batrec_masterid
// Description: 获取指定单位的主人的 BlockID, 以便聚合计算时参考
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     int32
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
inline int32 batrec_masterid(struct block_list* bl) {
	nullpo_retr(0, bl);

	if (!bl) return 0;

	TBL_PC* sd = nullptr;
	TBL_MOB* md = nullptr;
	TBL_HOM* hd = nullptr;
	TBL_PET* pd = nullptr;
	TBL_MER* mc = nullptr;
	TBL_ELEM* ed = nullptr;

	switch (bl->type)
	{
	case BL_MOB:
		if (!(md = map_id2md(bl->id))) break;
		return md->master_id;
	case BL_HOM:
		if (!(hd = map_id2hd(bl->id))) break;
		if (!(sd = map_charid2sd(hd->homunculus.char_id))) break;
		return sd->bl.id;
	case BL_PET:
		if (!(pd = map_id2pd(bl->id))) break;
		return pd->pet.account_id;
	case BL_MER:
		if (!(mc = map_id2mc(bl->id))) break;
		if (!(sd = map_charid2sd(mc->mercenary.char_id))) break;
		return sd->bl.id;
	case BL_ELEM :
		if (!(ed = map_id2ed(bl->id))) break;
		if (!(sd = map_charid2sd(ed->elemental.char_id))) break;
		return sd->bl.id;
	}
	return 0;
}

//************************************
// Method:      batrec_dorecord
// Description: 判断指定单位是否需要记录战斗信息
// Access:      public 
// Parameter:   struct block_list * bl
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/03/09 23:46
//************************************ 
bool batrec_dorecord(struct block_list* bl) {
	nullpo_retr(false, bl);

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) return false;

	// 如果该单位会进行战斗记录, 那么确保对应的字典是存在的
	if (ucd->batrec.dorecord) {
		if (!ucd->batrec.dmg_receive) {
			ucd->batrec.dmg_receive = new batrec_map;
		}
		if (!ucd->batrec.dmg_cause) {
			ucd->batrec.dmg_cause = new batrec_map;
		}
	}

	return ucd->batrec.dorecord;
}

//************************************
// Method:      batrec_getmap
// Description: 获取指定单位特定类型的战斗记录字典
// Access:      public 
// Parameter:   struct block_list * bl
// Parameter:   e_batrec_type type
// Returns:     batrec_map*
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
batrec_map* batrec_getmap(struct block_list* bl, e_batrec_type type) {
	nullpo_retr(nullptr, bl);

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) return nullptr;

	switch (type) {
	case BRT_DMG_RECEIVE:
		return ucd->batrec.dmg_receive;
	case BRT_DMG_CAUSE:
		return ucd->batrec.dmg_cause;
	default:
		return nullptr;
	}
}

//************************************
// Method:      batrec_aggregation
// Description: 对战斗记录数据进行聚合, 填充到 ret_rec 字典对象
// Access:      public 
// Parameter:   batrec_map * origin_rec
// Parameter:   batrec_map & ret_rec
// Parameter:   e_batrec_agg agg
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
void batrec_aggregation(batrec_map* origin_rec, batrec_map& ret_rec, e_batrec_agg agg) {
	nullpo_retv(origin_rec);

	if (agg != BRA_COMBINE) {
		ret_rec = *origin_rec;
		return;
	}
	ret_rec.clear();

	for (auto& record : *origin_rec) {
		if (!record.second->interactive_master_id) {
			// 没主人, 那么自己是独立的
			auto it = ret_rec.find(record.first);
			if (it == ret_rec.end()) {
				s_batrec_item_ptr item = std::make_shared<s_batrec_item>();
				item->interactive_block_id = record.second->interactive_block_id;
				item->interactive_block_type = record.second->interactive_block_type;
				item->interactive_master_id = 0;
				item->damage = record.second->damage;

				ret_rec.insert(std::make_pair(record.first, item));
			}
		}
		else {
			// 有主人, 将自己的值计入主人头上
			auto it = ret_rec.find(batrec_key(record.second->interactive_master_id));
			if (it == ret_rec.end()) {
				struct block_list* master_bl = nullptr;
				master_bl = map_id2bl(record.second->interactive_master_id);
				if (!master_bl) continue;

				s_batrec_item_ptr item = std::make_shared<s_batrec_item>();
				item->interactive_block_id = master_bl->id;
				item->interactive_block_type = master_bl->type;
				item->interactive_master_id = 0;
				item->damage = record.second->damage;

				ret_rec.insert(std::make_pair(batrec_key(record.second->interactive_master_id), item));
			}
			else {
				it->second->damage = rathena::util::safe_addition_cap(
					it->second->damage, record.second->damage, INT64_MAX
				);
			}
		}
	}

}

//************************************
// Method:      batrec_record
// Description: 在 mbl 的指定类型的战斗记录字典中写入 tbl 相关的伤害值
// Access:      public 
// Parameter:   struct block_list * mbl
// Parameter:   struct block_list * tbl
// Parameter:   e_batrec_type type
// Parameter:   int damage
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
bool batrec_record(struct block_list* mbl, struct block_list* tbl, e_batrec_type type, int damage) {
	nullpo_retr(false, mbl);
	nullpo_retr(false, tbl);

	if (!mbl || !tbl)
		return false;

	if (batrec_key(mbl) == batrec_key(tbl))
		return false;

	if (!batrec_dorecord(mbl))
		return false;

	batrec_map* rec = nullptr;
	if (!(rec = batrec_getmap(mbl, type)))
		return false;

	auto it = rec->find(batrec_key(tbl));

	if (it == rec->end()) {
		s_batrec_item_ptr item = std::make_shared<s_batrec_item>();
		item->interactive_block_id = tbl->id;
		item->interactive_block_type = tbl->type;
		item->interactive_master_id = batrec_masterid(tbl);
		item->damage = damage;

		rec->insert(std::make_pair(batrec_key(tbl), item));
	}
	else {
		it->second->damage = rathena::util::safe_addition_cap(
			it->second->damage, (int64)damage, INT64_MAX
		);
	}

	return true;
}

//************************************
// Method:      batrec_query
// Description: 查询 mbl 指定类型的战斗记录字典中与 id 相关的战斗记录值
// Access:      public 
// Parameter:   struct block_list * mbl
// Parameter:   uint32 id
// Parameter:   e_batrec_type type
// Parameter:   e_batrec_agg agg
// Returns:     int64
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
int64 batrec_query(struct block_list* mbl, uint32 id, e_batrec_type type, e_batrec_agg agg) {
	nullpo_retr(-1, mbl);

	batrec_map* origin_rec = nullptr;
	if (!(origin_rec = batrec_getmap(mbl, type)))
		return -1;

	batrec_map rec;
	batrec_aggregation(origin_rec, rec, agg);

	auto it = rec.find(batrec_key(id));
	if (it == rec.end()) {
		return -1;
	}

	return it->second->damage;
}

//************************************
// Method:      batrec_reset
// Description: 重置并清空指定单位的所有战斗记录字典
// Access:      public 
// Parameter:   struct block_list * mbl
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/03/21 15:27
//************************************ 
void batrec_reset(struct block_list* mbl) {
	nullpo_retv(mbl);

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(mbl))) return;

	if (ucd->batrec.dmg_receive) {
		ucd->batrec.dmg_receive->clear();
	}

	if (ucd->batrec.dmg_cause) {
		ucd->batrec.dmg_cause->clear();
	}
}

//************************************
// Method:      batrec_reset
// Description: 重置并清空指定单位的特定战斗记录字典
// Access:      public 
// Parameter:   struct block_list * mbl
// Parameter:   e_batrec_type type
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/02/14 13:43
//************************************ 
void batrec_reset(struct block_list* mbl, e_batrec_type type) {
	nullpo_retv(mbl);

	batrec_map* rec = nullptr;
	if (!(rec = batrec_getmap(mbl, type)))
		return;
	rec->clear();
}

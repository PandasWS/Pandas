#pragma once

#include "../common/serialize.hpp"

class ItemDatabase;
struct item_data;

class QuestDatabase;
struct s_quest_db;
struct s_quest_objective;
struct s_quest_dropitem;

class SkillDatabase;
struct s_skill_db;
struct s_skill_copyable;
struct s_skill_require;

namespace boost {
	namespace serialization {

		// ======================================================================
		// class ItemDatabase
		// ======================================================================

		template <typename Archive>
		void serialize(
			Archive& ar, ItemDatabase& t, const unsigned int version
		) {
			ar& boost::serialization::base_object<TypesafeCachedYamlDatabase<t_itemid, item_data>>(t);
		}

		// ======================================================================
		// struct item_data
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct item_data& t, const unsigned int version)
		{
			ar& t.nameid;
			ar& t.name;
			ar& t.ename;

			ar& t.value_buy;
			ar& t.value_sell;
			ar& t.type;
			ar& t.subtype;
			//ar& t.maxchance;				// ItemDatabase 默认不会为其赋值, 暂时无需处理
			ar& t.sex;
			ar& t.equip;
			ar& t.weight;
			ar& t.atk;
			ar& t.def;
			ar& t.range;
			ar& t.slots;
			ar& t.look;
			ar& t.elv;
			ar& t.wlv;
			ar& t.view_id;
			ar& t.elvmax;
#ifdef RENEWAL
			ar& t.matk;
#endif

			ar& t.class_base;
			ar& t.class_upper;

			ar& t.flag.available;
			//ar& t.flag.no_equip;			// ItemDatabase 默认不会为其赋值, 暂时无需处理
			ar& t.flag.no_refine;
			ar& t.flag.delay_consume;

			ar& t.flag.trade_restriction.drop;
			ar& t.flag.trade_restriction.trade;
			ar& t.flag.trade_restriction.trade_partner;
			ar& t.flag.trade_restriction.sell;
			ar& t.flag.trade_restriction.cart;
			ar& t.flag.trade_restriction.storage;
			ar& t.flag.trade_restriction.guild_storage;
			ar& t.flag.trade_restriction.mail;
			ar& t.flag.trade_restriction.auction;

			//ar& t.flag.autoequip;			// ItemDatabase 默认不会为其赋值, 暂时无需处理
			ar& t.flag.buyingstore;
			ar& t.flag.dead_branch;

			ar& t.flag.group;
			ar& t.flag.guid;
			ar& t.flag.broadcast;
			ar& t.flag.bindOnEquip;
			ar& t.flag.dropEffect;

			ar& t.stack.amount;
			ar& t.stack.inventory;
			ar& t.stack.cart;
			ar& t.stack.storage;
			ar& t.stack.guild_storage;

			ar& t.item_usage.override;
			ar& t.item_usage.sitting;

			ar& t.gm_lv_trade_override;
			//ar& t.combos;					// ItemDatabase 默认不会为其赋值, 暂时无需处理
			ar& t.delay.duration;
			ar& t.delay.sc;

#ifdef Pandas_Struct_Item_Data_Pandas

#ifdef Pandas_Struct_Item_Data_Script_Plaintext
			ar& t.pandas.script_plaintext.script;
			ar& t.pandas.script_plaintext.equip_script;
			ar& t.pandas.script_plaintext.unequip_script;
#endif // Pandas_Struct_Item_Data_Script_Plaintext

#ifdef Pandas_Struct_Item_Data_Taming_Mobid
			ar& t.pandas.taming_mobid;
#endif // Pandas_Struct_Item_Data_Taming_Mobid

#ifdef Pandas_Struct_Item_Data_Has_CallFunc
			ar& t.pandas.has_callfunc;
#endif // Pandas_Struct_Item_Data_Has_CallFunc

#ifdef Pandas_Struct_Item_Data_Properties
			ar& t.pandas.properties.avoid_use_consume;
			ar& t.pandas.properties.avoid_skill_consume;
			ar& t.pandas.properties.is_amulet;
			ar& t.pandas.properties.noview_mask;
			ar& t.pandas.properties.annouce_mask;
#endif // Pandas_Struct_Item_Data_Properties

#endif // Pandas_Struct_Item_Data_Pandas
		}

		// ======================================================================
		// class QuestDatabase
		// ======================================================================

		template <typename Archive>
		void serialize(
			Archive& ar, QuestDatabase& t, const unsigned int version
		) {
			ar& boost::serialization::base_object<TypesafeYamlDatabase<uint32, s_quest_db>>(t);
		}

		// ======================================================================
		// struct s_quest_db
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_db& t, const unsigned int version)
		{
			ar& t.id;
			ar& t.time;
			ar& t.time_at;
			ar& t.objectives;
			ar& t.dropitem;
			ar& t.name;
		}

		// ======================================================================
		// struct s_quest_objective
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_objective& t, const unsigned int version)
		{
			ar& t.index;
			ar& t.mob_id;
			ar& t.count;
			ar& t.min_level;
			ar& t.max_level;
			ar& t.race;
			ar& t.size;
			ar& t.element;
		}

		// ======================================================================
		// struct s_quest_dropitem
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_dropitem& t, const unsigned int version)
		{
			ar& t.nameid;
			ar& t.count;
			ar& t.rate;
			ar& t.mob_id;
		}

		// ======================================================================
		// class SkillDatabase
		// ======================================================================

		template <typename Archive>
		void serialize(
			Archive& ar, SkillDatabase& t, const unsigned int version
		) {
			ar& boost::serialization::base_object<TypesafeCachedYamlDatabase<uint16, s_skill_db>>(t);
		}

		// ======================================================================
		// struct s_skill_db
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_skill_db& t, const unsigned int version)
		{
			ar& t.nameid;
			ar& t.name;
			ar& t.desc;
			ar& t.range;
			ar& t.hit;
			ar& t.inf;
			ar& t.element;
			ar& t.nk;
			ar& t.splash;
			ar& t.max;
			ar& t.num;
			ar& t.castcancel;
			ar& t.cast_def_rate;
			ar& t.skill_type;
			ar& t.blewcount;
			ar& t.inf2;
			ar& t.maxcount;

			ar& t.castnodex;
			ar& t.delaynodex;

			//ar& t.nocast;					// SkillDatabase 默认不会为其赋值, 暂时无需处理

			ar& t.unit_id;
			ar& t.unit_id2;
			ar& t.unit_layout_type;
			ar& t.unit_range;
			ar& t.unit_interval;
			ar& t.unit_target;
			ar& t.unit_flag;

			ar& t.cast;
			ar& t.delay;
			ar& t.walkdelay;
			ar& t.upkeep_time;
			ar& t.upkeep_time2;
			ar& t.cooldown;
#ifdef RENEWAL_CAST
			ar& t.fixed_cast;
#endif // RENEWAL_CAST

			ar& t.require;

			ar& t.unit_nonearnpc_range;
			ar& t.unit_nonearnpc_type;

			//ar& t.damage;					// SkillDatabase 默认不会为其赋值, 暂时无需处理
			ar& t.copyable;

			//ar& t.abra_probability;		// SkillDatabase 默认不会为其赋值, 暂时无需处理
			//ar& t.reading_spellbook;		// SkillDatabase 默认不会为其赋值, 暂时无需处理
			//ar& t.improvisedsong_rate;	// SkillDatabase 默认不会为其赋值, 暂时无需处理
		}

		// ======================================================================
		// struct s_skill_copyable
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_skill_copyable& t, const unsigned int version)
		{
			ar& t.option;
			ar& t.req_opt;
		}

		// ======================================================================
		// struct s_skill_require
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_skill_require& t, const unsigned int version)
		{
			ar& t.hp;
			ar& t.mhp;
			ar& t.sp;
			ar& t.hp_rate;
			ar& t.sp_rate;
			ar& t.zeny;
			ar& t.weapon;
			ar& t.ammo;
			ar& t.ammo_qty;
			ar& t.state;
			ar& t.spiritball;
			ar& t.itemid;
			ar& t.amount;
			ar& t.eqItem;
			ar& t.status;
		}
	} // namespace serialization
} // namespace boost

// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CSV2YAML_HPP
#define CSV2YAML_HPP

#include "yaml.hpp"

// Required constant and structure definitions
#define MAX_GUILD_SKILL_REQUIRE 5
#define MAX_SKILL_ITEM_REQUIRE	10
#define MAX_SKILL_STATUS_REQUIRE 3
#define MAX_SKILL_EQUIP_REQUIRE 10
#define MAX_QUEST_DROPS 3
#define MAX_MAP_PER_INSTANCE 255

// Database to memory maps
struct s_skill_unit_csv : s_skill_db {
	std::string target_str;
	int unit_flag_csv;
};

std::unordered_map<uint16, s_skill_require> skill_require;
std::unordered_map<uint16, s_skill_db> skill_cast;
std::unordered_map<uint16, s_skill_db> skill_castnodex;
std::unordered_map<uint16, s_skill_unit_csv> skill_unit;
std::unordered_map<uint16, s_skill_copyable> skill_copyable;
std::unordered_map<uint16, s_skill_db> skill_nearnpc;

static unsigned int level_penalty[3][CLASS_MAX][MAX_LEVEL * 2 + 1];

struct s_item_flag_csv2yaml {
	bool buyingstore, dead_branch, group, guid, broadcast, bindOnEquip, delay_consume;
	e_item_drop_effect dropEffect;
};

struct s_item_delay_csv2yaml {
	uint32 delay;
	std::string sc;
};

struct s_item_stack_csv2yaml {
	uint16 amount;
	bool inventory, cart, storage, guild_storage;
};

struct s_item_nouse_csv2yaml {
	uint16 override;
	bool sitting;
};

struct s_item_trade_csv2yaml {
	uint16 override;
	bool drop, trade, trade_partner, sell, cart, storage, guild_storage, mail, auction;
};

std::unordered_map<t_itemid, t_itemid> item_avail;
std::unordered_map<t_itemid, bool> item_buyingstore;
std::unordered_map<t_itemid, s_item_flag_csv2yaml> item_flag;
std::unordered_map<t_itemid, s_item_delay_csv2yaml> item_delay;
std::unordered_map<t_itemid, s_item_stack_csv2yaml> item_stack;
std::unordered_map<t_itemid, s_item_nouse_csv2yaml> item_nouse;
std::unordered_map<t_itemid, s_item_trade_csv2yaml> item_trade;

struct s_random_opt_group_csv : s_random_opt_group {
	std::vector<uint16> rate;
};

std::unordered_map<uint16, std::string> rand_opt_db;
std::unordered_map<uint16, s_random_opt_group_csv> rand_opt_group;

static std::map<std::string, int> um_mapid2jobname {
	{ "Novice", JOB_NOVICE }, // Novice and Super Novice share the same value
	{ "SuperNovice", JOB_NOVICE },
	{ "Swordman", JOB_SWORDMAN },
	{ "Mage", JOB_MAGE },
	{ "Archer", JOB_ARCHER },
	{ "Acolyte", JOB_ACOLYTE },
	{ "Merchant", JOB_MERCHANT },
	{ "Thief", JOB_THIEF },
	{ "Knight", JOB_KNIGHT },
	{ "Priest", JOB_PRIEST },
	{ "Wizard", JOB_WIZARD },
	{ "Blacksmith", JOB_BLACKSMITH },
	{ "Hunter", JOB_HUNTER },
	{ "Assassin", JOB_ASSASSIN },
	{ "Crusader", JOB_CRUSADER },
	{ "Monk", JOB_MONK },
	{ "Sage", JOB_SAGE },
	{ "Rogue", JOB_ROGUE },
	{ "Alchemist", JOB_ALCHEMIST },
	{ "BardDancer", JOB_BARD }, // Bard and Dancer share the same value
	{ "BardDancer", JOB_DANCER },
	{ "Gunslinger", JOB_GUNSLINGER },
	{ "Ninja", JOB_NINJA },
	{ "Taekwon", 21 },
	{ "StarGladiator", 22 },
	{ "SoulLinker", 23 },
//	{ "Gangsi", 26 },
//	{ "DeathKnight", 27 },
//	{ "DarkCollector", 28 },
#ifdef RENEWAL
	{ "KagerouOboro", 29 }, // Kagerou and Oboro share the same value
	{ "Rebellion", 30 },
	{ "Summoner", 31 },
#endif
};

static std::unordered_map<std::string, equip_pos> um_equipnames {
	{ "Head_Low", EQP_HEAD_LOW },
	{ "Head_Mid", EQP_HEAD_MID },
	{ "Head_Top", EQP_HEAD_TOP },
	{ "Right_Hand", EQP_HAND_R },
	{ "Left_Hand", EQP_HAND_L },
	{ "Armor", EQP_ARMOR },
	{ "Shoes", EQP_SHOES },
	{ "Garment", EQP_GARMENT },
	{ "Right_Accessory", EQP_ACC_R },
	{ "Left_Accessory", EQP_ACC_L },
	{ "Costume_Head_Top", EQP_COSTUME_HEAD_TOP },
	{ "Costume_Head_Mid", EQP_COSTUME_HEAD_MID },
	{ "Costume_Head_Low", EQP_COSTUME_HEAD_LOW },
	{ "Costume_Garment", EQP_COSTUME_GARMENT },
	{ "Ammo", EQP_AMMO },
	{ "Shadow_Armor", EQP_SHADOW_ARMOR },
	{ "Shadow_Weapon", EQP_SHADOW_WEAPON },
	{ "Shadow_Shield", EQP_SHADOW_SHIELD },
	{ "Shadow_Shoes", EQP_SHADOW_SHOES },
	{ "Shadow_Right_Accessory", EQP_SHADOW_ACC_R },
	{ "Shadow_Left_Accessory", EQP_SHADOW_ACC_L },
};

// Initialize Random Option constants
void init_random_option_constants() {
	#define export_constant2(a, b) script_set_constant_(a, b, a, false, false)

	export_constant2("RDMOPT_VAR_MAXHPAMOUNT", 1);
	export_constant2("RDMOPT_VAR_MAXSPAMOUNT", 2);
	export_constant2("RDMOPT_VAR_STRAMOUNT", 3);
	export_constant2("RDMOPT_VAR_AGIAMOUNT", 4);
	export_constant2("RDMOPT_VAR_VITAMOUNT", 5);
	export_constant2("RDMOPT_VAR_INTAMOUNT", 6);
	export_constant2("RDMOPT_VAR_DEXAMOUNT", 7);
	export_constant2("RDMOPT_VAR_LUKAMOUNT", 8);
	export_constant2("RDMOPT_VAR_MAXHPPERCENT", 9);
	export_constant2("RDMOPT_VAR_MAXSPPERCENT", 10);
	export_constant2("RDMOPT_VAR_HPACCELERATION", 11);
	export_constant2("RDMOPT_VAR_SPACCELERATION", 12);
	export_constant2("RDMOPT_VAR_ATKPERCENT", 13);
	export_constant2("RDMOPT_VAR_MAGICATKPERCENT", 14);
	export_constant2("RDMOPT_VAR_PLUSASPD", 15);
	export_constant2("RDMOPT_VAR_PLUSASPDPERCENT", 16);
	export_constant2("RDMOPT_VAR_ATTPOWER", 17);
	export_constant2("RDMOPT_VAR_HITSUCCESSVALUE", 18);
	export_constant2("RDMOPT_VAR_ATTMPOWER", 19);
	export_constant2("RDMOPT_VAR_ITEMDEFPOWER", 20);
	export_constant2("RDMOPT_VAR_MDEFPOWER", 21);
	export_constant2("RDMOPT_VAR_AVOIDSUCCESSVALUE", 22);
	export_constant2("RDMOPT_VAR_PLUSAVOIDSUCCESSVALUE", 23);
	export_constant2("RDMOPT_VAR_CRITICALSUCCESSVALUE", 24);
	export_constant2("RDMOPT_ATTR_TOLERACE_NOTHING", 25);
	export_constant2("RDMOPT_ATTR_TOLERACE_WATER", 26);
	export_constant2("RDMOPT_ATTR_TOLERACE_GROUND", 27);
	export_constant2("RDMOPT_ATTR_TOLERACE_FIRE", 28);
	export_constant2("RDMOPT_ATTR_TOLERACE_WIND", 29);
	export_constant2("RDMOPT_ATTR_TOLERACE_POISON", 30);
	export_constant2("RDMOPT_ATTR_TOLERACE_SAINT", 31);
	export_constant2("RDMOPT_ATTR_TOLERACE_DARKNESS", 32);
	export_constant2("RDMOPT_ATTR_TOLERACE_TELEKINESIS", 33);
	export_constant2("RDMOPT_ATTR_TOLERACE_UNDEAD", 34);
	export_constant2("RDMOPT_ATTR_TOLERACE_ALLBUTNOTHING", 35);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_NOTHING_USER", 36);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_NOTHING_TARGET", 37);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_WATER_USER", 38);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_WATER_TARGET", 39);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_GROUND_USER", 40);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_GROUND_TARGET", 41);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_FIRE_USER", 42);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_FIRE_TARGET", 43);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_WIND_USER", 44);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_WIND_TARGET", 45);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_POISON_USER", 46);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_POISON_TARGET", 47);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_SAINT_USER", 48);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_SAINT_TARGET", 49);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_DARKNESS_USER", 50);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_DARKNESS_TARGET", 51);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_TELEKINESIS_USER", 52);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_TELEKINESIS_TARGET", 53);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_UNDEAD_USER", 54);
	export_constant2("RDMOPT_DAMAGE_PROPERTY_UNDEAD_TARGET", 55);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_NOTHING_USER", 56);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_NOTHING_TARGET", 57);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_WATER_USER", 58);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_WATER_TARGET", 59);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_GROUND_USER", 60);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_GROUND_TARGET", 61);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_FIRE_USER", 62);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_FIRE_TARGET", 63);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_WIND_USER", 64);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_WIND_TARGET", 65);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_POISON_USER", 66);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_POISON_TARGET", 67);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_SAINT_USER", 68);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_SAINT_TARGET", 69);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_DARKNESS_USER", 70);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_DARKNESS_TARGET", 71);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_TELEKINESIS_USER", 72);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_TELEKINESIS_TARGET", 73);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_UNDEAD_USER", 74);
	export_constant2("RDMOPT_MDAMAGE_PROPERTY_UNDEAD_TARGET", 75);
	export_constant2("RDMOPT_BODY_ATTR_NOTHING", 76);
	export_constant2("RDMOPT_BODY_ATTR_WATER", 77);
	export_constant2("RDMOPT_BODY_ATTR_GROUND", 78);
	export_constant2("RDMOPT_BODY_ATTR_FIRE", 79);
	export_constant2("RDMOPT_BODY_ATTR_WIND", 80);
	export_constant2("RDMOPT_BODY_ATTR_POISON", 81);
	export_constant2("RDMOPT_BODY_ATTR_SAINT", 82);
	export_constant2("RDMOPT_BODY_ATTR_DARKNESS", 83);
	export_constant2("RDMOPT_BODY_ATTR_TELEKINESIS", 84);
	export_constant2("RDMOPT_BODY_ATTR_UNDEAD", 85);
	export_constant2("RDMOPT_RACE_TOLERACE_NOTHING", 87);
	export_constant2("RDMOPT_RACE_TOLERACE_UNDEAD", 88);
	export_constant2("RDMOPT_RACE_TOLERACE_ANIMAL", 89);
	export_constant2("RDMOPT_RACE_TOLERACE_PLANT", 90);
	export_constant2("RDMOPT_RACE_TOLERACE_INSECT", 91);
	export_constant2("RDMOPT_RACE_TOLERACE_FISHS", 92);
	export_constant2("RDMOPT_RACE_TOLERACE_DEVIL", 93);
	export_constant2("RDMOPT_RACE_TOLERACE_HUMAN", 94);
	export_constant2("RDMOPT_RACE_TOLERACE_ANGEL", 95);
	export_constant2("RDMOPT_RACE_TOLERACE_DRAGON", 96);
	export_constant2("RDMOPT_RACE_DAMAGE_NOTHING", 97);
	export_constant2("RDMOPT_RACE_DAMAGE_UNDEAD", 98);
	export_constant2("RDMOPT_RACE_DAMAGE_ANIMAL", 99);
	export_constant2("RDMOPT_RACE_DAMAGE_PLANT", 100);
	export_constant2("RDMOPT_RACE_DAMAGE_INSECT", 101);
	export_constant2("RDMOPT_RACE_DAMAGE_FISHS", 102);
	export_constant2("RDMOPT_RACE_DAMAGE_DEVIL", 103);
	export_constant2("RDMOPT_RACE_DAMAGE_HUMAN", 104);
	export_constant2("RDMOPT_RACE_DAMAGE_ANGEL", 105);
	export_constant2("RDMOPT_RACE_DAMAGE_DRAGON", 106);
	export_constant2("RDMOPT_RACE_MDAMAGE_NOTHING", 107);
	export_constant2("RDMOPT_RACE_MDAMAGE_UNDEAD", 108);
	export_constant2("RDMOPT_RACE_MDAMAGE_ANIMAL", 109);
	export_constant2("RDMOPT_RACE_MDAMAGE_PLANT", 110);
	export_constant2("RDMOPT_RACE_MDAMAGE_INSECT", 111);
	export_constant2("RDMOPT_RACE_MDAMAGE_FISHS", 112);
	export_constant2("RDMOPT_RACE_MDAMAGE_DEVIL", 113);
	export_constant2("RDMOPT_RACE_MDAMAGE_HUMAN", 114);
	export_constant2("RDMOPT_RACE_MDAMAGE_ANGEL", 115);
	export_constant2("RDMOPT_RACE_MDAMAGE_DRAGON", 116);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_NOTHING", 117);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_UNDEAD", 118);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_ANIMAL", 119);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_PLANT", 120);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_INSECT", 121);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_FISHS", 122);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_DEVIL", 123);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_HUMAN", 124);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_ANGEL", 125);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_DRAGON", 126);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_NOTHING", 127);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_UNDEAD", 128);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_ANIMAL", 129);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_PLANT", 130);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_INSECT", 131);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_FISHS", 132);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_DEVIL", 133);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_HUMAN", 134);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_ANGEL", 135);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_DRAGON", 136);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_NOTHING", 137);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_UNDEAD", 138);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_ANIMAL", 139);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_PLANT", 140);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_INSECT", 141);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_FISHS", 142);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_DEVIL", 143);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_HUMAN", 144);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_ANGEL", 145);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_DRAGON", 146);
	export_constant2("RDMOPT_CLASS_DAMAGE_NORMAL_TARGET", 147);
	export_constant2("RDMOPT_CLASS_DAMAGE_BOSS_TARGET", 148);
	export_constant2("RDMOPT_CLASS_DAMAGE_NORMAL_USER", 149);
	export_constant2("RDMOPT_CLASS_DAMAGE_BOSS_USER", 150);
	export_constant2("RDMOPT_CLASS_MDAMAGE_NORMAL", 151);
	export_constant2("RDMOPT_CLASS_MDAMAGE_BOSS", 152);
	export_constant2("RDMOPT_CLASS_IGNORE_DEF_PERCENT_NORMAL", 153);
	export_constant2("RDMOPT_CLASS_IGNORE_DEF_PERCENT_BOSS", 154);
	export_constant2("RDMOPT_CLASS_IGNORE_MDEF_PERCENT_NORMAL", 155);
	export_constant2("RDMOPT_CLASS_IGNORE_MDEF_PERCENT_BOSS", 156);
	export_constant2("RDMOPT_DAMAGE_SIZE_SMALL_TARGET", 157);
	export_constant2("RDMOPT_DAMAGE_SIZE_MIDIUM_TARGET", 158);
	export_constant2("RDMOPT_DAMAGE_SIZE_LARGE_TARGET", 159);
	export_constant2("RDMOPT_DAMAGE_SIZE_SMALL_USER", 160);
	export_constant2("RDMOPT_DAMAGE_SIZE_MIDIUM_USER", 161);
	export_constant2("RDMOPT_DAMAGE_SIZE_LARGE_USER", 162);
	export_constant2("RDMOPT_DAMAGE_SIZE_PERFECT", 163);
	export_constant2("RDMOPT_DAMAGE_CRI_TARGET", 164);
	export_constant2("RDMOPT_DAMAGE_CRI_USER", 165);
	export_constant2("RDMOPT_RANGE_ATTACK_DAMAGE_TARGET", 166);
	export_constant2("RDMOPT_RANGE_ATTACK_DAMAGE_USER", 167);
	export_constant2("RDMOPT_HEAL_VALUE", 168);
	export_constant2("RDMOPT_HEAL_MODIFY_PERCENT", 169);
	export_constant2("RDMOPT_DEC_SPELL_CAST_TIME", 170);
	export_constant2("RDMOPT_DEC_SPELL_DELAY_TIME", 171);
	export_constant2("RDMOPT_DEC_SP_CONSUMPTION", 172);
	export_constant2("RDMOPT_WEAPON_ATTR_NOTHING", 175);
	export_constant2("RDMOPT_WEAPON_ATTR_WATER", 176);
	export_constant2("RDMOPT_WEAPON_ATTR_GROUND", 177);
	export_constant2("RDMOPT_WEAPON_ATTR_FIRE", 178);
	export_constant2("RDMOPT_WEAPON_ATTR_WIND", 179);
	export_constant2("RDMOPT_WEAPON_ATTR_POISON", 180);
	export_constant2("RDMOPT_WEAPON_ATTR_SAINT", 181);
	export_constant2("RDMOPT_WEAPON_ATTR_DARKNESS", 182);
	export_constant2("RDMOPT_WEAPON_ATTR_TELEKINESIS", 183);
	export_constant2("RDMOPT_WEAPON_ATTR_UNDEAD", 184);
	export_constant2("RDMOPT_WEAPON_INDESTRUCTIBLE", 185);
	export_constant2("RDMOPT_BODY_INDESTRUCTIBLE", 186);
	export_constant2("RDMOPT_MDAMAGE_SIZE_SMALL_TARGET", 187);
	export_constant2("RDMOPT_MDAMAGE_SIZE_MIDIUM_TARGET", 188);
	export_constant2("RDMOPT_MDAMAGE_SIZE_LARGE_TARGET", 189);
	export_constant2("RDMOPT_MDAMAGE_SIZE_SMALL_USER", 190);
	export_constant2("RDMOPT_MDAMAGE_SIZE_MIDIUM_USER", 191);
	export_constant2("RDMOPT_MDAMAGE_SIZE_LARGE_USER", 192);
	export_constant2("RDMOPT_ATTR_TOLERACE_ALL", 193);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_NOTHING", 194);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_UNDEAD", 195);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_ANIMAL", 196);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_PLANT", 197);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_INSECT", 198);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_FISHS", 199);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_DEVIL", 200);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_HUMAN", 201);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_ANGEL", 202);
	export_constant2("RDMOPT_RACE_WEAPON_TOLERACE_DRAGON", 203);
	export_constant2("RDMOPT_RACE_TOLERACE_PLAYER_HUMAN", 206);
	export_constant2("RDMOPT_RACE_TOLERACE_PLAYER_DORAM", 207);
	export_constant2("RDMOPT_RACE_DAMAGE_PLAYER_HUMAN", 208);
	export_constant2("RDMOPT_RACE_DAMAGE_PLAYER_DORAM", 209);
	export_constant2("RDMOPT_RACE_MDAMAGE_PLAYER_HUMAN", 210);
	export_constant2("RDMOPT_RACE_MDAMAGE_PLAYER_DORAM", 211);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_PLAYER_HUMAN", 212);
	export_constant2("RDMOPT_RACE_CRI_PERCENT_PLAYER_DORAM", 213);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_PLAYER_HUMAN", 214);
	export_constant2("RDMOPT_RACE_IGNORE_DEF_PERCENT_PLAYER_DORAM", 215);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_PLAYER_HUMAN", 216);
	export_constant2("RDMOPT_RACE_IGNORE_MDEF_PERCENT_PLAYER_DORAM", 217);
	export_constant2("RDMOPT_MELEE_ATTACK_DAMAGE_TARGET", 219);
	export_constant2("RDMOPT_MELEE_ATTACK_DAMAGE_USER", 220);

	#undef export_constant2
}

static bool guild_read_guildskill_tree_db( char* split[], int columns, int current );
static bool pet_read_db( const char* file );
static bool skill_parse_row_magicmushroomdb(char *split[], int column, int current);
static bool skill_parse_row_abradb(char* split[], int columns, int current);
static bool skill_parse_row_spellbookdb(char* split[], int columns, int current);
static bool mob_readdb_mobavail(char *str[], int columns, int current);
static bool skill_parse_row_requiredb(char* split[], int columns, int current);
static bool skill_parse_row_castdb(char* split[], int columns, int current);
static bool skill_parse_row_castnodexdb(char* split[], int columns, int current);
static bool skill_parse_row_unitdb(char* split[], int columns, int current);
static bool skill_parse_row_copyabledb(char* split[], int columns, int current);
static bool skill_parse_row_nonearnpcrangedb(char* split[], int columns, int current);
static bool skill_parse_row_skilldb(char* split[], int columns, int current);
static bool quest_read_db(char *split[], int columns, int current);
static bool instance_readdb_sub(char* str[], int columns, int current);
static bool itemdb_read_itemavail(char *str[], int columns, int current);
static bool itemdb_read_buyingstore(char* fields[], int columns, int current);
static bool itemdb_read_flag(char* fields[], int columns, int current);
static bool itemdb_read_itemdelay(char* str[], int columns, int current);
static bool itemdb_read_stack(char* fields[], int columns, int current);
static bool itemdb_read_nouse(char* fields[], int columns, int current);
static bool itemdb_read_itemtrade(char* fields[], int columns, int current);
static bool itemdb_read_db(const char *file);
static bool itemdb_read_randomopt(const char* file);
static bool itemdb_read_randomopt_group(char *str[], int columns, int current);
static bool itemdb_randomopt_group_yaml(void);
static bool pc_readdb_levelpenalty(char* fields[], int columns, int current);
static bool pc_levelpenalty_yaml();

#endif /* CSV2YAML_HPP */

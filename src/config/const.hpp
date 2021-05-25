// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CONFIG_CONST_H
#define CONFIG_CONST_H

#include "../common/cbasetypes.hpp"

/**
 * rAthena configuration file (http://rathena.org)
 * For detailed guidance on these check http://rathena.org/wiki/SRC/config/
 **/

/**
 * @INFO: This file holds constants that aims at making code smoother and more efficient
 */

/**
 * "Sane Checks" to save you from compiling with cool bugs
 **/
#if SECURE_NPCTIMEOUT_INTERVAL <= 0
	#error SECURE_NPCTIMEOUT_INTERVAL should be at least 1 (1s)
#endif
#if NPC_SECURE_TIMEOUT_INPUT < 0
	#error NPC_SECURE_TIMEOUT_INPUT cannot be lower than 0
#endif
#if NPC_SECURE_TIMEOUT_MENU < 0
	#error NPC_SECURE_TIMEOUT_MENU cannot be lower than 0
#endif
#if NPC_SECURE_TIMEOUT_NEXT < 0
	#error NPC_SECURE_TIMEOUT_NEXT cannot be lower than 0
#endif

/**
 * Path within the /db folder to (non-)renewal specific db files
 **/
#ifdef RENEWAL
	#define DBPATH "re/"
#else
	#define DBPATH "pre-re/"
#endif

#define DBIMPORT "import"

/**
 * DefType
 **/
#ifdef RENEWAL
	typedef short defType;
	#define DEFTYPE_MIN SHRT_MIN
	#define DEFTYPE_MAX SHRT_MAX
#else
	typedef signed char defType;
	#define DEFTYPE_MIN CHAR_MIN
	#define DEFTYPE_MAX CHAR_MAX
#endif

#ifdef Pandas_Extreme_Computing
	typedef int32 pec_short;
	typedef uint32 pec_ushort;
	typedef double pec_float;

	typedef int32 pec_defType;
	#define PEC_DEFTYPE_MIN INT_MIN
	#define PEC_DEFTYPE_MAX INT_MAX

	#define PEC_SHRT_MIN INT_MIN
	#define PEC_SHRT_MAX INT_MAX

	// 此处不能用 UINT_MAX 会导致 cap_value 判断出现溢出
	#define PEC_USHRT_MAX INT_MAX

	// 最大负重的上限其实可以到 0x7FFFFFFF (INT_MAX)
	// 但客户端的面板显示时, 整个负重信息最大只能显示 13 个字符 (包括斜杠)
	// 也就是说极限是: 999999/999999 因此这里限制为体验最佳数值 999999
	// 这里多加个 1 凑个整, 设置为 100万
	#define PEC_MAX_WEIGHT 1000000
#else
	typedef short pec_short;
	typedef unsigned short pec_ushort;
	typedef float pec_float;

	typedef defType pec_defType;
	#define PEC_DEFTYPE_MIN DEFTYPE_MIN
	#define PEC_DEFTYPE_MAX DEFTYPE_MAX

	#define PEC_SHRT_MIN SHRT_MIN
	#define PEC_SHRT_MAX SHRT_MAX

	#define PEC_USHRT_MAX USHRT_MAX
#endif // Pandas_Extreme_Computing

/**
 * EXP definition type
 */
typedef uint64 t_exp;

/// Max Base and Job EXP for players
#if PACKETVER >= 20170830
	const t_exp MAX_EXP = INT64_MAX;
#else
	const t_exp MAX_EXP = INT32_MAX;
#endif

/// Max EXP for guilds
const t_exp MAX_GUILD_EXP = INT32_MAX;
/// Max Base EXP for player on Max Base Level
const t_exp MAX_LEVEL_BASE_EXP = 99999999;
/// Max Job EXP for player on Max Job Level
const t_exp MAX_LEVEL_JOB_EXP = 999999999;

/* pointer size fix which fixes several gcc warnings */
#ifdef __64BIT__
	#define __64BPRTSIZE(y) (intptr)y
#else
	#define __64BPRTSIZE(y) y
#endif

/* ATCMD_FUNC(mobinfo) HIT and FLEE calculations */
#ifdef RENEWAL
	#define MOB_FLEE(mob) ( mob->lv + mob->status.agi + 100 )
	#define MOB_HIT(mob)  ( mob->lv + mob->status.dex + 175 )
#else
	#define MOB_FLEE(mob) ( mob->lv + mob->status.agi )
	#define MOB_HIT(mob)  ( mob->lv + mob->status.dex )
#endif

/* Renewal's dmg level modifier, used as a macro for a easy way to turn off. */
#ifdef RENEWAL_LVDMG
	#define RE_LVL_DMOD(val) \
		if( status_get_lv(src) > 99 && val > 0 ) \
			skillratio = skillratio * status_get_lv(src) / val;
	#define RE_LVL_MDMOD(val) \
		if( status_get_lv(src) > 99 && val > 0) \
			md.damage = md.damage * status_get_lv(src) / val;
	/* ranger traps special */
	#define RE_LVL_TMDMOD() \
		if( status_get_lv(src) > 99 ) \
			md.damage = md.damage * 150 / 100 + md.damage * status_get_lv(src) / 100;
#else
	#define RE_LVL_DMOD(val)
	#define RE_LVL_MDMOD(val)
	#define RE_LVL_TMDMOD()
#endif

// Renewal variable cast time reduction
#ifdef RENEWAL_CAST
	#define VARCAST_REDUCTION(val){ \
		if( (varcast_r += val) != 0 && varcast_r >= 0 ) \
			time = time * (1 - (float)min(val, 100) / 100); \
	}
#endif

/**
 * Default coordinate for new char
 * That map should be loaded by a mapserv
 **/
#ifdef RENEWAL
    #define MAP_DEFAULT_NAME "iz_int"
    #define MAP_DEFAULT_X 18
    #define MAP_DEFAULT_Y 26
#else
    #define MAP_DEFAULT_NAME "new_1-1"
    #define MAP_DEFAULT_X 53
    #define MAP_DEFAULT_Y 111
#endif

/**
 * End of File
 **/
#endif /* CONFIG_CONST_H */

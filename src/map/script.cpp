// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

//#define DEBUG_DISP
//#define DEBUG_DISASM
//#define DEBUG_RUN
//#define DEBUG_HASH
//#define DEBUG_DUMP_STACK

#include "script.hpp"

#include <atomic>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h> // atoi, strtol, strtoll, exit

#ifdef Pandas_ScriptEngine_Express
#include <cctype>	// toupper, tolower
#include <algorithm>	// transform
#endif // Pandas_ScriptEngine_Express

#ifdef Pandas_ScriptCommand_Preg_Search
#include <boost/regex.hpp>
#endif // Pandas_ScriptCommand_Preg_Search

#ifdef PCRE_SUPPORT
#include "../../3rdparty/pcre/include/pcre.h" // preg_match
#endif

#include "../common/cbasetypes.hpp"
#include "../common/ers.hpp"  // ers_destroy
#include "../common/malloc.hpp"
#include "../common/md5calc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/socket.hpp"
#include "../common/strlib.hpp"
#include "../common/timer.hpp"
#include "../common/utilities.hpp"
#include "../common/utils.hpp"

#include "achievement.hpp"
#include "atcommand.hpp"
#include "battle.hpp"
#include "battleground.hpp"
#include "channel.hpp"
#include "chat.hpp"
#include "chrif.hpp"
#include "clan.hpp"
#include "clif.hpp"
#include "date.hpp" // date type enum, date_get()
#include "elemental.hpp"
#include "guild.hpp"
#include "homunculus.hpp"
#include "instance.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "mail.hpp"
#include "map.hpp"
#include "mapreg.hpp"
#include "mercenary.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "pet.hpp"
#include "quest.hpp"
#include "storage.hpp"
#include "asyncquery.hpp"

#ifdef Pandas_Aura_Mechanism
#include "aura.hpp"
#endif // Pandas_Aura_Mechanism

#ifdef Pandas_Item_Properties
#include "itemprops.hpp"
#endif // Pandas_Item_Properties

#ifdef Pandas_Database_MobItem_FixedRatio
#include "mobdrop.hpp"
#endif // Pandas_Database_MobItem_FixedRatio

using namespace rathena;

const int64 SCRIPT_INT_MIN = INT64_MIN;
const int64 SCRIPT_INT_MAX = INT64_MAX;

struct eri *array_ers;
DBMap *st_db;
unsigned int active_scripts;
unsigned int next_id;
struct eri *st_ers;
struct eri *stack_ers;
std::atomic<int> script_batch{ 0 }; // For async SQL futures

static bool script_rid2sd_( struct script_state *st, struct map_session_data** sd, const char *func );

/**
 * Get `sd` from a account id in `loc` param instead of attached rid
 * @param st Script
 * @param loc Location to look account id in script parameter
 * @param sd Variable that will be assigned
 * @return True if `sd` is assigned, false otherwise
 **/
static bool script_accid2sd_(struct script_state *st, uint8 loc, struct map_session_data **sd, const char *func) {
	if (script_hasdata(st, loc)) {
		int id_ = script_getnum(st, loc);
		if (!(*sd = map_id2sd(id_))){
			ShowError("%s: Player with account id '%d' is not found.\n", func, id_);
			return false;
		}else{
			return true;
		}
	}
	else
		return script_rid2sd_(st,sd,func);
}

/**
 * Get `sd` from a char id in `loc` param instead of attached rid
 * @param st Script
 * @param loc Location to look char id in script parameter
 * @param sd Variable that will be assigned
 * @return True if `sd` is assigned, false otherwise
 **/
static bool script_charid2sd_(struct script_state *st, uint8 loc, struct map_session_data **sd, const char *func) {
	if (script_hasdata(st, loc)) {
		int id_ = script_getnum(st, loc);
		if (!(*sd = map_charid2sd(id_))){
			ShowError("%s: Player with char id '%d' is not found.\n", func, id_);
			return false;
		}else{
			return true;
		}
	}
	else
		return script_rid2sd_(st,sd,func);
}

/**
 * Get `sd` from a nick in `loc` param instead of attached rid
 * @param st Script
 * @param loc Location to look nick in script parameter
 * @param sd Variable that will be assigned
 * @return True if `sd` is assigned, false otherwise
 **/
static bool script_nick2sd_(struct script_state *st, uint8 loc, struct map_session_data **sd, const char *func) {
	if (script_hasdata(st, loc)) {
		const char *name_ = script_getstr(st, loc);
		if (!(*sd = map_nick2sd(name_,false))){
			ShowError("%s: Player with nick '%s' is not found.\n", func, name_);
			return false;
		}else{
			return true;
		}
	}
	else
		return script_rid2sd_(st,sd,func);
}

/**
 * Get `sd` from a mapid in `loc` param instead of attached rid
 * @param st Script
 * @param loc Location to look mapid in script parameter
 * @param sd Variable that will be assigned
 * @return True if `sd` is assigned, false otherwise
 **/
static bool script_mapid2sd_(struct script_state *st, uint8 loc, struct map_session_data **sd, const char *func) {
	if (script_hasdata(st, loc)) {
		int id_ = script_getnum(st, loc);
		if (!(*sd = map_id2sd(id_))){
			ShowError("%s: Player with map id '%d' is not found.\n", func, id_);
			return false;
		}else{
			return true;
		}
	}
	else
		return script_rid2sd_(st,sd,func);
}

/**
 * Get `bl` from an ID in `loc` param, if ID is 0 will return the `bl` of the script's activator.
 * @param st Script
 * @param loc Location to look for ID in script parameter
 * @param bl Variable that will be assigned
 * @return True if `bl` is assigned, false otherwise
 **/
static bool script_rid2bl_(struct script_state *st, uint8 loc, struct block_list **bl, const char *func) {
	int unit_id;

	if ( !script_hasdata(st, loc) || ( unit_id = script_getnum(st, loc) ) == 0)
		unit_id = st->rid;
		
	*bl =  map_id2bl(unit_id);

	if ( *bl )
		return true;
	else {
		ShowError("%s: Unit with ID '%d' is not found.\n", func, unit_id);
		return false;
	}
}

#define script_accid2sd(loc,sd) script_accid2sd_(st,(loc),&(sd),__FUNCTION__)
#define script_charid2sd(loc,sd) script_charid2sd_(st,(loc),&(sd),__FUNCTION__)
#define script_nick2sd(loc,sd) script_nick2sd_(st,(loc),&(sd),__FUNCTION__)
#define script_mapid2sd(loc,sd) script_mapid2sd_(st,(loc),&(sd),__FUNCTION__)
#define script_rid2sd(sd) script_rid2sd_(st,&(sd),__FUNCTION__)
#define script_rid2bl(loc,bl) script_rid2bl_(st,(loc),&(bl),__FUNCTION__)

/// temporary buffer for passing around compiled bytecode
/// @see add_scriptb, set_label, parse_script
static unsigned char* script_buf = NULL;
static int script_pos = 0, script_size = 0;

static inline int GETVALUE(const unsigned char* buf, int i)
{
	return (int)MakeDWord(MakeWord(buf[i], buf[i+1]), MakeWord(buf[i+2], 0));
}
static inline void SETVALUE(unsigned char* buf, int i, int n)
{
	buf[i]   = GetByte(n, 0);
	buf[i+1] = GetByte(n, 1);
	buf[i+2] = GetByte(n, 2);
}

// String buffer structures.
// str_data stores string information
static struct str_data_struct {
	enum c_op type;
	int str;
	int backpatch;
	int label;
	int (*func)(struct script_state *st);
	int64 val;
	int next;
	const char *name;
	bool deprecated;
} *str_data = nullptr;
static int str_data_size = 0; // size of the data
static int str_num = LABEL_START; // next id to be assigned

// str_buf holds the strings themselves
static char *str_buf;
static int str_size = 0; // size of the buffer
static int str_pos = 0; // next position to be assigned

#ifdef Pandas_ScriptEngine_AddStr_Realloc_Memory
// str_data 每次被重新分配时, 增加多少个字节的容量
#define STR_DATA_INCREASING_SIZE 128 * 4

// str_buf  每次被重新分配时, 增加多少个字节的容量
#define STR_BUF_INCREASING_SIZE 256 * 32
#endif // Pandas_ScriptEngine_AddStr_Realloc_Memory

// Using a prime number for SCRIPT_HASH_SIZE should give better distributions
#define SCRIPT_HASH_SIZE 1021
int str_hash[SCRIPT_HASH_SIZE];
// Specifies which string hashing method to use
//#define SCRIPT_HASH_DJB2
//#define SCRIPT_HASH_SDBM
#define SCRIPT_HASH_ELF

static DBMap* scriptlabel_db = NULL; // const char* label_name -> int script_pos
static DBMap* userfunc_db = NULL; // const char* func_name -> struct script_code*
static int parse_options = 0;
DBMap* script_get_label_db(void) { return scriptlabel_db; }
DBMap* script_get_userfunc_db(void) { return userfunc_db; }

// important buildin function references for usage in scripts
static int buildin_set_ref = 0;
static int buildin_callsub_ref = 0;
static int buildin_callfunc_ref = 0;
static int buildin_getelementofarray_ref = 0;

// Caches compiled autoscript item code.
// Note: This is not cleared when reloading itemdb.
static DBMap* autobonus_db = NULL; // char* script -> char* bytecode

struct Script_Config script_config = {
	1, // warn_func_mismatch_argtypes
	1, 65535, 2048, //warn_func_mismatch_paramnum/check_cmdcount/check_gotocount
	0, INT_MAX, // input_min_value/input_max_value
	// NOTE: None of these event labels should be longer than <EVENT_NAME_LENGTH> characters
	// PC related
	"OnPCDieEvent", //die_event_name
	"OnPCKillEvent", //kill_pc_event_name
	"OnNPCKillEvent", //kill_mob_event_name
	"OnPCLoginEvent", //login_event_name
	"OnPCLogoutEvent", //logout_event_name
	"OnPCLoadMapEvent", //loadmap_event_name
	"OnPCBaseLvUpEvent", //baselvup_event_name
	"OnPCJobLvUpEvent", //joblvup_event_name

	/************************************************************************/
	/* Filter 类型的过滤事件，这些事件可以被 processhalt 中断                    */
	/************************************************************************/

#ifdef Pandas_NpcFilter_IDENTIFY
	"OnPCIdentifyFilter",	// NPCF_IDENTIFY		// identify_filter_name	// 当玩家在装备鉴定列表中选择好装备, 并点击“确定”按钮时触发过滤器
#endif // Pandas_NpcFilter_IDENTIFY

#ifdef Pandas_NpcFilter_ENTERCHAT
	"OnPCInChatroomFilter",	// NPCF_ENTERCHAT		// enterchat_filter_name	// 当玩家进入 NPC 开启的聊天室时触发过滤器
#endif // Pandas_NpcFilter_ENTERCHAT

#ifdef Pandas_NpcFilter_INSERT_CARD
	"OnPCInsertCardFilter",	// NPCF_INSERT_CARD		// insert_card_filter_name	// 当玩家准备插入卡片时触发过滤器
#endif // Pandas_NpcFilter_INSERT_CARD

#ifdef Pandas_NpcFilter_USE_ITEM
	"OnPCUseItemFilter",	// NPCF_USE_ITEM		// use_item_filter_name	// 当玩家准备使用非装备类道具时触发过滤器
#endif // Pandas_NpcFilter_USE_ITEM

#ifdef Pandas_NpcFilter_USE_SKILL
	"OnPCUseSkillFilter",	// NPCF_USE_SKILL		// use_skill_filter_name	// 当玩家准备使用技能时触发过滤器
#endif // Pandas_NpcFilter_USE_SKILL

#ifdef Pandas_NpcFilter_ROULETTE_OPEN
	"OnPCOpenRouletteFilter",	// NPCF_ROULETTE_OPEN		// roulette_open_filter_name	// 当玩家准备打开乐透大转盘的时候触发过滤器
#endif // Pandas_NpcFilter_ROULETTE_OPEN

#ifdef Pandas_NpcFilter_VIEW_EQUIP
	"OnPCViewEquipFilter",	// NPCF_VIEW_EQUIP		// view_equip_filter_name	// 当玩家准备查看某个角色的装备时触发过滤器
#endif // Pandas_NpcFilter_VIEW_EQUIP

#ifdef Pandas_NpcFilter_EQUIP
	"OnPCEquipFilter",	// NPCF_EQUIP		// equip_filter_name	// 当玩家准备穿戴装备时触发过滤器
#endif // Pandas_NpcFilter_EQUIP

#ifdef Pandas_NpcFilter_UNEQUIP
	"OnPCUnequipFilter",	// NPCF_UNEQUIP		// unequip_filter_name	// 当玩家准备脱下装备时触发过滤器
#endif // Pandas_NpcFilter_UNEQUIP

#ifdef Pandas_NpcFilter_CHANGETITLE
	"OnPCChangeTitleFilter",	// NPCF_CHANGETITLE		// changetitle_filter_name	// 当玩家试图变更称号时将触发过滤器
#endif // Pandas_NpcFilter_CHANGETITLE

#ifdef Pandas_NpcFilter_SC_START
	"OnPCBuffStartFilter",	// NPCF_SC_START		// sc_start_filter_name	// 当玩家准备获得一个状态(Buff)时触发过滤器
#endif // Pandas_NpcFilter_SC_START

#ifdef Pandas_NpcFilter_USE_REVIVE_TOKEN
	"OnPCUseReviveTokenFilter",	// NPCF_USE_REVIVE_TOKEN		// use_revive_token_filter_name	// 当玩家使用菜单中的原地复活之证时触发过滤器
#endif // Pandas_NpcFilter_USE_REVIVE_TOKEN

#ifdef Pandas_NpcFilter_ONECLICK_IDENTIFY
	"OnPCUseOCIdentifyFilter",	// NPCF_ONECLICK_IDENTIFY		// oneclick_identify_filter_name	// 当玩家使用一键鉴定道具时触发过滤器
#endif // Pandas_NpcFilter_ONECLICK_IDENTIFY

#ifdef Pandas_NpcFilter_GUILDCREATE
	"OnPCGuildCreateFilter",	// NPCF_GUILDCREATE		// guildcreate_filter_name	// 当玩家准备创建公会时触发过滤器
#endif // Pandas_NpcFilter_GUILDCREATE

#ifdef Pandas_NpcFilter_GUILDJOIN
	"OnPCGuildJoinFilter",	// NPCF_GUILDJOIN		// guildjoin_filter_name	// 当玩家即将加入公会时触发过滤器
#endif // Pandas_NpcFilter_GUILDJOIN

#ifdef Pandas_NpcFilter_GUILDLEAVE
	"OnPCGuildLeaveFilter",	// NPCF_GUILDLEAVE		// guildleave_filter_name	// 当玩家准备离开公会时触发过滤器
#endif // Pandas_NpcFilter_GUILDLEAVE

#ifdef Pandas_NpcFilter_PARTYCREATE
	"OnPCPartyCreateFilter",	// NPCF_PARTYCREATE		// partycreate_filter_name	// 当玩家准备创建队伍时触发过滤器
#endif // Pandas_NpcFilter_PARTYCREATE

#ifdef Pandas_NpcFilter_PARTYJOIN
	"OnPCPartyJoinFilter",	// NPCF_PARTYJOIN		// partyjoin_filter_name	// 当玩家即将加入队伍时触发过滤器
#endif // Pandas_NpcFilter_PARTYJOIN

#ifdef Pandas_NpcFilter_PARTYLEAVE
	"OnPCPartyLeaveFilter",	// NPCF_PARTYLEAVE		// partyleave_filter_name	// 当玩家准备离开队伍时触发过滤器
#endif // Pandas_NpcFilter_PARTYLEAVE

#ifdef Pandas_NpcFilter_DROPITEM
	"OnPCDropItemFilter",	// NPCF_DROPITEM		// dropitem_filter_name	// 当玩家准备丢弃或掉落道具时触发过滤器
#endif // Pandas_NpcFilter_DROPITEM

#ifdef Pandas_NpcFilter_CLICKTOMB
	"OnPCClickTombFilter",	// NPCF_CLICKTOMB		// clicktomb_filter_name	// 当玩家点击魔物墓碑时触发过滤器
#endif // Pandas_NpcFilter_CLICKTOMB

#ifdef Pandas_NpcFilter_STORAGE_ADD
	"OnPCStorageAddFilter",	// NPCF_STORAGE_ADD		// storage_add_filter_name	// 当玩家准备将道具存入仓库时触发过滤器
#endif // Pandas_NpcFilter_STORAGE_ADD

#ifdef Pandas_NpcFilter_STORAGE_DEL
	"OnPCStorageDelFilter",	// NPCF_STORAGE_DEL		// storage_del_filter_name	// 当玩家准备将道具取出仓库时触发过滤器
#endif // Pandas_NpcFilter_STORAGE_DEL
	// PYHELP - NPCEVENT - INSERT POINT - <Section 5>

	/************************************************************************/
	/* Event  类型的标准事件，这些事件不能被 processhalt 打断                    */
	/************************************************************************/

#ifdef Pandas_NpcEvent_KILLMVP
	"OnPCKillMvpEvent",	// NPCE_KILLMVP		// killmvp_event_name	// 当玩家杀死 MVP 魔物后触发事件
#endif // Pandas_NpcEvent_KILLMVP

#ifdef Pandas_NpcEvent_IDENTIFY
	"OnPCIdentifyEvent",	// NPCE_IDENTIFY		// identify_event_name	// 当玩家成功鉴定了装备时触发事件
#endif // Pandas_NpcEvent_IDENTIFY

#ifdef Pandas_NpcEvent_INSERT_CARD
	"OnPCInsertCardEvent",	// NPCE_INSERT_CARD		// insert_card_event_name	// 当玩家成功插入卡片后触发事件
#endif // Pandas_NpcEvent_INSERT_CARD

#ifdef Pandas_NpcEvent_USE_ITEM
	"OnPCUseItemEvent",	// NPCE_USE_ITEM		// use_item_event_name	// 当玩家成功使用非装备类道具后触发事件
#endif // Pandas_NpcEvent_USE_ITEM

#ifdef Pandas_NpcEvent_USE_SKILL
	"OnPCUseSkillEvent",	// NPCE_USE_SKILL		// use_skill_event_name	// 当玩家成功使用技能后触发事件
#endif // Pandas_NpcEvent_USE_SKILL

#ifdef Pandas_NpcEvent_EQUIP
	"OnPCEquipEvent",	// NPCE_EQUIP		// equip_event_name	// 当玩家成功穿戴一件装备时触发事件
#endif // Pandas_NpcEvent_EQUIP

#ifdef Pandas_NpcEvent_UNEQUIP
	"OnPCUnequipEvent",	// NPCE_UNEQUIP		// unequip_event_name	// 当玩家成功脱下一件装备时触发事件
#endif // Pandas_NpcEvent_UNEQUIP
	// PYHELP - NPCEVENT - INSERT POINT - <Section 11>

	/************************************************************************/
	/* Express 类型的快速事件，这些事件将会被立刻执行, 不进事件队列                */
	/************************************************************************/

#ifdef Pandas_NpcExpress_STATCALC
	"OnPCStatCalcEvent",	// NPCE_STATCALC		// statcalc_express_name	// 当角色能力被重新计算时触发事件
#endif // Pandas_NpcExpress_STATCALC

#ifdef Pandas_NpcExpress_SC_END
	"OnPCBuffEndExpress",	// NPCX_SC_END		// sc_end_express_name	// 当玩家成功解除一个状态(Buff)后触发实时事件
#endif // Pandas_NpcExpress_SC_END

#ifdef Pandas_NpcExpress_SC_START
	"OnPCBuffStartExpress",	// NPCX_SC_START		// sc_start_express_name	// 当玩家成功获得一个状态(Buff)后触发实时事件
#endif // Pandas_NpcExpress_SC_START

#ifdef Pandas_NpcExpress_ENTERMAP
	"OnPCEnterMapExpress",	// NPCX_ENTERMAP		// entermap_express_name	// 当玩家进入或者改变地图时触发实时事件
#endif // Pandas_NpcExpress_ENTERMAP

#ifdef Pandas_NpcExpress_PROGRESSABORT
	"OnPCProgressAbortExpress",	// NPCX_PROGRESSABORT		// progressabort_express_name	// 当 progressbar 进度条被打断时触发实时事件
#endif // Pandas_NpcExpress_PROGRESSABORT

#ifdef Pandas_NpcExpress_UNIT_KILL
	"OnUnitKillExpress",	// NPCX_UNIT_KILL		// unit_kill_express_name	// 当某个单位被击杀时触发实时事件
#endif // Pandas_NpcExpress_UNIT_KILL

#ifdef Pandas_NpcExpress_MOBDROPITEM
	"OnMobDropItemExpress",	// NPCX_MOBDROPITEM		// mobdropitem_express_name	// 当魔物即将掉落道具时触发实时事件
#endif // Pandas_NpcExpress_MOBDROPITEM

#ifdef Pandas_NpcExpress_PCATTACK
	"OnPCAttackExpress",	// NPCX_PCATTACK		// pcattack_express_name	// 当玩家发起攻击并即将进行结算时触发实时事件 [聽風]
#endif // Pandas_NpcExpress_PCATTACK

#ifdef Pandas_NpcExpress_MER_CALL
	"OnPCMerCallExpress",	// NPCX_MER_CALL		// mer_call_express_name	// 当玩家成功召唤出佣兵时触发实时事件
#endif // Pandas_NpcExpress_MER_CALL

#ifdef Pandas_NpcExpress_MER_LEAVE
	"OnPCMerLeaveExpress",	// NPCX_MER_LEAVE		// mer_leave_express_name	// 当佣兵离开玩家时触发实时事件
#endif // Pandas_NpcExpress_MER_LEAVE

#ifdef Pandas_NpcExpress_PC_TALK
		"OnPCTalkExpress",	// NPCX_PC_TALK		// pc_talk_express_name	// 当玩家往聊天框发送信息时触发实时事件 [人鱼姬的思念]
#endif // Pandas_NpcExpress_PC_TALK

#ifdef Pandas_NpcExpress_PCHARMED
	"OnPCHarmedExpress",	// NPCX_PCHARMED		// pcharmed_express_name	// 当玩家受到伤害并即将进行结算时触发实时事件 [人鱼姬的思念]
#endif // Pandas_NpcExpress_PCHARMED
	// PYHELP - NPCEVENT - INSERT POINT - <Section 17>

	// NPC related
	"OnTouch_",	//ontouch_event_name (runs on first visible char to enter area, picks another char if the first char leaves)
	"OnTouch",	//ontouch2_event_name (run whenever a char walks into the OnTouch area)
	"OnTouchNPC", //ontouchnpc_event_name (run whenever a monster walks into the OnTouch area)
	"OnWhisperGlobal",	//onwhisper_event_name (is executed when a player sends a whisper message to the NPC)
	"OnCommand", //oncommand_event_name (is executed by script command cmdothernpc)
	"OnBuyItem", //onbuy_event_name (is executed when items are bought)
	"OnSellItem", //onsell_event_name (is executed when items are sold)
	// Init related
	"OnInit", //init_event_name (is executed on all npcs when all npcs were loaded)
	"OnInterIfInit", //inter_init_event_name (is executed on inter server connection)
	"OnInterIfInitOnce", //inter_init_once_event_name (is only executed on the first inter server connection)
	// Guild related
	"OnGuildBreak", //guild_break_event_name (is executed on all castles of the guild that is broken)
	"OnAgitStart", //agit_start_event_name (is executed when WoE FE is started)
	"OnAgitInit", //agit_init_event_name (is executed after all castle owning guilds have been loaded)
	"OnAgitEnd", //agit_end_event_name (is executed when WoE FE has ended)
	"OnAgitStart2", //agit_start2_event_name (is executed when WoE SE is started)
	"OnAgitInit2", //agit_init2_event_name (is executed after all castle owning guilds have been loaded)
	"OnAgitEnd2", //agit_end2_event_name (is executed when WoE SE has ended)
	"OnAgitStart3", //agit_start3_event_name (is executed when WoE TE is started)
	"OnAgitInit3", //agit_init3_event_name (is executed after all castle owning guilds have been loaded)
	"OnAgitEnd3", //agit_end3_event_name (is executed when WoE TE has ended)
	// Timer related
	"OnTimer", //timer_event_name (is executed by a timer at the specific second)
	"OnTimerQuit", //timer_quit_event_name (is executed when a timer is aborted)
	"OnMinute", //timer_minute_event_name (is executed by a timer at the specific minute)
	"OnHour", //timer_hour_event_name (is executed by a timer at the specific hour)
	"OnClock", //timer_clock_event_name (is executed by a timer at the specific hour and minute)
	"OnDay", //timer_day_event_name (is executed by a timer at the specific month and day)
	"OnSun", //timer_sunday_event_name (is executed by a timer on sunday at the specific hour and minute)
	"OnMon", //timer_monday_event_name (is executed by a timer on monday at the specific hour and minute)
	"OnTue", //timer_tuesday_event_name (is executed by a timer on tuesday at the specific hour and minute)
	"OnWed", //timer_wednesday_event_name (is executed by a timer on wednesday at the specific hour and minute)
	"OnThu", //timer_thursday_event_name (is executed by a timer on thursday at the specific hour and minute)
	"OnFri", //timer_friday_event_name (is executed by a timer on friday at the specific hour and minute)
	"OnSat", //timer_saturday_event_name (is executed by a timer on saturday at the specific hour and minute)
	// Instance related
	"OnInstanceInit", //instance_init_event_name (is executed right after instance creation)
	"OnInstanceDestroy", //instance_destroy_event_name (is executed right before instance destruction)
};

static jmp_buf     error_jump;
static char*       error_msg;
static const char* error_pos;
static int         error_report; // if the error should produce output
// Used by disp_warning_message
static const char* parser_current_src;
static const char* parser_current_file;
static int         parser_current_line;

// for advanced scripting support ( nested if, switch, while, for, do-while, function, etc )
// [Eoe / jA 1080, 1081, 1094, 1164]
enum curly_type {
	TYPE_NULL = 0,
	TYPE_IF,
	TYPE_SWITCH,
	TYPE_WHILE,
	TYPE_FOR,
	TYPE_DO,
	TYPE_USERFUNC,
	TYPE_ARGLIST // function argument list
};

enum e_arglist
{
	ARGLIST_UNDEFINED = 0,
	ARGLIST_NO_PAREN  = 1,
	ARGLIST_PAREN     = 2,
};

static struct {
	struct {
		enum curly_type type;
		int index;
		int count;
		int flag;
		struct linkdb_node *case_label;
	} curly[256];		// Information right parenthesis
	int curly_count;	// The number of right brackets
	int index;			// Number of the syntax used in the script
} syntax;

const char* parse_curly_close(const char* p);
const char* parse_syntax_close(const char* p);
const char* parse_syntax_close_sub(const char* p,int* flag);
const char* parse_syntax(const char* p);
static int parse_syntax_for_flag = 0;

extern short current_equip_item_index; //for New CARDS Scripts. It contains Inventory Index of the EQUIP_SCRIPT caller item. [Lupus]
extern unsigned int current_equip_combo_pos;

int potion_flag=0; //For use on Alchemist improved potions/Potion Pitcher. [Skotlex]
int potion_hp=0, potion_per_hp=0, potion_sp=0, potion_per_sp=0;
int potion_target = 0;
unsigned int *generic_ui_array = NULL;
unsigned int generic_ui_array_size = 0;


c_op get_com(unsigned char *script,int *pos);
int64 get_num(unsigned char *script,int *pos);

typedef struct script_function {
	int (*func)(struct script_state *st);
	const char *name;
	const char *arg;
	const char *deprecated;
} script_function;

extern script_function buildin_func[];

static struct linkdb_node *sleep_db; // int oid -> struct script_state *

/*==========================================
 * (Only those needed) local declaration prototype
 *------------------------------------------*/
const char* parse_subexpr(const char* p,int limit);
int run_func(struct script_state *st);
int script_instancegetid(struct script_state *st, e_instance_mode mode = IM_NONE);

const char* script_op2name(int op)
{
#define RETURN_OP_NAME(type) case type: return #type
	switch( op )
	{
	RETURN_OP_NAME(C_NOP);
	RETURN_OP_NAME(C_POS);
	RETURN_OP_NAME(C_INT);
	RETURN_OP_NAME(C_PARAM);
	RETURN_OP_NAME(C_FUNC);
	RETURN_OP_NAME(C_STR);
	RETURN_OP_NAME(C_CONSTSTR);
	RETURN_OP_NAME(C_ARG);
	RETURN_OP_NAME(C_NAME);
	RETURN_OP_NAME(C_EOL);
	RETURN_OP_NAME(C_RETINFO);
	RETURN_OP_NAME(C_USERFUNC);
	RETURN_OP_NAME(C_USERFUNC_POS);

	RETURN_OP_NAME(C_REF);

	// operators
	RETURN_OP_NAME(C_OP3);
	RETURN_OP_NAME(C_LOR);
	RETURN_OP_NAME(C_LAND);
	RETURN_OP_NAME(C_LE);
	RETURN_OP_NAME(C_LT);
	RETURN_OP_NAME(C_GE);
	RETURN_OP_NAME(C_GT);
	RETURN_OP_NAME(C_EQ);
	RETURN_OP_NAME(C_NE);
	RETURN_OP_NAME(C_XOR);
	RETURN_OP_NAME(C_OR);
	RETURN_OP_NAME(C_AND);
	RETURN_OP_NAME(C_ADD);
	RETURN_OP_NAME(C_SUB);
	RETURN_OP_NAME(C_MUL);
	RETURN_OP_NAME(C_DIV);
	RETURN_OP_NAME(C_MOD);
	RETURN_OP_NAME(C_NEG);
	RETURN_OP_NAME(C_LNOT);
	RETURN_OP_NAME(C_NOT);
	RETURN_OP_NAME(C_R_SHIFT);
	RETURN_OP_NAME(C_L_SHIFT);
	RETURN_OP_NAME(C_ADD_POST);
	RETURN_OP_NAME(C_SUB_POST);
	RETURN_OP_NAME(C_ADD_PRE);
	RETURN_OP_NAME(C_SUB_PRE);

	default:
		ShowDebug("script_op2name: unexpected op=%d\n", op);
		return "???";
	}
#undef RETURN_OP_NAME
}

#ifdef DEBUG_DUMP_STACK
static void script_dump_stack(struct script_state* st)
{
	int i;
	ShowMessage("\tstart = %d\n", st->start);
	ShowMessage("\tend   = %d\n", st->end);
	ShowMessage("\tdefsp = %d\n", st->stack->defsp);
	ShowMessage("\tsp    = %d\n", st->stack->sp);
	for( i = 0; i < st->stack->sp; ++i )
	{
		struct script_data* data = &st->stack->stack_data[i];
		ShowMessage("\t[%d] %s", i, script_op2name(data->type));
		switch( data->type )
		{
		case C_INT:
		case C_POS:
			ShowMessage(" %d\n", data->u.num);
			break;

		case C_STR:
		case C_CONSTSTR:
			ShowMessage(" \"%s\"\n", data->u.str);
			break;

		case C_NAME:
			ShowMessage(" \"%s\" (id=%d ref=%p subtype=%s)\n", reference_getname(data), data->u.num, data->ref, script_op2name(str_data[data->u.num].type));
			break;

		case C_RETINFO:
			{
				struct script_retinfo* ri = data->u.ri;
				ShowMessage(" %p {scope.vars=%p, scope.arrays=%p, script=%p, pos=%d, nargs=%d, defsp=%d}\n", ri, ri->scope.vars, ri->scope.arrays, ri->script, ri->pos, ri->nargs, ri->defsp);
			}
			break;
		default:
			ShowMessage("\n");
			break;
		}
	}
}
#endif

/// Reports on the console the src of a script error.
static void script_reportsrc(struct script_state *st)
{
	if( st->oid == 0 )
		return; //Can't report source.

	struct block_list* bl = map_id2bl(st->oid);

	if (!bl)
		return;

	switch( bl->type ) {
		case BL_NPC: {
			struct npc_data* nd = (struct npc_data*)bl;

			if( bl->m >= 0 )
				ShowDebug("Source (NPC): %s at %s (%d,%d)\n", nd->name, map_mapid2mapname(bl->m), bl->x, bl->y);
			else
				ShowDebug("Source (NPC): %s (invisible/not on a map)\n", nd->name);
			ShowDebug( "Source (NPC): %s is located in: %s\n", nd->name, nd->path );
			} break;
		default:
			if( bl->m >= 0 )
				ShowDebug("Source (Non-NPC type %d): name %s at %s (%d,%d)\n", bl->type, status_get_name(bl), map_mapid2mapname(bl->m), bl->x, bl->y);
			else
				ShowDebug("Source (Non-NPC type %d): name %s (invisible/not on a map)\n", bl->type, status_get_name(bl));
			break;
	}
}

/// Reports on the console information about the script data.
static void script_reportdata(struct script_data* data)
{
	if( data == NULL )
		return;
	switch( data->type ) {
		case C_NOP:// no value
			ShowDebug("Data: nothing (nil)\n");
			break;
		case C_INT:// number
			ShowDebug("Data: number value=%" PRId64 "\n", data->u.num);
			break;
		case C_STR:
		case C_CONSTSTR:// string
			if( data->u.str ) {
				ShowDebug("Data: string value=\"%s\"\n", data->u.str);
			} else {
				ShowDebug("Data: string value=NULL\n");
			}
			break;
		case C_NAME:// reference
			if( reference_tovariable(data) ) {// variable
				const char* name = reference_getname(data);
				ShowDebug("Data: variable name='%s' index=%d\n", name, reference_getindex(data));
			} else if( reference_toconstant(data) ) {// constant
				ShowDebug("Data: constant name='%s' value=%" PRId64 "\n", reference_getname(data), reference_getconstant(data));
			} else if( reference_toparam(data) ) {// param
				ShowDebug("Data: param name='%s' type=%" PRId64 "\n", reference_getname(data), reference_getparamtype(data));
			} else {// ???
				ShowDebug("Data: reference name='%s' type=%s\n", reference_getname(data), script_op2name(data->type));
				ShowDebug("Please report this!!! - script->str_data.type=%s\n", script_op2name(str_data[reference_getid(data)].type));
			}
			break;
		case C_POS:// label
			ShowDebug("Data: label pos=%" PRId64 "\n", data->u.num);
			break;
		default:
			ShowDebug("Data: %s\n", script_op2name(data->type));
			break;
	}
}


/// Reports on the console information about the current built-in function.
static void script_reportfunc(struct script_state* st)
{
	int params, id;
	struct script_data* data;

	if( !script_hasdata(st,0) )
	{// no stack
		return;
	}

	data = script_getdata(st,0);

	if( !data_isreference(data) || str_data[reference_getid(data)].type != C_FUNC )
	{// script currently not executing a built-in function or corrupt stack
		return;
	}

	id     = reference_getid(data);
	params = script_lastdata(st)-1;

	if( params > 0 )
	{
		int i;
		ShowDebug("Function: %s (%d parameter%s):\n", get_str(id), params, ( params == 1 ) ? "" : "s");

		for( i = 2; i <= script_lastdata(st); i++ )
		{
			script_reportdata(script_getdata(st,i));
		}
	}
	else
	{
		ShowDebug("Function: %s (no parameters)\n", get_str(id));
	}
}
/*==========================================
 * Output error message
 *------------------------------------------*/
static void disp_error_message2(const char *mes,const char *pos,int report)
{
	error_msg = aStrdup(mes);
	error_pos = pos;
	error_report = report;
	longjmp( error_jump, 1 );
}
#define disp_error_message(mes,pos) disp_error_message2(mes,pos,1)

static void disp_warning_message(const char *mes, const char *pos) {
	script_warning(parser_current_src,parser_current_file,parser_current_line,mes,pos);
}

/// Checks event parameter validity
static void check_event(struct script_state *st, const char *evt)
{
	if( evt && evt[0] && !stristr(evt, "::On") )
	{
		ShowWarning("NPC event parameter deprecated! Please use 'NPCNAME::OnEVENT' instead of '%s'.\n", evt);
		script_reportsrc(st);
	}
}

/*==========================================
 * Hashes the input string
 *------------------------------------------*/
static unsigned int calc_hash(const char* p)
{
	unsigned int h;

#if defined(SCRIPT_HASH_DJB2)
	h = 5381;
	while( *p ) // hash*33 + c
		h = ( h << 5 ) + h + ((unsigned char)TOLOWER(*p++));
#elif defined(SCRIPT_HASH_SDBM)
	h = 0;
	while( *p ) // hash*65599 + c
		h = ( h << 6 ) + ( h << 16 ) - h + ((unsigned char)TOLOWER(*p++));
#elif defined(SCRIPT_HASH_ELF) // UNIX ELF hash
	h = 0;
	while( *p ){
		unsigned int g;
		h = ( h << 4 ) + ((unsigned char)TOLOWER(*p++));
		g = h & 0xF0000000;
		if( g )
		{
			h ^= g >> 24;
			h &= ~g;
		}
	}
#else // athena hash
	h = 0;
	while( *p )
		h = ( h << 1 ) + ( h >> 3 ) + ( h >> 5 ) + ( h >> 8 ) + (unsigned char)TOLOWER(*p++);
#endif

	return h % SCRIPT_HASH_SIZE;
}

bool script_check_RegistryVariableLength(int pType, const char *val, size_t* vlen) 
{
	size_t len = strlen(val);

	if (vlen)
		*vlen = len;
	switch (pType) {
		case 0:
			return (len < 33); // key check
		case 1:
			return (len < 255); // value check
		default:
			return false;
	}
}

/*==========================================
 * str_data manipulation functions
 *------------------------------------------*/

/// Looks up string using the provided id.
const char* get_str(int id)
{
	Assert( id >= LABEL_START && id < str_size );
	return str_buf+str_data[id].str;
}

/// Returns the uid of the string, or -1.
static int search_str(const char* p)
{
	int i;

	for( i = str_hash[calc_hash(p)]; i != 0; i = str_data[i].next )
		if( strcasecmp(get_str(i),p) == 0 )
			return i;

	return -1;
}

/// Stores a copy of the string and returns its id.
/// If an identical string is already present, returns its id instead.
int add_str(const char* p)
{
	int h;
	int len;

	h = calc_hash(p);

	if( str_hash[h] == 0 )
	{// empty bucket, add new node here
		str_hash[h] = str_num;
	}
	else
	{// scan for end of list, or occurence of identical string
		int i;
		for( i = str_hash[h]; ; i = str_data[i].next )
		{
			if( strcasecmp(get_str(i),p) == 0 )
				return i; // string already in list
			if( str_data[i].next == 0 )
				break; // reached the end
		}

		// append node to end of list
		str_data[i].next = str_num;
	}

	// grow list if neccessary
	if( str_num >= str_data_size )
	{
#ifndef Pandas_ScriptEngine_AddStr_Realloc_Memory
		str_data_size += 128;
		RECREATE(str_data,struct str_data_struct,str_data_size);
		memset(str_data + (str_data_size - 128), '\0', 128);
#else
		str_data_size += STR_DATA_INCREASING_SIZE;
		RECREATE(str_data,struct str_data_struct,str_data_size);
		memset(str_data + (str_data_size - STR_DATA_INCREASING_SIZE), '\0', STR_DATA_INCREASING_SIZE);
#endif // Pandas_ScriptEngine_AddStr_Realloc_Memory
	}

	len=(int)strlen(p);

	// grow string buffer if neccessary
	while( str_pos+len+1 >= str_size )
	{
#ifndef Pandas_ScriptEngine_AddStr_Realloc_Memory
		str_size += 256;
		RECREATE(str_buf,char,str_size);
		memset(str_buf + (str_size - 256), '\0', 256);
#else
		str_size += STR_BUF_INCREASING_SIZE;
		RECREATE(str_buf,char,str_size);
		memset(str_buf + (str_size - STR_BUF_INCREASING_SIZE), '\0', STR_BUF_INCREASING_SIZE);
#endif // Pandas_ScriptEngine_AddStr_Realloc_Memory

#ifdef Pandas_ScriptEngine_Relocation_Funcname_After_StrBuf_Realloc
		DBIterator* iter = db_iterator(st_db);
		struct script_state* st = nullptr;

		for (st = static_cast<script_state*>(dbi_first(iter)); dbi_exists(iter); st = static_cast<script_state*>(dbi_next(iter))) {
			struct script_data* data = script_getdata(st, 0);
			st->funcname = reference_getname(data);
		}

		dbi_destroy(iter);
#endif // Pandas_ScriptEngine_Relocation_Funcname_After_StrBuf_Realloc
	}

	safestrncpy(str_buf+str_pos, p, len+1);
	str_data[str_num].type = C_NOP;
	str_data[str_num].str = str_pos;
	str_data[str_num].next = 0;
	str_data[str_num].func = NULL;
	str_data[str_num].backpatch = -1;
	str_data[str_num].label = -1;
	str_pos += len+1;

	return str_num++;
}


/// Appends 1 byte to the script buffer.
static void add_scriptb(uint8 a)
{
	if( script_pos+1 >= script_size )
	{
		script_size += SCRIPT_BLOCK_SIZE;
		RECREATE(script_buf,unsigned char,script_size);
	}
	script_buf[script_pos++] = a;
}

/// Appends a c_op value to the script buffer.
/// The value is variable-length encoded into 8-bit blocks.
/// The encoding scheme is ( 01?????? )* 00??????, LSB first.
/// All blocks but the last hold 7 bits of data, topmost bit is always 1 (carries).
static void add_scriptc(int a)
{
	while( a >= 0x40 )
	{
		add_scriptb((a&0x3f)|0x40);
		a = (a - 0x40) >> 6;
	}

	add_scriptb(a);
}

/// Appends an integer value to the script buffer.
/// The value is variable-length encoded into 8-bit blocks.
/// The encoding scheme is ( 11?????? )* 10??????, LSB first.
/// All blocks hold 6 bits of data.
static void add_scripti(int64 a){
	while( a > 0x3f ){
		add_scriptb((a & (int64)0x3f)|(int64)0xc0);
		a >>= 6;
	}

	add_scriptb(((a & (int64)0x3f)|(int64)0x80));
}

/// Appends a str_data object (label/function/variable/integer) to the script buffer.

///
/// @param l The id of the str_data entry
// Maximum up to 16M
static void add_scriptl(int l)
{
	int backpatch = str_data[l].backpatch;

	switch(str_data[l].type){
	case C_POS:
	case C_USERFUNC_POS:
		add_scriptc(C_POS);
		add_scriptb(str_data[l].label);
		add_scriptb(str_data[l].label>>8);
		add_scriptb(str_data[l].label>>16);
		break;
	case C_NOP:
	case C_USERFUNC:
		// Embedded data backpatch there is a possibility of label
		add_scriptc(C_NAME);
		str_data[l].backpatch = script_pos;
		add_scriptb(backpatch);
		add_scriptb(backpatch>>8);
		add_scriptb(backpatch>>16);
		break;
	case C_INT:
		add_scripti(std::abs(str_data[l].val));
		if( str_data[l].val < 0 ) //Notice that this is negative, from jA (Rayce)
			add_scriptc(C_NEG);
		break;
	default: // assume C_NAME
		add_scriptc(C_NAME);
		add_scriptb(l);
		add_scriptb(l>>8);
		add_scriptb(l>>16);
		break;
	}
}

/*==========================================
 * Resolve the label
 *------------------------------------------*/
void set_label(int l,int pos, const char* script_pos_cur)
{
	int i;

	if(str_data[l].type==C_INT || str_data[l].type==C_PARAM || str_data[l].type==C_FUNC)
	{	//Prevent overwriting constants values, parameters and built-in functions [Skotlex]
		disp_error_message("set_label: invalid label name",script_pos_cur);
		return;
	}
	if(str_data[l].label!=-1){
		disp_error_message("set_label: dup label ",script_pos_cur);
		return;
	}
	str_data[l].type=(str_data[l].type == C_USERFUNC ? C_USERFUNC_POS : C_POS);
	str_data[l].label=pos;
	for(i=str_data[l].backpatch;i>=0 && i!=0x00ffffff;){
		int next=GETVALUE(script_buf,i);
		script_buf[i-1]=(str_data[l].type == C_USERFUNC ? C_USERFUNC_POS : C_POS);
		SETVALUE(script_buf,i,pos);
		i=next;
	}
}

/// Skips spaces and/or comments.
const char* skip_space(const char* p)
{
	if( p == NULL )
		return NULL;
	for(;;)
	{
		while( ISSPACE(*p) )
			++p;
		if( *p == '/' && p[1] == '/' )
		{// line comment
			while(*p && *p!='\n')
				++p;
		}
		else if( *p == '/' && p[1] == '*' )
		{// block comment
			p += 2;
			for(;;)
			{
				if( *p == '\0' ) {
					disp_warning_message("script:script->skip_space: end of file while parsing block comment. expected " CL_BOLD "*/" CL_NORM, p);
					return p;
				}
				if( *p == '*' && p[1] == '/' )
				{// end of block comment
					p += 2;
					break;
				}
				++p;
			}
		}
		else
			break;
	}
	return p;
}

/// Skips a word.
/// A word consists of undercores and/or alphanumeric characters,
/// and valid variable prefixes/postfixes.
static const char* skip_word(const char* p)
{
	// prefix
	switch( *p ) {
		case '@':// temporary char variable
			++p; break;
		case '#':// account variable
			p += ( p[1] == '#' ? 2 : 1 ); break;
		case '\'':// instance variable
			++p; break;
		case '.':// npc variable
			p += ( p[1] == '@' ? 2 : 1 ); break;
		case '$':// global variable
			p += ( p[1] == '@' ? 2 : 1 ); break;
	}

	while( ISALNUM(*p) || *p == '_' )
		++p;

	// postfix
	if( *p == '$' )// string
		p++;

	return p;
}

/// Adds a word to str_data.
/// @see skip_word
/// @see add_str
static int add_word(const char* p)
{
	char* word;
	int len;
	int i;

	// Check for a word
	len = skip_word(p) - p;
	if( len == 0 )
		disp_error_message("script:add_word: invalid word. A word consists of undercores and/or alphanumeric characters, and valid variable prefixes/postfixes.", p);

	// Duplicate the word
	word = (char*)aMalloc(len+1);
	memcpy(word, p, len);
	word[len] = 0;

	// add the word
	i = add_str(word);
	aFree(word);
	return i;
}

/// Parses a function call.
/// The argument list can have parenthesis or not.
/// The number of arguments is checked.
static
const char* parse_callfunc(const char* p, int require_paren, int is_custom)
{
	const char* p2;
	const char* arg=NULL;
	int func;

	func = add_word(p);
	if( str_data[func].type == C_FUNC ){
		// buildin function
		add_scriptl(func);
		add_scriptc(C_ARG);
		arg = buildin_func[str_data[func].val].arg;
#if defined(SCRIPT_COMMAND_DEPRECATION)
		if( str_data[func].deprecated ){
			ShowWarning( "Usage of deprecated script function '%s'.\n", get_str(func) );
			ShowWarning( "This function was deprecated on '%s' and could become unavailable anytime soon.\n", buildin_func[str_data[func].val].deprecated );
		}
#endif
	} else if( str_data[func].type == C_USERFUNC || str_data[func].type == C_USERFUNC_POS ){
		// script defined function
		add_scriptl(buildin_callsub_ref);
		add_scriptc(C_ARG);
		add_scriptl(func);
		arg = buildin_func[str_data[buildin_callsub_ref].val].arg;
		if( *arg == 0 )
			disp_error_message("parse_callfunc: callsub has no arguments, please review its definition",p);
		if( *arg != '*' )
			++arg; // count func as argument
	} else {
		const char* name = get_str(func);
		if( !is_custom && strdb_get(userfunc_db, name) == NULL ) {
			disp_error_message("parse_line: expect command, missing function name or calling undeclared function",p);
		} else {;
			add_scriptl(buildin_callfunc_ref);
			add_scriptc(C_ARG);
			add_scriptc(C_STR);
			while( *name ) add_scriptb(*name ++);
			add_scriptb(0);
			arg = buildin_func[str_data[buildin_callfunc_ref].val].arg;
			if( *arg != '*' ) ++ arg;
		}
	}

	p = skip_word(p);
	p = skip_space(p);
	syntax.curly[syntax.curly_count].type = TYPE_ARGLIST;
	syntax.curly[syntax.curly_count].count = 0;
	if( *p == ';' )
	{// <func name> ';'
		syntax.curly[syntax.curly_count].flag = ARGLIST_NO_PAREN;
	} else if( *p == '(' && *(p2=skip_space(p+1)) == ')' )
	{// <func name> '(' ')'
		syntax.curly[syntax.curly_count].flag = ARGLIST_PAREN;
		p = p2;
	/*
	} else if( 0 && require_paren && *p != '(' )
	{// <func name>
		syntax.curly[syntax.curly_count].flag = ARGLIST_NO_PAREN;
	*/
	} else
	{// <func name> <arg list>
		if( require_paren ){
			if( *p != '(' )
				disp_error_message("need '('",p);
			++p; // skip '('
			syntax.curly[syntax.curly_count].flag = ARGLIST_PAREN;
		} else if( *p == '(' ){
			syntax.curly[syntax.curly_count].flag = ARGLIST_UNDEFINED;
		} else {
			syntax.curly[syntax.curly_count].flag = ARGLIST_NO_PAREN;
		}
		++syntax.curly_count;
		while( *arg ) {
			p2=parse_subexpr(p,-1);
			if( p == p2 )
				break; // not an argument
			if( *arg != '*' )
				++arg; // next argument

			p=skip_space(p2);
			if( *arg == 0 || *p != ',' )
				break; // no more arguments
			++p; // skip comma
		}
		--syntax.curly_count;
	}
	if( *arg && *arg != '?' && *arg != '*' )
		disp_error_message2("parse_callfunc: not enough arguments, expected ','", p, script_config.warn_func_mismatch_paramnum);
	if( syntax.curly[syntax.curly_count].type != TYPE_ARGLIST )
		disp_error_message("parse_callfunc: DEBUG last curly is not an argument list",p);
	if( syntax.curly[syntax.curly_count].flag == ARGLIST_PAREN ){
		if( *p != ')' )
			disp_error_message("parse_callfunc: expected ')' to close argument list",p);
		++p;
	}
	add_scriptc(C_FUNC);
	return p;
}

/// Processes end of logical script line.
/// @param first When true, only fix up scheduling data is initialized
/// @param p Script position for error reporting in set_label
static void parse_nextline(bool first, const char* p)
{
	if( !first )
	{
		add_scriptc(C_EOL);  // mark end of line for stack cleanup
		set_label(LABEL_NEXTLINE, script_pos, p);  // fix up '-' labels
	}

	// initialize data for new '-' label fix up scheduling
	str_data[LABEL_NEXTLINE].type      = C_NOP;
	str_data[LABEL_NEXTLINE].backpatch = -1;
	str_data[LABEL_NEXTLINE].label     = -1;
}

/**
 * Pushes a variable into stack, processing its array index if needed.
 * @see parse_variable
 */
void parse_variable_sub_push(int word, const char *p2)
{
	if( p2 ) { // Process the variable index
		const char *p3 = NULL;

		// Push the getelementofarray method into the stack
		add_scriptl(buildin_getelementofarray_ref);
		add_scriptc(C_ARG);
		add_scriptl(word);

		// Process the sub-expression for this assignment
		p3 = parse_subexpr(p2 + 1, 1);
		p3 = skip_space(p3);

		if( *p3 != ']' ) // Closing parenthesis is required for this script
			disp_error_message("Missing closing ']' parenthesis for the variable assignment.", p3);

		// Push the closing function stack operator onto the stack
		add_scriptc(C_FUNC);
		p3++;
	} else // No array index, simply push the variable or value onto the stack
		add_scriptl(word);
}

/// Parse a variable assignment using the direct equals operator
/// @param p script position where the function should run from
/// @return NULL if not a variable assignment, the new position otherwise
const char* parse_variable(const char* p) {
	int word;
	c_op type = C_NOP;
	const char *p2 = NULL;
	const char *var = p;

	if( p[0] == '+' && p[1] == '+' ){
		type = C_ADD_PRE; // pre ++
	}else if( p[0] == '-' && p[1] == '-' ){
		type = C_SUB_PRE; // pre --
	}

	if( type != C_NOP ){
		var = p = skip_space(&p[2]);
	}

	// skip the variable where applicable
	p = skip_word(p);
	p = skip_space(p);

	if( p == NULL ) {// end of the line or invalid buffer
		return NULL;
	}

	if( *p == '[' ) {// array variable so process the array as appropriate
		int i,j;
		for( p2 = p, i = 0, j = 1; p; ++ i ) {
			if( *p ++ == ']' && --(j) == 0 ) break;
			if( *p == '[' ) ++ j;
		}

		if( !(p = skip_space(p)) ) {// end of line or invalid characters remaining
			disp_error_message("Missing right expression or closing bracket for variable.", p);
		}
	}

	if( type == C_NOP ){
		if( p[0] == '=' && p[1] != '=' ){
			type = C_EQ; // =
		}else if( p[0] == '+' && p[1] == '=' ){
			type = C_ADD; // +=
		}else if( p[0] == '-' && p[1] == '=' ){
			type = C_SUB; // -=
		}else if( p[0] == '^' && p[1] == '=' ){
			type = C_XOR; // ^=
		}else if( p[0] == '|' && p[1] == '=' ){
			type = C_OR; // |=
		}else if( p[0] == '&' && p[1] == '=' ){
			type = C_AND; // &=
		}else if( p[0] == '*' && p[1] == '=' ){
			type = C_MUL; // *=
		}else if( p[0] == '/' && p[1] == '=' ){
			type = C_DIV; // /=
		}else if( p[0] == '%' && p[1] == '=' ){
			type = C_MOD; // %=
		}else if( p[0] == '~' && p[1] == '=' ){
			type = C_NOT; // ~=
		}else if( p[0] == '+' && p[1] == '+' ){
			type = C_ADD_POST; // post ++
		}else if( p[0] == '-' && p[1] == '-' ){
			type = C_SUB_POST; // post --
		}else if( p[0] == '<' && p[1] == '<' && p[2] == '=' ){
			type = C_L_SHIFT; // <<=
		}else if( p[0] == '>' && p[1] == '>' && p[2] == '=' ){
			type = C_R_SHIFT; // >>=
		}else{
			// failed to find a matching operator combination so invalid
			return nullptr;
		}
	}

	switch( type ) {
		case C_ADD_PRE: // pre ++
		case C_SUB_PRE: // pre --
			// Nothing more to skip
		break;

		case C_EQ: {// incremental modifier
			p = skip_space( &p[1] );
		}
		break;

		case C_L_SHIFT:
		case C_R_SHIFT: {// left or right shift modifier
			p = skip_space( &p[3] );
		}
		break;

		default: {// normal incremental command
			p = skip_space( &p[2] );
		}
	}

	if( p == NULL ) {// end of line or invalid buffer
		return NULL;
	}

	// push the set function onto the stack
	add_scriptl(buildin_set_ref);
	add_scriptc(C_ARG);

	// always append parenthesis to avoid errors
	syntax.curly[syntax.curly_count].type = TYPE_ARGLIST;
	syntax.curly[syntax.curly_count].count = 0;
	syntax.curly[syntax.curly_count].flag = ARGLIST_PAREN;

	// increment the total curly count for the position in the script
	++ syntax.curly_count;

	// parse the variable currently being modified
	word = add_word(var);

	if( str_data[word].type == C_FUNC || str_data[word].type == C_USERFUNC || str_data[word].type == C_USERFUNC_POS ) // cannot assign a variable which exists as a function or label
		disp_error_message("Cannot modify a variable which has the same name as a function or label.", p);

	parse_variable_sub_push(word, p2); // Push variable onto the stack

	if( type != C_EQ )
		parse_variable_sub_push(word, p2); // Push variable onto the stack once again (first argument of setr)

	if( type == C_ADD_POST || type == C_SUB_POST ) { // post ++ / --
		add_scripti(1);
		add_scriptc(type == C_ADD_POST ? C_ADD : C_SUB);
		parse_variable_sub_push(word, p2); // Push variable onto the stack (third argument of setr)
	} else if( type == C_ADD_PRE || type == C_SUB_PRE ) { // pre ++ / --
		add_scripti(1);
		add_scriptc(type == C_ADD_PRE ? C_ADD : C_SUB);
	} else { // Process the value as an expression
		p = parse_subexpr(p, -1);

		if( type != C_EQ )
		{// push the type of modifier onto the stack
			add_scriptc(type);
		}
	}

	// decrement the curly count for the position within the script
	-- syntax.curly_count;

	// close the script by appending the function operator
	add_scriptc(C_FUNC);

	// push the buffer from the method
	return p;
}

/*
 * Checks whether the gives string is a number literal
 *
 * Mainly necessary to differentiate between number literals and NPC name
 * constants, since several of those start with a digit.
 *
 * All this does is to check if the string begins with an optional + or - sign,
 * followed by a hexadecimal or decimal number literal literal and is NOT
 * followed by a underscore or letter.
 *
 * @author : Hercules.ws
 * @param p Pointer to the string to check
 * @return Whether the string is a number literal
 */
bool is_number(const char *p) {
	const char *np;
	if (!p)
		return false;
	if (*p == '-' || *p == '+')
		p++;
	np = p;
	if (*p == '0' && p[1] == 'x') {
		p+=2;
		np = p;
		// Hexadecimal
		while (ISXDIGIT(*np))
			np++;
	} else {
		// Decimal
		while (ISDIGIT(*np))
			np++;
	}
	if (p != np && *np != '_' && !ISALPHA(*np)) // At least one digit, and next isn't a letter or _
		return true;
	return false;
}

/*==========================================
 * Analysis section
 *------------------------------------------*/
const char* parse_simpleexpr(const char *p)
{
	int64 i;
	p=skip_space(p);

	if(*p==';' || *p==',')
		disp_error_message("parse_simpleexpr: unexpected end of expression",p);
	if(*p=='('){
		if( (i=syntax.curly_count-1) >= 0 && syntax.curly[i].type == TYPE_ARGLIST )
			++syntax.curly[i].count;
		p=parse_subexpr(p+1,-1);
		p=skip_space(p);
		if( (i=syntax.curly_count-1) >= 0 && syntax.curly[i].type == TYPE_ARGLIST &&
				syntax.curly[i].flag == ARGLIST_UNDEFINED && --syntax.curly[i].count == 0
		){
			if( *p == ',' ){
				syntax.curly[i].flag = ARGLIST_PAREN;
				return p;
			} else
				syntax.curly[i].flag = ARGLIST_NO_PAREN;
		}
		if( *p != ')' )
			disp_error_message("parse_simpleexpr: unmatched ')'",p);
		++p;
	} else if(is_number(p)) {
		char *np;
		bool error;

		// Skip leading zeroes
		while(*p == '0' && ISDIGIT(p[1])) p++;

		errno = 0;
		i=strtoll(p,&np,0);
		error = (errno == ERANGE);

		if( i < SCRIPT_INT_MIN || ( error && i == INT64_MIN ) ){
			i = SCRIPT_INT_MIN;
			disp_warning_message( "parse_simpleexpr: underflow detected, capping value to SCRIPT_INT_MIN", p );
		}else if( i > SCRIPT_INT_MAX || ( error && i == INT64_MAX ) ){
			i = SCRIPT_INT_MAX;
			disp_warning_message( "parse_simpleexpr: overflow detected, capping value to SCRIPT_INT_MAX", p );
		}

		add_scripti(i);
		p=np;
	} else if(*p=='"'){
		add_scriptc(C_STR);
		p++;
		while( *p && *p != '"' ){
#ifndef Pandas_ScriptEngine_DoubleByte_UnEscape_Detection
			if( (unsigned char)p[-1] <= 0x7e && *p == '\\' )
#else
			if (isDoubleByteCharacter((unsigned char)p[0], (unsigned char)p[1])) {
				add_scriptb(*p++);
				add_scriptb(*p++);
				continue;
			}
			else if (*p == '\\' && isEscapeSequence(p))
#endif // Pandas_ScriptEngine_DoubleByte_UnEscape_Detection
			{
				char buf[8];
				size_t len = skip_escaped_c(p) - p;
				size_t n = sv_unescape_c(buf, p, len);
				if( n != 1 )
					ShowDebug("parse_simpleexpr: unexpected length %d after unescape (\"%.*s\" -> %.*s)\n", (int)n, (int)len, p, (int)n, buf);
				p += len;
				add_scriptb(*buf);
				continue;
			}
			else if( *p == '\n' )
				disp_error_message("parse_simpleexpr: unexpected newline @ string",p);
			add_scriptb(*p++);
		}
		if(!*p)
			disp_error_message("parse_simpleexpr: unexpected eof @ string",p);
		add_scriptb(0);
		p++;	//'"'
	} else {
		int l;
		const char* pv;

		// label , register , function etc
		if(skip_word(p)==p)
			disp_error_message("parse_simpleexpr: unexpected character",p);

		l=add_word(p);
		if( str_data[l].type == C_FUNC || str_data[l].type == C_USERFUNC || str_data[l].type == C_USERFUNC_POS)
			return parse_callfunc(p,1,0);
		else {
			const char* name = get_str(l);
			if( strdb_get(userfunc_db,name) != NULL ) {
				return parse_callfunc(p,1,1);
			}
		}

		if( (pv = parse_variable(p)) )
		{// successfully processed a variable assignment
			return pv;
		}

#if defined(SCRIPT_CONSTANT_DEPRECATION)
		if( str_data[l].type == C_INT && str_data[l].deprecated ){
			ShowWarning( "Usage of deprecated constant '%s'.\n", get_str(l) );
			ShowWarning( "This constant was deprecated and could become unavailable anytime soon.\n" );
			if (str_data[l].name)
				ShowWarning( "Please use '%s' instead!\n", str_data[l].name );
			disp_warning_message("parse_simpleexpr: deprecated constant", p);
		}
#endif

		p=skip_word(p);
		if( *p == '[' ){
			// array(name[i] => getelementofarray(name,i) )
			add_scriptl(buildin_getelementofarray_ref);
			add_scriptc(C_ARG);
			add_scriptl(l);

			p=parse_subexpr(p+1,-1);
			p=skip_space(p);
			if( *p != ']' )
				disp_error_message("parse_simpleexpr: unmatched ']'",p);
			++p;
			add_scriptc(C_FUNC);
		}else
			add_scriptl(l);

	}

	return p;
}

/*==========================================
 * Analysis of the expression
 *------------------------------------------*/
const char* parse_subexpr(const char* p,int limit)
{
	int op,opl,len;

	p=skip_space(p);

	if( *p == '-' ){
		const char* tmpp = skip_space(p+1);
		if( *tmpp == ';' || *tmpp == ',' ){
			add_scriptl(LABEL_NEXTLINE);
			p++;
			return p;
		}
	}

	if( (op = C_ADD_PRE, p[0] == '+' && p[1] == '+') || (op = C_SUB_PRE, p[0] == '-' && p[1] == '-') ) // Pre ++ -- operators
		p = parse_variable(p);
	else if( (op = C_NEG, *p == '-') || (op = C_LNOT, *p == '!') || (op = C_NOT, *p == '~') ) { // Unary - ! ~ operators
		p = parse_subexpr(p + 1, 11);
		add_scriptc(op);
	} else
		p = parse_simpleexpr(p);
	p = skip_space(p);
	while((
			(op=C_OP3,opl=0,len=1,*p=='?') ||
			((op=C_ADD,opl=9,len=1,*p=='+') && p[1]!='+') ||
			((op=C_SUB,opl=9,len=1,*p=='-') && p[1]!='-') ||
			(op=C_MUL,opl=10,len=1,*p=='*') ||
			(op=C_DIV,opl=10,len=1,*p=='/') ||
			(op=C_MOD,opl=10,len=1,*p=='%') ||
			(op=C_LAND,opl=2,len=2,*p=='&' && p[1]=='&') ||
			(op=C_AND,opl=5,len=1,*p=='&') ||
			(op=C_LOR,opl=1,len=2,*p=='|' && p[1]=='|') ||
			(op=C_OR,opl=3,len=1,*p=='|') ||
			(op=C_XOR,opl=4,len=1,*p=='^') ||
			(op=C_EQ,opl=6,len=2,*p=='=' && p[1]=='=') ||
			(op=C_NE,opl=6,len=2,*p=='!' && p[1]=='=') ||
			(op=C_R_SHIFT,opl=8,len=2,*p=='>' && p[1]=='>') ||
			(op=C_GE,opl=7,len=2,*p=='>' && p[1]=='=') ||
			(op=C_GT,opl=7,len=1,*p=='>') ||
			(op=C_L_SHIFT,opl=8,len=2,*p=='<' && p[1]=='<') ||
			(op=C_LE,opl=7,len=2,*p=='<' && p[1]=='=') ||
			(op=C_LT,opl=7,len=1,*p=='<')) && opl>limit){
		p+=len;
		if(op == C_OP3) {
			p=parse_subexpr(p,-1);
			p=skip_space(p);
			if( *(p++) != ':')
				disp_error_message("parse_subexpr: expected ':'", p-1);
			p=parse_subexpr(p,-1);
		} else {
			p=parse_subexpr(p,opl);
		}
		add_scriptc(op);
		p=skip_space(p);
	}

	return p;  /* return first untreated operator */
}

/*==========================================
 * Evaluation of the expression
 *------------------------------------------*/
const char* parse_expr(const char *p)
{
	switch(*p){
	case ')': case ';': case ':': case '[': case ']':
	case '}':
		disp_error_message("parse_expr: unexpected character",p);
	}
	p=parse_subexpr(p,-1);
	return p;
}

/*==========================================
 * Analysis of the line
 *------------------------------------------*/
const char* parse_line(const char* p)
{
	const char* p2;

	p=skip_space(p);
	if(*p==';') {
		//Close decision for if(); for(); while();
		p = parse_syntax_close(p + 1);
		return p;
	}
	if(*p==')' && parse_syntax_for_flag)
		return p+1;

	p = skip_space(p);
	if(p[0] == '{') {
		syntax.curly[syntax.curly_count].type  = TYPE_NULL;
		syntax.curly[syntax.curly_count].count = -1;
		syntax.curly[syntax.curly_count].index = -1;
		syntax.curly_count++;
		return p + 1;
	} else if(p[0] == '}') {
		return parse_curly_close(p);
	}

	// Syntax-related processing
	p2 = parse_syntax(p);
	if(p2 != NULL)
		return p2;

	// attempt to process a variable assignment
	p2 = parse_variable(p);

	if( p2 != NULL )
	{// variable assignment processed so leave the method
		return parse_syntax_close(p2 + 1);
	}

	p = parse_callfunc(p,0,0);
	p = skip_space(p);

	if(parse_syntax_for_flag) {
		if( *p != ')' )
			disp_error_message("parse_line: expected ')'",p);
	} else {
		if( *p != ';' )
			disp_error_message("parse_line: expected ';'",p);
	}

	//Binding decision for if(), for(), while()
	p = parse_syntax_close(p+1);

	return p;
}

// { ... } Closing process
const char* parse_curly_close(const char* p)
{
	if(syntax.curly_count <= 0) {
		disp_error_message("parse_curly_close: unexpected string",p);
		return p + 1;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_NULL) {
		syntax.curly_count--;
		//Close decision  if, for , while
		p = parse_syntax_close(p + 1);
		return p;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_SWITCH) {
		//Closing switch()
		int pos = syntax.curly_count-1;
		char label[256];
		int l;
		// Remove temporary variables
		sprintf(label,"set $@__SW%x_VAL,0;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// Go to the end pointer unconditionally
		sprintf(label,"goto __SW%x_FIN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// You are here labeled
		sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
		l=add_str(label);
		set_label(l,script_pos, p);

		if(syntax.curly[pos].flag) {
			//Exists default
			sprintf(label,"goto __SW%x_DEF;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;
		}

		// Label end
		sprintf(label,"__SW%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		set_label(l,script_pos, p);
		linkdb_final(&syntax.curly[pos].case_label);	// free the list of case label
		syntax.curly_count--;
		//Closing decision if, for , while
		p = parse_syntax_close(p + 1);
		return p;
	} else {
		disp_error_message("parse_curly_close: unexpected string",p);
		return p + 1;
	}
}

// Syntax-related processing
//	 break, case, continue, default, do, for, function,
//	 if, switch, while ? will handle this internally.
const char* parse_syntax(const char* p)
{
	const char *p2 = skip_word(p);

	switch(*p) {
	case 'B':
	case 'b':
		if(p2 - p == 5 && !strncasecmp(p,"break",5)) {
			// break Processing
			char label[256];
			int pos = syntax.curly_count - 1;
			while(pos >= 0) {
				if(syntax.curly[pos].type == TYPE_DO) {
					sprintf(label,"goto __DO%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_FOR) {
					sprintf(label,"goto __FR%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_WHILE) {
					sprintf(label,"goto __WL%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_SWITCH) {
					sprintf(label,"goto __SW%x_FIN;",syntax.curly[pos].index);
					break;
				}
				pos--;
			}
			if(pos < 0) {
				disp_error_message("parse_syntax: unexpected 'break'",p);
			} else {
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;
			}
			p = skip_space(p2);
			if(*p != ';')
				disp_error_message("parse_syntax: expected ';'",p);
			// Closing decision if, for , while
			p = parse_syntax_close(p + 1);
			return p;
		}
		break;
	case 'c':
	case 'C':
		if(p2 - p == 4 && !strncasecmp(p,"case",4)) {
			//Processing case
			int pos = syntax.curly_count-1;
			if(pos < 0 || syntax.curly[pos].type != TYPE_SWITCH) {
				disp_error_message("parse_syntax: unexpected 'case' ",p);
				return p+1;
			} else {
				char label[256];
				int  l;
				int64 v;
				char *np;
				if(syntax.curly[pos].count != 1) {
					//Jump for FALLTHRU
					sprintf(label,"goto __SW%x_%xJ;",syntax.curly[pos].index,syntax.curly[pos].count);
					syntax.curly[syntax.curly_count++].type = TYPE_NULL;
					parse_line(label);
					syntax.curly_count--;

					// You are here labeled
					sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
					l=add_str(label);
					set_label(l,script_pos, p);
				}
				//Decision statement switch
				p = skip_space(p2);
				if(p == p2) {
					disp_error_message("parse_syntax: expected a space ' '",p);
				}
				// check whether case label is integer or not
				if(is_number(p)) {
					//Numeric value
					v = (int)strtol(p,&np,0);
					if((*p == '-' || *p == '+') && ISDIGIT(p[1])) // pre-skip because '-' can not skip_word
						p++;
					p = skip_word(p);
					if(np != p)
						disp_error_message("parse_syntax: 'case' label is not an integer",np);
				} else {
					//Check for constants
					p2 = skip_word(p);
					v = (int)(size_t) (p2-p); // length of word at p2
					memcpy(label,p,static_cast<int>(v));
					label[v]='\0';
					if( !script_get_constant(label, &v) )
						disp_error_message("parse_syntax: 'case' label is not an integer",p);
					p = skip_word(p);
				}
				p = skip_space(p);
				if(*p != ':')
					disp_error_message("parse_syntax: expect ':'",p);
				sprintf(label,"if(%" PRId64 " != $@__SW%x_VAL) goto __SW%x_%x;",
					v,syntax.curly[pos].index,syntax.curly[pos].index,syntax.curly[pos].count+1);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				// Bad I do not parse twice
				p2 = parse_line(label);
				parse_line(p2);
				syntax.curly_count--;
				if(syntax.curly[pos].count != 1) {
					// Label after the completion of FALLTHRU
					sprintf(label,"__SW%x_%xJ",syntax.curly[pos].index,syntax.curly[pos].count);
					l=add_str(label);
					set_label(l,script_pos,p);
				}
				// check duplication of case label [Rayce]
				if(linkdb_search(&syntax.curly[pos].case_label, (void*)__64BPRTSIZE(v)) != NULL)
					disp_error_message("parse_syntax: dup 'case'",p);
				linkdb_insert(&syntax.curly[pos].case_label, (void*)__64BPRTSIZE(v), (void*)1);

				sprintf(label,"set $@__SW%x_VAL,0;",syntax.curly[pos].index);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;

				parse_line(label);
				syntax.curly_count--;
				syntax.curly[pos].count++;
			}
			return p + 1;
		} else if(p2 - p == 8 && !strncasecmp(p,"continue",8)) {
			// Processing continue
			char label[256];
			int pos = syntax.curly_count - 1;
			while(pos >= 0) {
				if(syntax.curly[pos].type == TYPE_DO) {
					sprintf(label,"goto __DO%x_NXT;",syntax.curly[pos].index);
					syntax.curly[pos].flag = 1; //Flag put the link for continue
					break;
				} else if(syntax.curly[pos].type == TYPE_FOR) {
					sprintf(label,"goto __FR%x_NXT;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_WHILE) {
					sprintf(label,"goto __WL%x_NXT;",syntax.curly[pos].index);
					break;
				}
				pos--;
			}
			if(pos < 0) {
				disp_error_message("parse_syntax: unexpected 'continue'",p);
			} else {
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;
			}
			p = skip_space(p2);
			if(*p != ';')
				disp_error_message("parse_syntax: expected ';'",p);
			//Closing decision if, for , while
			p = parse_syntax_close(p + 1);
			return p;
		}
		break;
	case 'd':
	case 'D':
		if(p2 - p == 7 && !strncasecmp(p,"default",7)) {
			// Switch - default processing
			int pos = syntax.curly_count-1;
			if(pos < 0 || syntax.curly[pos].type != TYPE_SWITCH) {
				disp_error_message("parse_syntax: unexpected 'default'",p);
			} else if(syntax.curly[pos].flag) {
				disp_error_message("parse_syntax: dup 'default'",p);
			} else {
				char label[256];
				int l;
				// Put the label location
				p = skip_space(p2);
				if(*p != ':') {
					disp_error_message("parse_syntax: expected ':'",p);
				}
				sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
				l=add_str(label);
				set_label(l,script_pos,p);

				// Skip to the next link w/o condition
				sprintf(label,"goto __SW%x_%x;",syntax.curly[pos].index,syntax.curly[pos].count+1);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;

				// The default label
				sprintf(label,"__SW%x_DEF",syntax.curly[pos].index);
				l=add_str(label);
				set_label(l,script_pos,p);

				syntax.curly[syntax.curly_count - 1].flag = 1;
				syntax.curly[pos].count++;
			}
			return p + 1;
		} else if(p2 - p == 2 && !strncasecmp(p,"do",2)) {
			int l;
			char label[256];
			p=skip_space(p2);

			syntax.curly[syntax.curly_count].type  = TYPE_DO;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			// Label of the (do) form here
			sprintf(label,"__DO%x_BGN",syntax.curly[syntax.curly_count].index);
			l=add_str(label);
			set_label(l,script_pos,p);
			syntax.curly_count++;
			return p;
		}
		break;
	case 'f':
	case 'F':
		if(p2 - p == 3 && !strncasecmp(p,"for",3)) {
			int l;
			char label[256];
			int  pos = syntax.curly_count;
			syntax.curly[syntax.curly_count].type  = TYPE_FOR;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			syntax.curly_count++;

			p=skip_space(p2);

			if(*p != '(')
				disp_error_message("parse_syntax: expected '('",p);
			p++;

			// Execute the initialization statement
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			p=parse_line(p);
			syntax.curly_count--;

			// Form the start of label decision
			sprintf(label,"__FR%x_J",syntax.curly[pos].index);
			l=add_str(label);
			set_label(l,script_pos,p);

			p=skip_space(p);
			if(*p == ';') {
				// For (; Because the pattern of always true ;)
				;
			} else {
				// Skip to the end point if the condition is false
				sprintf(label,"__FR%x_FIN",syntax.curly[pos].index);
				add_scriptl(add_str("jump_zero"));
				add_scriptc(C_ARG);
				p=parse_expr(p);
				p=skip_space(p);
				add_scriptl(add_str(label));
				add_scriptc(C_FUNC);
			}
			if(*p != ';')
				disp_error_message("parse_syntax: expected ';'",p);
			p++;

			// Skip to the beginning of the loop
			sprintf(label,"goto __FR%x_BGN;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;

			// Labels to form the next loop
			sprintf(label,"__FR%x_NXT",syntax.curly[pos].index);
			l=add_str(label);
			set_label(l,script_pos,p);

			// Process the next time you enter the loop
			// A ')' last for; flag to be treated as'
			parse_syntax_for_flag = 1;
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			p=parse_line(p);
			syntax.curly_count--;
			parse_syntax_for_flag = 0;

			// Skip to the determination process conditions
			sprintf(label,"goto __FR%x_J;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;

			// Loop start labeling
			sprintf(label,"__FR%x_BGN",syntax.curly[pos].index);
			l=add_str(label);
			set_label(l,script_pos,p);
			return p;
		}
		else if( p2 - p == 8 && strncasecmp(p,"function",8) == 0 )
		{// internal script function
			const char *func_name;

			func_name = skip_space(p2);
			p = skip_word(func_name);
			if( p == func_name )
				disp_error_message("parse_syntax:function: function name is missing or invalid", p);
			p2 = skip_space(p);
			if( *p2 == ';' )
			{// function <name> ;
				// function declaration - just register the name
				int l;
				l = add_word(func_name);
				if( str_data[l].type == C_NOP )// register only, if the name was not used by something else
					str_data[l].type = C_USERFUNC;
				else if( str_data[l].type == C_USERFUNC )
					;  // already registered
				else
					disp_error_message("parse_syntax:function: function name is invalid", func_name);

				// Close condition of if, for, while
				p = parse_syntax_close(p2 + 1);
				return p;
			}
			else if(*p2 == '{')
			{// function <name> <line/block of code>
				char label[256];
				int l;

				syntax.curly[syntax.curly_count].type  = TYPE_USERFUNC;
				syntax.curly[syntax.curly_count].count = 1;
				syntax.curly[syntax.curly_count].index = syntax.index++;
				syntax.curly[syntax.curly_count].flag  = 0;
				++syntax.curly_count;

				// Jump over the function code
				sprintf(label, "goto __FN%x_FIN;", syntax.curly[syntax.curly_count-1].index);
				syntax.curly[syntax.curly_count].type = TYPE_NULL;
				++syntax.curly_count;
				parse_line(label);
				--syntax.curly_count;

				// Set the position of the function (label)
				l=add_word(func_name);
				if( str_data[l].type == C_NOP || str_data[l].type == C_USERFUNC )// register only, if the name was not used by something else
				{
					str_data[l].type = C_USERFUNC;
					set_label(l, script_pos, p);
					if( parse_options&SCRIPT_USE_LABEL_DB )
						strdb_iput(scriptlabel_db, get_str(l), script_pos);
				}
				else
					disp_error_message("parse_syntax:function: function name is invalid", func_name);

				return skip_space(p);
			}
			else
			{
				disp_error_message("expect ';' or '{' at function syntax",p);
			}
		}
		break;
	case 'i':
	case 'I':
		if(p2 - p == 2 && !strncasecmp(p,"if",2)) {
			// If process
			char label[256];
			p=skip_space(p2);
			if(*p != '(') { //Prevent if this {} non-c syntax. from Rayce (jA)
				disp_error_message("need '('",p);
			}
			syntax.curly[syntax.curly_count].type  = TYPE_IF;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			sprintf(label,"__IF%x_%x",syntax.curly[syntax.curly_count].index,syntax.curly[syntax.curly_count].count);
			syntax.curly_count++;
			add_scriptl(add_str("jump_zero"));
			add_scriptc(C_ARG);
			p=parse_expr(p);
			p=skip_space(p);
			add_scriptl(add_str(label));
			add_scriptc(C_FUNC);
			return p;
		}
		break;
	case 's':
	case 'S':
		if(p2 - p == 6 && !strncasecmp(p,"switch",6)) {
			// Processing of switch ()
			char label[256];
			p=skip_space(p2);
			if(*p != '(') {
				disp_error_message("need '('",p);
			}
			syntax.curly[syntax.curly_count].type  = TYPE_SWITCH;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			sprintf(label,"$@__SW%x_VAL",syntax.curly[syntax.curly_count].index);
			syntax.curly_count++;
			add_scriptl(add_str("set"));
			add_scriptc(C_ARG);
			add_scriptl(add_str(label));
			p=parse_expr(p);
			p=skip_space(p);
			if(*p != '{') {
				disp_error_message("parse_syntax: expected '{'",p);
			}
			add_scriptc(C_FUNC);
			return p + 1;
		}
		break;
	case 'w':
	case 'W':
		if(p2 - p == 5 && !strncasecmp(p,"while",5)) {
			int l;
			char label[256];
			p=skip_space(p2);
			if(*p != '(') {
				disp_error_message("need '('",p);
			}
			syntax.curly[syntax.curly_count].type  = TYPE_WHILE;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			// Form the start of label decision
			sprintf(label,"__WL%x_NXT",syntax.curly[syntax.curly_count].index);
			l=add_str(label);
			set_label(l,script_pos,p);

			// Skip to the end point if the condition is false
			sprintf(label,"__WL%x_FIN",syntax.curly[syntax.curly_count].index);
			syntax.curly_count++;
			add_scriptl(add_str("jump_zero"));
			add_scriptc(C_ARG);
			p=parse_expr(p);
			p=skip_space(p);
			add_scriptl(add_str(label));
			add_scriptc(C_FUNC);
			return p;
		}
		break;
	}
	return NULL;
}

const char* parse_syntax_close(const char *p) {
	// If (...) for (...) hoge (); as to make sure closed closed once again
	int flag;

	do {
		p = parse_syntax_close_sub(p,&flag);
	} while(flag);
	return p;
}

// Close judgment if, for, while, of do
//	 flag == 1 : closed
//	 flag == 0 : not closed
const char* parse_syntax_close_sub(const char* p,int* flag)
{
	char label[256];
	int pos = syntax.curly_count - 1;
	int l;
	*flag = 1;

	if(syntax.curly_count <= 0) {
		*flag = 0;
		return p;
	} else if(syntax.curly[pos].type == TYPE_IF) {
		const char *bp = p;
		const char *p2;

		// if-block and else-block end is a new line
		parse_nextline(false, p);

		// Skip to the last location if
		sprintf(label,"goto __IF%x_FIN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// Put the label of the location
		sprintf(label,"__IF%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
		l=add_str(label);
		set_label(l,script_pos,p);

		syntax.curly[pos].count++;
		p = skip_space(p);
		p2 = skip_word(p);
		if(!syntax.curly[pos].flag && p2 - p == 4 && !strncasecmp(p,"else",4)) {
			// else  or else - if
			p = skip_space(p2);
			p2 = skip_word(p);
			if(p2 - p == 2 && !strncasecmp(p,"if",2)) {
				// else - if
				p=skip_space(p2);
				if(*p != '(') {
					disp_error_message("need '('",p);
				}
				sprintf(label,"__IF%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
				add_scriptl(add_str("jump_zero"));
				add_scriptc(C_ARG);
				p=parse_expr(p);
				p=skip_space(p);
				add_scriptl(add_str(label));
				add_scriptc(C_FUNC);
				*flag = 0;
				return p;
			} else {
				// else
				if(!syntax.curly[pos].flag) {
					syntax.curly[pos].flag = 1;
					*flag = 0;
					return p;
				}
			}
		}
		// Close if
		syntax.curly_count--;
		// Put the label of the final location
		sprintf(label,"__IF%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		set_label(l,script_pos,p);
		if(syntax.curly[pos].flag == 1) {
			// Because the position of the pointer is the same if not else for this
			return bp;
		}
		return p;
	} else if(syntax.curly[pos].type == TYPE_DO) {
		int l2;
		char label2[256];
		const char *p2;

		if(syntax.curly[pos].flag) {
			// (Come here continue) to form the label here
			sprintf(label2,"__DO%x_NXT",syntax.curly[pos].index);
			l2=add_str(label2);
			set_label(l2,script_pos,p);
		}

		// Skip to the end point if the condition is false
		p = skip_space(p);
		p2 = skip_word(p);
		if(p2 - p != 5 || strncasecmp(p,"while",5))
			disp_error_message("parse_syntax: expected 'while'",p);

		p = skip_space(p2);
		if(*p != '(') {
			disp_error_message("need '('",p);
		}

		// do-block end is a new line
		parse_nextline(false, p);

		sprintf(label2,"__DO%x_FIN",syntax.curly[pos].index);
		add_scriptl(add_str("jump_zero"));
		add_scriptc(C_ARG);
		p=parse_expr(p);
		p=skip_space(p);
		add_scriptl(add_str(label2));
		add_scriptc(C_FUNC);

		// Skip to the starting point
		sprintf(label2,"goto __DO%x_BGN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label2);
		syntax.curly_count--;

		// Form label of the end point conditions
		sprintf(label2,"__DO%x_FIN",syntax.curly[pos].index);
		l2=add_str(label2);
		set_label(l2,script_pos,p);
		p = skip_space(p);
		if(*p != ';') {
			disp_error_message("parse_syntax: expected ';'",p);
			return p+1;
		}
		p++;
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[pos].type == TYPE_FOR) {
		// for-block end is a new line
		parse_nextline(false, p);

		// Skip to the next loop
		sprintf(label,"goto __FR%x_NXT;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// End for labeling
		sprintf(label,"__FR%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		set_label(l,script_pos,p);
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[pos].type == TYPE_WHILE) {
		// while-block end is a new line
		parse_nextline(false, p);

		// Skip to the decision while
		sprintf(label,"goto __WL%x_NXT;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// End while labeling
		sprintf(label,"__WL%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		set_label(l,script_pos,p);
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_USERFUNC) {
		int pos2 = syntax.curly_count-1;
		char label2[256];
		int l2;
		// Back
		sprintf(label2,"return;");
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label2);
		syntax.curly_count--;

		// Put the label of the location
		sprintf(label2,"__FN%x_FIN",syntax.curly[pos2].index);
		l2=add_str(label2);
		set_label(l2,script_pos,p);
		syntax.curly_count--;
		return p;
	} else {
		*flag = 0;
		return p;
	}
}

/*==========================================
 * Added built-in functions
 *------------------------------------------*/
static void add_buildin_func(void)
{
	int i;
	for( i = 0; buildin_func[i].func; i++ )
	{
		// arg must follow the pattern: (v|s|i|r|l)*\?*\*?
		// 'v' - value (either string or int or reference)
		// 's' - string
		// 'i' - int
		// 'r' - reference (of a variable)
		// 'l' - label
		// '?' - one optional parameter
		// '*' - unknown number of optional parameters
		const char* p = buildin_func[i].arg;
		while( *p == 'v' || *p == 's' || *p == 'i' || *p == 'r' || *p == 'l' ) ++p;
		while( *p == '?' ) ++p;
		if( *p == '*' ) ++p;
		if( *p != 0){
			ShowWarning("add_buildin_func: ignoring function \"%s\" with invalid arg \"%s\".\n", buildin_func[i].name, buildin_func[i].arg);
		} else if( *skip_word(buildin_func[i].name) != 0 ){
			ShowWarning("add_buildin_func: ignoring function with invalid name \"%s\" (must be a word).\n", buildin_func[i].name);
		} else {
			int n = add_str(buildin_func[i].name);
			str_data[n].type = C_FUNC;
			str_data[n].val = i;
			str_data[n].func = buildin_func[i].func;
			str_data[n].deprecated = (buildin_func[i].deprecated != NULL);

			if (!strcmp(buildin_func[i].name, "setr")) buildin_set_ref = n;
			else if (!strcmp(buildin_func[i].name, "callsub")) buildin_callsub_ref = n;
			else if (!strcmp(buildin_func[i].name, "callfunc")) buildin_callfunc_ref = n;
			else if( !strcmp(buildin_func[i].name, "getelementofarray") ) buildin_getelementofarray_ref = n;
		}
	}
}

/**
 * String comparison with a char array to a script constant
 * @param prefix: Char array to compare
 * @param value: Script constant
 * @return Constant name as char array or NULL otherwise
 */
const char* script_get_constant_str( const char* prefix, int64 value ){
	const char* name;

	for(int i = 0; i < str_data_size; i++ ){
		// Check if it is a constant
		if( str_data[i].type != C_INT ){
			continue;
		}

		// Check if the value matches
		if( str_data[i].val != value ){
			continue;
		}

		// Look up the actual string
		name = get_str(i);

		// Compare the prefix
		if( !strncasecmp( name, prefix, strlen(prefix) ) ){
			// We found a match
			return name;
		}
	}

	return NULL;
}

/// Retrieves the value of a constant parameter.
bool script_get_parameter(const char* name, int64* value)
{
	int n = search_str(name);

	if (n == -1 || str_data[n].type != C_PARAM)
	{// not found or not a parameter
		return false;
	}
	value[0] = str_data[n].val;

	return true;
}

/// Retrieves the value of a constant.
bool script_get_constant(const char* name, int64* value)
{
	int n = search_str(name);

	if( n == -1 || str_data[n].type != C_INT )
	{// not found or not a constant
		return false;
	}
	value[0] = str_data[n].val;

#if defined(SCRIPT_CONSTANT_DEPRECATION)
	if( str_data[n].deprecated ){
		ShowWarning( "Usage of deprecated constant '%s'.\n", name );
		ShowWarning( "This constant was deprecated and could become unavailable anytime soon.\n" );
		if (str_data[n].name)
			ShowWarning( "Please use '%s' instead!\n", str_data[n].name );
	}
#endif

	return true;
}

/// Creates new constant or parameter with given value.
void script_set_constant_(const char* name, int64 value, const char* constant_name, bool isparameter, bool deprecated)
{
	int n = add_str(name);

	if( str_data[n].type == C_NOP )
	{// new
		str_data[n].type = isparameter ? C_PARAM : C_INT;
		str_data[n].val  = value;
		str_data[n].deprecated = deprecated;
		str_data[n].name = constant_name;
	}
	else if( str_data[n].type == C_PARAM || str_data[n].type == C_INT )
	{// existing parameter or constant
		ShowError("script_set_constant: Attempted to overwrite existing %s '%s' (old value=%" PRId64 ", new value=%" PRId64 ").\n", ( str_data[n].type == C_PARAM ) ? "parameter" : "constant", name, str_data[n].val, value);
	}
	else
	{// existing name
		ShowError("script_set_constant: Invalid name for %s '%s' (already defined as %s).\n", isparameter ? "parameter" : "constant", name, script_op2name(str_data[n].type));
	}
}

const std::string ConstantDatabase::getDefaultLocation(){
	return std::string(db_path) + "/const.yml";
}

uint64 ConstantDatabase::parseBodyNode( const ryml::NodeRef& node ) {
	std::string constant_name;

	if (!this->asString( node, "Name", constant_name ))
		return 0;

	char name[1024];

	if (sscanf(constant_name.c_str(), "%1023[A-Za-z0-9/_]", name) != 1) {
		this->invalidWarning( node["Name"], "Invalid constant definition \"%s\", skipping.\n", constant_name.c_str() );
		return 0;
	}

	int64 val;

	if (!this->asInt64( node, "Value", val ))
		return 0;

	bool type = false;

	if (this->nodeExists(node, "Parameter") && !this->asBool( node, "Parameter", type ))
		return 0;

	script_set_constant(name, val, type, false);

	return 1;
}

ConstantDatabase constant_db;

/**
 * Sets source-end constants for NPC scripts to access.
 **/
void script_hardcoded_constants(void) {
	#include "script_constants.hpp"
}

/*==========================================
 * Display emplacement line of script
 *------------------------------------------*/
static const char* script_print_line(StringBuf* buf, const char* p, const char* mark, int line)
{
	int i;
	if( p == NULL || !p[0] ) return NULL;
	if( line < 0 )
		StringBuf_Printf(buf, "*% 5d : ", -line);
	else
		StringBuf_Printf(buf, " % 5d : ", line);
	for(i=0;p[i] && p[i] != '\n';i++){
		if(p + i != mark)
			StringBuf_Printf(buf, "%c", p[i]);
		else
			StringBuf_Printf(buf, "\'%c\'", p[i]);
	}
	StringBuf_AppendStr(buf, "\n");
	return p+i+(p[i] == '\n' ? 1 : 0);
}

void script_errorwarning_sub(StringBuf *buf, const char* src, const char* file, int start_line, const char* error_msg_cur, const char* error_pos_cur) {
	// Find the line where the error occurred
	int j;
	int line = start_line;
	const char *p;
	const char *linestart[5] = { NULL, NULL, NULL, NULL, NULL };

	for(p=src;p && *p;line++){
		const char *lineend=strchr(p,'\n');
		if(lineend==NULL || error_pos_cur<lineend){
			break;
		}
		for( j = 0; j < 4; j++ ) {
			linestart[j] = linestart[j+1];
		}
		linestart[4] = p;
		p=lineend+1;
	}

	StringBuf_Printf(buf, "script error on %s line %d\n", file, line);
	StringBuf_Printf(buf, "    %s\n", error_msg_cur);
	for(j = 0; j < 5; j++ ) {
		script_print_line(buf, linestart[j], NULL, line + j - 5);
	}
	p = script_print_line(buf, p, error_pos_cur, -line);
	for(j = 0; j < 5; j++) {
		p = script_print_line(buf, p, NULL, line + j + 1 );
	}
}

void script_error(const char* src, const char* file, int start_line, const char* error_msg_cur, const char* error_pos_cur) {
	StringBuf buf;

	StringBuf_Init(&buf);
	StringBuf_AppendStr(&buf, "\a\n");

	script_errorwarning_sub(&buf, src, file, start_line, error_msg_cur, error_pos_cur);

	ShowError("%s", StringBuf_Value(&buf));
	StringBuf_Destroy(&buf);
}

void script_warning(const char* src, const char* file, int start_line, const char* error_msg_cur, const char* error_pos_cur) {
	StringBuf buf;

	StringBuf_Init(&buf);

	script_errorwarning_sub(&buf, src, file, start_line, error_msg_cur, error_pos_cur);

	ShowWarning("%s", StringBuf_Value(&buf));
	StringBuf_Destroy(&buf);
}

/*==========================================
 * Analysis of the script
 *------------------------------------------*/
struct script_code* parse_script(const char *src,const char *file,int line,int options)
{
	const char *p,*tmpp;
	int i;
	struct script_code* code = NULL;
	char end;
	bool unresolved_names = false;

	parser_current_src = src;
	parser_current_file = file;
	parser_current_line = line;

	if( src == NULL )
		return NULL;// empty script

	memset(&syntax,0,sizeof(syntax));

	script_buf=(unsigned char *)aMalloc(SCRIPT_BLOCK_SIZE*sizeof(unsigned char));
	script_pos=0;
	script_size=SCRIPT_BLOCK_SIZE;
	parse_nextline(true, NULL);

	// who called parse_script is responsible for clearing the database after using it, but just in case... lets clear it here
	if( options&SCRIPT_USE_LABEL_DB )
		db_clear(scriptlabel_db);
	parse_options = options;

	if( setjmp( error_jump ) != 0 ) {
		//Restore program state when script has problems. [from jA]
		int j;
		const int size = ARRAYLENGTH(syntax.curly);
		if( error_report )
			script_error(src,file,line,error_msg,error_pos);
		aFree( error_msg );
		aFree( script_buf );
		script_pos  = 0;
		script_size = 0;
		script_buf  = NULL;
		for(j=LABEL_START;j<str_num;j++)
			if(str_data[j].type == C_NOP) str_data[j].type = C_NAME;
		for(j=0; j<size; j++)
			linkdb_final(&syntax.curly[j].case_label);
		return NULL;
	}

	parse_syntax_for_flag=0;
	p=src;
	p=skip_space(p);
	if( options&SCRIPT_IGNORE_EXTERNAL_BRACKETS )
	{// does not require brackets around the script
		if( *p == '\0' && !(options&SCRIPT_RETURN_EMPTY_SCRIPT) )
		{// empty script and can return NULL
			aFree( script_buf );
			script_pos  = 0;
			script_size = 0;
			script_buf  = NULL;
			return NULL;
		}
		end = '\0';
	}
	else
	{// requires brackets around the script
		if( *p != '{' )
			disp_error_message("not found '{'",p);
		p = skip_space(p+1);
		if( *p == '}' && !(options&SCRIPT_RETURN_EMPTY_SCRIPT) )
		{// empty script and can return NULL
			aFree( script_buf );
			script_pos  = 0;
			script_size = 0;
			script_buf  = NULL;
			return NULL;
		}
		end = '}';
	}

	// clear references of labels, variables and internal functions
	for(i=LABEL_START;i<str_num;i++){
		if(
			str_data[i].type==C_POS || str_data[i].type==C_NAME ||
			str_data[i].type==C_USERFUNC || str_data[i].type == C_USERFUNC_POS
		){
			str_data[i].type=C_NOP;
			str_data[i].backpatch=-1;
			str_data[i].label=-1;
		}
	}

	while( syntax.curly_count != 0 || *p != end )
	{
		if( *p == '\0' )
			disp_error_message("unexpected end of script",p);
		// Special handling only label
		tmpp=skip_space(skip_word(p));
		if(*tmpp==':' && !(!strncasecmp(p,"default:",8) && p + 7 == tmpp)){
			i=add_word(p);
			set_label(i,script_pos,p);
			if( parse_options&SCRIPT_USE_LABEL_DB )
				strdb_iput(scriptlabel_db, get_str(i), script_pos);
			p=tmpp+1;
			p=skip_space(p);
			continue;
		}

		// All other lumped
		p=parse_line(p);
		p=skip_space(p);

		parse_nextline(false, p);
	}

	add_scriptc(C_NOP);

	// trim code to size
	script_size = script_pos;
	RECREATE(script_buf,unsigned char,script_pos);

	// default unknown references to variables
	for(i=LABEL_START;i<str_num;i++){
		if(str_data[i].type==C_NOP){
			int j;
			str_data[i].type=C_NAME;
			str_data[i].label=i;
			for(j=str_data[i].backpatch;j>=0 && j!=0x00ffffff;){
				int next=GETVALUE(script_buf,j);
				SETVALUE(script_buf,j,i);
				j=next;
			}
		}
		else if( str_data[i].type == C_USERFUNC )
		{// 'function name;' without follow-up code
			ShowError("parse_script: function '%s' declared but not defined.\n", str_buf+str_data[i].str);
			unresolved_names = true;
		}
	}

	if( unresolved_names )
	{
		disp_error_message("parse_script: unresolved function references", p);
	}

#ifdef DEBUG_DISP
	for(i=0;i<script_pos;i++){
		if((i&15)==0) ShowMessage("%04x : ",i);
		ShowMessage("%02x ",script_buf[i]);
		if((i&15)==15) ShowMessage("\n");
	}
	ShowMessage("\n");
#endif
#ifdef DEBUG_DISASM
	{
		int i = 0,j;
		while(i < script_pos) {
			c_op op = get_com(script_buf,&i);

			ShowMessage("%06x %s", i, script_op2name(op));
			j = i;
			switch(op) {
			case C_INT:
				ShowMessage(" %d", get_num(script_buf,&i));
				break;
			case C_POS:
				ShowMessage(" 0x%06x", *(int*)(script_buf+i)&0xffffff);
				i += 3;
				break;
			case C_NAME:
				j = (*(int*)(script_buf+i)&0xffffff);
				ShowMessage(" %s", ( j == 0xffffff ) ? "?? unknown ??" : get_str(j));
				i += 3;
				break;
			case C_STR:
				j = strlen(script_buf + i);
				ShowMessage(" %s", script_buf + i);
				i += j+1;
				break;
			}
			ShowMessage(CL_CLL"\n");
		}
	}
#endif

	CREATE(code,struct script_code,1);
	code->script_buf  = script_buf;
	code->script_size = script_size;
	code->local.vars = NULL;
	code->local.arrays = NULL;
	return code;
}

/// Returns the player attached to this script, identified by the rid.
/// If there is no player attached, the script is terminated.
static bool script_rid2sd_( struct script_state *st, struct map_session_data** sd, const char *func ){
	*sd = map_id2sd( st->rid );

	if( *sd ){
		return true;
	}else{
		ShowError("%s: fatal error ! player not attached!\n",func);
		script_reportfunc(st);
		script_reportsrc(st);
		st->state = END;
		return false;
	}
}

/**
 * Dereferences a variable/constant, replacing it with a copy of the value.
 * @param st Script state
 * @param data Variable/constant
 * @param sd If NULL, will try to use sd from st->rid (for player's variables)
 */
struct script_data *get_val_(struct script_state* st, struct script_data* data, struct map_session_data *sd)
{
	const char* name;
	char prefix;
	char postfix;

	if( !data_isreference(data) )
		return data;// not a variable/constant

	name = reference_getname(data);
	prefix = name[0];
	postfix = name[strlen(name) - 1];

	//##TODO use reference_tovariable(data) when it's confirmed that it works [FlavioJS]
	if( !reference_toconstant(data) && not_server_variable(prefix) ) {
		if( sd == NULL && !script_rid2sd(sd) ) {// needs player attached
			if( postfix == '$' ) {// string variable
				ShowWarning("script:get_val: cannot access player variable '%s', defaulting to \"\"\n", name);
				data->type = C_CONSTSTR;
				data->u.str = const_cast<char *>("");
			} else {// integer variable
				ShowWarning("script:get_val: cannot access player variable '%s', defaulting to 0\n", name);
				data->type = C_INT;
				data->u.num = 0;
			}
			return data;
		}
	}

	if( postfix == '$' ) {// string variable

		switch( prefix ) {
			case '@':
				data->u.str = pc_readregstr(sd, data->u.num);
				break;
			case '$':
				data->u.str = mapreg_readregstr(data->u.num);
				break;
			case '#':
				if( name[1] == '#' )
					data->u.str = pc_readaccountreg2str(sd, data->u.num);// global
				else
					data->u.str = pc_readaccountregstr(sd, data->u.num);// local
				break;
			case '.':
				{
					struct DBMap* n = data->ref ?
							data->ref->vars : name[1] == '@' ?
							st->stack->scope.vars : // instance/scope variable
							st->script->local.vars; // npc variable
					if( n )
						data->u.str = (char*)i64db_get(n,reference_getuid(data));
					else
						data->u.str = NULL;
				}
				break;
			case '\'':
				{
					struct DBMap* n = nullptr;
					if (data->ref)
						n = data->ref->vars;
					else {
						std::shared_ptr<s_instance_data> idata = util::umap_find(instances, script_instancegetid(st));

						if (idata)
							n = idata->regs.vars;
					}
					if (n)
						data->u.str = (char*)i64db_get(n,reference_getuid(data));
					else {
						ShowWarning("script:get_val: cannot access instance variable '%s', defaulting to \"\"\n", name);
						data->u.str = NULL;
					}
					break;
				}
			default:
				data->u.str = pc_readglobalreg_str(sd, data->u.num);
				break;
		}

		if( data->u.str == NULL || data->u.str[0] == '\0' ) {// empty string
			data->type = C_CONSTSTR;
			data->u.str = const_cast<char *>("");
		} else {// duplicate string
			data->type = C_STR;
			data->u.str = aStrdup(data->u.str);
		}

	} else {// integer variable

		data->type = C_INT;

		if( reference_toconstant(data) ) {
			data->u.num = reference_getconstant(data);
		} else if( reference_toparam(data) ) {
			data->u.num = pc_readparam(sd, reference_getparamtype(data));
		} else
			switch( prefix ) {
				case '@':
					data->u.num = pc_readreg(sd, data->u.num);
					break;
				case '$':
					data->u.num = mapreg_readreg(data->u.num);
					break;
				case '#':
					if( name[1] == '#' )
						data->u.num = pc_readaccountreg2(sd, data->u.num);// global
					else
						data->u.num = pc_readaccountreg(sd, data->u.num);// local
					break;
				case '.':
					{
						struct DBMap* n = data->ref ?
								data->ref->vars : name[1] == '@' ?
								st->stack->scope.vars : // instance/scope variable
								st->script->local.vars; // npc variable
						if( n )
							data->u.num = i64db_i64get(n,reference_getuid(data));
						else
							data->u.num = 0;
					}
					break;
				case '\'':
					{
						struct DBMap* n = nullptr;
						if (data->ref)
							n = data->ref->vars;
						else {
							std::shared_ptr<s_instance_data> idata = util::umap_find(instances, script_instancegetid(st));

							if (idata)
								n = idata->regs.vars;
						}
						if (n)
							data->u.num = i64db_i64get(n,reference_getuid(data));
						else {
							ShowWarning("script:get_val: cannot access instance variable '%s', defaulting to 0\n", name);
							data->u.num = 0;
						}
						break;
					}
				default:
					data->u.num = pc_readglobalreg(sd, data->u.num);
					break;
			}

	}
	data->ref = NULL;

	return data;
}

struct script_data *get_val(struct script_state* st, struct script_data* data)
{
	return get_val_(st,data,NULL);
}

struct script_data* push_val2(struct script_stack* stack, enum c_op type, int64 val, struct reg_db* ref);

const char* get_val2_str( struct script_state* st, int64 uid, struct reg_db* ref ){
	push_val2( st->stack, C_NAME, uid, ref );

	struct script_data* data = script_getdatatop( st, -1 );

	get_val( st, data );

	const char* value = "";

	if( data->type == C_INT ){
		ShowError( "get_val2_num: Invalid call. Variable %s is a numeric type.\n", reference_getname( data ) );
	}else{
		value = data->u.str;
	}

	// Do NOT remove the value from stack here, the pointer is returned here and will be used by the caller [Lemongrass]
	// script_removetop( st, -1, 0 );

	return value;
}

int64 get_val2_num( struct script_state* st, int64 uid, struct reg_db* ref ){
	push_val2( st->stack, C_NAME, uid, ref );

	struct script_data* data = script_getdatatop( st, -1 );

	get_val( st, data );

	int64 value = 0;

	if( data->type == C_INT ){
		value = data->u.num;
	}else{
		ShowError( "get_val2_num: Invalid call. Variable %s is not a numeric type.\n", reference_getname( data ) );
	}

	script_removetop( st, -1, 0 );

	return value;
}

/**
 * Because, currently, array members with key 0 are indifferenciable from normal variables, we should ensure its actually in
 * Will be gone as soon as undefined var feature is implemented
 **/
void script_array_ensure_zero(struct script_state *st, struct map_session_data *sd, int64 uid, struct reg_db *ref)
{
	const char *name = get_str(script_getvarid(uid));
	// is here st can be null pointer and st->rid is wrong?
	struct reg_db *src = script_array_src(st, sd ? sd : st->rid ? map_id2sd(st->rid) : NULL, name, ref);
	bool insert = false;

	if (sd && !st) {
		// when sd comes, st isn't available
		insert = true;
	} else {
		if( is_string_variable(name) ) {
			const char* str = get_val2_str( st, uid, ref );
			if( str && *str )
				insert = true;
			// Remove stack entry from get_val2_str
			script_removetop( st, -1, 0 );
		} else {
			int64 num = get_val2_num( st, uid, ref );
			if( num )
				insert = true;
		}
	}

	if (src && src->arrays) {
		struct script_array *sa = static_cast<script_array *>(idb_get(src->arrays, script_getvarid(uid)));
		if (sa) {
			unsigned int i;

			ARR_FIND(0, sa->size, i, sa->members[i] == 0);
			if( i != sa->size ) {
				if( !insert )
					script_array_remove_member(src,sa,i);
				return;
			}

			script_array_add_member(sa,0);
		} else if (insert) {
			script_array_update(src,reference_uid(script_getvarid(uid), 0),false);
		}
	}
}

/**
 * Returns array size by ID
 **/
unsigned int script_array_size(struct script_state *st, struct map_session_data *sd, const char *name, struct reg_db *ref)
{
	struct script_array *sa = NULL;
	struct reg_db *src = script_array_src(st, sd, name, ref);

	if (src && src->arrays)
		sa = static_cast<script_array *>(idb_get(src->arrays, search_str(name)));

	return sa ? sa->size : 0;
}

/**
 * Returns array's highest key (for that awful getarraysize implementation that doesn't really gets the array size)
 **/
unsigned int script_array_highest_key(struct script_state *st, struct map_session_data *sd, const char *name, struct reg_db *ref)
{
	struct script_array *sa = NULL;
	struct reg_db *src = script_array_src(st, sd, name, ref);

	if (src && src->arrays) {
		int key = add_word(name);

		script_array_ensure_zero(st,sd,reference_uid(key, 0), ref);

		if( ( sa = static_cast<script_array *>(idb_get(src->arrays, key)) ) ) {
			unsigned int i, highest_key = 0;

			for(i = 0; i < sa->size; i++) {
				if( sa->members[i] > highest_key )
					highest_key = sa->members[i];
			}

			return sa->size ? highest_key + 1 : 0;
		}
	}
	
	return SCRIPT_CMD_SUCCESS;
}

int script_free_array_db(DBKey key, DBData *data, va_list ap)
{
	struct script_array *sa = static_cast<script_array *>(db_data2ptr(data));
	aFree(sa->members);
	ers_free(array_ers, sa);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Clears script_array and removes it from script->array_db
 **/
void script_array_delete(struct reg_db *src, struct script_array *sa)
{
	aFree(sa->members);
	idb_remove(src->arrays, sa->id);
	ers_free(array_ers, sa);
}

/**
 * Removes a member from a script_array list
 *
 * @param idx the index of the member in script_array struct list, not of the actual array member
 **/
void script_array_remove_member(struct reg_db *src, struct script_array *sa, unsigned int idx)
{
	unsigned int i, cursor;

	// it's the only member left, no need to do anything other than delete the array data
	if( sa->size == 1 ) {
		script_array_delete(src,sa);
		return;
	}

	sa->members[idx] = UINT_MAX;

	for(i = 0, cursor = 0; i < sa->size; i++) {
		if( sa->members[i] == UINT_MAX )
			continue;
		if( i != cursor )
			sa->members[cursor] = sa->members[i];
		cursor++;
	}

	sa->size = cursor;
}

/**
 * Appends a new array index to the list in script_array
 *
 * @param idx the index of the array member being inserted
 **/
void script_array_add_member(struct script_array *sa, unsigned int idx)
{
	RECREATE(sa->members, unsigned int, ++sa->size);

	sa->members[sa->size - 1] = idx;
}

/**
 * Obtains the source of the array database for this type and scenario
 * Initializes such database when not yet initialized.
 **/
struct reg_db *script_array_src(struct script_state *st, struct map_session_data *sd, const char *name, struct reg_db *ref)
{
	struct reg_db *src = NULL;

	switch( name[0] ) {
		// from player
		default:  // char reg
		case '@': // temp char reg
		case '#': // account reg
			src = &sd->regs;
			break;
		case '$': // map reg
			src = &regs;
			break;
		case '.': // npc/script
			if (ref)
				src = ref;
			else
				src = (name[1] == '@') ? &st->stack->scope : &st->script->local;
			break;
		case '\'': // instance
			{
				if (ref)
					src = ref;
				else {
					std::shared_ptr<s_instance_data> idata = util::umap_find(instances, script_instancegetid(st));

					if (idata)
						src = &idata->regs;
				}
				break;
			}
	}

	if( src ) {
		if( !src->arrays )
			src->arrays = idb_alloc(DB_OPT_BASE);
		return src;
	}

	return NULL;
}

/**
 * Processes a array member modification, and update data accordingly
 *
 * @param src[in,out] Variable source database. If the array database doesn't exist, it is created.
 * @param num[in]     Variable ID
 * @param empty[in]   Whether the modified member is empty (needs to be removed)
 **/
void script_array_update(struct reg_db *src, int64 num, bool empty)
{
	struct script_array *sa = NULL;
	int id = script_getvarid(num);
	unsigned int index = script_getvaridx(num);

	if (!src->arrays) {
		src->arrays = idb_alloc(DB_OPT_BASE);
	} else {
		sa = static_cast<script_array *>(idb_get(src->arrays, id));
	}

	if( sa ) {
		unsigned int i;

		// search
		for(i = 0; i < sa->size; i++) {
			if( sa->members[i] == index )
				break;
		}

		// if existent
		if( i != sa->size ) {
			// if empty, we gotta remove it
			if( empty ) {
				script_array_remove_member(src, sa, i);
			}
		} else if( !empty ) { /* new entry */
			script_array_add_member(sa,index);
			// we do nothing if its empty, no point in modifying array data for a new empty member
		}
	} else if ( !empty ) { // we only move to create if not empty
		sa = ers_alloc(array_ers, struct script_array);
		sa->id = id;
		sa->members = NULL;
		sa->size = 0;
		script_array_add_member(sa,index);
		idb_put(src->arrays, id, sa);
	}
}

/**
 * Stores the value of a script variable
 *
 * @param st:    current script state.
 * @param sd:    current character data.
 * @param num:   variable identifier.
 * @param name:  variable name.
 * @param value: new value.
 * @param ref:   variable container, in case of a npc/scope variable reference outside the current scope.
 * @return: 0 failure, 1 success.
 *
 * TODO: return values are screwed up, have been for some time (reaad: years), e.g. some functions return 1 failure and success.
 *------------------------------------------*/
bool set_reg_str( struct script_state* st, struct map_session_data* sd, int64 num, const char* name, const char* value, struct reg_db *ref ){
	char prefix = name[0];
	size_t vlen = 0;

	if( !script_check_RegistryVariableLength( 0, name, &vlen ) ){
		ShowError( "set_reg: Variable name length is too long (aid: %d, cid: %d): '%s' sz=%" PRIuPTR "\n", sd ? sd->status.account_id : -1, sd ? sd->status.char_id : -1, name, vlen );
		return false;
	}

	if( !is_string_variable( name ) ){
		// integer variable
		return false;
	}

	switch( prefix ){
		case '@':
			pc_setregstr( sd, num, value );
			return true;
		case '$':
			return mapreg_setregstr( num, value );
		case '#':
			return ( name[1] == '#' ) ? pc_setaccountreg2str( sd, num, value ) : pc_setaccountregstr( sd, num, value );
		case '.': {
				struct reg_db *n = ( ref ) ? ref : ( name[1] == '@' ) ? &st->stack->scope : &st->script->local;

				if( n ){
					if( value[0] ){
						i64db_put( n->vars, num, aStrdup( value ) );

						if( script_getvaridx( num ) ){
							script_array_update( n, num, false );
						}
					}else{
						i64db_remove( n->vars, num );

						if( script_getvaridx( num ) ){
							script_array_update( n, num, true );
						}
					}
				}
			}
			return true;
		case '\'': {
				struct reg_db *src = nullptr;

				if( ref ){
					src = ref;
				}else{
					std::shared_ptr<s_instance_data> idata = util::umap_find(instances, script_instancegetid(st));

					if (idata)
						src = &idata->regs;
				}

				if( src ){
					bool empty;

					if( value[0] ){
						i64db_put( src->vars, num, aStrdup( value ) );
						empty = false;
					}else{
						i64db_remove( src->vars, num );
						empty = true;
					}

					if( script_getvaridx( num ) != 0 ){
						script_array_update( src, num, empty );
					}
				}else{
					ShowError( "script_set_reg: cannot write instance variable '%s', NPC not in a instance!\n", name );
					script_reportsrc( st );
				}

				return true;
			}
		default:
			return pc_setglobalreg_str( sd, num, value );
	}
}

bool set_reg_num( struct script_state* st, struct map_session_data* sd, int64 num, const char* name, int64 value, struct reg_db *ref ){
	char prefix = name[0];
	size_t vlen = 0;

	if( !script_check_RegistryVariableLength( 0, name, &vlen ) ){
		ShowError( "set_reg: Variable name length is too long (aid: %d, cid: %d): '%s' sz=%" PRIuPTR "\n", sd ? sd->status.account_id : -1, sd ? sd->status.char_id : -1, name, vlen );
		return false;
	}

	if( is_string_variable( name ) ){
		// string variable
		return false;
	}

	if( str_data[script_getvarid(num)].type == C_PARAM ){
		if( pc_setparam( sd, str_data[script_getvarid(num)].val, value ) == 0 ){
			if( st != NULL ) {
				ShowError( "script_set_reg: failed to set param '%s' to %" PRId64 ".\n", name, value );
				script_reportsrc( st );
				st->state = END;
			}

			return false;
		}

		return true;
	}

	switch( prefix ){
		case '@':
			pc_setreg( sd, num, value );
			return true;
		case '$':
			return mapreg_setreg( num, value );
		case '#':
			return ( name[1] == '#' ) ? pc_setaccountreg2(sd, num, value) :	pc_setaccountreg( sd, num, value );
		case '.': {
				struct reg_db *n = ( ref ) ? ref : ( name[1] == '@' ) ? &st->stack->scope : &st->script->local;

				if( n ){
					if( value != 0 ){
						i64db_i64put( n->vars, num, value );

						if( script_getvaridx( num ) ){
							script_array_update( n, num, false );
						}
					}else{
						i64db_remove( n->vars, num );

						if( script_getvaridx( num ) ){
							script_array_update( n, num, true );
						}
					}
				}
			}
			return true;
		case '\'': {
				struct reg_db *src = nullptr;

				if( ref ){
					src = ref;
				}else{
					std::shared_ptr<s_instance_data> idata = util::umap_find(instances, script_instancegetid(st));

					if (idata)
						src = &idata->regs;
				}

				if( src ){
					bool empty;

					if( value != 0 ){
						i64db_i64put( src->vars, num, value );
						empty = false;
					}else{
						i64db_remove( src->vars, num );
						empty = true;
					}

					if( script_getvaridx( num ) != 0 ){
						script_array_update( src, num, empty );
					}
				}else{
					ShowError( "script_set_reg: cannot write instance variable '%s', NPC not in a instance!\n", name );
					script_reportsrc( st );
				}

				return true;
			}
		default:
			return pc_setglobalreg( sd, num, value );
	}
}

bool set_var_str( struct map_session_data* sd, const char* name, const char* val ){
	return set_reg_str( NULL, sd, reference_uid( add_str( name ), 0 ), name, val, NULL );
}

bool clear_reg( struct script_state* st, struct map_session_data* sd, int64 num, const char* name, struct reg_db *ref ){
	if( is_string_variable( name ) ){
		return set_reg_str( st, sd, num, name, "", ref );
	}else{
		return set_reg_num( st, sd, num, name, 0, ref );
	}
}

void setd_sub_num( struct script_state* st, struct map_session_data* sd, const char* varname, int elem, int64 value, struct reg_db* ref ){
	set_reg_num( st, sd, reference_uid( add_str( varname ), elem ), varname, value, ref );
}

void setd_sub_str( struct script_state* st, struct map_session_data* sd, const char* varname, int elem, const char* value, struct reg_db* ref ){
	set_reg_str( st, sd, reference_uid( add_str( varname ), elem ), varname, value, ref );
}

/**
 * Converts the data to a string
 * @param st
 * @param data
 * @param sd
 */
const char* conv_str_(struct script_state* st, struct script_data* data, struct map_session_data *sd)
{
	char* p;

	get_val_(st, data, sd);
	if( data_isstring(data) )
	{// nothing to convert
	}
	else if( data_isint(data) )
	{// int -> string
		CREATE(p, char, ITEM_NAME_LENGTH);
		snprintf(p, ITEM_NAME_LENGTH, "%" PRId64 "", data->u.num);
		p[ITEM_NAME_LENGTH-1] = '\0';
		data->type = C_STR;
		data->u.str = p;
	}
	else if( data_isreference(data) )
	{// reference -> string
		//##TODO when does this happen (check get_val) [FlavioJS] -- at getd!!
		data->type = C_CONSTSTR;
		data->u.str = reference_getname(data);
	}
	else
	{// unsupported data type
		ShowError("script:conv_str: cannot convert to string, defaulting to \"\"\n");
		script_reportdata(data);
		script_reportsrc(st);
		data->type = C_CONSTSTR;
		data->u.str = const_cast<char *>("");
	}
	return data->u.str;
}

const char* conv_str(struct script_state* st, struct script_data* data)
{
	return conv_str_(st, data, NULL);
}

/**
 * Converts the data to an int
 * @param st
 * @param data
 * @param sd
 */
int64 conv_num_(struct script_state* st, struct script_data* data, struct map_session_data *sd)
{
	get_val_(st, data, sd);
	if( data_isint(data) )
	{// nothing to convert
	}
	else if( data_isstring(data) )
	{// string -> int
		// the result does not overflow or underflow, it is capped instead
		// ex: 999999999999 is capped to INT_MAX (2147483647)
		char* p = data->u.str;
		int64 num;
		bool error;

		errno = 0;
		num = strtoll(data->u.str, NULL, 10);// change radix to 0 to support octal numbers "o377" and hex numbers "0xFF"
		error = (errno == ERANGE);

		if( num < SCRIPT_INT_MIN || ( error && num == INT64_MIN ) ){
			num = SCRIPT_INT_MIN;
			ShowError( "script:conv_num: underflow detected, capping value to %" PRId64 "", SCRIPT_INT_MIN );
			script_reportdata(data);
			script_reportsrc(st);
		}else if( num > SCRIPT_INT_MAX || ( error && num == INT64_MAX ) ){
			num = SCRIPT_INT_MAX;
			ShowError( "script:conv_num: overflow detected, capping value to %" PRId64 "", SCRIPT_INT_MAX );
			script_reportdata(data);
			script_reportsrc(st);
		}

		if( data->type == C_STR )
			aFree(p);
		data->type = C_INT;
		data->u.num = num;
	}
#if 0
	// FIXME this function is being used to retrieve the position of labels and
	// probably other stuff [FlavioJS]
	else
	{// unsupported data type
		ShowError("script:conv_num: cannot convert to number, defaulting to 0\n");
		script_reportdata(data);
		script_reportsrc(st);
		data->type = C_INT;
		data->u.num = 0;
	}
#endif
	return data->u.num;
}

int64 conv_num64(struct script_state* st, struct script_data* data)
{
	return conv_num_(st, data, NULL);
}

int conv_num(struct script_state* st, struct script_data* data)
{
	return static_cast<int>(conv_num_(st, data, NULL));
}

//
// Stack operations
//

/// Increases the size of the stack
void stack_expand(struct script_stack* stack)
{
	stack->sp_max += 64;
	stack->stack_data = (struct script_data*)aRealloc(stack->stack_data,
			stack->sp_max * sizeof(stack->stack_data[0]) );
	memset(stack->stack_data + (stack->sp_max - 64), 0,
			64 * sizeof(stack->stack_data[0]) );
}

/// Pushes a value into the stack
#define push_val(stack,type,val) push_val2(stack, type, val, NULL)

/// Pushes a value into the stack (with reference)
struct script_data* push_val2(struct script_stack* stack, enum c_op type, int64 val, struct reg_db *ref)
{
	if( stack->sp >= stack->sp_max )
		stack_expand(stack);
	stack->stack_data[stack->sp].type  = type;
	stack->stack_data[stack->sp].u.num = val;
	stack->stack_data[stack->sp].ref   = ref;
	stack->sp++;
	return &stack->stack_data[stack->sp-1];
}

/// Pushes a string into the stack
struct script_data* push_str(struct script_stack* stack, enum c_op type, char* str)
{
	if( stack->sp >= stack->sp_max )
		stack_expand(stack);
	stack->stack_data[stack->sp].type  = type;
	stack->stack_data[stack->sp].u.str = str;
	stack->stack_data[stack->sp].ref   = NULL;
	stack->sp++;
	return &stack->stack_data[stack->sp-1];
}

/// Pushes a retinfo into the stack
struct script_data* push_retinfo(struct script_stack* stack, struct script_retinfo* ri, struct reg_db *ref)
{
	if( stack->sp >= stack->sp_max )
		stack_expand(stack);
	stack->stack_data[stack->sp].type = C_RETINFO;
	stack->stack_data[stack->sp].u.ri = ri;
	stack->stack_data[stack->sp].ref  = ref;
	stack->sp++;
	return &stack->stack_data[stack->sp-1];
}

/// Pushes a copy of the target position into the stack
struct script_data* push_copy(struct script_stack* stack, int pos)
{
	switch( stack->stack_data[pos].type ) {
		case C_CONSTSTR:
			return push_str(stack, C_CONSTSTR, stack->stack_data[pos].u.str);
			break;
		case C_STR:
			return push_str(stack, C_STR, aStrdup(stack->stack_data[pos].u.str));
			break;
		case C_RETINFO:
			ShowFatalError("script:push_copy: can't create copies of C_RETINFO. Exiting...\n");
			exit(1);
			break;
		default:
			return push_val2(
				stack,stack->stack_data[pos].type,
				stack->stack_data[pos].u.num,
				stack->stack_data[pos].ref
			);
			break;
	}
}

/// Removes the values in indexes [start,end] from the stack.
/// Adjusts all stack pointers.
void pop_stack(struct script_state* st, int start, int end)
{
#ifdef Pandas_Crashfix_FunctionParams_Verify
	if (!st || !st->stack) return;
#endif // Pandas_Crashfix_FunctionParams_Verify

	struct script_stack* stack = st->stack;
	struct script_data* data;
	int i;

	if( start < 0 )
		start = 0;
	if( end > stack->sp )
		end = stack->sp;
	if( start >= end )
		return;// nothing to pop

	// free stack elements
	for( i = start; i < end; i++ )
	{
		data = &stack->stack_data[i];
		if( data->type == C_STR )
			aFree(data->u.str);
		if( data->type == C_RETINFO ) {
			struct script_retinfo* ri = data->u.ri;

			if (ri->scope.vars) {
				script_free_vars(ri->scope.vars);
				ri->scope.vars = NULL;
			}
			if (ri->scope.arrays) {
				ri->scope.arrays->destroy(ri->scope.arrays, script_free_array_db);
				ri->scope.arrays = NULL;
			}
			if( data->ref )
				aFree(data->ref);
			aFree(ri);
		}
		data->type = C_NOP;
	}

	// move the rest of the elements
	if( stack->sp > end )
	{
		memmove(&stack->stack_data[start], &stack->stack_data[end], sizeof(stack->stack_data[0])*(stack->sp - end));
		for( i = start + stack->sp - end; i < stack->sp; ++i )
			stack->stack_data[i].type = C_NOP;
	}

	// adjust stack pointers
	if( st->start > end ){
		st->start -= end - start;
	}else if( st->start > start ){
		st->start = start;
	}
	
	if( st->end > end ){
		st->end -= end - start;
	}else if( st->end > start ){
		st->end = start;
	}
	
	if( stack->defsp > end ){
		stack->defsp -= end - start;
	}else if( stack->defsp > start ){
		stack->defsp = start;
	}
	
	stack->sp -= end - start;
}

///
///
///

/*==========================================
 * Release script dependent variable, dependent variable of function
 *------------------------------------------*/
void script_free_vars(struct DBMap* storage)
{
	if( storage ) {
		// destroy the storage construct containing the variables
		db_destroy(storage);
	}
}

void script_free_code(struct script_code* code)
{
	nullpo_retv(code);

	if (code->instances)
		script_stop_scriptinstances(code);
	script_free_vars(code->local.vars);
	if (code->local.arrays)
		code->local.arrays->destroy(code->local.arrays, script_free_array_db);
	aFree(code->script_buf);

#ifdef Pandas_Crashfix_ScriptFreeCode_SetPointerNull
	code->local.vars = NULL;
	code->local.arrays = NULL;
	code->script_buf = NULL;
#endif // Pandas_Crashfix_ScriptFreeCode_SetPointerNull

	aFree(code);
}

/// Creates a new script state.
///
/// @param script Script code
/// @param pos Position in the code
/// @param rid Who is running the script (attached player)
/// @param oid Where the code is being run (npc 'object')
/// @return Script state
struct script_state* script_alloc_state(struct script_code* rootscript, int pos, int rid, int oid)
{
	struct script_state* st;

	st = ers_alloc(st_ers, struct script_state);
	st->stack = ers_alloc(stack_ers, struct script_stack);
	st->stack->sp = 0;
	st->stack->sp_max = 64;
	CREATE(st->stack->stack_data, struct script_data, st->stack->sp_max);
	st->stack->defsp = st->stack->sp;
	st->stack->scope.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
	st->stack->scope.arrays = NULL;
	st->state = RUN;
	st->script = rootscript;
	st->pos = pos;
	st->rid = rid;
	st->oid = oid;
	st->sleep.timer = INVALID_TIMER;
	st->npc_item_flag = battle_config.item_enabled_npc;
	st->asyncSleep = false;

#ifdef Pandas_ScriptCommand_UnlockCmd
	// 确保创建 script_state 的时候 unlockcmd 的值为 0
	st->unlockcmd = 0;
#endif // Pandas_ScriptCommand_UnlockCmd

#ifdef Pandas_ScriptCommand_GetInventoryList
	st->waiting_premium_storage = 0;
	st->waiting_guild_storage = 0;
#endif // Pandas_ScriptCommand_GetInventoryList

	if( st->script->instances != USHRT_MAX )
		st->script->instances++;
	else {
		struct npc_data *nd = map_id2nd(oid);

		ShowError("Over 65k instances of '%s' script are being run!\n",nd ? nd->name : "unknown");
	}

	if (!st->script->local.vars)
		st->script->local.vars = i64db_alloc(DB_OPT_RELEASE_DATA);

	st->id = next_id++;
	active_scripts++;

	idb_put(st_db, st->id, st);

	return st;
}

/// Frees a script state.
///
/// @param st Script state
void script_free_state(struct script_state* st)
{
	if (idb_exists(st_db, st->id)) {
		struct map_session_data *sd = st->rid ? map_id2sd(st->rid) : NULL;

#ifndef Pandas_ScriptEngine_MutliStackBackup
		if (st->bk_st) // backup was not restored
			ShowDebug("script_free_state: Previous script state lost (rid=%d, oid=%d, state=%d, bk_npcid=%d).\n", st->bk_st->rid, st->bk_st->oid, st->bk_st->state, st->bk_npcid);
#endif // Pandas_ScriptEngine_MutliStackBackup

		if (sd && sd->st == st) { // Current script is aborted.
			if(sd->state.using_fake_npc) {
				clif_clearunit_single(sd->npc_id, CLR_OUTSIGHT, sd->fd);
				sd->state.using_fake_npc = 0;
			}

#ifdef Pandas_Fix_Progressbar_Abort_Stuck
			// 若当前角色的 progressbar 信息里记录的来源 NPC 编号等于即将销毁的 st 的 NPC 编号,
			// 那么进行对应的清理操作避免后续角色出现卡住无法移动的情况
			if (sd->progressbar.npc_id == sd->st->oid && sd->status.account_id == sd->st->rid) {
				clif_progressbar_abort(sd);

				sd->progressbar.npc_id = 0;
				sd->progressbar.timeout = 0;

				if (battle_config.idletime_option & IDLE_NPC_PROGRESS) {
					sd->idletime = last_tick;
				}
			}
#endif // Pandas_Fix_Progressbar_Abort_Stuck

			sd->st = NULL;
			sd->npc_id = 0;
		}

		if (st->sleep.timer != INVALID_TIMER)
			delete_timer(st->sleep.timer, run_script_timer);
		if (st->stack) {
			script_free_vars(st->stack->scope.vars);
			if (st->stack->scope.arrays)
				st->stack->scope.arrays->destroy(st->stack->scope.arrays, script_free_array_db);
			pop_stack(st, 0, st->stack->sp);
			aFree(st->stack->stack_data);
			ers_free(stack_ers, st->stack);
			st->stack = NULL;
		}
		if (st->script && st->script->instances != USHRT_MAX && --st->script->instances == 0) {
			if (st->script->local.vars && !db_size(st->script->local.vars)) {
				script_free_vars(st->script->local.vars);
				st->script->local.vars = NULL;
			}
			if (st->script->local.arrays && !db_size(st->script->local.arrays)) {
				st->script->local.arrays->destroy(st->script->local.arrays, script_free_array_db);
				st->script->local.arrays = NULL;
			}
		}
		st->pos = -1;

		idb_remove(st_db, st->id);
		ers_free(st_ers, st);
		if (--active_scripts == 0)
			next_id = 0;
	}
}

//
// Main execution unit
//
/*==========================================
 * Read command
 *------------------------------------------*/
c_op get_com(unsigned char *script,int *pos)
{
	int i = 0, j = 0;

	if(script[*pos]>=0x80){
		return C_INT;
	}
	while(script[*pos]>=0x40){
		i=script[(*pos)++]<<j;
		j+=6;
	}
	return (c_op)(i+(script[(*pos)++]<<j));
}

/*==========================================
 *  Income figures
 *------------------------------------------*/
int64 get_num(unsigned char *script,int *pos)
{
	int i, end;
	int64 value;

	// Find the end of the data
	for( i = 0; script[*pos+i] >= 0xc0; i++ ) ; // do nothing just find it

	end = i; // store the end for later
	value = 0; // initialize empty value

	// If more than one byte of data exists
	for( ; i > 0; i-- ){
		value |= (int64)( script[*pos+i] & (int64)0x3f );
		value <<= 6;
	}

	// Put our last piece of data into the return value
	value |= (script[*pos+i] & (int64)0x3f);

	*pos += end + 1; // one byte after the last byte of data

	return value;
}

/// Ternary operators
/// test ? if_true : if_false
void op_3(struct script_state* st, int op)
{
	struct script_data* data;
	int flag = 0;

	data = script_getdatatop(st, -3);
	get_val(st, data);

	if( data_isstring(data) )
		flag = data->u.str[0];// "" -> false
	else if( data_isint(data) )
		flag = data->u.num == 0 ? 0 : 1;// 0 -> false
	else
	{
		ShowError("script:op_3: invalid data for the ternary operator test\n");
		script_reportdata(data);
		script_reportsrc(st);
		script_removetop(st, -3, 0);
		script_pushnil(st);
		return;
	}
	if( flag )
		script_pushcopytop(st, -2);
	else
		script_pushcopytop(st, -1);
	script_removetop(st, -4, -1);
}

/// Binary string operators
/// s1 EQ s2 -> i
/// s1 NE s2 -> i
/// s1 GT s2 -> i
/// s1 GE s2 -> i
/// s1 LT s2 -> i
/// s1 LE s2 -> i
/// s1 ADD s2 -> s
void op_2str(struct script_state* st, int op, const char* s1, const char* s2)
{
	int a = 0;

	switch(op){
		case C_EQ: a = (strcmp(s1,s2) == 0); break;
		case C_NE: a = (strcmp(s1,s2) != 0); break;
		case C_GT: a = (strcmp(s1,s2) >  0); break;
		case C_GE: a = (strcmp(s1,s2) >= 0); break;
		case C_LT: a = (strcmp(s1,s2) <  0); break;
		case C_LE: a = (strcmp(s1,s2) <= 0); break;
		case C_ADD:
			{
				char* buf = (char *)aMalloc((strlen(s1)+strlen(s2)+1)*sizeof(char));
				strcpy(buf, s1);
				strcat(buf, s2);
				script_pushstr(st, buf);
				return;
			}
		default:
			ShowError("script:op2_str: unexpected string operator %s\n", script_op2name(op));
			script_reportsrc(st);
			script_pushnil(st);
			st->state = END;
			return;
	}

	script_pushint(st,a);
}

/// Binary number operators
/// i OP i -> i
void op_2num( struct script_state* st, int op, int64 i1, int64 i2 ){
	int64 ret;

	switch( op ) {
		case C_AND:  ret = i1 & i2;		break;
		case C_OR:   ret = i1 | i2;		break;
		case C_XOR:  ret = i1 ^ i2;		break;
		case C_LAND: ret = (i1 && i2);	break;
		case C_LOR:  ret = (i1 || i2);	break;
		case C_EQ:   ret = (i1 == i2);	break;
		case C_NE:   ret = (i1 != i2);	break;
		case C_GT:   ret = (i1 >  i2);	break;
		case C_GE:   ret = (i1 >= i2);	break;
		case C_LT:   ret = (i1 <  i2);	break;
		case C_LE:   ret = (i1 <= i2);	break;
		case C_R_SHIFT: ret = i1>>i2;	break;
		case C_L_SHIFT: ret = i1<<i2;	break;
		case C_DIV:
		case C_MOD:
			if( i2 == 0 )
			{
				ShowError("script:op_2num: division by zero detected op=%s i1=%" PRId64 " i2=%" PRId64 "\n", script_op2name(op), i1, i2);
				script_reportsrc(st);
				script_pushnil(st);
				st->state = END;
				return;
			}
			else if( op == C_DIV )
				ret = i1 / i2;
			else//if( op == C_MOD )
				ret = i1 % i2;
			break;
		default:
			bool overflow;

			// operators that can overflow/underflow
			switch( op ) {
				case C_ADD: overflow = util::safe_addition( i1, i2, ret ); break;
				case C_SUB: overflow = util::safe_substraction( i1, i2, ret ); break;
				case C_MUL: overflow = util::safe_multiplication( i1, i2, ret ); break;
				default:
					ShowError("script:op_2num: unexpected number operator %s i1=%" PRId64 " i2=%" PRId64 "\n", script_op2name(op), i1, i2);
					script_reportsrc(st);
					script_pushnil(st);
					return;
			}

			if( overflow ){
				ShowWarning("script:op_2num: overflow detected op=%s i1=%" PRId64 " i2=%" PRId64 "\n", script_op2name(op), i1, i2);
				script_reportsrc(st);
				script_pushnil(st);
				st->state = END;
				return;
			}
	}
	script_pushint(st, ret);
}

/// Binary operators
void op_2(struct script_state *st, int op)
{
	struct script_data* left, leftref;
	struct script_data* right;

	leftref.type = C_NOP;

	left = script_getdatatop(st, -2);
	right = script_getdatatop(st, -1);

	if (st->op2ref) {
		if (data_isreference(left)) {
			leftref = *left;
		}

		st->op2ref = 0;
	}

	get_val(st, left);
	get_val(st, right);

	// automatic conversions
	switch( op )
	{
		case C_ADD:
			if( data_isint(left) && data_isstring(right) )
			{// convert int-string to string-string
				conv_str(st, left);
			}
			else if( data_isstring(left) && data_isint(right) )
			{// convert string-int to string-string
				conv_str(st, right);
			}
			break;
	}

	if( data_isstring(left) && data_isstring(right) )
	{// ss => op_2str
		op_2str(st, op, left->u.str, right->u.str);
		script_removetop(st, leftref.type == C_NOP ? -3 : -2, -1);// pop the two values before the top one

		if (leftref.type != C_NOP)
		{
			if (left->type == C_STR) // don't free C_CONSTSTR
				aFree(left->u.str);
			*left = leftref;
		}
	}
	else if( data_isint(left) && data_isint(right) )
	{// ii => op_2num
		int64 i1 = left->u.num;
		int64 i2 = right->u.num;

		script_removetop(st, leftref.type == C_NOP ? -2 : -1, 0);
		op_2num(st, op, i1, i2);

		if (leftref.type != C_NOP)
			*left = leftref;
	}
	else
	{// invalid argument
		ShowError("script:op_2: invalid data for operator %s\n", script_op2name(op));
		script_reportdata(left);
		script_reportdata(right);
		script_reportsrc(st);
		script_removetop(st, -2, 0);
		script_pushnil(st);
		st->state = END;
	}
}

/// Unary operators
/// NEG i -> i
/// NOT i -> i
/// LNOT i -> i
void op_1(struct script_state* st, int op)
{
	struct script_data* data;

	data = script_getdatatop(st, -1);
	get_val(st, data);

	if( !data_isint(data) )
	{// not a number
		ShowError("script:op_1: argument is not a number (op=%s)\n", script_op2name(op));
		script_reportdata(data);
		script_reportsrc(st);
		script_pushnil(st);
		st->state = END;
		return;
	}

	int64 i1 = data->u.num;
	script_removetop(st, -1, 0);
	switch( op )
	{
		case C_NEG: i1 = -i1; break;
		case C_NOT: i1 = ~i1; break;
		case C_LNOT: i1 = !i1; break;
		default:
			ShowError("script:op_1: unexpected operator %s i1=%" PRId64 "\n", script_op2name(op), i1);
			script_reportsrc(st);
			script_pushnil(st);
			st->state = END;
			return;
	}
	script_pushint(st, i1);
}


/// Checks the type of all arguments passed to a built-in function.
///
/// @param st Script state whose stack arguments should be inspected.
/// @param func Built-in function for which the arguments are intended.
static void script_check_buildin_argtype(struct script_state* st, int func)
{
	int idx, invalid = 0;

	for( idx = 2; script_hasdata(st, idx); idx++ )
	{
		struct script_data* data = script_getdata(st, idx);
		script_function* sf = &buildin_func[str_data[func].val];
		char type = sf->arg[idx-2];

		if( type == '?' || type == '*' )
		{// optional argument or unknown number of optional parameters ( no types are after this )
			break;
		}
		else if( type == 0 )
		{// more arguments than necessary ( should not happen, as it is checked before )
			ShowWarning("Found more arguments than necessary. unexpected arg type %s\n",script_op2name(data->type));
			invalid++;
			break;
		}
		else
		{
			const char* name = NULL;

			if( data_isreference(data) )
			{// get name for variables to determine the type they refer to
				name = reference_getname(data);
			}

			switch( type )
			{
				case 'v':
					if( !data_isstring(data) && !data_isint(data) && !data_isreference(data) )
					{// variant
						ShowWarning("Unexpected type for argument %d. Expected string, number or variable.\n", idx-1);
						script_reportdata(data);
						invalid++;
					}
					break;
				case 's':
					if( !data_isstring(data) && !( data_isreference(data) && is_string_variable(name) ) )
					{// string
						ShowWarning("Unexpected type for argument %d. Expected string.\n", idx-1);
						script_reportdata(data);
						invalid++;
					}
					break;
				case 'i':
					if( !data_isint(data) && !( data_isreference(data) && ( reference_toparam(data) || reference_toconstant(data) || !is_string_variable(name) ) ) )
					{// int ( params and constants are always int )
						ShowWarning("Unexpected type for argument %d. Expected number.\n", idx-1);
						script_reportdata(data);
						invalid++;
					}
					break;
				case 'r':
					if( !data_isreference(data) )
					{// variables
						ShowWarning("Unexpected type for argument %d. Expected variable, got %s.\n", idx-1,script_op2name(data->type));
						script_reportdata(data);
						invalid++;
					}
					break;
				case 'l':
					if( !data_islabel(data) && !data_isfunclabel(data) )
					{// label
						ShowWarning("Unexpected type for argument %d. Expected label, got %s\n", idx-1,script_op2name(data->type));
						script_reportdata(data);
						invalid++;
					}
					break;
			}
		}
	}

	if(invalid)
	{
		ShowDebug("Function: %s\n", get_str(func));
		script_reportsrc(st);
	}
}


/// Executes a buildin command.
/// Stack: C_NAME(<command>) C_ARG <arg0> <arg1> ... <argN>
int run_func(struct script_state *st)
{
	struct script_data* data;
	int i,start_sp,end_sp,func;

	end_sp = st->stack->sp;// position after the last argument
	for( i = end_sp-1; i > 0 ; --i )
		if( st->stack->stack_data[i].type == C_ARG )
			break;
	if( i == 0 ) {
		ShowError("script:run_func: C_ARG not found. please report this!!!\n");
		st->state = END;
		script_reportsrc(st);
		return 1;
	}
	start_sp = i-1;// C_NAME of the command
	st->start = start_sp;
	st->end = end_sp;

	data = &st->stack->stack_data[st->start];
	if( data->type == C_NAME && str_data[data->u.num].type == C_FUNC ) {
		func = (int)data->u.num;
		st->funcname = reference_getname(data);
	} else {
		ShowError("script:run_func: not a buildin command.\n");
		script_reportdata(data);
		script_reportsrc(st);
		st->state = END;
		return 1;
	}

	if( script_config.warn_func_mismatch_argtypes ) {
		script_check_buildin_argtype(st, func);
	}

	if(str_data[func].func) {
#if defined(SCRIPT_COMMAND_DEPRECATION)
		if( buildin_func[str_data[func].val].deprecated ){
			ShowWarning( "Usage of deprecated script function '%s'.\n", get_str(func) );
			ShowWarning( "This function was deprecated on '%s' and could become unavailable anytime soon.\n", buildin_func[str_data[func].val].deprecated );
			script_reportsrc(st);
		}
#endif

#ifdef Pandas_ScriptEngine_Express
		if (st && st->rid && !st->unlockcmd) {
			struct map_session_data *sd = map_id2sd(st->rid);
			if (sd && npc_event_is_realtime(sd->pandas.workinevent)) {
				// 以下为实时事件和过滤器事件中禁止使用的脚本指令, 需要定期更新 [Sola丶小克]
				static std::vector<std::string> blockcmd = {
					"mes", "next", "close", "close2", "menu", "select", "prompt", "input",
					"openstorage", "guildopenstorage", "produce", "cooking", "birthpet",
					"callshop", "sleep", "sleep2", "openmail", "openauction", "progressbar",
					"buyingstore", "makerune", "opendressroom", "openstorage2"
				};

				std::vector<std::string>::iterator iter;
				std::string funcname = std::string(get_str(func));
				std::transform(
					funcname.begin(), funcname.end(), funcname.begin(),
					static_cast<int(*)(int)>(std::tolower)
				);
				iter = std::find(blockcmd.begin(), blockcmd.end(), funcname);

				if (iter != blockcmd.end()) {
					ShowWarning("Please don't use '%s' command in '%s' event.\n", funcname.c_str(), npc_get_script_event_name(sd->pandas.workinevent));
					ShowWarning("If you insist and know what you are doing, you can use the 'unlockcmd' command to lift the restriction.\n");
					script_reportsrc(st);
					st->state = END;
					return 1;
				}
			}
		}
#endif // Pandas_ScriptEngine_Express

		if (str_data[func].func(st) == SCRIPT_CMD_FAILURE) //Report error
			script_reportsrc(st);
	} else {
		ShowError("script:run_func: '%s' (id=%d type=%s) has no C function. please report this!!!\n", get_str(func), func, script_op2name(str_data[func].type));
		script_reportsrc(st);
		st->state = END;
	}

	// Stack's datum are used when re-running functions [Eoe]
	if( st->state == RERUNLINE )
		return 0;

	pop_stack(st, st->start, st->end);
	if( st->state == RETFUNC ) {// return from a user-defined function
		struct script_retinfo* ri;
		int olddefsp = st->stack->defsp;
		int nargs;

		pop_stack(st, st->stack->defsp, st->start);// pop distractions from the stack
		if( st->stack->defsp < 1 || st->stack->stack_data[st->stack->defsp-1].type != C_RETINFO )
		{
			ShowWarning("script:run_func: return without callfunc or callsub!\n");
			script_reportsrc(st);
			st->state = END;
			return 1;
		}
		script_free_vars(st->stack->scope.vars);
		st->stack->scope.arrays->destroy(st->stack->scope.arrays, script_free_array_db);

		ri = st->stack->stack_data[st->stack->defsp-1].u.ri;
		nargs = ri->nargs;
		st->pos = ri->pos;
		st->script = ri->script;
		st->stack->scope.vars = ri->scope.vars;
		st->stack->scope.arrays = ri->scope.arrays;
		st->stack->defsp = ri->defsp;
		memset(ri, 0, sizeof(struct script_retinfo));

		pop_stack(st, olddefsp-nargs-1, olddefsp);// pop arguments and retinfo

		st->state = GOTO;
	}

	return 0;
}

/*==========================================
 * script execution
 *------------------------------------------*/
void run_script(struct script_code *rootscript, int pos, int rid, int oid)
{
	struct script_state *st;

	if( rootscript == NULL || pos < 0 )
		return;

	// TODO In jAthena, this function can take over the pending script in the player. [FlavioJS]
	//      It is unclear how that can be triggered, so it needs the be traced/checked in more detail.
	// NOTE At the time of this change, this function wasn't capable of taking over the script state because st->scriptroot was never set.
	st = script_alloc_state(rootscript, pos, rid, oid);
	run_script_main(st);
}

/**
 * Free all related script code
 * @param code: Script code to free
 */
void script_stop_scriptinstances(struct script_code *code) {
	DBIterator *iter;
	struct script_state* st;

	if( !active_scripts )
		return; // Don't even bother.

	iter = db_iterator(st_db);

	for( st = static_cast<script_state *>(dbi_first(iter)); dbi_exists(iter); st = static_cast<script_state *>(dbi_next(iter)) ) {
		if( st->script == code )
			script_free_state(st);
	}

	dbi_destroy(iter);
}

/*==========================================
 * Timer function for sleep
 *------------------------------------------*/
TIMER_FUNC(run_script_timer){
	struct script_state *st = (struct script_state *)data;
	struct linkdb_node *node = (struct linkdb_node *)sleep_db;

	// If it was a player before going to sleep and there is still a unit attached to the script
	if( id != 0 && st->rid ){
		struct map_session_data *sd = map_id2sd(st->rid);

		// Attached player is offline(logout) or another unit type(should not happen)
		if( !sd ){
			st->rid = 0;
			st->state = END;
		// Character mismatch. Cancel execution.
		}else if( sd->status.char_id != id ){
			ShowWarning( "Script sleep timer detected a character mismatch CID %d != %d\n", sd->status.char_id, id );
			script_reportsrc(st);
			st->rid = 0;
			st->state = END;
		}
	}

	while (node && st->sleep.timer != INVALID_TIMER) {
		if ((int)__64BPRTSIZE(node->key) == st->oid && ((struct script_state *)node->data)->sleep.timer == st->sleep.timer) {
			script_erase_sleepdb(node);
			st->sleep.timer = INVALID_TIMER;
			break;
		}
		node = node->next;
	}
	if(st->state != RERUNLINE)
		st->sleep.tick = 0;

#ifdef Pandas_Crashfix_Invaild_Script_Code
	if (st && (st->script == nullptr || st->script->script_buf == nullptr)) {
		ShowError("%s: The script that was resumed has been released, please report this to the developer (Trigger Point: %d).\n", __func__, 1);
		script_reportsrc(st);
		st->state = END;
		return 0;
	}
#endif // Pandas_Crashfix_Invaild_Script_Code

	run_script_main(st);
	return 0;
}

/**
 * Remove sleep timers from the NPC
 * @param id: NPC ID
 */
void script_stop_sleeptimers(int id) {
	for (;;) {
		struct script_state *st = (struct script_state *)linkdb_erase(&sleep_db, (void *)__64BPRTSIZE(id));

		if (!st)
			break; // No more sleep timers

		if (st->oid == id)
			script_free_state(st);
	}
}

/**
 * Delete the specified node from sleep_db
 * @param n: Linked list of sleep timers
 */
struct linkdb_node *script_erase_sleepdb(struct linkdb_node *n) {
	struct linkdb_node *retnode;

	if (!n)
		return NULL;

	if (!n->prev)
		sleep_db = n->next;
	else
		n->prev->next = n->next;

	if (n->next)
		n->next->prev = n->prev;

	retnode = n->next;
	aFree(n);

	return retnode;
}

#ifndef Pandas_ScriptEngine_MutliStackBackup

/// Detaches script state from possibly attached character and restores it's previous script if any.
///
/// @param st Script state to detach.
/// @param dequeue_event Whether to schedule any queued events, when there was no previous script.
static void script_detach_state(struct script_state* st, bool dequeue_event)
{
	struct map_session_data* sd;

	if(st->rid && (sd = map_id2sd(st->rid))!=NULL) {
		if( sd->state.using_fake_npc ){
			clif_clearunit_single( sd->npc_id, CLR_OUTSIGHT, sd->fd );
			sd->state.using_fake_npc = 0;
		}

		sd->st = st->bk_st;
		sd->npc_id = st->bk_npcid;
		sd->state.disable_atcommand_on_npc = 0;
		if(st->bk_st) {
			//Remove tag for removal.
			st->bk_st = NULL;
			st->bk_npcid = 0;
		} else if(dequeue_event) {

#ifdef SECURE_NPCTIMEOUT
			/**
			 * We're done with this NPC session, so we cancel the timer (if existent) and move on
			 **/
			if( sd->npc_idle_timer != INVALID_TIMER ) {
				delete_timer(sd->npc_idle_timer,npc_secure_timeout_timer);
				sd->npc_idle_timer = INVALID_TIMER;
			}
#endif
			npc_event_dequeue(sd);
		}
	}
	else if(st->bk_st)
	{// rid was set to 0, before detaching the script state
		ShowError("script_detach_state: Found previous script state without attached player (rid=%d, oid=%d, state=%d, bk_npcid=%d)\n", st->bk_st->rid, st->bk_st->oid, st->bk_st->state, st->bk_npcid);
		script_reportsrc(st->bk_st);

		script_free_state(st->bk_st);
		st->bk_st = NULL;
	}
}

#else // Pandas_ScriptEngine_MutliStackBackup

void script_detach_state(struct script_state* st, bool dequeue_event)
{
	struct map_session_data* sd;

	if (st->rid && (sd = map_id2sd(st->rid)) != NULL) {

		if (sd->state.using_fake_npc) {
			clif_clearunit_single(sd->npc_id, CLR_OUTSIGHT, sd->fd);
			sd->state.using_fake_npc = 0;
		}

		sd->st = nullptr;
		sd->npc_id = 0;
		sd->state.disable_atcommand_on_npc = 0;

		if (!sd->mbk_st.empty()) {
			struct mutli_state val = sd->mbk_st.top();
			sd->st = val.bk_st;
			sd->npc_id = val.bk_npcid;
			sd->mbk_st.pop();
		}

		if (!sd->st && dequeue_event) {
#ifdef SECURE_NPCTIMEOUT
			/**
			 * We're done with this NPC session, so we cancel the timer (if existent) and move on
			 **/
			if (sd->npc_idle_timer != INVALID_TIMER) {
				delete_timer(sd->npc_idle_timer, npc_secure_timeout_timer);
				sd->npc_idle_timer = INVALID_TIMER;
			}
#endif
			npc_event_dequeue(sd);
		}
	}
}

#endif // Pandas_ScriptEngine_MutliStackBackup

/// Attaches script state to possibly attached character and backups it's previous script, if any.
///
/// @param st Script state to attach.
void script_attach_state(struct script_state* st){
	struct map_session_data* sd;

	if(st->rid && (sd = map_id2sd(st->rid))!=NULL)
	{
		if(st!=sd->st)
		{
#ifndef Pandas_ScriptEngine_MutliStackBackup
			if(st->bk_st)
			{// there is already a backup
				ShowDebug("script_attach_state: Previous script state lost (rid=%d, oid=%d, state=%d, bk_npcid=%d).\n", st->bk_st->rid, st->bk_st->oid, st->bk_st->state, st->bk_npcid);
			}
			st->bk_st = sd->st;
			st->bk_npcid = sd->npc_id;
#else
			struct mutli_state mbk_st = { 0 };
			mbk_st.bk_st = sd->st;
			mbk_st.bk_npcid = sd->npc_id;
			sd->mbk_st.push(mbk_st);
#endif // Pandas_ScriptEngine_MutliStackBackup
		}
		sd->st = st;
		sd->npc_id = st->oid;
		sd->npc_item_flag = st->npc_item_flag; // load default.
		sd->state.disable_atcommand_on_npc = battle_config.atcommand_disable_npc && (!pc_has_permission(sd, PC_PERM_ENABLE_COMMAND));
#ifdef SECURE_NPCTIMEOUT
		if( sd->npc_idle_timer == INVALID_TIMER && !sd->state.ignoretimeout )
			sd->npc_idle_timer = add_timer(gettick() + (SECURE_NPCTIMEOUT_INTERVAL*1000),npc_secure_timeout_timer,sd->bl.id,0);
		sd->npc_idle_tick = gettick();
#endif
	}
}

/*==========================================
 * The main part of the script execution
 *------------------------------------------*/
void run_script_main(struct script_state *st)
{
#ifdef Pandas_Crashfix_Prevent_NullPointer
	nullpo_retv(st);
#endif // Pandas_Crashfix_Prevent_NullPointer
	int cmdcount = script_config.check_cmdcount;
	int gotocount = script_config.check_gotocount;
	TBL_PC *sd;
	struct script_stack *stack = st->stack;

	script_attach_state(st);

#ifdef Pandas_Crashfix_Invaild_Script_Code
	if (st && (st->script == nullptr || st->script->script_buf == nullptr)) {
		ShowError("%s: The script that was resumed has been released, please report this to the developer (Trigger Point: %d).\n", __func__, 2);
		script_reportsrc(st);
		st->state = END;
		return;
	}
#endif // Pandas_Crashfix_Invaild_Script_Code

	if(st->state == RERUNLINE) {
		run_func(st);
		if(st->state == GOTO)
			st->state = RUN;
	} else if(st->state != END)
		st->state = RUN;

	while(st->state == RUN) {
#ifdef Pandas_Crashfix_Invaild_Script_Code
		if (st && (st->script == nullptr || st->script->script_buf == nullptr)) {
			ShowError("%s: The script that was resumed has been released, please report this to the developer (Trigger Point: %d).\n", __func__, 3);
			script_reportsrc(st);
			st->state = END;
			return;
		}
#endif // Pandas_Crashfix_Invaild_Script_Code

		enum c_op c = get_com(st->script->script_buf,&st->pos);
		switch(c){
		case C_EOL:
			if( stack->defsp > stack->sp )
				ShowError("script:run_script_main: unexpected stack position (defsp=%d sp=%d). please report this!!!\n", stack->defsp, stack->sp);
			else
				pop_stack(st, stack->defsp, stack->sp);// pop unused stack data. (unused return value)
			break;
		case C_INT:
			push_val(stack,C_INT,get_num(st->script->script_buf,&st->pos));
			break;
		case C_POS:
		case C_NAME:
			push_val(stack,c,GETVALUE(st->script->script_buf,st->pos));
			st->pos+=3;
			break;
		case C_ARG:
			push_val(stack,c,0);
			break;
		case C_STR:
			push_str(stack,C_CONSTSTR,(char*)(st->script->script_buf+st->pos));
			while(st->script->script_buf[st->pos++]);
			break;
		case C_FUNC:
			run_func(st);
			if(st->state==GOTO){
				st->state = RUN;
				if( !st->freeloop && gotocount>0 && (--gotocount)<=0 ){
					ShowError("script:run_script_main: infinity loop !\n");
					script_reportsrc(st);
					st->state=END;
				}
			}
			break;

		case C_REF:
			st->op2ref = 1;
			break;

		case C_NEG:
		case C_NOT:
		case C_LNOT:
			op_1(st ,c);
			break;

		case C_ADD:
		case C_SUB:
		case C_MUL:
		case C_DIV:
		case C_MOD:
		case C_EQ:
		case C_NE:
		case C_GT:
		case C_GE:
		case C_LT:
		case C_LE:
		case C_AND:
		case C_OR:
		case C_XOR:
		case C_LAND:
		case C_LOR:
		case C_R_SHIFT:
		case C_L_SHIFT:
			op_2(st, c);
			break;

		case C_OP3:
			op_3(st, c);
			break;

		case C_NOP:
			st->state=END;
			break;

		default:
			ShowError("script:run_script_main:unknown command : %d @ %d\n",c,st->pos);
			st->state=END;
			break;
		}
		if( !st->freeloop && cmdcount>0 && (--cmdcount)<=0 ){
			ShowError("script:run_script_main: infinity loop !\n");
			script_reportsrc(st);
			st->state=END;
		}
	}

	if(st->sleep.tick > 0) {
		//Restore previous script
		script_detach_state(st, false);
		//Delay execution
		sd = map_id2sd(st->rid); // Get sd since script might have attached someone while running. [Inkfish]
		st->sleep.charid = sd?sd->status.char_id:0;
		st->sleep.timer = add_timer(gettick() + st->sleep.tick, run_script_timer, st->sleep.charid, (intptr_t)st);
		linkdb_insert(&sleep_db, (void *)__64BPRTSIZE(st->oid), st);
	} else if (st->asyncSleep) {
		script_detach_state(st, false);
#ifndef Pandas_ScriptEngine_MutliStackBackup
	} else if(st->state != END && st->rid) {
		//Resume later (st is already attached to player).
		if(st->bk_st) {
			ShowWarning("Unable to restore stack! Double continuation!\n");
			//Report BOTH scripts to see if that can help somehow.
			ShowDebug("Previous script (lost):\n");
			script_reportsrc(st->bk_st);
			ShowDebug("Current script:\n");
			script_reportsrc(st);

			script_free_state(st->bk_st);
			st->bk_st = NULL;
		}
#else
	} else if(st->state != END && st->rid) {
		return;
#endif // Pandas_ScriptEngine_MutliStackBackup
	} else {
		if (st->stack && st->stack->defsp >= 1 && st->stack->stack_data[st->stack->defsp - 1].type == C_RETINFO) {
			for (int i = 0; i < st->stack->sp; i++) {
				if (st->stack->stack_data[i].type == C_RETINFO) { // Grab the first, aka the original
					st->script = st->stack->stack_data[i].u.ri->script;
					break;
				}
			}
		}

		//Dispose of script.
		if ((sd = map_id2sd(st->rid))!=NULL)
		{	//Restore previous stack and save char.
			if(sd->state.using_fake_npc){
				clif_clearunit_single(sd->npc_id, CLR_OUTSIGHT, sd->fd);
				sd->state.using_fake_npc = 0;
			}
			//Restore previous script if any.
			script_detach_state(st, true);
			if (sd->vars_dirty)
				intif_saveregistry(sd);
		}

#ifdef Pandas_ScriptCommand_SelfDeletion
		selfdeletion_exec_endtalk(st);
#endif // Pandas_ScriptCommand_SelfDeletion

		script_free_state(st);
	}
}

int script_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;


	fp=fopen(cfgName,"r");
	if(fp==NULL){
		ShowError("File not found: %s\n", cfgName);
		return 1;
	}
	while(fgets(line, sizeof(line), fp))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%1023[^:]: %1023[^\r\n]",w1,w2);
		if(i!=2)
			continue;

		if(strcmpi(w1,"warn_func_mismatch_paramnum")==0) {
			script_config.warn_func_mismatch_paramnum = config_switch(w2);
		}
		else if(strcmpi(w1,"check_cmdcount")==0) {
			script_config.check_cmdcount = config_switch(w2);
		}
		else if(strcmpi(w1,"check_gotocount")==0) {
			script_config.check_gotocount = config_switch(w2);
		}
		else if(strcmpi(w1,"input_min_value")==0) {
			script_config.input_min_value = config_switch(w2);
		}
		else if(strcmpi(w1,"input_max_value")==0) {
			script_config.input_max_value = config_switch(w2);
		}
		else if(strcmpi(w1,"warn_func_mismatch_argtypes")==0) {
			script_config.warn_func_mismatch_argtypes = config_switch(w2);
		}
		else if(strcmpi(w1,"import")==0){
			script_config_read(w2);
		}
		else {
			ShowWarning("Unknown setting '%s' in file %s\n", w1, cfgName);
		}
	}
	fclose(fp);

	return 0;
}

/**
 * @see DBApply
 */
static int db_script_free_code_sub(DBKey key, DBData *data, va_list ap)
{
	struct script_code *code = static_cast<script_code *>(db_data2ptr(data));
	if (code)
		script_free_code(code);
	return 0;
}

void script_run_autobonus(const char *autobonus, struct map_session_data *sd, unsigned int pos)
{
	struct script_code *script = (struct script_code *)strdb_get(autobonus_db, autobonus);

	if( script )
	{
		int j;
		ARR_FIND( 0, EQI_MAX, j, sd->equip_index[j] >= 0 && sd->inventory.u.items_inventory[sd->equip_index[j]].equip == pos );
		if( j < EQI_MAX ) {
			//Single item autobonus
			current_equip_item_index = sd->equip_index[j];
			current_equip_combo_pos = 0;
		} else {
			//Combo autobonus
			current_equip_item_index = -1;
			current_equip_combo_pos = pos;
		}
		run_script(script,0,sd->bl.id,0);
	}
}

void script_add_autobonus(const char *autobonus)
{
	if( strdb_get(autobonus_db, autobonus) == NULL )
	{
		struct script_code *script = parse_script(autobonus, "autobonus", 0, 0);

		if( script )
			strdb_put(autobonus_db, autobonus, script);
	}
}


/// resets a temporary character array variable to given value
void script_cleararray_pc( struct map_session_data* sd, const char* varname ){
	struct script_array *sa = NULL;
	struct reg_db *src = NULL;
	unsigned int i, *list = NULL, size = 0;
	int key;

	key = add_str(varname);

	if( !(src = script_array_src(NULL,sd,varname,NULL) ) )
		return;

	if( !(sa = static_cast<script_array *>(idb_get(src->arrays, key))) ) /* non-existent array, nothing to empty */
		return;

	size = sa->size;
	list = script_array_cpy_list(sa);

	for( i = 0; i < size; i++ ){
		clear_reg( NULL, sd, reference_uid( key, list[i] ), varname, NULL );
	}
}


/// sets a temporary character array variable element idx to given value
/// @param refcache Pointer to an int variable, which keeps a copy of the reference to varname and must be initialized to 0. Can be NULL if only one element is set.
void script_setarray_pc(struct map_session_data* sd, const char* varname, uint32 idx, int64 value, int* refcache)
{
	int key;

	if( idx >= SCRIPT_MAX_ARRAYSIZE ) {
		ShowError("script_setarray_pc: Variable '%s' has invalid index '%u' (char_id=%d).\n", varname, idx, sd->status.char_id);
		return;
	}

	key = ( refcache && refcache[0] ) ? refcache[0] : add_str(varname);

	set_reg_num( NULL, sd, reference_uid( key, idx ), varname, value, NULL );

	if( refcache ) {
		// save to avoid repeated script->add_str calls
		refcache[0] = key;
	}
}

/**
 * Clears persistent variables from memory
 **/
int script_reg_destroy(DBKey key, DBData *data, va_list ap)
{
	struct script_reg_state *src;

	if( data->type != DB_DATA_PTR ) // got no need for those!
		return 0;

	src = static_cast<script_reg_state *>(db_data2ptr(data));

	if( src->type ) {
		struct script_reg_str *p = (struct script_reg_str *)src;

		if( p->value )
			aFree(p->value);

		ers_free(str_reg_ers,p);
	} else {
		ers_free(num_reg_ers,(struct script_reg_num*)src);
	}

	return 0;
}

/**
 * Clears a single persistent variable
 **/
void script_reg_destroy_single(struct map_session_data *sd, int64 reg, struct script_reg_state *data)
{
	i64db_remove(sd->regs.vars, reg);

	if( data->type ) {
		struct script_reg_str *p = (struct script_reg_str*)data;

		if( p->value )
			aFree(p->value);

		ers_free(str_reg_ers,p);
	} else {
		ers_free(num_reg_ers,(struct script_reg_num*)data);
	}
}

unsigned int *script_array_cpy_list(struct script_array *sa)
{
	if( sa->size > generic_ui_array_size )
		script_generic_ui_array_expand(sa->size);
	memcpy(generic_ui_array, sa->members, sizeof(unsigned int)*sa->size);
	return generic_ui_array;
}

void script_generic_ui_array_expand (unsigned int plus)
{
	generic_ui_array_size += plus + 100;
	RECREATE(generic_ui_array, unsigned int, generic_ui_array_size);
}

#ifdef Pandas_ScriptCommands
//************************************
// Method:      script_abort
// Description: 中断脚本的执行并打印相关的错误信息
// Access:      public 
// Parameter:   struct script_state * st
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2022/3/29 9:36
//************************************
void script_abort(struct script_state* st) {
	struct script_data* data = nullptr;

	if (script_hasdata(st, 2)) {
		data = script_getdata(st, 2);
	}

	script_reportsrc(st);
	script_reportfunc(st);

	if (data) {
		script_reportdata(data);
	}
	st->state = END;
}

//************************************
// Method:      script_get_optnum
// Description: 获取 st 中指定 loc 位置的可选数值参数
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   int loc
// Parameter:   const char * desc
// Parameter:   int & ret
// Parameter:   bool allow_notexists
// Parameter:   int defval
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/12 12:06
//************************************ 
bool script_get_optnum(struct script_state *st, int loc, const char* desc, int &ret, bool allow_notexists = false, int defval = 0) {
	if (!st) return false;

	if (!script_hasdata(st, loc)) {
		if (allow_notexists) {
			ret = defval;
			return true;
		}
		script_reportsrc(st);
		script_reportfunc(st);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter can not be found.\n", script_getfuncname(st), loc - 1);
		else
			ShowError("buildin_%s: the No.%d parameter (%s) can not be found.\n", script_getfuncname(st), loc - 1, desc);
		return false;
	}

	if (!script_isint(st, loc)) {
		script_reportsrc(st);
		script_reportfunc(st);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter must be integer type.\n", script_getfuncname(st), loc - 1);
		else
			ShowError("buildin_%s: the No.%d parameter (%s) must be integer type.\n", script_getfuncname(st), loc - 1, desc);
		return false;
	}

	ret = script_getnum(st, loc);
	return true;
}

//************************************
// Method:      script_get_optstr
// Description: 获取 st 中指定 loc 位置的可选字符串参数
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   int loc
// Parameter:   const char * desc
// Parameter:   std::string & ret
// Parameter:   bool allow_notexists
// Parameter:   std::string defval
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/12 12:08
//************************************ 
bool script_get_optstr(struct script_state *st, int loc, const char* desc, std::string &ret, bool allow_notexists = false, std::string defval = "") {
	if (!st) return false;

	if (!script_hasdata(st, loc)) {
		if (allow_notexists) {
			ret = defval;
			return true;
		}
		script_reportsrc(st);
		script_reportfunc(st);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter can not be found.\n", script_getfuncname(st), loc - 1);
		else
			ShowError("buildin_%s: the No.%d parameter (%s) can not be found.\n", script_getfuncname(st), loc - 1, desc);
		return false;
	}

	if (!script_isstring(st, loc)) {
		script_reportsrc(st);
		script_reportfunc(st);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter must be string type.\n", script_getfuncname(st), loc - 1);
		else
			ShowError("buildin_%s: the No.%d parameter (%s) must be string type.\n", script_getfuncname(st), loc - 1, desc);
		return false;
	}

	ret = script_getstr(st, loc);
	return true;
}

//************************************
// Method:      script_cleararray_st
// Description: 清理 st 中指定 loc 数组变量的内容
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   int loc
// Parameter:   bool bslient
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/12 11:42
//************************************ 
bool script_cleararray_st(struct script_state* st, int loc, bool bslient = true) {
	struct map_session_data* sd = nullptr;
	struct script_data* param_data = script_getdata(st, loc);
	int varid = reference_getid(param_data);
	const char* varname = reference_getname(param_data);

	if (!data_isreference(param_data)) {
		if (!bslient) {
			script_reportsrc(st);
			script_reportfunc(st);
			script_reportdata(param_data);
			ShowError("buildin_%s: variable '%s' is not a array.\n", script_getfuncname(st), varname);
		}
		return false;
	}

	if (not_server_variable(*varname)) {
		if (!script_rid2sd(sd)) {
			if (!bslient) {
				ShowError("buildin_%s: '%s' is not server variable, please attach to a player.\n", script_getfuncname(st), varname);
				script_reportsrc(st);
				script_reportfunc(st);
				script_reportdata(param_data);
			}
			return false;
		}
	}

	struct reg_db* src = nullptr;
	if (!(src = script_array_src(st, sd, varname, reference_getref(param_data))))
		return false;

	script_array_ensure_zero(st, sd, param_data->u.num, reference_getref(param_data));

	struct script_array* sa = nullptr;
	if (!(sa = static_cast<script_array*>(idb_get(src->arrays, varid))))
		return false;

	unsigned int len = 0;
	len = script_array_highest_key(st, sd, varname, reference_getref(param_data));

	unsigned int* list = script_array_cpy_list(sa);
	unsigned int size = sa->size;
	for (unsigned int i = 0; i < size; i++) {
		clear_reg(st, sd, reference_uid(varid, list[i]), varname, reference_getref(param_data));
	}
	return true;
}

//************************************
// Method:      script_get_array
// Description: 获取 st 中指定 loc 位置的数组参数
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   int loc
// Parameter:   int & ret_varid
// Parameter:   char * & ret_varname
// Parameter:   struct script_data * ret_vardata
// Parameter:   bool expected_str
// Parameter:   const char * desc
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/12 16:26
//************************************ 
bool script_get_array(struct script_state* st, int loc, int& ret_varid, char*& ret_varname, struct script_data*& ret_vardata, bool expected_str = false, const char* desc = nullptr) {
	if (!script_hasdata(st, loc)) {
		script_reportsrc(st);
		script_reportfunc(st);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter can not be found.\n", script_getfuncname(st), loc - 1);
		else
			ShowError("buildin_%s: the No.%d parameter (%s) can not be found.\n", script_getfuncname(st), loc - 1, desc);
		return false;
	}

	struct map_session_data* sd = nullptr;
	ret_vardata = script_getdata(st, loc);
	ret_varid = reference_getid(ret_vardata);
	ret_varname = reference_getname(ret_vardata);

	if (!data_isreference(ret_vardata)) {
		script_reportsrc(st);
		script_reportfunc(st);
		script_reportdata(ret_vardata);
		ShowError("buildin_%s: variable '%s' is not a array.\n", script_getfuncname(st), ret_varname);
		return false;
	}

	if (not_server_variable(*ret_varname)) {
		if (!script_rid2sd(sd)) {
			script_reportsrc(st);
			script_reportfunc(st);
			script_reportdata(ret_vardata);
			ShowError("buildin_%s: '%s' is not server variable, please attach to a player.\n", script_getfuncname(st), ret_varname);
			return false;
		}
	}

	struct reg_db* src = nullptr;
	if (!(src = script_array_src(st, sd, ret_varname, reference_getref(ret_vardata)))) {
		script_reportsrc(st);
		script_reportfunc(st);
		script_reportdata(ret_vardata);
		ShowError("buildin_%s: variable '%s' is not a array.\n", script_getfuncname(st), ret_varname);
		return false;
	}

	if (expected_str && !is_string_variable(ret_varname)) {
		script_reportsrc(st);
		script_reportfunc(st);
		script_reportdata(ret_vardata);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter '%s' must be string array type.\n", script_getfuncname(st), loc - 1, ret_varname);
		else
			ShowError("buildin_%s: the No.%d parameter '%s' (%s) must be string array type.\n", script_getfuncname(st), loc - 1, ret_varname, desc);
		script_pushint(st, -1);
		return false;
	}

	if (!expected_str && is_string_variable(ret_varname)) {
		script_reportsrc(st);
		script_reportfunc(st);
		script_reportdata(ret_vardata);
		if (!desc)
			ShowError("buildin_%s: the No.%d parameter '%s' must be integer array type.\n", script_getfuncname(st), loc - 1, ret_varname);
		else
			ShowError("buildin_%s: the No.%d parameter '%s' (%s) must be integer array type.\n", script_getfuncname(st), loc - 1, ret_varname, desc);
		script_pushint(st, -1);
		return false;
	}

	return true;
}

//************************************
// Method:      script_get_mapindex
// Description: 获取地图名对应的地图索引值 (自动处理 this 特殊地图名)
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   const char * mapname
// Parameter:   int & map_id
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/02/12 00:12
//************************************ 
bool script_get_mapindex(struct script_state *st, const char* mapname, int &mapindex, int char_id = 0) {
	if (stricmp(mapname, "this") != 0) {
		mapindex = map_mapname2mapid(mapname);
		return (mapindex >= 0);
	}

	struct map_session_data* sd = nullptr;

	if (char_id) {
		sd = map_charid2sd(char_id);
		if (!sd) {
			script_reportsrc(st);
			script_reportfunc(st);
			ShowError("buildin_%s: mapname is 'this', but player with char id '%d' is not found.\n", script_getfuncname(st));
			return false;
		}
	}
	else {
		sd = map_id2sd(st->rid);
		if (!sd) {
			script_reportsrc(st);
			script_reportfunc(st);
			ShowError("buildin_%s: mapname is 'this', please attach to a player.\n", script_getfuncname(st));
			return false;
		}
	}
	mapindex = sd->bl.m;

	return (mapindex >= 0);
}

//************************************
// Method:      script_both_setreg
// Description: 同时设置 $@ 和 @ 数值变量 (设置 @ 变量的前提是能找到 sd)
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   const char * varname_without_prefix
// Parameter:   int64 value
// Parameter:   bool isarray	是不是数组
// Parameter:   int index		如果是数组那么索引是多少
// Parameter:   int char_id		若提供了角色编号则将 @ 变量值写入该角色
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/08/17 23:34
//************************************ 
void script_both_setreg(struct script_state* st, const char* varname_without_prefix, int64 value, bool isarray, int index = -1, int char_id = 0) {
	struct map_session_data* sd = nullptr;
	int64 varid = 0;

	sd = map_id2sd(st->rid);
	if (char_id) {
		sd = map_charid2sd(char_id);
	}

	std::string varname = "$@";
	varname += varname_without_prefix;
	varid = (isarray ? reference_uid(add_str(varname.c_str()), index) : add_str(varname.c_str()));
	mapreg_setreg(varid, value);

	if (sd) {
		varname = "@";
		varname += varname_without_prefix;
		varid = (isarray ? reference_uid(add_str(varname.c_str()), index) : add_str(varname.c_str()));
		pc_setreg(sd, varid, value);
	}
}

//************************************
// Method:      script_both_setregstr
// Description: 同时设置 $@ 和 @ 字符串变量 (设置 @ 变量的前提是能找到 sd)
// Access:      public 
// Parameter:   struct script_state * st
// Parameter:   const char * varname_without_prefix
// Parameter:   const char * value
// Parameter:   bool isarray	是不是数组
// Parameter:   int index		如果是数组那么索引是多少
// Parameter:   int char_id		若提供了角色编号则将 @ 变量值写入该角色
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/08/17 23:36
//************************************ 
void script_both_setregstr(struct script_state* st, const char* varname_without_prefix, const char* value, bool isarray, int index = -1, int char_id = 0) {
	struct map_session_data* sd = nullptr;
	int64 varid = 0;

	sd = map_id2sd(st->rid);
	if (char_id) {
		sd = map_charid2sd(char_id);
	}

	std::string varname = "$@";
	varname += varname_without_prefix;
	varid = (isarray ? reference_uid(add_str(varname.c_str()), index) : add_str(varname.c_str()));
	mapreg_setregstr(varid, value);

	if (sd) {
		varname = "@";
		varname += varname_without_prefix;
		varid = (isarray ? reference_uid(add_str(varname.c_str()), index) : add_str(varname.c_str()));
		pc_setregstr(sd, varid, value);
	}
}

//************************************
// Method:      script_getstorage
// Description: 根据指令名称来获取不同的存储空间
// Parameter:   struct script_state * st
// Parameter:   struct map_session_data * sd
// Parameter:   struct s_storage * * stor
// Parameter:   struct item * * inventory
// Parameter:   int stor_id
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2022/08/06 12:11
//************************************
bool script_getstorage(struct script_state* st, struct map_session_data* sd, struct s_storage** stor, struct item** inventory, int stor_id = 0) {
	nullpo_retr(false, st);
	nullpo_retr(false, sd);
	nullpo_retr(false, stor);
	nullpo_retr(false, inventory);
	
	const char* command = script_getfuncname(st);
	
	if (strstr(command, "cart")) {
		if (!pc_iscarton(sd)) {
			ShowError("buildin_%s: player doesn't have cart (CID: %d).\n", command, sd->status.char_id);
			return false;
		}
		*stor = &sd->cart;
		*inventory = (*stor)->u.items_cart;
	}
	else if (strstr(command, "guildstorage")) {
		if (!sd->status.guild_id || !sd->guild) {
			ShowError("buildin_%s: player doesn't join the guild (CID: %d).\n", command, sd->status.char_id);
			return false;
		}

#ifdef OFFICIAL_GUILD_STORAGE
		if (!guild_checkskill(sd->guild, GD_GUILD_STORAGE)) {
			ShowError("buildin_%s: player's guild has not learned the GD_GUILD_STORAGE skill (CID: %d).\n", command, sd->status.char_id);
			return false;
		}
#endif // OFFICIAL_GUILD_STORAGE

		if (guild2storage2(sd->status.guild_id) || st->waiting_guild_storage) {
			// 如果该公会的仓库数据已经在地图服务器内存中, 则直接使用
			*stor = guild2storage2(sd->status.guild_id);
			if (!(*stor)) {
				ShowError("buildin_%s: player's guild does not have a guild storage (CID: %d | Guild ID: %d).\n", command, sd->status.char_id, sd->status.guild_id);
				return false;
			}
			*inventory = (*stor)->u.items_guild;

			st->waiting_guild_storage = 0;
			st->state = RUN;
		}
		else if (!st->waiting_guild_storage) {
			// 否则, 需要先发送请求给角色服务器, 用于加载指定的公会仓库内容
			st->state = RERUNLINE;
			st->waiting_guild_storage = 1;
			intif_request_guild_storage(sd->status.account_id, sd->status.guild_id);
		}
		else if (!guild2storage2(sd->status.guild_id)) {
			ShowError("buildin_%s: player's guild does not have a guild storage (CID: %d | Guild ID: %d).\n", command, sd->status.char_id, sd->status.guild_id);
			return false;
		}
	}
	else if (strstr(command, "storage")) {
		if (stor_id == 0) {
			*stor = &sd->storage;
			*inventory = (*stor)->u.items_storage;
		}
		else if (!storage_exists(stor_id)) {
			ShowError("buildin_%s: Invalid storage id '%d'!\n", command, stor_id);
			return false;
		}
		else {
			if (sd->premiumStorage.stor_id == stor_id || st->waiting_premium_storage) {
				// 如果现有的 premiumStorage 就是我们期望的拓展仓库
				// 参考 storage_premiumStorage_load 的逻辑, 此时的 premiumStorage 内容可信
				*stor = &sd->premiumStorage;
				*inventory = (*stor)->u.items_storage;

				st->waiting_premium_storage = 0;
				st->state = RUN;
			}
			else if (!st->waiting_premium_storage) {
				// 否则, 需要先发送请求给角色服务器, 用于加载指定的拓展仓库内容
				st->state = RERUNLINE;
				st->waiting_premium_storage = 1;
				intif_storage_request(sd, TABLE_STORAGE, stor_id, STOR_MODE_ALL);
			}
		}
	}
	else {
		if (!strstr(command, "inventory")) {
			ShowWarning("%s: unknow function command: '%s', defaulting to inventory.\n", __func__, command);
		}
		*stor = &sd->inventory;
		*inventory = (*stor)->u.items_inventory;
	}

	return true;
}

#endif // Pandas_ScriptCommands

/*==========================================
 * Destructor
 *------------------------------------------*/
void do_final_script() {
	int i;
	DBIterator *iter;
	struct script_state *st;

#ifdef DEBUG_HASH
	if (battle_config.etc_log)
	{
		FILE *fp = fopen("hash_dump.txt","wt");
		if(fp) {
			int count[SCRIPT_HASH_SIZE];
			int count2[SCRIPT_HASH_SIZE]; // number of buckets with a certain number of items
			int n=0;
			int min=INT_MAX,max=0,zero=0;
			double mean=0.0f;
			double median=0.0f;

			ShowNotice("Dumping script str hash information to hash_dump.txt\n");
			memset(count, 0, sizeof(count));
			fprintf(fp,"num : hash : data_name\n");
			fprintf(fp,"---------------------------------------------------------------\n");
			for(i=LABEL_START; i<str_num; i++) {
				unsigned int h = calc_hash(get_str(i));
				fprintf(fp,"%04d : %4u : %s\n",i,h, get_str(i));
				++count[h];
			}
			fprintf(fp,"--------------------\n\n");
			memset(count2, 0, sizeof(count2));
			for(i=0; i<SCRIPT_HASH_SIZE; i++) {
				fprintf(fp,"  hash %3d = %d\n",i,count[i]);
				if(min > count[i])
					min = count[i];		// minimun count of collision
				if(max < count[i])
					max = count[i];		// maximun count of collision
				if(count[i] == 0)
					zero++;
				++count2[count[i]];
			}
			fprintf(fp,"\n--------------------\n  items : buckets\n--------------------\n");
			for( i=min; i <= max; ++i ){
				fprintf(fp,"  %5d : %7d\n",i,count2[i]);
				mean += 1.0f*i*count2[i]/SCRIPT_HASH_SIZE; // Note: this will always result in <nr labels>/<nr buckets>
			}
			for( i=min; i <= max; ++i ){
				n += count2[i];
				if( n*2 >= SCRIPT_HASH_SIZE )
				{
					if( SCRIPT_HASH_SIZE%2 == 0 && SCRIPT_HASH_SIZE/2 == n )
						median = (i+i+1)/2.0f;
					else
						median = i;
					break;
				}
			}
			fprintf(fp,"--------------------\n    min = %d, max = %d, zero = %d\n    mean = %lf, median = %lf\n",min,max,zero,mean,median);
			fclose(fp);
		}
	}
#endif

	mapreg_final();

	db_destroy(scriptlabel_db);
	userfunc_db->destroy(userfunc_db, db_script_free_code_sub);
	autobonus_db->destroy(autobonus_db, db_script_free_code_sub);

	ers_destroy(array_ers);
	if (generic_ui_array)
		aFree(generic_ui_array);

	iter = db_iterator(st_db);
	for( st = static_cast<script_state *>(dbi_first(iter)); dbi_exists(iter); st = static_cast<script_state *>(dbi_next(iter)) )
		script_free_state(st);
	dbi_destroy(iter);

	if (str_data)
		aFree(str_data);
	if (str_buf)
		aFree(str_buf);

	for( i = 0; i < atcmd_binding_count; i++ ) {
		aFree(atcmd_binding[i]);
	}

	if( atcmd_binding_count != 0 )
		aFree(atcmd_binding);

	ers_destroy(st_ers);
	ers_destroy(stack_ers);
	db_destroy(st_db);
}
/*==========================================
 * Initialization
 *------------------------------------------*/
void do_init_script(void) {
	st_db = idb_alloc(DB_OPT_BASE);
	userfunc_db = strdb_alloc(DB_OPT_DUP_KEY,0);
	scriptlabel_db = strdb_alloc(DB_OPT_DUP_KEY,50);
	autobonus_db = strdb_alloc(DB_OPT_DUP_KEY,0);

	st_ers = ers_new(sizeof(struct script_state), "script.cpp::st_ers", ERS_CACHE_OPTIONS);
	stack_ers = ers_new(sizeof(struct script_stack), "script.cpp::script_stack", ERS_OPT_FLEX_CHUNK);
	array_ers = ers_new(sizeof(struct script_array), "script.cpp:array_ers", ERS_CLEAN_OPTIONS);

	ers_chunk_size(st_ers, 10);
	ers_chunk_size(stack_ers, 10);

	active_scripts = 0;
	next_id = 0;

	mapreg_init();
	add_buildin_func();
	constant_db.load();
	script_hardcoded_constants();
}

void script_reload(void) {
	int i;
	DBIterator *iter;
	struct script_state *st;

	script_batch++; // Increment script batch number to prevent any pending async SQL future jobs from executing callback function after this point
	userfunc_db->clear(userfunc_db, db_script_free_code_sub);
	db_clear(scriptlabel_db);

	// @commands (script based)
	// Clear bindings
	for( i = 0; i < atcmd_binding_count; i++ ) {
		aFree(atcmd_binding[i]);
	}

	if( atcmd_binding_count != 0 )
		aFree(atcmd_binding);

	atcmd_binding_count = 0;

	iter = db_iterator(st_db);
	for( st = static_cast<script_state *>(dbi_first(iter)); dbi_exists(iter); st = static_cast<script_state *>(dbi_next(iter)) )
		script_free_state(st);
	dbi_destroy(iter);
	db_clear(st_db);

#ifdef Pandas_ScriptEngine_MutliStackBackup
	{
		struct s_mapiterator* iter = mapit_getallusers();
		struct map_session_data* sd = nullptr;
		for (sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
			if (!sd) continue;
			while (!sd->mbk_st.empty()) {
				sd->mbk_st.pop();
			}
		}
		if (iter) mapit_free(iter);
	}
#endif // Pandas_ScriptEngine_MutliStackBackup

	mapreg_reload();
}

//-----------------------------------------------------------------------------
// buildin functions
//

#define BUILDIN_DEF(x,args) { buildin_ ## x , #x , args, NULL }
#define BUILDIN_DEF2(x,x2,args) { buildin_ ## x , x2 , args, NULL }
#define BUILDIN_DEF_DEPRECATED(x,args,deprecationdate) { buildin_ ## x , #x , args, deprecationdate }
#define BUILDIN_DEF2_DEPRECATED(x,x2,args,deprecationdate) { buildin_ ## x , x2 , args, deprecationdate }
#define BUILDIN_FUNC(x) int buildin_ ## x (struct script_state* st)

/////////////////////////////////////////////////////////////////////
// NPC interaction
//

/// Appends a message to the npc dialog.
/// If a dialog doesn't exist yet, one is created.
///
/// mes "<message>";
BUILDIN_FUNC(mes)
{
	TBL_PC* sd;
	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	if( !script_hasdata(st, 3) )
	{// only a single line detected in the script
		clif_scriptmes(sd, st->oid, script_getstr(st, 2));
	}
	else
	{// parse multiple lines as they exist
		int i;

		for( i = 2; script_hasdata(st, i); i++ )
		{
			// send the message to the client
			clif_scriptmes(sd, st->oid, script_getstr(st, i));
		}
	}

	st->mes_active = 1; // Invoking character has a NPC dialog box open.
	return SCRIPT_CMD_SUCCESS;
}

/// Displays the button 'next' in the npc dialog.
/// The dialog text is cleared and the script continues when the button is pressed.
///
/// next;
BUILDIN_FUNC(next)
{
	TBL_PC* sd;

	if (!st->mes_active) {
		ShowWarning("buildin_next: There is no mes active.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;
#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_WAIT;
#endif
	st->state = STOP;
	clif_scriptnext(sd, st->oid);
	return SCRIPT_CMD_SUCCESS;
}

/// Clears the dialog and continues the script without a next button.
///
/// clear;
BUILDIN_FUNC(clear)
{
	TBL_PC* sd;

	if (!st->mes_active) {
		ShowWarning("buildin_clear: There is no mes active.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;

	clif_scriptclear(sd, st->oid);
	return SCRIPT_CMD_SUCCESS;
}

/// Ends the script and displays the button 'close' on the npc dialog.
/// The dialog is closed when the button is pressed.
///
/// close;
BUILDIN_FUNC(close)
{
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	if( !st->mes_active ) {
		st->state = END; // Keep backwards compatibility.
		ShowWarning("Incorrect use of 'close' command!\n");
		script_reportsrc(st);
	} else {
		st->state = CLOSE;
		st->mes_active = 0;
	}

	clif_scriptclose(sd, st->oid);
	return SCRIPT_CMD_SUCCESS;
}

/// Displays the button 'close' on the npc dialog.
/// The dialog is closed and the script continues when the button is pressed.
///
/// close2;
BUILDIN_FUNC(close2)
{
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	st->state = STOP;

	if( st->mes_active )
		st->mes_active = 0;

	clif_scriptclose(sd, st->oid);
	return SCRIPT_CMD_SUCCESS;
}

/// Counts the number of valid and total number of options in 'str'
/// If max_count > 0 the counting stops when that valid option is reached
/// total is incremented for each option (NULL is supported)
static int menu_countoptions(const char* str, int max_count, int* total)
{
	int count = 0;
	int bogus_total;

	if( total == NULL )
		total = &bogus_total;
	++(*total);

	// initial empty options
	while( *str == ':' )
	{
		++str;
		++(*total);
	}
	// count menu options
	while( *str != '\0' )
	{
		++count;
		--max_count;
		if( max_count == 0 )
			break;
		while( *str != ':' && *str != '\0' )
			++str;
		while( *str == ':' )
		{
			++str;
			++(*total);
		}
	}
	return count;
}

/// Displays a menu with options and goes to the target label.
/// The script is stopped if cancel is pressed.
/// Options with no text are not displayed in the client.
///
/// Options can be grouped together, separated by the character ':' in the text:
///   ex: menu "A:B:C",L_target;
/// All these options go to the specified target label.
///
/// The index of the selected option is put in the variable @menu.
/// Indexes start with 1 and are consistent with grouped and empty options.
///   ex: menu "A::B",-,"",L_Impossible,"C",-;
///       // displays "A", "B" and "C", corresponding to indexes 1, 3 and 5
///
/// NOTE: the client closes the npc dialog when cancel is pressed
///
/// menu "<option_text>",<target_label>{,"<option_text>",<target_label>,...};
BUILDIN_FUNC(menu)
{
	int i;
	const char* text;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_MENU;
#endif

	// TODO detect multiple scripts waiting for input at the same time, and what to do when that happens
	if( sd->state.menu_or_input == 0 )
	{
		struct StringBuf buf;

		if( script_lastdata(st) % 2 == 0 )
		{// argument count is not even (1st argument is at index 2)
			ShowError("buildin_menu: Illegal number of arguments (%d).\n", (script_lastdata(st) - 1));
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}

		StringBuf_Init(&buf);
		sd->npc_menu = 0;
		for( i = 2; i < script_lastdata(st); i += 2 )
		{
			struct script_data* data;
			// menu options
			text = script_getstr(st, i);

			// target label
			data = script_getdata(st, i+1);
			if( !data_islabel(data) )
			{// not a label
				StringBuf_Destroy(&buf);
				ShowError("buildin_menu: Argument #%d (from 1) is not a label or label not found.\n", i);
				script_reportdata(data);
				st->state = END;
				return SCRIPT_CMD_FAILURE;
			}

			// append option(s)
			if( text[0] == '\0' )
				continue;// empty string, ignore
			if( sd->npc_menu > 0 )
				StringBuf_AppendStr(&buf, ":");
			StringBuf_AppendStr(&buf, text);
			sd->npc_menu += menu_countoptions(text, 0, NULL);
		}
		st->state = RERUNLINE;
		sd->state.menu_or_input = 1;

		/**
		 * menus beyond this length crash the client (see bugreport:6402)
		 **/
		if( StringBuf_Length(&buf) >= 2047 ) {
			struct npc_data * nd = map_id2nd(st->oid);
			char* menu;
			CREATE(menu, char, 2048);
			safestrncpy(menu, StringBuf_Value(&buf), 2047);
			ShowWarning("buildin_menu: NPC Menu too long! (source:%s / length:%d)\n",nd?nd->name:"Unknown",StringBuf_Length(&buf));
			clif_scriptmenu(sd, st->oid, menu);
			aFree(menu);
		} else
			clif_scriptmenu(sd, st->oid, StringBuf_Value(&buf));

		StringBuf_Destroy(&buf);

		if( sd->npc_menu >= 0xff )
		{// client supports only up to 254 entries; 0 is not used and 255 is reserved for cancel; excess entries are displayed but cause 'uint8' overflow
			ShowWarning("buildin_menu: Too many options specified (current=%d, max=254).\n", sd->npc_menu);
			script_reportsrc(st);
		}
	}
	else if( sd->npc_menu == 0xff )
	{// Cancel was pressed
		sd->state.menu_or_input = 0;
		st->state = END;
	}
	else
	{// goto target label
		int menu = 0;

		sd->state.menu_or_input = 0;
		if( sd->npc_menu <= 0 )
		{
			ShowDebug("buildin_menu: Unexpected selection (%d)\n", sd->npc_menu);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}

		// get target label
		for( i = 2; i < script_lastdata(st); i += 2 )
		{
			text = script_getstr(st, i);
			sd->npc_menu -= menu_countoptions(text, sd->npc_menu, &menu);
			if( sd->npc_menu <= 0 )
				break;// entry found
		}
		if( sd->npc_menu > 0 )
		{// Invalid selection
			ShowDebug("buildin_menu: Selection is out of range (%d pairs are missing?) - please report this\n", sd->npc_menu);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
		if( !data_islabel(script_getdata(st, i + 1)) )
		{// TODO remove this temporary crash-prevention code (fallback for multiple scripts requesting user input)
			ShowError("buildin_menu: Unexpected data in label argument\n");
			script_reportdata(script_getdata(st, i + 1));
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
		pc_setreg(sd, add_str("@menu"), menu);
		st->pos = script_getnum(st, i + 1);
		st->state = GOTO;
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Displays a menu with options and returns the selected option.
/// Behaves like 'menu' without the target labels.
///
/// select(<option_text>{,<option_text>,...}) -> <selected_option>
///
/// @see menu
BUILDIN_FUNC(select)
{
	int i;
	const char* text;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_MENU;
#endif

	if( sd->state.menu_or_input == 0 ) {
		struct StringBuf buf;

		StringBuf_Init(&buf);
		sd->npc_menu = 0;
		for( i = 2; i <= script_lastdata(st); ++i ) {
			text = script_getstr(st, i);

			if( sd->npc_menu > 0 )
				StringBuf_AppendStr(&buf, ":");

			StringBuf_AppendStr(&buf, text);
			sd->npc_menu += menu_countoptions(text, 0, NULL);
		}

		st->state = RERUNLINE;
		sd->state.menu_or_input = 1;

		/**
		 * menus beyond this length crash the client (see bugreport:6402)
		 **/
		if( StringBuf_Length(&buf) >= 2047 ) {
			struct npc_data * nd = map_id2nd(st->oid);
			char* menu;
			CREATE(menu, char, 2048);
			safestrncpy(menu, StringBuf_Value(&buf), 2047);
			ShowWarning("buildin_select: NPC Menu too long! (source:%s / length:%d)\n",nd?nd->name:"Unknown",StringBuf_Length(&buf));
			clif_scriptmenu(sd, st->oid, menu);
			aFree(menu);
		} else
			clif_scriptmenu(sd, st->oid, StringBuf_Value(&buf));
		StringBuf_Destroy(&buf);

		if( sd->npc_menu >= 0xff ) {
			ShowWarning("buildin_select: Too many options specified (current=%d, max=254).\n", sd->npc_menu);
			script_reportsrc(st);
		}
	} else if( sd->npc_menu == 0xff ) {// Cancel was pressed
		sd->state.menu_or_input = 0;
		st->state = END;
	} else {// return selected option
		int menu = 0;

		sd->state.menu_or_input = 0;
		for( i = 2; i <= script_lastdata(st); ++i ) {
			text = script_getstr(st, i);
			sd->npc_menu -= menu_countoptions(text, sd->npc_menu, &menu);
			if( sd->npc_menu <= 0 )
				break;// entry found
		}
		pc_setreg(sd, add_str("@menu"), menu);
		script_pushint(st, menu);
		st->state = RUN;
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Displays a menu with options and returns the selected option.
/// Behaves like 'menu' without the target labels, except when cancel is
/// pressed.
/// When cancel is pressed, the script continues and 255 is returned.
///
/// prompt(<option_text>{,<option_text>,...}) -> <selected_option>
///
/// @see menu
BUILDIN_FUNC(prompt)
{
	int i;
	const char *text;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_MENU;
#endif

	if( sd->state.menu_or_input == 0 )
	{
		struct StringBuf buf;

		StringBuf_Init(&buf);
		sd->npc_menu = 0;
		for( i = 2; i <= script_lastdata(st); ++i )
		{
			text = script_getstr(st, i);
			if( sd->npc_menu > 0 )
				StringBuf_AppendStr(&buf, ":");
			StringBuf_AppendStr(&buf, text);
			sd->npc_menu += menu_countoptions(text, 0, NULL);
		}

		st->state = RERUNLINE;
		sd->state.menu_or_input = 1;

		/**
		 * menus beyond this length crash the client (see bugreport:6402)
		 **/
		if( StringBuf_Length(&buf) >= 2047 ) {
			struct npc_data * nd = map_id2nd(st->oid);
			char* menu;
			CREATE(menu, char, 2048);
			safestrncpy(menu, StringBuf_Value(&buf), 2047);
			ShowWarning("buildin_prompt: NPC Menu too long! (source:%s / length:%d)\n",nd?nd->name:"Unknown",StringBuf_Length(&buf));
			clif_scriptmenu(sd, st->oid, menu);
			aFree(menu);
		} else
			clif_scriptmenu(sd, st->oid, StringBuf_Value(&buf));
		StringBuf_Destroy(&buf);

		if( sd->npc_menu >= 0xff )
		{
			ShowWarning("buildin_prompt: Too many options specified (current=%d, max=254).\n", sd->npc_menu);
			script_reportsrc(st);
		}
	}
	else if( sd->npc_menu == 0xff )
	{// Cancel was pressed
		sd->state.menu_or_input = 0;
		pc_setreg(sd, add_str("@menu"), 0xff);
		script_pushint(st, 0xff);
		st->state = RUN;
	}
	else
	{// return selected option
		int menu = 0;

		sd->state.menu_or_input = 0;
		for( i = 2; i <= script_lastdata(st); ++i )
		{
			text = script_getstr(st, i);
			sd->npc_menu -= menu_countoptions(text, sd->npc_menu, &menu);
			if( sd->npc_menu <= 0 )
				break;// entry found
		}
		pc_setreg(sd, add_str("@menu"), menu);
		script_pushint(st, menu);
		st->state = RUN;
	}
	return SCRIPT_CMD_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// ...
//

/// Jumps to the target script label.
///
/// goto <label>;
BUILDIN_FUNC(goto)
{
	if( !data_islabel(script_getdata(st,2)) )
	{
		ShowError("buildin_goto: Not a label\n");
		script_reportdata(script_getdata(st,2));
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	st->pos = script_getnum(st,2);
	st->state = GOTO;
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * user-defined function call
 *------------------------------------------*/
BUILDIN_FUNC(callfunc)
{
	int i, j;
	struct script_retinfo* ri;
	struct script_code* scr;
	const char* str = script_getstr(st,2);
	struct reg_db *ref = NULL;

	scr = (struct script_code*)strdb_get(userfunc_db, str);
	if(!scr) {
		ShowError("buildin_callfunc: Function not found! [%s]\n", str);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	ref = (struct reg_db *)aCalloc(sizeof(struct reg_db), 2);
	ref[0].vars = st->stack->scope.vars;
	if (!st->stack->scope.arrays)
		st->stack->scope.arrays = idb_alloc(DB_OPT_BASE); // TODO: Can this happen? when?
	ref[0].arrays = st->stack->scope.arrays;
	ref[1].vars = st->script->local.vars;
	if (!st->script->local.arrays)
		st->script->local.arrays = idb_alloc(DB_OPT_BASE); // TODO: Can this happen? when?
	ref[1].arrays = st->script->local.arrays;

	for(i = st->start+3, j = 0; i < st->end; i++, j++) {
		struct script_data* data = push_copy(st->stack,i);

		if (data_isreference(data) && !data->ref) {
			const char* name = reference_getname(data);

			if (name[0] == '.')
				data->ref = (name[1] == '@' ? &ref[0] : &ref[1]);
		}
	}

	CREATE(ri, struct script_retinfo, 1);
	ri->script       = st->script;              // script code
	ri->scope.vars   = st->stack->scope.vars;   // scope variables
	ri->scope.arrays = st->stack->scope.arrays; // scope arrays
	ri->pos          = st->pos;                 // script location
	ri->nargs        = j;                       // argument count
	ri->defsp        = st->stack->defsp;        // default stack pointer
	push_retinfo(st->stack, ri, ref);

	st->pos = 0;
	st->script = scr;
	st->stack->defsp = st->stack->sp;
	st->state = GOTO;
	st->stack->scope.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
	st->stack->scope.arrays = idb_alloc(DB_OPT_BASE);

	if (!st->script->local.vars)
		st->script->local.vars = i64db_alloc(DB_OPT_RELEASE_DATA);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * subroutine call
 *------------------------------------------*/
BUILDIN_FUNC(callsub)
{
	int i,j;
	struct script_retinfo* ri;
	int pos = script_getnum(st,2);
	struct reg_db *ref = NULL;

	if( !data_islabel(script_getdata(st,2)) && !data_isfunclabel(script_getdata(st,2)) ) {
		ShowError("buildin_callsub: Argument is not a label\n");
		script_reportdata(script_getdata(st,2));
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	ref = (struct reg_db *)aCalloc(sizeof(struct reg_db), 1);
	ref[0].vars = st->stack->scope.vars;
	if (!st->stack->scope.arrays)
		st->stack->scope.arrays = idb_alloc(DB_OPT_BASE); // TODO: Can this happen? when?
	ref[0].arrays = st->stack->scope.arrays;

	for(i = st->start+3, j = 0; i < st->end; i++, j++) {
		struct script_data* data = push_copy(st->stack,i);

		if (data_isreference(data) && !data->ref) {
			const char* name = reference_getname(data);

			if (name[0] == '.' && name[1] == '@')
				data->ref = &ref[0];
		}
	}

	CREATE(ri, struct script_retinfo, 1);
	ri->script       = st->script;              // script code
	ri->scope.vars   = st->stack->scope.vars;   // scope variables
	ri->scope.arrays = st->stack->scope.arrays; // scope arrays
	ri->pos          = st->pos;                 // script location
	ri->nargs        = j;                       // argument count
	ri->defsp        = st->stack->defsp;        // default stack pointer
	push_retinfo(st->stack, ri, ref);

	st->pos = pos;
	st->stack->defsp = st->stack->sp;
	st->state = GOTO;
	st->stack->scope.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
	st->stack->scope.arrays = idb_alloc(DB_OPT_BASE);

	return SCRIPT_CMD_SUCCESS;
}

/// Retrieves an argument provided to callfunc/callsub.
/// If the argument doesn't exist
///
/// getarg(<index>{,<default_value>}) -> <value>
BUILDIN_FUNC(getarg)
{
	struct script_retinfo* ri;
	int idx;

	if( st->stack->defsp < 1 || st->stack->stack_data[st->stack->defsp - 1].type != C_RETINFO )
	{
		ShowError("buildin_getarg: No callfunc or callsub!\n");
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	ri = st->stack->stack_data[st->stack->defsp - 1].u.ri;

	idx = script_getnum(st,2);

	if( idx >= 0 && idx < ri->nargs )
		push_copy(st->stack, st->stack->defsp - 1 - ri->nargs + idx);
	else if( script_hasdata(st,3) )
		script_pushcopy(st, 3);
	else
	{
		ShowError("buildin_getarg: Index (idx=%d) out of range (nargs=%d) and no default value found\n", idx, ri->nargs);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Returns from the current function, optionaly returning a value from the functions.
/// Don't use outside script functions.
///
/// return;
/// return <value>;
BUILDIN_FUNC(return)
{
	if( script_hasdata(st,2) )
	{// return value
		struct script_data* data;
		script_pushcopy(st, 2);
		data = script_getdatatop(st, -1);
		if( data_isreference(data) ) {
			const char* name = reference_getname(data);
			if( name[0] == '.' && name[1] == '@' ) { // scope variable
				if( !data->ref || data->ref->vars == st->stack->scope.vars )
					get_val(st, data); // current scope, convert to value
				if( data->ref && data->ref->vars == st->stack->stack_data[st->stack->defsp-1].u.ri->scope.vars )
					data->ref = NULL; // Reference to the parent scope, remove reference pointer
			}
		}
	}
	else
	{// no return value
		script_pushnil(st);
	}
	st->state = RETFUNC;
	return SCRIPT_CMD_SUCCESS;
}

/// Returns a random number from 0 to <range>-1.
/// Or returns a random number from <min> to <max>.
/// If <min> is greater than <max>, their numbers are switched.
/// rand(<range>) -> <int>
/// rand(<min>,<max>) -> <int>
BUILDIN_FUNC(rand)
{
	int range;
	int min;

	if( script_hasdata(st,3) )
	{// min,max
		int max = script_getnum(st,3);
		min = script_getnum(st,2);
		if( max < min )
			SWAP(min, max);
		range = max - min + 1;
	}
	else
	{// range
		min = 0;
		range = script_getnum(st,2);
	}
	if( range <= 1 )
		script_pushint(st, min);
	else
		script_pushint(st, rnd()%range + min);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Warp sd to str,x,y or Random or SavePoint/Save
 *------------------------------------------*/
BUILDIN_FUNC(warp)
{
	int ret;
	int x,y;
	const char* str;
	struct map_session_data* sd;

	if(!script_charid2sd(5, sd))
		return SCRIPT_CMD_SUCCESS;

	str = script_getstr(st,2);
	x = script_getnum(st,3);
	y = script_getnum(st,4);

	if(strcmp(str,"Random")==0)
		ret = pc_randomwarp(sd,CLR_TELEPORT,true);
	else if(strcmp(str,"SavePoint")==0 || strcmp(str,"Save")==0)
		ret = pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,CLR_TELEPORT);
	else
		ret = pc_setpos(sd,mapindex_name2id(str),x,y,CLR_OUTSIGHT);

	if( ret ) {
		ShowError("buildin_warp: moving player '%s' to \"%s\",%d,%d failed.\n", sd->status.name, str, x, y);
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * Warp a specified area
 *------------------------------------------*/
static int buildin_areawarp_sub(struct block_list *bl,va_list ap)
{
	int x2,y2,x3,y3;
	unsigned int index;

	index = va_arg(ap,unsigned int);
	x2 = va_arg(ap,int);
	y2 = va_arg(ap,int);
	x3 = va_arg(ap,int);
	y3 = va_arg(ap,int);

#ifdef Pandas_Support_Transfer_Autotrade_Player
	pc_mark_multitransfer(bl);
#endif // Pandas_Support_Transfer_Autotrade_Player

	if(index == 0)
		pc_randomwarp((TBL_PC *)bl,CLR_TELEPORT,true);
	else if(x3 && y3) {
		int max, tx, ty, j = 0;
		int16 m;

		m = map_mapindex2mapid(index);

		// choose a suitable max number of attempts
		if( (max = (y3-y2+1)*(x3-x2+1)*3) > 1000 )
			max = 1000;

		// find a suitable map cell
		do {
			tx = rnd()%(x3-x2+1)+x2;
			ty = rnd()%(y3-y2+1)+y2;
			j++;
		} while( map_getcell(m,tx,ty,CELL_CHKNOPASS) && j < max );

		pc_setpos((TBL_PC *)bl,index,tx,ty,CLR_OUTSIGHT);
	}
	else
		pc_setpos((TBL_PC *)bl,index,x2,y2,CLR_OUTSIGHT);
	return 0;
}

BUILDIN_FUNC(areawarp)
{
	int16 m, x0,y0,x1,y1, x2,y2,x3=0,y3=0;
	unsigned int index;
	const char *str;
	const char *mapname;

	mapname = script_getstr(st,2);
	x0  = script_getnum(st,3);
	y0  = script_getnum(st,4);
	x1  = script_getnum(st,5);
	y1  = script_getnum(st,6);
	str = script_getstr(st,7);
	x2  = script_getnum(st,8);
	y2  = script_getnum(st,9);

	if( script_hasdata(st,10) && script_hasdata(st,11) ) { // Warp area to area
		if( (x3 = script_getnum(st,10)) < 0 || (y3 = script_getnum(st,11)) < 0 ){
			x3 = 0;
			y3 = 0;
		} else if( x3 && y3 ) {
			// normalize x3/y3 coordinates
			if( x3 < x2 ) SWAP(x3,x2);
			if( y3 < y2 ) SWAP(y3,y2);
		}
	}

	if( (m = map_mapname2mapid(mapname)) < 0 )
		return SCRIPT_CMD_FAILURE;

	if( strcmp(str,"Random") == 0 )
		index = 0;
	else if( !(index=mapindex_name2id(str)) )
		return SCRIPT_CMD_FAILURE;

	map_foreachinallarea(buildin_areawarp_sub, m,x0,y0,x1,y1, BL_PC, index,x2,y2,x3,y3);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * areapercentheal <map>,<x1>,<y1>,<x2>,<y2>,<hp>,<sp>
 *------------------------------------------*/
static int buildin_areapercentheal_sub(struct block_list *bl,va_list ap)
{
	int hp, sp;
	hp = va_arg(ap, int);
	sp = va_arg(ap, int);
	pc_percentheal((TBL_PC *)bl,hp,sp);
	return 0;
}

BUILDIN_FUNC(areapercentheal)
{
	int hp,sp,m;
	const char *mapname;
	int x0,y0,x1,y1;

	mapname=script_getstr(st,2);
	x0=script_getnum(st,3);
	y0=script_getnum(st,4);
	x1=script_getnum(st,5);
	y1=script_getnum(st,6);
	hp=script_getnum(st,7);
	sp=script_getnum(st,8);

	if( (m=map_mapname2mapid(mapname))< 0)
		return SCRIPT_CMD_FAILURE;

	map_foreachinallarea(buildin_areapercentheal_sub,m,x0,y0,x1,y1,BL_PC,hp,sp);
	return SCRIPT_CMD_SUCCESS;
}

enum e_warpparty_target{
	WARPPARTY_RANDOM = 0,
	WARPPARTY_SAVEPOINTALL,
	WARPPARTY_SAVEPOINT,
	WARPPARTY_LEADER,
	WARPPARTY_RANDOMALL,
	WARPPARTY_RANDOMALLAREA
};

/*==========================================
 * Warpparty - [Fredzilla] [Paradox924X]
 * Syntax: warpparty "to_mapname",x,y,Party_ID,{<"from_mapname">,<range x>,<range y>};
 * If 'from_mapname' is specified, only the party members on that map will be warped
 *------------------------------------------*/
BUILDIN_FUNC(warpparty)
{
	TBL_PC *sd = NULL;
	TBL_PC *pl_sd;
	struct party_data* p;
	int mapindex = 0, m = -1, i, rx = 0, ry = 0;

	const char* str = script_getstr(st,2);
	int x = script_getnum(st,3);
	int y = script_getnum(st,4);
	int p_id = script_getnum(st,5);
	const char* str2 = NULL;
	if ( script_hasdata(st,6) )
		str2 = script_getstr(st,6);
	if (script_hasdata(st, 7))
		rx = script_getnum(st, 7);
	if (script_hasdata(st, 8))
		ry = script_getnum(st, 8);

	p = party_search(p_id);
	if(!p)
		return SCRIPT_CMD_SUCCESS;

	enum e_warpparty_target type = ( strcmp(str,"Random")==0 ) ? WARPPARTY_RANDOM
	     : ( strcmp(str,"SavePointAll")==0 ) ? WARPPARTY_SAVEPOINTALL
		 : ( strcmp(str,"SavePoint")==0 ) ? WARPPARTY_SAVEPOINT
		 : ( strcmp(str,"Leader")==0 ) ? WARPPARTY_LEADER
		 : ( strcmp(str,"RandomAll")==0 ) ? WARPPARTY_RANDOMALL
		 : WARPPARTY_RANDOMALLAREA;

	switch (type)
	{
		case WARPPARTY_SAVEPOINT:
			//"SavePoint" uses save point of the currently attached player
			if ( !script_rid2sd(sd) )
				return SCRIPT_CMD_FAILURE;
			break;
		case WARPPARTY_LEADER:
			for(i = 0; i < MAX_PARTY && !p->party.member[i].leader; i++);
			if (i == MAX_PARTY || !p->data[i].sd) //Leader not found / not online
				return SCRIPT_CMD_FAILURE;
			pl_sd = p->data[i].sd;
			mapindex = pl_sd->mapindex;
			m = map_mapindex2mapid(mapindex);
			x = pl_sd->bl.x;
			y = pl_sd->bl.y;
			break;
		case WARPPARTY_RANDOMALL: {
			if ( !script_rid2sd(sd) )
				return SCRIPT_CMD_FAILURE;

			mapindex = sd->mapindex;
			m = map_mapindex2mapid(mapindex);

			struct map_data *mapdata = map_getmapdata(m);

			if ( mapdata == nullptr || mapdata->flag[MF_NOWARP] || mapdata->flag[MF_NOTELEPORT] )
				return SCRIPT_CMD_FAILURE;

			i = 0;
			do {
				x = rnd()%(mapdata->xs - 2) + 1;
				y = rnd()%(mapdata->ys - 2) + 1;
			} while ((map_getcell(m,x,y,CELL_CHKNOPASS) || (!battle_config.teleport_on_portal && npc_check_areanpc(1,m,x,y,1))) && (i++) < 1000);

			if (i >= 1000) {
				ShowError("buildin_warpparty: moving player '%s' to \"%s\",%d,%d failed.\n", sd->status.name, str, x, y);
				return SCRIPT_CMD_FAILURE;
			}
			} break;
		case WARPPARTY_RANDOMALLAREA:
			mapindex = mapindex_name2id(str);
			if (!mapindex) {// Invalid map
				return SCRIPT_CMD_FAILURE;
			}
			m = map_mapindex2mapid(mapindex);
			break;
	}

	for (i = 0; i < MAX_PARTY; i++)
	{
		if( !(pl_sd = p->data[i].sd) || pl_sd->status.party_id != p_id )
			continue;

		map_data* mapdata = map_getmapdata(pl_sd->bl.m);

		if( str2 && strcmp(str2, mapdata->name) != 0 )
			continue;

#ifndef Pandas_ScriptCommand_WarpPartyRevive
		if( pc_isdead(pl_sd) )
			continue;
#else
		// 若使用的为 warppartyrevive 指令名, 那么死亡的队员也不会被忽略
		if (pc_isdead(pl_sd) && strcmpi(script_getfuncname(st), "warppartyrevive") != 0 && strcmpi(script_getfuncname(st), "warpparty2") != 0)
			continue;
#endif // Pandas_ScriptCommand_WarpPartyRevive

#ifdef Pandas_Support_Transfer_Autotrade_Player
		pc_mark_multitransfer(pl_sd);
#endif // Pandas_Support_Transfer_Autotrade_Player

		e_setpos ret = SETPOS_OK;

		switch( type )
		{
		case WARPPARTY_RANDOM:
			if (!mapdata->flag[MF_NOWARP])
				ret = (e_setpos)pc_randomwarp(pl_sd,CLR_TELEPORT);
		break;
		case WARPPARTY_SAVEPOINTALL:
			if (!mapdata->flag[MF_NORETURN])
				ret = pc_setpos(pl_sd,pl_sd->status.save_point.map,pl_sd->status.save_point.x,pl_sd->status.save_point.y,CLR_TELEPORT);
		break;
		case WARPPARTY_SAVEPOINT:
			if (!mapdata->flag[MF_NORETURN])
				ret = pc_setpos(pl_sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,CLR_TELEPORT);
		break;
		case WARPPARTY_LEADER:
			if (p->party.member[i].leader)
				continue;
			// Fall through
		case WARPPARTY_RANDOMALL:
			if (pl_sd == sd) {
				ret = pc_setpos(pl_sd, mapindex, x, y, CLR_TELEPORT);
				break;
			}
			// Fall through
		case WARPPARTY_RANDOMALLAREA:
			if(!mapdata->flag[MF_NORETURN] && !mapdata->flag[MF_NOWARP] && pc_job_can_entermap((enum e_job)pl_sd->status.class_, m, pc_get_group_level(pl_sd))){
				if (rx || ry) {
					int x1 = x + rx, y1 = y + ry,
						x0 = x - rx, y0 = y - ry,
						nx, ny;
					uint8 attempts = 10;

					do {
						nx = x0 + rnd()%(x1 - x0 + 1);
						ny = y0 + rnd()%(y1 - y0 + 1);
					} while ((--attempts) > 0 && !map_getcell(m, nx, ny, CELL_CHKPASS));

					if (attempts != 0) { //Keep the original coordinates if fails to find a valid cell within the range
						x = nx;
						y = ny;
					}
				}

				ret = pc_setpos(pl_sd, mapindex, x, y, CLR_TELEPORT);
			}
			break;
		}

		if( ret != SETPOS_OK ) {
			ShowError("buildin_warpparty: moving player '%s' to \"%s\",%d,%d failed.\n", pl_sd->status.name, str, x, y);
			if ( ( type == WARPPARTY_RANDOMALL || type == WARPPARTY_RANDOMALLAREA ) && (rx || ry) )
				continue;
			else
				return SCRIPT_CMD_FAILURE;
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Warpguild - [Fredzilla]
 * Syntax: warpguild "mapname",x,y,Guild_ID;
 *------------------------------------------*/
BUILDIN_FUNC(warpguild)
{
	TBL_PC *sd = NULL;
	TBL_PC *pl_sd;
	struct guild* g;
	struct s_mapiterator* iter;
	int type, mapindex = 0, m = -1;

	const char* str = script_getstr(st,2);
	int x           = script_getnum(st,3);
	int y           = script_getnum(st,4);
	int gid         = script_getnum(st,5);

	g = guild_search(gid);
	if( g == NULL )
		return SCRIPT_CMD_SUCCESS;

	type = ( strcmp(str,"Random")==0 ) ? 0
	     : ( strcmp(str,"SavePointAll")==0 ) ? 1
		 : ( strcmp(str,"SavePoint")==0 ) ? 2
		 : 3;

	if( type == 2 && !script_rid2sd(sd) )
	{// "SavePoint" uses save point of the currently attached player
		return SCRIPT_CMD_SUCCESS;
	}

	switch (type) {
		case 3:
			mapindex = mapindex_name2id(str);
			if (!mapindex)
				return SCRIPT_CMD_FAILURE;
			m = map_mapindex2mapid(mapindex);
			break;
	}

	iter = mapit_getallusers();
	for( pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter) )
	{
		if( pl_sd->status.guild_id != gid )
			continue;

#ifdef Pandas_Support_Transfer_Autotrade_Player
		pc_mark_multitransfer(pl_sd);
#endif // Pandas_Support_Transfer_Autotrade_Player

		switch( type )
		{
		case 0: // Random
			if(!map_getmapflag(pl_sd->bl.m, MF_NOWARP))
				pc_randomwarp(pl_sd,CLR_TELEPORT);
		break;
		case 1: // SavePointAll
			if(!map_getmapflag(pl_sd->bl.m, MF_NORETURN))
				pc_setpos(pl_sd,pl_sd->status.save_point.map,pl_sd->status.save_point.x,pl_sd->status.save_point.y,CLR_TELEPORT);
		break;
		case 2: // SavePoint
			if(!map_getmapflag(pl_sd->bl.m, MF_NORETURN))
				pc_setpos(pl_sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,CLR_TELEPORT);
		break;
		case 3: // m,x,y
			if(!map_getmapflag(pl_sd->bl.m, MF_NORETURN) && !map_getmapflag(pl_sd->bl.m, MF_NOWARP) && pc_job_can_entermap((enum e_job)pl_sd->status.class_, m, pc_get_group_level(pl_sd)))
				pc_setpos(pl_sd,mapindex,x,y,CLR_TELEPORT);
		break;
		}
	}
	mapit_free(iter);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Force Heal a player (hp and sp)
 *------------------------------------------*/
BUILDIN_FUNC(heal)
{
	TBL_PC *sd;
	int hp,sp;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;

	hp=script_getnum(st,2);
	sp=script_getnum(st,3);
	status_heal(&sd->bl, hp, sp, 1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Force Heal a player (ap)
 *------------------------------------------*/
BUILDIN_FUNC(healap)
{
	map_session_data* sd;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	status_heal(&sd->bl, 0, 0, script_getnum(st, 2), 1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Heal a player by item (get vit bonus etc)
 *------------------------------------------*/
BUILDIN_FUNC(itemheal)
{
	TBL_PC *sd;
	int hp,sp;

	hp=script_getnum(st,2);
	sp=script_getnum(st,3);

	if(potion_flag==1) {
		potion_hp = hp;
		potion_sp = sp;
		return SCRIPT_CMD_SUCCESS;
	}

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;

	pc_itemheal(sd,sd->itemid,hp,sp);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(percentheal)
{
	int hp,sp;
	TBL_PC* sd;

	hp=script_getnum(st,2);
	sp=script_getnum(st,3);

	if(potion_flag==1) {
		potion_per_hp = hp;
		potion_per_sp = sp;
		return SCRIPT_CMD_SUCCESS;
	}

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;

#ifdef RENEWAL
	if( sd->sc.data[SC_EXTREMITYFIST2] )
		sp = 0;
#endif

	if (sd->sc.data[SC_NORECOVER_STATE]) {
		hp = 0;
		sp = 0;
	}

	if (sd->sc.data[SC_BITESCAR])
		hp = 0;

	pc_percentheal(sd,hp,sp);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(jobchange)
{
	int job, upper=-1;

	job=script_getnum(st,2);
	if( script_hasdata(st,3) )
		upper=script_getnum(st,3);

	if (pcdb_checkid(job))
	{
		TBL_PC* sd;

		if (!script_charid2sd(4,sd))
			return SCRIPT_CMD_SUCCESS;

		pc_jobchange(sd, job, upper);
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(jobname)
{
	int class_=script_getnum(st,2);
	script_pushconststr(st, (char*)job_name(class_));
	return SCRIPT_CMD_SUCCESS;
}

/// Get input from the player.
/// For numeric inputs the value is capped to the range [min,max]. Returns 1 if
/// the value was higher than 'max', -1 if lower than 'min' and 0 otherwise.
/// For string inputs it returns 1 if the string was longer than 'max', -1 is
/// shorter than 'min' and 0 otherwise.
///
/// input(<var>{,<min>{,<max>}}) -> <int>
BUILDIN_FUNC(input)
{
	TBL_PC* sd;
	struct script_data* data;
	int64 uid;
	const char* name;
	int min;
	int max;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	data = script_getdata(st,2);
	if( !data_isreference(data) ){
		ShowError("script:input: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	uid = reference_getuid(data);
	name = reference_getname(data);
	min = (script_hasdata(st,3) ? script_getnum(st,3) : script_config.input_min_value);
	max = (script_hasdata(st,4) ? script_getnum(st,4) : script_config.input_max_value);

#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_WAIT;
#endif

	if( !sd->state.menu_or_input )
	{	// first invocation, display npc input box
		sd->state.menu_or_input = 1;
		st->state = RERUNLINE;
		if( is_string_variable(name) )
			clif_scriptinputstr(sd,st->oid);
		else
			clif_scriptinput(sd,st->oid);
	}
	else
	{	// take received text/value and store it in the designated variable
		sd->state.menu_or_input = 0;
		if( is_string_variable(name) )
		{
			int len = (int)strlen(sd->npc_str);
			set_reg_str(st, sd, uid, name, sd->npc_str, script_getref(st,2));
			script_pushint(st, (len > max ? 1 : len < min ? -1 : 0));
		}
		else
		{
			int amount = sd->npc_amount;
			set_reg_num(st, sd, uid, name, cap_value(amount,min,max), script_getref(st,2));
			script_pushint(st, (amount > max ? 1 : amount < min ? -1 : 0));
		}
		st->state = RUN;
	}
	return SCRIPT_CMD_SUCCESS;
}

// declare the copyarray method here for future reference
BUILDIN_FUNC(copyarray);

/// Sets the value of a variable.
/// The value is converted to the type of the variable.
///
/// set(<variable>,<value>{,<charid>})
BUILDIN_FUNC(setr)
{
	TBL_PC* sd = NULL;
	struct script_data* data;
	//struct script_data* datavalue;
	int64 num;
	uint8 pos = 4;
	const char* name, *command = script_getfuncname(st);
	char prefix;

	data = script_getdata(st,2);
	//datavalue = script_getdata(st,3);
	if( !data_isreference(data) )
	{
		ShowError("script:set: not a variable\n");
		script_reportdata(script_getdata(st,2));
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	num = reference_getuid(data);
	name = reference_getname(data);
	prefix = *name;

	if (!strcmp(command, "setr"))
		pos = 5;

	if (not_server_variable(prefix) && !script_charid2sd(pos,sd)) {
		ShowError("buildin_set: No player attached for player variable '%s'\n", name);
		return SCRIPT_CMD_FAILURE;
	}

#if 0
	if( data_isreference(datavalue) )
	{// the value being referenced is a variable
		const char* namevalue = reference_getname(datavalue);

		if( !not_array_variable(*namevalue) )
		{// array variable being copied into another array variable
			if( sd == NULL && not_server_variable(*namevalue) && !(sd = script_rid2sd(st)) )
			{// player must be attached in order to copy a player variable
				ShowError("script:set: no player attached for player variable '%s'\n", namevalue);
				return SCRIPT_CMD_SUCCESS;
			}

			if( is_string_variable(namevalue) != is_string_variable(name) )
			{// non-matching array value types
				ShowWarning("script:set: two array variables do not match in type.\n");
				return SCRIPT_CMD_SUCCESS;
			}

			// push the maximum number of array values to the stack
			push_val(st->stack, C_INT, SCRIPT_MAX_ARRAYSIZE);

			// call the copy array method directly
			return buildin_copyarray(st);
		}
	}
#endif

	if( !strcmp(command, "setr") && script_hasdata(st, 4) ) { // Optional argument used by post-increment/post-decrement constructs to return the previous value
		if( is_string_variable(name) )
			script_pushstrcopy(st,script_getstr(st, 4));
		else
			script_pushint(st,script_getnum64(st, 4));
	} else // Return a copy of the variable reference
		script_pushcopy(st, 2);

	if( is_string_variable(name) )
		set_reg_str( st, sd, num, name, script_getstr( st, 3 ), script_getref( st, 2 ) );
	else
		set_reg_num( st, sd, num, name, script_getnum64( st, 3 ), script_getref( st, 2 ) );

	return SCRIPT_CMD_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
/// Array variables
///

/// Sets values of an array, from the starting index.
/// ex: setarray arr[1],1,2,3;
///
/// setarray <array variable>,<value1>{,<value2>...};
BUILDIN_FUNC(setarray)
{
	struct script_data* data;
	const char* name;
	uint32 start;
	uint32 end;
	int32 id;
	int32 i;
	TBL_PC* sd = NULL;

	data = script_getdata(st, 2);
	if( !data_isreference(data) )
	{
		ShowError("script:setarray: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if( not_server_variable(*name) )
	{
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	end = start + script_lastdata(st) - 2;
	if( end > SCRIPT_MAX_ARRAYSIZE )
		end = SCRIPT_MAX_ARRAYSIZE;

	if( is_string_variable(name) )
	{// string array
		for( i = 3; start < end; ++start, ++i )
			set_reg_str( st, sd, reference_uid( id, start ), name, script_getstr( st, i ), reference_getref( data ) );
	}
	else
	{// int array
		for( i = 3; start < end; ++start, ++i )
			set_reg_num( st, sd, reference_uid( id, start ), name, script_getnum64( st, i ), reference_getref( data ) );
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Sets count values of an array, from the starting index.
/// ex: cleararray arr[0],0,1;
///
/// cleararray <array variable>,<value>,<count>;
BUILDIN_FUNC(cleararray)
{
	struct script_data* data;
	const char* name;
	uint32 start;
	uint32 end;
	int32 id;
	TBL_PC* sd = NULL;

	data = script_getdata(st, 2);
	if( !data_isreference(data) )
	{
		ShowError("script:cleararray: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if( not_server_variable(*name) )
	{
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	end = start + script_getnum(st, 4);
	if( end > SCRIPT_MAX_ARRAYSIZE )
		end = SCRIPT_MAX_ARRAYSIZE;

	if( is_string_variable( name ) ){
		const char* value = script_getstr( st, 3 );

		for( ; start < end; ++start ){
			set_reg_str( st, sd, reference_uid( id, start ), name, value, script_getref( st,2 ) );
		}
	}else{
		int64 value = script_getnum64( st, 3 );

		for( ; start < end; ++start ){
			set_reg_num( st, sd, reference_uid( id, start ), name, value, script_getref( st,2 ) );
		}
	}
	
	return SCRIPT_CMD_SUCCESS;
}

/// Copies data from one array to another.
/// ex: copyarray arr[0],arr[2],2;
///
/// copyarray <destination array variable>,<source array variable>,<count>;
BUILDIN_FUNC(copyarray)
{
	struct script_data* data1;
	struct script_data* data2;
	const char* name1;
	const char* name2;
	int32 idx1;
	int32 idx2;
	int32 id1;
	int32 id2;
	int32 i;
	uint32 count;
	TBL_PC* sd = NULL;

	data1 = script_getdata(st, 2);
	data2 = script_getdata(st, 3);
	if( !data_isreference(data1) || !data_isreference(data2) )
	{
		ShowError("script:copyarray: not a variable\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id1 = reference_getid(data1);
	id2 = reference_getid(data2);
	idx1 = reference_getindex(data1);
	idx2 = reference_getindex(data2);
	name1 = reference_getname(data1);
	name2 = reference_getname(data2);

	bool is_string = is_string_variable( name1 );

	if( is_string != is_string_variable( name2 ) ){
		ShowError("script:copyarray: type mismatch\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// data type mismatch
	}

	if( not_server_variable(*name1) || not_server_variable(*name2) )
	{
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	count = script_getnum(st, 4);
	if( count > SCRIPT_MAX_ARRAYSIZE - idx1 )
		count = SCRIPT_MAX_ARRAYSIZE - idx1;
	if( count <= 0 || (id1 == id2 && idx1 == idx2) )
		return SCRIPT_CMD_SUCCESS;// nothing to copy

	if( id1 == id2 && idx1 > idx2 ){
		// destination might be overlapping the source - copy in reverse order
		for( i = count - 1; i >= 0; --i ){
			if( is_string ){
				const char* value = get_val2_str( st, reference_uid( id2, idx2 + i ), reference_getref( data2 ) );
				set_reg_str( st, sd, reference_uid( id1, idx1 + i ), name1, value, reference_getref( data1 ) );
				// Remove stack entry from get_val2_str
				script_removetop( st, -1, 0 );
			}else{
				int64 value = get_val2_num( st, reference_uid( id2, idx2 + i ), reference_getref( data2 ) );
				set_reg_num( st, sd, reference_uid( id1, idx1 + i ), name1, value, reference_getref( data1 ) );
			}
		}
	}else{
		// normal copy
		for( i = 0; i < count; ++i ){
			if( idx2 + i < SCRIPT_MAX_ARRAYSIZE ){
				if( is_string ){
					const char* value = get_val2_str( st, reference_uid( id2, idx2 + i ), reference_getref( data2 ) );
					set_reg_str( st, sd, reference_uid( id1, idx1 + i ), name1, value, reference_getref( data1 ) );
					// Remove stack entry from get_val2_str
					script_removetop( st, -1, 0 );
				}else{
					int64 value = get_val2_num( st, reference_uid( id2, idx2 + i ), reference_getref( data2 ) );
					set_reg_num( st, sd, reference_uid( id1, idx1 + i ), name1, value, reference_getref( data1 ) );
				}
			}else{
				// out of range - assume ""/0
				clear_reg( st, sd, reference_uid( id1, idx1 + i ), name1, reference_getref( data1 ) );
			}
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the size of the array.
/// Assumes that everything before the starting index exists.
/// ex: getarraysize(arr[3])
///
/// getarraysize(<array variable>) -> <int>
BUILDIN_FUNC(getarraysize)
{
	struct script_data* data;
	const char* name;
	struct map_session_data* sd = NULL;

	data = script_getdata(st, 2);
	if( !data_isreference(data) )
	{
		ShowError("script:getarraysize: not a variable\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	name = reference_getname(data);

	if( not_server_variable(*name) ){
		if (!script_rid2sd(sd))
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	script_pushint(st, script_array_highest_key(st, sd, reference_getname(data), reference_getref(data)));
	return SCRIPT_CMD_SUCCESS;
}

int script_array_index_cmp(const void *a, const void *b) {
	return ( *(unsigned int*)a - *(unsigned int*)b );
}

/// Deletes count or all the elements in an array, from the starting index.
/// ex: deletearray arr[4],2;
///
/// deletearray <array variable>;
/// deletearray <array variable>,<count>;
BUILDIN_FUNC(deletearray)
{
	struct script_data* data;
	const char* name;
	unsigned int start, end, i;
	int id;
	TBL_PC *sd = NULL;
	struct script_array *sa = NULL;
	struct reg_db *src = NULL;

	data = script_getdata(st, 2);
	if( !data_isreference(data) ) {
		ShowError("script:deletearray: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if( not_server_variable(*name) ) {
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	if (!(src = script_array_src(st, sd, name, reference_getref(data)))) {
		ShowError("script:deletearray: not a array\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	script_array_ensure_zero(st, NULL, data->u.num, reference_getref(data));

	if ( !(sa = static_cast<script_array *>(idb_get(src->arrays, id))) ) { // non-existent array, nothing to empty
		return SCRIPT_CMD_SUCCESS;// not a variable
	}

	end = script_array_highest_key(st, sd, name, reference_getref(data));

	if( start >= end )
		return SCRIPT_CMD_SUCCESS;// nothing to free

	if( script_hasdata(st,3) ) {
		unsigned int count = script_getnum(st, 3);
		if( count > end - start )
			count = end - start;
		if( count <= 0 )
			return SCRIPT_CMD_SUCCESS;// nothing to free

		bool is_string = is_string_variable( name );

		if( end - start < sa->size ) {
			// Better to iterate directly on the array, no speed-up from using sa
			for( ; start + count < end; ++start ) {
				// Compact and overwrite
				if( is_string ){
					const char* value = get_val2_str( st, reference_uid( id, start + count ), reference_getref( data ) );
					set_reg_str( st, sd, reference_uid( id, start ), name, value, reference_getref( data ) );
					// Remove stack entry from get_val2_str
					script_removetop( st, -1, 0 );
				}else{
					int64 value = get_val2_num( st, reference_uid( id, start + count ), reference_getref( data ) );
					set_reg_num( st, sd, reference_uid( id, start ), name, value, reference_getref( data ) );
				}
			}
			for( ; start < end; start++ ) {
				// Clean up any leftovers that weren't overwritten
				clear_reg( st, sd, reference_uid( id, start ), name, reference_getref( data ) );
			}
		} else {
			// using sa to speed up
			unsigned int *list = NULL, size = 0;
			list = script_array_cpy_list(sa);
			size = sa->size;
			qsort(list, size, sizeof(unsigned int), script_array_index_cmp);
			
			ARR_FIND(0, size, i, list[i] >= start);
			
			for( ; i < size && list[i] < start + count; i++ ) {
				// Clear any entries between start and start+count, if they exist
				clear_reg( st, sd, reference_uid( id, list[i] ), name, reference_getref( data ) );
			}
			
			for( ; i < size && list[i] < end; i++ ) {
				// Move back count positions any entries between start+count to fill the gaps
				if( is_string ){
					const char* value = get_val2_str( st, reference_uid( id, list[i] ), reference_getref( data ) );
					set_reg_str( st, sd, reference_uid( id, list[i] - count ), name, value, reference_getref( data ) );
					// Remove stack entry from get_val2_str
					script_removetop( st, -1, 0 );
				}else{
					int64 value = get_val2_num( st, reference_uid( id, list[i] ), reference_getref( data ) );
					set_reg_num( st, sd, reference_uid( id, list[i] - count ), name, value, reference_getref( data ) );
				}

				// Clear their originals
				clear_reg( st, sd, reference_uid( id, list[i] ), name, reference_getref( data ) );
			}
		}
	} else {
		unsigned int *list = NULL, size = 0;
		list = script_array_cpy_list(sa);
		size = sa->size;
		
		for(i = 0; i < size; i++) {
			if( list[i] >= start ) // Less expensive than sorting it, most likely
				clear_reg( st, sd, reference_uid( id, list[i] ), name, reference_getref( data ) );
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Returns a reference to the target index of the array variable.
/// Equivalent to var[index].
///
/// getelementofarray(<array variable>,<index>) -> <variable reference>
BUILDIN_FUNC(getelementofarray)
{
	struct script_data* data;
	int32 id;
	int64 i;

	data = script_getdata(st, 2);
	if( !data_isreference(data) )
	{
		ShowError("script:getelementofarray: not a variable\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_SUCCESS;// not a variable
	}

	id = reference_getid(data);

	i = script_getnum(st, 3);
	if (i < 0 || i >= SCRIPT_MAX_ARRAYSIZE) {
		ShowWarning("script:getelementofarray: index out of range (%" PRId64 ")\n", i);
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// out of range
	}

	push_val2(st->stack, C_NAME, reference_uid(id, i), reference_getref(data));
	return SCRIPT_CMD_SUCCESS;
}

/// Return the index number of the first matching value in an array.
/// ex: inarray arr,1;
///
/// inarray <array variable>,<value>;
BUILDIN_FUNC(inarray)
{
	struct script_data *data;
	const char* name;
	int id, i, array_size;
	struct map_session_data* sd = NULL;
	struct reg_db *ref = NULL;
	data = script_getdata(st, 2);

	if (!data_isreference(data))
	{
		ShowError("buildin_inarray: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	name = reference_getname(data);
	ref = reference_getref(data);

	if (not_server_variable(*name) && !script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;

	array_size = script_array_highest_key(st, sd, name, ref) - 1;

	if (array_size < 0)
	{
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (array_size > SCRIPT_MAX_ARRAYSIZE)
	{
		ShowError("buildin_inarray: The array is too large.\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	id = reference_getid(data);

	if( is_string_variable( name ) ){
		const char* value = script_getstr( st, 3 );

		for( i = 0; i <= array_size; ++i ){
			const char* temp = get_val2_str( st, reference_uid( id, i ), ref );

			if( !strcmp( temp, value ) ){
				// Remove stack entry from get_val2_str
				script_removetop( st, -1, 0 );
				script_pushint( st, i );
				return SCRIPT_CMD_SUCCESS;
			}

			// Remove stack entry from get_val2_str
			script_removetop( st, -1, 0 );
		}
	}else{
		int64 value = script_getnum64( st, 3 );

		for( i = 0; i <= array_size; ++i ){
			int64 temp = get_val2_num( st, reference_uid( id, i ), ref );

			if( temp == value ){
				script_pushint( st, i );
				return SCRIPT_CMD_SUCCESS;
			}	
		}
	}

	script_pushint(st, -1);
	return SCRIPT_CMD_SUCCESS;
}

/// Return the number of matches in two arrays.
/// ex: countinarray arr[0],arr1[0];
///
/// countinarray <array variable>,<array variable>;
BUILDIN_FUNC(countinarray)
{
	struct script_data *data1 , *data2;
	const char* name1;
	const char* name2;
	int id1, id2, i, j, array_size1, array_size2, case_count = 0;
	struct map_session_data* sd = NULL;
	struct reg_db *ref1 = NULL, *ref2 = NULL;
	data1 = script_getdata(st, 2);
	data2 = script_getdata(st, 3);

	if (!data_isreference(data1) || !data_isreference(data2))
	{
		ShowError("buildin_countinarray: not a variable\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	name1 = reference_getname(data1);
	name2 = reference_getname(data2);
	ref1 = reference_getref(data1);
	ref2 = reference_getref(data2);

	if (not_server_variable(*name1) && not_server_variable(*name2) && !script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;

	array_size1 = script_array_highest_key(st, sd, name1, ref1) - 1;
	array_size2 = script_array_highest_key(st, sd, name2, ref2) - 1;

	if (array_size1 < 0 || array_size2 < 0)
	{
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (array_size1 > SCRIPT_MAX_ARRAYSIZE || array_size2 > SCRIPT_MAX_ARRAYSIZE)
	{
		ShowError("buildin_countinarray: The array is too large.\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	i = reference_getindex(data1);
	j = reference_getindex(data2);
	if (array_size1 < i || array_size2 < j)
	{	//To prevent unintended behavior
		ShowError("buildin_countinarray: The given index of the array is higher than the array size.\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	id1 = reference_getid(data1);
	id2 = reference_getid(data2);

	if( is_string_variable( name1 ) && is_string_variable( name2 ) ){
		for( ; i <= array_size1; ++i ){
			const char* temp1 = get_val2_str( st, reference_uid( id1, i ), ref1 );

			for( j = reference_getindex( data2 ); j <= array_size2; j++ ){
				const char* temp2 = get_val2_str( st, reference_uid( id2, j ), ref2 );

				if( !strcmp( temp1, temp2 ) ){
					case_count++;
				}

				// Remove stack entry from get_val2_str
				script_removetop( st, -1, 0 );
			}

			// Remove stack entry from get_val2_str
			script_removetop( st, -1, 0 );
		}
	}else if( !is_string_variable( name1 ) && !is_string_variable( name2 ) ){
		for( ; i <= array_size1; ++i ){
			int64 temp1 = get_val2_num( st, reference_uid( id1, i ), ref1 );

			for( j = reference_getindex( data2 ); j <= array_size2; j++ ){
				int64 temp2 = get_val2_num( st, reference_uid( id2, j ), ref2 );

				if( temp1 == temp2 ){
					case_count++;
				}
			}
		}
	}else{
		ShowError("buildin_countinarray: Arrays do not match , You can't compare an int array to a string array.\n");
		script_reportdata(data1);
		script_reportdata(data2);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, case_count);
	return SCRIPT_CMD_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
/// ...
///

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(setlook)
{
	int type,val;
	TBL_PC* sd;

	type = script_getnum(st,2);
	val = script_getnum(st,3);

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;

	pc_changelook(sd,type,val);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(changelook)
{ // As setlook but only client side
	int type,val;
	TBL_PC* sd;

	type = script_getnum(st,2);
	val = script_getnum(st,3);

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;

	switch(type) {
	case LOOK_HAIR:
		val = cap_value(val, MIN_HAIR_STYLE, MAX_HAIR_STYLE);
		break;
	case LOOK_HAIR_COLOR:
		val = cap_value(val, MIN_HAIR_COLOR, MAX_HAIR_COLOR);
		break;
	case LOOK_CLOTHES_COLOR:
		val = cap_value(val, MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);
		break;
	case LOOK_BODY2:
		val = cap_value(val, MIN_BODY_STYLE, MAX_BODY_STYLE);
		break;
	default:
		break;
	}
	clif_changelook(&sd->bl, type, val);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(cutin)
{
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	clif_cutin(sd,script_getstr(st,2),script_getnum(st,3));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(viewpoint)
{
	int type,x,y,id,color;
	TBL_PC* sd;

	if (!script_charid2sd(7, sd)) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	type=script_getnum(st,2);
	x=script_getnum(st,3);
	y=script_getnum(st,4);
	id=script_getnum(st,5);
	color=script_getnum(st,6);

	clif_viewpoint(sd,st->oid,type,x,y,id,color);

	return SCRIPT_CMD_SUCCESS;
}

static int buildin_viewpointmap_sub(block_list *bl, va_list ap) {
	int oid, type, x, y, id, color;

	oid = va_arg(ap, int);
	type = va_arg(ap, int);
	x = va_arg(ap, int);
	y = va_arg(ap, int);
	id = va_arg(ap, int);
	color = va_arg(ap, int);

	clif_viewpoint((map_session_data *)bl, oid, type, x, y, id, color);
	return 0;
}

BUILDIN_FUNC(viewpointmap) {
	const char* map = script_getstr(st, 2);
	int16 mapid = map_mapname2mapid(map);

	if (mapid < 0) {
		ShowError("buildin_viewpointmap: Unknown map name %s.\n", map);
		return SCRIPT_CMD_FAILURE;
	}

	map_foreachinmap(buildin_viewpointmap_sub, mapid, BL_PC, st->oid, script_getnum(st, 3), script_getnum(st, 4), script_getnum(st, 5), script_getnum(st, 6), script_getnum(st, 7));
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set random options for new item
 * @param st Script state
 * @param it Temporary item data
 * @param funcname Function name
 * @param x First position of random option id array from the script
 **/
static int script_getitem_randomoption(struct script_state *st, struct map_session_data* sd, struct item *it, const char *funcname, int x) {
	int i, opt_id_n;
	struct script_data *opt_id = script_getdata(st,x);
	struct script_data *opt_val = script_getdata(st,x+1);
	struct script_data *opt_param = script_getdata(st,x+2);
	const char *opt_id_var = reference_getname(opt_id);
	const char *opt_val_var = reference_getname(opt_val);
	const char *opt_param_var = reference_getname(opt_param);
	int32 opt_id_id, opt_id_idx;
	int32 opt_val_id, opt_val_idx;
	int32 opt_param_id, opt_param_idx;
	struct reg_db *opt_id_ref = NULL, *opt_val_ref = NULL, *opt_param_ref = NULL;

	// Check if the variable requires a player
	if( not_server_variable(opt_id_var[0]) && sd == nullptr ){
		// If no player is attached
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_%s: variable \"%s\" was not a server variable, but no player was attached.\n", funcname, opt_id_var );
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( !data_isreference(opt_id) || !script_array_src( st, sd, opt_id_var, reference_getref(opt_id) ) ){
		ShowError( "buildin_%s: The option id parameter is not an array.\n", funcname );
		return SCRIPT_CMD_FAILURE;
	}

	if (is_string_variable(opt_id_var)) {
		ShowError("buildin_%s: The array %s is not numeric type.\n", funcname, opt_id_var);
		return SCRIPT_CMD_FAILURE;
	}

	// Check if the variable requires a player
	if( not_server_variable(opt_val_var[0]) && sd == nullptr ){
		// If no player is attached
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_%s: variable \"%s\" was not a server variable, but no player was attached.\n", funcname, opt_val_var );
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( !data_isreference(opt_val) || !script_array_src( st, sd, opt_val_var, reference_getref(opt_val) ) ){
		ShowError( "buildin_%s: The option value parameter is not an array.\n", funcname );
		return SCRIPT_CMD_FAILURE;
	}

	if (is_string_variable(opt_val_var)) {
		ShowError("buildin_%s: The array %s is not numeric type.\n", funcname, opt_val_var);
		return SCRIPT_CMD_FAILURE;
	}

	// Check if the variable requires a player
	if( not_server_variable(opt_param_var[0]) && sd == nullptr ){
		// If no player is attached
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_%s: variable \"%s\" was not a server variable, but no player was attached.\n", funcname, opt_param_var );
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( !data_isreference(opt_param) || !script_array_src( st, sd, opt_param_var, reference_getref(opt_param) ) ){
		ShowError( "buildin_%s: The option param parameter is not an array.\n", funcname );
		return SCRIPT_CMD_FAILURE;
	}

	if (is_string_variable(opt_param_var)) {
		ShowError("buildin_%s: The array %s is not numeric type.\n", funcname, opt_param_var);
		return SCRIPT_CMD_FAILURE;
	}

	opt_id_ref = reference_getref(opt_id);
	opt_id_n = script_array_highest_key(st, sd, opt_id_var, opt_id_ref);

	opt_val_ref = reference_getref(opt_val);
	opt_param_ref = reference_getref(opt_param);

	opt_id_id = reference_getid(opt_id);
	opt_val_id = reference_getid(opt_val);
	opt_param_id = reference_getid(opt_param);

	opt_id_idx = reference_getindex(opt_id);
	opt_val_idx = reference_getindex(opt_val);
	opt_param_idx = reference_getindex(opt_param);
	
	for (i = 0; i < opt_id_n && i < MAX_ITEM_RDM_OPT; i++) {
		it->option[i].id = (short)get_val2_num( st, reference_uid( opt_id_id, opt_id_idx + i ), opt_id_ref );
		it->option[i].value = (short)get_val2_num( st, reference_uid( opt_val_id, opt_val_idx + i ), opt_val_ref );
		it->option[i].param = (char)get_val2_num( st, reference_uid( opt_param_id, opt_param_idx + i ), opt_param_ref );
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Sub function for counting items
 * @param items: Item array to search
 * @param id: Item data to search for
 * @param size: Maximum size of array
 * @param expanded: If the script command has extra arguments
 * @param random_option: If the struct command has random option arguments
 * @param st: Script state (required for random options)
 * @param sd: Player data (required for random options)
 * @param rental: Whether or not to count rental items
 * @return Total count of item being searched
 */
static int script_countitem_sub(struct item *items, std::shared_ptr<item_data> id, int size, bool expanded, bool random_option, struct script_state *st, struct map_session_data *sd = nullptr, bool rental = false) {
	nullpo_retr(-1, items);
	nullpo_retr(-1, st);

	int count = 0;

	if (!expanded) { // For non-expanded functions
		t_itemid nameid = id->nameid;

		for (int i = 0; i < size; i++) {
			item *itm = &items[i];

			if (itm == nullptr || itm->nameid == 0 || itm->amount < 1)
				continue;
			if (itm->nameid == nameid && ((rental && itm->expire_time > 0) || (!rental && itm->expire_time == 0)))
				count += itm->amount;
		}
	} else { // For expanded functions
		item it;

		memset(&it, 0, sizeof(it));

		it.nameid = id->nameid;
		it.identify = script_getnum(st,3);
		it.refine  = script_getnum(st,4);
		it.attribute = script_getnum(st,5);
		it.card[0] = script_getnum(st,6);
		it.card[1] = script_getnum(st,7);
		it.card[2] = script_getnum(st,8);
		it.card[3] = script_getnum(st,9);

		if (random_option) {
			if (!sd) {
				ShowError("buildin_countitem3: Player not attached.\n");
				return -1;
			}

			int res = script_getitem_randomoption(st, sd, &it, "countitem3", 10);

			if (res != SCRIPT_CMD_SUCCESS)
				return -1;
		}

		for (int i = 0; i < size; i++) {
			item *itm = &items[i];

			if (itm == nullptr || itm->nameid == 0 || itm->amount < 1)
				continue;
			if (itm->nameid != it.nameid || itm->identify != it.identify || itm->refine != it.refine || itm->attribute != it.attribute)
				continue;
			if ((!rental && itm->expire_time > 0) || (rental && itm->expire_time == 0))
				continue;
			if (memcmp(it.card, itm->card, sizeof(it.card)))
				continue;
			if (random_option) {
				uint8 j;

				for (j = 0; j < MAX_ITEM_RDM_OPT; j++) {
					if (itm->option[j].id != it.option[j].id || itm->option[j].value != it.option[j].value || itm->option[j].param != it.option[j].param)
						break;
				}
				if (j != MAX_ITEM_RDM_OPT)
					continue;
			}

			count += items[i].amount;
		}
	}

	return count;
}

/**
 * Returns number of items in inventory
 * countitem(<nameID>{,<accountID>})
 * countitem2(<nameID>,<Identified>,<Refine>,<Attribute>,<Card0>,<Card1>,<Card2>,<Card3>{,<accountID>}) [Lupus]
 * countitem3(<item id>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<accountID>})
 * countitem3("<item name>",<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<accountID>})
 */
BUILDIN_FUNC(countitem)
{
	TBL_PC *sd;
	char *command = (char *)script_getfuncname(st);
	int aid = 3;
	bool random_option = false;

	if (command[strlen(command)-1] == '2')
		aid = 10;
	else if (command[strlen(command)-1] == '3') {
		aid = 13;
		random_option = true;
	}

	if (!script_accid2sd(aid, sd))
		return SCRIPT_CMD_FAILURE;

	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) // item name
		id = item_db.searchname( script_getstr( st, 2 ) );
	else // item id
		id = item_db.find( script_getnum( st, 2 ) );

	if (!id) {
		ShowError("buildin_%s: Invalid item '%s'.\n", command, script_getstr(st, 2)); // returns string, regardless of what it was
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	int count = script_countitem_sub(sd->inventory.u.items_inventory, id, MAX_INVENTORY, (aid > 3) ? true : false, random_option, st, sd);
	if (count < 0) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns number of items in cart
 * cartcountitem(<nameID>{,<accountID>})
 * cartcountitem2(<nameID>,<Identified>,<Refine>,<Attribute>,<Card0>,<Card1>,<Card2>,<Card3>{,<accountID>})
 */
BUILDIN_FUNC(cartcountitem)
{
	TBL_PC *sd;
	char *command = (char *)script_getfuncname(st);
	int aid = 3;

	if (command[strlen(command) - 1] == '2')
		aid = 10;

	if (!script_accid2sd(aid, sd))
		return SCRIPT_CMD_FAILURE;

	if (!pc_iscarton(sd)) {
		ShowError("buildin_%s: Player doesn't have cart (CID:%d).\n", command, sd->status.char_id);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) // item name
		id = item_db.searchname( script_getstr( st, 2 ) );
	else // item id
		id = item_db.find( script_getnum( st, 2 ) );

	if (!id) {
		ShowError("buildin_%s: Invalid item '%s'.\n", command, script_getstr(st, 2)); // returns string, regardless of what it was
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	int count = script_countitem_sub(sd->cart.u.items_cart, id, MAX_CART, (aid > 3) ? true : false, false, st);
	if (count < 0) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns number of items in storage
 * storagecountitem(<nameID>{,<accountID>})
 * storagecountitem2(<nameID>,<Identified>,<Refine>,<Attribute>,<Card0>,<Card1>,<Card2>,<Card3>{,<accountID>})
 */
BUILDIN_FUNC(storagecountitem)
{
	TBL_PC *sd;
	char *command = (char *)script_getfuncname(st);
	int aid = 3;

	if (command[strlen(command) - 1] == '2')
		aid = 10;

	if (!script_accid2sd(aid, sd))
		return SCRIPT_CMD_FAILURE;

	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) // item name
		id = item_db.searchname( script_getstr( st, 2 ) );
	else // item id
		id = item_db.find( script_getnum( st, 2 ) );

	if (!id) {
		ShowError("buildin_%s: Invalid item '%s'.\n", command, script_getstr(st, 2)); // returns string, regardless of what it was
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (sd->state.storage_flag != 0) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	int count = script_countitem_sub(sd->storage.u.items_storage, id, MAX_STORAGE, (aid > 3) ? true : false, false, st);
	if (count < 0) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns number of items in guild storage
 * guildstoragecountitem(<nameID>{,<accountID>})
 * guildstoragecountitem2(<nameID>,<Identified>,<Refine>,<Attribute>,<Card0>,<Card1>,<Card2>,<Card3>{,<accountID>})
 */
BUILDIN_FUNC(guildstoragecountitem)
{
	TBL_PC *sd;
	char *command = (char *)script_getfuncname(st);
	int aid = 3;

	if (command[strlen(command) - 1] == '2')
		aid = 10;

	if (!script_accid2sd(aid, sd))
		return SCRIPT_CMD_FAILURE;

	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) // item name
		id = item_db.searchname( script_getstr( st, 2 ) );
	else // item id
		id = item_db.find( script_getnum( st, 2 ) );

	if (!id) {
		ShowError("buildin_%s: Invalid item '%s'.\n", command, script_getstr(st, 2)); // returns string, regardless of what it was
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	struct s_storage *gstor = guild2storage(sd->status.guild_id);

	if (!gstor || (gstor && sd->state.storage_flag != 0)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	gstor->lock = true;

	int count = script_countitem_sub(gstor->u.items_guild, id, MAX_GUILD_STORAGE, (aid > 3) ? true : false, false, st);

	storage_guild_storageclose(sd);
	gstor->lock = false;

	if (count < 0) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns number of rental items in inventory
 * rentalcountitem(<nameID>{,<accountID>})
 * rentalcountitem2(<nameID>,<Identified>,<Refine>,<Attribute>,<Card0>,<Card1>,<Card2>,<Card3>{,<accountID>})
 * rentalcountitem3(<item id>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<accountID>})
 * rentalcountitem3("<item name>",<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<accountID>})
 */
BUILDIN_FUNC(rentalcountitem)
{
	char *command = (char *)script_getfuncname(st);
	int aid = 3;
	bool random_option = false;

	if (command[strlen(command) - 1] == '2')
		aid = 10;
	else if (command[strlen(command) - 1] == '3') {
		aid = 13;
		random_option = true;
	}

	map_session_data *sd;

	if (!script_accid2sd(aid, sd))
		return SCRIPT_CMD_FAILURE;

	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) // item name
		id = item_db.searchname( script_getstr( st, 2 ) );
	else // item id
		id = item_db.find( script_getnum( st, 2 ) );

	if (!id) {
		ShowError("buildin_%s: Invalid item '%s'.\n", command, script_getstr(st, 2)); // returns string, regardless of what it was
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	int count = script_countitem_sub(sd->inventory.u.items_inventory, id, MAX_INVENTORY, (aid > 3) ? true : false, random_option, st, sd, true);
	if (count < 0) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Check if item with this amount can fit in inventory
 * Checking : weight, stack amount >(MAX_AMOUNT), slots amount >(MAX_INVENTORY)
 * Return
 *	0 : fail
 *	1 : success (npc side only)
 *------------------------------------------*/
BUILDIN_FUNC(checkweight)
{
	int slots = 0;
	unsigned short amount2 = 0;
	unsigned int weight = 0, i, nbargs;
	std::shared_ptr<item_data> id;
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	nbargs = script_lastdata(st)+1;
	if(nbargs%2) {
		ShowError("buildin_checkweight: Invalid nb of args should be a multiple of 2.\n");
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}
	slots = pc_inventoryblank(sd); //nb of empty slot

	for (i = 2; i < nbargs; i += 2) {
		t_itemid nameid;
		unsigned short amount;

		if( script_isstring(st, i) ) // item name
			id = item_db.searchname( script_getstr( st, i ) );
		else // item id
			id = item_db.find( script_getnum( st, i ) );
		if( id == nullptr ){
			ShowError("buildin_checkweight: Invalid item '%s'.\n", script_getstr(st,i));  // returns string, regardless of what it was
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
		nameid = id->nameid;

		amount = script_getnum(st,i+1);
		if( amount < 1 ) {
			ShowError("buildin_checkweight: Invalid amount '%d'.\n", amount);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}

		weight += itemdb_weight(nameid)*amount; //total weight for all chk
		if( weight + sd->weight > sd->max_weight )
		{// too heavy
			script_pushint(st,0);
			return SCRIPT_CMD_SUCCESS;
		}

		switch( pc_checkadditem(sd, nameid, amount) ) {
			case CHKADDITEM_EXIST:
				// item is already in inventory, but there is still space for the requested amount
				break;
			case CHKADDITEM_NEW:
				if( itemdb_isstackable(nameid) ) {
					// stackable
					amount2++;
					if( slots < amount2 ) {
						script_pushint(st,0);
						return SCRIPT_CMD_SUCCESS;
					}
				} else {
					// non-stackable
					amount2 += amount;
					if( slots < amount2) {
						script_pushint(st,0);
						return SCRIPT_CMD_SUCCESS;
					}
				}
				break;
			case CHKADDITEM_OVERAMOUNT:
				script_pushint(st,0);
				return SCRIPT_CMD_SUCCESS;
		}
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(checkweight2)
{
	//variable sub checkweight
	int i = 0, slots = 0, weight = 0;
	short fail = 0;
	unsigned short amount2 = 0;

	//variable for array parsing
	struct script_data* data_it;
	struct script_data* data_nb;
	const char* name_it;
	const char* name_nb;
	int32 id_it, id_nb;
	int32 idx_it, idx_nb;
	int nb_it, nb_nb; //array size

	TBL_PC *sd;
	
	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	data_it = script_getdata(st, 2);
	data_nb = script_getdata(st, 3);

	if( !data_isreference(data_it) || !data_isreference(data_nb)) {
		ShowError("buildin_checkweight2: parameter not a variable\n");
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id_it = reference_getid(data_it);
	id_nb = reference_getid(data_nb);
	idx_it = reference_getindex(data_it);
	idx_nb = reference_getindex(data_nb);
	name_it = reference_getname(data_it);
	name_nb = reference_getname(data_nb);

	if(is_string_variable(name_it) || is_string_variable(name_nb)) {
		ShowError("buildin_checkweight2: illegal type, need int\n");
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;// not supported
	}
	nb_it = script_array_highest_key(st, sd, reference_getname(data_it), reference_getref(data_it));
	nb_nb = script_array_highest_key(st, sd, reference_getname(data_nb), reference_getref(data_nb));
	if(nb_it != nb_nb) {
		ShowError("buildin_checkweight2: Size mistmatch: nb_it=%d, nb_nb=%d\n",nb_it,nb_nb);
		fail = 1;
	}

	slots = pc_inventoryblank(sd);
	for(i=0; i<nb_it; i++) {
		t_itemid nameid = (t_itemid)get_val2_num( st, reference_uid( id_it, idx_it + i ), reference_getref( data_it ) );
		uint16 amount = (uint16)get_val2_num( st, reference_uid( id_nb, idx_nb + i ), reference_getref( data_nb ) );

		if(fail)
			continue; //cpntonie to depop rest

		if(itemdb_exists(nameid) == NULL ) {
			ShowError("buildin_checkweight2: Invalid item '%u'.\n", nameid);
			fail=1;
			continue;
		}
		if(amount == 0 ) {
			ShowError("buildin_checkweight2: Invalid amount '%d'.\n", amount);
			fail = 1;
			continue;
		}

		weight += itemdb_weight(nameid)*amount;
		if( weight + sd->weight > sd->max_weight ) {
			fail = 1;
			continue;
		}
		switch( pc_checkadditem(sd, nameid, amount) ) {
			case CHKADDITEM_EXIST:
				// item is already in inventory, but there is still space for the requested amount
				break;
			case CHKADDITEM_NEW:
				if( itemdb_isstackable(nameid) ) {// stackable
					amount2++;
					if( slots < amount2 )
						fail = 1;
				} else {// non-stackable
					amount2 += amount;
					if( slots < amount2 ) {
						fail = 1;
					}
				}
				break;
			case CHKADDITEM_OVERAMOUNT:
				fail = 1;
		} //end switch
	} //end loop DO NOT break it prematurly we need to depop all stack

	fail ? script_pushint(st,0) : script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getitem <item id>,<amount>{,<account ID>};
 * getitem "<item name>",<amount>{,<account ID>};
 *
 * getitembound <item id>,<amount>,<type>{,<account ID>};
 * getitembound "<item id>",<amount>,<type>{,<account ID>};
 * Type:
 *	0 - No bound
 *	1 - Account Bound
 *	2 - Guild Bound
 *	3 - Party Bound
 *	4 - Character Bound
 *------------------------------------------*/
BUILDIN_FUNC(getitem)
{
	int get_count, i;
	t_itemid nameid;
	unsigned short amount;
	struct item it;
	struct map_session_data *sd;
	unsigned char flag = 0;
	const char* command = script_getfuncname(st);
	std::shared_ptr<item_data> id;

	if( script_isstring(st, 2) ) {// "<item name>"
		const char *name = script_getstr(st, 2);

		id = item_db.searchname( name );

		if( id == nullptr ){
			ShowError("buildin_getitem: Nonexistant item %s requested.\n", name);
			return SCRIPT_CMD_FAILURE; //No item created.
		}
		nameid = id->nameid;
	} else {// <item id>
		nameid = script_getnum(st, 2);

		id = item_db.find( nameid );

		if( id == nullptr ){
			ShowError("buildin_getitem: Nonexistant item %u requested.\n", nameid);
			return SCRIPT_CMD_FAILURE; //No item created.
		}
	}

	// <amount>
	if( (amount = script_getnum(st,3)) <= 0)
		return SCRIPT_CMD_SUCCESS; //return if amount <=0, skip the useles iteration

	memset(&it,0,sizeof(it));
	it.nameid = nameid;
	it.identify = 1;
	it.bound = BOUND_NONE;

	if( !strcmp(command,"getitembound") ) {
		char bound = script_getnum(st,4);
		if( bound < BOUND_NONE || bound >= BOUND_MAX ) {
			ShowError("script_getitembound: Not a correct bound type! Type=%d\n",bound);
			return SCRIPT_CMD_FAILURE;
		}
		script_mapid2sd(5,sd);
		it.bound = bound;
	} else {
		script_mapid2sd(4,sd);
	}

	if( sd == NULL ) // no target
		return SCRIPT_CMD_SUCCESS;

	//Check if it's stackable.
	if( !itemdb_isstackable2( id.get() ) ){
		get_count = 1;
	}else{
		get_count = amount;
	}

	for (i = 0; i < amount; i += get_count)
	{
		// if not pet egg
		if (!pet_create_egg(sd, nameid))
		{
			if ((flag = pc_additem(sd, &it, get_count, LOG_TYPE_SCRIPT)))
			{
				clif_additem(sd, 0, 0, flag);
				if( pc_candrop(sd,&it) )
					map_addflooritem(&it,get_count,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
			}
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getitem2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>};
 * getitem2 "<item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>};
 *
 * getitembound2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<bound type>{,<account ID>};
 * getitembound2 "<item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<bound type>{,<account ID>};
 *
 * getitem3 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
 * getitem3 "<item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
 *
 * getitembound3 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<bound type>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
 * getitembound3 "<item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<bound type>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
 * Type:
 *	0 - No bound
 *	1 - Account Bound
 *	2 - Guild Bound
 *	3 - Party Bound
 *	4 - Character Bound
 *------------------------------------------*/
BUILDIN_FUNC(getitem2)
{
	t_itemid nameid;
	unsigned short amount;
	int iden, ref, attr;
	t_itemid c1, c2, c3, c4;
	char bound = BOUND_NONE;
	std::shared_ptr<item_data> item_data;
	struct item item_tmp;
	TBL_PC *sd;
	const char* command = script_getfuncname(st);
	int offset = 0;

#ifdef Pandas_ScriptCommand_GetGradeItem
	if (strcmpi(command, "getgradeitem") == 0) {
		offset = 12;
		script_mapid2sd(15, sd);
	}
	else
#endif // Pandas_ScriptCommand_GetGradeItem
	if( !strncmp(command,"getitembound",12) ) {
		int aid_pos = 12;
		bound = script_getnum(st,11);
		if( bound < BOUND_NONE || bound >= BOUND_MAX ) {
			ShowError("script_getitembound2: Not a correct bound type! Type=%d\n",bound);
			return SCRIPT_CMD_FAILURE;
		}
		if (command[strlen(command)-1] == '3') {
			offset = 12;
			aid_pos = 15;
		}
		script_mapid2sd(aid_pos,sd);
	} else {
		int aid_pos = 11;
		if (strcmpi(command,"getitem3") == 0) {
			offset = 11;
			aid_pos = 14;
		} 
		script_mapid2sd(aid_pos,sd);
	}

	if( sd == NULL ) // no target
		return SCRIPT_CMD_SUCCESS;

	if( script_isstring(st, 2) ) {
		const char *name = script_getstr(st, 2);

		item_data = item_db.searchname( name );

		if( item_data == nullptr ){
			ShowError("buildin_getitem2: Nonexistant item %s requested (by conv_str).\n", name);
			return SCRIPT_CMD_FAILURE; //No item created.
		}
		nameid = item_data->nameid;
	} else {
		nameid = script_getnum(st, 2);

		item_data = item_db.find( nameid );

		if( item_data == nullptr ){
			ShowError("buildin_getitem2: Nonexistant item %u requested (by conv_num).\n", nameid);
			return SCRIPT_CMD_FAILURE; //No item created.
		}
	}

	amount = script_getnum(st,3);
	iden = script_getnum(st,4);
	ref = script_getnum(st,5);
	attr = script_getnum(st,6);
	c1 = script_getnum(st,7);
	c2 = script_getnum(st,8);
	c3 = script_getnum(st,9);
	c4 = script_getnum(st,10);

	if( item_data ) {
		int get_count = 0, i;
		memset(&item_tmp,0,sizeof(item_tmp));
		if( item_data->type == IT_WEAPON || item_data->type == IT_ARMOR || item_data->type == IT_SHADOWGEAR ) {
			if(ref > MAX_REFINE)
				ref = MAX_REFINE;
		}
		else if( item_data->type == IT_PETEGG ) {
			iden = 1;
			ref = 0;
		}
		else {
			iden = 1;
			ref = attr = 0;
		}

		item_tmp.nameid = nameid;
		item_tmp.identify = iden;
#ifdef Pandas_BattleConfig_Force_Identified
		item_tmp.identify = (battle_config.force_identified & 128 ? 1 : item_tmp.identify);
#endif // Pandas_BattleConfig_Force_Identified
		item_tmp.refine = ref;
		item_tmp.attribute = attr;
		item_tmp.card[0] = c1;
		item_tmp.card[1] = c2;
		item_tmp.card[2] = c3;
		item_tmp.card[3] = c4;
		item_tmp.bound = bound;

#ifdef Pandas_ScriptCommand_GetGradeItem
		if (strcmpi(command, "getgradeitem") == 0) {
			item_tmp.enchantgrade = cap_value(script_getnum(st, offset-1), 0, MAX_ENCHANTGRADE);
		}
#endif // Pandas_ScriptCommand_GetGradeItem

		if (offset != 0) {
			int res = script_getitem_randomoption(st, sd, &item_tmp, command, offset);
			if (res == SCRIPT_CMD_FAILURE)
				return SCRIPT_CMD_FAILURE;
		}

		//Check if it's stackable.
		if( !itemdb_isstackable2( item_data.get() ) ){
			get_count = 1;
		}else{
			get_count = amount;
		}

		for (i = 0; i < amount; i += get_count)
		{
			// if not pet egg
			if (!pet_create_egg(sd, nameid))
			{
				unsigned char flag = 0;
				if ((flag = pc_additem(sd, &item_tmp, get_count, LOG_TYPE_SCRIPT)))
				{
					clif_additem(sd, 0, 0, flag);
					if( pc_candrop(sd,&item_tmp) )
						map_addflooritem(&item_tmp,get_count,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
				}
			}
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/** Gives rental item to player
 * rentitem <item id>,<seconds>{,<account_id>}
 * rentitem "<item name>",<seconds>{,<account_id>}
 */
BUILDIN_FUNC(rentitem) {
	struct map_session_data *sd;
	struct item it;
	int seconds;
	t_itemid nameid = 0;
	unsigned char flag = 0;

	if (!script_accid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_isstring(st, 2) )
	{
		const char *name = script_getstr(st, 2);
		std::shared_ptr<item_data> itd = item_db.searchname( name );

		if( itd == nullptr ){
			ShowError("buildin_rentitem: Nonexistant item %s requested.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
		nameid = itd->nameid;
	}
	else
	{
		nameid = script_getnum(st, 2);
		if( nameid == 0 || !itemdb_exists(nameid) )
		{
			ShowError("buildin_rentitem: Nonexistant item %u requested.\n", nameid);
			return SCRIPT_CMD_FAILURE;
		}
	}

	seconds = script_getnum(st,3);
	memset(&it, 0, sizeof(it));
	it.nameid = nameid;
	it.identify = 1;
	it.expire_time = (unsigned int)(time(NULL) + seconds);
	it.bound = BOUND_NONE;

	if( (flag = pc_additem(sd, &it, 1, LOG_TYPE_SCRIPT)) )
	{
		clif_additem(sd, 0, 0, flag);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Gives rental item to player with advanced option
 * rentitem2 <item id>,<time>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account_id>};
 * rentitem2 "<item name>",<time>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account_id>};
 *
 * rentitem3 <item id>,<time>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account_id>};
 * rentitem3 "<item name>",<time>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account_id>};
 */
BUILDIN_FUNC(rentitem2) {
	struct map_session_data *sd;
	struct item it;
	std::shared_ptr<item_data> id;
	int seconds;
	t_itemid nameid = 0;
	unsigned char flag = 0;
	int iden,ref,attr;
	t_itemid c1,c2,c3,c4;
	const char *funcname = script_getfuncname(st);

	if (funcname[strlen(funcname)-1] == '3') {
		if (!script_accid2sd(14,sd))
			return SCRIPT_CMD_FAILURE;
	} else if (!script_accid2sd(11,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_isstring(st, 2) ) {
		const char *name = script_getstr(st, 2);

		id = item_db.searchname( name );

		if( id == nullptr ) {
			ShowError("buildin_rentitem2: Nonexistant item %s requested.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
		nameid = id->nameid;
	} else {
		nameid = script_getnum(st, 2);

		id = item_db.find( nameid );

		if( id == nullptr ){
			ShowError("buildin_rentitem2: Nonexistant item %u requested.\n", nameid);
			return SCRIPT_CMD_FAILURE;
		}
	}
	
	seconds = script_getnum(st,3);
	iden = script_getnum(st,4);
	ref = script_getnum(st,5);
	attr = script_getnum(st,6);

	if (id->type==IT_WEAPON || id->type==IT_ARMOR || id->type==IT_SHADOWGEAR) {
		if(ref > MAX_REFINE) ref = MAX_REFINE;
	}
	else if (id->type==IT_PETEGG) {
		iden = 1;
		ref = 0;
	}
	else {
		iden = 1;
		ref = attr = 0;
	}

	c1 = script_getnum(st,7);
	c2 = script_getnum(st,8);
	c3 = script_getnum(st,9);
	c4 = script_getnum(st,10);

	memset(&it, 0, sizeof(it));
	it.nameid = nameid;
	it.identify = iden;
#ifdef Pandas_BattleConfig_Force_Identified
	it.identify = (battle_config.force_identified & 256 ? 1 : it.identify);
#endif // Pandas_BattleConfig_Force_Identified
	it.refine = ref;
	it.attribute = attr;
	it.card[0] = c1;
	it.card[1] = c2;
	it.card[2] = c3;
	it.card[3] = c4;
	it.expire_time = (unsigned int)(time(NULL) + seconds);
	it.bound = BOUND_NONE;

	if (funcname[strlen(funcname)-1] == '3') {
		int res = script_getitem_randomoption(st, sd, &it, funcname, 11);
		if (res != SCRIPT_CMD_SUCCESS)
			return res;
	}

	if( (flag = pc_additem(sd, &it, 1, LOG_TYPE_SCRIPT)) ) {
		clif_additem(sd, 0, 0, flag);
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * gets an item with someone's name inscribed [Skotlex]
 * getinscribeditem item_num, character_name
 * Returned Qty is always 1, only works on equip-able
 * equipment
 *------------------------------------------*/
BUILDIN_FUNC(getnameditem)
{
	t_itemid nameid;
	struct item item_tmp;
	TBL_PC *sd, *tsd;

	if (!script_rid2sd(sd))
	{	//Player not attached!
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if( script_isstring(st, 2) ){
		const char *name = script_getstr(st, 2);
		std::shared_ptr<item_data> item_data = item_db.searchname( name );

		// Failed
		if( item_data == nullptr){
			script_pushint(st,0);
			return SCRIPT_CMD_SUCCESS;
		}
		nameid = item_data->nameid;
	}else
		nameid = script_getnum(st, 2);

	if(!itemdb_exists(nameid)/* || itemdb_isstackable(nameid)*/)
	{	//Even though named stackable items "could" be risky, they are required for certain quests.
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if( script_isstring(st, 3) )	//Char Name
		tsd = map_nick2sd(script_getstr(st, 3),false);
	else	//Char Id was given
		tsd = map_charid2sd(script_getnum(st, 3));

	if( tsd == NULL )
	{	//Failed
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	memset(&item_tmp,0,sizeof(item_tmp));
	item_tmp.nameid=nameid;
	item_tmp.amount=1;
	item_tmp.identify=1;
	item_tmp.card[0]=CARD0_CREATE; //we don't use 255! because for example SIGNED WEAPON shouldn't get TOP10 BS Fame bonus [Lupus]
	item_tmp.card[2]=GetWord(tsd->status.char_id,0);
	item_tmp.card[3]=GetWord(tsd->status.char_id,1);
	if(pc_additem(sd,&item_tmp,1,LOG_TYPE_SCRIPT)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;	//Failed to add item, we will not drop if they don't fit
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * gets a random item ID from an item group [Skotlex]
 * groupranditem <group_num>{,<sub_group>};
 *------------------------------------------*/
BUILDIN_FUNC(grouprandomitem) {
	int sub_group = 1;

	FETCH(3, sub_group);
	std::shared_ptr<s_item_group_entry> entry = itemdb_group.get_random_entry(script_getnum(st,2),sub_group);
	if (!entry) {
		ShowError("buildin_grouprandomitem: Invalid item group with group_id '%d', sub_group '%d'.\n", script_getnum(st,2), sub_group);
		script_pushint(st,UNKNOWN_ITEM_ID);
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st,entry->nameid);
	return SCRIPT_CMD_SUCCESS;
}

/**
* makeitem <item id>,<amount>,"<map name>",<X>,<Y>{,<canShowEffect>};
* makeitem "<item name>",<amount>,"<map name>",<X>,<Y>{,<canShowEffect>};
*/
BUILDIN_FUNC(makeitem) {
	t_itemid nameid;
	uint16 amount, flag = 0, x, y;
	const char *mapname;
	int m;
	struct item item_tmp;
	bool canShowEffect = false;

	if( script_isstring(st, 2) ){
		const char *name = script_getstr(st, 2);
		std::shared_ptr<item_data> item_data = item_db.searchname( name );

		if( item_data )
			nameid = item_data->nameid;
		else{
			ShowError( "buildin_makeitem: Unknown item %s\n", name );
			return SCRIPT_CMD_FAILURE;
		}
	}
	else {
		int32 val = script_getnum( st, 2 );

		if( val < 0 ){
			flag = 1;
			nameid = (t_itemid)( -1 * val );
		}else{
			nameid = (t_itemid)val;
		}

		if( !itemdb_exists( nameid ) ){
			ShowError( "buildin_makeitem: Unknown item id %u\n", nameid );
			return SCRIPT_CMD_FAILURE;
		}
	}

	amount = script_getnum(st,3);
	mapname	= script_getstr(st,4);
	x = script_getnum(st,5);
	y = script_getnum(st,6);

	if (script_hasdata(st, 7))
		canShowEffect = script_getnum(st, 7) != 0;

	if (strcmp(mapname, "this")==0) {
		TBL_PC *sd;
		if (!script_rid2sd(sd))
			return SCRIPT_CMD_SUCCESS; //Failed...
		m = sd->bl.m;
	} else
		m = map_mapname2mapid(mapname);

	memset(&item_tmp,0,sizeof(item_tmp));
	item_tmp.nameid = nameid;
	if (!flag)
		item_tmp.identify = 1;
	else
		item_tmp.identify = itemdb_isidentified(nameid);
#ifdef Pandas_BattleConfig_Force_Identified
		item_tmp.identify = (battle_config.force_identified & 32 ? 1 : item_tmp.identify);
#endif // Pandas_BattleConfig_Force_Identified

#ifdef Pandas_ScriptCommand_Next_Dropitem_Special
	if (next_dropitem_special.bound != -1) {
		item_tmp.bound = cap_value(next_dropitem_special.bound, BOUND_NONE, BOUND_MAX - 1);
		next_dropitem_special.bound = -1;
	}
	if (next_dropitem_special.rent_duration != 0) {
		item_tmp.expire_time = (unsigned int)(time(NULL) + next_dropitem_special.rent_duration);
		next_dropitem_special.rent_duration = 0;
	}
	// 提示: 在这里无需处理 next_dropitem_special.drop_effect,
	// 这部分放在了 clif_dropflooritem 进行, 底部的 map_addflooritem 最终会调用它
#endif // Pandas_ScriptCommand_Next_Dropitem_Special

	map_addflooritem(&item_tmp, amount, m, x, y, 0, 0, 0, 4, 0, canShowEffect);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * makeitem2 <item id>,<amount>,"<map name>",<X>,<Y>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<canShowEffect>};
 * makeitem2 "<item name>",<amount>,"<map name>",<X>,<Y>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<canShowEffect>};
 *
 * makeitem3 <item id>,<amount>,"<map name>",<X>,<Y>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<canShowEffect>};
 * makeitem3 "<item name>",<amount>,"<map name>",<X>,<Y>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<canShowEffect>};
 */
BUILDIN_FUNC(makeitem2) {
	t_itemid nameid;
	uint16 amount, x, y;
	const char *mapname;
	int m;
	struct item item_tmp;
	struct item_data *id;
	const char *funcname = script_getfuncname(st);
	bool canShowEffect = false;

	if( script_isstring( st, 2 ) ){
		const char *name = script_getstr( st, 2 );
		std::shared_ptr<item_data> item_data = item_db.searchname( name );

		if( item_data ){
			nameid = item_data->nameid;
		}else{
			ShowError( "buildin_%s: Unknown item %s\n", funcname, name );
			return SCRIPT_CMD_FAILURE;
		}
	}else{
		nameid = (t_itemid)script_getnum( st, 2 );

		if( !itemdb_exists( nameid ) ){
			ShowError( "buildin_%s: Unknown item id %u\n", funcname, nameid );
			return SCRIPT_CMD_FAILURE;
		}
	}

	amount = script_getnum(st,3);
	mapname	= script_getstr(st,4);
	x = script_getnum(st,5);
	y = script_getnum(st,6);

	if (strcmp(mapname,"this")==0) {
		TBL_PC *sd;
		if (!script_rid2sd(sd))
			return SCRIPT_CMD_SUCCESS; //Failed...
		m = sd->bl.m;
	}
	else
		m = map_mapname2mapid(mapname);
	
	if ((id = itemdb_search(nameid))) {
		char iden, ref, attr;
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid = nameid;

		iden = (char)script_getnum(st,7);
		ref = (char)script_getnum(st,8);
		attr = (char)script_getnum(st,9);		

		if (id->type==IT_WEAPON || id->type==IT_ARMOR || id->type==IT_SHADOWGEAR) {
			if(ref > MAX_REFINE) ref = MAX_REFINE;
		}
		else if (id->type==IT_PETEGG) {
			iden = 1;
			ref = 0;
		}
		else {
			iden = 1;
			ref = attr = 0;
		}
		
		item_tmp.identify = iden;
#ifdef Pandas_BattleConfig_Force_Identified
		item_tmp.identify = (battle_config.force_identified & 32 ? 1 : item_tmp.identify);
#endif // Pandas_BattleConfig_Force_Identified
		item_tmp.refine = ref;
		item_tmp.attribute = attr;
		item_tmp.card[0] = script_getnum(st,10);
		item_tmp.card[1] = script_getnum(st,11);
		item_tmp.card[2] = script_getnum(st,12);
		item_tmp.card[3] = script_getnum(st,13);

#ifdef Pandas_ScriptCommand_Next_Dropitem_Special
		if (next_dropitem_special.bound != -1) {
			item_tmp.bound = cap_value(next_dropitem_special.bound, BOUND_NONE, BOUND_MAX - 1);
			next_dropitem_special.bound = -1;
		}
		if (next_dropitem_special.rent_duration != 0) {
			item_tmp.expire_time = (unsigned int)(time(NULL) + next_dropitem_special.rent_duration);
			next_dropitem_special.rent_duration = 0;
		}
		// 提示: 在这里无需处理 next_dropitem_special.drop_effect,
		// 这部分放在了 clif_dropflooritem 进行, 底部的 map_addflooritem 最终会调用它
#endif // Pandas_ScriptCommand_Next_Dropitem_Special

		if (funcname[strlen(funcname)-1] == '3') {
			int res = script_getitem_randomoption(st, nullptr, &item_tmp, funcname, 14);
			if (res != SCRIPT_CMD_SUCCESS)
				return res;

			if (script_hasdata(st, 17)) {
				if (script_getnum(st, 17) != 0)
					canShowEffect = script_getnum(st, 17) != 0;
			}
		}
		else {
			if (script_hasdata(st, 14)) {
				if (script_getnum(st, 14) != 0)
					canShowEffect = script_getnum(st, 14) != 0;
			}
		}

		map_addflooritem(&item_tmp, amount, m, x, y, 0, 0, 0, 4, 0, canShowEffect);
	}
	else
		return SCRIPT_CMD_FAILURE;
	return SCRIPT_CMD_SUCCESS;
}

/// Counts / deletes the current item given by idx.
/// Used by buildin_delitem_search
/// Relies on all input data being already fully valid.
static void buildin_delitem_delete(struct map_session_data* sd, int idx, int* amount, uint8 loc, bool delete_items)
{
	int delamount;
	struct item *itm = NULL;
	struct s_storage *gstor = NULL;

	switch(loc) {
		case TABLE_CART:
			itm = &sd->cart.u.items_cart[idx];
			break;
		case TABLE_STORAGE:
			itm = &sd->storage.u.items_storage[idx];
			break;
		case TABLE_GUILD_STORAGE:
		{
			gstor = guild2storage2(sd->status.guild_id);

			itm = &gstor->u.items_guild[idx];
		}
			break;
		default: // TABLE_INVENTORY
			itm = &sd->inventory.u.items_inventory[idx];
			break;
	}

	delamount = ( amount[0] < itm->amount ) ? amount[0] : itm->amount;

	if( delete_items )
	{
		if( itemdb_type(itm->nameid) == IT_PETEGG && itm->card[0] == CARD0_PET )
		{// delete associated pet
			intif_delete_petdata(MakeDWord(itm->card[1], itm->card[2]));
		}
		switch(loc) {
			case TABLE_CART:
				pc_cart_delitem(sd,idx,delamount,0,LOG_TYPE_SCRIPT);
				break;
			case TABLE_STORAGE:
				storage_delitem(sd,&sd->storage,idx,delamount);
				log_pick_pc(sd,LOG_TYPE_SCRIPT,-delamount,itm);
				break;
			case TABLE_GUILD_STORAGE:
				gstor->lock = true;
				storage_guild_delitem(sd, gstor, idx, delamount);
				log_pick_pc(sd, LOG_TYPE_SCRIPT, -delamount, itm);
				storage_guild_storageclose(sd);
				gstor->lock = false;
				break;
			default: // TABLE_INVENTORY
				pc_delitem(sd, idx, delamount, 0, 0, LOG_TYPE_SCRIPT);
				break;
		}
	}

	amount[0]-= delamount;
}


/// Searches for item(s) and checks, if there is enough of them.
/// Used by delitem and delitem2
/// Relies on all input data being already fully valid.
/// @param exact_match will also match item by specified attributes
///   0x0: Only item id
///   0x1: identify, attributes, cards
///   0x2: random option
/// @return true when all items could be deleted, false when there were not enough items to delete
static bool buildin_delitem_search(struct map_session_data* sd, struct item* it, uint8 exact_match, uint8 loc)
{
	bool delete_items = false;
	int i, amount, size;
	struct item *items;

	// prefer always non-equipped items
	it->equip = 0;

	// when searching for nameid only, prefer additionally
	if( !exact_match )
	{
		// non-refined items
		it->refine = 0;
		// card-less items
		memset(it->card, 0, sizeof(it->card));
	}

	switch(loc) {
		case TABLE_CART:
			size = MAX_CART;
			items = sd->cart.u.items_cart;
			break;
		case TABLE_STORAGE:
			size = MAX_STORAGE;
			items = sd->storage.u.items_storage;
			break;
		case TABLE_GUILD_STORAGE:
		{
			struct s_storage *gstor = guild2storage2(sd->status.guild_id);

			size = MAX_GUILD_STORAGE;
			items = gstor->u.items_guild;
		}
			break;
		default: // TABLE_INVENTORY
			size = MAX_INVENTORY;
			items = sd->inventory.u.items_inventory;
			break;
	}

	for(;;)
	{
		unsigned short important = 0;
		amount = it->amount;

		// 1st pass -- less important items / exact match
		for( i = 0; amount && i < size; i++ )
		{
			struct item *itm = NULL;

			if( !&items[i] || !(itm = &items[i])->nameid || itm->nameid != it->nameid )
			{// wrong/invalid item
				continue;
			}

			if( itm->equip != it->equip || itm->refine != it->refine )
			{// not matching attributes
				important++;
				continue;
			}

			if( exact_match )
			{
				if( (exact_match&0x1) && ( itm->identify != it->identify || itm->attribute != it->attribute || memcmp(itm->card, it->card, sizeof(itm->card)) ) )
				{// not matching exact attributes
					continue;
				}
				if (exact_match&0x2) {
					uint8 j;
					for (j = 0; j < MAX_ITEM_RDM_OPT; j++) {
						if (itm->option[j].id != it->option[j].id || itm->option[j].value != it->option[j].value || itm->option[j].param != it->option[j].param)
							break;
					}
					if (j != MAX_ITEM_RDM_OPT)
						continue;
				}
			}
			else
			{
				if( itemdb_type(itm->nameid) == IT_PETEGG )
				{
					if( itm->card[0] == CARD0_PET && CheckForCharServer() )
					{// pet which cannot be deleted
						continue;
					}
				}
				else if( memcmp(itm->card, it->card, sizeof(itm->card)) )
				{// named/carded item
					important++;
					continue;
				}
			}

			// count / delete item
			buildin_delitem_delete(sd, i, &amount, loc, delete_items);
		}

		// 2nd pass -- any matching item
		if( amount == 0 || important == 0 )
		{// either everything was already consumed or no items were skipped
			;
		}
		else for( i = 0; amount && i < size; i++ )
		{
			struct item *itm = NULL;

			if( !&items[i] || !(itm = &items[i])->nameid || itm->nameid != it->nameid )
			{// wrong/invalid item
				continue;
			}

			if( itemdb_type(itm->nameid) == IT_PETEGG && itm->card[0] == CARD0_PET && CheckForCharServer() )
			{// pet which cannot be deleted
				continue;
			}

			if( exact_match )
			{
				if( (exact_match&0x1) && ( itm->refine != it->refine || itm->identify != it->identify || itm->attribute != it->attribute || memcmp(itm->card, it->card, sizeof(itm->card)) ) )
				{// not matching attributes
					continue;
				}
				if (exact_match&0x2) {
					uint8 j;
					for (j = 0; j < MAX_ITEM_RDM_OPT; j++) {
						if (itm->option[j].id != it->option[j].id || itm->option[j].value != it->option[j].value || itm->option[j].param != it->option[j].param)
							break;
					}
					if (j != MAX_ITEM_RDM_OPT)
						continue;
				}
			}

			// count / delete item
			buildin_delitem_delete(sd, i, &amount, loc, delete_items);
		}

		if( amount )
		{// not enough items
			return false;
		}
		else if( delete_items )
		{// we are done with the work
			return true;
		}
		else
		{// get rid of the items now
			delete_items = true;
		}
	}
}


/// Deletes items from the target/attached player.
/// Prioritizes ordinary items.
///
/// delitem <item id>,<amount>{,<account id>}
/// delitem "<item name>",<amount>{,<account id>}
/// cartdelitem <item id>,<amount>{,<account id>}
/// cartdelitem "<item name>",<amount>{,<account id>}
/// storagedelitem <item id>,<amount>{,<account id>}
/// storagedelitem "<item name>",<amount>{,<account id>}
/// guildstoragedelitem <item id>,<amount>{,<account id>}
/// guildstoragedelitem "<item name>",<amount>{,<account id>}
BUILDIN_FUNC(delitem)
{
	TBL_PC *sd;
	struct item it;
	uint8 loc = 0;
	char* command = (char*)script_getfuncname(st);

	if(!strncmp(command, "cart", 4))
		loc = TABLE_CART;
	else if(!strncmp(command, "storage", 7))
		loc = TABLE_STORAGE;
	else if(!strncmp(command, "guildstorage", 12))
		loc = TABLE_GUILD_STORAGE;

	if( !script_accid2sd(4,sd) ){
		// In any case cancel script execution
		st->state = END;
		return SCRIPT_CMD_SUCCESS;
	}

	if (loc == TABLE_CART && !pc_iscarton(sd)) {
		ShowError("buildin_cartdelitem: player doesn't have cart (CID=%d).\n", sd->status.char_id);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (loc == TABLE_GUILD_STORAGE) {
		struct s_storage *gstor = guild2storage2(sd->status.guild_id);

		if (gstor == NULL || sd->state.storage_flag) {
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( script_isstring(st, 2) )
	{
		const char* item_name = script_getstr(st, 2);
		std::shared_ptr<item_data> id = item_db.searchname(item_name);

		if( id == nullptr ){
			ShowError("buildin_%s: unknown item \"%s\".\n", command, item_name);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
		it.nameid = id->nameid;// "<item name>"
	}
	else
	{
		it.nameid = script_getnum(st, 2);// <item id>
		if( !itemdb_exists( it.nameid ) )
		{
			ShowError("buildin_%s: unknown item \"%u\".\n", command, it.nameid);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}

	it.amount=script_getnum(st,3);

	if( it.amount <= 0 )
		return SCRIPT_CMD_SUCCESS;// nothing to do

	if( buildin_delitem_search(sd, &it, 0, loc) )
	{// success
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_%s: failed to delete %d items (AID=%d item_id=%u).\n", command, it.amount, sd->status.account_id, it.nameid);
	st->state = END;
	st->mes_active = 0;
	clif_scriptclose(sd, st->oid);
	return SCRIPT_CMD_FAILURE;
}

/// Deletes items from the target/attached player.
///
/// delitem2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// delitem2 "<Item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// cartdelitem2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// cartdelitem2 "<Item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// storagedelitem2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// storagedelitem2 "<Item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// guildstoragedelitem2 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// guildstoragedelitem2 "<Item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>{,<account ID>}
/// delitem3 <item id>,<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
/// delitem3 "<item name>",<amount>,<identify>,<refine>,<attribute>,<card1>,<card2>,<card3>,<card4>,<RandomIDArray>,<RandomValueArray>,<RandomParamArray>{,<account ID>};
BUILDIN_FUNC(delitem2)
{
	TBL_PC *sd;
	struct item it;
	uint8 loc = 0;
	char* command = (char*)script_getfuncname(st);
	int aid_pos = 11;
	uint8 flag = 0x1;

	if(!strncmp(command, "cart", 4))
		loc = TABLE_CART;
	else if(!strncmp(command, "storage", 7))
		loc = TABLE_STORAGE;
	else if(!strncmp(command, "guildstorage", 12))
		loc = TABLE_GUILD_STORAGE;

	if (command[strlen(command)-1] == '3')
		aid_pos = 14;

	if( !script_accid2sd(aid_pos,sd) ){
		// In any case cancel script execution
		st->state = END;
		return SCRIPT_CMD_SUCCESS;
	}

	if (loc == TABLE_CART && !pc_iscarton(sd)) {
		ShowError("buildin_cartdelitem: player doesn't have cart (CID=%d).\n", sd->status.char_id);
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}
	if (loc == TABLE_GUILD_STORAGE) {
		struct s_storage *gstor = guild2storage2(sd->status.guild_id);

		if (gstor == NULL || sd->state.storage_flag) {
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}
	}

	memset(&it, 0, sizeof(it));

	if( script_isstring(st, 2) )
	{
		const char* item_name = script_getstr(st, 2);
		std::shared_ptr<item_data> id = item_db.searchname( item_name );

		if( id == nullptr ){
			ShowError("buildin_%s: unknown item \"%s\".\n", command, item_name);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
		it.nameid = id->nameid;// "<item name>"
	}
	else
	{
		it.nameid = script_getnum(st, 2);// <item id>
		if( !itemdb_exists( it.nameid ) )
		{
			ShowError("buildin_%s: unknown item \"%u\".\n", command, it.nameid);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}

	it.amount=script_getnum(st,3);
	it.identify=script_getnum(st,4);
	it.refine=script_getnum(st,5);
	it.attribute=script_getnum(st,6);
	it.card[0]=script_getnum(st,7);
	it.card[1]=script_getnum(st,8);
	it.card[2]=script_getnum(st,9);
	it.card[3]=script_getnum(st,10);

	if (command[strlen(command)-1] == '3') {
		int res = script_getitem_randomoption(st, sd, &it, command, 11);
		if (res != SCRIPT_CMD_SUCCESS)
			return SCRIPT_CMD_FAILURE;
		flag |= 0x2;
	}

	if( it.amount <= 0 )
		return SCRIPT_CMD_SUCCESS;// nothing to do

	if( buildin_delitem_search(sd, &it, flag, loc) )
	{// success
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_%s: failed to delete %d items (AID=%d item_id=%u).\n", command, it.amount, sd->status.account_id, it.nameid);
	st->state = END;
	st->mes_active = 0;
	clif_scriptclose(sd, st->oid);
	return SCRIPT_CMD_FAILURE;
}

/// Deletes items from the target/attached player at given index.
/// delitemidx <index>{,<amount>{,<char id>}};
BUILDIN_FUNC(delitemidx) {
	struct map_session_data* sd;

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	int idx = script_getnum(st, 2);
	if (idx < 0 || idx >= MAX_INVENTORY) {
		ShowWarning("buildin_delitemidx: Index %d is out of the range 0-%d.\n", idx, MAX_INVENTORY - 1);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (sd->inventory_data[idx] == nullptr) {
		ShowWarning("buildin_delitemidx: No item can be deleted from index %d of player %s (AID: %u, CID: %u).\n", idx, sd->status.name, sd->status.account_id, sd->status.char_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	int amount;
	if (script_hasdata(st, 3))
		amount = script_getnum(st, 3);
	else
		amount = sd->inventory.u.items_inventory[idx].amount;

	if (amount > 0)
		script_pushint(st, pc_delitem(sd, idx, amount, 0, 0, LOG_TYPE_SCRIPT) == 0);
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Enables/Disables use of items while in an NPC [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(enableitemuse)
{
	TBL_PC *sd;
	if (script_rid2sd(sd))
		st->npc_item_flag = sd->npc_item_flag = 1;
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(disableitemuse)
{
	TBL_PC *sd;
	if (script_rid2sd(sd))
		st->npc_item_flag = sd->npc_item_flag = 0;
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns a character's specified stat.
 * Check pc_readparam for available options.
 * readparam <param>{,"<nick>"}
 * readparam <param>{,<char_id>}
 *------------------------------------------*/
BUILDIN_FUNC(readparam)
{
	int value;
	struct script_data *data = script_getdata(st, 2);
	TBL_PC *sd = NULL;

	if( script_hasdata(st, 3) ){
		if (script_isint(st, 3))
			script_charid2sd(3, sd);
		else
			script_nick2sd(3, sd);
	}else{
		script_rid2sd(sd);
	}
	
	if( !sd ){
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	// If you use a parameter, return the value behind it
	if( reference_toparam(data) ){
		get_val_(st, data, sd);
		value = (int)data->u.num;
	}else{
		value = static_cast<int>(pc_readparam(sd,script_getnum(st, 2)));
	}

	script_pushint(st,value);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Return charid identification
 * return by @num :
 *	0 : char_id
 *	1 : party_id
 *	2 : guild_id
 *	3 : account_id
 *	4 : bg_id
 *	5 : clan_id
 *------------------------------------------*/
BUILDIN_FUNC(getcharid)
{
	int num;
	TBL_PC *sd;

	num = script_getnum(st,2);

	if( !script_nick2sd(3,sd) ){
		script_pushint(st,0); //return 0, according docs
		return SCRIPT_CMD_SUCCESS;
	}

	switch( num ) {
	case 0: script_pushint(st,sd->status.char_id); break;
	case 1: script_pushint(st,sd->status.party_id); break;
	case 2: script_pushint(st,sd->status.guild_id); break;
	case 3: script_pushint(st,sd->status.account_id); break;
	case 4: script_pushint(st,sd->bg_id); break;
	case 5: script_pushint(st,sd->status.clan_id); break;
	default:
		ShowError("buildin_getcharid: invalid parameter (%d).\n", num);
		script_pushint(st,0);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * returns the GID of an NPC
 *------------------------------------------*/
BUILDIN_FUNC(getnpcid)
{
	int num = script_getnum(st,2);
	struct npc_data* nd = NULL;

	if( script_hasdata(st,3) )
	{// unique npc name
		if( ( nd = npc_name2id(script_getstr(st,3)) ) == NULL )
		{
			ShowError("buildin_getnpcid: No such NPC '%s'.\n", script_getstr(st,3));
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
	}

	switch (num) {
		case 0:
			script_pushint(st,nd ? nd->bl.id : st->oid);
			break;
		default:
			ShowError("buildin_getnpcid: invalid parameter (%d).\n", num);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Return the name of the party_id
 * null if not found
 *------------------------------------------*/
BUILDIN_FUNC(getpartyname)
{
	int party_id;
	struct party_data* p;

	party_id = script_getnum(st,2);

	if( ( p = party_search(party_id) ) != NULL )
	{
		script_pushstrcopy(st,p->party.name);
	}
	else
	{
		script_pushconststr(st,"null");
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the information of the members of a party by type
 * @party_id, @type
 * return by @type :
 *	- : nom des membres
 *	1 : char_id des membres
 *	2 : account_id des membres
 *------------------------------------------*/
BUILDIN_FUNC(getpartymember)
{
	struct party_data *p;
	uint8 j = 0;

	p = party_search(script_getnum(st,2));

	if (p != NULL) {
		uint8 i, type = 0;
		struct script_data *data = NULL;
		char *varname = NULL;

		if (script_hasdata(st,3))
 			type = script_getnum(st,3);

		if (script_hasdata(st,4)) {
			data = script_getdata(st, 4);
			if (!data_isreference(data)) {
				ShowError("buildin_getpartymember: Error in argument! Please give a variable to store values in.\n");
				return SCRIPT_CMD_FAILURE;
			}
			varname = reference_getname(data);
			if (type <= 0 && varname[strlen(varname)-1] != '$') {
				ShowError("buildin_getpartymember: The array %s is not string type.\n", varname);
				return SCRIPT_CMD_FAILURE;
			}
			if (not_server_variable(*varname)) {
				struct map_session_data *sd;

				if (!script_rid2sd(sd)) {
					ShowError("buildin_getpartymember: Cannot use a player variable '%s' if no player is attached.\n", varname);
					return SCRIPT_CMD_FAILURE;
				}
			}
		}

		for (i = 0; i < MAX_PARTY; i++) {
			if (p->party.member[i].account_id) {
				switch (type) {
					case 2:
						if (data)
							setd_sub_num( st, NULL, varname, j, p->party.member[i].account_id, data->ref );
						else
							mapreg_setreg(reference_uid(add_str("$@partymemberaid"), j),p->party.member[i].account_id);
						break;
					case 1:
						if (data)
							setd_sub_num( st, NULL, varname, j, p->party.member[i].char_id, data->ref );
						else
							mapreg_setreg(reference_uid(add_str("$@partymembercid"), j),p->party.member[i].char_id);
						break;
					default:
						if (data)
							setd_sub_str( st, NULL, varname, j, p->party.member[i].name, data->ref );
						else
							mapreg_setregstr(reference_uid(add_str("$@partymembername$"), j),p->party.member[i].name);
						break;
				}

				j++;
			}
		}
	}

	mapreg_setreg(add_str("$@partymembercount"),j);
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Retrieves party leader. if flag is specified,
 * return some of the leader data. Otherwise, return name.
 *------------------------------------------*/
BUILDIN_FUNC(getpartyleader)
{
	int party_id, type = 0, i=0;
	struct party_data *p;

	party_id=script_getnum(st,2);
	if( script_hasdata(st,3) )
		type=script_getnum(st,3);

	p=party_search(party_id);

	if (p) //Search leader
	for(i = 0; i < MAX_PARTY && !p->party.member[i].leader; i++);

	if (!p || i == MAX_PARTY) { //leader not found
		if (type)
			script_pushint(st,-1);
		else
			script_pushconststr(st,"null");
		return SCRIPT_CMD_SUCCESS;
	}

	switch (type) {
		case 1: script_pushint(st,p->party.member[i].account_id); break;
		case 2: script_pushint(st,p->party.member[i].char_id); break;
		case 3: script_pushint(st,p->party.member[i].class_); break;
		case 4: script_pushstrcopy(st,mapindex_id2name(p->party.member[i].map)); break;
		case 5: script_pushint(st,p->party.member[i].lv); break;
		default: script_pushstrcopy(st,p->party.member[i].name); break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Return the name of the @guild_id
 * null if not found
 *------------------------------------------*/
BUILDIN_FUNC(getguildname)
{
	int guild_id;
	struct guild* g;

	guild_id = script_getnum(st,2);
	if( ( g = guild_search(guild_id) ) != NULL )
		script_pushstrcopy(st,g->name);
	else 
		script_pushconststr(st,"null");
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Return the name of the guild master of @guild_id
 * null if not found
 *------------------------------------------*/
BUILDIN_FUNC(getguildmaster)
{
	int guild_id;
	struct guild* g;

	guild_id = script_getnum(st,2);
	if( ( g = guild_search(guild_id) ) != NULL )
		script_pushstrcopy(st,g->member[0].name);
	else 
		script_pushconststr(st,"null");
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getguildmasterid)
{
	int guild_id;
	struct guild* g;

	guild_id = script_getnum(st,2);
	if( ( g = guild_search(guild_id) ) != NULL )
		script_pushint(st,g->member[0].char_id);
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get char string information by type :
 * Return by @type :
 *	0 : char_name
 *	1 : party_name or ""
 *	2 : guild_name or ""
 *	3 : map_name
 *	- : ""
 * strcharinfo(<type>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(strcharinfo)
{
	TBL_PC *sd;
	int num;
	struct guild* g;
	struct party_data* p;

	if (!script_charid2sd(3,sd)) {
		script_pushconststr(st,"");
		return SCRIPT_CMD_FAILURE;
	}

	num=script_getnum(st,2);
	switch(num){
		case 0:
			script_pushstrcopy(st,sd->status.name);
			break;
		case 1:
			if( ( p = party_search(sd->status.party_id) ) != NULL ) {
				script_pushstrcopy(st,p->party.name);
			} else {
				script_pushconststr(st,"");
			}
			break;
		case 2:
			if( ( g = sd->guild ) != NULL ) {
				script_pushstrcopy(st,g->name);
			} else {
				script_pushconststr(st,"");
			}
			break;
		case 3:
			script_pushconststr(st,map_getmapdata(sd->bl.m)->name);
			break;
		default:
			ShowWarning("buildin_strcharinfo: unknown parameter.\n");
			script_pushconststr(st,"");
			break;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get npc string information by type
 * return by @type:
 *	0 : name
 *	1 : str#
 *	2 : #str
 *	3 : ::str
 *	4 : map name
 *------------------------------------------*/
BUILDIN_FUNC(strnpcinfo)
{
	TBL_NPC* nd;
	int num;
	char *buf,*name=NULL;

	nd = map_id2nd(st->oid);
	if (!nd) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_SUCCESS;
	}

	num = script_getnum(st,2);
	switch(num){
		case 0: // display name
			name = aStrdup(nd->name);
			break;
		case 1: // visible part of display name
			if((buf = strchr(nd->name,'#')) != NULL)
			{
				name = aStrdup(nd->name);
				name[buf - nd->name] = 0;
			} else // Return the name, there is no '#' present
				name = aStrdup(nd->name);
			break;
		case 2: // # fragment
			if((buf = strchr(nd->name,'#')) != NULL)
				name = aStrdup(buf+1);
			break;
		case 3: // unique name
			name = aStrdup(nd->exname);
			break;
		case 4: // map name
			if (nd->bl.m >= 0)
				name = aStrdup(map_getmapdata(nd->bl.m)->name);
			break;
	}

	if(name)
		script_pushstr(st, name);
	else
		script_pushconststr(st, "");

	return SCRIPT_CMD_SUCCESS;
}

/**
 * getequipid({<equipment slot>,<char_id>})
 **/
BUILDIN_FUNC(getequipid)
{
	int i, num;
	TBL_PC* sd;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 2))
		num = script_getnum(st, 2);
	else
		num = EQI_COMPOUND_ON;

	if (num == EQI_COMPOUND_ON)
		i = current_equip_item_index;
	else if (equip_index_check(num)) // get inventory position of item
		i = pc_checkequip(sd, equip_bitmask[num]);
	else {
		ShowError( "buildin_getequipid: Unknown equip index '%d'\n", num );
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if (i >= 0 && i < MAX_INVENTORY && sd->inventory_data[i])
		script_pushint(st, sd->inventory_data[i]->nameid);
	else
		script_pushint(st, -1);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getequipuniqueid(<equipment slot>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getequipuniqueid)
{
	int i, num;
	TBL_PC* sd;
	struct item* item;

	if (!script_charid2sd(3, sd)) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	num = script_getnum(st,2);
	if ( !equip_index_check(num) ) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	// get inventory position of item
	i = pc_checkequip(sd,equip_bitmask[num]);
	if (i < 0) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	item = &sd->inventory.u.items_inventory[i];
	if (item != 0) {
		int maxlen = 256;
		char *buf = (char *)aMalloc(maxlen*sizeof(char));

		memset(buf, 0, maxlen);
		snprintf(buf, maxlen-1, "%llu", (unsigned long long)item->unique_id);

		script_pushstr(st, buf);
	} else
		script_pushconststr(st, "");

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the equipement name at pos
 * return item jname or ""
 * getequipname(<equipment slot>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getequipname)
{
	int i, num;
	TBL_PC* sd;
	struct item_data* item;

	if (!script_charid2sd(3, sd)) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	num = script_getnum(st,2);
	if( !equip_index_check(num) )
	{
		script_pushconststr(st,"");
		return SCRIPT_CMD_SUCCESS;
	}

	// get inventory position of item
	i = pc_checkequip(sd,equip_bitmask[num]);
	if( i < 0 )
	{
		script_pushconststr(st,"");
		return SCRIPT_CMD_SUCCESS;
	}

	item = sd->inventory_data[i];
	if( item != 0 )
		script_pushstrcopy(st,item->ename.c_str());
	else
		script_pushconststr(st,"");

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getbrokenid [Valaris]
 * getbrokenid(<number>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getbrokenid)
{
	int i,num,id = 0,brokencounter = 0;
	TBL_PC *sd;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	num = script_getnum(st,2);
	for(i = 0; i < MAX_INVENTORY; i++) {
		if( sd->inventory.u.items_inventory[i].attribute == 1 && !itemdb_ishatched_egg( &sd->inventory.u.items_inventory[i] ) ){
				brokencounter++;
				if(num == brokencounter){
					id = sd->inventory.u.items_inventory[i].nameid;
					break;
				}
		}
	}

	script_pushint(st,id);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * repair [Valaris]
 * repair <num>{,<char_id>};
 *------------------------------------------*/
BUILDIN_FUNC(repair)
{
	int i,num;
	int repaircounter = 0;
	TBL_PC *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	num = script_getnum(st,2);
	for(i = 0; i < MAX_INVENTORY; i++) {
		if( sd->inventory.u.items_inventory[i].attribute == 1 && !itemdb_ishatched_egg( &sd->inventory.u.items_inventory[i] ) ){
				repaircounter++;
				if(num == repaircounter) {
					sd->inventory.u.items_inventory[i].attribute = 0;
					clif_equiplist(sd);
					clif_produceeffect(sd, 0, sd->inventory.u.items_inventory[i].nameid);
					clif_misceffect(&sd->bl, 3);
					break;
				}
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * repairall {<char_id>}
 *------------------------------------------*/
BUILDIN_FUNC(repairall)
{
	int i, repaircounter = 0;
	TBL_PC *sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	for(i = 0; i < MAX_INVENTORY; i++)
	{
		if( sd->inventory.u.items_inventory[i].nameid && sd->inventory.u.items_inventory[i].attribute == 1 && !itemdb_ishatched_egg( &sd->inventory.u.items_inventory[i] ) ){
			sd->inventory.u.items_inventory[i].attribute = 0;
			clif_produceeffect(sd,0,sd->inventory.u.items_inventory[i].nameid);
			repaircounter++;
		}
	}

	if(repaircounter)
	{
		clif_misceffect(&sd->bl, 3);
		clif_equiplist(sd);
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Chk if player have something equiped at pos
 * getequipisequiped <pos>{,<char_id>}
 *------------------------------------------*/
BUILDIN_FUNC(getequipisequiped)
{
	int i = -1,num;
	TBL_PC *sd;

	num = script_getnum(st,2);

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if ( equip_index_check(num) )
		i=pc_checkequip(sd,equip_bitmask[num]);

	if(i >= 0)
		script_pushint(st,1);
	else
		 script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Chk if the player have something equiped at pos
 * if so chk if this item ain't marked not refinable or rental
 * return (npc)
 *	1 : true
 *	0 : false
 * getequipisenableref(<equipment slot>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getequipisenableref)
{
	int i = -1,num;
	TBL_PC *sd;

	num = script_getnum(st,2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if( equip_index_check(num) )
		i = pc_checkequip(sd,equip_bitmask[num]);
	if( i >= 0 && sd->inventory_data[i] && !sd->inventory_data[i]->flag.no_refine && !sd->inventory.u.items_inventory[i].expire_time )
		script_pushint(st,1);
	else
		script_pushint(st,0);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the item refined value at pos
 * return (npc)
 *	x : refine amount
 *	0 : false (not refined)
 * getequiprefinerycnt(<equipment slot>{,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getequiprefinerycnt)
{
	int i = -1,num;
	TBL_PC *sd;

	num = script_getnum(st,2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (equip_index_check(num))
		i=pc_checkequip(sd,equip_bitmask[num]);
	if(i >= 0)
		script_pushint(st,sd->inventory.u.items_inventory[i].refine);
	else
		script_pushint(st,0);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the weapon level value at pos
 * (pos should normally only be EQI_HAND_L or EQI_HAND_R)
 * return (npc)
 *	x : weapon level
 *	0 : false
 * getequipweaponlv({<equipment slot>{,<char_id>}})
 *------------------------------------------*/
BUILDIN_FUNC(getequipweaponlv)
{
	int num;

	if( script_hasdata( st, 2 ) ){
		num = script_getnum(st, 2);
	}else{
		num = EQI_COMPOUND_ON;
	}

	struct map_session_data* sd;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	int i = -1;

	if( num == EQI_COMPOUND_ON ){
		if( current_equip_item_index == -1 ){
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}

		i = current_equip_item_index;
	}else if (equip_index_check(num))
		i = pc_checkequip(sd, equip_bitmask[num]);
	if( i >= 0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_WEAPON ){
		script_pushint( st, sd->inventory_data[i]->weapon_level );
	}else{
		script_pushint( st, 0 );
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getequiparmorlv){
	int num;

	if( script_hasdata( st, 2 ) ){
		num = script_getnum(st, 2);
	}else{
		num = EQI_COMPOUND_ON;
	}

	struct map_session_data* sd;

	if( !script_charid2sd( 3, sd ) ){
		script_pushint( st, 0 );
		return SCRIPT_CMD_FAILURE;
	}

	int i = -1;

	if( num == EQI_COMPOUND_ON ){
		if( current_equip_item_index == -1 ){
			script_pushint( st, 0 );
			return SCRIPT_CMD_FAILURE;
		}

		i = current_equip_item_index;
	}else if( equip_index_check( num ) ){
		i = pc_checkequip( sd, equip_bitmask[num] );
	}else{
		ShowError( "buildin_getequiparmorlv: unsupported equip index %d.\n", num );
		script_pushint( st, 0 );
		return SCRIPT_CMD_FAILURE;
	}

	if( i >= 0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_ARMOR ){
		script_pushint( st, sd->inventory_data[i]->armor_level );
	}else{
		script_pushint( st, 0 );
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the item refine chance (from refine.txt) for item at pos
 * return (npc)
 *	x : refine chance
 *	0 : false (max refine level or unequip..)
 * getequippercentrefinery(<equipment slot>{,<enriched>,<char_id>})
 *------------------------------------------*/
BUILDIN_FUNC(getequippercentrefinery)
{
	int i = -1,num;
	bool enriched = false;
	TBL_PC *sd;

	num = script_getnum(st,2);
	if (script_hasdata(st, 3))
		enriched = script_getnum(st, 3) != 0;

	if (!script_charid2sd(4, sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (equip_index_check(num))
		i = pc_checkequip(sd,equip_bitmask[num]);
	if (i >= 0 && sd->inventory.u.items_inventory[i].nameid) {
		std::shared_ptr<s_refine_level_info> info = refine_db.findLevelInfo( *sd->inventory_data[i], sd->inventory.u.items_inventory[i] );

		if( info == nullptr ){
			script_pushint( st, 0 );
			return SCRIPT_CMD_SUCCESS;
		}

		std::shared_ptr<s_refine_cost> cost = util::umap_find( info->costs, (uint16)( enriched ? REFINE_COST_ENRICHED : REFINE_COST_NORMAL ) );

		if( cost == nullptr ){
			script_pushint( st, 0 );
			return SCRIPT_CMD_SUCCESS;
		}

		script_pushint( st, cost->chance / 100 );
	}
	else
		script_pushint(st,0);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Refine +1 item at pos and log and display refine
 * successrefitem <equipment slot>{,<count>{,<char_id>}}
 *------------------------------------------*/
BUILDIN_FUNC(successrefitem) {
	short i = -1, up = 1;
	int pos;
	TBL_PC *sd;

	pos = script_getnum(st,2);

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 3))
		up = script_getnum(st, 3);

	if (equip_index_check(pos))
		i = pc_checkequip(sd,equip_bitmask[pos]);
	if (i >= 0) {
		unsigned int ep = sd->inventory.u.items_inventory[i].equip;

		//Logs items, got from (N)PC scripts [Lupus]
		log_pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->inventory.u.items_inventory[i]);

		if (sd->inventory.u.items_inventory[i].refine >= MAX_REFINE) {
			script_pushint(st, MAX_REFINE);
			return SCRIPT_CMD_SUCCESS;
		}

		sd->inventory.u.items_inventory[i].refine += up;
		sd->inventory.u.items_inventory[i].refine = cap_value( sd->inventory.u.items_inventory[i].refine, 0, MAX_REFINE);
		pc_unequipitem(sd,i,2); // status calc will happen in pc_equipitem() below

		clif_refine(sd->fd,0,i,sd->inventory.u.items_inventory[i].refine);
		clif_delitem(sd,i,1,3);

		//Logs items, got from (N)PC scripts [Lupus]
		log_pick_pc(sd, LOG_TYPE_SCRIPT, 1, &sd->inventory.u.items_inventory[i]);

		clif_additem(sd,i,1,0);
		pc_equipitem(sd,i,ep);
		clif_misceffect(&sd->bl,3);
		if( sd->inventory_data[i]->type == IT_WEAPON ){
			achievement_update_objective(sd, AG_ENCHANT_SUCCESS, 2, sd->inventory_data[i]->weapon_level, sd->inventory.u.items_inventory[i].refine);
		}
		if (sd->inventory.u.items_inventory[i].refine == battle_config.blacksmith_fame_refine_threshold &&
			sd->inventory.u.items_inventory[i].card[0] == CARD0_FORGE &&
			sd->status.char_id == (int)MakeDWord(sd->inventory.u.items_inventory[i].card[2],sd->inventory.u.items_inventory[i].card[3]) &&
			sd->inventory_data[i]->type == IT_WEAPON )
		{ // Fame point system [DracoRPG]
			switch( sd->inventory_data[i]->weapon_level ){
				case 1:
					pc_addfame(*sd, battle_config.fame_refine_lv1); // Success to refine to +10 a lv1 weapon you forged = +1 fame point
					break;
				case 2:
					pc_addfame(*sd, battle_config.fame_refine_lv2); // Success to refine to +10 a lv2 weapon you forged = +25 fame point
					break;
				case 3:
					pc_addfame(*sd, battle_config.fame_refine_lv3); // Success to refine to +10 a lv3 weapon you forged = +1000 fame point
					break;
			 }
		}
		script_pushint(st, sd->inventory.u.items_inventory[i].refine);
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_successrefitem: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st, -1);
	return SCRIPT_CMD_FAILURE;
}

/*==========================================
 * Show a failed Refine +1 attempt
 * failedrefitem <equipment slot>{,<char_id>}
 *------------------------------------------*/
BUILDIN_FUNC(failedrefitem) {
	short i = -1;
	int pos;
	TBL_PC *sd;

	pos = script_getnum(st,2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (equip_index_check(pos))
		i = pc_checkequip(sd,equip_bitmask[pos]);
	if (i >= 0) {
		sd->inventory.u.items_inventory[i].refine = 0;
		pc_unequipitem(sd,i,3); //recalculate bonus
		clif_refine(sd->fd,1,i,sd->inventory.u.items_inventory[i].refine); //notify client of failure
		pc_delitem(sd,i,1,0,2,LOG_TYPE_SCRIPT);
		clif_misceffect(&sd->bl,2); 	// display failure effect
		achievement_update_objective(sd, AG_ENCHANT_FAIL, 1, 1);
		script_pushint(st, 1);
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_failedrefitem: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st, 0);
	return SCRIPT_CMD_FAILURE;
}

/*==========================================
 * Downgrades an Equipment Part by -1 . [Masao]
 * downrefitem <equipment slot>{,<count>{,<char_id>}}
 *------------------------------------------*/
BUILDIN_FUNC(downrefitem) {
	short i = -1, down = 1;
	int pos;
	TBL_PC *sd;

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	pos = script_getnum(st,2);
	if (script_hasdata(st, 3))
		down = script_getnum(st, 3);

	if (equip_index_check(pos))
		i = pc_checkequip(sd,equip_bitmask[pos]);
	if (i >= 0) {
		unsigned int ep = sd->inventory.u.items_inventory[i].equip;

		//Logs items, got from (N)PC scripts [Lupus]
		log_pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->inventory.u.items_inventory[i]);

		pc_unequipitem(sd,i,2); // status calc will happen in pc_equipitem() below
		sd->inventory.u.items_inventory[i].refine -= down;
		sd->inventory.u.items_inventory[i].refine = cap_value( sd->inventory.u.items_inventory[i].refine, 0, MAX_REFINE);

		clif_refine(sd->fd,2,i,sd->inventory.u.items_inventory[i].refine);
		clif_delitem(sd,i,1,3);

		//Logs items, got from (N)PC scripts [Lupus]
		log_pick_pc(sd, LOG_TYPE_SCRIPT, 1, &sd->inventory.u.items_inventory[i]);

		clif_additem(sd,i,1,0);
		pc_equipitem(sd,i,ep);
		clif_misceffect(&sd->bl,2);
		achievement_update_objective(sd, AG_ENCHANT_FAIL, 1, sd->inventory.u.items_inventory[i].refine);
		script_pushint(st, sd->inventory.u.items_inventory[i].refine);
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_downrefitem: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st, -1);
	return SCRIPT_CMD_FAILURE;
}

/**
 * Delete the item equipped at pos.
 * delequip <equipment slot>{,<char_id>};
 **/
BUILDIN_FUNC(delequip) {
	short i = -1;
	int pos;
	int8 ret;
	TBL_PC *sd;

	pos = script_getnum(st,2);
	if (!script_charid2sd(3,sd)) {
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if (equip_index_check(pos))
		i = pc_checkequip(sd,equip_bitmask[pos]);
	if (i >= 0) {
		pc_unequipitem(sd,i,3); //recalculate bonus
		ret = !(pc_delitem(sd,i,1,0,2,LOG_TYPE_SCRIPT));
	}
	else {
		ShowError("buildin_delequip: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,ret);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Break the item equipped at pos.
 * breakequip <equipment slot>{,<char_id>};
 **/
BUILDIN_FUNC(breakequip) {
	short i = -1;
	int pos;
	TBL_PC *sd;

	pos = script_getnum(st,2);
	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if (equip_index_check(pos))
		i = pc_checkequip(sd,equip_bitmask[pos]);
	if (i >= 0) {
		sd->inventory.u.items_inventory[i].attribute = 1;
		pc_unequipitem(sd,i,3);
		clif_equiplist(sd);
		script_pushint(st,1);
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_breakequip: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st,0);
	return SCRIPT_CMD_FAILURE;
}

/**
 * statusup <stat>{,<char_id>};
 **/
BUILDIN_FUNC(statusup)
{
	int type;
	TBL_PC *sd;

	type = script_getnum(st,2);

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	pc_statusup(sd, type, 1);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * statusup2 <stat>,<amount>{,<char_id>};
 **/
BUILDIN_FUNC(statusup2)
{
	int type,val;
	TBL_PC *sd;

	type=script_getnum(st,2);
	val=script_getnum(st,3);

	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	pc_statusup2(sd,type,val);

	return SCRIPT_CMD_SUCCESS;
}

/**
* traitstatusup <stat>{,<char_id>};
**/
BUILDIN_FUNC(traitstatusup)
{
	struct map_session_data *sd;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	int type = script_getnum( st, 2 );

	if( type < SP_POW || type > SP_CRT ){
		ShowError( "buildin_traitstatusup: Unknown trait type %d\n", type );
		return SCRIPT_CMD_FAILURE;
	}

	pc_traitstatusup( sd, type, 1 );

	return SCRIPT_CMD_SUCCESS;
}

/**
* traitstatusup2 <stat>,<amount>{,<char_id>};
**/
BUILDIN_FUNC(traitstatusup2)
{
	struct map_session_data *sd;

	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	int type = script_getnum( st, 2 );

	if( type < SP_POW || type > SP_CRT ){
		ShowError( "buildin_traitstatusup2: Unknown trait type %d\n", type );
		return SCRIPT_CMD_FAILURE;
	}

	pc_traitstatusup2( sd, type, script_getnum( st, 3 ) );

	return SCRIPT_CMD_SUCCESS;
}

/// See 'doc/item_bonus.txt'
///
/// bonus <bonus type>,<val1>;
/// bonus2 <bonus type>,<val1>,<val2>;
/// bonus3 <bonus type>,<val1>,<val2>,<val3>;
/// bonus4 <bonus type>,<val1>,<val2>,<val3>,<val4>;
/// bonus5 <bonus type>,<val1>,<val2>,<val3>,<val4>,<val5>;
BUILDIN_FUNC(bonus)
{
	int type;
	int val1 = 0;
	int val2 = 0;
	int val3 = 0;
	int val4 = 0;
	int val5 = 0;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS; // no player attached

	type = script_getnum(st,2);
	switch( type ) {
		case SP_AUTOSPELL:
		case SP_AUTOSPELL_WHENHIT:
		case SP_AUTOSPELL_ONSKILL:
		case SP_SKILL_ATK:
		case SP_SKILL_HEAL:
		case SP_SKILL_HEAL2:
		case SP_ADD_SKILL_BLOW:
		case SP_CASTRATE:
		case SP_ADDEFF_ONSKILL:
		case SP_SKILL_USE_SP_RATE:
		case SP_SKILL_COOLDOWN:
		case SP_SKILL_FIXEDCAST:
		case SP_SKILL_VARIABLECAST:
		case SP_VARCASTRATE:
		case SP_FIXCASTRATE:
		case SP_SKILL_DELAY:
		case SP_SKILL_USE_SP:
		case SP_SUB_SKILL:
#ifdef Pandas_Bonus2_bAddSkillRange
		case SP_PANDAS_ADDSKILLRANGE:
#endif // Pandas_Bonus2_bAddSkillRange
#ifdef Pandas_Bonus2_bSkillNoRequire
		case SP_PANDAS_SKILLNOREQUIRE:
#endif // Pandas_Bonus2_bSkillNoRequire
			// these bonuses support skill names
			if (script_isstring(st, 3)) {
				const char *name = script_getstr(st, 3);

				if (!(val1 = skill_name2id(name))) {
					ShowError("buildin_bonus: Invalid skill name %s passed to item bonus. Skipping.\n", name);
					return SCRIPT_CMD_FAILURE;
				}
			} else {
				val1 = script_getnum(st, 3);

				if (strcmpi(script_getfuncname(st), "bonus") && !skill_get_index(val1)) { // Only check skill ID for bonus2, bonus3, bonus4, or bonus5
					ShowError("buildin_bonus: Invalid skill ID %d passed to item bonus. Skipping.\n", val1);
					return SCRIPT_CMD_FAILURE;
				}
			}
			break;
		default:
			if (script_hasdata(st, 3))
				val1 = script_getnum(st, 3);
			break;
	}

	switch( script_lastdata(st)-2 ) {
		case 0:
		case 1:
			pc_bonus(sd, type, val1);
			break;
		case 2:
			val2 = script_getnum(st,4);
			pc_bonus2(sd, type, val1, val2);
			break;
		case 3:
			val2 = script_getnum(st,4);
			val3 = script_getnum(st,5);
			pc_bonus3(sd, type, val1, val2, val3);
			break;
		case 4:
			if( type == SP_AUTOSPELL_ONSKILL && script_isstring(st, 4) )
				val2 = skill_name2id(script_getstr(st,4)); // 2nd value can be skill name
			else
				val2 = script_getnum(st,4);

			val3 = script_getnum(st,5);
			val4 = script_getnum(st,6);
			pc_bonus4(sd, type, val1, val2, val3, val4);
			break;
		case 5:
			if( type == SP_AUTOSPELL_ONSKILL && script_isstring(st, 4) )
				val2 = skill_name2id(script_getstr(st,4)); // 2nd value can be skill name
			else
				val2 = script_getnum(st,4);

			val3 = script_getnum(st,5);
			val4 = script_getnum(st,6);
			val5 = script_getnum(st,7);
			pc_bonus5(sd, type, val1, val2, val3, val4, val5);
			break;
		default:
			ShowDebug("buildin_bonus: unexpected number of arguments (%d)\n", (script_lastdata(st) - 1));
			break;
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(autobonus)
{
	unsigned int dur, pos;
	short rate;
	uint16 atk_type = 0;
	TBL_PC* sd;
	const char *bonus_script, *other_script = NULL;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS; // no player attached

	if (current_equip_combo_pos)
		pos = current_equip_combo_pos;
	else
		pos = sd->inventory.u.items_inventory[current_equip_item_index].equip;

	rate = script_getnum(st,3);
	dur = script_getnum(st,4);
	bonus_script = script_getstr(st,2);
	if( !rate || !dur || !pos || !bonus_script )
		return SCRIPT_CMD_SUCCESS;

	if( script_hasdata(st,5) )
		atk_type = script_getnum(st,5);
	if( script_hasdata(st,6) )
		other_script = script_getstr(st,6);

	if( pc_addautobonus(sd->autobonus, bonus_script, rate, dur, atk_type, other_script, pos, false) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
			script_add_autobonus(other_script);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(autobonus2)
{
	unsigned int dur, pos;
	short rate;
	uint16 atk_type = 0;
	TBL_PC* sd;
	const char *bonus_script, *other_script = NULL;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS; // no player attached

	if (current_equip_combo_pos)
		pos = current_equip_combo_pos;
	else
		pos = sd->inventory.u.items_inventory[current_equip_item_index].equip;

	rate = script_getnum(st,3);
	dur = script_getnum(st,4);
	bonus_script = script_getstr(st,2);
	if( !rate || !dur || !pos || !bonus_script )
		return SCRIPT_CMD_SUCCESS;

	if( script_hasdata(st,5) )
		atk_type = script_getnum(st,5);
	if( script_hasdata(st,6) )
		other_script = script_getstr(st,6);

	if( pc_addautobonus(sd->autobonus2, bonus_script, rate,dur, atk_type, other_script, pos, false) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
			script_add_autobonus(other_script);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(autobonus3)
{
	unsigned int dur, pos;
	short rate;
	uint16 skill_id = 0;
	TBL_PC* sd;
	const char *bonus_script, *other_script = NULL;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS; // no player attached

	if (current_equip_combo_pos)
		pos = current_equip_combo_pos;
	else
		pos = sd->inventory.u.items_inventory[current_equip_item_index].equip;

	rate = script_getnum(st,3);
	dur = script_getnum(st,4);
	if (script_isstring(st, 5)) {
		const char *name = script_getstr(st, 5);

		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_autobonus3: Invalid skill name %s passed to item bonus. Skipping.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		skill_id = script_getnum(st, 5);

		if (!skill_get_index(skill_id)) {
			ShowError("buildin_autobonus3: Invalid skill ID %d passed to item bonus. Skipping.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	bonus_script = script_getstr(st,2);
	if( !rate || !dur || !pos || !bonus_script )
		return SCRIPT_CMD_SUCCESS;

	if( script_hasdata(st,6) )
		other_script = script_getstr(st,6);

	if( pc_addautobonus(sd->autobonus3, bonus_script, rate, dur, skill_id, other_script, pos, true) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
			script_add_autobonus(other_script);
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Changes the level of a player skill.
/// <flag> defaults to 1
/// <flag>=0 : set the level of the skill
/// <flag>=1 : set the temporary level of the skill
/// <flag>=2 : add to the level of the skill
///
/// skill <skill id>,<level>,<flag>
/// skill <skill id>,<level>
/// skill "<skill name>",<level>,<flag>
/// skill "<skill name>",<level>
BUILDIN_FUNC(skill)
{
	int id;
	int level;
	int flag = ADDSKILL_TEMP;
	TBL_PC* sd;
	const char* command = script_getfuncname(st);

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;// no player attached, report source

	if (strcmpi(command, "addtoskill") == 0)
		flag = ADDSKILL_TEMP_ADDLEVEL;

	id = ( script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2) );
	level = script_getnum(st,3);
	if( script_hasdata(st,4) )
		flag = script_getnum(st,4);
	pc_skill(sd, id, level, (enum e_addskill_type)flag);

	return SCRIPT_CMD_SUCCESS;
}

/// Increases the level of a guild skill.
///
/// guildskill <skill id>,<amount>;
/// guildskill "<skill name>",<amount>;
BUILDIN_FUNC(guildskill)
{
	int id;
	int level;
	TBL_PC* sd;
	int i;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;// no player attached, report source

	id = ( script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2) );
	level = script_getnum(st,3);
	for( i=0; i < level; i++ )
		guild_skillup(sd, id);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns the level of the player skill.
///
/// getskilllv(<skill id>) -> <level>
/// getskilllv("<skill name>") -> <level>
BUILDIN_FUNC(getskilllv)
{
	int id;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;// no player attached, report source

	id = ( script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2) );
	script_pushint(st, pc_checkskill(sd,id));

	return SCRIPT_CMD_SUCCESS;
}

/// Returns the level of the guild skill.
///
/// getgdskilllv(<guild id>,<skill id>) -> <level>
/// getgdskilllv(<guild id>,"<skill name>") -> <level>
BUILDIN_FUNC(getgdskilllv)
{
	int guild_id;
	uint16 skill_id;
	struct guild* g;

	guild_id = script_getnum(st,2);
	skill_id = ( script_isstring(st, 3) ? skill_name2id(script_getstr(st,3)) : script_getnum(st,3) );
	g = guild_search(guild_id);
	if( g == NULL )
		script_pushint(st, -1);
	else
		script_pushint(st, guild_checkskill(g,skill_id));

	return SCRIPT_CMD_SUCCESS;
}

/// Returns the 'basic_skill_check' setting.
/// This config determines if the server checks the skill level of NV_BASIC
/// before allowing the basic actions.
///
/// basicskillcheck() -> <bool>
BUILDIN_FUNC(basicskillcheck)
{
	script_pushint(st, battle_config.basic_skill_check);
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the GM level of the player.
///
/// getgmlevel({<char_id>}) -> <level>
BUILDIN_FUNC(getgmlevel)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	script_pushint(st, pc_get_group_level(sd));
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the group ID of the player.
///
/// getgroupid({<char_id>}) -> <int>
BUILDIN_FUNC(getgroupid)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	script_pushint(st, pc_get_group_id(sd));
	return SCRIPT_CMD_SUCCESS;
}

/// Terminates the execution of this script instance.
///
/// end
BUILDIN_FUNC(end)
{
	TBL_PC* sd;

	sd = map_id2sd(st->rid);

	st->state = END;

#ifdef Pandas_ScriptEngine_Express
	// 防止在穿透事件的脚本代码中使用 end 指令, 会导致角色正在执行的脚本或对话被强制中断,
	// 或与 NPC 进行中的对话框直接显示出 [关闭] 按钮的问题 [Sola丶小克]
	if (sd && npc_event_is_realtime(sd->pandas.workinevent))
		return SCRIPT_CMD_SUCCESS;
#endif // Pandas_ScriptEngine_Express

	if( st->mes_active )
		st->mes_active = 0;

	if (sd){
		if (sd->state.callshop == 0)
			clif_scriptclose(sd, st->oid); // If a menu/select/prompt is active, close it.
		else 
			sd->state.callshop = 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Checks if the player has that effect state (option).
///
/// checkoption(<option>{,<char_id>}) -> <bool>
BUILDIN_FUNC(checkoption)
{
	int option;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	option = script_getnum(st,2);
	if( sd->sc.option&option )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Checks if the player is in that body state (opt1).
///
/// checkoption1(<opt1>{,<char_id>}) -> <bool>
BUILDIN_FUNC(checkoption1)
{
	int opt1;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	opt1 = script_getnum(st,2);
	if( sd->sc.opt1 == opt1 )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Checks if the player has that health state (opt2).
///
/// checkoption2(<opt2>{,<char_id>}) -> <bool>
BUILDIN_FUNC(checkoption2)
{
	int opt2;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	opt2 = script_getnum(st,2);
	if( sd->sc.opt2&opt2 )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Changes the effect state (option) of the player.
/// <flag> defaults to 1
/// <flag>=0 : removes the option
/// <flag>=other : adds the option
///
/// setoption <option>{,<flag>{,<char_id>}};
BUILDIN_FUNC(setoption)
{
	int option;
	int flag = 1;
	TBL_PC* sd;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	option = script_getnum(st,2);
	if( script_hasdata(st,3) )
		flag = script_getnum(st,3);
	else if( !option ){// Request to remove everything.
		flag = 0;
		option = OPTION_FALCON|OPTION_RIDING;
#ifndef NEW_CARTS
		option |= OPTION_CART;
#endif
	}
	if( flag ){// Add option
		if( option&OPTION_WEDDING && !battle_config.wedding_modifydisplay )
			option &= ~OPTION_WEDDING;// Do not show the wedding sprites
		pc_setoption(sd, sd->sc.option|option);
	} else// Remove option
		pc_setoption(sd, sd->sc.option&~option);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns if the player has a cart.
///
/// checkcart({char_id}) -> <bool>
///
/// @author Valaris
BUILDIN_FUNC(checkcart)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( pc_iscarton(sd) )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Sets the cart of the player.
/// <type> defaults to 1
/// <type>=0 : removes the cart
/// <type>=1 : Normal cart
/// <type>=2 : Wooden cart
/// <type>=3 : Covered cart with flowers and ferns
/// <type>=4 : Wooden cart with a Panda doll on the back
/// <type>=5 : Normal cart with bigger wheels, a roof and a banner on the back
///
/// setcart {<type>{,<char_id>}};
BUILDIN_FUNC(setcart)
{
	int type = 1;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_hasdata(st,2) )
		type = script_getnum(st,2);
	pc_setcart(sd, type);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns if the player has a falcon.
///
/// checkfalcon({char_id}) -> <bool>
///
/// @author Valaris
BUILDIN_FUNC(checkfalcon)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( pc_isfalcon(sd) )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Sets if the player has a falcon or not.
/// <flag> defaults to 1
///
/// setfalcon {<flag>{,<char_id>}};
BUILDIN_FUNC(setfalcon)
{
	int flag = 1;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_hasdata(st,2) )
		flag = script_getnum(st,2);

	pc_setfalcon(sd, flag);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns if the player is riding.
///
/// checkriding({char_id}) -> <bool>
///
/// @author Valaris
BUILDIN_FUNC(checkriding)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( pc_isriding(sd) )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Sets if the player is riding.
/// <flag> defaults to 1
///
/// setriding {<flag>{,<char_id>}};
BUILDIN_FUNC(setriding)
{
	int flag = 1;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_hasdata(st,2) )
		flag = script_getnum(st,2);
	pc_setriding(sd, flag);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns if the player has a warg.
///
/// checkwug({char_id}) -> <bool>
///
BUILDIN_FUNC(checkwug)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( pc_iswug(sd) || pc_isridingwug(sd) )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Returns if the player is wearing MADO Gear.
///
/// checkmadogear({<char_id>}) -> <bool>
///
BUILDIN_FUNC(checkmadogear)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( pc_ismadogear(sd) )
		script_pushint(st, 1);
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Sets if the player is riding MADO Gear.
/// <flag> defaults to true
/// <type> defaults to MADO_ROBOT
///
/// setmadogear {<flag>{,type{,<char_id>}}};
BUILDIN_FUNC(setmadogear)
{
	bool flag = true;
	TBL_PC* sd;
	uint16 type = MADO_ROBOT;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	if( script_hasdata(st,2) )
		flag = script_getnum(st,2) != 0;
	if (script_hasdata(st, 3)) {
		type = script_getnum(st, 3);

		if (type == MADO_UNUSED || type >= MADO_MAX) {
			ShowError("buildin_setmadogear: Invalid mado gear type %hu, defaulting to robot...\n", type);
			type = MADO_ROBOT;
		}
	}

	pc_setmadogear(sd, flag, static_cast<e_mado_type>(type));

	return SCRIPT_CMD_SUCCESS;
}

/// Sets the save point of the player.
///
/// save "<map name>",<x>,<y>{,{<range x>,<range y>,}<char_id>}
/// savepoint "<map name>",<x>,<y>{,{<range x>,<range y>,}<char_id>}
BUILDIN_FUNC(savepoint)
{
	int x, y, m, cid_pos = 5;
	unsigned short map_idx;
	const char* str;
	TBL_PC* sd;

	if (script_lastdata(st) > 5)
		cid_pos = 7;

	if (!script_charid2sd(cid_pos,sd))
		return SCRIPT_CMD_FAILURE;// no player attached, report source

	str = script_getstr(st, 2);

	map_idx = mapindex_name2id(str);
	if( !map_idx )
		return SCRIPT_CMD_FAILURE;

	x = script_getnum(st,3);
	y = script_getnum(st,4);
	m = map_mapindex2mapid(map_idx);

	if (cid_pos == 7) {
		int dx = script_getnum(st,5), dy = script_getnum(st,6),
			x1 = x + dx, y1 = y + dy,
			x0 = x - dx, y0 = y - dy;
		uint8 n = 10;
		do {
			x = x0 + rnd()%(x1-x0+1);
			y = y0 + rnd()%(y1-y0+1);
		} while (m != -1 && (--n) > 0 && !map_getcell(m, x, y, CELL_CHKPASS));
	}

	// Check for valid coordinates if map in local map-server
	if (m != -1 && !map_getcell(m, x, y, CELL_CHKPASS)) {
		ShowError("buildin_savepoint: Invalid coordinates %d,%d at map %s.\n", x, y, str);
		return SCRIPT_CMD_FAILURE;
	}

	pc_setsavepoint(sd, map_idx, x, y);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * GetTimeTick(0: System Tick, 1: Time Second Tick, 2: Unix epoch)
 *------------------------------------------*/
BUILDIN_FUNC(gettimetick)	/* Asgard Version */
{
	int type;
	time_t timer;
	struct tm *t;

	type=script_getnum(st,2);

	switch(type){
	case 2:
		//type 2:(Get the number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC
		//        from the system clock.)
		script_pushint(st,(int)time(NULL));
		break;
	case 1:
		//type 1:(Second Ticks: 0-86399, 00:00:00-23:59:59)
		time(&timer);
		t=localtime(&timer);
		script_pushint(st,((t->tm_hour)*3600+(t->tm_min)*60+t->tm_sec));
		break;
	case 0:
	default:
		//type 0:(System Ticks)
		script_pushint(st,gettick());
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * GetTime(Type)
 *
 * Returns the current value of a certain date type.
 * Possible types are:
 * - DT_SECOND Current seconds
 * - DT_MINUTE Current minute
 * - DT_HOUR Current hour
 * - DT_DAYOFWEEK Day of current week
 * - DT_DAYOFMONTH Day of current month
 * - DT_MONTH Current month
 * - DT_YEAR Current year
 * - DT_DAYOFYEAR Day of current year
 *
 * If none of the above types is supplied -1 will be returned to the script
 * and the script source will be reported into the mapserver console.
 *------------------------------------------*/
BUILDIN_FUNC(gettime)
{
	int type;

	type = script_getnum(st,2);

	if( type <= DT_MIN || type >= DT_MAX ){
		ShowError( "buildin_gettime: Invalid date type %d\n", type );
		script_reportsrc(st);
		script_pushint(st,-1);
	}else{
		script_pushint(st,date_get((enum e_date_type)type));
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns the current server time or the provided time in a readable format.
 * gettimestr(<"time_format">,<max_length>{,<time_tick>});
 */
BUILDIN_FUNC(gettimestr)
{
	char *tmpstr;
	const char *fmtstr;
	int maxlen;
	time_t now;

	fmtstr = script_getstr(st,2);
	maxlen = script_getnum(st,3);

	if (script_hasdata(st, 4)) {
		if (script_getnum(st, 4) < 0) {
			ShowWarning("buildin_gettimestr: a positive value must be supplied to be valid.\n");
			return SCRIPT_CMD_FAILURE;
		} else
			now = (time_t)script_getnum(st, 4);
	} else
		now = time(NULL);

	tmpstr = (char *)aMalloc((maxlen+1)*sizeof(char));
	strftime(tmpstr,maxlen,fmtstr,localtime(&now));
	tmpstr[maxlen] = '\0';

	script_pushstr(st,tmpstr);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Open player storage
 *------------------------------------------*/
BUILDIN_FUNC(openstorage)
{
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	storage_storageopen(sd);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(guildopenstorage)
{
	TBL_PC* sd;
	int ret;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	ret = storage_guild_storageopen(sd);
	script_pushint(st,ret);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(guildopenstorage_log){
#if PACKETVER < 20140205
	ShowError( "buildin_guildopenstorage_log: This command requires PACKETVER 2014-02-05 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd;

	if( !script_charid2sd( 2, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint( st, storage_guild_log_read( sd ) );

	return SCRIPT_CMD_SUCCESS;
#endif
}

BUILDIN_FUNC(guild_has_permission){
	struct map_session_data* sd;

	if( !script_charid2sd( 3, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	int permission = script_getnum(st,2);

	if( permission == 0 ){
		ShowError( "buildin_guild_has_permission: No permission given.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( ( permission & GUILD_PERM_ALL ) == 0 ){
		ShowError( "buildin_guild_has_permission: Invalid permission '%d'.\n", permission );
		return SCRIPT_CMD_FAILURE;
	}

	if( !sd->guild ){
		script_pushint( st, false );

		return SCRIPT_CMD_SUCCESS;
	}

	int position = guild_getposition(sd);
	
	if( position < 0 || ( sd->guild->position[position].mode&permission ) != permission ){
		script_pushint( st, false );

		return SCRIPT_CMD_SUCCESS;
	}
	
	script_pushint( st, true );

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Make player use a skill trought item usage
 *------------------------------------------*/
/// itemskill <skill id>,<level>
/// itemskill "<skill name>",<level>
BUILDIN_FUNC(itemskill)
{
	int id;
	int lv;
	bool keep_requirement;
	TBL_PC* sd;

	if( !script_rid2sd(sd) || sd->ud.skilltimer != INVALID_TIMER )
		return SCRIPT_CMD_SUCCESS;

	if (script_isstring(st, 2)) {
		const char *name = script_getstr(st, 2);

		if (!(id = skill_name2id(name))) {
			ShowError("buildin_itemskill: Invalid skill name %s passed to item bonus. Skipping.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		id = script_getnum(st, 2);

		if (!skill_get_index(id)) {
			ShowError("buildin_itemskill: Invalid skill ID %d passed to item bonus. Skipping.\n", id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	lv = script_getnum(st,3);
	if (script_hasdata(st, 4)) {
		keep_requirement = (script_getnum(st, 4) != 0);
	} else {
		keep_requirement = false;
	}

	sd->skillitem=id;
	sd->skillitemlv=lv;
	sd->skillitem_keep_requirement = keep_requirement;
	clif_item_skill(sd,id,lv);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * Attempt to create an item
 *------------------------------------------*/
BUILDIN_FUNC(produce)
{
	int trigger;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	trigger=script_getnum(st,2);
	clif_skill_produce_mix_list(sd, -1, trigger);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(cooking)
{
	int trigger;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	trigger=script_getnum(st,2);
	clif_cooking_list(sd, trigger, AM_PHARMACY, 1, 1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Create a pet
 *------------------------------------------*/
BUILDIN_FUNC(makepet)
{
	struct map_session_data* sd;
	uint16 mob_id;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_FAILURE;

	mob_id = script_getnum(st,2);
	std::shared_ptr<s_pet_db> pet = pet_db.find(mob_id);

	if( !pet ){
		ShowError( "buildin_makepet: failed to create a pet with mob id %hu\n", mob_id);
		return SCRIPT_CMD_FAILURE;
	}

	sd->catch_target_class = mob_id;

	std::shared_ptr<s_mob_db> mdb = mob_db.find(pet->class_);

	intif_create_pet( sd->status.account_id, sd->status.char_id, pet->class_, mdb->lv, pet->EggID, 0, pet->intimate, 100, 0, 1, mdb->jname.c_str() );

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Give player exp base,job * quest_exp_rate/100
 * getexp <base xp>,<job xp>{,<char_id>};
 **/
BUILDIN_FUNC(getexp){
	struct map_session_data* sd;

	if( !script_charid2sd( 4, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	int64 base = script_getnum64( st, 2 );

	if( base < 0 ){
		ShowError( "buildin_getexp: Called with negative base exp.\n" );
		return SCRIPT_CMD_FAILURE;
	}
	
	int64 job = script_getnum64( st, 3 );

	if( job < 0 ){
		ShowError( "buildin_getexp: Called with negative job exp.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( base == 0 && job == 0 ){
		ShowError( "buildin_getexp: Called with base and job exp 0.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	// bonus for npc-given exp
	double bonus = battle_config.quest_exp_rate / 100.;

	if (base)
		base = (int64) cap_value(base * bonus, 0, MAX_EXP);
	if (job)
		job = (int64) cap_value(job * bonus, 0, MAX_EXP);

	pc_gainexp(sd, NULL, base, job, 1);
#ifdef RENEWAL
	if (base && sd->hd)
		hom_gainexp(sd->hd, base * battle_config.homunculus_exp_gain / 100); // Homunculus only receive 10% of EXP
#endif

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Gain guild exp [Celest]
 *------------------------------------------*/
BUILDIN_FUNC(guildgetexp){
	struct map_session_data* sd;

	if( !script_rid2sd( sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	int64 exp = script_getnum64( st, 2 );

	if( exp <= 0 ){
		ShowError( "buildin_guildgetexp: Called with exp <= 0.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->status.guild_id <= 0 ){
		ShowError( "buildin_guildgetexp: Called for player %s (AID: %u, CID: %u) without a guild.\n", sd->status.name, sd->status.account_id, sd->status.char_id );
		return SCRIPT_CMD_FAILURE;
	}

	guild_getexp( sd, exp );

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Changes the guild master of a guild [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(guildchangegm)
{
	TBL_PC *sd;
	int guild_id;
	const char *name;

	guild_id = script_getnum(st,2);
	name = script_getstr(st,3);
	sd=map_nick2sd(name,false);

	if (!sd)
		script_pushint(st,0);
	else
		script_pushint(st,guild_gm_change(guild_id, sd->status.char_id));

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Spawn a monster:
 * *monster "<map name>",<x>,<y>,"<name to show>",<mob id>,<amount>{,"<event label>",<size>,<ai>};
 *------------------------------------------*/
BUILDIN_FUNC(monster)
{
	const char* mapn	= script_getstr(st,2);
	int x				= script_getnum(st,3);
	int y				= script_getnum(st,4);
	const char* str		= script_getstr(st,5);
	int class_			= script_getnum(st,6);
	int amount			= script_getnum(st,7);
	const char* event	= "";
	unsigned int size	= SZ_SMALL;
	enum mob_ai ai		= AI_NONE;

	struct map_session_data* sd;
	int16 m;
	int i;

	if (script_hasdata(st, 8)) {
		event = script_getstr(st, 8);
		check_event(st, event);
	}

	if (script_hasdata(st, 9)) {
		size = script_getnum(st, 9);
		if (size > SZ_BIG) {
#ifndef Pandas_ScriptCommand_BossMonster
			ShowWarning("buildin_monster: Attempted to spawn non-existing size %d for monster class %d\n", size, class_);
#else
			ShowWarning("buildin_%s: Attempted to spawn non-existing size %d for monster class %d\n", script_getfuncname(st), size, class_);
#endif // Pandas_ScriptCommand_BossMonster
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (script_hasdata(st, 10)) {
		ai = static_cast<enum mob_ai>(script_getnum(st, 10));
		if (ai >= AI_MAX) {
#ifndef Pandas_ScriptCommand_BossMonster
			ShowWarning("buildin_monster: Attempted to spawn non-existing ai %d for monster class %d\n", ai, class_);
#else
			ShowWarning("buildin_%s: Attempted to spawn non-existing ai %d for monster class %d\n", script_getfuncname(st), ai, class_);
#endif // Pandas_ScriptCommand_BossMonster
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (class_ >= 0 && !mobdb_checkid(class_)) {
#ifndef Pandas_ScriptCommand_BossMonster
		ShowWarning("buildin_monster: Attempted to spawn non-existing monster class %d\n", class_);
#else
		ShowWarning("buildin_%s: Attempted to spawn non-existing monster class %d\n", script_getfuncname(st), class_);
#endif // Pandas_ScriptCommand_BossMonster
		return SCRIPT_CMD_FAILURE;
	}

	sd = map_id2sd(st->rid);

	if (sd && strcmp(mapn, "this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapn);

	for(i = 0; i < amount; i++) { //not optimised
#ifndef Pandas_ScriptCommand_BossMonster
		int mobid = mob_once_spawn(sd, m, x, y, str, class_, 1, event, size, ai);
#else
		int mobid = mob_once_spawn(sd, m, x, y, str, class_, 1, event, size, ai, (!strcmp(script_getfuncname(st), "boss_monster")) ? 1 : 0);
#endif // Pandas_ScriptCommand_BossMonster

		if (mobid)
			mapreg_setreg(reference_uid(add_str("$@mobid"), i), mobid);
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Request List of Monster Drops
 *------------------------------------------*/
BUILDIN_FUNC(getmobdrops)
{
	int class_ = script_getnum(st,2);
	int i, j = 0;

	if( !mobdb_checkid(class_) )
	{
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<s_mob_db> mob = mob_db.find(class_);

	for( i = 0; i < MAX_MOB_DROP_TOTAL; i++ )
	{
		if( mob->dropitem[i].nameid == 0 )
			continue;
		if( itemdb_exists(mob->dropitem[i].nameid) == NULL )
			continue;

		mapreg_setreg(reference_uid(add_str("$@MobDrop_item"), j), mob->dropitem[i].nameid);
		mapreg_setreg(reference_uid(add_str("$@MobDrop_rate"), j), mob->dropitem[i].rate);
		mapreg_setreg(reference_uid(add_str("$@MobDrop_nosteal"), j), mob->dropitem[i].steal_protected);
		mapreg_setreg(reference_uid(add_str("$@MobDrop_randomopt"), j), mob->dropitem[i].randomopt_group);

		j++;
	}

	mapreg_setreg(add_str("$@MobDrop_count"), j);
	script_pushint(st, 1);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Spawn a monster in a random location
 * in x0,x1,y0,y1 area.
 *------------------------------------------*/
BUILDIN_FUNC(areamonster)
{
	const char* mapn	= script_getstr(st,2);
	int x0				= script_getnum(st,3);
	int y0				= script_getnum(st,4);
	int x1				= script_getnum(st,5);
	int y1				= script_getnum(st,6);
	const char* str		= script_getstr(st,7);
	int class_			= script_getnum(st,8);
	int amount			= script_getnum(st,9);
	const char* event	= "";
	unsigned int size	= SZ_SMALL;
	enum mob_ai ai		= AI_NONE;

	struct map_session_data* sd;
	int16 m;
	int i;

	if (script_hasdata(st,10)) {
		event = script_getstr(st, 10);
		check_event(st, event);
	}

	if (script_hasdata(st, 11)) {
		size = script_getnum(st, 11);
		if (size > 3) {
			ShowWarning("buildin_monster: Attempted to spawn non-existing size %d for monster class %d\n", size, class_);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (script_hasdata(st, 12)) {
		ai = static_cast<enum mob_ai>(script_getnum(st, 12));
		if (ai >= AI_MAX) {
			ShowWarning("buildin_monster: Attempted to spawn non-existing ai %d for monster class %d\n", ai, class_);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (class_ >= 0 && !mobdb_checkid(class_)) {
		ShowWarning("buildin_monster: Attempted to spawn non-existing monster class %d\n", class_);
		return SCRIPT_CMD_FAILURE;
	}

	sd = map_id2sd(st->rid);

	if (sd && strcmp(mapn, "this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapn);

	for(i = 0; i < amount; i++) { //not optimised
		int mobid = mob_once_spawn_area(sd, m, x0, y0, x1, y1, str, class_, 1, event, size, ai);

		if (mobid)
			mapreg_setreg(reference_uid(add_str("$@mobid"), i), mobid);
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * KillMonster subcheck, verify if mob to kill ain't got an even to handle, could be force kill by allflag
 *------------------------------------------*/
 static int buildin_killmonster_sub_strip(struct block_list *bl,va_list ap)
{ //same fix but with killmonster instead - stripping events from mobs.
	TBL_MOB* md = (TBL_MOB*)bl;
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	md->state.npc_killmonster = 1;

	if(!allflag){
		if(strcmp(event,md->npc_event)==0)
			status_kill(bl);
	}else{
		if(!md->spawn)
			status_kill(bl);
	}
	md->state.npc_killmonster = 0;
	return SCRIPT_CMD_SUCCESS;
}

static int buildin_killmonster_sub(struct block_list *bl,va_list ap)
{
	TBL_MOB* md = (TBL_MOB*)bl;
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	if(!allflag){
		if(strcmp(event,md->npc_event)==0)
			status_kill(bl);
	}else{
		if(!md->spawn)
			status_kill(bl);
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(killmonster)
{
	const char *mapname,*event;
	int16 m,allflag=0;
	mapname=script_getstr(st,2);
	event=script_getstr(st,3);
	if(strcmp(event,"All")==0)
		allflag = 1;
	else
		check_event(st, event);

	if( (m=map_mapname2mapid(mapname))<0 )
		return SCRIPT_CMD_SUCCESS;

	if( script_hasdata(st,4) ) {
		if ( script_getnum(st,4) == 1 ) {
			map_foreachinmap(buildin_killmonster_sub, m, BL_MOB, event ,allflag);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	map_freeblock_lock();
	map_foreachinmap(buildin_killmonster_sub_strip, m, BL_MOB, event ,allflag);
	map_freeblock_unlock();
	return SCRIPT_CMD_SUCCESS;
}

static int buildin_killmonsterall_sub_strip(struct block_list *bl,va_list ap)
{ //Strips the event from the mob if it's killed the old method.
	struct mob_data *md;

	md = BL_CAST(BL_MOB, bl);
	if (md->npc_event[0])
		md->npc_event[0] = 0;

	status_kill(bl);
	return 0;
}
static int buildin_killmonsterall_sub(struct block_list *bl,va_list ap)
{
	status_kill(bl);
	return 0;
}
BUILDIN_FUNC(killmonsterall)
{
	const char *mapname;
	int16 m;
	mapname=script_getstr(st,2);

	if( (m = map_mapname2mapid(mapname))<0 )
		return SCRIPT_CMD_SUCCESS;

	if( script_hasdata(st,3) ) {
		if ( script_getnum(st,3) == 1 ) {
			map_foreachinmap(buildin_killmonsterall_sub,m,BL_MOB);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	map_foreachinmap(buildin_killmonsterall_sub_strip,m,BL_MOB);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Creates a clone of a player.
 * clone map, x, y, event, char_id, master_id, mode, flag, duration
 *------------------------------------------*/
BUILDIN_FUNC(clone)
{
	TBL_PC *sd, *msd=NULL;
	uint32 char_id, master_id = 0, x, y, flag = 0;
	int16 m;
	enum e_mode mode = MD_NONE;

	unsigned int duration = 0;
	const char *mapname,*event;

	mapname=script_getstr(st,2);
	x=script_getnum(st,3);
	y=script_getnum(st,4);
	event=script_getstr(st,5);
	char_id=script_getnum(st,6);

	if( script_hasdata(st,7) )
		master_id=script_getnum(st,7);

	if( script_hasdata(st,8) )
		mode=static_cast<e_mode>(script_getnum(st,8));

	if( script_hasdata(st,9) )
		flag=script_getnum(st,9);

	if( script_hasdata(st,10) )
		duration=script_getnum(st,10);

	check_event(st, event);

	m = map_mapname2mapid(mapname);
	if (m < 0)
		return SCRIPT_CMD_SUCCESS;

	sd = map_charid2sd(char_id);

	if (master_id) {
		msd = map_charid2sd(master_id);
		if (msd)
			master_id = msd->bl.id;
		else
			master_id = 0;
	}
	if (sd) //Return ID of newly crafted clone.
		script_pushint(st,mob_clone_spawn(sd, m, x, y, event, master_id, mode, flag, 1000*duration));
	else //Failed to create clone.
		script_pushint(st,0);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(doevent)
{
	const char* event;
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	event = script_getstr(st,2);

	check_event(st, event);
	npc_event(sd, event, 0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(donpcevent)
{
	const char* event = script_getstr(st,2);
	check_event(st, event);
	if( !npc_event_do(event) ) {
		struct npc_data * nd = map_id2nd(st->oid);
		ShowDebug("NPCEvent '%s' not found! (source: %s)\n",event,nd?nd->name:"Unknown");
		script_pushint(st, 0);
	} else
		script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}

/// for Aegis compatibility
/// basically a specialized 'donpcevent', with the event specified as two arguments instead of one
BUILDIN_FUNC(cmdothernpc)	// Added by RoVeRT
{
	const char* npc = script_getstr(st,2);
	const char* command = script_getstr(st,3);
	char event[EVENT_NAME_LENGTH];

	safesnprintf(event,EVENT_NAME_LENGTH, "%s::%s%s",npc,script_config.oncommand_event_name,command);
	check_event(st, event);

	if( npc_event_do(event) ){
		script_pushint(st, true);
	}else{
		struct npc_data * nd = map_id2nd(st->oid);
		ShowDebug("NPCEvent '%s' not found! (source: %s)\n", event, nd ? nd->name : "Unknown");
		script_pushint(st, false);
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(addtimer)
{
	int tick;
	const char* event;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	tick = script_getnum(st,2);
	event = script_getstr(st, 3);

	check_event(st, event);

	if (!pc_addeventtimer(sd,tick,event)) {
		ShowWarning("buildin_addtimer: Event timer is full, can't add new event timer. (cid:%d timer:%s)\n",sd->status.char_id,event);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(deltimer)
{
	const char *event;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	event=script_getstr(st, 2);

	check_event(st, event);
	pc_deleventtimer(sd,event);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(addtimercount)
{
	const char *event;
	int tick;
	TBL_PC* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	tick=script_getnum(st,2);
	event=script_getstr(st,3);

	check_event(st, event);
	pc_addeventtimercount(sd,event,tick);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(initnpctimer)
{
	struct npc_data *nd;
	int flag = 0;

	if( script_hasdata(st,3) )
	{	//Two arguments: NPC name and attach flag.
		nd = npc_name2id(script_getstr(st, 2));
		flag = script_getnum(st,3);
	}
	else if( script_hasdata(st,2) )
	{	//Check if argument is numeric (flag) or string (npc name)
		if( script_isstring(st, 2) ) //NPC name
			nd = npc_name2id(script_getstr(st, 2));
		else //Flag
		{
			nd = (struct npc_data *)map_id2bl(st->oid);
			flag = script_getnum(st, 2);
		}
	}
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( !nd )
		return SCRIPT_CMD_SUCCESS;
	if( flag ) //Attach
	{
		TBL_PC* sd;
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;
		nd->u.scr.rid = sd->bl.id;
	}

	nd->u.scr.timertick = 0;
	npc_settimerevent_tick(nd,0);
	npc_timerevent_start(nd, st->rid);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(startnpctimer)
{
	struct npc_data *nd;
	int flag = 0;

	if( script_hasdata(st,3) )
	{	//Two arguments: NPC name and attach flag.
		nd = npc_name2id(script_getstr(st, 2));
		flag = script_getnum(st,3);
	}
	else if( script_hasdata(st,2) )
	{	//Check if argument is numeric (flag) or string (npc name)
		if( script_isstring(st, 2) ) //NPC name
			nd = npc_name2id(script_getstr(st, 2));
		else //Flag
		{
			nd = (struct npc_data *)map_id2bl(st->oid);
			flag = script_getnum(st, 2);
		}
	}
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if( !nd )
		return SCRIPT_CMD_SUCCESS;
	if( flag ) //Attach
	{
		TBL_PC* sd;
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;
		nd->u.scr.rid = sd->bl.id;
	}

	npc_timerevent_start(nd, st->rid);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(stopnpctimer)
{
	struct npc_data *nd;
	int flag = 0;

	if( script_hasdata(st,3) )
	{	//Two arguments: NPC name and attach flag.
		nd = npc_name2id(script_getstr(st, 2));
		flag = script_getnum(st,3);
	}
	else if( script_hasdata(st,2) )
	{	//Check if argument is numeric (flag) or string (npc name)
		if( script_isstring(st, 2) ) //NPC name
			nd = npc_name2id(script_getstr(st, 2));
		else //Flag
		{
			nd = (struct npc_data *)map_id2bl(st->oid);
			flag = script_getnum(st, 2);
		}
	}
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if( !nd )
		return SCRIPT_CMD_SUCCESS;
	if( flag ) //Detach
		nd->u.scr.rid = 0;

	npc_timerevent_stop(nd);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(getnpctimer)
{
	struct npc_data *nd;
	TBL_PC *sd;
	int type = script_getnum(st,2);
	t_tick val = 0;

	if( script_hasdata(st,3) )
		nd = npc_name2id(script_getstr(st,3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( !nd || nd->bl.type != BL_NPC )
	{
		script_pushint(st,0);
		ShowError("getnpctimer: Invalid NPC.\n");
		return SCRIPT_CMD_FAILURE;
	}

	switch( type )
	{
	case 0: val = npc_gettimerevent_tick(nd); break;
	case 1:
		if( nd->u.scr.rid )
		{
			sd = map_id2sd(nd->u.scr.rid);
			if( !sd )
			{
				ShowError("buildin_getnpctimer: Attached player not found!\n");
				break;
			}
			val = (sd->npc_timer_id != INVALID_TIMER);
		}
		else
			val = (nd->u.scr.timerid != INVALID_TIMER);
		break;
	case 2: val = nd->u.scr.timeramount; break;
	}

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(setnpctimer)
{
	int tick;
	struct npc_data *nd;

	tick = script_getnum(st,2);
	if( script_hasdata(st,3) )
		nd = npc_name2id(script_getstr(st,3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( !nd || nd->bl.type != BL_NPC )
	{
		script_pushint(st,1);
		ShowError("setnpctimer: Invalid NPC.\n");
		return SCRIPT_CMD_FAILURE;
	}

	npc_settimerevent_tick(nd,tick);
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * attaches the player rid to the timer [Celest]
 *------------------------------------------*/
BUILDIN_FUNC(attachnpctimer)
{
	TBL_PC *sd;
	struct npc_data *nd = (struct npc_data *)map_id2bl(st->oid);

	if( !nd || nd->bl.type != BL_NPC )
	{
		script_pushint(st,1);
		ShowError("setnpctimer: Invalid NPC.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if( !script_nick2sd(2,sd) ){
		script_pushint(st,1);
		ShowWarning("attachnpctimer: Invalid player.\n");
		return SCRIPT_CMD_FAILURE;
	}

	nd->u.scr.rid = sd->bl.id;
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * detaches a player rid from the timer [Celest]
 *------------------------------------------*/
BUILDIN_FUNC(detachnpctimer)
{
	struct npc_data *nd;

	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st,2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( !nd || nd->bl.type != BL_NPC )
	{
		script_pushint(st,1);
		ShowError("detachnpctimer: Invalid NPC.\n");
		return SCRIPT_CMD_FAILURE;
	}

	nd->u.scr.rid = 0;
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * To avoid "player not attached" script errors, this function is provided,
 * it checks if there is a player attached to the current script. [Skotlex]
 * If no, returns 0, if yes, returns the account_id of the attached player.
 *------------------------------------------*/
BUILDIN_FUNC(playerattached)
{
	if(st->rid == 0 || map_id2sd(st->rid) == NULL)
		script_pushint(st,0);
	else
		script_pushint(st,st->rid);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(announce)
{
	const char *mes       = script_getstr(st,2);
	int         flag      = script_getnum(st,3);
	const char *fontColor = script_hasdata(st,4) ? script_getstr(st,4) : NULL;
	int         fontType  = script_hasdata(st,5) ? script_getnum(st,5) : FW_NORMAL; // default fontType
	int         fontSize  = script_hasdata(st,6) ? script_getnum(st,6) : 12;    // default fontSize
	int         fontAlign = script_hasdata(st,7) ? script_getnum(st,7) : 0;     // default fontAlign
	int         fontY     = script_hasdata(st,8) ? script_getnum(st,8) : 0;     // default fontY

#ifndef Pandas_ScriptCommand_Announce
	if (flag&(BC_TARGET_MASK|BC_SOURCE_MASK)) // Broadcast source or broadcast region defined
#else
	if (flag&(BC_TARGET_MASK|BC_SOURCE_MASK|BC_NAME)) // Broadcast source or broadcast region defined
#endif // Pandas_ScriptCommand_Announce
	{
		send_target target;
		struct block_list *bl;

		// If bc_npc flag is set, use NPC as broadcast source
		if(flag&BC_NPC){
			bl = map_id2bl(st->oid);
		}else{
			struct map_session_data* sd;

			if(script_charid2sd(9, sd))
				bl = &sd->bl;
			else
				bl = NULL;
		}
		
		if (bl == NULL)
			return SCRIPT_CMD_SUCCESS;

		switch (flag&BC_TARGET_MASK) {
			case BC_MAP:	target = ALL_SAMEMAP;	break;
			case BC_AREA:	target = AREA;			break;
			case BC_SELF:	target = SELF;			break;
			default:		target = ALL_CLIENT;	break; // BC_ALL
		}

#ifndef Pandas_ScriptCommand_Announce
		if (fontColor)
			clif_broadcast2(bl, mes, (int)strlen(mes)+1, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY, target);
		else
			clif_broadcast(bl, mes, (int)strlen(mes)+1, flag&BC_COLOR_MASK, target);
#else
		if (flag & BC_NAME && bl && bl->type == BL_PC) {
			// 若携带了 BC_NAME 标记位, 则强制只能走 clif_broadcast
			char output[CHAT_SIZE_MAX] = { 0 };

			// 若没有指定颜色, 则指定黄色作为默认色
			if (!fontColor)
				fontColor = "0xFFFF00";

			// 将颜色代码放到文本信息的最开头
			sprintf(output, "%06lx%s", strtol(fontColor, (char**)NULL, 0), mes);

			// 调用 clif_broadcast 将信息发送给客户端
			clif_broadcast(bl, output, (int)strlen(output) + 1, flag, target);
		}
		else {
			// 在原始流程中如果携带了字体颜色则走 clif_broadcast2 否则走 clif_broadcast
			if (fontColor)
				clif_broadcast2(bl, mes, (int)strlen(mes) + 1, strtol(fontColor, (char**)NULL, 0), fontType, fontSize, fontAlign, fontY, target);
			else
				clif_broadcast(bl, mes, (int)strlen(mes) + 1, flag & BC_COLOR_MASK, target);
		}
#endif // Pandas_ScriptCommand_Announce
	}
	else
	{
		if (fontColor)
			intif_broadcast2(mes, (int)strlen(mes)+1, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY);
		else
			intif_broadcast(mes, (int)strlen(mes)+1, flag&BC_COLOR_MASK);
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
static int buildin_announce_sub(struct block_list *bl, va_list ap)
{
	char *mes       = va_arg(ap, char *);
	int   len       = va_arg(ap, int);
	int   type      = va_arg(ap, int);
	char *fontColor = va_arg(ap, char *);
	short fontType  = (short)va_arg(ap, int);
	short fontSize  = (short)va_arg(ap, int);
	short fontAlign = (short)va_arg(ap, int);
	short fontY     = (short)va_arg(ap, int);
	if (fontColor)
		clif_broadcast2(bl, mes, len, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY, SELF);
	else
		clif_broadcast(bl, mes, len, type, SELF);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mapannounce)
{
	const char *mapname   = script_getstr(st,2);
	const char *mes       = script_getstr(st,3);
	int         flag      = script_getnum(st,4);
	const char *fontColor = script_hasdata(st,5) ? script_getstr(st,5) : NULL;
	int         fontType  = script_hasdata(st,6) ? script_getnum(st,6) : FW_NORMAL; // default fontType
	int         fontSize  = script_hasdata(st,7) ? script_getnum(st,7) : 12;    // default fontSize
	int         fontAlign = script_hasdata(st,8) ? script_getnum(st,8) : 0;     // default fontAlign
	int         fontY     = script_hasdata(st,9) ? script_getnum(st,9) : 0;     // default fontY
	int16 m;

	if ((m = map_mapname2mapid(mapname)) < 0)
		return SCRIPT_CMD_SUCCESS;

	map_foreachinmap(buildin_announce_sub, m, BL_PC,
			mes, strlen(mes)+1, flag&BC_COLOR_MASK, fontColor, fontType, fontSize, fontAlign, fontY);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(areaannounce)
{
	const char *mapname   = script_getstr(st,2);
	int         x0        = script_getnum(st,3);
	int         y0        = script_getnum(st,4);
	int         x1        = script_getnum(st,5);
	int         y1        = script_getnum(st,6);
	const char *mes       = script_getstr(st,7);
	int         flag      = script_getnum(st,8);
	const char *fontColor = script_hasdata(st,9) ? script_getstr(st,9) : NULL;
	int         fontType  = script_hasdata(st,10) ? script_getnum(st,10) : FW_NORMAL; // default fontType
	int         fontSize  = script_hasdata(st,11) ? script_getnum(st,11) : 12;    // default fontSize
	int         fontAlign = script_hasdata(st,12) ? script_getnum(st,12) : 0;     // default fontAlign
	int         fontY     = script_hasdata(st,13) ? script_getnum(st,13) : 0;     // default fontY
	int16 m;

	if ((m = map_mapname2mapid(mapname)) < 0)
		return SCRIPT_CMD_SUCCESS;

	map_foreachinallarea(buildin_announce_sub, m, x0, y0, x1, y1, BL_PC,
		mes, strlen(mes)+1, flag&BC_COLOR_MASK, fontColor, fontType, fontSize, fontAlign, fontY);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(getusers)
{
	int flag, val = 0;
	struct map_session_data* sd;
	struct block_list* bl = NULL;

	flag = script_getnum(st,2);

	switch(flag&0x07)
	{
		case 0:
			if(flag&0x8)
			{// npc
				bl = map_id2bl(st->oid);
			}
			else if(script_rid2sd(sd))
			{// pc
				bl = &sd->bl;
			}

			if(bl)
			{
				val = map_getmapdata(bl->m)->users;
			}
			break;
		case 1:
			val = map_getusers();
			break;
		default:
			ShowWarning("buildin_getusers: Unknown type %d.\n", flag);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getmapguildusers("mapname",guild ID) Returns the number guild members present on a map [Reddozen]
 *------------------------------------------*/
BUILDIN_FUNC(getmapguildusers)
{
	const char *str;
	int16 m;
	int gid;
	int c=0;
	struct guild *g = NULL;
	str=script_getstr(st,2);
	gid=script_getnum(st,3);
	if ((m = map_mapname2mapid(str)) < 0) { // map id on this server (m == -1 if not in actual map-server)
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}
	g = guild_search(gid);

	if (g){
		unsigned short i;
		for(i = 0; i < g->max_member; i++)
		{
			if (g->member[i].sd && g->member[i].sd->bl.m == m)
				c++;
		}
	}

	script_pushint(st,c);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(getmapusers)
{
	const char *str;
	int16 m;
	str=script_getstr(st,2);
	if( (m=map_mapname2mapid(str))< 0){
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}
	script_pushint(st,map_getmapdata(m)->users);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 *------------------------------------------*/
static int buildin_getareausers_sub(struct block_list *bl,va_list ap)
{
	int *users = va_arg(ap, int *);
	(*users)++;
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getareausers)
{
	const char *str;
	int16 m,x0,y0,x1,y1;
	int users = 0;
	str=script_getstr(st,2);
	x0=script_getnum(st,3);
	y0=script_getnum(st,4);
	x1=script_getnum(st,5);
	y1=script_getnum(st,6);
	if( (m=map_mapname2mapid(str))< 0){
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}
	map_foreachinallarea(buildin_getareausers_sub,
		m,x0,y0,x1,y1,BL_PC,&users);
	script_pushint(st,users);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * getunits(<type>{,<array_variable>[<first value>]})
 * getmapunits(<type>,<"map name">{,<array_variable>[<first value>]})
 * getareaunits(<type>,<"map name">,<x1>,<y1>,<x2>,<y2>{,<array_variable>[<first value>]})
 *------------------------------------------*/
BUILDIN_FUNC(getunits)
{
	struct block_list *bl = NULL;
	struct map_session_data *sd = NULL;
	struct script_data *data = NULL;
	char *command = (char *)script_getfuncname(st);
	const char *str;
	const char *name;
	int type = script_getnum(st, 2);
	int size = 0;
	int32 idx, id;
	int16 m = -1, x0 = 0, y0 = 0, x1 = 0, y1 = 0;

	if (!strcmp(command, "getmapunits"))
	{
		str = script_getstr(st, 3);
		if ((m = map_mapname2mapid(str)) < 0) {
			script_pushint(st, -1);
			st->state = END;
			ShowWarning("buildin_%s: Unknown map '%s'.\n", command, str);
			return SCRIPT_CMD_FAILURE;
		}
		if (script_hasdata(st, 4))
			data = script_getdata(st, 4);
	}
	else if (!strcmp(command, "getareaunits"))
	{
		str = script_getstr(st, 3);
		if ((m = map_mapname2mapid(str)) < 0) {
			script_pushint(st, -1);
			st->state = END;
			ShowWarning("buildin_%s: Unknown map '%s'.\n", command, str);
			return SCRIPT_CMD_FAILURE;
		}
		x0 = min(script_getnum(st, 4), script_getnum(st, 6));
		y0 = min(script_getnum(st, 5), script_getnum(st, 7));
		x1 = max(script_getnum(st, 4), script_getnum(st, 6));
		y1 = max(script_getnum(st, 5), script_getnum(st, 7));

		if (script_hasdata(st, 8))
			data = script_getdata(st, 8);
	}
	else
	{
		if (script_hasdata(st, 3))
			data = script_getdata(st, 3);
	}

	if (data)
	{
		if (!data_isreference(data))
		{
			ShowError("buildin_%s: not a variable\n", command);
			script_reportdata(data);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
		id = reference_getid(data);
		idx = reference_getindex(data);
		name = reference_getname(data);

		if (not_server_variable(*name) && !script_rid2sd(sd)) {
			ShowError("buildin_%s: Cannot use a player variable '%s' if no player is attached.\n", command, name);
			return SCRIPT_CMD_FAILURE;
		}
	}

	struct s_mapiterator *iter = mapit_alloc(MAPIT_NORMAL, bl_type(type));
	for (bl = (struct block_list*)mapit_first(iter); mapit_exists(iter); bl = (struct block_list*)mapit_next(iter))
	{
		if (m == -1 || (m == bl->m && !x0 && !y0 && !x1 && !y1) || (bl->m == m && (bl->x >= x0 && bl->y >= y0) && (bl->x <= x1 && bl->y <= y1)))
		{
			if( data ){
				if( is_string_variable( name ) ){
					set_reg_str( st, sd, reference_uid( id, idx + size ), name, status_get_name( bl ), reference_getref( data ) );
				}else{
					set_reg_num( st, sd, reference_uid( id, idx + size ), name, bl->id, reference_getref( data ) );
				}
			}

			size++;
		}
	}

	mapit_free(iter);

	script_pushint(st, size);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
static int buildin_getareadropitem_sub(struct block_list *bl,va_list ap)
{
	t_itemid nameid = va_arg(ap, t_itemid);
	unsigned short *amount = (unsigned short *)va_arg(ap, int *);
	struct flooritem_data *drop=(struct flooritem_data *)bl;

	if(drop->item.nameid==nameid)
		(*amount)+=drop->item.amount;

	return SCRIPT_CMD_SUCCESS;
}
BUILDIN_FUNC(getareadropitem)
{
	const char *str;
	int16 m,x0,y0,x1,y1;
	t_itemid nameid;
	unsigned short amount = 0;

	str=script_getstr(st,2);
	x0=script_getnum(st,3);
	y0=script_getnum(st,4);
	x1=script_getnum(st,5);
	y1=script_getnum(st,6);

	if( script_isstring(st, 7) ){
		const char *name = script_getstr(st, 7);
		std::shared_ptr<item_data> item_data = item_db.searchname( name );

		if( item_data )
			nameid=item_data->nameid;
		else{
			ShowError( "buildin_getareadropitem: Unknown item %s\n", name );
			return SCRIPT_CMD_FAILURE;
		}
	}else{
		nameid = script_getnum(st, 7);

		if( !itemdb_exists( nameid ) ){
			ShowError( "buildin_getareadropitem: Unknown item id %u\n", nameid );
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( (m=map_mapname2mapid(str))< 0){
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}
	map_foreachinallarea(buildin_getareadropitem_sub,
		m,x0,y0,x1,y1,BL_ITEM,nameid,&amount);
	script_pushint(st,amount);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(enablenpc)
{
	npc_data* nd;
	e_npcv_status flag = NPCVIEW_DISABLE;
	const char* command = script_getfuncname(st);

	if (script_hasdata(st, 2))
		nd = npc_name2id(script_getstr(st,2));
	else
		nd = map_id2nd(st->oid);

	if (!strcmp(command,"enablenpc"))
		flag = NPCVIEW_ENABLE;
	else if (!strcmp(command,"disablenpc"))
		flag = NPCVIEW_DISABLE;
	else if (!strcmp(command,"hideoffnpc"))
		flag = NPCVIEW_HIDEOFF;
	else if (!strcmp(command,"hideonnpc"))
		flag = NPCVIEW_HIDEON;
	else if (!strcmp(command,"cloakoffnpc") || !strcmp(command,"cloakoffnpcself"))
		flag = NPCVIEW_CLOAKOFF;
	else if (!strcmp(command,"cloakonnpc") || !strcmp(command,"cloakonnpcself"))
		flag = NPCVIEW_CLOAKON;
	else{
		ShowError( "buildin_enablenpc: Undefined command \"%s\".\n", command );
		return SCRIPT_CMD_FAILURE;
	}

	if (!nd) {
		if (script_hasdata(st, 2))
			ShowError("buildin_%s: Attempted to %s a non-existing NPC '%s' (flag=%d).\n", (flag & NPCVIEW_VISIBLE) ? "show" : "hide", command, script_getstr(st,2), flag);
		else
			ShowError("buildin_%s: Attempted to %s a non-existing NPC (flag=%d).\n", (flag & NPCVIEW_VISIBLE) ? "show" : "hide", command, flag);
		return SCRIPT_CMD_FAILURE;
	}

	int char_id = 0;

	if (script_hasdata(st, 3))
		char_id = script_getnum(st, 3);
	else if (!strcmp(command,"cloakoffnpcself") || !strcmp(command,"cloakonnpcself")) {
		map_session_data* sd;

		if (!script_rid2sd(sd))
			return SCRIPT_CMD_SUCCESS;

		char_id = sd->status.char_id;
	}

	if (npc_enable_target(*nd, char_id, flag))
		return SCRIPT_CMD_SUCCESS;

	return SCRIPT_CMD_FAILURE;
}

/* Starts a status effect on the target unit or on the attached player.
 *
 * sc_start  <effect_id>,<duration>,<val1>{,<rate>,<flag>,{<unit_id>}};
 * sc_start2 <effect_id>,<duration>,<val1>,<val2>{,<rate,<flag>,{<unit_id>}};
 * sc_start4 <effect_id>,<duration>,<val1>,<val2>,<val3>,<val4>{,<rate,<flag>,{<unit_id>}};
 * <flag>: enum e_status_change_start_flags
 */
BUILDIN_FUNC(sc_start)
{
	TBL_NPC * nd = map_id2nd(st->oid);
	struct block_list* bl;
	enum sc_type type;
	int tick, val1, val2, val3, val4=0, rate, flag;
	char start_type;
	const char* command = script_getfuncname(st);

	if(strstr(command, "4"))
		start_type = 4;
	else if(strstr(command, "2"))
		start_type = 2;
	else
		start_type = 1;

	type = (sc_type)script_getnum(st,2);
	tick = script_getnum(st,3);
	val1 = script_getnum(st,4);

	//If from NPC we make default flag 1 to be unavoidable
	if(nd && nd->bl.id == fake_nd->bl.id)
		flag = script_hasdata(st,5+start_type)?script_getnum(st,5+start_type):SCSTART_NOTICKDEF;
	else
		flag = script_hasdata(st,5+start_type)?script_getnum(st,5+start_type):SCSTART_NOAVOID;

	rate = script_hasdata(st,4+start_type)?min(script_getnum(st,4+start_type),10000):10000;

	if(script_hasdata(st,(6+start_type)))
		bl = map_id2bl(script_getnum(st,(6+start_type)));
	else
		bl = map_id2bl(st->rid);

	uint16 skill_id;

	if(tick == 0 && val1 > 0 && type > SC_NONE && type < SC_MAX && (skill_id = status_db.getSkill(type)) > 0)
	{// When there isn't a duration specified, try to get it from the skill_db
		tick = skill_get_time(skill_id, val1);
	}

	if(potion_flag == 1 && potion_target) { //skill.cpp set the flags before running the script, this is a potion-pitched effect.
		bl = map_id2bl(potion_target);
		tick /= 2;// Thrown potions only last half.
		val4 = 1;// Mark that this was a thrown sc_effect
	}

	if(!bl)
		return SCRIPT_CMD_SUCCESS;

	switch(start_type) {
		case 1:
			status_change_start(bl, bl, type, rate, val1, 0, 0, val4, tick, flag);
			break;
		case 2:
			val2 = script_getnum(st,5);
			status_change_start(bl, bl, type, rate, val1, val2, 0, val4, tick, flag);
			break;
		case 4:
			val2 = script_getnum(st,5);
			val3 = script_getnum(st,6);
			val4 = script_getnum(st,7);
			status_change_start(bl, bl, type, rate, val1, val2, val3, val4, tick, flag);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Ends one or all status effects on the target unit or on the attached player.
///
/// sc_end <effect_id>{,<unit_id>};
BUILDIN_FUNC(sc_end)
{
	struct block_list* bl;
	int type;

	type = script_getnum(st, 2);
	if (script_hasdata(st, 3))
		bl = map_id2bl(script_getnum(st, 3));
	else
		bl = map_id2bl(st->rid);

	if (potion_flag == 1 && potion_target) //##TODO how does this work [FlavioJS]
		bl = map_id2bl(potion_target);

	if (!bl)
		return SCRIPT_CMD_SUCCESS;

	if (type > SC_NONE && type < SC_MAX) {
		struct status_change *sc = status_get_sc(bl);

		if (sc == nullptr)
			return SCRIPT_CMD_SUCCESS;

		struct status_change_entry *sce = sc->data[type];

		if (sce == nullptr)
			return SCRIPT_CMD_SUCCESS;

		std::shared_ptr<s_status_change_db> sc_db = status_db.find( type );

		if( sc_db == nullptr ){
			ShowError( "buildin_sc_end: Unknown status change %d.\n", type );
			return SCRIPT_CMD_FAILURE;
		}

		if( sc_db->flag[SCF_NOCLEARBUFF] && sc_db->flag[SCF_NOFORCEDEND] ){
			ShowError( "buildin_sc_end: Status %d cannot be cleared.\n", type );
			return SCRIPT_CMD_FAILURE;
		}

		//This should help status_change_end force disabling the SC in case it has no limit.
		sce->val1 = sce->val2 = sce->val3 = sce->val4 = 0;
		status_change_end(bl, (sc_type)type, INVALID_TIMER);
	} else
		status_change_clear(bl, 3); // remove all effects

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Ends all status effects from any learned skill on the attached player.
 * if <job_id> was given it will end the effect of that class for the attached player
 * sc_end_class {<char_id>{,<job_id>}};
 */
BUILDIN_FUNC(sc_end_class)
{
	struct map_session_data *sd;
	int class_;

	if (!script_charid2sd(2, sd))
		return SCRIPT_CMD_FAILURE;

	if (script_hasdata(st, 3))
		class_ = script_getnum(st, 3);
	else
		class_ = sd->status.class_;

	if (!pcdb_checkid(class_)) {
		ShowError("buildin_sc_end_class: Invalid job ID '%d' given.\n", script_getnum(st, 3));
		return SCRIPT_CMD_FAILURE;
	}

	status_db.changeSkillTree(sd, class_);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * @FIXME atm will return reduced tick, 0 immune, 1 no tick
 *------------------------------------------*/
BUILDIN_FUNC(getscrate)
{
	struct block_list *bl;
	int type;
	t_tick rate;

	type=script_getnum(st,2);
	rate=script_getnum(st,3);
	if( script_hasdata(st,4) ) //get for the bl assigned
		bl = map_id2bl(script_getnum(st,4));
	else
		bl = map_id2bl(st->rid);

	if (bl)
		rate = status_get_sc_def(NULL,bl, (sc_type)type, 10000, 10000, SCSTART_NONE);

	script_pushint(st,rate);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getstatus(<effect type>{,<type>{,<char_id>}});
 **/
BUILDIN_FUNC(getstatus)
{
	int id, type;
	struct map_session_data* sd;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	id = script_getnum(st, 2);
	type = script_hasdata(st, 3) ? script_getnum(st, 3) : 0;

	if( id <= SC_NONE || id >= SC_MAX )
	{// invalid status type given
		ShowWarning("script.cpp:getstatus: Invalid status type given (%d).\n", id);
		return SCRIPT_CMD_SUCCESS;
	}

	if( sd->sc.count == 0 || !sd->sc.data[id] )
	{// no status is active
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch( type )
	{
		case 1:	 script_pushint(st, sd->sc.data[id]->val1);	break;
		case 2:  script_pushint(st, sd->sc.data[id]->val2);	break;
		case 3:  script_pushint(st, sd->sc.data[id]->val3);	break;
		case 4:  script_pushint(st, sd->sc.data[id]->val4);	break;
		case 5:
			{
				struct TimerData* timer = (struct TimerData*)get_timer(sd->sc.data[id]->timer);

				if( timer )
				{// return the amount of time remaining
					script_pushint(st, timer->tick - gettick());
				} else {
					script_pushint(st, -1);
				}
			}
			break;
		default: script_pushint(st, 1); break;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(debugmes)
{
	const char *str;
	str=script_getstr(st,2);
	ShowDebug("script debug : %d %d : %s\n",st->rid,st->oid,str);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( errormes ){
	ShowError( "%s\n", script_getstr( st, 2 ) );
	script_reportsrc( st );

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *------------------------------------------*/
BUILDIN_FUNC(catchpet)
{
	int pet_id;
	TBL_PC *sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	pet_id= script_getnum(st,2);

	pet_catch_process1(sd,pet_id);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * [orn]
 *------------------------------------------*/
BUILDIN_FUNC(homunculus_evolution)
{
	TBL_PC *sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	if(hom_is_active(sd->hd))
	{
		if (sd->hd->homunculus.intimacy >= battle_config.homunculus_evo_intimacy_need)
			hom_evolution(sd->hd);
		else
			clif_emotion(&sd->hd->bl, ET_SWEAT);
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Checks for vaporized morph state
 * and deletes ITEMID_STRANGE_EMBRYO.
 *------------------------------------------*/
BUILDIN_FUNC(homunculus_mutate)
{
	int homun_id;
	TBL_PC *sd;

	if( !script_rid2sd(sd) || sd->hd == NULL )
		return SCRIPT_CMD_SUCCESS;

	if(script_hasdata(st,2))
		homun_id = script_getnum(st,2);
	else
		homun_id = 6048 + (rnd() % 4);

	if( sd->hd->homunculus.vaporize == HOM_ST_MORPH ) {
		int m_class = hom_class2mapid(sd->hd->homunculus.class_);
		int m_id = hom_class2mapid(homun_id);
		short i = pc_search_inventory(sd, ITEMID_STRANGE_EMBRYO);

		if ( m_class != -1 && m_id != -1 && m_class&HOM_EVO && m_id&HOM_S && sd->hd->homunculus.level >= 99 && i >= 0 ) {
			sd->hd->homunculus.vaporize = HOM_ST_REST; // Remove morph state.
			hom_call(sd); // Respawn homunculus.
			hom_mutate(sd->hd, homun_id);
			pc_delitem(sd, i, 1, 0, 0, LOG_TYPE_SCRIPT);
			script_pushint(st, 1);
			return SCRIPT_CMD_SUCCESS;
		} else
			clif_emotion(&sd->bl, ET_SWEAT);
	} else
		clif_emotion(&sd->bl, ET_SWEAT);

	script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Puts homunculus into morph state
 * and gives ITEMID_STRANGE_EMBRYO.
 *------------------------------------------*/
BUILDIN_FUNC(morphembryo)
{
	struct item item_tmp;
	TBL_PC *sd;

	if( !script_rid2sd(sd) || sd->hd == NULL )
		return SCRIPT_CMD_SUCCESS;

	if( hom_is_active(sd->hd) ) {
		int m_class = hom_class2mapid(sd->hd->homunculus.class_);

		if ( m_class != -1 && m_class&HOM_EVO && sd->hd->homunculus.level >= 99 ) {
			char i;
			memset(&item_tmp, 0, sizeof(item_tmp));
			item_tmp.nameid = ITEMID_STRANGE_EMBRYO;
			item_tmp.identify = 1;

			if( (i = pc_additem(sd, &item_tmp, 1, LOG_TYPE_SCRIPT)) ) {
				clif_additem(sd, 0, 0, i);
				clif_emotion(&sd->bl, ET_SWEAT); // Fail to avoid item drop exploit.
			} else {
				hom_vaporize(sd, HOM_ST_MORPH);
				script_pushint(st, 1);
				return SCRIPT_CMD_SUCCESS;
			}
		} else
			clif_emotion(&sd->hd->bl, ET_SWEAT);
	} else
		clif_emotion(&sd->bl, ET_SWEAT);

	script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

// [Zephyrus]
BUILDIN_FUNC(homunculus_shuffle)
{
	TBL_PC *sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	if(hom_is_active(sd->hd))
		hom_shuffle(sd->hd);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Check for homunculus state.
 * Return: -1 = No homunculus
 *          0 = Homunculus is active
 *          1 = Homunculus is vaporized (rest)
 *          2 = Homunculus is in morph state
 *------------------------------------------*/
BUILDIN_FUNC(checkhomcall)
{
	TBL_PC *sd;
	TBL_HOM *hd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	hd = sd->hd;

	if( !hd )
		script_pushint(st, -1);
	else
		script_pushint(st, hd->homunculus.vaporize);

	return SCRIPT_CMD_SUCCESS;
}

//These two functions bring the eA MAPID_* class functionality to scripts.
BUILDIN_FUNC(eaclass)
{
	int class_;
	if( script_hasdata(st,2) )
		class_ = script_getnum(st,2);
	else {
		TBL_PC *sd;

		if (!script_charid2sd(3,sd)) {
			script_pushint(st,-1);
			return SCRIPT_CMD_SUCCESS;
		}
		class_ = sd->status.class_;
	}
	script_pushint(st,pc_jobid2mapid(class_));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(roclass)
{
	int class_ =script_getnum(st,2);
	int sex;
	if( script_hasdata(st,3) )
		sex = script_getnum(st,3);
	else {
		TBL_PC *sd;
		if (st->rid && script_rid2sd(sd))
			sex = sd->status.sex;
		else
			sex = SEX_MALE; //Just use male when not found.
	}
	script_pushint(st,pc_mapid2jobid(class_, sex));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Tells client to open a hatching window, used for pet incubator
 *------------------------------------------*/
BUILDIN_FUNC(birthpet)
{
	TBL_PC *sd;
	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	if( sd->status.pet_id )
	{// do not send egg list, when you already have a pet
		return SCRIPT_CMD_SUCCESS;
	}

	clif_sendegg(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * resetlvl <action type>{,<char_id>};
 * @param action_type:
 *	1 : make like after rebirth
 *	2 : blvl,jlvl=1, skillpoint=0
 * 	3 : don't reset skill, blvl=1
 *	4 : jlvl=0
 * @author AppleGirl
 **/
BUILDIN_FUNC(resetlvl)
{
	TBL_PC *sd;

	int type=script_getnum(st,2);

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	pc_resetlvl(sd,type);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Reset a player status point
 * resetstatus({<char_id>});
 **/
BUILDIN_FUNC(resetstatus)
{
	TBL_PC *sd;
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	pc_resetstate(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Reset player's skill
 * resetskill({<char_id>});
 **/
BUILDIN_FUNC(resetskill)
{
	TBL_PC *sd;
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	pc_resetskill(sd,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Reset SG designated maps
 * resetfeel({<char_id>});
 **/
BUILDIN_FUNC(resetfeel)
{
	TBL_PC *sd;
	if (!script_charid2sd(2,sd) || (sd->class_&MAPID_UPPERMASK) != MAPID_STAR_GLADIATOR)
		return SCRIPT_CMD_FAILURE;
	pc_resetfeel(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Reset SG designated monsters
 * resethate({<char_id>});
 **/
BUILDIN_FUNC(resethate)
{
	TBL_PC *sd;
	if (!script_charid2sd(2,sd) || (sd->class_&MAPID_UPPERMASK) != MAPID_STAR_GLADIATOR)
		return SCRIPT_CMD_FAILURE;
	pc_resethate(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Counts total amount of skill points.
 * skillpointcount({<char_id>})
 **/
BUILDIN_FUNC(skillpointcount)
{
	TBL_PC *sd;
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	script_pushint(st,sd->status.skill_point + pc_resetskill(sd,2));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(changebase)
{
	TBL_PC *sd=NULL;
	int vclass;

	if( !script_mapid2sd(3,sd) )
		return SCRIPT_CMD_SUCCESS;

	vclass = script_getnum(st,2);
	if(vclass == JOB_WEDDING)
	{
		if (!battle_config.wedding_modifydisplay || //Do not show the wedding sprites
			sd->class_&JOBL_BABY //Baby classes screw up when showing wedding sprites. [Skotlex] They don't seem to anymore.
			)
		return SCRIPT_CMD_SUCCESS;
	}

	if(!sd->disguise && vclass != sd->vd.class_) {
		status_set_viewdata(&sd->bl, vclass);
		//Updated client view. Base, Weapon and Cloth Colors.
		clif_changelook(&sd->bl,LOOK_BASE,sd->vd.class_);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		if (sd->vd.cloth_color)
			clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->vd.cloth_color);
		if (sd->vd.body_style)
			clif_changelook(&sd->bl,LOOK_BODY2,sd->vd.body_style);
		clif_skillinfoblock(sd);
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Change account sex and unequip all item and request for a changesex to char-serv
 * changesex({<char_id>});
 */
BUILDIN_FUNC(changesex)
{
	int i;
	TBL_PC *sd = NULL;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	pc_resetskill(sd,4);
	// to avoid any problem with equipment and invalid sex, equipment is unequiped.
	for(i = 0; i < EQI_MAX; i++) {
		if (sd->equip_index[i] >= 0)
			pc_unequipitem(sd, sd->equip_index[i], 3);
	}

	chrif_changesex(sd, true);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Change character's sex and unequip all item and request for a changesex to char-serv
 * changecharsex({<char_id>});
 */
BUILDIN_FUNC(changecharsex)
{
#if PACKETVER >= 20141016
	int i;
	TBL_PC *sd = NULL;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	pc_resetskill(sd,4);
	// to avoid any problem with equipment and invalid sex, equipment is unequiped.
	for (i = 0; i < EQI_MAX; i++) {
		if (sd->equip_index[i] >= 0)
			pc_unequipitem(sd, sd->equip_index[i], 3);
	}

	chrif_changesex(sd, false);
	return SCRIPT_CMD_SUCCESS;
#else
	return SCRIPT_CMD_FAILURE;
#endif
}

/*==========================================
 * Works like 'announce' but outputs in the common chat window
 *------------------------------------------*/
BUILDIN_FUNC(globalmes)
{
	struct block_list *bl = map_id2bl(st->oid);
	struct npc_data *nd = (struct npc_data *)bl;
	const char *name=NULL,*mes;

	mes=script_getstr(st,2);
	if(mes==NULL)
		return SCRIPT_CMD_SUCCESS;

	if(script_hasdata(st,3)){	//  npc name to display
		name=script_getstr(st,3);
	} else {
		name=nd->name; //use current npc name
	}

	npc_globalmessage(name,mes);	// broadcast  to all players connected
	return SCRIPT_CMD_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// NPC waiting room (chat room)
//

/// Creates a waiting room (chat room) for this npc.
///
/// waitingroom "<title>",<limit>{,"<event>"{,<trigger>{,<zeny>{,<minlvl>{,<maxlvl>}}}}};
BUILDIN_FUNC(waitingroom)
{
	struct npc_data* nd;
	const char* title = script_getstr(st, 2);
	int limit = script_getnum(st, 3);
	const char* ev = script_hasdata(st,4) ? script_getstr(st,4) : "";
	int trigger =  script_hasdata(st,5) ? script_getnum(st,5) : limit;
	int zeny =  script_hasdata(st,6) ? script_getnum(st,6) : 0;
	int minLvl =  script_hasdata(st,7) ? script_getnum(st,7) : 1;
	int maxLvl =  script_hasdata(st,8) ? script_getnum(st,8) : MAX_LEVEL;

	nd = (struct npc_data *)map_id2bl(st->oid);
	if( nd != NULL )
		chat_createnpcchat(nd, title, limit, 1, trigger, ev, zeny, minLvl, maxLvl);

	return SCRIPT_CMD_SUCCESS;
}

/// Removes the waiting room of the current or target npc.
///
/// delwaitingroom "<npc_name>";
/// delwaitingroom;
BUILDIN_FUNC(delwaitingroom)
{
	struct npc_data* nd;
	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st, 2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);
	if( nd != NULL )
		chat_deletenpcchat(nd);
	return SCRIPT_CMD_SUCCESS;
}

/// Kick the specified player from the waiting room of the target npc.
///
/// waitingroomkick "<npc_name>", <kickusername>;
BUILDIN_FUNC(waitingroomkick)
{
	struct npc_data* nd;
	struct chat_data* cd;
	const char* kickusername;
	
	nd = npc_name2id(script_getstr(st,2));
	kickusername = script_getstr(st,3);

	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_npckickchat(cd, kickusername);
	return SCRIPT_CMD_SUCCESS;
}

/// Get Users in waiting room and stores gids in .@waitingroom_users[]
/// Num users stored in .@waitingroom_usercount
///
/// getwaitingroomusers "<npc_name>";
BUILDIN_FUNC(getwaitingroomusers)
{
	struct npc_data* nd;
	struct chat_data* cd;

	int i, j=0;

	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st, 2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);
	
	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL ) {
		for(i = 0; i < cd->users; ++i) {
			setd_sub_num( st, NULL, ".@waitingroom_users", j, cd->usersd[i]->status.account_id, NULL );
			j++;
		}
		setd_sub_num( st, NULL, ".@waitingroom_usercount", 0, j, NULL );
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Kicks all the players from the waiting room of the current or target npc.
///
/// kickwaitingroomall "<npc_name>";
/// kickwaitingroomall;
BUILDIN_FUNC(waitingroomkickall)
{
	struct npc_data* nd;
	struct chat_data* cd;

	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st,2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_npckickall(cd);
	return SCRIPT_CMD_SUCCESS;
}

/// Enables the waiting room event of the current or target npc.
///
/// enablewaitingroomevent "<npc_name>";
/// enablewaitingroomevent;
BUILDIN_FUNC(enablewaitingroomevent)
{
	struct npc_data* nd;
	struct chat_data* cd;

	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st, 2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_enableevent(cd);
	return SCRIPT_CMD_SUCCESS;
}

/// Disables the waiting room event of the current or target npc.
///
/// disablewaitingroomevent "<npc_name>";
/// disablewaitingroomevent;
BUILDIN_FUNC(disablewaitingroomevent)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( script_hasdata(st,2) )
		nd = npc_name2id(script_getstr(st, 2));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_disableevent(cd);
	return SCRIPT_CMD_SUCCESS;
}

/// Returns info on the waiting room of the current or target npc.
/// Returns -1 if the type unknown
/// <type>=0 : current number of users
/// <type>=1 : maximum number of users allowed
/// <type>=2 : the number of users that trigger the event
/// <type>=3 : if the trigger is disabled
/// <type>=4 : the title of the waiting room
/// <type>=5 : the password of the waiting room
/// <type>=16 : the name of the waiting room event
/// <type>=32 : if the waiting room is full
/// <type>=33 : if there are enough users to trigger the event
///
/// getwaitingroomstate(<type>,"<npc_name>") -> <info>
/// getwaitingroomstate(<type>) -> <info>
BUILDIN_FUNC(getwaitingroomstate)
{
	struct npc_data *nd;
	struct chat_data *cd;
	int type;

	type = script_getnum(st,2);
	if( script_hasdata(st,3) )
		nd = npc_name2id(script_getstr(st, 3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( nd == NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id)) == NULL )
	{
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	switch(type)
	{
	case 0:  script_pushint(st, cd->users); break;
	case 1:  script_pushint(st, cd->limit); break;
	case 2:  script_pushint(st, cd->trigger&0x7f); break;
	case 3:  script_pushint(st, ((cd->trigger&0x80)!=0)); break;
	case 4:  script_pushstrcopy(st, cd->title); break;
	case 5:  script_pushstrcopy(st, cd->pass); break;
	case 16: script_pushstrcopy(st, cd->npc_event);break;
	case 32: script_pushint(st, (cd->users >= cd->limit)); break;
	case 33: script_pushint(st, (cd->users >= cd->trigger)); break;
	default: script_pushint(st, -1); break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Warps the trigger or target amount of players to the target map and position.
/// Players are automatically removed from the waiting room.
/// Those waiting the longest will get warped first.
/// The target map can be "Random" for a random position in the current map,
/// and "SavePoint" for the savepoint map+position.
/// The map flag noteleport of the current map is only considered when teleporting to the savepoint.
///
/// The id's of the teleported players are put into the array $@warpwaitingpc[]
/// The total number of teleported players is put into $@warpwaitingpcnum
///
/// warpwaitingpc "<map name>",<x>,<y>,<number of players>;
/// warpwaitingpc "<map name>",<x>,<y>;
BUILDIN_FUNC(warpwaitingpc)
{
	int x;
	int y;
	int i;
	int n;
	const char* map_name;
	struct npc_data* nd;
	struct chat_data* cd;

	nd = (struct npc_data *)map_id2bl(st->oid);
	if( nd == NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id)) == NULL )
		return SCRIPT_CMD_SUCCESS;

	map_name = script_getstr(st,2);
	x = script_getnum(st,3);
	y = script_getnum(st,4);
	n = cd->trigger&0x7f;

	if( script_hasdata(st,5) )
		n = script_getnum(st,5);

	for( i = 0; i < n && cd->users > 0; i++ )
	{
		TBL_PC* sd = cd->usersd[0];

		if( strcmp(map_name,"SavePoint") == 0 && map_getmapflag(sd->bl.m, MF_NOTELEPORT) )
		{// can't teleport on this map
			break;
		}

		if( cd->zeny )
		{// fee set
			if( (uint32)sd->status.zeny < cd->zeny )
			{// no zeny to cover set fee
				break;
			}
			pc_payzeny(sd, cd->zeny, LOG_TYPE_NPC, NULL);
		}

		mapreg_setreg(reference_uid(add_str("$@warpwaitingpc"), i), sd->bl.id);

#ifdef Pandas_Support_Transfer_Autotrade_Player
		pc_mark_multitransfer(sd);
#endif // Pandas_Support_Transfer_Autotrade_Player

		if( strcmp(map_name,"Random") == 0 )
			pc_randomwarp(sd,CLR_TELEPORT,true);
		else if( strcmp(map_name,"SavePoint") == 0 )
			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, CLR_TELEPORT);
		else
			pc_setpos(sd, mapindex_name2id(map_name), x, y, CLR_OUTSIGHT);
	}
	mapreg_setreg(add_str("$@warpwaitingpcnum"), i);
	return SCRIPT_CMD_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// ...
//

/// Detaches a character from a script.
///
/// @param st Script state to detach the character from.
void script_detach_rid(struct script_state* st)
{
	if(st->rid)
	{
		script_detach_state(st, false);
		st->rid = 0;
	}
}

/*=========================================================================
 * Attaches a set of RIDs to the current script. [digitalhamster]
 * addrid(<type>{,<flag>{,<parameters>}});
 * <type>:
 *	0 : All players in the server.
 *	1 : All players in the map of the invoking player, or the invoking NPC if no player is attached.
 *	2 : Party members of a specified party ID.
 *	    [ Parameters: <party id> ]
 *	3 : Guild members of a specified guild ID.
 *	    [ Parameters: <guild id> ]
 *	4 : All players in a specified area of the map of the invoking player (or NPC).
 *	    [ Parameters: <x0>,<y0>,<x1>,<y1> ]
 *	5 : All players in the map.
 *	    [ Parameters: "<map name>" ]
 *	Account ID: The specified account ID.
 * <flag>:
 *	0 : Players are always attached. (default)
 *	1 : Players currently running another script will not be attached.
 *-------------------------------------------------------------------------*/
static int buildin_addrid_sub(struct block_list *bl,va_list ap)
{
	int forceflag;
	struct map_session_data *sd = (TBL_PC *)bl;
	struct script_state* st;

	st = va_arg(ap,struct script_state*);
	forceflag = va_arg(ap,int);

	if(!forceflag || !sd->st)
		if(sd->status.account_id != st->rid)
			run_script(st->script,st->pos,sd->status.account_id,st->oid);
	return 0;
}

BUILDIN_FUNC(addrid)
{
	struct s_mapiterator* iter;
	struct block_list *bl;
	TBL_PC *sd;

	if(st->rid < 1) {
		st->state = END;
		bl = map_id2bl(st->oid);
	} else
		bl = map_id2bl(st->rid); //if run without rid it'd error,also oid if npc, else rid for map
	iter = mapit_getallusers();

	switch(script_getnum(st,2)) {
		case 0:
			for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
				if(!script_getnum(st,3) || !sd->st)
					if(sd->status.account_id != st->rid) //attached player already runs.
						run_script(st->script,st->pos,sd->status.account_id,st->oid);
			}
			break;
		case 1:
			for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
				if(!script_getnum(st,3) || !sd->st)
					if((sd->bl.m == bl->m) && (sd->status.account_id != st->rid))
						run_script(st->script,st->pos,sd->status.account_id,st->oid);
			}
			break;
		case 2:
			if(script_getnum(st,4) == 0) {
				script_pushint(st,0);
				mapit_free(iter);
				return SCRIPT_CMD_SUCCESS;
			}
			for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
				if(!script_getnum(st,3) || !sd->st)
					if((sd->status.account_id != st->rid) && (sd->status.party_id == script_getnum(st,4))) //attached player already runs.
						run_script(st->script,st->pos,sd->status.account_id,st->oid);
			}
			break;
		case 3:
			if(script_getnum(st,4) == 0) {
				script_pushint(st,0);
				mapit_free(iter);
				return SCRIPT_CMD_SUCCESS;
			}
			for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
				if(!script_getnum(st,3) || !sd->st)
					if((sd->status.account_id != st->rid) && (sd->status.guild_id == script_getnum(st,4))) //attached player already runs.
						run_script(st->script,st->pos,sd->status.account_id,st->oid);
			}
			break;
		case 4:
			map_foreachinallarea(buildin_addrid_sub,
			bl->m,script_getnum(st,4),script_getnum(st,5),script_getnum(st,6),script_getnum(st,7),BL_PC,
			st,script_getnum(st,3));//4-x0 , 5-y0 , 6-x1, 7-y1
			break;
		case 5:
			if (script_getstr(st, 4) == NULL) {
				script_pushint(st, 0);
				mapit_free(iter);
				return SCRIPT_CMD_FAILURE;
			}
			if (map_mapname2mapid(script_getstr(st, 4)) < 0) {
				script_pushint(st, 0);
				mapit_free(iter);
				return SCRIPT_CMD_FAILURE;
			}
			map_foreachinmap(buildin_addrid_sub, map_mapname2mapid(script_getstr(st, 4)), BL_PC, st, script_getnum(st, 3));
			break;
		default:
			mapit_free(iter);
			if((map_id2sd(script_getnum(st,2))) == NULL) { // Player not found.
				script_pushint(st,0);
				return SCRIPT_CMD_SUCCESS;
			}
			if(!script_getnum(st,3) || !map_id2sd(script_getnum(st,2))->st) {
				run_script(st->script,st->pos,script_getnum(st,2),st->oid);
				script_pushint(st,1);
			}
			return SCRIPT_CMD_SUCCESS;
	}
	mapit_free(iter);
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Attach sd char id to script and detach current one if any
 *------------------------------------------*/
BUILDIN_FUNC(attachrid)
{
	int rid = script_getnum(st,2);
	bool force;

	if( script_hasdata(st,3) ){
		force = script_getnum(st,3) != 0;
	}else{
		force = true;
	}

	struct map_session_data* sd = map_id2sd(rid);

	if( sd != NULL && ( !sd->npc_id || force ) ){
		script_detach_rid(st);

		st->rid = rid;
		script_attach_state(st);
		script_pushint(st,true);
	}else{
		script_pushint(st,false);
	}

	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * Detach script to rid
 *------------------------------------------*/
BUILDIN_FUNC(detachrid)
{
	script_detach_rid(st);
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * Chk if account connected, (and charid from account if specified)
 *------------------------------------------*/
BUILDIN_FUNC(isloggedin)
{
	TBL_PC* sd = map_id2sd(script_getnum(st,2));
	if (script_hasdata(st,3) && sd &&
		sd->status.char_id != script_getnum(st,3))
		sd = NULL;
	push_val(st->stack,C_INT,sd!=NULL);
	return SCRIPT_CMD_SUCCESS;
}


/*==========================================
 *
 *------------------------------------------*/
BUILDIN_FUNC(setmapflagnosave)
{
	int16 m,x,y;
	unsigned short mapindex;
	const char *str,*str2;
	union u_mapflag_args args = {};

	str=script_getstr(st,2);
	str2=script_getstr(st,3);
	x=script_getnum(st,4);
	y=script_getnum(st,5);
	m = map_mapname2mapid(str);
	mapindex = mapindex_name2id(str2);

	if(m < 0) {
		ShowWarning("buildin_setmapflagnosave: Invalid map name %s.\n", str);
		return SCRIPT_CMD_FAILURE;
	}

	args.nosave.map = mapindex;
	args.nosave.x = x;
	args.nosave.y = y;

	map_setmapflag_sub(m, MF_NOSAVE, true, &args);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getmapflag)
{
	int16 m;
	int mf;
	const char *str;

	str=script_getstr(st,2);

	m = map_mapname2mapid(str);
	if (m < 0) {
		ShowWarning("buildin_getmapflag: Invalid map name %s.\n", str);
		return SCRIPT_CMD_FAILURE;
	}

	mf = script_getnum(st, 3);

	if( mf < MF_MIN || mf > MF_MAX ){
		ShowError( "buildin_getmapflag: Unsupported mapflag '%d'.\n", mf );
		return SCRIPT_CMD_FAILURE;
	}

	union u_mapflag_args args = {};

	if (mf == MF_SKILL_DAMAGE && !script_hasdata(st, 4))
		args.flag_val = SKILLDMG_MAX;
	// PYHELP - MAPFLAG - INSERT POINT - <Section 10>
	else
		FETCH(4, args.flag_val);

	script_pushint(st, map_getmapflag_sub(m, static_cast<e_mapflag>(mf), &args));

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(setmapflag)
{
	int16 m;
	int mf;
	const char *str;

	str = script_getstr(st,2);

	m = map_mapname2mapid(str);
	if (m < 0) {
		ShowWarning("buildin_setmapflag: Invalid map name %s.\n", str);
		return SCRIPT_CMD_FAILURE;
	}

	mf = script_getnum(st, 3);

	if( mf < MF_MIN || mf > MF_MAX ){
		ShowError( "buildin_setmapflag: Unsupported mapflag '%d'.\n", mf );
		return SCRIPT_CMD_FAILURE;
	}

	union u_mapflag_args args = {};

	switch(mf) {
		case MF_SKILL_DAMAGE:
			if (script_hasdata(st, 4) && script_hasdata(st, 5))
				args.skill_damage.rate[script_getnum(st, 5)] = script_getnum(st, 4);
			else {
				ShowWarning("buildin_setmapflag: Unable to set skill_damage mapflag as flag data is missing.\n");
				return SCRIPT_CMD_FAILURE;
			}
			break;
		case MF_SKILL_DURATION:
			if (script_hasdata(st, 4) && script_hasdata(st, 5)) {
				args.skill_duration.skill_id = script_getnum(st, 4);

				if (!skill_get_index(args.skill_duration.skill_id)) {
					ShowError("buildin_setmapflag: Invalid skill ID %d for skill_duration mapflag.\n", args.skill_duration.skill_id);
					return SCRIPT_CMD_FAILURE;
				}
				args.skill_duration.per = script_getnum(st, 5);
			} else {
				ShowWarning("buildin_setmapflag: Unable to set skill_duration mapflag as flag data is missing.\n");
				return SCRIPT_CMD_FAILURE;
			}
			break;
		case MF_NOSAVE: // Assume setting "SavePoint"
			args.nosave.map = 0;
			args.nosave.x = -1;
			args.nosave.y = -1;
			break;
		case MF_PVP_NIGHTMAREDROP: // Assume setting standard drops
			args.nightmaredrop.drop_id = -1;
			args.nightmaredrop.drop_per = 300;
			args.nightmaredrop.drop_type = NMDT_EQUIP;
			break;
		// PYHELP - MAPFLAG - INSERT POINT - <Section 9>
		default:
			FETCH(4, args.flag_val);
			break;
	}

	map_setmapflag_sub(m, static_cast<e_mapflag>(mf), true, &args);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(removemapflag)
{
	int16 m;
	int mf;
	const char *str;
	union u_mapflag_args args = {};

	str = script_getstr(st, 2);
	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_removemapflag: Invalid map name %s.\n", str);
		return SCRIPT_CMD_FAILURE;
	}

	mf = script_getnum(st, 3);

	if( mf < MF_MIN || mf > MF_MAX ){
		ShowError( "buildin_removemapflag: Unsupported mapflag '%d'.\n", mf );
		return SCRIPT_CMD_FAILURE;
	}

	FETCH(4, args.flag_val);

	map_setmapflag_sub(m, static_cast<e_mapflag>(mf), false, &args);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pvpon)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_pvpon: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (!map_getmapflag(m, MF_PVP))
		map_setmapflag(m, MF_PVP, true);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pvpoff)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_pvpoff: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (map_getmapflag(m, MF_PVP))
		map_setmapflag(m, MF_PVP, false);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(gvgon)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_gvgon: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (!map_getmapflag(m, MF_GVG))
		map_setmapflag(m, MF_GVG, true);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(gvgoff)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_gvgoff: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (map_getmapflag(m, MF_GVG))
		map_setmapflag(m, MF_GVG, false);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(gvgon3)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_gvgon3: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (!map_getmapflag(m, MF_GVG_TE))
		map_setmapflag(m, MF_GVG_TE, true);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(gvgoff3)
{
	int16 m;
	const char *str = script_getstr(st,2);

	m = map_mapname2mapid(str);

	if (m < 0) {
		ShowWarning("buildin_gvgoff3: Unknown map '%s'.\n", str);
		return SCRIPT_CMD_FAILURE;
	}
	if (map_getmapflag(m, MF_GVG_TE))
		map_setmapflag(m, MF_GVG_TE, false);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Shows an emotion on top of a NPC by default or the given GID
 * emotion <emotion ID>{,<target ID>};
 */
BUILDIN_FUNC(emotion)
{
	struct block_list *bl = NULL;
	int type = script_getnum(st,2);

	if (type < ET_SURPRISE || type >= ET_MAX) {
		ShowWarning("buildin_emotion: Unknown emotion %d (min=%d, max=%d).\n", type, ET_SURPRISE, (ET_MAX-1));
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 3) && !script_rid2bl(3, bl)) {
		ShowWarning("buildin_emotion: Unknown game ID supplied %d.\n", script_getnum(st, 3));
		return SCRIPT_CMD_FAILURE;
	}
	if (!bl)
		bl = map_id2bl(st->oid);

	clif_emotion(bl, type);
	return SCRIPT_CMD_SUCCESS;
}


static int buildin_maprespawnguildid_sub_pc(struct map_session_data* sd, va_list ap)
{
	int16 m=va_arg(ap,int);
	int g_id=va_arg(ap,int);
	int flag=va_arg(ap,int);

	if(!sd || sd->bl.m != m)
		return 0;
	if(
		(sd->status.guild_id == g_id && flag&1) || //Warp out owners
		(sd->status.guild_id != g_id && flag&2) || //Warp out outsiders
		(sd->status.guild_id == 0 && flag&2)	// Warp out players not in guild
	)
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,CLR_TELEPORT);
	return 1;
}

static int buildin_maprespawnguildid_sub_mob(struct block_list *bl,va_list ap)
{
	struct mob_data *md=(struct mob_data *)bl;

	if(!md->guardian_data && md->mob_id != MOBID_EMPERIUM && ( !mob_is_clone(md->mob_id) || battle_config.guild_maprespawn_clones ))
		status_kill(bl);

	return 1;
}

/*
 * Function to kick guild members out of a map and to their save points.
 * m : mapid
 * g_id : owner guild id
 * flag & 1 : Warp out owners
 * flag & 2 : Warp out outsiders
 * flag & 4 : reserved for mobs
 * */
BUILDIN_FUNC(maprespawnguildid)
{
	const char *mapname=script_getstr(st,2);
	int g_id=script_getnum(st,3);
	int flag=script_getnum(st,4);

	int16 m=map_mapname2mapid(mapname);

	if(m == -1)
		return SCRIPT_CMD_SUCCESS;

	//Catch ALL players (in case some are 'between maps' on execution time)
	map_foreachpc(buildin_maprespawnguildid_sub_pc,m,g_id,flag);
	if (flag&4) //Remove script mobs.
		map_foreachinmap(buildin_maprespawnguildid_sub_mob,m,BL_MOB);
	return SCRIPT_CMD_SUCCESS;
}

/// Siege commands

/**
 * Start WoE:FE
 * agitstart();
 */
BUILDIN_FUNC(agitstart)
{
	guild_agit_start();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * End WoE:FE
 * agitend();
 */
BUILDIN_FUNC(agitend)
{
	guild_agit_end();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Start WoE:SE
 * agitstart2();
 */
BUILDIN_FUNC(agitstart2)
{
	guild_agit2_start();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * End WoE:SE
 * agitend();
 */
BUILDIN_FUNC(agitend2)
{
	guild_agit2_end();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Start WoE:TE
 * agitstart3();
 */
BUILDIN_FUNC(agitstart3)
{
	guild_agit3_start();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * End WoE:TE
 * agitend3();
 */
BUILDIN_FUNC(agitend3)
{
	guild_agit3_end();

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns whether WoE:FE is on or off.
 * agitcheck();
 */
BUILDIN_FUNC(agitcheck)
{
	script_pushint(st, agit_flag);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns whether WoE:SE is on or off.
 * agitcheck2();
 */
BUILDIN_FUNC(agitcheck2)
{
	script_pushint(st, agit2_flag);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns whether WoE:TE is on or off.
 * agitcheck3();
 */
BUILDIN_FUNC(agitcheck3)
{
	script_pushint(st, agit3_flag);
	return SCRIPT_CMD_SUCCESS;
}

/// Sets the guild_id of this npc.
///
/// flagemblem <guild_id>;
BUILDIN_FUNC(flagemblem)
{
	TBL_NPC* nd;
	int g_id = script_getnum(st,2);

	if(g_id < 0)
		return SCRIPT_CMD_SUCCESS;

	nd = (TBL_NPC*)map_id2nd(st->oid);
	if( nd == NULL ) {
		ShowError("script:flagemblem: npc %d not found\n", st->oid);
	} else if( nd->subtype != NPCTYPE_SCRIPT ) {
		ShowError("script:flagemblem: unexpected subtype %d for npc %d '%s'\n", nd->subtype, st->oid, nd->exname);
	} else {
		bool changed = ( nd->u.scr.guild_id != g_id )?true:false;
		nd->u.scr.guild_id = g_id;
		clif_guild_emblem_area(&nd->bl);
		/* guild flag caching */
		if( g_id ) /* adding a id */
			guild_flag_add(nd);
		else if( changed ) /* removing a flag */
			guild_flag_remove(nd);
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getcastlename)
{
	const char* mapname = mapindex_getmapname(script_getstr(st,2),NULL);
	std::shared_ptr<guild_castle> gc = castle_db.mapname2gc(mapname);
	const char* name = (gc) ? gc->castle_name : "";
	script_pushstrcopy(st,name);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getcastledata)
{
	const char *mapname = mapindex_getmapname(script_getstr(st,2),NULL);
	int index = script_getnum(st,3);
	std::shared_ptr<guild_castle> gc = castle_db.mapname2gc(mapname);

	if (gc == NULL) {
		script_pushint(st,0);
		ShowWarning("buildin_getcastledata: guild castle for map '%s' not found\n", mapname);
		return SCRIPT_CMD_FAILURE;
	}

	switch (index) {
		case CD_GUILD_ID:
			script_pushint(st,gc->guild_id); break;
		case CD_CURRENT_ECONOMY:
			script_pushint(st,gc->economy); break;
		case CD_CURRENT_DEFENSE:
			script_pushint(st,gc->defense); break;
		case CD_INVESTED_ECONOMY:
			script_pushint(st,gc->triggerE); break;
		case CD_INVESTED_DEFENSE:
			script_pushint(st,gc->triggerD); break;
		case CD_NEXT_TIME:
			script_pushint(st,gc->nextTime); break;
		case CD_PAY_TIME:
			script_pushint(st,gc->payTime); break;
		case CD_CREATE_TIME:
			script_pushint(st,gc->createTime); break;
		case CD_ENABLED_KAFRA:
			script_pushint(st,gc->visibleC); break;
		default:
			if (index >= CD_ENABLED_GUARDIAN00 && index < CD_MAX) {
				script_pushint(st,gc->guardian[index - CD_ENABLED_GUARDIAN00].visible);
				break;
			}
			script_pushint(st,0);
			ShowWarning("buildin_getcastledata: index = '%d' is out of allowed range\n", index);
			return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(setcastledata)
{
	const char *mapname = mapindex_getmapname(script_getstr(st,2),NULL);
	int index = script_getnum(st,3);
	int value = script_getnum(st,4);
	std::shared_ptr<guild_castle> gc = castle_db.mapname2gc(mapname);

	if (gc == NULL) {
		ShowWarning("buildin_setcastledata: guild castle for map '%s' not found\n", mapname);
		return SCRIPT_CMD_FAILURE;
	}

	if (index <= CD_NONE || index >= CD_MAX) {
		ShowWarning("buildin_setcastledata: index = '%d' is out of allowed range\n", index);
		return SCRIPT_CMD_FAILURE;
	}

	guild_castledatasave(gc->castle_id, index, value);
	return SCRIPT_CMD_SUCCESS;
}

/* =====================================================================
 * ---------------------------------------------------------------------*/
BUILDIN_FUNC(requestguildinfo)
{
	int guild_id=script_getnum(st,2);
	const char *event=NULL;

	if( script_hasdata(st,3) ){
		event=script_getstr(st,3);
		check_event(st, event);
	}

	if(guild_id>0)
		guild_npc_request_info(guild_id,event);
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the number of cards that have been compounded onto the specified equipped item.
/// getequipcardcnt(<equipment slot>);
BUILDIN_FUNC(getequipcardcnt)
{
	int i=-1,j,num;
	TBL_PC *sd;
	int count;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	num=script_getnum(st,2);
	if (equip_index_check(num))
		i=pc_checkequip(sd,equip_bitmask[num]);

	if (i < 0 || !sd->inventory_data[i]) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if(itemdb_isspecial(sd->inventory.u.items_inventory[i].card[0]))
	{
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	count = 0;
	for( j = 0; j < sd->inventory_data[i]->slots; j++ )
		if( sd->inventory.u.items_inventory[i].card[j] && itemdb_type(sd->inventory.u.items_inventory[i].card[j]) == IT_CARD )
			count++;

	script_pushint(st,count);
	return SCRIPT_CMD_SUCCESS;
}

/// Removes all cards from the item found in the specified equipment slot of the invoking character,
/// and give them to the character. If any cards were removed in this manner, it will also show a success effect.
/// successremovecards <slot>;
BUILDIN_FUNC(successremovecards) {
	int i=-1,c,cardflag=0;

	TBL_PC* sd;
	int num;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	num = script_getnum(st,2);

	if (equip_index_check(num))
		i=pc_checkequip(sd,equip_bitmask[num]);

	if (i < 0 || !sd->inventory_data[i]) {
		return SCRIPT_CMD_SUCCESS;
	}

	if(itemdb_isspecial(sd->inventory.u.items_inventory[i].card[0]))
		return SCRIPT_CMD_SUCCESS;

	for( c = sd->inventory_data[i]->slots - 1; c >= 0; --c ) {
		if( sd->inventory.u.items_inventory[i].card[c] && itemdb_type(sd->inventory.u.items_inventory[i].card[c]) == IT_CARD ) {// extract this card from the item
			unsigned char flag = 0;
			struct item item_tmp;
			memset(&item_tmp,0,sizeof(item_tmp));
			cardflag = 1;
			item_tmp.nameid   = sd->inventory.u.items_inventory[i].card[c];
			item_tmp.identify = 1;

			if((flag=pc_additem(sd,&item_tmp,1,LOG_TYPE_SCRIPT))){	// get back the cart in inventory
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
			}
		}
	}

	if(cardflag == 1) {//if card was remove remplace item with no card
		unsigned char flag = 0;
		struct item item_tmp;
		memset(&item_tmp,0,sizeof(item_tmp));

		item_tmp.nameid      = sd->inventory.u.items_inventory[i].nameid;
		item_tmp.identify    = 1;
		item_tmp.refine      = sd->inventory.u.items_inventory[i].refine;
		item_tmp.attribute   = sd->inventory.u.items_inventory[i].attribute;
		item_tmp.expire_time = sd->inventory.u.items_inventory[i].expire_time;
		item_tmp.bound       = sd->inventory.u.items_inventory[i].bound;
		item_tmp.enchantgrade = sd->inventory.u.items_inventory[i].enchantgrade;

		for (int j = sd->inventory_data[i]->slots; j < MAX_SLOTS; j++)
			item_tmp.card[j]=sd->inventory.u.items_inventory[i].card[j];
		
		for (int j = 0; j < MAX_ITEM_RDM_OPT; j++){
			item_tmp.option[j].id=sd->inventory.u.items_inventory[i].option[j].id;
			item_tmp.option[j].value=sd->inventory.u.items_inventory[i].option[j].value;
			item_tmp.option[j].param=sd->inventory.u.items_inventory[i].option[j].param;
		}

		pc_delitem(sd,i,1,0,3,LOG_TYPE_SCRIPT);
		if((flag=pc_additem(sd,&item_tmp,1,LOG_TYPE_SCRIPT))){	//chk if can be spawn in inventory otherwise put on floor
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
		}

		clif_misceffect(&sd->bl,3);
	}
	return SCRIPT_CMD_SUCCESS;
}

/// Removes all cards from the item found in the specified equipment slot of the invoking character.
/// failedremovecards <slot>, <type>;
/// <type>=0 : will destroy both the item and the cards.
/// <type>=1 : will keep the item, but destroy the cards.
/// <type>=2 : will keep the cards, but destroy the item.
/// <type>=? : will just display the failure effect.
BUILDIN_FUNC(failedremovecards) {
	int i=-1,c,cardflag=0;

	TBL_PC* sd;
	int num;
	int typefail;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	num = script_getnum(st,2);
	typefail = script_getnum(st,3);

	if (equip_index_check(num))
		i=pc_checkequip(sd,equip_bitmask[num]);

	if (i < 0 || !sd->inventory_data[i])
		return SCRIPT_CMD_SUCCESS;

	if(itemdb_isspecial(sd->inventory.u.items_inventory[i].card[0]))
		return SCRIPT_CMD_SUCCESS;

	for( c = sd->inventory_data[i]->slots - 1; c >= 0; --c ) {
		if( sd->inventory.u.items_inventory[i].card[c] && itemdb_type(sd->inventory.u.items_inventory[i].card[c]) == IT_CARD ) {
			cardflag = 1;

			if(typefail == 2) {// add cards to inventory, clear
				unsigned char flag = 0;
				struct item item_tmp;

				memset(&item_tmp,0,sizeof(item_tmp));

				item_tmp.nameid   = sd->inventory.u.items_inventory[i].card[c];
				item_tmp.identify = 1;

				if((flag=pc_additem(sd,&item_tmp,1,LOG_TYPE_SCRIPT))){
					clif_additem(sd,0,0,flag);
					map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
				}
			}
		}
	}

	if(cardflag == 1) {
		if(typefail == 0 || typefail == 2){	// destroy the item
			pc_delitem(sd,i,1,0,2,LOG_TYPE_SCRIPT);
		}else if(typefail == 1){ // destroy the card
			unsigned char flag = 0;
			struct item item_tmp;

			memset(&item_tmp,0,sizeof(item_tmp));

			item_tmp.nameid      = sd->inventory.u.items_inventory[i].nameid;
			item_tmp.identify    = 1;
			item_tmp.refine      = sd->inventory.u.items_inventory[i].refine;
			item_tmp.attribute   = sd->inventory.u.items_inventory[i].attribute;
			item_tmp.expire_time = sd->inventory.u.items_inventory[i].expire_time;
			item_tmp.bound       = sd->inventory.u.items_inventory[i].bound;
			item_tmp.enchantgrade = sd->inventory.u.items_inventory[i].enchantgrade;

			for (int j = sd->inventory_data[i]->slots; j < MAX_SLOTS; j++)
				item_tmp.card[j]=sd->inventory.u.items_inventory[i].card[j];
			
			for (int j = 0; j < MAX_ITEM_RDM_OPT; j++){
				item_tmp.option[j].id=sd->inventory.u.items_inventory[i].option[j].id;
				item_tmp.option[j].value=sd->inventory.u.items_inventory[i].option[j].value;
				item_tmp.option[j].param=sd->inventory.u.items_inventory[i].option[j].param;
			}

			pc_delitem(sd,i,1,0,2,LOG_TYPE_SCRIPT);

			if((flag=pc_additem(sd,&item_tmp,1,LOG_TYPE_SCRIPT))){
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
			}
		}
		clif_misceffect(&sd->bl,2);
	}
	return SCRIPT_CMD_SUCCESS;
}

/* ================================================================
 * mapwarp "<from map>","<to map>",<x>,<y>,<type>,<ID for Type>;
 * type: 0=everyone, 1=guild, 2=party;	[Reddozen]
 * improved by [Lance]
 * ================================================================*/
BUILDIN_FUNC(mapwarp)	// Added by RoVeRT
{
	int x,y,m,check_val=0,check_ID=0,i=0;
	struct guild *g = NULL;
	struct party_data *p = NULL;
	const char *str;
	const char *mapname;
	unsigned int index;
	mapname=script_getstr(st,2);
	str=script_getstr(st,3);
	x=script_getnum(st,4);
	y=script_getnum(st,5);
	if(script_hasdata(st,7)){
		check_val=script_getnum(st,6);
		check_ID=script_getnum(st,7);
	}

	if((m=map_mapname2mapid(mapname))< 0)
		return SCRIPT_CMD_SUCCESS;

	if(!(index=mapindex_name2id(str)))
		return SCRIPT_CMD_SUCCESS;

	switch(check_val){
		case 1:
			g = guild_search(check_ID);
			if (g){
				for( i=0; i < g->max_member; i++)
				{
					if(g->member[i].sd && g->member[i].sd->bl.m==m){
#ifdef Pandas_Support_Transfer_Autotrade_Player
						pc_mark_multitransfer(g->member[i].sd);
#endif // Pandas_Support_Transfer_Autotrade_Player
						pc_setpos(g->member[i].sd,index,x,y,CLR_TELEPORT);
					}
				}
			}
			break;
		case 2:
			p = party_search(check_ID);
			if(p){
				for(i=0;i<MAX_PARTY; i++){
					if(p->data[i].sd && p->data[i].sd->bl.m == m){
#ifdef Pandas_Support_Transfer_Autotrade_Player
						pc_mark_multitransfer(p->data[i].sd);
#endif // Pandas_Support_Transfer_Autotrade_Player
						pc_setpos(p->data[i].sd,index,x,y,CLR_TELEPORT);
					}
				}
			}
			break;
		default:
			map_foreachinmap(buildin_areawarp_sub,m,BL_PC,index,x,y,0,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

static int buildin_mobcount_sub(struct block_list *bl,va_list ap)	// Added by RoVeRT
{
	char *event=va_arg(ap,char *);
	struct mob_data *md = ((struct mob_data *)bl);
	if( md->status.hp > 0 && (!event || strcmp(event,md->npc_event) == 0) )
		return 1;
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mobcount)	// Added by RoVeRT
{
	const char *mapname,*event;
	int16 m;
	mapname=script_getstr(st,2);
	event=script_getstr(st,3);

	if( strcmp(event, "all") == 0 )
		event = NULL;
	else
		check_event(st, event);

	if( strcmp(mapname, "this") == 0 ) {
		struct map_session_data *sd;
		if( script_rid2sd(sd) )
			m = sd->bl.m;
		else {
			script_pushint(st,-1);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	else if( (m = map_mapname2mapid(mapname)) < 0 ) {
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st,map_foreachinmap(buildin_mobcount_sub, m, BL_MOB, event));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(marriage)
{
	TBL_PC *sd;
	TBL_PC *p_sd;

	if(!script_rid2sd(sd) || !(p_sd=map_nick2sd(script_getstr(st,2),false)) || !pc_marriage(sd,p_sd)){
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(wedding_effect)
{
	TBL_PC *sd;
	struct block_list *bl;

	if(!script_rid2sd(sd)) {
		bl=map_id2bl(st->oid);
	} else
		bl=&sd->bl;
	clif_wedding_effect(bl);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * divorce({<char_id>})
 **/
BUILDIN_FUNC(divorce)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd) || !pc_divorce(sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * ispartneron({<char_id>})
 **/
BUILDIN_FUNC(ispartneron)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd) || !pc_ismarried(sd) ||
		map_charid2sd(sd->status.partner_id) == NULL)
	{
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getpartnerid({<char_id>})
 **/
BUILDIN_FUNC(getpartnerid)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,sd->status.partner_id);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getchildid({<char_id>})
 **/
BUILDIN_FUNC(getchildid)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,sd->status.child);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getmotherid({<char_id>})
 **/
BUILDIN_FUNC(getmotherid)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,sd->status.mother);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getfatherid({<char_id>})
 **/
BUILDIN_FUNC(getfatherid)
{
	TBL_PC *sd;

	if (!script_charid2sd(2,sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,sd->status.father);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(warppartner)
{
	int x,y;
	unsigned short mapindex;
	const char *str;
	TBL_PC *sd;
	TBL_PC *p_sd;

	if(!script_rid2sd(sd) || !pc_ismarried(sd) ||
	(p_sd=map_charid2sd(sd->status.partner_id)) == NULL) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	str=script_getstr(st,2);
	x=script_getnum(st,3);
	y=script_getnum(st,4);

	mapindex = mapindex_name2id(str);
	if (mapindex) {
		pc_setpos(p_sd,mapindex,x,y,CLR_OUTSIGHT);
		script_pushint(st,1);
	} else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*================================================
 * Script for Displaying MOB Information [Valaris]
 *------------------------------------------------*/
BUILDIN_FUNC(strmobinfo)
{

	int num=script_getnum(st,2);
	int class_=script_getnum(st,3);

	if(!mobdb_checkid(class_))
	{
		if (num < 3) //requested a string
			script_pushconststr(st,"");
		else
			script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<s_mob_db> mob = mob_db.find(class_);

	switch (num) {
	case 1: script_pushstrcopy(st,mob->name.c_str()); break;
	case 2: script_pushstrcopy(st, mob->jname.c_str()); break;
	case 3: script_pushint(st,mob->lv); break;
	case 4: script_pushint(st,mob->status.max_hp); break;
	case 5: script_pushint(st,mob->status.max_sp); break;
	case 6: script_pushint(st,mob->base_exp); break;
	case 7: script_pushint(st,mob->job_exp); break;
	default:
		script_pushint(st,0);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Summon guardians [Valaris]
 * guardian("<map name>",<x>,<y>,"<name to show>",<mob id>{,"<event label>"}{,<guardian index>}) -> <id>
 *------------------------------------------*/
BUILDIN_FUNC(guardian)
{
	int class_=0,x=0,y=0,guardian=0;
	const char *str,*mapname,*evt="";
	bool has_index = false;

	mapname	  =script_getstr(st,2);
	x	  =script_getnum(st,3);
	y	  =script_getnum(st,4);
	str	  =script_getstr(st,5);
	class_=script_getnum(st,6);

	if( script_hasdata(st,8) )
	{// "<event label>",<guardian index>
		evt=script_getstr(st,7);
		guardian=script_getnum(st,8);
		has_index = true;
	} else if( script_hasdata(st,7) ){
		if( script_isstring(st, 7) )
		{// "<event label>"
			evt=script_getstr(st,7);
		} else
		{// <guardian index>
			guardian=script_getnum(st,7);
			has_index = true;
		}
	}

	check_event(st, evt);
	script_pushint(st, mob_spawn_guardian(mapname,x,y,str,class_,evt,guardian,has_index));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Invisible Walls [Zephyrus]
 *------------------------------------------*/
BUILDIN_FUNC(setwall)
{
	const char *mapname, *name;
	int x, y, m, size, dir;
	bool shootable;

	mapname = script_getstr(st,2);
	x = script_getnum(st,3);
	y = script_getnum(st,4);
	size = script_getnum(st,5);
	dir = script_getnum(st,6);
	shootable = script_getnum(st,7) != 0;
	name = script_getstr(st,8);

	if( (m = map_mapname2mapid(mapname)) < 0 )
		return SCRIPT_CMD_SUCCESS; // Invalid Map

	map_iwall_set(m, x, y, size, dir, shootable, name);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(delwall)
{
	const char *name = script_getstr(st,2);

	if( !map_iwall_remove(name) ){
		ShowError( "buildin_delwall: wall \"%s\" does not exist.\n", name );
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(checkwall)
{
	const char *wall_name = script_getstr(st, 2);

	script_pushint(st, map_iwall_exist(wall_name));
	return SCRIPT_CMD_SUCCESS;
}

/// Retrieves various information about the specified guardian.
///
/// guardianinfo("<map_name>", <index>, <type>) -> <value>
/// type: 0 - whether it is deployed or not
///       1 - maximum hp
///       2 - current hp
///
BUILDIN_FUNC(guardianinfo)
{
	const char* mapname = mapindex_getmapname(script_getstr(st,2),NULL);
	int id = script_getnum(st,3);
	int type = script_getnum(st,4);

	std::shared_ptr<guild_castle> gc = castle_db.mapname2gc(mapname);
	struct mob_data* gd;

	if( gc == NULL || id < 0 || id >= MAX_GUARDIANS )
	{
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}

	if( type == 0 )
		script_pushint(st, gc->guardian[id].visible);
	else
	if( !gc->guardian[id].visible )
		script_pushint(st,-1);
	else
	if( (gd = map_id2md(gc->guardian[id].id)) == NULL )
		script_pushint(st,-1);
	else
	{
		if     ( type == 1 ) script_pushint(st,gd->status.max_hp);
		else if( type == 2 ) script_pushint(st,gd->status.hp);
		else
			script_pushint(st,-1);
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get the item name by item_id or null
 *------------------------------------------*/
BUILDIN_FUNC(getitemname)
{
	std::shared_ptr<item_data> i_data;

	if( script_isstring(st, 2) ){
		i_data = item_db.searchname( script_getstr( st, 2 ) );
	}else{
		i_data = item_db.find( script_getnum( st, 2 ) );
	}

	if( i_data == nullptr ){
		script_pushconststr(st,"null");
		return SCRIPT_CMD_SUCCESS;
	}

	char* item_name = (char *)aMalloc( ITEM_NAME_LENGTH * sizeof( char ) );

	memcpy(item_name, i_data->ename.c_str(), ITEM_NAME_LENGTH);
	script_pushstr(st,item_name);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns number of slots an item has. [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(getitemslots)
{
	t_itemid item_id=script_getnum(st,2);
	std::shared_ptr<item_data> i_data = item_db.find(item_id);

	if (i_data != nullptr)
		script_pushint(st,i_data->slots);
	else
		script_pushint(st,-1);
	return SCRIPT_CMD_SUCCESS;
}

// TODO: add matk here if needed/once we get rid of RENEWAL

/*==========================================
 * Returns some values of an item [Lupus]
 * Price, Weight, etc...
 *------------------------------------------*/
BUILDIN_FUNC(getiteminfo)
{
	std::shared_ptr<item_data> i_data;
	int type = script_getnum(st, 3);

	if (script_isstring(st, 2))
		i_data = item_db.searchname( script_getstr( st, 2 ) );
	else
		i_data = item_db.find( script_getnum( st, 2 ) );

	if (i_data == nullptr) {
		if (type != ITEMINFO_AEGISNAME)
			script_pushint(st, -1);
		else
			script_pushstrcopy(st, "");
		return SCRIPT_CMD_SUCCESS;
	}
	switch( type ) {
		case ITEMINFO_BUY: script_pushint(st, i_data->value_buy); break;
		case ITEMINFO_SELL: script_pushint(st, i_data->value_sell); break;
		case ITEMINFO_TYPE: script_pushint(st, i_data->type); break;
		case ITEMINFO_MAXCHANCE: script_pushint(st, i_data->maxchance); break;
		case ITEMINFO_GENDER: script_pushint(st, i_data->sex); break;
		case ITEMINFO_LOCATIONS: script_pushint(st, i_data->equip); break;
		case ITEMINFO_WEIGHT: script_pushint(st, i_data->weight); break;
		case ITEMINFO_ATTACK: script_pushint(st, i_data->atk); break;
		case ITEMINFO_DEFENSE: script_pushint(st, i_data->def); break;
		case ITEMINFO_RANGE: script_pushint(st, i_data->range); break;
		case ITEMINFO_SLOT: script_pushint(st, i_data->slots); break;
		case ITEMINFO_VIEW:
			if (i_data->type == IT_WEAPON || i_data->type == IT_AMMO) {	// keep old compatibility
				script_pushint(st, i_data->subtype);
			} else {
				script_pushint(st, i_data->look);
			}
			break;
		case ITEMINFO_EQUIPLEVELMIN: script_pushint(st, i_data->elv); break;
		case ITEMINFO_WEAPONLEVEL:
			if( i_data->type == IT_WEAPON ){
				script_pushint( st, i_data->weapon_level );
			}else{
				script_pushint( st, 0 );
			}
			break;
		case ITEMINFO_ARMORLEVEL:
			if( i_data->type == IT_ARMOR ){
				script_pushint( st, i_data->armor_level );
			}else{
				script_pushint( st, 0 );
			}
			break;
		case ITEMINFO_ALIASNAME: script_pushint(st, i_data->view_id); break;
		case ITEMINFO_EQUIPLEVELMAX: script_pushint(st, i_data->elvmax); break;
		case ITEMINFO_MAGICATTACK: {
#ifdef RENEWAL
			script_pushint(st, i_data->matk);
#else
			script_pushint(st, 0);
#endif
			break;
		}
		case ITEMINFO_ID: script_pushint(st, i_data->nameid); break;
		case ITEMINFO_AEGISNAME: script_pushstrcopy(st, i_data->name.c_str()); break;
		case ITEMINFO_SUBTYPE: script_pushint(st, i_data->subtype); break;
		
#ifdef Pandas_ScriptParams_GetItemInfo
		case -1: script_pushint(st, i_data->flag.no_refine ? 0 : 1); break;
		case -2: {
			int64 trade_mask = 0;
			if (i_data && i_data->flag.trade_restriction.drop)
				trade_mask |= 1;
			if (i_data && i_data->flag.trade_restriction.trade)
				trade_mask |= 2;
			if (i_data && i_data->flag.trade_restriction.trade_partner)
				trade_mask |= 4;
			if (i_data && i_data->flag.trade_restriction.sell)
				trade_mask |= 8;
			if (i_data && i_data->flag.trade_restriction.cart)
				trade_mask |= 16;
			if (i_data && i_data->flag.trade_restriction.storage)
				trade_mask |= 32;
			if (i_data && i_data->flag.trade_restriction.guild_storage)
				trade_mask |= 64;
			if (i_data && i_data->flag.trade_restriction.mail)
				trade_mask |= 128;
			if (i_data && i_data->flag.trade_restriction.auction)
				trade_mask |= 256;
			script_pushint(st, trade_mask);
			break;
		}

#ifdef Pandas_Struct_Item_Data_Properties
		case -3:
			script_pushint(st, ITEM_PROPERTIES_HASFLAG(i_data, special_mask, ITEM_PRO_AVOID_CONSUME_FOR_USE) ? 1 : 0);
			break;
		case -4:
			script_pushint(st, ITEM_PROPERTIES_HASFLAG(i_data, special_mask, ITEM_PRO_AVOID_CONSUME_FOR_SKILL) ? 1 : 0);
			break;
#else
		case -3: script_pushint(st, 0);	break;
		case -4: script_pushint(st, 0);	break;
#endif // Pandas_Struct_Item_Data_Properties

#ifdef Pandas_Struct_Item_Data_Taming_Mobid
		case -5: {
			if (script_hasdata(st, 4) && i_data->pandas.taming_mobid.size()) {
				int idx = 0;
				struct script_data* data = NULL;
				char* varname = NULL;

				data = script_getdata(st, 4);
				if (!data_isreference(data)) {
					ShowError("buildin_getiteminfo: Error in argument! Please give a variable to store values in.\n");
					return SCRIPT_CMD_FAILURE;
				}

				varname = reference_getname(data);
				if (varname[strlen(varname) - 1] == '$') {
					ShowError("buildin_getiteminfo: The array %s is not integer type.\n", varname);
					return SCRIPT_CMD_FAILURE;
				}

				if (not_server_variable(*varname)) {
					struct map_session_data* sd;

					if (!script_rid2sd(sd)) {
						ShowError("buildin_getiteminfo: Cannot use a player variable '%s' if no player is attached.\n", varname);
						return SCRIPT_CMD_FAILURE;
					}
				}

				for (auto it : i_data->pandas.taming_mobid) {
					setd_sub_num(st, NULL, varname, idx, it, data->ref);
					idx++;
				}
			}

			script_pushint(st, i_data->pandas.taming_mobid.size() ? 1 : 0);
			break;
		}
#else
		case -5: script_pushint(st, -2); break;
#endif // Pandas_Struct_Item_Data_Taming_Mobid

#ifdef Pandas_Struct_Item_Data_Has_CallFunc
		case -6: script_pushint(st, i_data->pandas.has_callfunc); break;
#else
		case -6: script_pushint(st, -2); break;
#endif // Pandas_Struct_Item_Data_Has_CallFunc

#ifdef Pandas_Persistence_Itemdb_Script
		case -7: {
			script_pushstrcopy(st, i_data->pandas.script_plaintext.script.c_str());
			break;
		}
		case -8: {
			script_pushstrcopy(st, i_data->pandas.script_plaintext.equip_script.c_str());
			break;
		}
		case -9: {
			script_pushstrcopy(st, i_data->pandas.script_plaintext.unequip_script.c_str());
			break;
		}
#else
		case -7: script_pushconststr(st, "UnCompiled"); break;
		case -8: script_pushconststr(st, "UnCompiled"); break;
		case -9: script_pushconststr(st, "UnCompiled"); break;
#endif // Pandas_Persistence_Itemdb_Script
#endif // Pandas_ScriptParams_GetItemInfo
		
		default:
			script_pushint(st, -1);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Set some values of an item [Lupus]
 * Price, Weight, etc...
 *------------------------------------------*/
BUILDIN_FUNC(setiteminfo)
{
	std::shared_ptr<item_data> i_data;

	if (script_isstring(st, 2))
		i_data = item_db.search_aegisname( script_getstr( st, 2 ) );
	else
		i_data = item_db.find( script_getnum( st, 2 ) );

	if (i_data == nullptr) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}
	int value = script_getnum(st,4);

	switch( script_getnum(st, 3) ) {
		case ITEMINFO_BUY: i_data->value_buy = static_cast<uint32>(value); break;
		case ITEMINFO_SELL: i_data->value_sell = static_cast<uint32>(value); break;
		case ITEMINFO_TYPE: i_data->type = static_cast<item_types>(value); break;
		case ITEMINFO_MAXCHANCE: i_data->maxchance = static_cast<int>(value); break;
		case ITEMINFO_GENDER: i_data->sex = static_cast<uint8>(value); break;
		case ITEMINFO_LOCATIONS: i_data->equip = static_cast<uint32>(value); break;
		case ITEMINFO_WEIGHT: i_data->weight = static_cast<uint32>(value); break;
		case ITEMINFO_ATTACK: i_data->atk = static_cast<uint32>(value); break;
		case ITEMINFO_DEFENSE: i_data->def = static_cast<uint32>(value); break;
		case ITEMINFO_RANGE: i_data->range = static_cast<uint16>(value); break;
		case ITEMINFO_SLOT: i_data->slots = static_cast<uint16>(value); break;
		case ITEMINFO_VIEW:
			if (i_data->type == IT_WEAPON || i_data->type == IT_AMMO) {	// keep old compatibility
				i_data->subtype = static_cast<uint8>(value);
			} else {
				i_data->look = static_cast<uint32>(value);
			}
			break;
		case ITEMINFO_EQUIPLEVELMIN: i_data->elv = static_cast<uint16>(value); break;
		case ITEMINFO_WEAPONLEVEL:
			if( i_data->type == IT_WEAPON ){
				if( value > MAX_WEAPON_LEVEL ){
					ShowError( "buildin_setiteminfo: weapon level %d is above maximum %d.\n", value, MAX_WEAPON_LEVEL );
					script_pushint( st, -1 );
					return SCRIPT_CMD_FAILURE;
				}

				i_data->weapon_level = static_cast<uint16>(value);
				break;
			}else{
				ShowError( "buildin_setiteminfo: Can not set a weapon level for %d.\n", i_data->nameid );
				script_pushint( st, -1 );
				return SCRIPT_CMD_FAILURE;
			}
		case ITEMINFO_ARMORLEVEL:
			if( i_data->type == IT_ARMOR ){
				if( value > MAX_ARMOR_LEVEL ){
					ShowError( "buildin_setiteminfo: armor level %d is above maximum %d.\n", value, MAX_ARMOR_LEVEL );
					script_pushint( st, -1 );
					return SCRIPT_CMD_FAILURE;
				}

				i_data->armor_level = static_cast<uint16>(value);
				break;
			}else{
				ShowError( "buildin_setiteminfo: Can not set an armor level for %d.\n", i_data->nameid );
				script_pushint( st, -1 );
				return SCRIPT_CMD_FAILURE;
			}
		case ITEMINFO_ALIASNAME: i_data->view_id = static_cast<t_itemid>(value); break;
		case ITEMINFO_EQUIPLEVELMAX: i_data->elvmax = static_cast<uint16>(value); break;
		case ITEMINFO_MAGICATTACK: {
#ifdef RENEWAL
			i_data->matk = static_cast<uint32>(value);
#else
			value = 0;
#endif
			break;
		}
		case ITEMINFO_SUBTYPE: i_data->subtype = static_cast<uint8>(value); break;
		default:
			script_pushint(st, -1);
			break;
	}
	script_pushint(st, value);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns value from equipped item slot n [Lupus]
	getequipcardid(num,slot)
	where
		num = eqip position slot
		slot = 0,1,2,3 (Card Slot N)

	This func returns CARD ID, 255,254,-255 (for card 0, if the item is produced)
		it's useful when you want to check item cards or if it's signed
	Useful for such quests as "Sign this refined item with players name" etc
		Hat[0] +4 -> Player's Hat[0] +4
 *------------------------------------------*/
BUILDIN_FUNC(getequipcardid)
{
	int i=-1,num,slot;
	TBL_PC *sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	num=script_getnum(st,2);
	slot=script_getnum(st,3);

	if (equip_index_check(num))
		i=pc_checkequip(sd,equip_bitmask[num]);
	if(i >= 0 && slot>=0 && slot<4)
		script_pushint(st,sd->inventory.u.items_inventory[i].card[slot]);
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * petskillbonus [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(petskillbonus)
{
	struct pet_data *pd;
	TBL_PC *sd;

	if(!script_rid2sd(sd) || sd->pd == NULL)
		return SCRIPT_CMD_FAILURE;

	pd = sd->pd;
	if (pd->bonus)
	{ //Clear previous bonus
		if (pd->bonus->timer != INVALID_TIMER)
			delete_timer(pd->bonus->timer, pet_skill_bonus_timer);
	} else //init
		pd->bonus = (struct pet_bonus *) aMalloc(sizeof(struct pet_bonus));

	pd->bonus->type = script_getnum(st,2);
	pd->bonus->val = script_getnum(st,3);
	pd->bonus->duration = script_getnum(st,4);
	pd->bonus->delay = script_getnum(st,5);

	if (pd->state.skillbonus == 1)
		pd->state.skillbonus = 0;	// waiting state

	// wait for timer to start
	if (battle_config.pet_equip_required && pd->pet.equip == 0)
		pd->bonus->timer = INVALID_TIMER;
	else
		pd->bonus->timer = add_timer(gettick()+pd->bonus->delay*1000, pet_skill_bonus_timer, sd->bl.id, 0);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * pet looting [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(petloot)
{
	int max;
	struct pet_data *pd;
	TBL_PC *sd;

	if(!script_rid2sd(sd) || sd->pd==NULL)
		return SCRIPT_CMD_SUCCESS;

	max=script_getnum(st,2);

	if(max < 1)
		max = 1;	//Let'em loot at least 1 item.
	else if (max > MAX_PETLOOT_SIZE)
		max = MAX_PETLOOT_SIZE;

	pd = sd->pd;
	if (pd->loot != NULL)
	{	//Release whatever was there already and reallocate memory
		pet_lootitem_drop(pd, pd->master);
		aFree(pd->loot->item);
	}
	else
		pd->loot = (struct pet_loot *)aMalloc(sizeof(struct pet_loot));

	pd->loot->item = (struct item *)aCalloc(max,sizeof(struct item));

	pd->loot->max=max;
	pd->loot->count = 0;
	pd->loot->weight = 0;
	return SCRIPT_CMD_SUCCESS;
}

#ifndef Pandas_ScriptCommand_GetInventoryList
/*==========================================
 * Set arrays with info of all sd inventory :
 * @inventorylist_id, @inventorylist_amount, @inventorylist_equip,
 * @inventorylist_refine, @inventorylist_identify, @inventorylist_attribute,
 * @inventorylist_card(0..3), @inventorylist_expire
 * @inventorylist_count = scalar
 *------------------------------------------*/
BUILDIN_FUNC(getinventorylist)
{
	TBL_PC *sd;
	char card_var[NAME_LENGTH], randopt_var[50];
	int i,j=0,k;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->inventory.u.items_inventory[i].nameid > 0 && sd->inventory.u.items_inventory[i].amount > 0){
			pc_setreg(sd,reference_uid(add_str("@inventorylist_id"), j),sd->inventory.u.items_inventory[i].nameid);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_idx"), j),i);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_amount"), j),sd->inventory.u.items_inventory[i].amount);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_equip"), j),sd->inventory.u.items_inventory[i].equip);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_refine"), j),sd->inventory.u.items_inventory[i].refine);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_identify"), j),sd->inventory.u.items_inventory[i].identify);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_attribute"), j),sd->inventory.u.items_inventory[i].attribute);
			for (k = 0; k < MAX_SLOTS; k++)
			{
				sprintf(card_var, "@inventorylist_card%d",k+1);
				pc_setreg(sd,reference_uid(add_str(card_var), j),sd->inventory.u.items_inventory[i].card[k]);
			}
			pc_setreg(sd,reference_uid(add_str("@inventorylist_expire"), j),sd->inventory.u.items_inventory[i].expire_time);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_bound"), j),sd->inventory.u.items_inventory[i].bound);
			pc_setreg(sd,reference_uid(add_str("@inventorylist_enchantgrade"), j),sd->inventory.u.items_inventory[i].enchantgrade);
			for (k = 0; k < MAX_ITEM_RDM_OPT; k++)
			{
				sprintf(randopt_var, "@inventorylist_option_id%d",k+1);
				pc_setreg(sd,reference_uid(add_str(randopt_var), j),sd->inventory.u.items_inventory[i].option[k].id);
				sprintf(randopt_var, "@inventorylist_option_value%d",k+1);
				pc_setreg(sd,reference_uid(add_str(randopt_var), j),sd->inventory.u.items_inventory[i].option[k].value);
				sprintf(randopt_var, "@inventorylist_option_parameter%d",k+1);
				pc_setreg(sd,reference_uid(add_str(randopt_var), j),sd->inventory.u.items_inventory[i].option[k].param);
			}
			pc_setreg(sd,reference_uid(add_str("@inventorylist_tradable"), j),pc_can_trade_item(sd, i));
			pc_setreg(sd,reference_uid(add_str("@inventorylist_favorite"), j),sd->inventory.u.items_inventory[i].favorite);
			j++;
		}
	}
	pc_setreg(sd,add_str("@inventorylist_count"),j);
	return SCRIPT_CMD_SUCCESS;
}
#else
/* ===========================================================
 * getinventorylist {<角色编号>{,<想查询的数据类型>}};
 * getcartlist {<角色编号>{,<想查询的数据类型>}};
 * getguildstoragelist {<角色编号>{,<想查询的数据类型>}};
 * getstoragelist {<角色编号>{,<想查询的数据类型>{,<仓库编号>}}};
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getinventorylist) {
	struct map_session_data* sd = nullptr;
	char card_var[NAME_LENGTH] = { 0 }, randopt_var[50] = { 0 };
	int j = 0, k = 0;

	if (!script_charid2sd(2, sd)) {
		return SCRIPT_CMD_FAILURE;
	}

	uint32 query_flag = INV_ALL;
	if (script_hasdata(st, 3))
		query_flag = script_getnum(st, 3);

	struct s_storage* stor = nullptr;
	struct item* inventory = nullptr;
	int stor_id = (script_hasdata(st, 4) ? script_getnum(st, 4) : 0);

	pc_setreg(sd, add_str("@inventorylist_count"), 0);
	
	if (!script_getstorage(st, sd, &stor, &inventory, stor_id)) {
		return SCRIPT_CMD_FAILURE;
	}

	if (st->state == RERUNLINE) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (!stor || !inventory) {
		const char* command = script_getfuncname(st);
		ShowError("buildin_%s: cannot read inventory or storage data.\n", command);
		return SCRIPT_CMD_FAILURE;
	}

#define setreg(flag, arrayname, value)\
	{ \
		if ((query_flag & flag) == flag) \
			pc_setreg(sd, reference_uid(add_str(arrayname), j), value); \
	}
#define setregstr(flag, arrayname, value)\
	{ \
		if ((query_flag & flag) == flag) \
			pc_setregstr(sd, reference_uid(add_str(arrayname), j), value); \
	}

	script_cleararray_pc(sd, "@inventorylist_id");
	script_cleararray_pc(sd, "@inventorylist_idx");
	script_cleararray_pc(sd, "@inventorylist_amount");
	script_cleararray_pc(sd, "@inventorylist_equip");
	script_cleararray_pc(sd, "@inventorylist_refine");
	script_cleararray_pc(sd, "@inventorylist_identify");
	script_cleararray_pc(sd, "@inventorylist_attribute");
	for (k = 0; k < MAX_SLOTS; k++) {
		sprintf(card_var, "@inventorylist_card%d", k + 1);
		script_cleararray_pc(sd, card_var);
	}
	script_cleararray_pc(sd, "@inventorylist_expire");
	script_cleararray_pc(sd, "@inventorylist_bound");
	script_cleararray_pc(sd, "@inventorylist_enchantgrade");
	for (k = 0; k < MAX_ITEM_RDM_OPT; k++) {
		sprintf(randopt_var, "@inventorylist_option_id%d", k + 1);
		script_cleararray_pc(sd, randopt_var);
		sprintf(randopt_var, "@inventorylist_option_value%d", k + 1);
		script_cleararray_pc(sd, randopt_var);
		sprintf(randopt_var, "@inventorylist_option_parameter%d", k + 1);
		script_cleararray_pc(sd, randopt_var);
	}
	script_cleararray_pc(sd, "@inventorylist_tradable");
	script_cleararray_pc(sd, "@inventorylist_favorite");
	script_cleararray_pc(sd, "@inventorylist_uid$");
	script_cleararray_pc(sd, "@inventorylist_equipswitch");

	for (int i = 0; i < stor->max_amount; i++) {
		if (inventory[i].nameid <= 0 || inventory[i].amount <= 0)
			continue;
		
		setreg(INV_ID, "@inventorylist_id", inventory[i].nameid);
		setreg(INV_IDX, "@inventorylist_idx", i);
		setreg(INV_AMOUNT, "@inventorylist_amount", inventory[i].amount);
		setreg(INV_EQUIP, "@inventorylist_equip", inventory[i].equip);
		setreg(INV_REFINE, "@inventorylist_refine", inventory[i].refine);
		setreg(INV_IDENTIFY, "@inventorylist_identify", inventory[i].identify);
		setreg(INV_ATTRIBUTE, "@inventorylist_attribute", inventory[i].attribute);
		for (k = 0; k < MAX_SLOTS; k++) {
			sprintf(card_var, "@inventorylist_card%d", k + 1);
			setreg(INV_CARD, card_var, inventory[i].card[k]);
		}
		setreg(INV_EXPIRE, "@inventorylist_expire", inventory[i].expire_time);
		setreg(INV_BOUND, "@inventorylist_bound", inventory[i].bound);
		setreg(INV_ENCHANTGRADE, "@inventorylist_enchantgrade", inventory[i].enchantgrade);
		for (k = 0; k < MAX_ITEM_RDM_OPT; k++) {
			sprintf(randopt_var, "@inventorylist_option_id%d", k + 1);
			setreg(INV_OPTION, randopt_var, inventory[i].option[k].id);
			sprintf(randopt_var, "@inventorylist_option_value%d", k + 1);
			setreg(INV_OPTION, randopt_var, inventory[i].option[k].value);
			sprintf(randopt_var, "@inventorylist_option_parameter%d", k + 1);
			setreg(INV_OPTION, randopt_var, inventory[i].option[k].param);
		}
		setreg(INV_TRADABLE, "@inventorylist_tradable", pc_can_trade_item(sd, inventory[i]));
		setreg(INV_FAVORITE, "@inventorylist_favorite", inventory[i].favorite);

		char unique_id[64 + 1] = { 0 };
		sprintf(unique_id, "%" PRIu64, inventory[i].unique_id);
		setregstr(INV_UID, "@inventorylist_uid$", unique_id);

		setreg(INV_EQUIPSWITCH, "@inventorylist_equipswitch", inventory[i].equipSwitch);
		j++;
	}
	pc_setreg(sd, add_str("@inventorylist_count"), j);

#undef setreg
#undef setregstr
	
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetInventoryList

/**
 * getskilllist ({<char_id>});
 **/
BUILDIN_FUNC(getskilllist)
{
	TBL_PC *sd;
	int i,j=0;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	for(i=0;i<MAX_SKILL;i++){
		if(sd->status.skill[i].id > 0 && sd->status.skill[i].lv > 0){
			pc_setreg(sd,reference_uid(add_str("@skilllist_id"), j),sd->status.skill[i].id);
			pc_setreg(sd,reference_uid(add_str("@skilllist_lv"), j),sd->status.skill[i].lv);
			pc_setreg(sd,reference_uid(add_str("@skilllist_flag"), j),sd->status.skill[i].flag);
			j++;
		}
	}
	pc_setreg(sd,add_str("@skilllist_count"),j);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * clearitem ({<char_id>});
 **/
BUILDIN_FUNC(clearitem)
{
	TBL_PC *sd;
	int i;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	for (i=0; i<MAX_INVENTORY; i++) {
		if (sd->inventory.u.items_inventory[i].amount) {
			pc_delitem(sd, i, sd->inventory.u.items_inventory[i].amount, 0, 0, LOG_TYPE_SCRIPT);
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Disguise Player (returns Mob/NPC ID if success, 0 on fail)
 * disguise <Monster ID>{,<char_id>};
 **/
BUILDIN_FUNC(disguise)
{
	int id;
	TBL_PC* sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	id = script_getnum(st,2);

	if (mobdb_checkid(id) || npcdb_checkid(id)) {
		pc_disguise(sd, id);
		script_pushint(st,id);
	} else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Undisguise Player (returns 1 if success, 0 on fail)
 * undisguise {<char_id>};
 **/
BUILDIN_FUNC(undisguise)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if (sd->disguise) {
		pc_disguise(sd, 0);
		script_pushint(st,0);
	} else {
		script_pushint(st,1);
	}
	return SCRIPT_CMD_SUCCESS;
}

 /**
 * Transform an NPC to another _class
 *
 * classchange(<view id>{,"<NPC name>","<flag>"});
 * @param flag: Specify target
 *   BC_AREA - Sprite is sent to players in the vicinity of the source (default).
 *   BC_SELF - Sprite is sent only to player attached.
 */
BUILDIN_FUNC(classchange)
{
	int _class, type = 1;
	struct npc_data* nd = NULL;
	TBL_PC *sd = map_id2sd(st->rid);
	send_target target = AREA;

	_class = script_getnum(st,2);

	if (script_hasdata(st, 3) && strlen(script_getstr(st,3)) > 0)
		nd = npc_name2id(script_getstr(st, 3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if (nd == NULL)
		return SCRIPT_CMD_FAILURE;

	if (script_hasdata(st, 4)) {
		switch(script_getnum(st, 4)) {
			case BC_SELF:	target = SELF;			break;
			case BC_AREA:
			default:		target = AREA;			break;
		}
	}
	if (target != SELF)
		clif_class_change(&nd->bl,_class,type);
	else if (sd == NULL)
		return SCRIPT_CMD_FAILURE;
	else
		clif_class_change_target(&nd->bl,_class,type,target,sd);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Display an effect
 *------------------------------------------*/
BUILDIN_FUNC(misceffect)
{
	int type;

	type=script_getnum(st,2);

	if( type <= EF_NONE || type >= EF_MAX ){
		ShowError( "buildin_misceffect: unsupported effect id %d\n", type );
		return SCRIPT_CMD_FAILURE;
	}


	if(st->oid && st->oid != fake_nd->bl.id) {
		struct block_list *bl = map_id2bl(st->oid);
		if (bl)
			clif_specialeffect(bl,type,AREA);
	} else{
		TBL_PC *sd;
		if(script_rid2sd(sd))
			clif_specialeffect(&sd->bl,type,AREA);
	}
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * Play a BGM on a single client [Rikter/Yommy]
 *------------------------------------------*/
BUILDIN_FUNC(playBGM)
{
	struct map_session_data* sd;

	if( script_rid2sd(sd) ) {
		clif_playBGM(sd, script_getstr(st,2));
	}
	return SCRIPT_CMD_SUCCESS;
}

static int playBGM_sub(struct block_list* bl,va_list ap)
{
	const char* name = va_arg(ap,const char*);
	clif_playBGM(BL_CAST(BL_PC, bl), name);
	return 0;
}

static int playBGM_foreachpc_sub(struct map_session_data* sd, va_list args)
{
	const char* name = va_arg(args, const char*);
	clif_playBGM(sd, name);
	return 0;
}

/*==========================================
 * Play a BGM on multiple client [Rikter/Yommy]
 *------------------------------------------*/
BUILDIN_FUNC(playBGMall)
{
	const char* name;
	name = script_getstr(st,2);

	if( script_hasdata(st,7) ) {// specified part of map
		const char* mapname = script_getstr(st,3);
		int x0 = script_getnum(st,4);
		int y0 = script_getnum(st,5);
		int x1 = script_getnum(st,6);
		int y1 = script_getnum(st,7);

		map_foreachinallarea(playBGM_sub, map_mapname2mapid(mapname), x0, y0, x1, y1, BL_PC, name);
	}
	else if( script_hasdata(st,3) ) {// entire map
		const char* mapname = script_getstr(st,3);

		map_foreachinmap(playBGM_sub, map_mapname2mapid(mapname), BL_PC, name);
	}
	else {// entire server
		map_foreachpc(&playBGM_foreachpc_sub, name);
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Play a .wav sound for sd
 *------------------------------------------*/
BUILDIN_FUNC(soundeffect)
{
	TBL_PC* sd;

	if(script_rid2sd(sd)){
		const char* name = script_getstr(st,2);
		int type = script_getnum(st,3);

		clif_soundeffect(sd,&sd->bl,name,type);
	}
	return SCRIPT_CMD_SUCCESS;
}

int soundeffect_sub(struct block_list* bl,va_list ap)
{
	char* name = va_arg(ap,char*);
	int type = va_arg(ap,int);

	clif_soundeffect((TBL_PC *)bl, bl, name, type);

	return 0;
}

/*==========================================
 * Play a sound effect (.wav) on multiple clients
 * soundeffectall "<filepath>",<type>{,"<map name>"}{,<x0>,<y0>,<x1>,<y1>};
 *------------------------------------------*/
BUILDIN_FUNC(soundeffectall)
{
	struct block_list* bl;
	struct map_session_data* sd;
	const char* name;
	int type;

	if( st->rid && script_rid2sd(sd) )
		bl = &sd->bl;
	else
		bl = map_id2bl(st->oid);

	if (!bl)
		return SCRIPT_CMD_SUCCESS;

	name = script_getstr(st,2);
	type = script_getnum(st,3);

	//FIXME: enumerating map squares (map_foreach) is slower than enumerating the list of online players (map_foreachpc?) [ultramage]

	if(!script_hasdata(st,4))
	{	// area around
		clif_soundeffectall(bl, name, type, AREA);
	}
	else
	if(!script_hasdata(st,5))
	{	// entire map
		const char* mapname = script_getstr(st,4);
		map_foreachinmap(soundeffect_sub, map_mapname2mapid(mapname), BL_PC, name, type);
	}
	else
	if(script_hasdata(st,8))
	{	// specified part of map
		const char* mapname = script_getstr(st,4);
		int x0 = script_getnum(st,5);
		int y0 = script_getnum(st,6);
		int x1 = script_getnum(st,7);
		int y1 = script_getnum(st,8);
		map_foreachinallarea(soundeffect_sub, map_mapname2mapid(mapname), x0, y0, x1, y1, BL_PC, name, type);
	}
	else
	{
		ShowError("buildin_soundeffectall: insufficient arguments for specific area broadcast.\n");
	}
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * pet status recovery [Valaris] / Rewritten by [Skotlex]
 *------------------------------------------*/
BUILDIN_FUNC(petrecovery)
{
	struct pet_data *pd;
	TBL_PC *sd;
	int sc;

	if(!script_rid2sd(sd) || sd->pd == NULL)
		return SCRIPT_CMD_FAILURE;

	sc = script_getnum(st,2);
	if (sc <= SC_NONE || sc >= SC_MAX) {
		ShowError("buildin_petrecovery: Invalid SC type: %d\n", sc);
		return SCRIPT_CMD_FAILURE;
	}

	pd = sd->pd;

	if (pd->recovery)
	{ //Halt previous bonus
		if (pd->recovery->timer != INVALID_TIMER)
			delete_timer(pd->recovery->timer, pet_recovery_timer);
	} else //Init
		pd->recovery = (struct pet_recovery *)aMalloc(sizeof(struct pet_recovery));

	pd->recovery->type = (sc_type)sc;
	pd->recovery->delay = script_getnum(st,3);
	pd->recovery->timer = INVALID_TIMER;
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * pet attack skills [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------*/
/// petskillattack <skill id>,<level>,<rate>,<bonusrate>
/// petskillattack "<skill name>",<level>,<rate>,<bonusrate>
BUILDIN_FUNC(petskillattack)
{
	struct pet_data *pd;
	TBL_PC *sd;
	int id = 0;

	if(!script_rid2sd(sd) || sd->pd == NULL)
		return SCRIPT_CMD_FAILURE;

	id = (script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : skill_get_index(script_getnum(st,2)));
	if (!id) {
		ShowError("buildin_petskillattack: Invalid skill defined!\n");
		return SCRIPT_CMD_FAILURE;
	}

	pd = sd->pd;
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_skill_attack *)aMalloc(sizeof(struct pet_skill_attack));

	pd->a_skill->id = id;
	pd->a_skill->damage = 0;
	pd->a_skill->lv = (unsigned short)min(script_getnum(st,3), skill_get_max(pd->a_skill->id));
	pd->a_skill->div_ = 0;
	pd->a_skill->rate = script_getnum(st,4);
	pd->a_skill->bonusrate = script_getnum(st,5);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * pet attack skills [Valaris]
 *------------------------------------------*/
/// petskillattack2 <skill id>,<damage>,<div>,<rate>,<bonusrate>
/// petskillattack2 "<skill name>",<damage>,<div>,<rate>,<bonusrate>
BUILDIN_FUNC(petskillattack2)
{
	struct pet_data *pd;
	TBL_PC *sd;
	int id = 0;

	if(!script_rid2sd(sd) || sd->pd == NULL)
		return SCRIPT_CMD_FAILURE;

	id = (script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : skill_get_index(script_getnum(st,2)));
	if (!id) {
		ShowError("buildin_petskillattack2: Invalid skill defined!\n");
		return SCRIPT_CMD_FAILURE;
	}

	pd = sd->pd;
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_skill_attack *)aMalloc(sizeof(struct pet_skill_attack));

	pd->a_skill->id = id;
	pd->a_skill->damage = script_getnum(st,3); // Fixed damage
	pd->a_skill->lv = (unsigned short)skill_get_max(pd->a_skill->id); // Adjust to max skill level
	pd->a_skill->div_ = script_getnum(st,4);
	pd->a_skill->rate = script_getnum(st,5);
	pd->a_skill->bonusrate = script_getnum(st,6);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * pet support skills [Skotlex]
 *------------------------------------------*/
/// petskillsupport <skill id>,<level>,<delay>,<hp>,<sp>
/// petskillsupport "<skill name>",<level>,<delay>,<hp>,<sp>
BUILDIN_FUNC(petskillsupport)
{
	struct pet_data *pd;
	TBL_PC *sd;
	int id = 0;

	if(!script_rid2sd(sd) || sd->pd == NULL)
		return SCRIPT_CMD_FAILURE;

	id = (script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : skill_get_index(script_getnum(st,2)));
	if (!id) {
		ShowError("buildin_petskillsupport: Invalid skill defined!\n");
		return SCRIPT_CMD_FAILURE;
	}

	pd = sd->pd;
	if (pd->s_skill)
	{ //Clear previous skill
		if (pd->s_skill->timer != INVALID_TIMER)
		{
			if (pd->s_skill->id)
				delete_timer(pd->s_skill->timer, pet_skill_support_timer);
			else
				delete_timer(pd->s_skill->timer, pet_heal_timer);
		}
	} else //init memory
		pd->s_skill = (struct pet_skill_support *) aMalloc(sizeof(struct pet_skill_support));

	pd->s_skill->id = id;
	pd->s_skill->lv = script_getnum(st,3);
	pd->s_skill->delay = script_getnum(st,4);
	pd->s_skill->hp = script_getnum(st,5);
	pd->s_skill->sp = script_getnum(st,6);

	//Use delay as initial offset to avoid skill/heal exploits
	if (battle_config.pet_equip_required && pd->pet.equip == 0)
		pd->s_skill->timer = INVALID_TIMER;
	else
		pd->s_skill->timer = add_timer(gettick()+pd->s_skill->delay*1000,pet_skill_support_timer,sd->bl.id,0);
	return SCRIPT_CMD_SUCCESS;
}

static inline void script_skill_effect(block_list *bl, uint16 skill_id, uint16 skill_lv, int16 x, int16 y) {
	nullpo_retv(bl);

	switch (skill_get_casttype(skill_id)) {
		case CAST_GROUND:
			clif_skill_poseffect(bl, skill_id, skill_lv, x, y, gettick());
			break;
		case CAST_NODAMAGE:
			clif_skill_nodamage(bl, bl, skill_id, skill_lv, 1);
			break;
		case CAST_DAMAGE:
			clif_skill_damage(bl, bl, gettick(), status_get_amotion(bl), status_get_dmotion(bl), 0, 1, skill_id, skill_lv, skill_get_hit(skill_id));
			break;
	}
}

/*==========================================
 * Scripted skill effects [Celest]
 *------------------------------------------*/
/// skilleffect <skill id>,<level>
/// skilleffect "<skill name>",<level>
BUILDIN_FUNC(skilleffect)
{
	TBL_PC *sd;
	uint16 skill_id, skill_lv;
	
	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_FAILURE;
	
	skill_id = ( script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2) );
	skill_lv = script_getnum(st,3);

	if (skill_db.find(skill_id) == nullptr) {
		ShowError("buildin_skilleffect: Invalid skill defined (%s)!\n", script_getstr(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	/* Ensure we're standing because the following packet causes the client to virtually set the char to stand,
	 * which leaves the server thinking it still is sitting. */
	if( pc_issit(sd) && pc_setstand(sd, false) ) {
		skill_sit(sd, 0);
		clif_standing(&sd->bl);
	}

	script_skill_effect(&sd->bl, skill_id, skill_lv, sd->bl.x, sd->bl.y);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * NPC skill effects [Valaris]
 *------------------------------------------*/
/// npcskilleffect <skill id>,<level>,<x>,<y>
/// npcskilleffect "<skill name>",<level>,<x>,<y>
BUILDIN_FUNC(npcskilleffect)
{
	struct block_list *bl= map_id2bl(st->oid);

	if (bl == nullptr) {
		ShowError("buildin_npcskilleffect: Invalid object attached to NPC.");
		return SCRIPT_CMD_FAILURE;
	}

	uint16 skill_id, skill_lv;
	int16 x, y;

	skill_id=( script_isstring(st, 2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2) );
	skill_lv=script_getnum(st,3);
	x=script_getnum(st,4);
	y=script_getnum(st,5);

	if (skill_db.find(skill_id) == nullptr) {
		ShowError("buildin_npcskilleffect: Invalid skill defined (%s)!\n", script_getstr(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	script_skill_effect(bl, skill_id, skill_lv, bl->x, bl->y);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Special effects [Valaris]
 *------------------------------------------*/
BUILDIN_FUNC(specialeffect)
{
	struct block_list *bl=map_id2bl(st->oid);
	int type = script_getnum(st,2);
	enum send_target target = script_hasdata(st,3) ? (send_target)script_getnum(st,3) : AREA;

	if(bl==NULL)
		return SCRIPT_CMD_SUCCESS;

	if( type <= EF_NONE || type >= EF_MAX ){
		ShowError( "buildin_specialeffect: unsupported effect id %d\n", type );
		return SCRIPT_CMD_FAILURE;
	}

	if( script_hasdata(st,4) )
	{
		TBL_NPC *nd = npc_name2id(script_getstr(st,4));
		if(nd)
			clif_specialeffect(&nd->bl, type, target);
	}
	else
	{
		if (target == SELF) {
			TBL_PC *sd;
			if (script_rid2sd(sd))
				clif_specialeffect_single(bl,type,sd->fd);
		} else {
			clif_specialeffect(bl, type, target);
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(specialeffect2)
{
	TBL_PC *sd;

	if( script_nick2sd(4,sd) ){
		int type = script_getnum(st,2);
		enum send_target target = script_hasdata(st,3) ? (send_target)script_getnum(st,3) : AREA;

		if( type <= EF_NONE || type >= EF_MAX ){
			ShowError( "buildin_specialeffect2: unsupported effect id %d\n", type );
			return SCRIPT_CMD_FAILURE;
		}

		clif_specialeffect(&sd->bl, type, target);
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(removespecialeffect)
{
	const char* command = script_getfuncname(st);

#if PACKETVER < 20181002
	ShowError("buildin_%s: This command is not supported for PACKETVER older than 2018-10-02.\n", command);

	return SCRIPT_CMD_FAILURE;
#endif

	int effect = script_getnum(st, 2);

	if (effect <= EF_NONE || effect >= EF_MAX) {
		ShowError("buildin_%s: unsupported effect id %d.\n", command, effect);
		return SCRIPT_CMD_FAILURE;
	}

	send_target e_target = script_hasdata(st, 3) ? static_cast<send_target>(script_getnum(st, 3)) : AREA;

	struct block_list *bl_src;
	struct block_list *bl_target;
	struct map_session_data *sd;

	if( strcmp(command, "removespecialeffect") == 0 ) {
		if (!script_hasdata(st, 4)) {
			bl_src = map_id2bl(st->oid);

			if (bl_src == nullptr) {
				ShowError("buildin_%s: npc not attached.\n", command);
				return SCRIPT_CMD_FAILURE;
			}
		}
		else {
			struct npc_data *nd = npc_name2id(script_getstr(st, 4));
			if (nd == nullptr) {
				ShowError("buildin_%s: unknown npc %s.\n", command, script_getstr(st, 4));
				return SCRIPT_CMD_FAILURE;
			}
			bl_src = &nd->bl;
		}

		if (e_target != SELF)
			bl_target = bl_src;
		else {
			if (!script_rid2sd(sd))
				return SCRIPT_CMD_FAILURE;
			bl_target = &sd->bl;
		}
	}else{
		if (!script_nick2sd(4, sd))
			return SCRIPT_CMD_FAILURE;

		bl_src = bl_target = &sd->bl;
	}

	clif_specialeffect_remove(bl_src, effect, e_target, bl_target);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * nude({<char_id>});
 * @author [Valaris]
 **/
BUILDIN_FUNC(nude)
{
	TBL_PC *sd;
	int i, calcflag = 0;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	for( i = 0 ; i < EQI_MAX; i++ ) {
		if( sd->equip_index[ i ] >= 0 ) {
			if( !calcflag )
				calcflag = 1;
			pc_unequipitem( sd , sd->equip_index[ i ] , 2);
		}
	}

	if( calcflag )
		status_calc_pc(sd,SCO_NONE);
	return SCRIPT_CMD_SUCCESS;
}

int atcommand_sub(struct script_state* st,int type) {
	TBL_PC *sd, dummy_sd;
	int fd;
	const char *cmd;

	cmd = script_getstr(st,2);

	if (st->rid) {
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;

		fd = sd->fd;
	} else { //Use a dummy character.
		sd = &dummy_sd;
		fd = 0;

		memset(&dummy_sd, 0, sizeof(TBL_PC));
		if (st->oid) {
			struct block_list* bl = map_id2bl(st->oid);
			memcpy(&dummy_sd.bl, bl, sizeof(struct block_list));
			if (bl->type == BL_NPC)
				safestrncpy(dummy_sd.status.name, ((TBL_NPC*)bl)->name, NAME_LENGTH);
			sd->mapindex = (bl->m > 0) ? map_id2index(bl->m) : mapindex_name2id(map_default.mapname);
		}

		// Init Group ID, Level, & permissions
		sd->group_id = 99;
		pc_group_pc_load( sd );
	}

	if (!is_atcommand(fd, sd, cmd, type)) {
		ShowWarning("buildin_atcommand: failed to execute command '%s'\n", cmd);
		script_reportsrc(st);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * gmcommand [MouseJstr]
 *------------------------------------------*/
BUILDIN_FUNC(atcommand) {
	return atcommand_sub(st,0);
}

/** Displays a message for the player only (like system messages like "you got an apple" )
* dispbottom("<message>"{,<color>{,<char_id>}})
* @param message 
* @param color Hex color default (Green)
*/
BUILDIN_FUNC(dispbottom)
{
	TBL_PC *sd;
	int color = 0;
	const char *message;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	message = script_getstr(st,2);

	if (script_hasdata(st,3))
		color = script_getnum(st,3); // <color>

	if(sd) {
		if (script_hasdata(st,3))
			clif_messagecolor(&sd->bl, color, message, true, SELF);		// [Napster]
		else
			clif_messagecolor(&sd->bl, color_table[COLOR_LIGHT_GREEN], message, false, SELF);
	}
	return SCRIPT_CMD_SUCCESS;
}

/*===================================
 * Heal portion of recovery command
 *-----------------------------------*/
int recovery_sub(struct map_session_data* sd, int revive)
{
	if(revive&(1|4) && pc_isdead(sd)) {
		status_revive(&sd->bl, 100, 100);
		clif_displaymessage(sd->fd,msg_txt(sd,16)); // You've been revived!
		clif_specialeffect(&sd->bl, EF_RESURRECTION, AREA);
	} else if(revive&(1|2) && !pc_isdead(sd)) {
		status_percent_heal(&sd->bl, 100, 100);
		clif_displaymessage(sd->fd,msg_txt(sd,680)); // You have been recovered!
	}
	return SCRIPT_CMD_SUCCESS;
}

/*=========================================================================
 * Fully Recover a Character's HP/SP - [Capuche] & [Akinari]
 * recovery <type>{,<option>,<revive_flag>{,<map name>}};
 * <type> determines <option>:
 *	0 : char_id
 *	1 : party_id
 *	2 : guild_id
 *	3 : map_name
 *	4 : all characters
 * <revive_flag>:
 *	1 : Revive and heal all players (default)
 *	2 : Heal living players only
 *	4 : Revive dead players only
 * <map name>:
 *	for types 1-2 : map_name (null = all maps)
 *-------------------------------------------------------------------------*/
BUILDIN_FUNC(recovery)
{
	TBL_PC *sd;
	int map_idx = 0, type = 0, revive = 1;

	type = script_getnum(st,2);

	if(script_hasdata(st,4))
		revive = script_getnum(st,4);

	switch(type) {
		case 0:
			if(!script_charid2sd(3,sd))
				return SCRIPT_CMD_SUCCESS; //If we don't have sd by now, bail out
			recovery_sub(sd, revive);
			break;
		case 1:
		{
			struct party_data* p;
			//When no party given, we use invoker party
			int p_id = 0, i;
			if(script_hasdata(st,5)) {//Bad maps shouldn't cause issues
				map_idx = map_mapname2mapid(script_getstr(st,5));
				if(map_idx < 1) { //But we'll check anyways
					ShowDebug("recovery: bad map name given (%s)\n", script_getstr(st,5));
					return SCRIPT_CMD_FAILURE;
				}
			}
			if(script_hasdata(st,3))
				p_id = script_getnum(st,3);
			else if(script_rid2sd(sd))
				p_id = sd->status.party_id;
			p = party_search(p_id);
			if(p == NULL)
				return SCRIPT_CMD_SUCCESS;
			for (i = 0; i < MAX_PARTY; i++) {
				struct map_session_data* pl_sd;
				if((!(pl_sd = p->data[i].sd) || pl_sd->status.party_id != p_id)
					|| (map_idx && pl_sd->bl.m != map_idx))
					continue;
				recovery_sub(pl_sd, revive);
			}
			break;
		}
		case 2:
		{
			struct guild* g;
			//When no guild given, we use invoker guild
			int g_id = 0, i;
			if(script_hasdata(st,5)) {//Bad maps shouldn't cause issues
				map_idx = map_mapname2mapid(script_getstr(st,5));
				if(map_idx < 1) { //But we'll check anyways
					ShowDebug("recovery: bad map name given (%s)\n", script_getstr(st,5));
					return SCRIPT_CMD_FAILURE;
				}
			}
			if(script_hasdata(st,3))
				g_id = script_getnum(st,3);
			else if(script_rid2sd(sd))
				g_id = sd->status.guild_id;
			g = guild_search(g_id);
			if(g == NULL)
				return SCRIPT_CMD_SUCCESS;
			for (i = 0; i < MAX_GUILD; i++) {
				struct map_session_data* pl_sd;
				if((!(pl_sd = g->member[i].sd) || pl_sd->status.guild_id != g_id)
					|| (map_idx && pl_sd->bl.m != map_idx))
					continue;
				recovery_sub(pl_sd, revive);
			}
			break;
		}
		case 3:
			if(script_hasdata(st,3))
				map_idx = map_mapname2mapid(script_getstr(st,3));
			else if(script_rid2sd(sd))
				map_idx = sd->bl.m;
			if(map_idx < 1)
				return SCRIPT_CMD_FAILURE; //No sd and no map given - return
		case 4:
		{
			struct s_mapiterator *iter;

			if(script_hasdata(st,3) && script_isint(st, 3))
				revive = script_getnum(st,3); // recovery 4,<revive_flag>;
			iter = mapit_getallusers();
			for (sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter)) {
				if(type == 3 && sd->bl.m != map_idx)
					continue;
				recovery_sub(sd, revive);
			}
			mapit_free(iter);
			break;
		}
		default:
			ShowWarning("script: buildin_recovery: Invalid type %d\n", type);
			script_pushint(st,-1);
			return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st,1); //Successfully executed without errors
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get your pet info: getpetinfo <type>{,<char_id>}
 * n -> 0:pet_id 1:pet_class 2:pet_name
 * 3:friendly 4:hungry, 5: rename flag.6:level,
 * 7:game id
 *------------------------------------------*/
BUILDIN_FUNC(getpetinfo)
{
	TBL_PC *sd;
	TBL_PET *pd;
	int type = script_getnum(st,2);

	if (!script_charid2sd(3, sd) || !(pd = sd->pd)) {
		if (type == 2)
			script_pushconststr(st,"null");
		else
			script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch(type){
		case PETINFO_ID:		script_pushint(st,pd->pet.pet_id); break;
		case PETINFO_CLASS:		script_pushint(st,pd->pet.class_); break;
		case PETINFO_NAME:		script_pushstrcopy(st,pd->pet.name); break;
		case PETINFO_INTIMATE:	script_pushint(st,pd->pet.intimate); break;
		case PETINFO_HUNGRY:	script_pushint(st,pd->pet.hungry); break;
		case PETINFO_RENAMED:	script_pushint(st,pd->pet.rename_flag); break;
		case PETINFO_LEVEL:		script_pushint(st,(int)pd->pet.level); break;
		case PETINFO_BLOCKID:	script_pushint(st,pd->bl.id); break;
		case PETINFO_EGGID:		script_pushint(st,pd->pet.egg_id); break;
		case PETINFO_FOODID:	script_pushint(st,pd->get_pet_db()->FoodID); break;
		default:
			script_pushint(st,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Get your homunculus info: gethominfo <type>{,<char_id>};
 * type -> 0:hom_id 1:class 2:name
 * 3:friendly 4:hungry, 5: rename flag.
 * 6: level, 7: game id
 *------------------------------------------*/
BUILDIN_FUNC(gethominfo)
{
	TBL_PC *sd;
	TBL_HOM *hd;
	int type=script_getnum(st,2);

	if (!script_charid2sd(3, sd) || !(hd = sd->hd)) {
		if (type == 2)
			script_pushconststr(st,"null");
		else
			script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch(type){
		case 0: script_pushint(st,hd->homunculus.hom_id); break;
		case 1: script_pushint(st,hd->homunculus.class_); break;
		case 2: script_pushstrcopy(st,hd->homunculus.name); break;
		case 3: script_pushint(st,hd->homunculus.intimacy); break;
		case 4: script_pushint(st,hd->homunculus.hunger); break;
		case 5: script_pushint(st,hd->homunculus.rename_flag); break;
		case 6: script_pushint(st,hd->homunculus.level); break;
		case 7: script_pushint(st,hd->bl.id); break;
		default:
			script_pushint(st,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(addhomintimacy)
{
	map_session_data *sd;
	homun_data *hd;

	if (!script_charid2sd(3, sd) || !(hd = sd->hd))
		return SCRIPT_CMD_FAILURE;

	int32 value = script_getnum(st, 2);

	if (value == 0) // Nothing to change
		return SCRIPT_CMD_SUCCESS;

	if (value > 0)
		hom_increase_intimacy(hd, (uint32)value);
	else
		hom_decrease_intimacy(hd, (uint32)abs(value));
	clif_send_homdata(sd, SP_INTIMATE, hd->homunculus.intimacy / 100);
	return SCRIPT_CMD_SUCCESS;
}

/// Retrieves information about character's mercenary
/// getmercinfo <type>[,<char id>];
BUILDIN_FUNC(getmercinfo)
{
	int type;
	struct map_session_data* sd;
	s_mercenary_data* md;

	if( !script_charid2sd(3,sd) ){
		script_pushnil(st);
		return SCRIPT_CMD_FAILURE;
	}

	type = script_getnum(st,2);
	md = sd->md;

	if( md == NULL ){
		if( type == 2 )
			script_pushconststr(st,"");
		else
			script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch( type )
	{
		case 0: script_pushint(st,md->mercenary.mercenary_id); break;
		case 1: script_pushint(st,md->mercenary.class_); break;
		case 2: script_pushstrcopy(st,md->db->name.c_str()); break;
		case 3: script_pushint(st,mercenary_get_faith(md)); break;
		case 4: script_pushint(st,mercenary_get_calls(md)); break;
		case 5: script_pushint(st,md->mercenary.kill_count); break;
		case 6: script_pushint(st,mercenary_get_lifetime(md)); break;
		case 7: script_pushint(st,md->db->lv); break;
		case 8: script_pushint(st,md->bl.id); break;
		default:
			ShowError("buildin_getmercinfo: Invalid type %d (char_id=%d).\n", type, sd->status.char_id);
			script_pushnil(st);
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Shows wether your inventory(and equips) contain
   selected card or not.
	checkequipedcard(4001);
 *------------------------------------------*/
BUILDIN_FUNC(checkequipedcard)
{
	TBL_PC *sd;

	if(script_rid2sd(sd)){
		int n,i,c=0;
		c=script_getnum(st,2);

		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->inventory.u.items_inventory[i].nameid > 0 && sd->inventory.u.items_inventory[i].amount && sd->inventory_data[i]){
				if (itemdb_isspecial(sd->inventory.u.items_inventory[i].card[0]))
					continue;
				for (n=0; n < MAX_SLOTS; n++) {
					if(sd->inventory.u.items_inventory[i].card[n] == c) {
						script_pushint(st,1);
						return SCRIPT_CMD_SUCCESS;
					}
				}
			}
		}
	}
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(jump_zero)
{
	int64 sel=script_getnum64(st,2);
	if(!sel) {
		int pos;
		if( !data_islabel(script_getdata(st,3)) ){
			ShowError("script: jump_zero: not label !\n");
			st->state=END;
			return SCRIPT_CMD_FAILURE;
		}

		pos=script_getnum(st,3);
		st->pos=pos;
		st->state=GOTO;
	}
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * movenpc [MouseJstr]
 *------------------------------------------*/
BUILDIN_FUNC(movenpc)
{
	TBL_NPC *nd = NULL;
	const char *npc;
	int x,y;

	npc = script_getstr(st,2);
	x = script_getnum(st,3);
	y = script_getnum(st,4);

	if ((nd = npc_name2id(npc)) == NULL){
		ShowError("script: movenpc: NPC with ID '%s' was not found!\n", npc );
		return -1;
	}

	if (script_hasdata(st,5))
		nd->ud.dir = script_getnum(st,5) % 8;
	npc_movenpc(nd, x, y);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * message [MouseJstr]
 *------------------------------------------*/
BUILDIN_FUNC(message)
{
	const char *msg,*player;
	TBL_PC *pl_sd = NULL;

	player = script_getstr(st,2);
	msg = script_getstr(st,3);

	if((pl_sd=map_nick2sd((char *) player,false)) == NULL)
		return SCRIPT_CMD_SUCCESS;
	clif_displaymessage(pl_sd->fd, msg);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * npctalk("<message>"{,"<NPC name>","<flag>"});
 * @param flag: Specify target
 *   BC_ALL  - Broadcast message is sent server-wide.
 *   BC_MAP  - Message is sent to everyone in the same map as the source of the npc.
 *   BC_AREA - Message is sent to players in the vicinity of the source (default).
 *   BC_SELF - Message is sent only to player attached.
 */
BUILDIN_FUNC(npctalk)
{
	struct npc_data* nd = NULL;
	const char* str = script_getstr(st,2);
	int color = 0xFFFFFF;

	if (script_hasdata(st, 3) && strlen(script_getstr(st,3)) > 0)
		nd = npc_name2id(script_getstr(st, 3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if (script_hasdata(st, 5))
		color = script_getnum(st, 5);

	if (nd != NULL) {
		send_target target = AREA;
		char message[CHAT_SIZE_MAX];

		if (script_hasdata(st, 4)) {
			switch(script_getnum(st, 4)) {
				case BC_ALL:	target = ALL_CLIENT;	break;
				case BC_MAP:	target = ALL_SAMEMAP;	break;
				case BC_SELF:	target = SELF;			break;
				case BC_AREA:
				default:		target = AREA;			break;
			}
		}
		safesnprintf(message, sizeof(message), "%s", str);
		if (target != SELF)
			clif_messagecolor(&nd->bl, color, message, true, target);
		else {
			TBL_PC *sd = map_id2sd(st->rid);
			if (sd == NULL)
				return SCRIPT_CMD_FAILURE;
			clif_messagecolor_target(&nd->bl, color, message, true, target, sd);
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Sends a message to the waitingroom of the invoking NPC.
 * chatmes "<message>"{,"<NPC name>"};
 * @author Jey
 */
BUILDIN_FUNC(chatmes)
{
	struct npc_data* nd = NULL;
	const char* str = script_getstr(st,2);

	if (script_hasdata(st, 3))
		nd = npc_name2id(script_getstr(st, 3));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if (nd != NULL && nd->chat_id) {
		char message[256];
		safesnprintf(message, sizeof(message), "%s", str);
		clif_GlobalMessage(map_id2bl(nd->chat_id), message, CHAT_WOS);
	}
	return SCRIPT_CMD_SUCCESS;
}

// change npc walkspeed [Valaris]
BUILDIN_FUNC(npcspeed)
{
	struct npc_data* nd;
	int speed;

	speed = script_getnum(st,2);
	nd =(struct npc_data *)map_id2bl(st->oid);

	if( nd ) {
		nd->speed = speed;
		nd->ud.state.speed_changed = 1;
	}

	return SCRIPT_CMD_SUCCESS;
}
// make an npc walk to a position [Valaris]
BUILDIN_FUNC(npcwalkto)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	int x=0,y=0;

	x=script_getnum(st,2);
	y=script_getnum(st,3);

	if(nd) {
		if (!nd->status.hp)
			status_calc_npc(nd, SCO_FIRST);
		else
			status_calc_npc(nd, SCO_NONE);
		unit_walktoxy(&nd->bl,x,y,0);
	}
	return SCRIPT_CMD_SUCCESS;
}

// stop an npc's movement [Valaris]
BUILDIN_FUNC(npcstop)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd) {
		unit_stop_walking(&nd->bl,1|4);
	}
	return SCRIPT_CMD_SUCCESS;
}


/**
 * getlook(<type>{,<char_id>})
 **/
BUILDIN_FUNC(getlook)
{
	int type,val;
	TBL_PC *sd;

	if (!script_charid2sd(3,sd)) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	type=script_getnum(st,2);
	val=-1;
	switch(type) {
		// TODO: implement LOOK_BASE as stated in script doc
		case LOOK_HAIR:     	val=sd->status.hair; break; //1
		case LOOK_WEAPON:   	val=sd->status.weapon; break; //2
		case LOOK_HEAD_BOTTOM:	val=sd->status.head_bottom; break; //3
		case LOOK_HEAD_TOP: 	val=sd->status.head_top; break; //4
		case LOOK_HEAD_MID: 	val=sd->status.head_mid; break; //5
		case LOOK_HAIR_COLOR:	val=sd->status.hair_color; break; //6
		case LOOK_CLOTHES_COLOR:	val=sd->status.clothes_color; break; //7
		case LOOK_SHIELD:   	val=sd->status.shield; break; //8
		case LOOK_SHOES:    	break; //9
		case LOOK_ROBE:     	val=sd->status.robe; break; //12
		case LOOK_BODY2:		val=sd->status.body; break; //13
	}

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * getsavepoint(<information type>{,<char_id>})
 * @param type 0- map name, 1- x, 2- y
 **/
BUILDIN_FUNC(getsavepoint)
{
	TBL_PC* sd;
	int type;

	if (!script_charid2sd(3,sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	type = script_getnum(st,2);

	switch(type) {
		case 0: script_pushstrcopy(st,mapindex_id2name(sd->status.save_point.map)); break;
		case 1: script_pushint(st,sd->status.save_point.x); break;
		case 2: script_pushint(st,sd->status.save_point.y); break;
		default:
			script_pushint(st,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get position for BL objects.
 * getmapxy(<map name>,<x>,<y>,<type>{,<char name>});
 * @param mapname: String variable for output map name
 * @param x: Integer variable for output coord X
 * @param y: Integer variable for output coord Y
 * @param type: Type of object
 *   BL_PC - Character coord
 *   BL_NPC - NPC coord
 *   BL_PET - Pet coord
 *   BL_HOM - Homun coord
 *   BL_MER - Mercenary coord
 *   BL_ELEM - Elemental coord
 * @param charname: Name object. If empty or "this" use the current object
 * @return 0 - success; -1 - some error, MapName$,MapX,MapY contains unknown value.
 */
BUILDIN_FUNC(getmapxy)
{
	struct block_list *bl = NULL;
	TBL_PC *sd=NULL;

	int64 num;
	const char *name;
	char prefix;

	int x,y,type = BL_PC;
	char mapname[MAP_NAME_LENGTH];

	if( !data_isreference(script_getdata(st,2)) ) {
		ShowWarning("script: buildin_getmapxy: mapname value is not a variable.\n");
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}
	if( !data_isreference(script_getdata(st,3)) ) {
		ShowWarning("script: buildin_getmapxy: mapx value is not a variable.\n");
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}
	if( !data_isreference(script_getdata(st,4)) ) {
		ShowWarning("script: buildin_getmapxy: mapy value is not a variable.\n");
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( !is_string_variable(reference_getname(script_getdata(st, 2))) ) {
		ShowWarning("script: buildin_getmapxy: %s is not a string variable.\n",reference_getname(script_getdata(st, 2)));
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( is_string_variable(reference_getname(script_getdata(st, 3))) ) {
		ShowWarning("script: buildin_getmapxy: %s is a string variable, should be an INT.\n",reference_getname(script_getdata(st, 3)));
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( is_string_variable(reference_getname(script_getdata(st, 4))) ) {
		ShowWarning("script: buildin_getmapxy: %s is a string variable, should be an INT.\n",reference_getname(script_getdata(st, 4)));
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	// Possible needly check function parameters on C_STR,C_INT,C_INT
	if (script_hasdata(st, 5))
		type = script_getnum(st, 5);

	switch (type) {
		case BL_PC:	//Get Character Position
			if ((script_isstring(st, 6) && script_nick2sd(6, sd)) || script_mapid2sd(6, sd))
				bl = &sd->bl;
			break;
		case BL_NPC:	//Get NPC Position
			if (script_hasdata(st, 6)) {
				struct npc_data *nd;

				if (script_isstring(st, 6))
					nd = npc_name2id(script_getstr(st, 6));
				else
					nd = map_id2nd(script_getnum(st, 6));
				if (nd)
					bl = &nd->bl;
			} else //In case the origin is not an NPC?
				bl = map_id2bl(st->oid);
			break;
		case BL_PET:	//Get Pet Position
			if (sd->pd && ((script_isstring(st, 6) && script_nick2sd(6, sd)) || script_mapid2sd(6, sd)))
				bl = &sd->pd->bl;
			break;
		case BL_HOM:	//Get Homun Position
			if (sd->hd && ((script_isstring(st, 6) && script_nick2sd(6, sd)) || script_mapid2sd(6, sd)))
				bl = &sd->hd->bl;
			break;
		case BL_MER: //Get Mercenary Position
			if (sd->md && ((script_isstring(st, 6) && script_nick2sd(6, sd)) || script_mapid2sd(6, sd)))
				bl = &sd->md->bl;
			break;
		case BL_ELEM: //Get Elemental Position
			if (sd->ed && ((script_isstring(st, 6) && script_nick2sd(6, sd)) || script_mapid2sd(6, sd)))
				bl = &sd->ed->bl;
			break;
		default:
			ShowWarning("script: buildin_getmapxy: Invalid type %d.\n", type);
			script_pushint(st,-1);
			return SCRIPT_CMD_FAILURE;
	}
	if (!bl) { //No object found.
		script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}

	x= bl->x;
	y= bl->y;
	if (bl->m >= 0)
		safestrncpy(mapname, map_getmapdata(bl->m)->name, MAP_NAME_LENGTH);
	else
		memset(mapname, '\0', sizeof(mapname));

	//Set MapName$
	num=st->stack->stack_data[st->start+2].u.num;
	name=get_str(script_getvarid(num));
	prefix=*name;

	if(not_server_variable(prefix)){
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_getmapxy: variable '%s' for mapname is not a server variable, but no player is attached!", name );
			return SCRIPT_CMD_FAILURE;
		}
	}else
		sd=NULL;
	set_reg_str( st, sd, num, name, mapname, script_getref( st, 2 ) );

	//Set MapX
	num=st->stack->stack_data[st->start+3].u.num;
	name=get_str(script_getvarid(num));
	prefix=*name;

	if(not_server_variable(prefix)){
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_getmapxy: variable '%s' for mapX is not a server variable, but no player is attached!", name );
			return SCRIPT_CMD_FAILURE;
		}
	}else
		sd=NULL;
	set_reg_num( st, sd, num, name, x, script_getref( st, 3 ) );

	//Set MapY
	num=st->stack->stack_data[st->start+4].u.num;
	name=get_str(script_getvarid(num));
	prefix=*name;

	if(not_server_variable(prefix)){
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_getmapxy: variable '%s' for mapY is not a server variable, but no player is attached!", name );
			return SCRIPT_CMD_FAILURE;
		}
	}else
		sd=NULL;
	set_reg_num( st, sd, num, name, y, script_getref( st, 4 ) );

	//Return Success value
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the map name of given map ID.
///
/// mapid2name <map ID>;
BUILDIN_FUNC(mapid2name)
{
	uint16 m = script_getnum(st, 2);

	if (m >= MAX_MAP_PER_SERVER) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	script_pushstrcopy(st, map_mapid2mapname(m));

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Allows player to write NPC logs (i.e. Bank NPC, etc) [Lupus]
 *------------------------------------------*/
BUILDIN_FUNC(logmes)
{
	const char *str;
	struct map_session_data* sd = map_id2sd(st->rid);

	str = script_getstr(st,2);
	if( sd ){
		log_npc(sd,str);
	}else{
		struct npc_data* nd = map_id2nd(st->oid);

		if( !nd ){
			ShowError( "buildin_logmes: Invalid usage without player or npc.\n" );
			return SCRIPT_CMD_FAILURE;
		}

		log_npc(nd,str);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(summon)
{
	int _class, timeout=0;
	const char *str,*event="";
	TBL_PC *sd;
	struct mob_data *md;
	t_tick tick = gettick();

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;

	str	=script_getstr(st,2);
	_class=script_getnum(st,3);
	if( script_hasdata(st,4) )
		timeout=script_getnum(st,4);
	if( script_hasdata(st,5) ){
		event=script_getstr(st,5);
		check_event(st, event);
	}

#ifdef Pandas_Crashfix_ScriptCommand_Summon
	if (_class < 0 || !mobdb_checkid(_class)) {
		ShowWarning("buildin_summon: Attempted to summon non-existing monster class %d\n", _class);
		return SCRIPT_CMD_FAILURE;
	}
#endif // Pandas_Crashfix_ScriptCommand_Summon

	clif_skill_poseffect(&sd->bl,AM_CALLHOMUN,1,sd->bl.x,sd->bl.y,tick);

	md = mob_once_spawn_sub(&sd->bl, sd->bl.m, sd->bl.x, sd->bl.y, str, _class, event, SZ_SMALL, AI_NONE);
	if (md) {
		md->master_id=sd->bl.id;
		md->special_state.ai = AI_ATTACK;
		if( md->deletetimer != INVALID_TIMER )
			delete_timer(md->deletetimer, mob_timer_delete);
		md->deletetimer = add_timer(tick+(timeout>0?timeout:60000),mob_timer_delete,md->bl.id,0);
		mob_spawn (md); //Now it is ready for spawning.
		clif_specialeffect(&md->bl,EF_ENTRY2,AREA);
		sc_start4(NULL,&md->bl, SC_MODECHANGE, 100, 1, 0, MD_AGGRESSIVE, 0, 60000);
	}
	script_pushint(st, md->bl.id);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Checks whether it is daytime/nighttime
 *------------------------------------------*/
BUILDIN_FUNC(isnight)
{
	script_pushint(st,(night_flag == 1));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(isday)
{
	script_pushint(st,(night_flag == 0));
	return SCRIPT_CMD_SUCCESS;
}

/*================================================
 * Check how many items/cards in the list are
 * equipped - used for 2/15's cards patch [celest]
 *------------------------------------------------*/
BUILDIN_FUNC(isequippedcnt)
{
	TBL_PC *sd;

	if (!script_rid2sd(sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	int ret = 0;
	int total = script_lastdata(st);
	std::vector<int32> list(total);

	for (int i = 2; i <= total; ++i) {
		int id = script_getnum(st,i);
		if (id <= 0)
			continue;
		if (std::find(list.begin(), list.end(), id) != list.end())
			continue;
		list.push_back(id);

		for (short j = 0; j < EQI_MAX; j++) {
			short index = sd->equip_index[j];
			if (index < 0)
				continue;
			if (pc_is_same_equip_index((enum equip_index)j, sd->equip_index, index))
				continue;

			if (!sd->inventory_data[index])
				continue;

			if (itemdb_type(id) != IT_CARD) { //No card. Count amount in inventory.
				if (sd->inventory_data[index]->nameid == id)
					ret += sd->inventory.u.items_inventory[index].amount;
			} else { //Count cards.
				if (itemdb_isspecial(sd->inventory.u.items_inventory[index].card[0]))
					continue; //No cards
				for (short k = 0; k < MAX_SLOTS; k++) {
					if (sd->inventory.u.items_inventory[index].card[k] == id)
						ret++; //[Lupus]
				}
			}
		}
	}

	script_pushint(st,ret);
	return SCRIPT_CMD_SUCCESS;
}

/*================================================
 * Check whether another card has been
 * equipped - used for 2/15's cards patch [celest]
 * -- Items checked cannot be reused in another
 * card set to prevent exploits
 *------------------------------------------------*/
BUILDIN_FUNC(isequipped)
{
	TBL_PC *sd;
	int i, id = 1;
	int ret = -1;
	//Original hash to reverse it when full check fails.
	unsigned int setitem_hash = 0, setitem_hash2 = 0;

	if (!script_rid2sd(sd)) { //If the player is not attached it is a script error anyway... but better prevent the map server from crashing...
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	setitem_hash = sd->bonus.setitem_hash;
	setitem_hash2 = sd->bonus.setitem_hash2;
	for (i=0; id!=0; i++) {
		int flag = 0;
		short j;
		FETCH (i+2, id) else id = 0;
		if (id <= 0)
			continue;
		for (j=0; j<EQI_MAX; j++) {
			short index = sd->equip_index[j];
			if(index < 0)
				continue;
			if (pc_is_same_equip_index((enum equip_index)i, sd->equip_index, index))
				continue;

			if(!sd->inventory_data[index])
				continue;

			if (itemdb_type(id) != IT_CARD) {
				if (sd->inventory_data[index]->nameid != id)
					continue;
				flag = 1;
				break;
			} else { //Cards
				short k;
				if (itemdb_isspecial(sd->inventory.u.items_inventory[index].card[0]))
					continue;

				for (k = 0; k < MAX_SLOTS; k++)
				{	//New hash system which should support up to 4 slots on any equipment. [Skotlex]
					unsigned int hash = 0;
					if (sd->inventory.u.items_inventory[index].card[k] != id)
						continue;

					hash = 1<<((j<5?j:j-5)*4 + k);
					// check if card is already used by another set
					if ( ( j < 5 ? sd->bonus.setitem_hash : sd->bonus.setitem_hash2 ) & hash)
						continue;

					// We have found a match
					flag = 1;
					// Set hash so this card cannot be used by another
					if (j<5)
						sd->bonus.setitem_hash |= hash;
					else
						sd->bonus.setitem_hash2 |= hash;
					break;
				}
			}
			if (flag) break; //Card found
		}
		if (ret == -1)
			ret = flag;
		else
			ret &= flag;
		if (!ret) break;
	}
	if (!ret) {//When check fails, restore original hash values. [Skotlex]
		sd->bonus.setitem_hash = setitem_hash;
		sd->bonus.setitem_hash2 = setitem_hash2;
	}
	script_pushint(st,ret);
	return SCRIPT_CMD_SUCCESS;
}

/*================================================
 * Check how many given inserted cards in the CURRENT
 * weapon - used for 2/15's cards patch [Lupus]
 *------------------------------------------------*/
BUILDIN_FUNC(cardscnt)
{
	TBL_PC *sd;
	int i, k, id = 1;
	int ret = 0;
	int index;

	if( !script_rid2sd(sd) ){
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	for (i=0; id!=0; i++) {
		FETCH (i+2, id) else id = 0;
		if (id <= 0)
			continue;

		index = current_equip_item_index; //we get CURRENT WEAPON inventory index from status.cpp [Lupus]
		if(index < 0) continue;

		if(!sd->inventory_data[index])
			continue;

		if(itemdb_type(id) != IT_CARD) {
			if (sd->inventory_data[index]->nameid == id)
				ret+= sd->inventory.u.items_inventory[index].amount;
		} else {
			if (itemdb_isspecial(sd->inventory.u.items_inventory[index].card[0]))
				continue;
			for (k=0; k < MAX_SLOTS; k++) {
				if (sd->inventory.u.items_inventory[index].card[k] == id)
					ret++;
			}
		}
	}
	script_pushint(st,ret);
//	script_pushint(st,current_equip_item_index);
	return SCRIPT_CMD_SUCCESS;
}

/*=======================================================
 * Returns the refined number of the current item, or an
 * item with inventory index specified
 *-------------------------------------------------------*/
BUILDIN_FUNC(getrefine)
{
	TBL_PC *sd;
	if (script_rid2sd(sd)){
		if( current_equip_item_index == -1 ){
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}

		script_pushint(st,sd->inventory.u.items_inventory[current_equip_item_index].refine);
	}else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/*=======================================================
 * Day/Night controls
 *-------------------------------------------------------*/
BUILDIN_FUNC(night)
{
	if (night_flag != 1) map_night_timer(night_timer_tid, 0, 0, 1);
	return SCRIPT_CMD_SUCCESS;
}
BUILDIN_FUNC(day)
{
	if (night_flag != 0) map_day_timer(day_timer_tid, 0, 0, 1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * unequip <equipment slot>{,<char_id>};
 * @author [Spectre]
 **/
BUILDIN_FUNC(unequip) {
	int pos;
	TBL_PC *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	pos = script_getnum(st,2);
	if (equip_index_check(pos)) {
		short i = pc_checkequip(sd,equip_bitmask[pos]);
		if (i >= 0) {
			pc_unequipitem(sd,i,1|2);
			script_pushint(st, 1);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	ShowError("buildin_unequip: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st, 0);
	return SCRIPT_CMD_FAILURE;
}

/**
 * equip <item id>{,<char_id>};
 **/
BUILDIN_FUNC(equip) {
	TBL_PC *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	t_itemid nameid = script_getnum(st,2);
	std::shared_ptr<item_data> id = item_db.find(nameid);

	if (id != nullptr) {
		int i;

		ARR_FIND( 0, MAX_INVENTORY, i, sd->inventory.u.items_inventory[i].nameid == nameid );
		if (i < MAX_INVENTORY) {
			pc_equipitem(sd,i,id->equip);
			script_pushint(st,1);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	ShowError("buildin_equip: Item %u cannot be equipped\n",nameid);
	script_pushint(st,0);
	return SCRIPT_CMD_FAILURE;
}

BUILDIN_FUNC(autoequip)
{
	t_itemid nameid=script_getnum(st,2);
	int flag=script_getnum(st,3);
	std::shared_ptr<item_data> id = item_db.find(nameid);

	if( id == nullptr )
	{
		ShowError("buildin_autoequip: Invalid item '%u'.\n", nameid);
		return SCRIPT_CMD_FAILURE;
	}

	if( !itemdb_isequip2(id.get()) )
	{
		ShowError("buildin_autoequip: Item '%u' cannot be equipped.\n", nameid);
		return SCRIPT_CMD_FAILURE;
	}

	id->flag.autoequip = flag>0?1:0;
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set the value of a given battle config
 * setbattleflag "<battle flag>",<value>{,<reload>};
 */
BUILDIN_FUNC(setbattleflag)
{
	const char *flag, *value;

	flag = script_getstr(st,2);
	value = script_getstr(st,3);  // HACK: Retrieve number as string (auto-converted) for battle_set_value

	if (battle_set_value(flag, value) == 0)
		ShowWarning("buildin_setbattleflag: unknown battle_config flag '%s'\n",flag);
	else {
		ShowInfo("buildin_setbattleflag: battle_config flag '%s' is now set to '%s'.\n",flag,value);

		if (script_hasdata(st, 4) && script_getnum(st, 4)) { // Only attempt to reload monster data if told to
			const char *expdrop_flags[] = { // Only certain flags require a reload, check for those types
				"base_exp_rate", "job_exp_rate", "mvp_exp_rate", "quest_exp_rate", "heal_exp", "resurrection_exp",
				"item_rate_common", "item_rate_common_boss", "item_rate_common_mvp", "item_drop_common_min", "item_drop_common_max",
				"item_rate_heal", "item_rate_heal_boss", "item_rate_heal_mvp", "item_rate_heal_min", "item_rate_heal_max",
				"item_rate_use", "item_rate_use_boss", "item_rate_use_mvp", "item_rate_use_min", "item_rate_use_max",
				"item_rate_equip", "item_rate_equip_boss", "item_rate_equip_mvp", "item_rate_equip_min", "item_rate_equip_max",
				"item_rate_card", "item_rate_card_boss", "item_rate_card_mvp", "item_rate_card_min", "item_rate_card_max",
				"item_rate_mvp", "item_rate_mvp_min", "item_rate_mvp_max", "item_rate_mvp_mode",
				"item_rate_treasure", "item_rate_treasure_min", "item_rate_treasure_max",
				"item_logarithmic_drops", "drop_rate0item", "drop_rateincrease",
			};
			uint8 i;

			for (i = 0; i < ARRAYLENGTH(expdrop_flags); i++) {
				if (!strcmpi(flag, expdrop_flags[i])) {
					mob_reload();
					break;
				}
			}
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get the value of a given battle config
 * getbattleflag("<battle flag>")
 */
BUILDIN_FUNC(getbattleflag)
{
	const char *flag = script_getstr(st,2);

	script_pushint(st,battle_get_value(flag));
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// strlen [Valaris]
//-------------------------------------------------------
BUILDIN_FUNC(getstrlen)
{

	const char *str = script_getstr(st,2);
	int len = (str) ? (int)strlen(str) : 0;

	script_pushint(st,len);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// isalpha [Valaris]
//-------------------------------------------------------
BUILDIN_FUNC(charisalpha)
{
	const char *str=script_getstr(st,2);
	int pos=script_getnum(st,3);

	int val = ( str && pos >= 0 && (unsigned int)pos < strlen(str) ) ? ISALPHA( str[pos] ) != 0 : 0;

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// charisupper <str>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(charisupper)
{
	const char *str = script_getstr(st,2);
	int pos = script_getnum(st,3);

	int val = ( str && pos >= 0 && (unsigned int)pos < strlen(str) ) ? ISUPPER( str[pos] ) : 0;

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// charislower <str>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(charislower)
{
	const char *str = script_getstr(st,2);
	int pos = script_getnum(st,3);

	int val = ( str && pos >= 0 && (unsigned int)pos < strlen(str) ) ? ISLOWER( str[pos] ) : 0;

	script_pushint(st,val);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// charat <str>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(charat) {
	const char *str = script_getstr(st,2);
	int pos = script_getnum(st,3);

	if( pos >= 0 && (unsigned int)pos < strlen(str) ) {
		char output[2];
		output[0] = str[pos];
		output[1] = '\0';
		script_pushstrcopy(st, output);
	} else
		script_pushconststr(st, "");
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// setchar <string>, <char>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(setchar)
{
	const char *str = script_getstr(st,2);
	const char *c = script_getstr(st,3);
	int index = script_getnum(st,4);
	char *output = aStrdup(str);

	if(index >= 0 && index < strlen(output))
		output[index] = *c;

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// insertchar <string>, <char>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(insertchar)
{
	const char *str = script_getstr(st,2);
	const char *c = script_getstr(st,3);
	int index = script_getnum(st,4);
	char *output;
	size_t len = strlen(str);

	if(index < 0)
		index = 0;
	else if(index > len)
		index = len;

	output = (char*)aMalloc(len + 2);

	memcpy(output, str, index);
	output[index] = c[0];
	memcpy(&output[index+1], &str[index], len - index);
	output[len+1] = '\0';

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// delchar <string>, <index>
//-------------------------------------------------------
BUILDIN_FUNC(delchar)
{
	const char *str = script_getstr(st,2);
	int index = script_getnum(st,3);
	char *output;
	size_t len = strlen(str);

#ifndef Pandas_Crashfix_ScriptCommand_Delchar
	if(index < 0 || index > len) {
#else
	if(index < 0 || index > len || !len) {
#endif // Pandas_Crashfix_ScriptCommand_Delchar
		//return original
		output = aStrdup(str);
		script_pushstr(st, output);
		return SCRIPT_CMD_SUCCESS;
	}

	output = (char*)aMalloc(len);

	memcpy(output, str, index);
	memcpy(&output[index], &str[index+1], len - index);

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// strtoupper <str>
//-------------------------------------------------------
BUILDIN_FUNC(strtoupper)
{
	const char *str = script_getstr(st,2);
	char *output = aStrdup(str);
	char *cursor = output;

	while (*cursor != '\0') {
		*cursor = TOUPPER(*cursor);
		cursor++;
	}

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// strtolower <str>
//-------------------------------------------------------
BUILDIN_FUNC(strtolower)
{
	const char *str = script_getstr(st,2);
	char *output = aStrdup(str);
	char *cursor = output;

	while (*cursor != '\0') {
		*cursor = TOLOWER(*cursor);
		cursor++;
	}

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// substr <str>, <start>, <end>
//-------------------------------------------------------
BUILDIN_FUNC(substr)
{
	const char *str = script_getstr(st,2);
	char *output;
	int start = script_getnum(st,3);
	int end = script_getnum(st,4);
	int len = 0;

	if(start >= 0 && end < strlen(str) && start <= end) {
		len = end - start + 1;
		output = (char*)aMalloc(len + 1);
		memcpy(output, &str[start], len);
	} else
		output = (char*)aMalloc(1);

	output[len] = '\0';

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// explode <dest_string_array>, <str>, <delimiter>
// Note: delimiter is limited to 1 char
//-------------------------------------------------------
BUILDIN_FUNC(explode)
{
	struct script_data* data = script_getdata(st, 2);
	const char *str = script_getstr(st,3);
	const char delimiter = script_getstr(st, 4)[0];
	int32 id;
	size_t len = strlen(str);
	int i = 0, j = 0;
	int start;
	char *temp;
	const char* name;
	TBL_PC* sd = NULL;

	temp = (char*)aMalloc(len + 1);

	if( !data_isreference(data) ) {
		ShowError("script:explode: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if( !is_string_variable(name) ) {
		ShowError("script:explode: not string array\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// data type mismatch
	}

	if( not_server_variable(*name) ) {
		if( !script_rid2sd(sd) )
			return SCRIPT_CMD_SUCCESS;// no player attached
	}

	while(str[i] != '\0') {
		if(str[i] == delimiter && start < SCRIPT_MAX_ARRAYSIZE-1) { //break at delimiter but ignore after reaching last array index
			temp[j] = '\0';
			set_reg_str( st, sd, reference_uid( id, start++ ), name, temp, reference_getref( data ) );
			j = 0;
			++i;
		} else {
			temp[j++] = str[i++];
		}
	}

	//set last string
	temp[j] = '\0';
	set_reg_str( st, sd, reference_uid( id, start ), name, temp, reference_getref( data ) );

	aFree(temp);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// implode <string_array>
// implode <string_array>, <glue>
//-------------------------------------------------------
BUILDIN_FUNC(implode)
{
	struct script_data* data = script_getdata(st, 2);
	const char *name;
	uint32 glue_len = 0, array_size, id;
	char *output;
	TBL_PC* sd = NULL;

	if( !data_isreference(data) ) {
		ShowError("script:implode: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	name = reference_getname(data);

	if( !is_string_variable(name) ) {
		ShowError("script:implode: not string array\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// data type mismatch
	}

	if( not_server_variable(*name) && !script_rid2sd(sd) ) {
		return SCRIPT_CMD_SUCCESS;// no player attached
	}

	//count chars
	array_size = script_array_highest_key(st, sd, name, reference_getref(data)) - 1;

	if(array_size == -1) { //empty array check (AmsTaff)
		ShowWarning("script:implode: array length = 0\n");
		output = (char*)aMalloc(sizeof(char)*5);
		sprintf(output,"%s","NULL");
	} else {
		const char *glue = NULL, *temp;
		size_t len = 0;
		int i, k = 0;

		for(i = 0; i <= array_size; ++i) {
			temp = get_val2_str( st, reference_uid( id, i ), reference_getref( data ) );
			len += strlen(temp);
			// Remove stack entry from get_val2_str
			script_removetop( st, -1, 0 );
		}

		//allocate mem
		if( script_hasdata(st,3) ) {
			glue = script_getstr(st,3);
			glue_len = strlen(glue);
#ifndef Pandas_LGTM_Optimization
			len += glue_len * (array_size);
#else
			// 乘法计算时使用较大的数值类型来避免计算结果溢出: https://lgtm.com/rules/2157860313/
			len += (size_t)glue_len * (array_size);
#endif // Pandas_LGTM_Optimization
		}
		output = (char*)aMalloc(len + 1);

		//build output
		for(i = 0; i < array_size; ++i) {
			temp = get_val2_str( st, reference_uid( id, i ), reference_getref( data ) );
			len = strlen(temp);
			memcpy(&output[k], temp, len);
			k += len;
			// Remove stack entry from get_val2_str
			script_removetop( st, -1, 0 );

			if(glue_len != 0) {
				memcpy(&output[k], glue, glue_len);
				k += glue_len;
			}
		}

		temp = get_val2_str( st, reference_uid( id, array_size ), reference_getref( data ) );
		len = strlen(temp);
		memcpy(&output[k], temp, len);
		k += len;
		output[k] = '\0';
		// Remove stack entry from get_val2_str
		script_removetop( st, -1, 0 );
	}

	script_pushstr(st, output);
	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// sprintf(<format>, ...);
// Implements C sprintf, except format %n. The resulting string is
// returned, instead of being saved in variable by reference.
//-------------------------------------------------------
BUILDIN_FUNC(sprintf)
{
	unsigned int len, argc = 0, arg = 0, buf2_len = 0;
	const char* format;
	char* p;
	char* q;
	char* buf  = NULL;
	char* buf2 = NULL;
	struct script_data* data;
	StringBuf final_buf;

	// Fetch init data
	format = script_getstr(st, 2);
	argc = script_lastdata(st)-2;
	len = strlen(format);

	// Skip parsing, where no parsing is required.
	if(len == 0) {
		script_pushconststr(st,"");
		return SCRIPT_CMD_SUCCESS;
	}

	// Pessimistic alloc
	CREATE(buf, char, len+1);

	// Need not be parsed, just solve stuff like %%.
	if(argc == 0) {
		memcpy(buf,format,len+1);
		script_pushstrcopy(st, buf);
		aFree(buf);
		return SCRIPT_CMD_SUCCESS;
	}

	safestrncpy(buf, format, len+1);

	// Issue sprintf for each parameter
	StringBuf_Init(&final_buf);
	q = buf;
	while((p = strchr(q, '%')) != NULL) {
		if(p != q) {
			len = p - q + 1;

			if(buf2_len < len) {
				RECREATE(buf2, char, len);
				buf2_len = len;
			}

			safestrncpy(buf2, q, len);
			StringBuf_AppendStr(&final_buf, buf2);
			q = p;
		}

		p = q + 1;

		if(*p == '%') {  // %%
			StringBuf_AppendStr(&final_buf, "%");
			q+=2;
			continue;
		}

		if(*p == 'n') {  // %n
			ShowWarning("buildin_sprintf: Format %%n not supported! Skipping...\n");
			script_reportsrc(st);
			q+=2;
			continue;
		}

		if(arg >= argc) {
			ShowError("buildin_sprintf: Not enough arguments passed!\n");
			if(buf) aFree(buf);
			if(buf2) aFree(buf2);
			StringBuf_Destroy(&final_buf);
			script_pushconststr(st,"");
			return SCRIPT_CMD_FAILURE;
		}

		if((p = strchr(q+1, '%')) == NULL)
			p = strchr(q, 0);  // EOS

		len = p - q + 1;

		if(buf2_len < len) {
			RECREATE(buf2, char, len);
			buf2_len = len;
		}

		safestrncpy(buf2, q, len);
		q = p;

		// Note: This assumes the passed value being the correct
		// type to the current format specifier. If not, the server
		// probably crashes or returns anything else, than expected,
		// but it would behave in normal code the same way so it's
		// the scripter's responsibility.
		data = script_getdata(st, arg+3);

		if(data_isstring(data))  // String
			StringBuf_Printf(&final_buf, buf2, script_getstr(st, arg+3));
		else if(data_isint(data))  // Number
#ifndef Pandas_Fix_Sprintf_ScriptCommand_Unsupport_Int64
			StringBuf_Printf(&final_buf, buf2, script_getnum(st, arg+3));
#else
			StringBuf_Printf(&final_buf, buf2, script_getnum64(st, arg+3));
#endif // Pandas_Fix_Sprintf_ScriptCommand_Unsupport_Int64
		else if(data_isreference(data)) {  // Variable
			char* name = reference_getname(data);

			if(name[strlen(name)-1]=='$')  // var Str
				StringBuf_Printf(&final_buf, buf2, script_getstr(st, arg+3));
			else  // var Int
#ifndef Pandas_Fix_Sprintf_ScriptCommand_Unsupport_Int64
				StringBuf_Printf(&final_buf, buf2, script_getnum(st, arg+3));
#else
				StringBuf_Printf(&final_buf, buf2, script_getnum64(st, arg+3));
#endif // Pandas_Fix_Sprintf_ScriptCommand_Unsupport_Int64
		} else {  // Unsupported type
			ShowError("buildin_sprintf: Unknown argument type!\n");
			if(buf) aFree(buf);
			if(buf2) aFree(buf2);
			StringBuf_Destroy(&final_buf);
			script_pushconststr(st,"");
			return SCRIPT_CMD_FAILURE;
		}

		arg++;
	}

	// Append anything left
	if(*q)
		StringBuf_AppendStr(&final_buf, q);

	// Passed more, than needed
	if(arg < argc) {
		ShowWarning("buildin_sprintf: Unused arguments passed.\n");
		script_reportsrc(st);
	}

	script_pushstrcopy(st, StringBuf_Value(&final_buf));

	if(buf) aFree(buf);
	if(buf2) aFree(buf2);
	StringBuf_Destroy(&final_buf);

	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// sscanf(<str>, <format>, ...);
// Implements C sscanf.
//-------------------------------------------------------
BUILDIN_FUNC(sscanf){
	unsigned int argc, arg = 0, len;
	struct script_data* data;
	struct map_session_data* sd = NULL;
	const char* str;
	const char* format;
	const char* p;
	const char* q;
	char* buf = NULL;
	char* buf_p;
	char* ref_str = NULL;
	int ref_int;

	// Get data
	str = script_getstr(st, 2);
	format = script_getstr(st, 3);
	argc = script_lastdata(st)-3;

	len = strlen(format);


	if (len != 0 && strlen(str) == 0) {
		// If the source string is empty but the format string is not, we return -1
		// according to the C specs. (if the format string is also empty, we shall
		// continue and return 0: 0 conversions took place out of the 0 attempted.)
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	CREATE(buf, char, len*2+1);

	// Issue sscanf for each parameter
	*buf = 0;
	q = format;
	while((p = strchr(q, '%'))){
		if(p!=q){
			strncat(buf, q, (size_t)(p-q));
			q = p;
		}
		p = q+1;
		if(*p=='*' || *p=='%'){  // Skip
			strncat(buf, q, 2);
			q+=2;
			continue;
		}
		if(arg>=argc){
			ShowError("buildin_sscanf: Not enough arguments passed!\n");
			script_pushint(st, -1);
			if(buf) aFree(buf);
			if(ref_str) aFree(ref_str);
			return SCRIPT_CMD_FAILURE;
		}
		if((p = strchr(q+1, '%'))==NULL){
			p = strchr(q, 0);  // EOS
		}
		len = p-q;
		strncat(buf, q, len);
		q = p;

		// Validate output
		data = script_getdata(st, arg+4);
		if(!data_isreference(data) || !reference_tovariable(data)){
			ShowError("buildin_sscanf: Target argument is not a variable!\n");
			script_pushint(st, -1);
			if(buf) aFree(buf);
			if(ref_str) aFree(ref_str);
			return SCRIPT_CMD_FAILURE;
		}
		buf_p = reference_getname(data);
		if(not_server_variable(*buf_p) && !script_rid2sd(sd)){
			script_pushint(st, -1);
			if(buf) aFree(buf);
			if(ref_str) aFree(ref_str);
			return SCRIPT_CMD_SUCCESS;
		}

		// Save value if any
		if(buf_p[strlen(buf_p)-1]=='$'){  // String
			if(ref_str==NULL){
				CREATE(ref_str, char, strlen(str)+1);
			}
			if(sscanf(str, buf, ref_str)==0){
				break;
			}
			set_reg_str( st, sd, reference_uid( reference_getid( data ), reference_getindex( data ) ), buf_p, ref_str, reference_getref( data ) );
		} else {  // Number
			if(sscanf(str, buf, &ref_int)==0){
				break;
			}
			set_reg_num( st, sd, reference_uid( reference_getid( data ), reference_getindex( data ) ), buf_p, ref_int, reference_getref( data ) );
		}
		arg++;

		// Disable used format (%... -> %*...)
		buf_p = strchr(buf, 0);
		memmove(buf_p-len+2, buf_p-len+1, len);
		*(buf_p-len+1) = '*';
	}

	script_pushint(st, arg);
	if(buf) aFree(buf);
	if(ref_str) aFree(ref_str);

	return SCRIPT_CMD_SUCCESS;
}

//=======================================================
// strpos(<haystack>, <needle>)
// strpos(<haystack>, <needle>, <offset>)
//
// Implements PHP style strpos. Adapted from code from
// http://www.daniweb.com/code/snippet313.html, Dave Sinkula
//-------------------------------------------------------
BUILDIN_FUNC(strpos) {
	const char *haystack = script_getstr(st,2);
	const char *needle = script_getstr(st,3);
	int i;
	size_t len;

	if( script_hasdata(st,4) )
		i = script_getnum(st,4);
	else
		i = 0;

	if (needle[0] == '\0') {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	len = strlen(haystack);
	for ( ; i < len; ++i ) {
		if ( haystack[i] == *needle ) {
			// matched starting char -- loop through remaining chars
			const char *h, *n;
			for ( h = &haystack[i], n = needle; *h && *n; ++h, ++n ) {
				if ( *h != *n ) {
					break;
				}
			}
			if ( !*n ) { // matched all of 'needle' to null termination
				script_pushint(st, i);
				return SCRIPT_CMD_SUCCESS;
			}
		}
	}
	script_pushint(st, -1);
	return SCRIPT_CMD_SUCCESS;
}

//===============================================================
// replacestr <input>, <search>, <replace>{, <usecase>{, <count>}}
//
// Note: Finds all instances of <search> in <input> and replaces
// with <replace>. If specified will only replace as many
// instances as specified in <count>. By default will be case
// sensitive.
//---------------------------------------------------------------
BUILDIN_FUNC(replacestr)
{
	const char *input = script_getstr(st, 2);
	const char *find = script_getstr(st, 3);
	const char *replace = script_getstr(st, 4);
	size_t inputlen = strlen(input);
	size_t findlen = strlen(find);
	struct StringBuf output;
	bool usecase = true;

	int count = 0;
	int numFinds = 0;
	int i = 0, f = 0;

	if(findlen == 0) {
		ShowError("script:replacestr: Invalid search length.\n");
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if(script_hasdata(st, 5)) {
		if( script_isint(st, 5) )
			usecase = script_getnum(st, 5) != 0;
		else {
			ShowError("script:replacestr: Invalid usecase value. Expected int got string\n");
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}

	if(script_hasdata(st, 6)) {
		count = script_getnum(st, 6);
		if(count == 0) {
			ShowError("script:replacestr: Invalid count value. Expected int got string\n");
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}

	StringBuf_Init(&output);

	for(; i < inputlen; i++) {
		if(count && count == numFinds) {	//found enough, stop looking
			break;
		}

		for(f = 0; f <= findlen; f++) {
			if(f == findlen) { //complete match
				numFinds++;
				StringBuf_AppendStr(&output, replace);

				i += findlen - 1;
				break;
			} else {
				if(usecase) {
					if((i + f) > inputlen || input[i + f] != find[f]) {
						StringBuf_Printf(&output, "%c", input[i]);
						break;
					}
				} else {
					if(((i + f) > inputlen || input[i + f] != find[f]) && TOUPPER(input[i+f]) != TOUPPER(find[f])) {
						StringBuf_Printf(&output, "%c", input[i]);
						break;
					}
				}
			}
		}
	}

	//append excess after enough found
	if(i < inputlen)
		StringBuf_AppendStr(&output, &(input[i]));

	script_pushstrcopy(st, StringBuf_Value(&output));
	StringBuf_Destroy(&output);
	return SCRIPT_CMD_SUCCESS;
}

//========================================================
// countstr <input>, <search>{, <usecase>}
//
// Note: Counts the number of times <search> occurs in
// <input>. By default will be case sensitive.
//--------------------------------------------------------
BUILDIN_FUNC(countstr)
{
	const char *input = script_getstr(st, 2);
	const char *find = script_getstr(st, 3);
	size_t inputlen = strlen(input);
	size_t findlen = strlen(find);
	bool usecase = true;

	int numFinds = 0;
	int i = 0, f = 0;

	if(findlen == 0) {
		ShowError("script:countstr: Invalid search length.\n");
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if(script_hasdata(st, 4)) {
		if( script_isint(st, 4) )
			usecase = script_getnum(st, 4) != 0;
		else {
			ShowError("script:countstr: Invalid usecase value. Expected int got string\n");
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}

	for(; i < inputlen; i++) {
		for(f = 0; f <= findlen; f++) {
			if(f == findlen) { //complete match
				numFinds++;
				i += findlen - 1;
				break;
			} else {
				if(usecase) {
					if((i + f) > inputlen || input[i + f] != find[f]) {
						break;
					}
				} else {
					if(((i + f) > inputlen || input[i + f] != find[f]) && TOUPPER(input[i+f]) != TOUPPER(find[f])) {
						break;
					}
				}
			}
		}
	}
	script_pushint(st, numFinds);
	return SCRIPT_CMD_SUCCESS;
}


/// Changes the display name and/or display class of the npc.
/// Returns 0 is successful, 1 if the npc does not exist.
///
/// setnpcdisplay("<npc name>", "<new display name>", <new class id>, <new size>) -> <int>
/// setnpcdisplay("<npc name>", "<new display name>", <new class id>) -> <int>
/// setnpcdisplay("<npc name>", "<new display name>") -> <int>
/// setnpcdisplay("<npc name>", <new class id>) -> <int>
BUILDIN_FUNC(setnpcdisplay)
{
	const char* name;
	const char* newname = NULL;
	int class_ = JT_FAKENPC, size = -1;
	struct npc_data* nd;

	name = script_getstr(st,2);

	if( script_hasdata(st,4) )
		class_ = script_getnum(st,4);
	if( script_hasdata(st,5) )
		size = script_getnum(st,5);

	if( script_isstring(st, 3) )
 		newname = script_getstr(st, 3);
	else
 		class_ = script_getnum(st, 3);

	nd = npc_name2id(name);
	if( nd == NULL )
	{// not found
		script_pushint(st,1);
		return SCRIPT_CMD_SUCCESS;
	}

	// update npc
	if( newname )
		npc_setdisplayname(nd, newname);

	if( size != -1 && size != (int)nd->size )
		nd->size = size;
	else
		size = -1;

	if( class_ != JT_FAKENPC && nd->class_ != class_ )
		npc_setclass(nd, class_);
	else if( size != -1 )
		unit_refresh(&nd->bl); // Required to update the visual size

	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(atoi)
{
	const char *value;
	value = script_getstr(st,2);
	script_pushint(st,atoi(value));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(axtoi)
{
	const char *hex = script_getstr(st,2);
	long value = strtol(hex, NULL, 16);
#if LONG_MAX > INT_MAX || LONG_MIN < INT_MIN
	value = cap_value(value, INT_MIN, INT_MAX);
#endif
	script_pushint(st, (int)value);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(strtol)
{
	const char *string = script_getstr(st, 2);
	int base = script_getnum(st, 3);
	long value = strtol(string, NULL, base);
#if LONG_MAX > INT_MAX || LONG_MIN < INT_MIN
	value = cap_value(value, INT_MIN, INT_MAX);
#endif
	script_pushint(st, (int)value);
	return SCRIPT_CMD_SUCCESS;
}

// case-insensitive substring search [lordalfa]
BUILDIN_FUNC(compare)
{
	const char *message;
	const char *cmpstring;
	message = script_getstr(st,2);
	cmpstring = script_getstr(st,3);
	script_pushint(st,(stristr(message,cmpstring) != NULL));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(strcmp)
{
	const char *str1;
	const char *str2;
	str1 = script_getstr(st,2);
	str2 = script_getstr(st,3);
	script_pushint(st,strcmp(str1, str2));
	return SCRIPT_CMD_SUCCESS;
}

// [zBuffer] List of mathematics commands --->
BUILDIN_FUNC(sqrt)
{
	double i, a;
	i = script_getnum(st,2);
	a = sqrt(i);
	script_pushint(st,(int)a);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pow)
{
	double i, a, b;
	a = script_getnum(st,2);
	b = script_getnum(st,3);
	i = pow(a,b);
	script_pushint(st,(int)i);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(distance)
{
	int x0, y0, x1, y1;

	x0 = script_getnum(st,2);
	y0 = script_getnum(st,3);
	x1 = script_getnum(st,4);
	y1 = script_getnum(st,5);

	script_pushint(st,distance_xy(x0,y0,x1,y1));
	return SCRIPT_CMD_SUCCESS;
}

// <--- [zBuffer] List of mathematics commands

BUILDIN_FUNC(md5)
{
	const char *tmpstr;
	char *md5str;

	tmpstr = script_getstr(st,2);
	md5str = (char *)aMalloc((32+1)*sizeof(char));
	MD5_String(tmpstr, md5str);
	script_pushstr(st, md5str);
	return SCRIPT_CMD_SUCCESS;
}

// [zBuffer] List of dynamic var commands --->
/**
 * setd "<variable name>",<value>{,<char_id>};
 **/
BUILDIN_FUNC(setd)
{
	TBL_PC *sd = NULL;
	char varname[100];
	const char *buffer;
	int elem;
	buffer = script_getstr(st, 2);

	if(sscanf(buffer, "%99[^[][%11d]", varname, &elem) < 2)
		elem = 0;

#ifdef Pandas_Crashfix_ScriptCommand_Getd_And_Setd
	trim(varname);
#endif // Pandas_Crashfix_ScriptCommand_Getd_And_Setd

	if( not_server_variable(*varname) ) {
		if (!script_charid2sd(4,sd))
			return SCRIPT_CMD_FAILURE;
	}

	if( is_string_variable(varname) ) {
		setd_sub_str( st, sd, varname, elem, script_getstr( st, 3 ), NULL );
	} else {
		setd_sub_num( st, sd, varname, elem, script_getnum64( st, 3 ), NULL );
	}

	return SCRIPT_CMD_SUCCESS;
}

int buildin_query_sql_sub(struct script_state* st, Sql* handle)
{
	int i, j;
	TBL_PC* sd = NULL;
	const char* query;
	struct script_data* data;
	const char* name;
	unsigned int max_rows = SCRIPT_MAX_ARRAYSIZE; // maximum number of rows
	int num_vars;
	int num_cols;

	// check target variables
	for( i = 3; script_hasdata(st,i); ++i ) {
		data = script_getdata(st, i);
		if( data_isreference(data) ) { // it's a variable
			name = reference_getname(data);
			if( not_server_variable(*name) && sd == NULL ) { // requires a player
				if( !script_rid2sd(sd) ) { // no player attached
					script_reportdata(data);
					st->state = END;
					return SCRIPT_CMD_FAILURE;
				}
			}
		} else {
			ShowError("script:query_sql: not a variable\n");
			script_reportdata(data);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}
	}
	num_vars = i - 3;

	// Execute the query
	query = script_getstr(st,2);

	if( SQL_ERROR == Sql_QueryStr(handle, query) ) {
		Sql_ShowDebug(handle);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if( Sql_NumRows(handle) == 0 ) { // No data received
		Sql_FreeResult(handle);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	// Count the number of columns to store
	num_cols = Sql_NumColumns(handle);
	if( num_vars < num_cols ) {
		ShowWarning("script:query_sql: Too many columns, discarding last %u columns.\n", (unsigned int)(num_cols-num_vars));
		script_reportsrc(st);
	} else if( num_vars > num_cols ) {
		ShowWarning("script:query_sql: Too many variables (%u extra).\n", (unsigned int)(num_vars-num_cols));
		script_reportsrc(st);
	}

	// Store data
	for( i = 0; i < max_rows && SQL_SUCCESS == Sql_NextRow(handle); ++i ) {
		for( j = 0; j < num_vars; ++j ) {
			char* str = NULL;

			if( j < num_cols )
				Sql_GetData(handle, j, &str, NULL);

			data = script_getdata(st, j+3);
			name = reference_getname(data);

			if( is_string_variable(name) )
				setd_sub_str( st, sd, name, i, str ? str : "", reference_getref( data ) );
			else
				setd_sub_num( st, sd, name, i, str ? strtoll( str, nullptr, 10 ) : 0, reference_getref( data ) );
		}
	}
	if( i == max_rows && max_rows < Sql_NumRows(handle) ) {
		ShowWarning("script:query_sql: Only %d/%u rows have been stored.\n", max_rows, (unsigned int)Sql_NumRows(handle));
		script_reportsrc(st);
	}

	// Free data
	Sql_FreeResult(handle);
	script_pushint(st, i);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(query_sql) {
	return buildin_query_sql_sub(st, qsmysql_handle);
}

BUILDIN_FUNC(query_logsql) {
	if( !log_config.sql_logs ) {// logmysql_handle == NULL
		ShowWarning("buildin_query_logsql: SQL logs are disabled, query '%s' will not be executed.\n", script_getstr(st,2));
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	return buildin_query_sql_sub(st, logmysql_handle);
}

std::map<int, DBResultData*> query_sql_db;

static int buildin_query_sql_aysnc_sub(struct script_state* st, dbType type)
{
	if (!st->asyncSleep) {
		// 若当前 st 没有处于异步休眠状态
		// 那么说明这是一次全新执行的异步查询调用

		// 获取对应的查询语句
		const char* query = script_getstr(st, 2);

		int batch_number = script_batch;

		// 如果有传递用来接收返回值的变量, 那么执行异步查询并要求返回结果
		if (script_hasdata(st, 3)) {

			// 下次脚本被执行的时候将直接从当前脚本行继续往下执行
			st->state = RERUNLINE;

			// 设置当前的脚本进入异步休眠状态
			st->asyncSleep = true;

			// 设置查询任务, 并要求在查询结束之后执行匿名回调函数
			asyncquery_addDBJob(
				type,
				query,
				[st, batch_number](FutureData result_data) {
					if (batch_number != script_batch) {
						return; // st is probably a danging pointer by now, do nothing.
					}

					// 将查询的结果保存到 query_sql_db 字典中
					query_sql_db[st->id] = (DBResultData*)result_data;

					// 恢复脚本的执行
					run_script_main(st);
				}
			);
		}
		else
			// 如果没有要求需要保存返回值, 那么不设置匿名回调函数
			asyncquery_addDBJob(type, query);
	}
	else {
		// 若当前 st 处于异步休眠状态
		// 那么说明现在异步线程已经完成了数据查询工作并回调继续执行脚本

		// 从 query_sql_db 字典中读取当前 st 对应的查询结果
		DBResultData result_data(query_sql_db[st->id]);

		// 读取完成后直接移除字典中保存的内容
		delete query_sql_db[st->id];
		query_sql_db.erase(st->id);

		size_t i, j;
		TBL_PC* sd = NULL;
		struct script_data* data;
		const char* name;
		unsigned int max_rows = SCRIPT_MAX_ARRAYSIZE; // maximum number of rows
		size_t num_vars;
		size_t num_cols;

		// 取消当前脚本的异步休眠状态
		st->asyncSleep = false;

		// 设置脚本的状态为恢复正常执行
		st->state = RUN;

		// check target variables
		for (i = 3; script_hasdata(st, (int)i); ++i) {
			data = script_getdata(st, i);
			if (data_isreference(data)) { // it's a variable
				name = reference_getname(data);
				if (not_server_variable(*name) && sd == NULL) { // requires a player
					if (!script_rid2sd(sd)) { // no player attached
						script_reportdata(data);
						st->state = END;
						return SCRIPT_CMD_FAILURE;
					}
				}
			}
			else {
				ShowError("script:query_sql_async: not a variable\n");
				script_reportdata(data);
				st->state = END;
				return SCRIPT_CMD_FAILURE;
			}
		}
		num_vars = i - 3;

		if (SQL_ERROR == result_data.sql_result_value) {
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}

		if (result_data.RowNum == 0) { // No data received
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}

		// Count the number of columns to store
		num_cols = result_data.ColumnNum;
		if (num_vars < num_cols) {
			ShowWarning("script:query_sql_async: Too many columns, discarding last %u columns.\n", (unsigned int)(num_cols - num_vars));
			script_reportsrc(st);
		}
		else if (num_vars > num_cols) {
			ShowWarning("script:query_sql_async: Too many variables (%u extra).\n", (unsigned int)(num_vars - num_cols));
			script_reportsrc(st);
		}

		// Store data
		for (i = 0; i < max_rows && i < result_data.RowNum; ++i) {
			for (j = 0; j < num_vars; ++j) {
				const char* str = NULL;

				if (j < num_cols)
					str = result_data.GetData(i, j);

				data = script_getdata(st, j + 3);
				name = reference_getname(data);

				if (is_string_variable(name))
					setd_sub_str(st, sd, name, i, str ? str : "", reference_getref(data));
				else
					setd_sub_num(st, sd, name, i, str ? strtoll(str, nullptr, 10) : 0, reference_getref(data));
			}
		}
		if (i == max_rows && max_rows < result_data.RowNum) {
			ShowWarning("script:query_sql_async: Only %d/%u rows have been stored.\n", max_rows, (unsigned int)result_data.RowNum);
			script_reportsrc(st);
		}

		script_pushint(st, i);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(query_sql_async) {
	return buildin_query_sql_aysnc_sub(st, dbType::MAIN_DB);
}

BUILDIN_FUNC(query_logsql_async) {
	if( !log_config.sql_logs ) {// logmysql_handle == NULL
		ShowWarning("buildin_query_logsql_async: SQL logs are disabled, query '%s' will not be executed.\n", script_getstr(st,2));
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	return buildin_query_sql_aysnc_sub(st, dbType::LOG_DB);
}

//Allows escaping of a given string.
BUILDIN_FUNC(escape_sql)
{
	const char *str;
	char *esc_str;
	size_t len;

	str = script_getstr(st,2);
	len = strlen(str);
	esc_str = (char*)aMalloc(len*2+1);
	Sql_EscapeStringLen(mmysql_handle, esc_str, str, len);
	script_pushstr(st, esc_str);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getd)
{
	char varname[100];
	const char *buffer;
	int elem;

	buffer = script_getstr(st, 2);

	if(sscanf(buffer, "%99[^[][%11d]", varname, &elem) < 2)
		elem = 0;

#ifdef Pandas_Crashfix_ScriptCommand_Getd_And_Setd
	trim(varname);
#endif // Pandas_Crashfix_ScriptCommand_Getd_And_Setd

	// Push the 'pointer' so it's more flexible [Lance]
	push_val(st->stack, C_NAME, reference_uid(add_str(varname), elem));

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(callshop)
{
	TBL_PC *sd = NULL;
	struct npc_data *nd;
	const char *shopname;
	int flag = 0;
	if (!script_rid2sd(sd)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}
	shopname = script_getstr(st, 2);
	if (script_hasdata(st,3))
		flag = script_getnum(st,3);
	nd = npc_name2id(shopname);
	if( !nd || nd->bl.type != BL_NPC || (nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP && nd->subtype != NPCTYPE_BARTER) ) {
		ShowError("buildin_callshop: Shop [%s] not found (or NPC is not shop type)\n", shopname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (nd->subtype == NPCTYPE_SHOP) {
		// flag the user as using a valid script call for opening the shop (for floating NPCs)
		sd->state.callshop = 1;

		switch (flag) {
			case 1: npc_buysellsel(sd,nd->bl.id,0); break; //Buy window
			case 2: npc_buysellsel(sd,nd->bl.id,1); break; //Sell window
			default: clif_npcbuysell(sd,nd->bl.id); break; //Show menu
		}
	}
#if PACKETVER >= 20131223
	else if (nd->subtype == NPCTYPE_MARKETSHOP) {
		unsigned short i;

		for (i = 0; i < nd->u.shop.count; i++) {
			if (nd->u.shop.shop_item[i].qty)
				break;
		}

		if (i == nd->u.shop.count) {
			clif_messagecolor(&sd->bl, color_table[COLOR_RED], msg_txt(sd, 534), false, SELF);
			return SCRIPT_CMD_FAILURE;
		}

		sd->npc_shopid = nd->bl.id;
		clif_npc_market_open(sd, nd);
		script_pushint(st,1);
		return SCRIPT_CMD_SUCCESS;
	}
#endif
	else if( nd->subtype == NPCTYPE_BARTER ){
		// flag the user as using a valid script call for opening the shop (for floating NPCs)
		sd->state.callshop = 1;

		if( nd->u.barter.extended ){
			clif_barter_extended_open( *sd, *nd );
		}else{
			clif_barter_open( *sd, *nd );
		}
	}else
		clif_cashshop_show(sd, nd);

	sd->npc_shopid = nd->bl.id;
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(npcshopitem)
{
	const char* npcname = script_getstr(st, 2);
	struct npc_data* nd = npc_name2id(npcname);
	int n, i;
	int amount;
	uint16 offs = 2;

	if( !nd || ( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP ) ) { // Not found.
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

#if PACKETVER >= 20131223
	if (nd->subtype == NPCTYPE_MARKETSHOP) {
		offs = 3;
		npc_market_delfromsql_(nd->exname, 0, true);
	}
#endif

	// get the count of new entries
	amount = (script_lastdata(st)-2)/offs;

	// generate new shop item list
	RECREATE(nd->u.shop.shop_item, struct npc_item_list, amount);
	nd->u.shop.count = 0;
	for (n = 0, i = 3; n < amount; n++, i+=offs) {
		t_itemid nameid = script_getnum( st, i );

		if( itemdb_exists( nameid ) == nullptr ){
			ShowError( "builtin_npcshopitem: Item ID %u does not exist.\n", nameid );
			script_pushint( st, 0 );
			return SCRIPT_CMD_FAILURE;
		}

		nd->u.shop.shop_item[n].nameid = nameid;
		nd->u.shop.shop_item[n].value = script_getnum(st,i+1);
#if PACKETVER >= 20131223
		if (nd->subtype == NPCTYPE_MARKETSHOP) {
			nd->u.shop.shop_item[n].qty = script_getnum(st,i+2);
			nd->u.shop.shop_item[n].flag = 1;
			npc_market_tosql(nd->exname, &nd->u.shop.shop_item[n]);
		}
#endif
		nd->u.shop.count++;
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(npcshopadditem)
{
	const char* npcname = script_getstr(st,2);
	struct npc_data* nd = npc_name2id(npcname);
	uint16 offs = 2, amount;

	if (!nd || ( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP)) { // Not found.
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (nd->subtype == NPCTYPE_MARKETSHOP)
		offs = 3;

	// get the count of new entries
	amount = (script_lastdata(st)-2)/offs;

#if PACKETVER >= 20131223
	if (nd->subtype == NPCTYPE_MARKETSHOP) {
		for (int n = 0, i = 3; n < amount; n++, i += offs) {
			t_itemid nameid = script_getnum(st,i);
			uint16 j;

			if( itemdb_exists( nameid ) == nullptr ){
				ShowError( "builtin_npcshopadditem: Item ID %u does not exist.\n", nameid );
				script_pushint( st, 0 );
				return SCRIPT_CMD_FAILURE;
			}

			// Check existing entries
			ARR_FIND(0, nd->u.shop.count, j, nd->u.shop.shop_item[j].nameid == nameid);
			if (j == nd->u.shop.count) {
				RECREATE(nd->u.shop.shop_item, struct npc_item_list, nd->u.shop.count+1);
				j = nd->u.shop.count;
				nd->u.shop.shop_item[j].flag = 1;
				nd->u.shop.count++;
			}

			int32 stock = script_getnum( st, i + 2 );

			if( stock < -1 ){
				ShowError( "builtin_npcshopadditem: Invalid stock amount in marketshop '%s'.\n", nd->exname );
				script_pushint( st, 0 );
				return SCRIPT_CMD_FAILURE;
			}

			nd->u.shop.shop_item[j].nameid = nameid;
			nd->u.shop.shop_item[j].value = script_getnum(st,i+1);
			nd->u.shop.shop_item[j].qty = stock;

			npc_market_tosql(nd->exname, &nd->u.shop.shop_item[j]);
		}
		script_pushint(st,1);
		return SCRIPT_CMD_SUCCESS;
	}
#endif

	// append new items to existing shop item list
	RECREATE(nd->u.shop.shop_item, struct npc_item_list, nd->u.shop.count+amount);
	for (int n = nd->u.shop.count, i = 3, j = 0; j < amount; n++, i+=offs, j++)
	{
		t_itemid nameid = script_getnum( st, i );

		if( itemdb_exists( nameid ) == nullptr ){
			ShowError( "builtin_npcshopadditem: Item ID %u does not exist.\n", nameid );
			script_pushint( st, 0 );
			return SCRIPT_CMD_FAILURE;
		}

		nd->u.shop.shop_item[n].nameid = nameid;
		nd->u.shop.shop_item[n].value = script_getnum(st,i+1);
		nd->u.shop.count++;
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(npcshopdelitem)
{
	const char* npcname = script_getstr(st,2);
	struct npc_data* nd = npc_name2id(npcname);
	int n, i, size;
	unsigned short amount;

	if (!nd || ( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP)) { // Not found.
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	amount = script_lastdata(st)-2;
	size = nd->u.shop.count;

	// remove specified items from the shop item list
	for( i = 3; i < 3 + amount; i++ ) {
		t_itemid nameid = script_getnum(st,i);

		ARR_FIND( 0, size, n, nd->u.shop.shop_item[n].nameid == nameid );
		if( n < size ) {
			if (n+1 != size)
				memmove(&nd->u.shop.shop_item[n], &nd->u.shop.shop_item[n+1], sizeof(nd->u.shop.shop_item[0])*(size-n));
#if PACKETVER >= 20131223
			if (nd->subtype == NPCTYPE_MARKETSHOP)
				npc_market_delfromsql_(nd->exname, nameid, false);
#endif
			size--;
		}
	}

	RECREATE(nd->u.shop.shop_item, struct npc_item_list, size);
	nd->u.shop.count = size;

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Sets a script to attach to a shop npc.
 * npcshopattach "<npc_name>";
 **/
BUILDIN_FUNC(npcshopattach)
{
	const char* npcname = script_getstr(st,2);
	struct npc_data* nd = npc_name2id(npcname);
	int flag = 1;

	if( script_hasdata(st,3) )
		flag = script_getnum(st,3);

	if (!nd || ( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP)) { // Not found.
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (flag)
		nd->master_nd = ((struct npc_data *)map_id2bl(st->oid));
	else
		nd->master_nd = NULL;

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns some values of an item [Lupus]
 * Price, Weight, etc...
	setitemscript(itemID,"{new item bonus script}",[n]);
   Where n:
	0 - script
	1 - Equip script
	2 - Unequip script
 *------------------------------------------*/
BUILDIN_FUNC(setitemscript)
{
	int n = 0;
	const char *script;
	struct script_code **dstscript;

	t_itemid item_id = script_getnum(st,2);
	script = script_getstr(st,3);
	if( script_hasdata(st,4) )
		n=script_getnum(st,4);

	std::shared_ptr<item_data> i_data = item_db.find(item_id);

	if (!i_data || script==NULL || ( script[0] && script[0]!='{' )) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}
	switch (n) {
	case 2:
		dstscript = &i_data->unequip_script;
		break;
	case 1:
		dstscript = &i_data->equip_script;
		break;
	default:
		dstscript = &i_data->script;
		break;
	}
	if(*dstscript)
		script_free_code(*dstscript);

	*dstscript = script[0] ? parse_script(script, "script_setitemscript", 0, 0) : NULL;
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/*=======================================================
 * Add or Update a mob drop temporarily [Akinari]
 * Original Idea By: [Lupus]
 *
 * addmonsterdrop <mob_id or name>,<item_id>,<rate>,{<steal protected>,{<random option group id>}};
 *
 * If given an item the mob already drops, the rate
 * is updated to the new rate. Rate range is 1-10000.
 * Returns true if succeeded (added/updated a mob drop) false otherwise.
 *-------------------------------------------------------*/
BUILDIN_FUNC(addmonsterdrop)
{
	std::shared_ptr<s_mob_db> mob;

	if (script_isstring(st, 2))
		mob = mob_db.find(mobdb_searchname(script_getstr(st, 2)));
	else
		mob = mob_db.find(script_getnum(st, 2));

	if (mob == nullptr) {
		if (script_isstring(st, 2))
			ShowError("addmonsterdrop: bad mob name given %s\n", script_getstr(st, 2));
		else
			ShowError("addmonsterdrop: bad mob id given %d\n", script_getnum(st, 2));
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	t_itemid item_id = script_getnum(st, 3);
	std::shared_ptr<item_data> itm = item_db.find(item_id);

	if (itm == nullptr) {
		ShowError("addmonsterdrop: Nonexistant item %u requested.\n", item_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	int rate = script_getnum(st, 4);

	if (rate < 1 || rate > 10000) {
		ShowError("addmonsterdrop: Invalid rate %d (min 1, max 10000).\n", rate);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	uint16 c = 0;

	for (uint16 i = 0; i < MAX_MOB_DROP_TOTAL; i++) {
		if (mob->dropitem[i].nameid > 0) {
			if (mob->dropitem[i].nameid == item_id) { // If it equals item_id we update that drop
				c = i;
				break;
			}
			continue;
		}
		if (c == 0) // Accept first available slot only
			c = i;
	}
	if (c == 0) { // No place to put the new drop
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}

	int steal_protected = 0;
	int group = 0;

	if (script_hasdata(st,5))
		steal_protected = script_getnum(st, 5);
	if (script_hasdata(st,6)) {
		group = script_getnum(st, 6);

		if (!random_option_group.exists(group)) {
			ShowError("buildin_addmonsterdrop: Unknown random option group %d.\n", group);
			script_pushint(st, false);
			return SCRIPT_CMD_FAILURE;
		}
		if (itm->type != IT_WEAPON && itm->type != IT_ARMOR && itm->type != IT_SHADOWGEAR) {
			ShowError("buildin_addmonsterdrop: Random option group can't be used with this type of item (item Id: %d).\n", item_id);
			script_pushint(st, false);
			return SCRIPT_CMD_FAILURE;
		}
	}

	// Fill in the slot with the item and rate
	mob->dropitem[c].nameid = item_id;
	mob->dropitem[c].rate = rate;
	mob->dropitem[c].steal_protected = steal_protected > 0;
	mob->dropitem[c].randomopt_group = group;
	mob_reload_itemmob_data(); // Reload the mob search data stored in the item_data

	script_pushint(st, true);
	return SCRIPT_CMD_SUCCESS;

}

/*=======================================================
 * Delete a mob drop temporarily [Akinari]
 * Original Idea By: [Lupus]
 *
 * delmonsterdrop <mob_id or name>,<item_id>;
 *
 * Returns 1 if succeeded (deleted a mob drop)
 *-------------------------------------------------------*/
BUILDIN_FUNC(delmonsterdrop)
{
	std::shared_ptr<s_mob_db> mob;

	if(script_isstring(st, 2))
		mob = mob_db.find(mobdb_searchname(script_getstr(st,2)));
	else
		mob = mob_db.find(script_getnum(st,2));

	t_itemid item_id=script_getnum(st,3);

	if(!item_db.exists(item_id)){
		ShowError("delmonsterdrop: Nonexistant item %u requested.\n", item_id );
		return SCRIPT_CMD_FAILURE;
	}

	if(mob) { //We got a valid monster, check for item drop on monster
		unsigned char i;
		for(i = 0; i < MAX_MOB_DROP_TOTAL; i++) {
			if(mob->dropitem[i].nameid == item_id) {
				mob->dropitem[i].nameid = 0;
				mob->dropitem[i].rate = 0;
				mob->dropitem[i].steal_protected = false;
				mob->dropitem[i].randomopt_group = 0;
				mob_reload_itemmob_data(); // Reload the mob search data stored in the item_data
				script_pushint(st,1);
				return SCRIPT_CMD_SUCCESS;
			}
		}
		//No drop on that monster
		script_pushint(st,0);
	} else {
		ShowWarning("delmonsterdrop: bad mob id given %d\n",script_getnum(st,2));
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}


/*==========================================
 * Returns some values of a monster [Lupus]
 * Name, Level, race, size, etc...
	getmonsterinfo(monsterID,queryIndex);
 *------------------------------------------*/
BUILDIN_FUNC(getmonsterinfo)
{
	int mob_id;

	mob_id	= script_getnum(st,2);
	if (!mobdb_checkid(mob_id)) {
		//ShowError("buildin_getmonsterinfo: Wrong Monster ID: %i\n", mob_id);
		if ( script_getnum(st,3) == MOB_NAME ) // requested the name
			script_pushconststr(st,"null");
		else
			script_pushint(st,-1);
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<s_mob_db> mob = mob_db.find(mob_id);

	switch ( script_getnum(st,3) ) {
		case MOB_NAME:		script_pushstrcopy(st,mob->jname.c_str()); break;
		case MOB_LV:		script_pushint(st,mob->lv); break;
		case MOB_MAXHP:		script_pushint(st,mob->status.max_hp); break;
		case MOB_BASEEXP:	script_pushint(st,mob->base_exp); break;
		case MOB_JOBEXP:	script_pushint(st,mob->job_exp); break;
		case MOB_ATK1:		script_pushint(st,mob->status.rhw.atk); break;
		case MOB_ATK2:		script_pushint(st,mob->status.rhw.atk2); break;
		case MOB_DEF:		script_pushint(st,mob->status.def); break;
		case MOB_MDEF:		script_pushint(st,mob->status.mdef); break;
		case MOB_RES:		script_pushint(st, mob->status.res); break;
		case MOB_MRES:		script_pushint(st, mob->status.mres); break;
		case MOB_STR:		script_pushint(st,mob->status.str); break;
		case MOB_AGI:		script_pushint(st,mob->status.agi); break;
		case MOB_VIT:		script_pushint(st,mob->status.vit); break;
		case MOB_INT:		script_pushint(st,mob->status.int_); break;
		case MOB_DEX:		script_pushint(st,mob->status.dex); break;
		case MOB_LUK:		script_pushint(st,mob->status.luk); break;
		case MOB_RANGE:		script_pushint(st,mob->status.rhw.range); break;
		case MOB_RANGE2:	script_pushint(st,mob->range2); break;
		case MOB_RANGE3:	script_pushint(st,mob->range3); break;
		case MOB_SIZE:		script_pushint(st,mob->status.size); break;
		case MOB_RACE:		script_pushint(st,mob->status.race); break;
		case MOB_ELEMENT:	script_pushint(st,mob->status.def_ele); break;
		case MOB_MODE:		script_pushint(st,mob->status.mode); break;
		case MOB_MVPEXP:	script_pushint(st,mob->mexp); break;
		default: script_pushint(st,-1); //wrong Index
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Check player's vending/buyingstore/autotrade state
 * checkvending({"<Player Name>"})
 * @author [Nab4]
 */
BUILDIN_FUNC(checkvending) {
	TBL_PC *sd = NULL;

	if (!script_nick2sd(2,sd) ) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}
	else {
		int8 ret = 0;
		if (sd->state.vending)
			ret = 1;
		else if (sd->state.buyingstore)
			ret = 4;

#ifndef Pandas_Struct_Autotrade_Extend
		if (sd->state.autotrade)
			ret |= 2;
#else
		// 经由 Pandas_Struct_Autotrade_Extend 改造之后
		// sd->state.autotrade 同时也包含了其他挂机模式的位值在其中
		// 因此不能仅判断 sd->state.autotrade 是否非 0, 而应该进行明确指定的位运算判断
		if ((sd->state.autotrade & AUTOTRADE_VENDING) == AUTOTRADE_VENDING ||
			(sd->state.autotrade & AUTOTRADE_BUYINGSTORE) == AUTOTRADE_BUYINGSTORE) {
			ret |= 2;
		}
#endif // Pandas_Struct_Autotrade_Extend
		script_pushint(st, ret);
	}
	return SCRIPT_CMD_SUCCESS;
}


BUILDIN_FUNC(checkchatting) // check chatting [Marka]
{
	TBL_PC *sd = NULL;

	if( script_nick2sd(2,sd) )
		script_pushint(st,(sd->chatID != 0));
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(checkidle)
{
	TBL_PC *sd = NULL;

	if( script_nick2sd(2,sd) )
		script_pushint(st, DIFF_TICK(last_tick, sd->idletime));
	else
		script_pushint(st, 0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(checkidlehom)
{
	TBL_PC *sd = NULL;

	if( script_nick2sd(2,sd) )
		script_pushint(st, DIFF_TICK(last_tick, sd->idletime_hom));
	else
		script_pushint(st, 0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(checkidlemer)
{
	TBL_PC *sd;

	if( script_nick2sd(2,sd) )
		script_pushint(st, DIFF_TICK(last_tick, sd->idletime_mer));
	else
		script_pushint(st, 0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(searchitem)
{
	struct script_data* data = script_getdata(st, 2);
	const char *itemname = script_getstr(st,3);
	std::map<t_itemid, std::shared_ptr<item_data>> items;
	int count;

	char* name;
	int32 start;
	int32 id;
	int32 i;
	TBL_PC* sd = NULL;

	if ((items[0] = item_db.find(strtoul(itemname, nullptr, 10))))
		count = 1;
	else
		count = itemdb_searchname_array(items, MAX_SEARCH, itemname);

	if (!count) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if( !data_isreference(data) )
	{
		ShowError("script:searchitem: not a variable\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if( not_server_variable(*name) && !script_rid2sd(sd) )
	{
		return SCRIPT_CMD_SUCCESS;// no player attached
	}

	if( is_string_variable(name) )
	{// string array
		ShowError("script:searchitem: not an integer array reference\n");
		script_reportdata(data);
		st->state = END;
		return SCRIPT_CMD_FAILURE;// not supported
	}

	for( i = 0; i < count; ++start, ++i )
	{// Set array
		set_reg_num( st, sd, reference_uid( id, start ), name, items[i]->nameid, reference_getref( data ) );
	}

	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

// [zBuffer] List of player cont commands --->
BUILDIN_FUNC(rid2name)
{
	struct block_list *bl = NULL;
	int rid = script_getnum(st,2);
	if((bl = map_id2bl(rid)))
	{
		switch(bl->type) {
			case BL_MOB: script_pushstrcopy(st,((TBL_MOB*)bl)->name); break;
			case BL_PC:  script_pushstrcopy(st,((TBL_PC*)bl)->status.name); break;
			case BL_NPC: script_pushstrcopy(st,((TBL_NPC*)bl)->exname); break;
			case BL_PET: script_pushstrcopy(st,((TBL_PET*)bl)->pet.name); break;
			case BL_HOM: script_pushstrcopy(st,((TBL_HOM*)bl)->homunculus.name); break;
			case BL_MER: script_pushstrcopy(st,((TBL_MER*)bl)->db->name.c_str()); break;
			default:
				ShowError("buildin_rid2name: BL type unknown.\n");
				script_pushconststr(st,"");
				break;
		}
	} else {
		ShowError("buildin_rid2name: invalid RID\n");
		script_pushconststr(st,"(null)");
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Toggle a unit from moving.
 * pcblockmove(<unit_id>,<option>);
 */
BUILDIN_FUNC(pcblockmove)
{
	struct block_list *bl = NULL;

	if (script_getnum(st, 2))
		bl = map_id2bl(script_getnum(st,2));
	else
		bl = map_id2bl(st->rid);

	if (bl) {
		struct unit_data *ud = unit_bl2ud(bl);

		if (ud)
			ud->state.blockedmove = script_getnum(st,3) > 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Toggle a unit from casting skills.
 * pcblockskill(<unit_id>,<option>);
 */
BUILDIN_FUNC(pcblockskill)
{
	struct block_list *bl = NULL;

	if (script_getnum(st, 2))
		bl = map_id2bl(script_getnum(st,2));
	else
		bl = map_id2bl(st->rid);

	if (bl) {
		struct unit_data *ud = unit_bl2ud(bl);

		if (ud)
			ud->state.blockedskill = script_getnum(st, 3) > 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(setpcblock)
{
	TBL_PC *sd;

	if (script_mapid2sd(4, sd)) {
		enum e_pcblock_action_flag type = (e_pcblock_action_flag)script_getnum(st, 2);

		if (script_getnum(st, 3) > 0)
			sd->state.block_action |= type;
		else
			sd->state.block_action &= ~type;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getpcblock)
{
	TBL_PC *sd;

	if (script_mapid2sd(2, sd))
		script_pushint(st, sd->state.block_action);
	else
		script_pushint(st, 0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pcfollow)
{
	TBL_PC *sd;

	if( script_mapid2sd(2,sd) )
		pc_follow(sd, script_getnum(st,3));

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pcstopfollow)
{
	TBL_PC *sd;

	if(script_mapid2sd(2,sd))
		pc_stop_following(sd);

	return SCRIPT_CMD_SUCCESS;
}
// <--- [zBuffer] List of player cont commands
// [zBuffer] List of unit control commands --->

/// Checks to see if the unit exists.
///
/// unitexists <unit id>;
BUILDIN_FUNC(unitexists)
{
	struct block_list* bl;

	bl = map_id2bl(script_getnum(st, 2));

#ifndef Pandas_ScriptCommand_UnitExists
	if (!bl)
		script_pushint(st, false);
	else
		script_pushint(st, true);
#else
	int require_alive = 0;
	if (!script_get_optnum(st, 3, "Require Unit alive or not", require_alive, true, 0)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (!require_alive) {
		if (!bl)
			script_pushint(st, false);
		else
			script_pushint(st, true);
	}
	else {
		if (!bl || status_isdead(bl) == 1)
			script_pushint(st, false);
		else
			script_pushint(st, true);
	}
#endif // Pandas_ScriptCommand_UnitExists

	return SCRIPT_CMD_SUCCESS;
}

/// Gets the type of the given Game ID.
///
/// getunittype <unit id>;
BUILDIN_FUNC(getunittype)
{
	struct block_list* bl;
	int value = 0;

	if(!script_rid2bl(2,bl))
	{
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	switch (bl->type) {
		case BL_PC:
		case BL_NPC:
		case BL_PET:
		case BL_MOB:
		case BL_HOM:
		case BL_MER:
		case BL_ELEM:
			value = bl->type;
			break;
		default:
			value = -1;
			break;
	}

	script_pushint(st, value);
	return SCRIPT_CMD_SUCCESS;
}

/// Gets specific live information of a bl.
///
/// getunitdata <unit id>,<arrayname>;
BUILDIN_FUNC(getunitdata)
{
	TBL_PC *sd = st->rid ? map_id2sd(st->rid) : NULL;
	struct block_list* bl;
	TBL_MOB* md = NULL;
	TBL_HOM* hd = NULL;
	TBL_MER* mc = NULL;
	TBL_PET* pd = NULL;
	TBL_ELEM* ed = NULL;
	TBL_NPC* nd = NULL;
	char* name;
	struct script_data *data = script_getdata(st, 3);

	if (!data_isreference(data)) {
		ShowWarning("buildin_getunitdata: Error in argument! Please give a variable to store values in.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if(!script_rid2bl(2,bl))
	{
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	switch (bl->type) {
		case BL_MOB:  md = map_id2md(bl->id); break;
		case BL_HOM:  hd = map_id2hd(bl->id); break;
		case BL_PET:  pd = map_id2pd(bl->id); break;
		case BL_MER:  mc = map_id2mc(bl->id); break;
		case BL_ELEM: ed = map_id2ed(bl->id); break;
		case BL_NPC:  nd = map_id2nd(bl->id); break;
		default:
			ShowWarning("buildin_getunitdata: Invalid object type!\n");
			return SCRIPT_CMD_FAILURE;
	}

	name = reference_getname(data);

	if (not_server_variable(*name) && !script_rid2sd(sd)) {
		ShowError("buildin_getunitdata: Cannot use a player variable '%s' if no player is attached.\n", name);
		return SCRIPT_CMD_FAILURE;
	}

#define getunitdata_sub(idx__,var__) setd_sub_num(st,sd,name,(idx__),(var__),data->ref)

	switch(bl->type) {
		case BL_MOB:
			if (!md) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_MOB!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UMOB_SIZE, md->status.size);
			getunitdata_sub(UMOB_LEVEL, md->level);
			getunitdata_sub(UMOB_HP, md->status.hp);
			getunitdata_sub(UMOB_MAXHP, md->status.max_hp);
			getunitdata_sub(UMOB_MASTERAID, md->master_id);
			getunitdata_sub(UMOB_MAPID, md->bl.m);
			getunitdata_sub(UMOB_X, md->bl.x);
			getunitdata_sub(UMOB_Y, md->bl.y);
			getunitdata_sub(UMOB_SPEED, md->status.speed);
			getunitdata_sub(UMOB_MODE, md->status.mode);
			getunitdata_sub(UMOB_AI, md->special_state.ai);
			getunitdata_sub(UMOB_SCOPTION, md->sc.option);
			getunitdata_sub(UMOB_SEX, md->vd->sex);
			getunitdata_sub(UMOB_CLASS, md->vd->class_);
			getunitdata_sub(UMOB_HAIRSTYLE, md->vd->hair_style);
			getunitdata_sub(UMOB_HAIRCOLOR, md->vd->hair_color);
			getunitdata_sub(UMOB_HEADBOTTOM, md->vd->head_bottom);
			getunitdata_sub(UMOB_HEADMIDDLE, md->vd->head_mid);
			getunitdata_sub(UMOB_HEADTOP, md->vd->head_top);
			getunitdata_sub(UMOB_CLOTHCOLOR, md->vd->cloth_color);
			getunitdata_sub(UMOB_SHIELD, md->vd->shield);
			getunitdata_sub(UMOB_WEAPON, md->vd->weapon);
			getunitdata_sub(UMOB_LOOKDIR, md->ud.dir);
			getunitdata_sub(UMOB_CANMOVETICK, md->ud.canmove_tick);
			getunitdata_sub(UMOB_STR, md->status.str);
			getunitdata_sub(UMOB_AGI, md->status.agi);
			getunitdata_sub(UMOB_VIT, md->status.vit);
			getunitdata_sub(UMOB_INT, md->status.int_);
			getunitdata_sub(UMOB_DEX, md->status.dex);
			getunitdata_sub(UMOB_LUK, md->status.luk);
			getunitdata_sub(UMOB_SLAVECPYMSTRMD, md->state.copy_master_mode);
			getunitdata_sub(UMOB_DMGIMMUNE, md->ud.immune_attack);
			getunitdata_sub(UMOB_ATKRANGE, md->status.rhw.range);
			getunitdata_sub(UMOB_ATKMIN, md->status.rhw.atk);
			getunitdata_sub(UMOB_ATKMAX, md->status.rhw.atk2);
			getunitdata_sub(UMOB_MATKMIN, md->status.matk_min);
			getunitdata_sub(UMOB_MATKMAX, md->status.matk_max);
			getunitdata_sub(UMOB_DEF, md->status.def);
			getunitdata_sub(UMOB_MDEF, md->status.mdef);
			getunitdata_sub(UMOB_HIT, md->status.hit);
			getunitdata_sub(UMOB_FLEE, md->status.flee);
			getunitdata_sub(UMOB_PDODGE, md->status.flee2);
			getunitdata_sub(UMOB_CRIT, md->status.cri);
			getunitdata_sub(UMOB_RACE, md->status.race);
			getunitdata_sub(UMOB_ELETYPE, md->status.def_ele);
			getunitdata_sub(UMOB_ELELEVEL, md->status.ele_lv);
			getunitdata_sub(UMOB_AMOTION, md->status.amotion);
			getunitdata_sub(UMOB_ADELAY, md->status.adelay);
			getunitdata_sub(UMOB_DMOTION, md->status.dmotion);
			getunitdata_sub(UMOB_TARGETID, md->target_id);
			getunitdata_sub(UMOB_ROBE, md->vd->robe);
			getunitdata_sub(UMOB_BODY2, md->vd->body_style);
			getunitdata_sub(UMOB_GROUP_ID, md->ud.group_id);
			getunitdata_sub(UMOB_IGNORE_CELL_STACK_LIMIT, md->ud.state.ignore_cell_stack_limit);
			getunitdata_sub(UMOB_RES, md->status.res);
			getunitdata_sub(UMOB_MRES, md->status.mres);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UMOB_AURA, md->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
#ifdef Pandas_ScriptParams_UnitData_DamageTaken
			getunitdata_sub(UMOB_DAMAGETAKEN, md->pandas.damagetaken);
			getunitdata_sub(UMOB_DAMAGETAKEN_DB, md->db->damagetaken);
#endif // Pandas_ScriptParams_UnitData_DamageTaken
#ifdef Pandas_ScriptParams_UnitData_Experience
			getunitdata_sub(UMOB_MOBBASEEXP, md->pandas.base_exp);
			getunitdata_sub(UMOB_MOBBASEEXP_DB, md->db->base_exp);
			getunitdata_sub(UMOB_MOBJOBEXP, md->pandas.job_exp);
			getunitdata_sub(UMOB_MOBJOBEXP_DB, md->db->job_exp);
#endif // Pandas_ScriptParams_UnitData_Experience
			break;

		case BL_HOM:
			if (!hd) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_HOM!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UHOM_SIZE, hd->base_status.size);
			getunitdata_sub(UHOM_LEVEL, hd->homunculus.level);
			getunitdata_sub(UHOM_HP, hd->homunculus.hp);
			getunitdata_sub(UHOM_MAXHP, hd->homunculus.max_hp);
			getunitdata_sub(UHOM_SP, hd->homunculus.sp);
			getunitdata_sub(UHOM_MAXSP, hd->homunculus.max_sp);
			getunitdata_sub(UHOM_MASTERCID, hd->homunculus.char_id);
			getunitdata_sub(UHOM_MAPID, hd->bl.m);
			getunitdata_sub(UHOM_X, hd->bl.x);
			getunitdata_sub(UHOM_Y, hd->bl.y);
			getunitdata_sub(UHOM_HUNGER, hd->homunculus.hunger);
			getunitdata_sub(UHOM_INTIMACY, hd->homunculus.intimacy);
			getunitdata_sub(UHOM_SPEED, hd->base_status.speed);
			getunitdata_sub(UHOM_LOOKDIR, hd->ud.dir);
			getunitdata_sub(UHOM_CANMOVETICK, hd->ud.canmove_tick);
			getunitdata_sub(UHOM_STR, hd->base_status.str);
			getunitdata_sub(UHOM_AGI, hd->base_status.agi);
			getunitdata_sub(UHOM_VIT, hd->base_status.vit);
			getunitdata_sub(UHOM_INT, hd->base_status.int_);
			getunitdata_sub(UHOM_DEX, hd->base_status.dex);
			getunitdata_sub(UHOM_LUK, hd->base_status.luk);
			getunitdata_sub(UHOM_DMGIMMUNE, hd->ud.immune_attack);
			getunitdata_sub(UHOM_ATKRANGE, hd->battle_status.rhw.range);
			getunitdata_sub(UHOM_ATKMIN, hd->base_status.rhw.atk);
			getunitdata_sub(UHOM_ATKMAX, hd->base_status.rhw.atk2);
			getunitdata_sub(UHOM_MATKMIN, hd->base_status.matk_min);
			getunitdata_sub(UHOM_MATKMAX, hd->base_status.matk_max);
			getunitdata_sub(UHOM_DEF, hd->battle_status.def);
			getunitdata_sub(UHOM_MDEF, hd->battle_status.mdef);
			getunitdata_sub(UHOM_HIT, hd->battle_status.hit);
			getunitdata_sub(UHOM_FLEE, hd->battle_status.flee);
			getunitdata_sub(UHOM_PDODGE, hd->battle_status.flee2);
			getunitdata_sub(UHOM_CRIT, hd->battle_status.cri);
			getunitdata_sub(UHOM_RACE, hd->battle_status.race);
			getunitdata_sub(UHOM_ELETYPE, hd->battle_status.def_ele);
			getunitdata_sub(UHOM_ELELEVEL, hd->battle_status.ele_lv);
			getunitdata_sub(UHOM_AMOTION, hd->battle_status.amotion);
			getunitdata_sub(UHOM_ADELAY, hd->battle_status.adelay);
			getunitdata_sub(UHOM_DMOTION, hd->battle_status.dmotion);
			getunitdata_sub(UHOM_TARGETID, hd->ud.target);
			getunitdata_sub(UHOM_GROUP_ID, hd->ud.group_id);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UHOM_AURA, hd->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
			break;

		case BL_PET:
			if (!pd) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_PET!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UPET_SIZE, pd->status.size);
			getunitdata_sub(UPET_LEVEL, pd->pet.level);
			getunitdata_sub(UPET_HP, pd->status.hp);
			getunitdata_sub(UPET_MAXHP, pd->status.max_hp);
			getunitdata_sub(UPET_MASTERAID, pd->pet.account_id);
			getunitdata_sub(UPET_MAPID, pd->bl.m);
			getunitdata_sub(UPET_X, pd->bl.x);
			getunitdata_sub(UPET_Y, pd->bl.y);
			getunitdata_sub(UPET_HUNGER, pd->pet.hungry);
			getunitdata_sub(UPET_INTIMACY, pd->pet.intimate);
			getunitdata_sub(UPET_SPEED, pd->status.speed);
			getunitdata_sub(UPET_LOOKDIR, pd->ud.dir);
			getunitdata_sub(UPET_CANMOVETICK, pd->ud.canmove_tick);
			getunitdata_sub(UPET_STR, pd->status.str);
			getunitdata_sub(UPET_AGI, pd->status.agi);
			getunitdata_sub(UPET_VIT, pd->status.vit);
			getunitdata_sub(UPET_INT, pd->status.int_);
			getunitdata_sub(UPET_DEX, pd->status.dex);
			getunitdata_sub(UPET_LUK, pd->status.luk);
			getunitdata_sub(UPET_DMGIMMUNE, pd->ud.immune_attack);
			getunitdata_sub(UPET_ATKRANGE, pd->status.rhw.range);
			getunitdata_sub(UPET_ATKMIN, pd->status.rhw.atk);
			getunitdata_sub(UPET_ATKMAX, pd->status.rhw.atk2);
			getunitdata_sub(UPET_MATKMIN, pd->status.matk_min);
			getunitdata_sub(UPET_MATKMAX, pd->status.matk_max);
			getunitdata_sub(UPET_DEF, pd->status.def);
			getunitdata_sub(UPET_MDEF, pd->status.mdef);
			getunitdata_sub(UPET_HIT, pd->status.hit);
			getunitdata_sub(UPET_FLEE, pd->status.flee);
			getunitdata_sub(UPET_PDODGE, pd->status.flee2);
			getunitdata_sub(UPET_CRIT, pd->status.cri);
			getunitdata_sub(UPET_RACE, pd->status.race);
			getunitdata_sub(UPET_ELETYPE, pd->status.def_ele);
			getunitdata_sub(UPET_ELELEVEL, pd->status.ele_lv);
			getunitdata_sub(UPET_AMOTION, pd->status.amotion);
			getunitdata_sub(UPET_ADELAY, pd->status.adelay);
			getunitdata_sub(UPET_DMOTION, pd->status.dmotion);
			getunitdata_sub(UPET_GROUP_ID, pd->ud.group_id);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UPET_AURA, pd->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
			break;

		case BL_MER:
			if (!mc) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_MER!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UMER_SIZE, mc->base_status.size);
			getunitdata_sub(UMER_HP, mc->base_status.hp);
			getunitdata_sub(UMER_MAXHP, mc->base_status.max_hp);
			getunitdata_sub(UMER_MASTERCID, mc->mercenary.char_id);
			getunitdata_sub(UMER_MAPID, mc->bl.m);
			getunitdata_sub(UMER_X, mc->bl.x);
			getunitdata_sub(UMER_Y, mc->bl.y);
			getunitdata_sub(UMER_KILLCOUNT, mc->mercenary.kill_count);
			getunitdata_sub(UMER_LIFETIME, mc->mercenary.life_time);
			getunitdata_sub(UMER_SPEED, mc->base_status.speed);
			getunitdata_sub(UMER_LOOKDIR, mc->ud.dir);
			getunitdata_sub(UMER_CANMOVETICK, mc->ud.canmove_tick);
			getunitdata_sub(UMER_STR, mc->base_status.str);
			getunitdata_sub(UMER_AGI, mc->base_status.agi);
			getunitdata_sub(UMER_VIT, mc->base_status.vit);
			getunitdata_sub(UMER_INT, mc->base_status.int_);
			getunitdata_sub(UMER_DEX, mc->base_status.dex);
			getunitdata_sub(UMER_LUK, mc->base_status.luk);
			getunitdata_sub(UMER_DMGIMMUNE, mc->ud.immune_attack);
			getunitdata_sub(UMER_ATKRANGE, mc->base_status.rhw.range);
			getunitdata_sub(UMER_ATKMIN, mc->base_status.rhw.atk);
			getunitdata_sub(UMER_ATKMAX, mc->base_status.rhw.atk2);
			getunitdata_sub(UMER_MATKMIN, mc->base_status.matk_min);
			getunitdata_sub(UMER_MATKMAX, mc->base_status.matk_max);
			getunitdata_sub(UMER_DEF, mc->base_status.def);
			getunitdata_sub(UMER_MDEF, mc->base_status.mdef);
			getunitdata_sub(UMER_HIT, mc->base_status.hit);
			getunitdata_sub(UMER_FLEE, mc->base_status.flee);
			getunitdata_sub(UMER_PDODGE, mc->base_status.flee2);
			getunitdata_sub(UMER_CRIT, mc->base_status.cri);
			getunitdata_sub(UMER_RACE, mc->base_status.race);
			getunitdata_sub(UMER_ELETYPE, mc->base_status.def_ele);
			getunitdata_sub(UMER_ELELEVEL, mc->base_status.ele_lv);
			getunitdata_sub(UMER_AMOTION, mc->base_status.amotion);
			getunitdata_sub(UMER_ADELAY, mc->base_status.adelay);
			getunitdata_sub(UMER_DMOTION, mc->base_status.dmotion);
			getunitdata_sub(UMER_TARGETID, mc->ud.target);
			getunitdata_sub(UMER_GROUP_ID, mc->ud.group_id);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UMER_AURA, mc->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
			break;

		case BL_ELEM:
			if (!ed) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_ELEM!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UELE_SIZE, ed->base_status.size);
			getunitdata_sub(UELE_HP, ed->elemental.hp);
			getunitdata_sub(UELE_MAXHP, ed->elemental.max_hp);
			getunitdata_sub(UELE_SP, ed->elemental.sp);
			getunitdata_sub(UELE_MAXSP, ed->elemental.max_sp);
			getunitdata_sub(UELE_MASTERCID, ed->elemental.char_id);
			getunitdata_sub(UELE_MAPID, ed->bl.m);
			getunitdata_sub(UELE_X, ed->bl.x);
			getunitdata_sub(UELE_Y, ed->bl.y);
			getunitdata_sub(UELE_LIFETIME, ed->elemental.life_time);
			getunitdata_sub(UELE_MODE, ed->elemental.mode);
			getunitdata_sub(UELE_SP, ed->base_status.speed);
			getunitdata_sub(UELE_LOOKDIR, ed->ud.dir);
			getunitdata_sub(UELE_CANMOVETICK, ed->ud.canmove_tick);
			getunitdata_sub(UELE_STR, ed->base_status.str);
			getunitdata_sub(UELE_AGI, ed->base_status.agi);
			getunitdata_sub(UELE_VIT, ed->base_status.vit);
			getunitdata_sub(UELE_INT, ed->base_status.int_);
			getunitdata_sub(UELE_DEX, ed->base_status.dex);
			getunitdata_sub(UELE_LUK, ed->base_status.luk);
			getunitdata_sub(UELE_DMGIMMUNE, ed->ud.immune_attack);
			getunitdata_sub(UELE_ATKRANGE, ed->base_status.rhw.range);
			getunitdata_sub(UELE_ATKMIN, ed->base_status.rhw.atk);
			getunitdata_sub(UELE_ATKMAX, ed->base_status.rhw.atk2);
			getunitdata_sub(UELE_MATKMIN, ed->base_status.matk_min);
			getunitdata_sub(UELE_MATKMAX, ed->base_status.matk_max);
			getunitdata_sub(UELE_DEF, ed->base_status.def);
			getunitdata_sub(UELE_MDEF, ed->base_status.mdef);
			getunitdata_sub(UELE_HIT, ed->base_status.hit);
			getunitdata_sub(UELE_FLEE, ed->base_status.flee);
			getunitdata_sub(UELE_PDODGE, ed->base_status.flee2);
			getunitdata_sub(UELE_CRIT, ed->base_status.cri);
			getunitdata_sub(UELE_RACE, ed->base_status.race);
			getunitdata_sub(UELE_ELETYPE, ed->base_status.def_ele);
			getunitdata_sub(UELE_ELELEVEL, ed->base_status.ele_lv);
			getunitdata_sub(UELE_AMOTION, ed->base_status.amotion);
			getunitdata_sub(UELE_ADELAY, ed->base_status.adelay);
			getunitdata_sub(UELE_DMOTION, ed->base_status.dmotion);
			getunitdata_sub(UELE_TARGETID, ed->ud.target);
			getunitdata_sub(UELE_GROUP_ID, ed->ud.group_id);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UELE_AURA, ed->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
			break;

		case BL_NPC:
			if (!nd) {
				ShowWarning("buildin_getunitdata: Error in finding object BL_NPC!\n");
				return SCRIPT_CMD_FAILURE;
			}
			getunitdata_sub(UNPC_LEVEL, nd->level);
			getunitdata_sub(UNPC_HP, nd->status.hp);
			getunitdata_sub(UNPC_MAXHP, nd->status.max_hp);
			getunitdata_sub(UNPC_MAPID, nd->bl.m);
			getunitdata_sub(UNPC_X, nd->bl.x);
			getunitdata_sub(UNPC_Y, nd->bl.y);
			getunitdata_sub(UNPC_LOOKDIR, nd->ud.dir);
			getunitdata_sub(UNPC_STR, nd->status.str);
			getunitdata_sub(UNPC_AGI, nd->status.agi);
			getunitdata_sub(UNPC_VIT, nd->status.vit);
			getunitdata_sub(UNPC_INT, nd->status.int_);
			getunitdata_sub(UNPC_DEX, nd->status.dex);
			getunitdata_sub(UNPC_LUK, nd->status.luk);
			getunitdata_sub(UNPC_PLUSALLSTAT, nd->stat_point);
			getunitdata_sub(UNPC_DMGIMMUNE, nd->ud.immune_attack);
			getunitdata_sub(UNPC_ATKRANGE, nd->status.rhw.range);
			getunitdata_sub(UNPC_ATKMIN, nd->status.rhw.atk);
			getunitdata_sub(UNPC_ATKMAX, nd->status.rhw.atk2);
			getunitdata_sub(UNPC_MATKMIN, nd->status.matk_min);
			getunitdata_sub(UNPC_MATKMAX, nd->status.matk_max);
			getunitdata_sub(UNPC_DEF, nd->status.def);
			getunitdata_sub(UNPC_MDEF, nd->status.mdef);
			getunitdata_sub(UNPC_HIT, nd->status.hit);
			getunitdata_sub(UNPC_FLEE, nd->status.flee);
			getunitdata_sub(UNPC_PDODGE, nd->status.flee2);
			getunitdata_sub(UNPC_CRIT, nd->status.cri);
			getunitdata_sub(UNPC_RACE, nd->status.race);
			getunitdata_sub(UNPC_ELETYPE, nd->status.def_ele);
			getunitdata_sub(UNPC_ELELEVEL, nd->status.ele_lv);
			getunitdata_sub(UNPC_AMOTION,  nd->status.amotion);
			getunitdata_sub(UNPC_ADELAY, nd->status.adelay);
			getunitdata_sub(UNPC_DMOTION, nd->status.dmotion);
			getunitdata_sub(UNPC_SEX, nd->vd.sex);
			getunitdata_sub(UNPC_CLASS, nd->vd.class_);
			getunitdata_sub(UNPC_HAIRSTYLE, nd->vd.hair_style);
			getunitdata_sub(UNPC_HAIRCOLOR, nd->vd.hair_color);
			getunitdata_sub(UNPC_HEADBOTTOM, nd->vd.head_bottom);
			getunitdata_sub(UNPC_HEADMIDDLE, nd->vd.head_mid);
			getunitdata_sub(UNPC_HEADTOP, nd->vd.head_top);
			getunitdata_sub(UNPC_CLOTHCOLOR, nd->vd.cloth_color);
			getunitdata_sub(UNPC_SHIELD, nd->vd.shield);
			getunitdata_sub(UNPC_WEAPON, nd->vd.weapon);
			getunitdata_sub(UNPC_ROBE, nd->vd.robe);
			getunitdata_sub(UNPC_BODY2, nd->vd.body_style);
			getunitdata_sub(UNPC_DEADSIT, nd->vd.dead_sit);
			getunitdata_sub(UNPC_GROUP_ID, nd->ud.group_id);
#ifdef Pandas_Struct_Unit_CommonData_Aura
			getunitdata_sub(UNPC_AURA, nd->ucd.aura.id);
#endif // Pandas_Struct_Unit_CommonData_Aura
			break;

		default:
			ShowWarning("buildin_getunitdata: Unknown object type!\n");
			return SCRIPT_CMD_FAILURE;
	}

#undef getunitdata_sub

	return SCRIPT_CMD_SUCCESS;
}

/// Changes the live data of a bl.
///
/// setunitdata <unit id>,<type>,<value>;
BUILDIN_FUNC(setunitdata)
{
	struct block_list* bl;

	if(!script_rid2bl(2,bl))
	{
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	TBL_MOB* md;
	TBL_HOM* hd;
	TBL_MER* mc;
	TBL_PET* pd;
	TBL_ELEM* ed;
	TBL_NPC* nd;

	switch (bl->type) {
		case BL_MOB:  md = map_id2md(bl->id); break;
		case BL_HOM:  hd = map_id2hd(bl->id); break;
		case BL_PET:  pd = map_id2pd(bl->id); break;
		case BL_MER:  mc = map_id2mc(bl->id); break;
		case BL_ELEM: ed = map_id2ed(bl->id); break;
		case BL_NPC:
			nd = map_id2nd(bl->id);
			if (!nd->status.hp)
				status_calc_npc(nd, SCO_FIRST);
			else
				status_calc_npc(nd, SCO_NONE);
			break;
		default:
			ShowError("buildin_setunitdata: Invalid object!\n");
			return SCRIPT_CMD_FAILURE;
	}

	int type = script_getnum(st, 3), value = 0;
	const char *mapname = NULL;
	bool calc_status = false;
#ifdef Pandas_ScriptParams_UnitData_Experience
	int64 value64 = 0;
#endif // Pandas_ScriptParams_UnitData_Experience

	if ((type == UMOB_MAPID || type == UHOM_MAPID || type == UPET_MAPID || type == UMER_MAPID || type == UELE_MAPID || type == UNPC_MAPID) && script_isstring(st, 4))
		mapname = script_getstr(st, 4);
	else
		value = script_getnum(st, 4);

#ifdef Pandas_ScriptParams_UnitData_Experience
	if (bl->type == BL_MOB && (type == UMOB_MOBBASEEXP || type == UMOB_MOBJOBEXP)) {
		value64 = script_getnum64(st, 4);
	}
#endif // Pandas_ScriptParams_UnitData_Experience

	switch (bl->type) {
	case BL_MOB:
		if (!md) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_MOB!\n");
			return SCRIPT_CMD_FAILURE;
		}
		if (!md->base_status) {
			md->base_status = (struct status_data*)aCalloc(1, sizeof(struct status_data));
			memcpy(md->base_status, &md->db->status, sizeof(struct status_data));
#ifdef Pandas_Struct_Mob_Data_Special_SetUnitData
			if (md->pandas.special_setunitdata) {
				md->pandas.special_setunitdata->clear();
			}
#endif // Pandas_Struct_Mob_Data_Special_SetUnitData
		}

		// Check if the view data will be modified
		switch( type ){
			case UMOB_SEX:
			//case UMOB_CLASS: // Called by status_set_viewdata
			case UMOB_HAIRSTYLE:
			case UMOB_HAIRCOLOR:
			case UMOB_HEADBOTTOM:
			case UMOB_HEADMIDDLE:
			case UMOB_HEADTOP:
			case UMOB_CLOTHCOLOR:
			case UMOB_SHIELD:
			case UMOB_WEAPON:
			case UMOB_ROBE:
			case UMOB_BODY2:
				mob_set_dynamic_viewdata( md );
				break;
		}

		switch (type) {
			case UMOB_SIZE: md->status.size = md->base_status->size = (unsigned char)value; break;
			case UMOB_LEVEL: md->level = (unsigned short)value; clif_name_area(&md->bl); break;
			case UMOB_HP: md->base_status->hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); clif_name_area(&md->bl); break;
			case UMOB_MAXHP: md->base_status->hp = md->base_status->max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); clif_name_area(&md->bl); break;
			case UMOB_MASTERAID: md->master_id = value; break;
			case UMOB_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UMOB_X: if (!unit_walktoxy(bl, (short)value, md->bl.y, 2)) unit_movepos(bl, (short)value, md->bl.y, 0, 0); break;
			case UMOB_Y: if (!unit_walktoxy(bl, md->bl.x, (short)value, 2)) unit_movepos(bl, md->bl.x, (short)value, 0, 0); break;
			case UMOB_SPEED: md->base_status->speed = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_MODE: md->base_status->mode = (enum e_mode)value; calc_status = true; break;
			case UMOB_AI: md->special_state.ai = (enum mob_ai)value; break;
			case UMOB_SCOPTION: md->sc.option = (unsigned short)value; break;
			case UMOB_SEX: md->vd->sex = (char)value; unit_refresh(bl); break;
			case UMOB_CLASS: status_set_viewdata(bl, (unsigned short)value); unit_refresh(bl); break;
			case UMOB_HAIRSTYLE: clif_changelook(bl, LOOK_HAIR, (unsigned short)value); break;
			case UMOB_HAIRCOLOR: clif_changelook(bl, LOOK_HAIR_COLOR, (unsigned short)value); break;
			case UMOB_HEADBOTTOM: clif_changelook(bl, LOOK_HEAD_BOTTOM, (unsigned short)value); break;
			case UMOB_HEADMIDDLE: clif_changelook(bl, LOOK_HEAD_MID, (unsigned short)value); break;
			case UMOB_HEADTOP: clif_changelook(bl, LOOK_HEAD_TOP, (unsigned short)value); break;
			case UMOB_CLOTHCOLOR: clif_changelook(bl, LOOK_CLOTHES_COLOR, (unsigned short)value); break;
			case UMOB_SHIELD: clif_changelook(bl, LOOK_SHIELD, (unsigned short)value); break;
			case UMOB_WEAPON: clif_changelook(bl, LOOK_WEAPON, (unsigned short)value); break;
			case UMOB_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UMOB_CANMOVETICK: md->ud.canmove_tick = value > 0 ? (unsigned int)value : 0; break;
			case UMOB_STR: md->base_status->str = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_AGI: md->base_status->agi = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_VIT: md->base_status->vit = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_INT: md->base_status->int_ = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_DEX: md->base_status->dex = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_LUK: md->base_status->luk = (unsigned short)value; status_calc_misc(bl, &md->status, md->level); calc_status = true; break;
			case UMOB_SLAVECPYMSTRMD:
				if (value > 0) {
					TBL_MOB *md2 = NULL;
					if (!md->master_id || !(md2 = map_id2md(md->master_id))) {
						ShowWarning("buildin_setunitdata: Trying to set UMOB_SLAVECPYMSTRMD on mob without master!\n");
						break;
					}
					md->base_status->mode = md2->status.mode;
					md->state.copy_master_mode = 1;
				} else
					md->state.copy_master_mode = 0;
				calc_status = true;
				break;
			case UMOB_DMGIMMUNE: md->ud.immune_attack = value > 0; break;
			case UMOB_ATKRANGE: md->base_status->rhw.range = (unsigned short)value; calc_status = true; break;
			case UMOB_ATKMIN: md->base_status->rhw.atk = (unsigned short)value; calc_status = true; break;
			case UMOB_ATKMAX: md->base_status->rhw.atk2 = (unsigned short)value; calc_status = true; break;
			case UMOB_MATKMIN: md->base_status->matk_min = (unsigned short)value; calc_status = true; break;
			case UMOB_MATKMAX: md->base_status->matk_max = (unsigned short)value; calc_status = true; break;
			case UMOB_DEF: md->base_status->def = (pec_defType)value; calc_status = true; break;
			case UMOB_MDEF: md->base_status->mdef = (pec_defType)value; calc_status = true; break;
			case UMOB_HIT: md->base_status->hit = (short)value; calc_status = true; break;
			case UMOB_FLEE: md->base_status->flee = (short)value; calc_status = true; break;
			case UMOB_PDODGE: md->base_status->flee2 = (short)value; calc_status = true; break;
			case UMOB_CRIT: md->base_status->cri = (short)value; calc_status = true; break;
			case UMOB_RACE: md->status.race = md->base_status->race = (unsigned char)value; break;
			case UMOB_ELETYPE: md->base_status->def_ele = (unsigned char)value; calc_status = true; break;
			case UMOB_ELELEVEL: md->base_status->ele_lv = (unsigned char)value; calc_status = true; break;
			case UMOB_AMOTION: md->base_status->amotion = (short)value; calc_status = true; break;
			case UMOB_ADELAY: md->base_status->adelay = (short)value; calc_status = true; break;
			case UMOB_DMOTION: md->base_status->dmotion = (short)value; calc_status = true; break;
			case UMOB_TARGETID: {
				if (value==0) {
					mob_unlocktarget(md,gettick());
					break;
				}
				struct block_list* target = map_id2bl(value);
				if (!target) {
					ShowWarning("buildin_setunitdata: Error in finding target for BL_MOB!\n");
					return SCRIPT_CMD_FAILURE;
				}
				mob_target(md,target,0);
				break;
			}
			case UMOB_ROBE: clif_changelook(bl, LOOK_ROBE, (unsigned short)value); break;
			case UMOB_BODY2: clif_changelook(bl, LOOK_BODY2, (unsigned short)value); break;
			case UMOB_GROUP_ID: md->ud.group_id = value; unit_refresh(bl); break;
			case UMOB_IGNORE_CELL_STACK_LIMIT: md->ud.state.ignore_cell_stack_limit = value > 0; break;
			case UMOB_RES: md->base_status->res = (pec_short)value; calc_status = true; break;
			case UMOB_MRES: md->base_status->mres = (pec_short)value; calc_status = true; break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UMOB_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
#ifdef Pandas_ScriptParams_UnitData_DamageTaken
			case UMOB_DAMAGETAKEN: md->pandas.damagetaken = cap_value(value, -1, UINT16_MAX); break;
#endif // Pandas_ScriptParams_UnitData_DamageTaken
#ifdef Pandas_ScriptParams_UnitData_Experience
			case UMOB_MOBBASEEXP: md->pandas.base_exp = cap_value(value64, -1, (int64)MAX_EXP); break;
			case UMOB_MOBJOBEXP: md->pandas.job_exp = cap_value(value64, -1, (int64)MAX_EXP); break;
#endif // Pandas_ScriptParams_UnitData_Experience
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_MOB.\n", type);
				return SCRIPT_CMD_FAILURE;
			}

#ifdef Pandas_Persistent_SetUnitData_For_Monster_StatusData
		// 使用 setunitdata 对此魔物进行了什么项目的修改, 都记录下来
		if (md->pandas.special_setunitdata) {
			int data_type = type;

			// UMOB_SLAVECPYMSTRMD 最终修改的还是 base_status->mode 的值
			// 因此如果设置的数据类型是 UMOB_SLAVECPYMSTRMD 则把他视为 UMOB_MODE 来处理
			if (type == UMOB_SLAVECPYMSTRMD)
				data_type = UMOB_MODE;

			(*md->pandas.special_setunitdata)[data_type] = value;
		}
#endif // Pandas_Persistent_SetUnitData_For_Monster_StatusData

			if (calc_status)
				status_calc_bl_(&md->bl, status_db.getSCB_BATTLE());
		break;

	case BL_HOM:
		if (!hd) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_HOM!\n");
			return SCRIPT_CMD_FAILURE;
		}
		switch (type) {
			case UHOM_SIZE: hd->battle_status.size = hd->base_status.size = (unsigned char)value; break;
			case UHOM_LEVEL: hd->homunculus.level = (unsigned short)value; break;
			case UHOM_HP: hd->base_status.hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); break;
			case UHOM_MAXHP: hd->base_status.hp = hd->base_status.max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); break;
			case UHOM_SP: hd->base_status.sp = (unsigned int)value; status_set_sp(bl, (unsigned int)value, 0); break;
			case UHOM_MAXSP: hd->base_status.sp = hd->base_status.max_sp = (unsigned int)value; status_set_maxsp(bl, (unsigned int)value, 0); break;
			case UHOM_MASTERCID: hd->homunculus.char_id = (uint32)value; break;
			case UHOM_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UHOM_X: if (!unit_walktoxy(bl, (short)value, hd->bl.y, 2)) unit_movepos(bl, (short)value, hd->bl.y, 0, 0); break;
			case UHOM_Y: if (!unit_walktoxy(bl, hd->bl.x, (short)value, 2)) unit_movepos(bl, hd->bl.x, (short)value, 0, 0); break;
			case UHOM_HUNGER: hd->homunculus.hunger = (short)value; clif_send_homdata(map_charid2sd(hd->homunculus.char_id), SP_HUNGRY, hd->homunculus.hunger); break;
			case UHOM_INTIMACY: hom_increase_intimacy(hd, (unsigned int)value); clif_send_homdata(map_charid2sd(hd->homunculus.char_id), SP_INTIMATE, hd->homunculus.intimacy / 100); break;
			case UHOM_SPEED: hd->base_status.speed = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UHOM_CANMOVETICK: hd->ud.canmove_tick = value > 0 ? (unsigned int)value : 0; break;
			case UHOM_STR: hd->base_status.str = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_AGI: hd->base_status.agi = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_VIT: hd->base_status.vit = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_INT: hd->base_status.int_ = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_DEX: hd->base_status.dex = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_LUK: hd->base_status.luk = (unsigned short)value; status_calc_misc(bl, &hd->base_status, hd->homunculus.level); calc_status = true; break;
			case UHOM_DMGIMMUNE: hd->ud.immune_attack = value > 0; break;
			case UHOM_ATKRANGE: hd->base_status.rhw.range = (unsigned short)value; calc_status = true; break;
			case UHOM_ATKMIN: hd->base_status.rhw.atk = (unsigned short)value; calc_status = true; break;
			case UHOM_ATKMAX: hd->base_status.rhw.atk2 = (unsigned short)value; calc_status = true; break;
			case UHOM_MATKMIN: hd->base_status.matk_min = (unsigned short)value; calc_status = true; break;
			case UHOM_MATKMAX: hd->base_status.matk_max = (unsigned short)value; calc_status = true; break;
			case UHOM_DEF: hd->base_status.def = (pec_defType)value; calc_status = true; break;
			case UHOM_MDEF: hd->base_status.mdef = (pec_defType)value; calc_status = true; break;
			case UHOM_HIT: hd->base_status.hit = (short)value; calc_status = true; break;
			case UHOM_FLEE: hd->base_status.flee = (short)value; calc_status = true; break;
			case UHOM_PDODGE: hd->base_status.flee2 = (short)value; calc_status = true; break;
			case UHOM_CRIT: hd->base_status.cri = (short)value; calc_status = true; break;
			case UHOM_RACE: hd->battle_status.race = hd->base_status.race = (unsigned char)value; break;
			case UHOM_ELETYPE: hd->base_status.def_ele = (unsigned char)value; calc_status = true; break;
			case UHOM_ELELEVEL: hd->base_status.ele_lv = (unsigned char)value; calc_status = true; break;
			case UHOM_AMOTION: hd->base_status.amotion = (short)value; calc_status = true; break;
			case UHOM_ADELAY: hd->base_status.adelay = (short)value; calc_status = true; break;
			case UHOM_DMOTION: hd->base_status.dmotion = (short)value; calc_status = true; break;
			case UHOM_TARGETID: {
				if (value==0) {
					unit_stop_attack(&hd->bl);
					break;
				}
				struct block_list* target = map_id2bl(value);
				if (!target) {
					ShowWarning("buildin_setunitdata: Error in finding target for BL_HOM!\n");
					return SCRIPT_CMD_FAILURE;
				}
				unit_attack(&hd->bl, target->id, 1);
				break;
			}
			case UHOM_GROUP_ID: hd->ud.group_id = value; unit_refresh(bl); break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UHOM_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_HOM.\n", type);
				return SCRIPT_CMD_FAILURE;
			}
			if (calc_status)
				status_calc_bl_(&hd->bl, status_db.getSCB_BATTLE());
		break;

	case BL_PET:
		if (!pd) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_PET!\n");
			return SCRIPT_CMD_FAILURE;
		}
		switch (type) {
			case UPET_SIZE: pd->status.size = (unsigned char)value; break;
			case UPET_LEVEL: pd->pet.level = (unsigned short)value; break;
			case UPET_HP: pd->status.hp = pd->status.max_hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); break;
			case UPET_MAXHP: pd->status.hp = pd->status.max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); break;
			case UPET_MASTERAID: pd->pet.account_id = (unsigned int)value; break;
			case UPET_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UPET_X: if (!unit_walktoxy(bl, (short)value, pd->bl.y, 2)) unit_movepos(bl, (short)value, md->bl.y, 0, 0); break;
			case UPET_Y: if (!unit_walktoxy(bl, pd->bl.x, (short)value, 2)) unit_movepos(bl, pd->bl.x, (short)value, 0, 0); break;
			case UPET_HUNGER: pd->pet.hungry = cap_value((short)value, 0, 100); clif_send_petdata(map_id2sd(pd->pet.account_id), pd, 2, pd->pet.hungry); break;
			case UPET_INTIMACY: pet_set_intimate(pd, (unsigned int)value); clif_send_petdata(map_id2sd(pd->pet.account_id), pd, 1, pd->pet.intimate); break;
			case UPET_SPEED: pd->status.speed = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UPET_CANMOVETICK: pd->ud.canmove_tick = value > 0 ? (unsigned int)value : 0; break;
			case UPET_STR: pd->status.str = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_AGI: pd->status.agi = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_VIT: pd->status.vit = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_INT: pd->status.int_ = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_DEX: pd->status.dex = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_LUK: pd->status.luk = (unsigned short)value; status_calc_misc(bl, &pd->status, pd->pet.level); break;
			case UPET_DMGIMMUNE: pd->ud.immune_attack = value > 0; break;
			case UPET_ATKRANGE: pd->status.rhw.range = (unsigned short)value; break;
			case UPET_ATKMIN: pd->status.rhw.atk = (unsigned short)value; break;
			case UPET_ATKMAX: pd->status.rhw.atk2 = (unsigned short)value; break;
			case UPET_MATKMIN: pd->status.matk_min = (unsigned short)value; break;
			case UPET_MATKMAX: pd->status.matk_max = (unsigned short)value; break;
			case UPET_DEF: pd->status.def = (pec_defType)value; break;
			case UPET_MDEF: pd->status.mdef = (pec_defType)value; break;
			case UPET_HIT: pd->status.hit = (short)value; break;
			case UPET_FLEE: pd->status.flee = (short)value; break;
			case UPET_PDODGE: pd->status.flee2 = (short)value; break;
			case UPET_CRIT: pd->status.cri = (short)value; break;
			case UPET_RACE: pd->status.race = (unsigned char)value; break;
			case UPET_ELETYPE: pd->status.def_ele = (unsigned char)value; break;
			case UPET_ELELEVEL: pd->status.ele_lv = (unsigned char)value; break;
			case UPET_AMOTION: pd->status.amotion = (short)value; break;
			case UPET_ADELAY: pd->status.adelay = (short)value; break;
			case UPET_DMOTION: pd->status.dmotion = (short)value; break;
			case UPET_GROUP_ID: pd->ud.group_id = value; unit_refresh(bl); break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UPET_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_PET.\n", type);
				return SCRIPT_CMD_FAILURE;
			}
		break;

	case BL_MER:
		if (!mc) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_MER!\n");
			return SCRIPT_CMD_FAILURE;
		}
		switch (type) {
			case UMER_SIZE: mc->battle_status.size = mc->base_status.size = (unsigned char)value; break;
			case UMER_HP: mc->base_status.hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); break;
			case UMER_MAXHP: mc->base_status.hp = mc->base_status.max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); break;
			case UMER_MASTERCID: mc->mercenary.char_id = (uint32)value; break;
			case UMER_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UMER_X: if (!unit_walktoxy(bl, (short)value, mc->bl.y, 2)) unit_movepos(bl, (short)value, mc->bl.y, 0, 0); break;
			case UMER_Y: if (!unit_walktoxy(bl, mc->bl.x, (short)value, 2)) unit_movepos(bl, mc->bl.x, (short)value, 0, 0); break;
			case UMER_KILLCOUNT: mc->mercenary.kill_count = (unsigned int)value; break;
			case UMER_LIFETIME: mc->mercenary.life_time = (unsigned int)value; break;
			case UMER_SPEED: mc->base_status.speed = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UMER_CANMOVETICK: mc->ud.canmove_tick = value > 0 ? (unsigned int)value : 0; break;
			case UMER_STR: mc->base_status.str = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_AGI: mc->base_status.agi = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_VIT: mc->base_status.vit = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_INT: mc->base_status.int_ = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_DEX: mc->base_status.dex = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_LUK: mc->base_status.luk = (unsigned short)value; status_calc_misc(bl, &mc->base_status, mc->db->lv); calc_status = true; break;
			case UMER_DMGIMMUNE: mc->ud.immune_attack = value > 0; break;
			case UMER_ATKRANGE: mc->base_status.rhw.range = (unsigned short)value; calc_status = true; break;
			case UMER_ATKMIN: mc->base_status.rhw.atk = (unsigned short)value; calc_status = true; break;
			case UMER_ATKMAX: mc->base_status.rhw.atk2 = (unsigned short)value; calc_status = true; break;
			case UMER_MATKMIN: mc->base_status.matk_min = (unsigned short)value; calc_status = true; break;
			case UMER_MATKMAX: mc->base_status.matk_max = (unsigned short)value; calc_status = true; break;
			case UMER_DEF: mc->base_status.def = (pec_defType)value; calc_status = true; break;
			case UMER_MDEF: mc->base_status.mdef = (pec_defType)value; calc_status = true; break;
			case UMER_HIT: mc->base_status.hit = (short)value; calc_status = true; break;
			case UMER_FLEE: mc->base_status.flee = (short)value; calc_status = true; break;
			case UMER_PDODGE: mc->base_status.flee2 = (short)value; calc_status = true; break;
			case UMER_CRIT: mc->base_status.cri = (short)value; calc_status = true; break;
			case UMER_RACE: mc->battle_status.race = mc->base_status.race = (unsigned char)value; break;
			case UMER_ELETYPE: mc->base_status.def_ele = (unsigned char)value; calc_status = true; break;
			case UMER_ELELEVEL: mc->base_status.ele_lv = (unsigned char)value; calc_status = true; break;
			case UMER_AMOTION: mc->base_status.amotion = (short)value; calc_status = true; break;
			case UMER_ADELAY: mc->base_status.adelay = (short)value; calc_status = true; break;
			case UMER_DMOTION: mc->base_status.dmotion = (short)value; calc_status = true; break;
			case UMER_TARGETID: {
				if (value==0) {
					unit_stop_attack(&mc->bl);
					break;
				}
				struct block_list* target = map_id2bl(value);
				if (!target) {
					ShowWarning("buildin_setunitdata: Error in finding target for BL_MER!\n");
					return SCRIPT_CMD_FAILURE;
				}
				unit_attack(&mc->bl, target->id, 1);
				break;
			}
			case UMER_GROUP_ID: mc->ud.group_id = value; unit_refresh(bl); break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UMER_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_MER.\n", type);
				return SCRIPT_CMD_FAILURE;
			}
			if (calc_status)
				status_calc_bl_(&mc->bl, status_db.getSCB_BATTLE());
		break;

	case BL_ELEM:
		if (!ed) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_ELEM!\n");
			return SCRIPT_CMD_FAILURE;
		}
		switch (type) {
			case UELE_SIZE: ed->battle_status.size = ed->base_status.size = (unsigned char)value; break;
			case UELE_HP: ed->base_status.hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); break;
			case UELE_MAXHP: ed->base_status.hp = ed->base_status.max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); break;
			case UELE_SP: ed->base_status.sp = (unsigned int)value; status_set_sp(bl, (unsigned int)value, 0); break;
			case UELE_MAXSP: ed->base_status.sp = ed->base_status.max_sp = (unsigned int)value; status_set_maxsp(bl, (unsigned int)value, 0); break;
			case UELE_MASTERCID: ed->elemental.char_id = (uint32)value; break;
			case UELE_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UELE_X: if (!unit_walktoxy(bl, (short)value, ed->bl.y, 2)) unit_movepos(bl, (short)value, ed->bl.y, 0, 0); break;
			case UELE_Y: if (!unit_walktoxy(bl, ed->bl.x, (short)value, 2)) unit_movepos(bl, ed->bl.x, (short)value, 0, 0); break;
			case UELE_LIFETIME: ed->elemental.life_time = (unsigned int)value; break;
			case UELE_MODE: ed->elemental.mode = (enum e_mode)value; calc_status = true; break;
			case UELE_SPEED: ed->base_status.speed = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UELE_CANMOVETICK: ed->ud.canmove_tick = value > 0 ? (unsigned int)value : 0; break;
			case UELE_STR: ed->base_status.str = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_AGI: ed->base_status.agi = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_VIT: ed->base_status.vit = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_INT: ed->base_status.int_ = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_DEX: ed->base_status.dex = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_LUK: ed->base_status.luk = (unsigned short)value; status_calc_misc(bl, &ed->base_status, ed->db->lv); calc_status = true; break;
			case UELE_DMGIMMUNE: ed->ud.immune_attack = value > 0; break;
			case UELE_ATKRANGE: ed->base_status.rhw.range = (unsigned short)value; calc_status = true; break;
			case UELE_ATKMIN: ed->base_status.rhw.atk = (unsigned short)value; calc_status = true; break;
			case UELE_ATKMAX: ed->base_status.rhw.atk2 = (unsigned short)value; calc_status = true; break;
			case UELE_MATKMIN: ed->base_status.matk_min = (unsigned short)value; calc_status = true; break;
			case UELE_MATKMAX: ed->base_status.matk_max = (unsigned short)value; calc_status = true; break;
			case UELE_DEF: ed->base_status.def = (pec_defType)value; calc_status = true; break;
			case UELE_MDEF: ed->base_status.mdef = (pec_defType)value; calc_status = true; break;
			case UELE_HIT: ed->base_status.hit = (short)value; calc_status = true; break;
			case UELE_FLEE: ed->base_status.flee = (short)value; calc_status = true; break;
			case UELE_PDODGE: ed->base_status.flee2 = (short)value; calc_status = true; break;
			case UELE_CRIT: ed->base_status.cri = (short)value; calc_status = true; break;
			case UELE_RACE: ed->battle_status.race = ed->base_status.race = (unsigned char)value; break;
			case UELE_ELETYPE: ed->base_status.def_ele = (unsigned char)value; calc_status = true; break;
			case UELE_ELELEVEL: ed->base_status.ele_lv = (unsigned char)value; calc_status = true; break;
			case UELE_AMOTION: ed->base_status.amotion = (short)value; calc_status = true; break;
			case UELE_ADELAY: ed->base_status.adelay = (short)value; calc_status = true; break;
			case UELE_DMOTION: ed->base_status.dmotion = (short)value; calc_status = true; break;
			case UELE_TARGETID: {
				if (value==0) {
					unit_stop_attack(&ed->bl);
					break;
				}
				struct block_list* target = map_id2bl(value);
				if (!target) {
					ShowWarning("buildin_setunitdata: Error in finding target for BL_ELEM!\n");
					return SCRIPT_CMD_FAILURE;
				}
				elemental_change_mode(ed, EL_MODE_AGGRESSIVE);
				unit_attack(&ed->bl, target->id, 1);
				break;
			}
			case UELE_GROUP_ID: ed->ud.group_id = value; unit_refresh(bl); break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UELE_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_ELEM.\n", type);
				return SCRIPT_CMD_FAILURE;
			}
			if (calc_status)
				status_calc_bl_(&ed->bl, status_db.getSCB_BATTLE());
		break;

	case BL_NPC:
		if (!nd) {
			ShowWarning("buildin_setunitdata: Error in finding object BL_NPC!\n");
			return SCRIPT_CMD_FAILURE;
		}
		switch (type) {
			case UNPC_LEVEL: nd->level = (unsigned int)value; break;
			case UNPC_HP: nd->status.hp = (unsigned int)value; status_set_hp(bl, (unsigned int)value, 0); break;
			case UNPC_MAXHP: nd->status.hp = nd->status.max_hp = (unsigned int)value; status_set_maxhp(bl, (unsigned int)value, 0); break;
			case UNPC_MAPID: if (mapname) value = map_mapname2mapid(mapname); unit_warp(bl, (short)value, 0, 0, CLR_TELEPORT); break;
			case UNPC_X: if (!unit_walktoxy(bl, (short)value, nd->bl.y, 2)) unit_movepos(bl, (short)value, nd->bl.x, 0, 0); break;
			case UNPC_Y: if (!unit_walktoxy(bl, nd->bl.x, (short)value, 2)) unit_movepos(bl, nd->bl.x, (short)value, 0, 0); break;
			case UNPC_LOOKDIR: unit_setdir(bl, (uint8)value); break;
			case UNPC_STR: nd->params.str = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_AGI: nd->params.agi = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_VIT: nd->params.vit = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_INT: nd->params.int_ = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_DEX: nd->params.dex = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_LUK: nd->params.luk = (unsigned short)value; status_calc_misc(bl, &nd->status, nd->level); break;
			case UNPC_PLUSALLSTAT: nd->stat_point = (unsigned int)value; break;
			case UNPC_ATKRANGE: nd->status.rhw.range = (unsigned short)value; break;
			case UNPC_ATKMIN: nd->status.rhw.atk = (unsigned short)value; break;
			case UNPC_ATKMAX: nd->status.rhw.atk2 = (unsigned short)value; break;
			case UNPC_MATKMIN: nd->status.matk_min = (unsigned short)value; break;
			case UNPC_MATKMAX: nd->status.matk_max = (unsigned short)value; break;
			case UNPC_DEF: nd->status.def = (pec_defType)value; break;
			case UNPC_MDEF: nd->status.mdef = (pec_defType)value; break;
			case UNPC_HIT: nd->status.hit = (short)value; break;
			case UNPC_FLEE: nd->status.flee = (short)value; break;
			case UNPC_PDODGE: nd->status.flee2 = (short)value; break;
			case UNPC_CRIT: nd->status.cri = (short)value; break;
			case UNPC_RACE: nd->status.race = (unsigned char)value; break;
			case UNPC_ELETYPE: nd->status.def_ele = (unsigned char)value; break;
			case UNPC_ELELEVEL: nd->status.ele_lv = (unsigned char)value; break;
			case UNPC_AMOTION: nd->status.amotion = (short)value; break;
			case UNPC_ADELAY: nd->status.adelay = (short)value; break;
			case UNPC_DMOTION: nd->status.dmotion = (short)value; break;
			case UNPC_SEX: nd->vd.sex = (char)value; unit_refresh(bl); break;
			case UNPC_CLASS: npc_setclass(nd, (short)value); break;
			case UNPC_HAIRSTYLE: clif_changelook(bl, LOOK_HAIR, (unsigned short)value); break;
			case UNPC_HAIRCOLOR: clif_changelook(bl, LOOK_HAIR_COLOR, (unsigned short)value); break;
			case UNPC_HEADBOTTOM: clif_changelook(bl, LOOK_HEAD_BOTTOM, (unsigned short)value); break;
			case UNPC_HEADMIDDLE: clif_changelook(bl, LOOK_HEAD_MID, (unsigned short)value); break;
			case UNPC_HEADTOP: clif_changelook(bl, LOOK_HEAD_TOP, (unsigned short)value); break;
			case UNPC_CLOTHCOLOR: clif_changelook(bl, LOOK_CLOTHES_COLOR, (unsigned short)value); break;
			case UNPC_SHIELD: clif_changelook(bl, LOOK_SHIELD, (unsigned short)value); break;
			case UNPC_WEAPON: clif_changelook(bl, LOOK_WEAPON, (unsigned short)value); break;
			case UNPC_ROBE: clif_changelook(bl, LOOK_ROBE, (unsigned short)value); break;
			case UNPC_BODY2: clif_changelook(bl, LOOK_BODY2, (unsigned short)value); break;
			case UNPC_DEADSIT: nd->vd.dead_sit = (char)value; unit_refresh(bl); break;
			case UNPC_GROUP_ID: nd->ud.group_id = value; unit_refresh(bl); break;
#ifdef Pandas_Struct_Unit_CommonData_Aura
			case UNPC_AURA: aura_make_effective(bl, value); break;
#endif // Pandas_Struct_Unit_CommonData_Aura
			default:
				ShowError("buildin_setunitdata: Unknown data identifier %d for BL_NPC.\n", type);
				return SCRIPT_CMD_FAILURE;
			}
		break;

	default:
		ShowWarning("buildin_setunitdata: Unknown object type!\n");
		return SCRIPT_CMD_FAILURE;
	}

	// Client information updates
	switch (bl->type) {
		case BL_HOM:
			clif_send_homdata(hd->master, SP_ACK, 0);
			break;
		case BL_PET:
			clif_send_petstatus(pd->master);
			break;
		case BL_MER:
			clif_mercenary_info(map_charid2sd(mc->mercenary.char_id));
			clif_mercenary_skillblock(map_charid2sd(mc->mercenary.char_id));
			break;
		case BL_ELEM:
			clif_elemental_info(ed->master);
			break;
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Gets the name of a bl.
/// Supported types are [MOB|HOM|PET|NPC].
/// MER and ELEM don't support custom names.
///
/// getunitname <unit id>;
BUILDIN_FUNC(getunitname)
{
	struct block_list* bl = NULL;

	if(!script_rid2bl(2,bl)){
		script_pushconststr(st, "Unknown");
		return SCRIPT_CMD_FAILURE;
	}

	script_pushstrcopy(st, status_get_name(bl));

	return SCRIPT_CMD_SUCCESS;
}

/// Changes the name of a bl.
/// Supported types are [MOB|HOM|PET].
/// For NPC see 'setnpcdisplay', MER and ELEM don't support custom names.
///
/// setunitname <unit id>,<name>;
BUILDIN_FUNC(setunitname)
{
	struct block_list* bl = NULL;
	TBL_MOB* md = NULL;
	TBL_HOM* hd = NULL;
	TBL_PET* pd = NULL;

	if(!script_rid2bl(2,bl))
	{
		script_pushconststr(st, "Unknown");
		return SCRIPT_CMD_FAILURE;
	}

	switch (bl->type) {
		case BL_MOB:  md = map_id2md(bl->id); break;
		case BL_HOM:  hd = map_id2hd(bl->id); break;
		case BL_PET:  pd = map_id2pd(bl->id); break;
		default:
			ShowWarning("buildin_setunitname: Invalid object type!\n");
			return SCRIPT_CMD_FAILURE;
	}

	switch (bl->type) {
		case BL_MOB:
			if (!md) {
				ShowWarning("buildin_setunitname: Error in finding object BL_MOB!\n");
				return SCRIPT_CMD_FAILURE;
			}
			safestrncpy(md->name, script_getstr(st, 3), NAME_LENGTH);
			break;
		case BL_HOM:
			if (!hd) {
				ShowWarning("buildin_setunitname: Error in finding object BL_HOM!\n");
				return SCRIPT_CMD_FAILURE;
			}
			safestrncpy(hd->homunculus.name, script_getstr(st, 3), NAME_LENGTH);
			break;
		case BL_PET:
			if (!pd) {
				ShowWarning("buildin_setunitname: Error in finding object BL_PET!\n");
				return SCRIPT_CMD_FAILURE;
			}
			safestrncpy(pd->pet.name, script_getstr(st, 3), NAME_LENGTH);
			break;
		default:
			ShowWarning("buildin_setunitname: Unknown object type!\n");
			return SCRIPT_CMD_FAILURE;
	}
	clif_name_area(bl); // Send update to client.

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Sets a unit's title.
 * setunittitle <GID>,<title>;
 */
BUILDIN_FUNC(setunittitle)
{
	int gid = script_getnum(st, 2);
	block_list *bl = map_id2bl(gid);

	if (bl == nullptr) {
		ShowWarning("buildin_setunittitle: Unable to find object with given game ID %d!\n", gid);
		return SCRIPT_CMD_FAILURE;
	}

	unit_data *ud = unit_bl2ud(bl);

	if (ud == nullptr) {
		ShowWarning("buildin_setunittitle: Unable to find unit_data for given game ID %d!\n", gid);
		return SCRIPT_CMD_FAILURE;
	}

	safestrncpy(ud->title, script_getstr(st, 3), NAME_LENGTH);
	clif_name_area(bl);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Gets a unit's title.
 * getunittitle <GID>;
 */
BUILDIN_FUNC(getunittitle)
{
	int gid = script_getnum(st, 2);
	block_list *bl = map_id2bl(gid);

	if (bl == nullptr) {
		ShowWarning("buildin_getunittitle: Unable to find object with given game ID %d!\n", gid);
		return SCRIPT_CMD_FAILURE;
	}

	unit_data *ud = unit_bl2ud(bl);

	if (ud == nullptr) {
		ShowWarning("buildin_getunittitle: Unable to find unit_data for given game ID %d!\n", gid);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushstrcopy(st, ud->title);
	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit walk to target position or map.
/// Returns if it was successful.
///
/// unitwalk(<unit_id>,<x>,<y>{,<event_label>}) -> <bool>
/// unitwalkto(<unit_id>,<target_id>{,<event_label>}) -> <bool>
BUILDIN_FUNC(unitwalk)
{
	struct block_list* bl;
	struct unit_data *ud = NULL;
	const char *cmd = script_getfuncname(st), *done_label = "";
	uint8 off = 5;
	
	if(!script_rid2bl(2,bl))
	{
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	ud = unit_bl2ud(bl);

	// Unit was already forced to walk.
	if (ud != nullptr && ud->state.force_walk) {
		script_pushint(st, 0);
		ShowWarning("buildin_%s: Unit has already been forced to walk and not reached it's destination yet.\n", cmd);
		return SCRIPT_CMD_FAILURE;
	}

	if (bl->type == BL_NPC) {
		if (!((TBL_NPC*)bl)->status.hp)
			status_calc_npc(((TBL_NPC*)bl), SCO_FIRST);
		else
			status_calc_npc(((TBL_NPC*)bl), SCO_NONE);
	}

	if (!strcmp(cmd,"unitwalk")) {
		int x = script_getnum(st,3);
		int y = script_getnum(st,4);

		if (script_pushint(st, unit_can_reach_pos(bl,x,y,0))) {
			if (ud != nullptr)
				ud->state.force_walk = true;
			add_timer(gettick()+50, unit_delay_walktoxy_timer, bl->id, (x<<16)|(y&0xFFFF)); // Need timer to avoid mismatches
		}
	} else {
		struct block_list* tbl = map_id2bl(script_getnum(st,3));

		if (!tbl) {
			ShowError("buildin_unitwalk: Bad target destination.\n");
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		} else if (script_pushint(st, unit_can_reach_bl(bl, tbl, distance_bl(bl, tbl)+1, 0, NULL, NULL))) {
			if (ud != nullptr)
				ud->state.force_walk = true;
			add_timer(gettick()+50, unit_delay_walktobl_timer, bl->id, tbl->id); // Need timer to avoid mismatches
		}
		off = 4;
	}

	if (ud && script_hasdata(st, off)) {
		done_label = script_getstr(st, off);
		check_event(st, done_label);
		safestrncpy(ud->walk_done_event, done_label, sizeof(ud->walk_done_event));
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Kills the unit.
///
/// unitkill <unit_id>;
BUILDIN_FUNC(unitkill)
{
	struct block_list* bl;

	if(script_rid2bl(2,bl))
		status_kill(bl);

	return SCRIPT_CMD_SUCCESS;
}

/// Warps the unit to the target position in the target map.
/// Returns if it was successful.
///
/// unitwarp(<unit_id>,"<map name>",<x>,<y>) -> <bool>
BUILDIN_FUNC(unitwarp)
{
	int map_idx;
	short x;
	short y;
	struct block_list* bl;
	const char *mapname;

	mapname = script_getstr(st, 3);
	x = (short)script_getnum(st,4);
	y = (short)script_getnum(st,5);

	if(!script_rid2bl(2,bl))
	{
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!strcmp(mapname,"this"))
		map_idx = bl?bl->m:-1;
	else
		map_idx = map_mapname2mapid(mapname);

	if (map_idx >= 0 && bl != NULL)
		script_pushint(st, unit_warp(bl,map_idx,x,y,CLR_OUTSIGHT));
	else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit attack the target.
/// If the unit is a player and <action type> is not 0, it does a continuous
/// attack instead of a single attack.
/// Returns if the request was successful.
///
/// unitattack(<unit_id>,"<target name>"{,<action type>}) -> <bool>
/// unitattack(<unit_id>,<target_id>{,<action type>}) -> <bool>
BUILDIN_FUNC(unitattack)
{
	struct block_list* unit_bl;
	struct block_list* target_bl = NULL;
	int actiontype = 0;

	if (!script_rid2bl(2,unit_bl)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_isstring(st, 3)) {
		TBL_PC* sd = map_nick2sd(script_getstr(st, 3),false);
		if( sd != NULL )
			target_bl = &sd->bl;
	} else
		target_bl = map_id2bl(script_getnum(st, 3));

	if (!target_bl) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st,4))
		actiontype = script_getnum(st,4);

	switch(unit_bl->type) {
		case BL_PC: {
			struct map_session_data* sd = (struct map_session_data*)unit_bl;

			clif_parse_ActionRequest_sub(sd, actiontype > 0 ? 0x07 : 0x00, target_bl->id, gettick());
			script_pushint(st, sd->ud.target == target_bl->id);
			return SCRIPT_CMD_SUCCESS;
		}
		case BL_MOB:
			((TBL_MOB *)unit_bl)->target_id = target_bl->id;
			break;
		case BL_PET:
			((TBL_PET *)unit_bl)->target_id = target_bl->id;
			break;
		default:
			ShowError("buildin_unitattack: Unsupported source unit type %d.\n", unit_bl->type);
			script_pushint(st, false);
			return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, unit_walktobl(unit_bl, target_bl, 65025, 2));
	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit stop attacking.
///
/// unitstopattack <unit_id>;
BUILDIN_FUNC(unitstopattack)
{
	struct block_list* bl;

	if(script_rid2bl(2,bl))
	{
		unit_stop_attack(bl);
		if (bl->type == BL_MOB)
			((TBL_MOB*)bl)->target_id = 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit stop walking.
///
/// unitstopwalk <unit_id>{,<flag>};
BUILDIN_FUNC(unitstopwalk)
{
	struct block_list* bl;
	int flag = USW_NONE;

	if (script_hasdata(st, 3))
		flag = script_getnum(st, 3);

	if(script_rid2bl(2,bl)) {
		unit_data *ud = unit_bl2ud(bl);

		if (ud != nullptr)
			ud->state.force_walk = false;

		if (unit_stop_walking(bl, flag) == 0 && flag != USW_FORCE_STOP) {
			ShowWarning("buildin_unitstopwalk: Unable to find unit or unit is not walking.\n");
			return SCRIPT_CMD_FAILURE;
		}

		return SCRIPT_CMD_SUCCESS;
	} else {
		return SCRIPT_CMD_FAILURE;
	}
}

/**
 * Makes the unit say the given message.
 *
 * unittalk <unit_id>,"<message>"{,"<flag>"};
 * @param flag: Specify target
 *   bc_area - Message is sent to players in the vicinity of the source (default).
 *   bc_self - Message is sent only to player attached.
 */
BUILDIN_FUNC(unittalk)
{
	const char* message;
	struct block_list* bl;

	message = script_getstr(st, 3);

	if(script_rid2bl(2,bl))
	{
		send_target target = AREA;
		struct StringBuf sbuf;

		if (script_hasdata(st, 4)) {
			if (script_getnum(st, 4) == BC_SELF) {
				if (map_id2sd(bl->id) == NULL) {
					ShowWarning("script: unittalk: bc_self can't be used for non-players objects.\n");
					return SCRIPT_CMD_FAILURE;
				}
				target = SELF;
			}
		}

		StringBuf_Init(&sbuf);
		StringBuf_Printf(&sbuf, "%s", message);
		clif_disp_overhead_(bl, StringBuf_Value(&sbuf), target);
		StringBuf_Destroy(&sbuf);
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit do an emotion.
///
/// unitemote <unit_id>,<emotion>;
///
/// @see ET_* in script_constants.hpp
BUILDIN_FUNC(unitemote)
{
	int emotion;
	struct block_list* bl;

	emotion = script_getnum(st,3);

	if (emotion < ET_SURPRISE || emotion >= ET_MAX) {
		ShowWarning("buildin_emotion: Unknown emotion %d (min=%d, max=%d).\n", emotion, ET_SURPRISE, (ET_MAX-1));
		return SCRIPT_CMD_FAILURE;
	}

	if (script_rid2bl(2,bl))
		clif_emotion(bl, emotion);

	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit cast the skill on the target or self if no target is specified.
///
/// unitskilluseid <unit_id>,<skill_id>,<skill_lv>{,<target_id>,<casttime>,<cancel>,<Line_ID>};
/// unitskilluseid <unit_id>,"<skill name>",<skill_lv>{,<target_id>,<casttime>,<cancel>,<Line_ID>};
BUILDIN_FUNC(unitskilluseid)
{
	int unit_id, target_id, casttime;
	uint16 skill_id, skill_lv;
	struct block_list* bl;

	unit_id  = script_getnum(st,2);
	if (script_isstring(st, 3)) {
		const char *name = script_getstr(st, 3);

		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_unitskilluseid: Invalid skill name %s passed to item bonus. Skipping.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		skill_id = script_getnum(st, 3);

		if (!skill_get_index(skill_id)) {
			ShowError("buildin_unitskilluseid: Invalid skill ID %d passed to item bonus. Skipping.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	skill_lv = script_getnum(st,4);
	target_id = ( script_hasdata(st,5) ? script_getnum(st,5) : unit_id );
	casttime = ( script_hasdata(st,6) ? script_getnum(st,6) : 0 );
	bool cancel = ( script_hasdata(st,7) ? script_getnum(st,7) > 0 : skill_get_castcancel(skill_id) );
	int msg_id = (script_hasdata(st, 8) ? script_getnum(st, 8) : 0);

	if(script_rid2bl(2,bl)){
		if (msg_id > 0) {
			if (bl->type != BL_MOB) {
				ShowError("buildin_unitskilluseid: Msg can only be used for monster.\n");
				return SCRIPT_CMD_FAILURE;
			}
			TBL_MOB* md = map_id2md(bl->id);
			if (md)
				mob_chat_display_message(*md, static_cast<uint16>(msg_id));
		}
		if (bl->type == BL_NPC) {
			if (!((TBL_NPC*)bl)->status.hp)
				status_calc_npc(((TBL_NPC*)bl), SCO_FIRST);
			else
				status_calc_npc(((TBL_NPC*)bl), SCO_NONE);
		}
		unit_skilluse_id2(bl, target_id, skill_id, skill_lv, (casttime * 1000) + skill_castfix(bl, skill_id, skill_lv), cancel);
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Makes the unit cast the skill on the target position.
///
/// unitskillusepos <unit_id>,<skill_id>,<skill_lv>,<target_x>,<target_y>{,<casttime>,<cancel>,<Line_ID>};
/// unitskillusepos <unit_id>,"<skill name>",<skill_lv>,<target_x>,<target_y>{,<casttime>,<cancel>,<Line_ID>};
BUILDIN_FUNC(unitskillusepos)
{
	int skill_x, skill_y, casttime;
	uint16 skill_id, skill_lv;
	struct block_list* bl;

	if (script_isstring(st, 3)) {
		const char *name = script_getstr(st, 3);

		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_unitskillusepos: Invalid skill name %s.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		skill_id = script_getnum(st, 3);

		if (!skill_get_index(skill_id)) {
			ShowError("buildin_unitskillusepos: Invalid skill ID %d.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	skill_lv = script_getnum(st,4);
	skill_x  = script_getnum(st,5);
	skill_y  = script_getnum(st,6);
	casttime = ( script_hasdata(st,7) ? script_getnum(st,7) : 0 );
	bool cancel = ( script_hasdata(st,8) ? script_getnum(st,8) > 0 : skill_get_castcancel(skill_id) );
	int msg_id = (script_hasdata(st, 9) ? script_getnum(st, 9) : 0);

	if(script_rid2bl(2,bl)){
		if (msg_id > 0) {
			if (bl->type != BL_MOB) {
				ShowError("buildin_unitskilluseid: Msg can only be used for monster.\n");
				return SCRIPT_CMD_FAILURE;
			}
			TBL_MOB* md = map_id2md(bl->id);
			if (md)
				mob_chat_display_message(*md, static_cast<uint16>(msg_id));
		}
		if (bl->type == BL_NPC) {
			if (!((TBL_NPC*)bl)->status.hp)
				status_calc_npc(((TBL_NPC*)bl), SCO_FIRST);
			else
				status_calc_npc(((TBL_NPC*)bl), SCO_NONE);
		}
		unit_skilluse_pos2(bl, skill_x, skill_y, skill_id, skill_lv, (casttime * 1000) + skill_castfix(bl, skill_id, skill_lv), cancel);
	}

	return SCRIPT_CMD_SUCCESS;
}

// <--- [zBuffer] List of unit control commands

/// Pauses the execution of the script, detaching the player
///
/// sleep <milli seconds>;
BUILDIN_FUNC(sleep)
{
	// First call(by function call)
	if (st->sleep.tick == 0) {
		int ticks;

		ticks = script_getnum(st, 2);

		if (ticks <= 0) {
			ShowError("buildin_sleep: negative or zero amount('%d') of milli seconds is not supported\n", ticks);
			return SCRIPT_CMD_FAILURE;
		}

		// detach the player
		script_detach_rid(st);

		// sleep for the target amount of time
		st->state = RERUNLINE;
		st->sleep.tick = ticks;
	// Second call(by timer after sleeping time is over)
	} else {		
		// Continue the script
		st->state = RUN;
		st->sleep.tick = 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Pauses the execution of the script, keeping the unit attached
/// Stops the script if no unit is attached
///
/// sleep2(<milli seconds>)
BUILDIN_FUNC(sleep2)
{
	// First call(by function call)
	if (st->sleep.tick == 0) {
		int ticks;

		ticks = script_getnum(st, 2);

		if (ticks <= 0) {
			ShowError( "buildin_sleep2: negative or zero amount('%d') of milli seconds is not supported\n", ticks );
			return SCRIPT_CMD_FAILURE;
		}

		if (map_id2bl(st->rid) == NULL) {
			ShowError( "buildin_sleep2: no unit is attached\n" );
			return SCRIPT_CMD_FAILURE;
		}
		
		// sleep for the target amount of time
		st->state = RERUNLINE;
		st->sleep.tick = ticks;
	// Second call(by timer after sleeping time is over)
	} else {		
		// Check if the unit is still attached
		// NOTE: This should never happen, since run_script_timer already checks this
		if (map_id2bl(st->rid) == NULL) {
			// The unit is not attached anymore - terminate the script
			st->rid = 0;
			st->state = END;
		} else {
			// The unit is still attached - continue the script
			st->state = RUN;
			st->sleep.tick = 0;
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Awakes all the sleep timers of the target npc
///
/// awake "<npc name>";
BUILDIN_FUNC(awake)
{
	DBIterator *iter;
	struct script_state *tst;
	struct npc_data* nd;

	if ((nd = npc_name2id(script_getstr(st, 2))) == NULL) {
		ShowError("buildin_awake: NPC \"%s\" not found\n", script_getstr(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	int rid = st->rid;

	// No need to keep the player attached if we are going to run other scripts now, where he might get attached
	if( rid ){
		script_detach_rid(st);
	}

	iter = db_iterator(st_db);

	for (tst = static_cast<script_state *>(dbi_first(iter)); dbi_exists(iter); tst = static_cast<script_state *>(dbi_next(iter))) {
		if (tst->oid == nd->bl.id) {
			if (tst->sleep.timer == INVALID_TIMER) { // already awake ???
				continue;
			}

			delete_timer(tst->sleep.timer, run_script_timer);

			// Trigger the timer function
			run_script_timer(INVALID_TIMER, gettick(), tst->sleep.charid, (intptr_t)tst);
		}
	}
	dbi_destroy(iter);

	// If a player had been attached, now is the time to restore it
	if( rid ){
		st->rid = rid;
		script_attach_state(st);
	}

	return SCRIPT_CMD_SUCCESS;
}

/// Returns a reference to a variable of the target NPC.
/// Returns 0 if an error occurs.
///
/// getvariableofnpc(<variable>, "<npc name>") -> <reference>
BUILDIN_FUNC(getvariableofnpc)
{
	struct script_data* data;
	const char* name;
	struct npc_data* nd;

	data = script_getdata(st,2);
	if( !data_isreference(data) )
	{// Not a reference (aka varaible name)
		ShowError("buildin_getvariableofnpc: not a variable\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	name = reference_getname(data);
	if( *name != '.' || name[1] == '@' )
	{// not a npc variable
		ShowError("buildin_getvariableofnpc: invalid scope (not npc variable)\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	nd = npc_name2id(script_getstr(st,3));
	if( nd == NULL || nd->subtype != NPCTYPE_SCRIPT || nd->u.scr.script == NULL )
	{// NPC not found or has no script
		ShowError("buildin_getvariableofnpc: can't find npc %s\n", script_getstr(st,3));
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if (!nd->u.scr.script->local.vars)
		nd->u.scr.script->local.vars = i64db_alloc(DB_OPT_RELEASE_DATA);

	push_val2(st->stack, C_NAME, reference_getuid(data), &nd->u.scr.script->local);
	return SCRIPT_CMD_SUCCESS;
}

/// Opens a warp portal.
/// Has no "portal opening" effect/sound, it opens the portal immediately.
///
/// warpportal <source x>,<source y>,"<target map>",<target x>,<target y>;
///
/// @author blackhole89
BUILDIN_FUNC(warpportal)
{
	int spx;
	int spy;
	unsigned short mapindex;
	int tpx;
	int tpy;
	std::shared_ptr<s_skill_unit_group> group;
	struct block_list* bl;

	bl = map_id2bl(st->oid);
	if( bl == NULL ) {
		ShowError("buildin_warpportal: NPC is needed\n");
		return SCRIPT_CMD_FAILURE;
	}

	spx = script_getnum(st,2);
	spy = script_getnum(st,3);
	mapindex = mapindex_name2id(script_getstr(st, 4));
	tpx = script_getnum(st,5);
	tpy = script_getnum(st,6);

	if( mapindex == 0 ) {
		ShowError("buildin_warpportal: Target map not found %s.\n", script_getstr(st, 4));
		return SCRIPT_CMD_FAILURE;
	}

	group = skill_unitsetting(bl, AL_WARP, 4, spx, spy, 0);
	if( group == NULL )
		return SCRIPT_CMD_FAILURE;// failed
	group->val1 = (group->val1<<16)|(short)0;
	group->val2 = (tpx<<16) | tpy;
	group->val3 = mapindex;

	return SCRIPT_CMD_SUCCESS;
}

/**
 * openmail({<char_id>});
 **/
BUILDIN_FUNC(openmail)
{
	struct map_session_data* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

#if PACKETVER < 20150513
	mail_openmail(sd);

	return SCRIPT_CMD_SUCCESS;
#else
	ShowError( "buildin_openmail: This command is not supported for PACKETVER 2015-05-13 or newer.\n" );

	return SCRIPT_CMD_FAILURE;
#endif
}

/**
 * openauction({<char_id>});
 **/
BUILDIN_FUNC(openauction)
{
	TBL_PC* sd;

	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;

	if( !battle_config.feature_auction ) {
		clif_messagecolor(&sd->bl, color_table[COLOR_RED], msg_txt(sd, 517), false, SELF);
		return SCRIPT_CMD_SUCCESS;
	}

	clif_Auction_openwindow(sd);

	return SCRIPT_CMD_SUCCESS;
}

/// Retrieves the value of the specified flag of the specified cell.
///
/// checkcell("<map name>",<x>,<y>,<type>) -> <bool>
///
/// @see cell_chk* constants in src/map/script_constants.hpp for the types
BUILDIN_FUNC(checkcell)
{
	int16 m = map_mapname2mapid(script_getstr(st,2));
	int16 x = script_getnum(st,3);
	int16 y = script_getnum(st,4);
	cell_chk type = (cell_chk)script_getnum(st,5);

	script_pushint(st, map_getcell(m, x, y, type));

	return SCRIPT_CMD_SUCCESS;
}

/// Modifies flags of cells in the specified area.
///
/// setcell "<map name>",<x1>,<y1>,<x2>,<y2>,<type>,<flag>;
///
/// @see cell_* constants in src/map/script_constants.hpp for the types
BUILDIN_FUNC(setcell)
{
	int16 m = map_mapname2mapid(script_getstr(st,2));
	int16 x1 = script_getnum(st,3);
	int16 y1 = script_getnum(st,4);
	int16 x2 = script_getnum(st,5);
	int16 y2 = script_getnum(st,6);
	cell_t type = (cell_t)script_getnum(st,7);
	bool flag = script_getnum(st,8) != 0;

	int x,y;

	if( x1 > x2 ) SWAP(x1,x2);
	if( y1 > y2 ) SWAP(y1,y2);

	for( y = y1; y <= y2; ++y )
		for( x = x1; x <= x2; ++x )
			map_setcell(m, x, y, type, flag);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Gets a free cell in the specified area.
 * getfreecell "<map name>",<rX>,<rY>{,<x>,<y>,<rangeX>,<rangeY>,<flag>};
 */
BUILDIN_FUNC(getfreecell)
{
	const char *mapn = script_getstr(st, 2), *name_x, *name_y;
	struct script_data
		*data_x = script_getdata(st, 3),
		*data_y = script_getdata(st, 4);
	struct block_list* bl = map_id2bl(st->rid);
	struct map_session_data *sd = nullptr;
	int16 m, x = 0, y = 0;
	int rx = -1, ry = -1, flag = 1;

	if (!data_isreference(data_x)) {
		ShowWarning("script: buildin_getfreecell: rX is not a variable.\n");
		return SCRIPT_CMD_FAILURE;
	}
	if (!data_isreference(data_y)) {
		ShowWarning("script: buildin_getfreecell: rY is not a variable.\n");
		return SCRIPT_CMD_FAILURE;
	}
	name_x = reference_getname(data_x),
	name_y = reference_getname(data_y);

	if (is_string_variable(name_x)) {
		ShowWarning("script: buildin_getfreecell: rX is a string, must be an INT.\n");
		return SCRIPT_CMD_FAILURE;
	}
	if (is_string_variable(name_y)) {
		ShowWarning("script: buildin_getfreecell: rY is a string, must be an INT.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if (not_server_variable(*name_x) || not_server_variable(*name_y)) {
		if (!script_rid2sd(sd)) {
			if (not_server_variable(*name_x))
				ShowError( "buildin_getfreecell: variable '%s' for mapX is not a server variable, but no player is attached!", name_x );
			else
				ShowError( "buildin_getfreecell: variable '%s' for mapY is not a server variable, but no player is attached!", name_y );
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (script_hasdata(st, 5))
		x = script_getnum(st, 5);

	if (script_hasdata(st, 6))
		y = script_getnum(st, 6);

	if (script_hasdata(st, 7))
		rx = script_getnum(st, 7);

	if (script_hasdata(st, 8))
		ry = script_getnum(st, 8);

	if (script_hasdata(st, 9))
		flag = script_getnum(st, 9);

	if (bl && strcmp(mapn, "this") == 0)
		m = bl->m;
	else
		m = map_mapname2mapid(mapn);

	map_search_freecell(bl, m, &x, &y, rx, ry, flag);

	set_reg_num(st, sd, reference_getuid(data_x), name_x, x, data_x->ref);
	set_reg_num(st, sd, reference_getuid(data_y), name_y, y, data_y->ref);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Mercenary Commands
 *------------------------------------------*/
BUILDIN_FUNC(mercenary_create)
{
	struct map_session_data *sd;
	int class_, contract_time;

	if( !script_rid2sd(sd) || sd->md || sd->status.mer_id != 0 )
		return SCRIPT_CMD_SUCCESS;

	class_ = script_getnum(st,2);

	if( !mercenary_db.exists(class_) )
		return SCRIPT_CMD_SUCCESS;

	contract_time = script_getnum(st,3);
	mercenary_create(sd, class_, contract_time);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_delete)
{
	struct map_session_data *sd;
	int type = 0;

	if( !script_charid2sd(2, sd) )
		return SCRIPT_CMD_FAILURE;

	if( sd->md == nullptr ) {
		ShowWarning("buildin_mercenary_delete: Tried to delete a non existant mercenary from player '%s' (AID: %u, CID: %u)\n", sd->status.name, sd->status.account_id, sd->status.char_id);
		return SCRIPT_CMD_FAILURE;
	}

	if( script_hasdata(st, 3) ) {
		type = script_getnum(st, 3);
		if( type < 0 || type > 3 ) {
			ShowWarning("buildin_mercenary_delete: invalid type value of %d.\n", type);
			return SCRIPT_CMD_FAILURE;
		}
	}
	
	mercenary_delete(sd->md, type);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_heal)
{
	struct map_session_data *sd;
	int hp, sp;

	if( !script_rid2sd(sd) || sd->md == NULL )
		return SCRIPT_CMD_SUCCESS;
	hp = script_getnum(st,2);
	sp = script_getnum(st,3);

	status_heal(&sd->md->bl, hp, sp, 0);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_sc_start)
{
	struct map_session_data *sd;
	enum sc_type type;
	int tick, val1;

	if( !script_rid2sd(sd) || sd->md == NULL )
		return SCRIPT_CMD_SUCCESS;

	type = (sc_type)script_getnum(st,2);
	tick = script_getnum(st,3);
	val1 = script_getnum(st,4);

	status_change_start(NULL, &sd->md->bl, type, 10000, val1, 0, 0, 0, tick, SCSTART_NOTICKDEF);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_get_calls)
{
	struct map_session_data *sd;
	int guild;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	guild = script_getnum(st,2);
	switch( guild )
	{
		case ARCH_MERC_GUILD:
			script_pushint(st,sd->status.arch_calls);
			break;
		case SPEAR_MERC_GUILD:
			script_pushint(st,sd->status.spear_calls);
			break;
		case SWORD_MERC_GUILD:
			script_pushint(st,sd->status.sword_calls);
			break;
		default:
			script_pushint(st,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_set_calls)
{
	struct map_session_data *sd;
	int guild, value, *calls;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	guild = script_getnum(st,2);
	value = script_getnum(st,3);

	switch( guild )
	{
		case ARCH_MERC_GUILD:
			calls = &sd->status.arch_calls;
			break;
		case SPEAR_MERC_GUILD:
			calls = &sd->status.spear_calls;
			break;
		case SWORD_MERC_GUILD:
			calls = &sd->status.sword_calls;
			break;
		default:
			ShowError("buildin_mercenary_set_calls: Invalid guild '%d'.\n", guild);
			return SCRIPT_CMD_SUCCESS; // Invalid Guild
	}

	*calls += value;
	*calls = cap_value(*calls, 0, INT_MAX);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_get_faith)
{
	struct map_session_data *sd;
	int guild;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	guild = script_getnum(st,2);
	switch( guild )
	{
		case ARCH_MERC_GUILD:
			script_pushint(st,sd->status.arch_faith);
			break;
		case SPEAR_MERC_GUILD:
			script_pushint(st,sd->status.spear_faith);
			break;
		case SWORD_MERC_GUILD:
			script_pushint(st,sd->status.sword_faith);
			break;
		default:
			script_pushint(st,0);
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(mercenary_set_faith)
{
	struct map_session_data *sd;
	int guild, value, *calls;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	guild = script_getnum(st,2);
	value = script_getnum(st,3);

	switch( guild )
	{
		case ARCH_MERC_GUILD:
			calls = &sd->status.arch_faith;
			break;
		case SPEAR_MERC_GUILD:
			calls = &sd->status.spear_faith;
			break;
		case SWORD_MERC_GUILD:
			calls = &sd->status.sword_faith;
			break;
		default:
			ShowError("buildin_mercenary_set_faith: Invalid guild '%d'.\n", guild);
			return SCRIPT_CMD_SUCCESS; // Invalid Guild
	}

	*calls += value;
	*calls = cap_value(*calls, 0, INT_MAX);
	if( mercenary_get_guild(sd->md) == guild )
		clif_mercenary_updatestatus(sd,SP_MERCFAITH);

	return SCRIPT_CMD_SUCCESS;
}

/*------------------------------------------
 * Book Reading
 *------------------------------------------*/
BUILDIN_FUNC(readbook)
{
	struct map_session_data *sd;
	int book_id, page;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	book_id = script_getnum(st,2);
	page = script_getnum(st,3);

	clif_readbook(sd->fd, book_id, page);
	return SCRIPT_CMD_SUCCESS;
}

/******************
Questlog script commands
*******************/

/// questinfo <Icon>{,<Map Mark Color>{,<condition>}};
BUILDIN_FUNC(questinfo)
{
	TBL_NPC* nd = map_id2nd(st->oid);

	if (!nd || nd->bl.m == -1) {
		ShowError("buildin_questinfo: No NPC attached.\n");
		return SCRIPT_CMD_FAILURE;
	}

	int icon = script_getnum(st, 2);

#if PACKETVER >= 20120410
	switch(icon){
		case QTYPE_QUEST:
		case QTYPE_QUEST2:
		case QTYPE_JOB:
		case QTYPE_JOB2:
		case QTYPE_EVENT:
		case QTYPE_EVENT2:
		// Warg icons were replaced in this client
#if PACKETVER < 20170315
		case QTYPE_WARG:
		case QTYPE_WARG2:
#else
		case QTYPE_CLICKME:
		case QTYPE_DAILYQUEST:
		case QTYPE_EVENT3:
		case QTYPE_JOBQUEST:
		case QTYPE_JUMPING_PORING:
#endif
			// Leave everything as it is
			break;
		case QTYPE_NONE:
		default:
			// Default to nothing if icon id is invalid.
			icon = QTYPE_NONE;
			break;
	}
#else
	if (icon < QTYPE_QUEST || icon > 7) // TODO: check why 7 and not QTYPE_WARG, might be related to icon + 1 below
		icon = QTYPE_QUEST;
	else
		icon = icon + 1;
#endif

	int color = QMARK_NONE;

	if (script_hasdata(st, 3)) {
		color = script_getnum(st, 3);
		if (color < QMARK_NONE || color >= QMARK_MAX) {
			ShowWarning("buildin_questinfo: invalid color '%d', defaulting to QMARK_NONE.\n",color);
			script_reportfunc(st);
			color = QMARK_NONE;
		}
	}

	struct script_code *script = nullptr;

	if (script_hasdata(st, 4)) {
		const char *str = script_getstr(st, 4);

		if (str) {
			std::string condition(str);

			if (condition.find( "achievement_condition" ) == std::string::npos)
				condition = "achievement_condition( " + condition + " );";

			script = parse_script(condition.c_str(), "questinfoparsing", 0, SCRIPT_IGNORE_EXTERNAL_BRACKETS);
			if (!script) {
				st->state = END;
				return SCRIPT_CMD_FAILURE;
			}
		}
	}

	std::shared_ptr<s_questinfo> qi = std::make_shared<s_questinfo>();

	qi->icon = static_cast<e_questinfo_types>(icon);
	qi->color = static_cast<e_questinfo_markcolor>(color);
	qi->condition = script;

	nd->qi_data.push_back(qi);

	struct map_data *mapdata = map_getmapdata(nd->bl.m);

	if (mapdata && !util::vector_exists(mapdata->qi_npc, nd->bl.id))
		mapdata->qi_npc.push_back(nd->bl.id);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * questinfo_refresh {<char_id>};
 **/
BUILDIN_FUNC(questinfo_refresh)
{
	struct map_session_data *sd;

	if (!script_charid2sd(2, sd))
		return SCRIPT_CMD_FAILURE;

	pc_show_questinfo(sd); 
	return SCRIPT_CMD_SUCCESS;
}

/**
 * setquest <ID>{,<char_id>};
 **/
BUILDIN_FUNC(setquest)
{
	struct map_session_data *sd;
	int quest_id;

	quest_id = script_getnum(st, 2);

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if( quest_add(sd, quest_id)  == -1 ){
		script_reportsrc(st);
		script_reportfunc(st);
	}

	//20120410 or 20090218 ? no reason that shouldn't work for 2009
	pc_show_questinfo(sd); 
	return SCRIPT_CMD_SUCCESS;
}

/**
 * erasequest <ID>{,<char_id>};
 **/
BUILDIN_FUNC(erasequest)
{
	struct map_session_data *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if( quest_delete(sd, script_getnum(st, 2))  == -1 ){
		script_reportsrc(st);
		script_reportfunc(st);
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * completequest <ID>{,<char_id>};
 **/
BUILDIN_FUNC(completequest)
{
	struct map_session_data *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	quest_update_status(sd, script_getnum(st, 2), Q_COMPLETE);
	//20120410 or 20090218
	pc_show_questinfo(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * changequest <ID>,<ID2>{,<char_id>};
 **/
BUILDIN_FUNC(changequest)
{
	struct map_session_data *sd;
	
	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	if( quest_change(sd, script_getnum(st, 2),script_getnum(st, 3)) == -1 ){
		script_reportsrc(st);
		script_reportfunc(st);
	}

	//20120410 or 20090218
	pc_show_questinfo(sd);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * checkquest(<ID>{,PLAYTIME|HUNTING{,<char_id>}})
 **/
BUILDIN_FUNC(checkquest)
{
	struct map_session_data *sd;
	e_quest_check_type type = HAVEQUEST;

	if( script_hasdata(st, 3) )
		type = (e_quest_check_type)script_getnum(st, 3);

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	script_pushint(st, quest_check(sd, script_getnum(st, 2), type));

	return SCRIPT_CMD_SUCCESS;
}

/**
 * isbegin_quest(<ID>{,<char_id>})
 **/
BUILDIN_FUNC(isbegin_quest)
{
	struct map_session_data *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	int i = quest_check(sd, script_getnum(st, 2), HAVEQUEST);
	script_pushint(st, i + (i < 1));

	return SCRIPT_CMD_SUCCESS;
}

/**
 * showevent <icon>{,<mark color>{,<char_id>}}
 **/
BUILDIN_FUNC(showevent)
{
	TBL_PC *sd;

	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_FAILURE;

	struct npc_data *nd = map_id2nd(st->oid);

	if (!nd)
		return SCRIPT_CMD_SUCCESS;

	int color = QMARK_NONE;
	int icon = script_getnum(st, 2);

	if (script_hasdata(st, 3)) {
		color = script_getnum(st, 3);
		if (color < QMARK_NONE || color >= QMARK_MAX) {
			ShowWarning("buildin_showevent: Invalid color '%d', defaulting to QMARK_NONE.\n",color);
			script_reportfunc(st);
			color = QMARK_NONE;
		}
	}

#if PACKETVER >= 20120410
	if (icon < 0 || (icon > 8 && icon != QTYPE_NONE) || icon == 7)
		icon = QTYPE_NONE; // Default to nothing if icon id is invalid.
#else
	if (icon < 0 || icon > 7)
		icon = 0;
	else
		icon = icon + 1;
#endif

	clif_quest_show_event(sd, &nd->bl, static_cast<e_questinfo_types>(icon), static_cast<e_questinfo_markcolor>(color));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * BattleGround System
 *------------------------------------------*/
BUILDIN_FUNC(waitingroom2bg)
{
	struct npc_data *nd;
	struct chat_data *cd;
	const char *map_name;
	int mapindex = 0, bg_id;
	unsigned char i,c=0;
	struct s_battleground_team team;

	if( script_hasdata(st,7) )
		nd = npc_name2id(script_getstr(st,7));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if( nd == NULL || (cd = (struct chat_data *)map_id2bl(nd->chat_id)) == NULL )
	{
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	map_name = script_getstr(st,2);
	if (strcmp(map_name, "-") != 0 && (mapindex = mapindex_name2id(map_name)) == 0)
	{ // Invalid Map
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	team.warp_x = script_getnum(st,3);
	team.warp_y = script_getnum(st,4);
	if (script_hasdata(st,5)) {
		team.quit_event = script_getstr(st,5); // Logout Event
		check_event(st, team.quit_event.c_str());
	} else
		team.quit_event = "";
	if (script_hasdata(st,6)) {
		team.death_event = script_getstr(st,6); // Die Event
		check_event(st, team.death_event.c_str());
	} else
		team.death_event = "";

	if( (bg_id = bg_create(mapindex, &team)) == 0 )
	{ // Creation failed
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	for (i = 0; i < cd->users; i++) { // Only add those who are in the chat room
		struct map_session_data *sd;
		if( (sd = cd->usersd[i]) != NULL && bg_team_join(bg_id, sd, false) ){
			mapreg_setreg(reference_uid(add_str("$@arenamembers"), c), sd->bl.id);
			++c;
		}
	}

	mapreg_setreg(add_str("$@arenamembersnum"), c);
	script_pushint(st,bg_id);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(waitingroom2bg_single)
{
	const char* map_name;
	struct npc_data *nd;
	struct chat_data *cd;
	struct map_session_data *sd;
	int x, y, mapindex, bg_id = script_getnum(st,2);
	std::shared_ptr<s_battleground_data> bg = util::umap_find(bg_team_db, bg_id);

	if (!bg) {
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}
	if (script_hasdata(st, 3)) {
		map_name = script_getstr(st, 3);
		if ((mapindex = mapindex_name2id(map_name)) == 0) {
			script_pushint(st, false);
			return SCRIPT_CMD_SUCCESS; // Invalid Map
		}
		x = script_getnum(st, 4);
		y = script_getnum(st, 5);
	}
	else {
		mapindex = bg->cemetery.map;
		x = bg->cemetery.x;
		y = bg->cemetery.y;
	}
	if (!map_getmapflag(map_mapindex2mapid(mapindex), MF_BATTLEGROUND)) {
		ShowWarning("buildin_waitingroom2bg_single: Map %s requires the mapflag MF_BATTLEGROUND.\n", mapindex_id2name(mapindex));
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	nd = npc_name2id(script_getstr(st,6));

	if( nd == NULL || (cd = (struct chat_data *)map_id2bl(nd->chat_id)) == NULL || cd->users <= 0 )
		return SCRIPT_CMD_SUCCESS;

	if( (sd = cd->usersd[0]) == NULL )
		return SCRIPT_CMD_SUCCESS;

	if( bg_team_join(bg_id, sd, false) && pc_setpos(sd, mapindex, x, y, CLR_TELEPORT) == SETPOS_OK)
	{
		script_pushint(st, true);
	}
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}


/// Creates an instance of battleground battle group.
/// *bg_create("<map name>",<x>,<y>{,"<On Quit Event>","<On Death Event>"});
/// @author [secretdataz]
BUILDIN_FUNC(bg_create) {
	const char *map_name;
	int mapindex = 0;
	struct s_battleground_team team;

	map_name = script_getstr(st, 2);
	if (strcmp(map_name, "-") != 0 && (mapindex = mapindex_name2id(map_name)) == 0)
	{ // Invalid Map
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	team.warp_x = script_getnum(st,3);
	team.warp_y = script_getnum(st,4);
	if (script_hasdata(st,5)) {
		team.quit_event = script_getstr(st,5); // Logout Event
		check_event(st, team.quit_event.c_str());
	} else
		team.quit_event = "";
	if (script_hasdata(st,6)) {
		team.death_event = script_getstr(st,6); // Die Event
		check_event(st, team.death_event.c_str());
	} else
		team.death_event = "";

	script_pushint(st, bg_create(mapindex, &team));
	return SCRIPT_CMD_SUCCESS;
}

/// Adds attached player or <char id> (if specified) to an existing 
/// battleground group and warps it to the specified coordinates on
/// the given map.
/// bg_join(<battle group>,{"<map name>",<x>,<y>{,<char id>}});
/// @author [secretdataz]
BUILDIN_FUNC(bg_join) {
	const char* map_name;
	struct map_session_data *sd;
	int x, y, mapindex, bg_id = script_getnum(st, 2);
	std::shared_ptr<s_battleground_data> bg = util::umap_find(bg_team_db, bg_id);

	if (!bg) {
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}
	if (script_hasdata(st, 3)) {
		map_name = script_getstr(st, 3);
		if ((mapindex = mapindex_name2id(map_name)) == 0) {
			script_pushint(st, false);
			return SCRIPT_CMD_SUCCESS; // Invalid Map
		}
		x = script_getnum(st, 4);
		y = script_getnum(st, 5);
	} else {
		mapindex = bg->cemetery.map;
		x = bg->cemetery.x;
		y = bg->cemetery.y;
	}

	if (!script_charid2sd(6, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}
	if (!map_getmapflag(map_mapindex2mapid(mapindex), MF_BATTLEGROUND)) {
		ShowWarning("buildin_bg_join: Map %s requires the mapflag MF_BATTLEGROUND.\n", mapindex_id2name(mapindex));
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (bg_team_join(bg_id, sd, false) && pc_setpos(sd, mapindex, x, y, CLR_TELEPORT) == SETPOS_OK)
	{
		script_pushint(st, true);
	}
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_team_setxy)
{
	int bg_id = script_getnum(st,2);
	std::shared_ptr<s_battleground_data> bg = util::umap_find(bg_team_db, bg_id);

	if (bg) {
		bg->cemetery.x = script_getnum(st, 3);
		bg->cemetery.y = script_getnum(st, 4);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_warp)
{
	int x, y, mapindex, bg_id;
	const char* map_name;

	bg_id = script_getnum(st,2);
	map_name = script_getstr(st,3);
	if( (mapindex = mapindex_name2id(map_name)) == 0 )
		return SCRIPT_CMD_SUCCESS; // Invalid Map
	x = script_getnum(st,4);
	y = script_getnum(st,5);
	bg_team_warp(bg_id, mapindex, x, y);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_monster)
{
	int class_ = 0, x = 0, y = 0, bg_id = 0;
	const char *str,*mapname, *evt="";

	bg_id  = script_getnum(st,2);
	mapname    = script_getstr(st,3);
	x      = script_getnum(st,4);
	y      = script_getnum(st,5);
	str    = script_getstr(st,6);
	class_ = script_getnum(st,7);
	if( script_hasdata(st,8) ) evt = script_getstr(st,8);
	check_event(st, evt);
	script_pushint(st, mob_spawn_bg(mapname,x,y,str,class_,evt,bg_id));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_monster_set_team)
{
	struct mob_data *md;
	struct block_list *mbl;
	int id = script_getnum(st,2),
		bg_id = script_getnum(st,3);

	if( id == 0 || (mbl = map_id2bl(id)) == NULL || mbl->type != BL_MOB )
		return SCRIPT_CMD_SUCCESS;
	md = (TBL_MOB *)mbl;
	md->bg_id = bg_id;

	mob_stop_attack(md);
	mob_stop_walking(md, 0);
	md->target_id = md->attacked_id = 0;
	clif_name_area(&md->bl);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_leave)
{
	struct map_session_data *sd = NULL;
	bool deserter = false;

	if( !script_charid2sd(2,sd) || !sd->bg_id )
		return SCRIPT_CMD_SUCCESS;

	if (!strcmp(script_getfuncname(st), "bg_desert"))
		deserter = true;

	bg_team_leave(sd, false, deserter);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_destroy)
{
	int bg_id = script_getnum(st,2);
	bg_team_delete(bg_id);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_getareausers)
{
	const char *str = script_getstr(st, 3);
	int16 m, x0, y0, x1, y1;
	int bg_id = script_getnum(st, 2), c = 0;
	std::shared_ptr<s_battleground_data> bg = util::umap_find(bg_team_db, bg_id);

	if (!bg || (m = map_mapname2mapid(str)) < 0) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	x0 = script_getnum(st,4);
	y0 = script_getnum(st,5);
	x1 = script_getnum(st,6);
	y1 = script_getnum(st,7);

	for (const auto &member : bg->members) {
		if( member.sd->bl.m != m || member.sd->bl.x < x0 || member.sd->bl.y < y0 || member.sd->bl.x > x1 || member.sd->bl.y > y1 )
			continue;
		c++;
	}

	script_pushint(st,c);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_updatescore)
{
	const char *str;
	int16 m;

	str = script_getstr(st,2);
	if( (m = map_mapname2mapid(str)) < 0 )
		return SCRIPT_CMD_SUCCESS;

	struct map_data *mapdata = map_getmapdata(m);

	mapdata->bgscore_lion = script_getnum(st,3);
	mapdata->bgscore_eagle = script_getnum(st,4);

	clif_bg_updatescore(m);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(bg_get_data)
{
	int bg_id = script_getnum(st,2), type = script_getnum(st,3), i = 0;
	std::shared_ptr<s_battleground_data> bg = util::umap_find(bg_team_db, bg_id);

	if (bg) {
		switch (type) {
		case 0:
			script_pushint(st, bg->members.size());
			break;
		case 1:
			for (const auto &member : bg->members)
				mapreg_setreg(reference_uid(add_str("$@arenamembers"), i++), member.sd->bl.id);
			mapreg_setreg(add_str("$@arenamemberscount"), i);
			script_pushint(st, i);
			break;
		default:
			ShowError("script:bg_get_data: unknown data identifier %d\n", type);
			break;
		}
	} else
		script_pushint(st, 0);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Reserves a slot for the given Battleground.
 * bg_reserve("<battleground_map_name>"{,<ended>});
 */
BUILDIN_FUNC(bg_reserve)
{
	const char *str = script_getstr(st, 2);
	bool ended = script_hasdata(st, 3) ? script_getnum(st, 3) != 0 : false;

	if (!bg_queue_reserve(str, ended))
		ShowWarning("buildin_bg_reserve: Could not reserve battleground with name %s\n", str);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Removes a spot for the given Battleground.
 * bg_unbook("<battleground_map_name>");
 */
BUILDIN_FUNC(bg_unbook)
{
	const char *str = script_getstr(st, 2);

	if (!bg_queue_unbook(str))
		ShowWarning("buildin_bg_unbook: Could not unreserve battleground with name %s\n", str);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Gets battleground database information.
 * bg_info("<battleground name>", <type>);
 */
BUILDIN_FUNC(bg_info)
{
	std::shared_ptr<s_battleground_type> bg = bg_search_name(script_getstr(st, 2));

	if (!bg) {
		ShowError("bg_info: Invalid Battleground name %s.\n", script_getstr(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	int type = script_getnum(st, 3);

	switch (type) {
		case BG_INFO_ID:
			script_pushint(st, bg->id);
			break;
		case BG_INFO_REQUIRED_PLAYERS:
			script_pushint(st, bg->required_players);
			break;
		case BG_INFO_MAX_PLAYERS:
			script_pushint(st, bg->max_players);
			break;
		case BG_INFO_MIN_LEVEL:
			script_pushint(st, bg->min_lvl);
			break;
		case BG_INFO_MAX_LEVEL:
			script_pushint(st, bg->max_lvl);
			break;
		case BG_INFO_MAPS: {
			size_t i;

			for (i = 0; i < bg->maps.size(); i++)
				setd_sub_str(st, nullptr, ".@bgmaps$", i, mapindex_id2name(bg->maps[i].mapindex), nullptr);
			setd_sub_num(st, nullptr, ".@bgmapscount", 0, i, nullptr);
			script_pushint(st, i);
			break;
		}
		case BG_INFO_DESERTER_TIME:
			script_pushint(st, bg->deserter_time);
			break;
		default:
			ShowError("bg_info: Unknown battleground info type %d given.\n", type);
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Instancing System
 *------------------------------------------*/
/**
 * Returns an Instance ID.
 * @param st: Script state
 * @param mode: Instance mode
 * @return instance ID on success or 0 otherwise
 */
int script_instancegetid(struct script_state* st, e_instance_mode mode)
{
#ifdef Pandas_Crashfix_FunctionParams_Verify
	if (!st) return 0;
#endif // Pandas_Crashfix_FunctionParams_Verify

	int instance_id = 0;

	if (mode == IM_NONE) {
		struct npc_data *nd = map_id2nd(st->oid);

#ifndef Pandas_Crashfix_Prevent_NullPointer
		if (nd->instance_id > 0)
#else
		// 此处必须对 nd 进行空指针校验.
		// 若副本中的 NPC 在调用了 instance_destroy 销毁自己所在的副本之后,
		// 还在后续的脚本中还企图调用与副本相关的指令时 (比如 '开头的副本变量, instance_ 开头的一系列脚本指令等),
		// 那么对应的 NPC 早已经在调用 instance_destroy 的时候被销毁了, 不加以判断将引发空指针崩溃.
		// 重现脚本: https://github.com/PandasWS/Pandas/issues/386
		if (nd && nd->instance_id > 0)
#endif // Pandas_Crashfix_Prevent_NullPointer
			instance_id = nd->instance_id;
	} else {
		struct map_session_data *sd = map_id2sd(st->rid);

		if (sd) {
			switch (mode) {
				case IM_CHAR:
					if (sd->instance_id > 0)
						instance_id = sd->instance_id;
					break;
				case IM_PARTY: {
					struct party_data *pd = party_search(sd->status.party_id);

					if (pd && pd->instance_id > 0)
						instance_id = pd->instance_id;
				}
					break;
				case IM_GUILD: {
					struct guild *gd = guild_search(sd->status.guild_id);

					if (gd && gd->instance_id > 0)
						instance_id = gd->instance_id;
				}
					break;
				case IM_CLAN: {
					struct clan *cd = clan_search(sd->status.clan_id);

					if (cd && cd->instance_id > 0)
						instance_id = cd->instance_id;
				}
					break;
				default: // Unsupported type
					break;
			}
		}
	}

	return instance_id;
}

/*==========================================
 * Creates the instance
 * Returns Instance ID if created successfully
 *------------------------------------------*/
BUILDIN_FUNC(instance_create)
{
	e_instance_mode mode = IM_PARTY;
	int owner_id = 0;

	if (script_hasdata(st, 3)) {
		mode = static_cast<e_instance_mode>(script_getnum(st, 3));

		if (mode < IM_NONE || mode >= IM_MAX) {
			ShowError("buildin_instance_create: Unknown instance mode %d for '%s'\n", mode, script_getstr(st, 2));
			return SCRIPT_CMD_FAILURE;
		}
	}
	if (script_hasdata(st, 4))
		owner_id = script_getnum(st, 4);
	else {
		// If sd is NULL, instance_create will return -2.
		struct map_session_data *sd = NULL;

		switch(mode) {
			case IM_NONE:
				owner_id = st->oid;
				break;
			case IM_CHAR:
				if (script_rid2sd(sd))
					owner_id = sd->status.char_id;
				break;
			case IM_PARTY:
				if (script_rid2sd(sd))
					owner_id = sd->status.party_id;
				break;
			case IM_GUILD:
				if (script_rid2sd(sd))
					owner_id = sd->status.guild_id;
				break;
			case IM_CLAN:
				if (script_rid2sd(sd))
					owner_id = sd->status.clan_id;
				break;
			default:
				ShowError("buildin_instance_create: Invalid instance mode (instance name: %s)\n", script_getstr(st, 2));
				return SCRIPT_CMD_FAILURE;
		}
	}

	script_pushint(st, instance_create(owner_id, script_getstr(st, 2), mode));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Destroys an instance (unofficial)
 * Officially instances are only destroyed by timeout
 *
 * instance_destroy {<instance_id>};
 *------------------------------------------*/
BUILDIN_FUNC(instance_destroy)
{
	int instance_id;

	if( script_hasdata(st,2) )
		instance_id = script_getnum(st,2);
	else
		instance_id = script_instancegetid(st);

	if( instance_id == 0 ) {
		ShowError("buildin_instance_destroy: Trying to destroy invalid instance %d.\n", instance_id);
		return SCRIPT_CMD_FAILURE;
	}

	instance_destroy(instance_id);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Warps player to instance
 * Results:
 *	IE_OK: Success
 *	IE_NOMEMBER: Character not in party/guild (for party/guild type instances)
 *	IE_NOINSTANCE: Character/Party/Guild doesn't have instance
 *	IE_OTHER: Other errors (instance not in DB, instance doesn't match with character/party/guild, etc.)
 *------------------------------------------*/
BUILDIN_FUNC(instance_enter)
{
	struct map_session_data *sd = NULL;
	int x = script_hasdata(st,3) ? script_getnum(st, 3) : -1;
	int y = script_hasdata(st,4) ? script_getnum(st, 4) : -1;
	int instance_id;

	if (script_hasdata(st, 6))
		instance_id = script_getnum(st, 6);
	else
		instance_id = script_instancegetid(st, IM_PARTY);

	if (!script_charid2sd(5,sd))
		return SCRIPT_CMD_FAILURE;

	script_pushint(st, instance_enter(sd, instance_id, script_getstr(st, 2), x, y));

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns the name of a duplicated NPC
 *
 * instance_npcname <npc_name>{,<instance_id>};
 * <npc_name> is the full name of an NPC.
 *------------------------------------------*/
BUILDIN_FUNC(instance_npcname)
{
	const char *str;
	int instance_id;
	struct npc_data *nd;

	str = script_getstr(st,2);
	if( script_hasdata(st,3) )
		instance_id = script_getnum(st,3);
	else
		instance_id = script_instancegetid(st);

	if( instance_id > 0 && (nd = npc_name2id(str)) != NULL ) {
		static char npcname[NAME_LENGTH];
		snprintf(npcname, sizeof(npcname), "dup_%d_%d", instance_id, nd->bl.id);
		script_pushconststr(st,npcname);
	} else {
		ShowError("buildin_instance_npcname: Invalid instance NPC (instance_id: %d, NPC name: \"%s\".)\n", instance_id, str);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns the name of a duplicated map
 *
 * instance_mapname <map_name>{,<instance_id>};
 *------------------------------------------*/
BUILDIN_FUNC(instance_mapname)
{
 	const char *str;
	int16 m;
	int instance_id;

	str = script_getstr(st,2);

	if( script_hasdata(st,3) )
		instance_id = script_getnum(st,3);
	else
		instance_id = script_instancegetid(st);

	// Check that instance mapname is a valid map
	if(instance_id <= 0 || (m = instance_mapid(map_mapname2mapid(str), instance_id)) < 0)
		script_pushconststr(st, "");
	else
		script_pushconststr(st, map_getmapdata(m)->name);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Returns an Instance ID
 *------------------------------------------*/
BUILDIN_FUNC(instance_id)
{
	int mode = IM_NONE; // Default to the attached NPC

	if (script_hasdata(st, 2)) {
		mode = script_getnum(st, 2);

		if (mode <= IM_NONE || mode >= IM_MAX) {
			ShowError("buildin_instance_id: Unknown instance mode %d.\n", mode);
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	script_pushint(st, script_instancegetid(st, static_cast<e_instance_mode>(mode)));
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Warps all players inside an instance
 *
 * instance_warpall <map_name>,<x>,<y>{,<instance_id>};
 *------------------------------------------*/
static int buildin_instance_warpall_sub(struct block_list *bl, va_list ap)
{
	unsigned int m = va_arg(ap,unsigned int);
	int x = va_arg(ap,int);
	int y = va_arg(ap,int);
	int instance_id = va_arg(ap, int);
	struct map_session_data *sd;

	nullpo_retr(0, bl);

	if (bl->type != BL_PC)
		return 0;

	sd = (TBL_PC *)bl;

	std::shared_ptr<s_instance_data> idata = util::umap_find(instances, instance_id);

	if (!idata)
		return 0;

	int owner_id = idata->owner_id;

	switch(idata->mode) {
		case IM_NONE:
			break;
		case IM_CHAR:
			if (sd->status.char_id != owner_id)
				return 0;
			break;
		case IM_PARTY:
			if (sd->status.party_id != owner_id)
				return 0;
			break;
		case IM_GUILD:
			if (sd->status.guild_id != owner_id)
				return 0;
		case IM_CLAN:
			if (sd->status.clan_id != owner_id)
				return 0;
	}

#ifdef Pandas_Support_Transfer_Autotrade_Player
	pc_mark_multitransfer(sd);
#endif // Pandas_Support_Transfer_Autotrade_Player
	pc_setpos(sd, m, x, y, CLR_TELEPORT);

	return 1;
}

BUILDIN_FUNC(instance_warpall)
{
	int16 m;
	int instance_id;
	const char *mapn;
	int x, y;

	mapn = script_getstr(st,2);
	x    = script_getnum(st,3);
	y    = script_getnum(st,4);
	if( script_hasdata(st,5) )
		instance_id = script_getnum(st,5);
	else
		instance_id = script_instancegetid(st, IM_PARTY);

	if( instance_id <= 0 || (m = map_mapname2mapid(mapn)) < 0 || (m = instance_mapid(m, instance_id)) < 0)
		return SCRIPT_CMD_FAILURE;

	std::shared_ptr<s_instance_data> idata = util::umap_find(instances, instance_id);

	if (!idata) {
		ShowError("buildin_instance_warpall: Instance is not found.\n");
		return SCRIPT_CMD_FAILURE;
	}

	for(const auto &it : idata->map)
		map_foreachinmap(buildin_instance_warpall_sub, it.m, BL_PC, map_id2index(m), x, y, instance_id);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Broadcasts to all maps inside an instance
 *
 * instance_announce <instance id>,"<text>",<flag>{,<fontColor>{,<fontType>{,<fontSize>{,<fontAlign>{,<fontY>}}}}};
 * Using 0 for <instance id> will auto-detect the id.
 *------------------------------------------*/
BUILDIN_FUNC(instance_announce) {
	int instance_id            = script_getnum(st,2);
	const char     *mes        = script_getstr(st,3);
	int            flag        = script_getnum(st,4);
	const char     *fontColor  = script_hasdata(st,5) ? script_getstr(st,5) : NULL;
	int            fontType    = script_hasdata(st,6) ? script_getnum(st,6) : FW_NORMAL; // default fontType
	int            fontSize    = script_hasdata(st,7) ? script_getnum(st,7) : 12;    // default fontSize
	int            fontAlign   = script_hasdata(st,8) ? script_getnum(st,8) : 0;     // default fontAlign
	int            fontY       = script_hasdata(st,9) ? script_getnum(st,9) : 0;     // default fontY

	if (instance_id <= 0)
		instance_id = script_instancegetid(st);

	std::shared_ptr<s_instance_data> idata = util::umap_find(instances, instance_id);

	if (instance_id <= 0 || !idata) {
		ShowError("buildin_instance_announce: Instance not found.\n");
		return SCRIPT_CMD_FAILURE;
	}

	for (const auto &it : idata->map)
		map_foreachinmap(buildin_announce_sub, it.m, BL_PC, mes, strlen(mes)+1, flag&BC_COLOR_MASK, fontColor, fontType, fontSize, fontAlign, fontY);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * instance_check_party [malufett]
 * Values:
 * party_id : Party ID of the invoking character. [Required Parameter]
 * amount : Amount of needed Partymembers for the Instance. [Optional Parameter]
 * min : Minimum Level needed to join the Instance. [Optional Parameter]
 * max : Maxium Level allowed to join the Instance. [Optional Parameter]
 * Example: instance_check_party (getcharid(1){,amount}{,min}{,max});
 * Example 2: instance_check_party (getcharid(1),1,1,99);
 *------------------------------------------*/
BUILDIN_FUNC(instance_check_party)
{
	int amount, min, max, i, party_id, c = 0;
	struct party_data *p;

	amount = script_hasdata(st,3) ? script_getnum(st,3) : 1; // Amount of needed Partymembers for the Instance.
	min = script_hasdata(st,4) ? script_getnum(st,4) : 1; // Minimum Level needed to join the Instance.
	max  = script_hasdata(st,5) ? script_getnum(st,5) : MAX_LEVEL; // Maxium Level allowed to join the Instance.

	if( min < 1 || min > MAX_LEVEL) {
		ShowError("buildin_instance_check_party: Invalid min level, %d\n", min);
		return SCRIPT_CMD_FAILURE;
	} else if(  max < 1 || max > MAX_LEVEL) {
		ShowError("buildin_instance_check_party: Invalid max level, %d\n", max);
		return SCRIPT_CMD_FAILURE;
	}

	if( script_hasdata(st,2) )
		party_id = script_getnum(st,2);
	else return SCRIPT_CMD_FAILURE;

	if( !(p = party_search(party_id)) ) {
		script_pushint(st, 0); // Returns false if party does not exist.
		return SCRIPT_CMD_FAILURE;
	}

	for( i = 0; i < MAX_PARTY; i++ ) {
		struct map_session_data *pl_sd;
		if( (pl_sd = p->data[i].sd) )
			if(map_id2bl(pl_sd->bl.id) && !pl_sd->state.autotrade) {
				if(pl_sd->status.base_level < min) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				} else if(pl_sd->status.base_level > max) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				}
					c++;
			}
	}

	if(c < amount)
		script_pushint(st, 0); // Not enough Members in the Party to join Instance.
	else
		script_pushint(st, 1);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * instance_check_guild
 * Values:
 * guild_id : Guild ID of the invoking character. [Required Parameter]
 * amount : Amount of needed Guild members for the Instance. [Optional Parameter]
 * min : Minimum Level needed to join the Instance. [Optional Parameter]
 * max : Maxium Level allowed to join the Instance. [Optional Parameter]
 * Example: instance_check_guild (getcharid(2){,amount}{,min}{,max});
 * Example 2: instance_check_guild (getcharid(2),1,1,99);
 *------------------------------------------*/
BUILDIN_FUNC(instance_check_guild)
{
	int amount, min, max, i, guild_id = 0, c = 0;
	struct guild *g = NULL;

	amount = script_hasdata(st,3) ? script_getnum(st,3) : 1; // Amount of needed Guild members for the Instance.
	min = script_hasdata(st,4) ? script_getnum(st,4) : 1; // Minimum Level needed to join the Instance.
	max  = script_hasdata(st,5) ? script_getnum(st,5) : MAX_LEVEL; // Maxium Level allowed to join the Instance.

	if (min < 1 || min > MAX_LEVEL) {
		ShowError("buildin_instance_check_guild: Invalid min level, %d\n", min);
		return SCRIPT_CMD_FAILURE;
	} else if (max < 1 || max > MAX_LEVEL) {
		ShowError("buildin_instance_check_guild: Invalid max level, %d\n", max);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st,2))
		guild_id = script_getnum(st,2);
	else
		return SCRIPT_CMD_FAILURE;

	if (!(g = guild_search(guild_id))) {
		script_pushint(st, 0); // Returns false if guild does not exist.
		return SCRIPT_CMD_FAILURE;
	}

	for(i = 0; i < MAX_GUILD; i++) {
		struct map_session_data *pl_sd;

		if ((pl_sd = g->member[i].sd)) {
			if (map_id2bl(pl_sd->bl.id) && !pl_sd->state.autotrade) {
				if (pl_sd->status.base_level < min) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				} else if (pl_sd->status.base_level > max) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				}
				c++;
			}
		}
	}

	if (c < amount)
		script_pushint(st, 0); // Not enough Members in the Guild to join Instance.
	else
		script_pushint(st, 1);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * instance_check_clan
 * Values:
 * clan_id : Clan ID of the invoking character. [Required Parameter]
 * amount : Amount of needed Clan members for the Instance. [Optional Parameter]
 * min : Minimum Level needed to join the Instance. [Optional Parameter]
 * max : Maxium Level allowed to join the Instance. [Optional Parameter]
 * Example: instance_check_clan (getcharid(5){,amount}{,min}{,max});
 * Example 2: instance_check_clan (getcharid(5),1,1,99);
 *------------------------------------------*/
BUILDIN_FUNC(instance_check_clan)
{
	int amount, min, max, i, clan_id = 0, c = 0;
	struct clan *cd = NULL;

	amount = script_hasdata(st,3) ? script_getnum(st,3) : 1; // Amount of needed Clan members for the Instance.
	min = script_hasdata(st,4) ? script_getnum(st,4) : 1; // Minimum Level needed to join the Instance.
	max  = script_hasdata(st,5) ? script_getnum(st,5) : MAX_LEVEL; // Maxium Level allowed to join the Instance.

	if (min < 1 || min > MAX_LEVEL) {
		ShowError("buildin_instance_check_clan: Invalid min level, %d\n", min);
		return SCRIPT_CMD_FAILURE;
	} else if (max < 1 || max > MAX_LEVEL) {
		ShowError("buildin_instance_check_clan: Invalid max level, %d\n", max);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st,2))
		clan_id = script_getnum(st,2);
	else
		return SCRIPT_CMD_FAILURE;

	if (!(cd = clan_search(clan_id))) {
		script_pushint(st, 0); // Returns false if clan does not exist.
		return SCRIPT_CMD_FAILURE;
	}

	for(i = 0; i < MAX_CLAN; i++) {
		struct map_session_data *pl_sd;

		if ((pl_sd = cd->members[i])) {
			if (map_id2bl(pl_sd->bl.id) && !pl_sd->state.autotrade) {
				if (pl_sd->status.base_level < min) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				} else if (pl_sd->status.base_level > max) {
					script_pushint(st, 0);
					return SCRIPT_CMD_SUCCESS;
				}
				c++;
			}
		}
	}

	if (c < amount)
		script_pushint(st, 0); // Not enough Members in the Clan to join Instance.
	else
		script_pushint(st, 1);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
* instance_info
* Values:
* name : name of the instance you want to look up. [Required Parameter]
* type : type of information you want to look up for the specified instance. [Required Parameter]
* index : index of the map in the instance. [Optional Parameter]
*------------------------------------------*/
BUILDIN_FUNC(instance_info)
{
	const char* name = script_getstr(st, 2);
	int type = script_getnum(st, 3);
	int index = 0;
	std::shared_ptr<s_instance_db> db = instance_search_db_name(name);

	if (!db) {
		ShowError( "buildin_instance_info: Unknown instance name \"%s\".\n", name );
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	switch( type ){
		case IIT_ID:
			script_pushint(st, db->id);
			break;
		case IIT_TIME_LIMIT:
			script_pushint(st, db->limit);
			break;
		case IIT_IDLE_TIMEOUT:
			script_pushint(st, db->timeout);
			break;
		case IIT_ENTER_MAP:
			script_pushstrcopy(st, map_mapid2mapname(db->enter.map));
			break;
		case IIT_ENTER_X:
			script_pushint(st, db->enter.x);
			break;
		case IIT_ENTER_Y:
			script_pushint(st, db->enter.y);
			break;
		case IIT_MAPCOUNT:
			script_pushint(st, db->maplist.size());
			break;
		case IIT_MAP:
			if( !script_hasdata(st, 4) || script_isstring(st, 4) ){
				ShowError( "buildin_instance_info: Type IIT_MAP requires a numeric index argument.\n" );
				script_pushconststr(st, "");
				return SCRIPT_CMD_FAILURE;
			}
			
			index = script_getnum(st, 4);

			if( index < 0 ){
				ShowError( "buildin_instance_info: Type IIT_MAP does not support a negative index argument.\n" );
				script_pushconststr(st, "");
				return SCRIPT_CMD_FAILURE;
			}

			if( index > UINT8_MAX ){
				ShowError( "buildin_instance_info: Type IIT_MAP does only support up to index %hu.\n", UINT8_MAX );
				script_pushconststr(st, "");
				return SCRIPT_CMD_FAILURE;
			}

			script_pushstrcopy(st, map_mapid2mapname(db->maplist[index]));
			break;

		default:
			ShowError("buildin_instance_info: Unknown instance information type \"%d\".\n", type );
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/*------------------------------------------
*instance_live_info( <info type>{, <instance id>} );
- ILI_NAME : Instance Name
- ILI_MODE : Instance Mode
- ILI_OWNER : owner id
*------------------------------------------*/
BUILDIN_FUNC(instance_live_info)
{
	int type = script_getnum(st, 2);
	int id = 0;

	if (type < ILI_NAME || type > ILI_OWNER) {
		ShowError("buildin_instance_live_info: Unknown instance information type \"%d\".\n", type);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (!script_hasdata(st, 3))
		id = script_instancegetid(st);
	else
		id = script_getnum(st, 3);

	std::shared_ptr<s_instance_db> db = nullptr;
	std::shared_ptr<s_instance_data> im = nullptr;

	if (id > 0 && id < INT_MAX) {
		im = util::umap_find(instances, id);

		if (im)
			db = instance_db.find(im->id);
	}

	if (!im || !db) {
		if (type == ILI_NAME)
			script_pushconststr(st, "");
		else
			script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	switch( type ) {
	case ILI_NAME:
		script_pushstrcopy(st, db->name.c_str());
		break;
	case ILI_MODE:
		script_pushint(st, im->mode);
		break;
	case ILI_OWNER:
		script_pushint(st, im->owner_id);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}
/*==========================================
 * instance_list(<"map name">{,<instance mode>});
 * set '.@instance_list' to a list of the live instance ids for the map with the mode.
 * return the array size of '.@instance_list'
 *------------------------------------------*/
BUILDIN_FUNC(instance_list)
{
	int src_id = map_mapname2mapid(script_getstr(st, 2));
	if (src_id == 0) {
		ShowError("buildin_instance_list: map '%s' doesn't exist\n", script_getstr(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	e_instance_mode mode = IM_MAX;
	if (script_hasdata(st, 3)) {
		mode = static_cast<e_instance_mode>(script_getnum(st, 3));
		if (mode < IM_NONE || mode >= IM_MAX) {
			ShowError("buildin_instance_list: Unknown instance mode %d for '%s'\n", mode, script_getstr(st, 3));
			return SCRIPT_CMD_FAILURE;
		}
	}

	int j = 0;
	for (int i = instance_start; i < map_num; i++) {
		struct map_data* mapdata = &map[i];
		if (mapdata->instance_src_map == src_id) {
			std::shared_ptr<s_instance_data> idata = util::umap_find(instances, mapdata->instance_id);
			if (idata && (mode == IM_MAX || idata->mode == mode)) {
				setd_sub_num(st, nullptr, ".@instance_list", j, mapdata->instance_id, nullptr);
				j++;
			}
		}
	}
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * Custom Fonts
 *------------------------------------------*/
BUILDIN_FUNC(setfont)
{
	struct map_session_data *sd;
	int font;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	font = script_getnum(st,2);

	if( sd->status.font != font )
		sd->status.font = font;
	else
		sd->status.font = 0;

	clif_font(sd);
	return SCRIPT_CMD_SUCCESS;
}

static int buildin_mobuseskill_sub(struct block_list *bl,va_list ap)
{
	TBL_MOB* md		= (TBL_MOB*)bl;
	struct block_list *tbl;
	int mobid		= va_arg(ap,int);
	uint16 skill_id	= va_arg(ap,int);
	uint16 skill_lv	= va_arg(ap,int);
	int casttime	= va_arg(ap,int);
	int cancel		= va_arg(ap,int);
	int emotion		= va_arg(ap,int);
	int target		= va_arg(ap,int);

	if( md->mob_id != mobid )
		return 0;

	// 0:self, 1:target, 2:master, default:random
	switch( target ) {
		case 0: tbl = map_id2bl(md->bl.id); break;
		case 1: tbl = map_id2bl(md->target_id); break;
		case 2: tbl = map_id2bl(md->master_id); break;
		default:tbl = battle_getenemy(&md->bl, DEFAULT_ENEMY_TYPE(md), skill_get_range2(&md->bl, skill_id, skill_lv, true)); break;
	}

	if( !tbl )
		return 0;

	if( md->ud.skilltimer != INVALID_TIMER ) // Cancel the casting skill.
		unit_skillcastcancel(bl,0);

	if( skill_get_casttype(skill_id) == CAST_GROUND )
		unit_skilluse_pos2(&md->bl, tbl->x, tbl->y, skill_id, skill_lv, casttime, cancel);
	else
		unit_skilluse_id2(&md->bl, tbl->id, skill_id, skill_lv, casttime, cancel);

	clif_emotion(&md->bl, emotion);

	return 1;
}

/*==========================================
 * areamobuseskill "Map Name",<x>,<y>,<range>,<Mob ID>,"Skill Name"/<Skill ID>,<Skill Lv>,<Cast Time>,<Cancelable>,<Emotion>,<Target Type>;
 *------------------------------------------*/
BUILDIN_FUNC(areamobuseskill)
{
	struct block_list center;
	int16 m;
	int range,mobid,skill_id,skill_lv,casttime,emotion,target,cancel;

	if( (m = map_mapname2mapid(script_getstr(st,2))) < 0 ) {
		ShowError("areamobuseskill: invalid map name.\n");
		return SCRIPT_CMD_SUCCESS;
	}

	center.m = m;
	center.x = script_getnum(st,3);
	center.y = script_getnum(st,4);
	range = script_getnum(st,5);
	mobid = script_getnum(st,6);
	if (script_isstring(st, 7)) {
		const char *name = script_getstr(st, 7);

		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_areamobuseskill: Invalid skill name %s.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		skill_id = script_getnum(st, 7);

		if (!skill_get_index(skill_id)) {
			ShowError("buildin_areamobuseskill: Invalid skill ID %d.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	if( (skill_lv = script_getnum(st,8)) > battle_config.mob_max_skilllvl )
		skill_lv = battle_config.mob_max_skilllvl;

	casttime = script_getnum(st,9);
	cancel = script_getnum(st,10);
	emotion = script_getnum(st,11);
	target = script_getnum(st,12);

	map_foreachinallrange(buildin_mobuseskill_sub, &center, range, BL_MOB, mobid, skill_id, skill_lv, casttime, cancel, emotion, target);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Display a progress bar above a character
 * progressbar "<color>",<seconds>;
 */
BUILDIN_FUNC(progressbar)
{
	struct map_session_data * sd;
	const char * color;
	unsigned int second;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;

	st->state = STOP;

	color = script_getstr(st,2);
	second = script_getnum(st,3);

	sd->progressbar.npc_id = st->oid;
	sd->progressbar.timeout = gettick() + second*1000;
	sd->state.workinprogress = WIP_DISABLE_ALL;

	clif_progressbar(sd, strtol(color, (char **)NULL, 0), second);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Display a progress bar above an NPC
 * progressbar_npc "<color>",<seconds>{,<"NPC Name">};
 */
BUILDIN_FUNC(progressbar_npc){
	map_session_data *sd = map_id2sd(st->rid);
	struct npc_data* nd = NULL;

	if( script_hasdata(st, 4) ){
		const char* name = script_getstr(st, 4);

		nd = npc_name2id(name);

		if( !nd ){
			ShowError( "buildin_progressbar_npc: NPC \"%s\" was not found.\n", name );
			return SCRIPT_CMD_FAILURE;
		}
	}else{
		nd = map_id2nd(st->oid);
	}

	// First call(by function call)
	if( !nd->progressbar.timeout ){
		const char *color;
		int second;

		color = script_getstr(st, 2);
		second = script_getnum(st, 3);

		if( second < 0 ){
			ShowError( "buildin_progressbar_npc: negative amount('%d') of seconds is not supported\n", second );
			return SCRIPT_CMD_FAILURE;
		}

		if (sd) { // Player attached - keep them from doing other things
			sd->state.workinprogress = WIP_DISABLE_ALL;
			sd->state.block_action |= (PCBLOCK_MOVE | PCBLOCK_ATTACK | PCBLOCK_SKILL);
		}

		// sleep for the target amount of time
		st->state = RERUNLINE;
		st->sleep.tick = second * 1000;
		nd->progressbar.timeout = gettick() + second * 1000;
		nd->progressbar.color = strtol(color, (char **)NULL, 0);

		clif_progressbar_npc_area(nd);
	// Second call(by timer after sleeping time is over)
	} else {
		// Continue the script
		if (sd) { // Player attached - remove restrictions
			sd->state.workinprogress = WIP_DISABLE_NONE;
			sd->state.block_action &= ~(PCBLOCK_MOVE | PCBLOCK_ATTACK | PCBLOCK_SKILL);
		}

		st->state = RUN;
		st->sleep.tick = 0;
		nd->progressbar.timeout = nd->progressbar.color = 0;
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pushpc)
{
	uint8 dir;
	int cells, dx, dy;
	struct map_session_data* sd;

	if(!script_rid2sd(sd))
	{
		return SCRIPT_CMD_SUCCESS;
	}

	dir = script_getnum(st,2);
	cells     = script_getnum(st,3);

	if(dir >= DIR_MAX)
	{
		ShowWarning("buildin_pushpc: Invalid direction %d specified.\n", dir);
		script_reportsrc(st);

		dir%= DIR_MAX;  // trim spin-over
	}

	if(!cells)
	{// zero distance
		return SCRIPT_CMD_SUCCESS;
	}
	else if(cells<0)
	{// pushing backwards
		dir = (dir+DIR_MAX/2)%DIR_MAX;  // turn around
		cells     = -cells;
	}

	dx = dirx[dir];
	dy = diry[dir];

	unit_blown(&sd->bl, dx, dy, cells, BLOWN_NONE);
	return SCRIPT_CMD_SUCCESS;
}


/// Invokes buying store preparation window
/// buyingstore <slots>;
BUILDIN_FUNC(buyingstore)
{
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
	{
		return SCRIPT_CMD_SUCCESS;
	}

	if( npc_isnear(&sd->bl) ) {
		char output[150];
		sprintf(output, msg_txt(sd,662), battle_config.min_npc_vendchat_distance);
		clif_displaymessage(sd->fd, output);
		return SCRIPT_CMD_SUCCESS;
	}

	buyingstore_setup(sd, script_getnum(st,2));
	return SCRIPT_CMD_SUCCESS;
}


/// Invokes search store info window
/// searchstores <uses>,<effect>;
BUILDIN_FUNC(searchstores)
{
	unsigned short effect;
	unsigned int uses;
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
	{
		return SCRIPT_CMD_SUCCESS;
	}

	uses   = script_getnum(st,2);
	effect = script_getnum(st,3);

	if( !uses )
	{
		ShowError("buildin_searchstores: Amount of uses cannot be zero.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if( effect > 1 )
	{
		ShowError("buildin_searchstores: Invalid effect id %hu, specified.\n", effect);
		return SCRIPT_CMD_FAILURE;
	}

	searchstore_open(sd, uses, effect);
	return SCRIPT_CMD_SUCCESS;
}
/// Displays a number as large digital clock.
/// showdigit <value>[,<type>];
BUILDIN_FUNC(showdigit)
{
	unsigned int type = 0;
	int value;
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
	{
		return SCRIPT_CMD_SUCCESS;
	}

	value = script_getnum(st,2);

	if( script_hasdata(st,3) ){
		type = script_getnum(st,3);
	}

	switch( type ){
		case 0:
			break;
		case 1:
		case 2:
			// Use absolute value and then the negative value of it as starting value
			// This is what gravity's client does for these counters
			value = -abs(value);
			break;
		case 3:
			value = abs(value);
			if( value > 99 ){
				ShowWarning("buildin_showdigit: type 3 can display 2 digits at max. Capping value %d to 99...\n", value);
				script_reportsrc(st);
				value = 99;
			}
			break;
		default:
			ShowError("buildin_showdigit: Invalid type %u.\n", type);
			return SCRIPT_CMD_FAILURE;
	}

	clif_showdigit(sd, (unsigned char)type, value);
	return SCRIPT_CMD_SUCCESS;
}
/**
 * Rune Knight
 * makerune <% success bonus>{,<char_id>};
 **/
BUILDIN_FUNC(makerune) {
	TBL_PC* sd;
	
	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;
	clif_skill_produce_mix_list(sd,RK_RUNEMASTERY,24);
	sd->itemid = script_getnum(st,2);
	return SCRIPT_CMD_SUCCESS;
}
/**
 * checkdragon({<char_id>}) returns 1 if mounting a dragon or 0 otherwise.
 **/
BUILDIN_FUNC(checkdragon) {
	TBL_PC* sd;
	
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	if( pc_isridingdragon(sd) )
		script_pushint(st,1);
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}
/**
 * setdragon({optional Color{,<char_id>}}) returns 1 on success or 0 otherwise
 * - Toggles the dragon on a RK if he can mount;
 * @param Color - when not provided uses the green dragon;
 * - 1 : Green Dragon
 * - 2 : Brown Dragon
 * - 3 : Gray Dragon
 * - 4 : Blue Dragon
 * - 5 : Red Dragon
 **/
BUILDIN_FUNC(setdragon) {
	TBL_PC* sd;
	int color = script_hasdata(st,2) ? script_getnum(st,2) : 0;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;
	if( !pc_checkskill(sd,RK_DRAGONTRAINING) || (sd->class_&MAPID_THIRDMASK) != MAPID_RUNE_KNIGHT )
		script_pushint(st,0);//Doesn't have the skill or it's not a Rune Knight
	else if ( pc_isridingdragon(sd) ) {//Is mounted; release
		pc_setoption(sd, sd->sc.option&~OPTION_DRAGON);
		script_pushint(st,1);
	} else {//Not mounted; Mount now.
		unsigned int option = OPTION_DRAGON1;
		if( color ) {
			option = ( color == 1 ? OPTION_DRAGON1 :
					   color == 2 ? OPTION_DRAGON2 :
					   color == 3 ? OPTION_DRAGON3 :
					   color == 4 ? OPTION_DRAGON4 :
					   color == 5 ? OPTION_DRAGON5 : 0);
			if( !option ) {
				ShowWarning("script_setdragon: Unknown Color %d used; changing to green (1)\n",color);
				option = OPTION_DRAGON1;
			}
		}
		pc_setoption(sd, sd->sc.option|option);
		script_pushint(st,1);
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
 * ismounting({<char_id>}) returns 1 if mounting a new mount or 0 otherwise
 **/
BUILDIN_FUNC(ismounting) {
	TBL_PC* sd;
	
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	if( sd->sc.data[SC_ALL_RIDING] )
		script_pushint(st,1);
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * setmounting({<char_id>}) returns 1 on success or 0 otherwise
 * - Toggles new mounts on a player when he can mount
 * - Will fail if the player is mounting a non-new mount, e.g. dragon, peco, wug, etc.
 * - Will unmount the player is he is already mounting
 **/
BUILDIN_FUNC(setmounting) {
	TBL_PC* sd;
	
	if (!script_charid2sd(2,sd))
		return SCRIPT_CMD_FAILURE;
	if( sd->sc.option&(OPTION_WUGRIDER|OPTION_RIDING|OPTION_DRAGON|OPTION_MADOGEAR) ) {
		clif_msg(sd, NEED_REINS_OF_MOUNT);
		script_pushint(st,0); //can't mount with one of these
	} else if (sd->sc.data[SC_CLOAKING] || sd->sc.data[SC_CHASEWALK] || sd->sc.data[SC_CLOAKINGEXCEED] || sd->sc.data[SC_CAMOUFLAGE] || sd->sc.data[SC_STEALTHFIELD] || sd->sc.data[SC__FEINTBOMB]) {
		// SC_HIDING, SC__INVISIBILITY, SC__SHADOWFORM, SC_SUHIDE already disable item usage
		script_pushint(st, 0); // Silent failure
	} else {
		if( sd->sc.data[SC_ALL_RIDING] )
			status_change_end(&sd->bl, SC_ALL_RIDING, INVALID_TIMER); //release mount
		else
			sc_start(NULL, &sd->bl, SC_ALL_RIDING, 10000, 1, INFINITE_TICK); //mount
		script_pushint(st,1);//in both cases, return 1.
	}
	return SCRIPT_CMD_SUCCESS;
}
/**
 * Retrieves quantity of arguments provided to callfunc/callsub.
 * getargcount() -> amount of arguments received in a function
 **/
BUILDIN_FUNC(getargcount) {
	struct script_retinfo* ri;

	if( st->stack->defsp < 1 || st->stack->stack_data[st->stack->defsp - 1].type != C_RETINFO ) {
		ShowError("script:getargcount: used out of function or callsub label!\n");
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}
	ri = st->stack->stack_data[st->stack->defsp - 1].u.ri;

	script_pushint(st, ri->nargs);
	return SCRIPT_CMD_SUCCESS;
}
/**
 * getcharip(<account ID>/<character ID>/<character name>)
 **/
BUILDIN_FUNC(getcharip)
{
	struct map_session_data* sd = NULL;

	/* check if a character name is specified */
	if( script_hasdata(st, 2) )
	{
		if (script_isstring(st, 2))
			sd = map_nick2sd(script_getstr(st, 2),false);
		else
		{
			int id = script_getnum(st, 2);

			sd = (map_id2sd(id) ? map_id2sd(id) : map_charid2sd(id));
		}
	}
	else
		script_rid2sd(sd);

	/* check for sd and IP */
	if (!sd || !session[sd->fd]->client_addr)
	{
		script_pushconststr(st, "");
		return SCRIPT_CMD_SUCCESS;
	}

	/* return the client ip_addr converted for output */
	if (sd && sd->fd && session[sd->fd])
	{
		/* initiliaze */
		const char *ip_addr = NULL;
		uint32 ip;

		/* set ip, ip_addr and convert to ip and push str */
		ip = session[sd->fd]->client_addr;
		ip_addr = ip2str(ip, NULL);
		script_pushstrcopy(st, ip_addr);
	}
	return SCRIPT_CMD_SUCCESS;
}
/**
 * is_function(<function name>) -> 1 if function exists, 0 otherwise
 **/
BUILDIN_FUNC(is_function) {
	const char* str = script_getstr(st,2);

	if( strdb_exists(userfunc_db, str) )
		script_pushint(st,1);
	else
		script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
}
/**
 * get_revision() -> retrieves the current svn revision (if available)
 **/
BUILDIN_FUNC(get_revision) {
	const char *svn = get_svn_revision();

	if ( svn[0] != UNKNOWN_VERSION )
		script_pushint(st,atoi(svn));
	else
		script_pushint(st,-1); //unknown
	return SCRIPT_CMD_SUCCESS;
}
/* get_hash() -> retrieves the current git hash (if available)*/
BUILDIN_FUNC(get_githash) {
	const char* git = get_git_hash();
	char buf[CHAT_SIZE_MAX];
	safestrncpy(buf,git,strlen(git)+1);

	if ( git[0] != UNKNOWN_VERSION )
		script_pushstrcopy(st,buf);
	else
		script_pushconststr(st,"Unknown"); //unknown
	return SCRIPT_CMD_SUCCESS;
}
/**
 * freeloop(<toggle>) -> toggles this script instance's looping-check ability
 **/
BUILDIN_FUNC(freeloop) {

	if( script_hasdata(st,2) ) {
		if( script_getnum(st,2) )
			st->freeloop = 1;
		else
			st->freeloop = 0;
	}

	script_pushint(st, st->freeloop);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * @commands (script based)
 **/
BUILDIN_FUNC(bindatcmd) {
	const char* atcmd;
	const char* eventName;
	int i, level = 0, level2 = 100;
	bool create = false;

	atcmd = script_getstr(st,2);
	eventName = script_getstr(st,3);

	if( *atcmd == atcommand_symbol || *atcmd == charcommand_symbol )
		atcmd++;

	if( script_hasdata(st,4) ) level = script_getnum(st,4);
	if( script_hasdata(st,5) ) level2 = script_getnum(st,5);

	if( atcmd_binding_count == 0 ) {
		CREATE(atcmd_binding,struct atcmd_binding_data*,1);

		create = true;
	} else {
		ARR_FIND(0, atcmd_binding_count, i, strcmp(atcmd_binding[i]->command,atcmd) == 0);
		if( i < atcmd_binding_count ) {/* update existent entry */
			safestrncpy(atcmd_binding[i]->npc_event, eventName, EVENT_NAME_LENGTH);
			atcmd_binding[i]->level = level;
			atcmd_binding[i]->level2 = level2;
		} else
			create = true;
	}

	if( create ) {
		i = atcmd_binding_count;

		if( atcmd_binding_count++ != 0 )
			RECREATE(atcmd_binding,struct atcmd_binding_data*,atcmd_binding_count);

		CREATE(atcmd_binding[i],struct atcmd_binding_data,1);

		safestrncpy(atcmd_binding[i]->command, atcmd, 50);
		safestrncpy(atcmd_binding[i]->npc_event, eventName, EVENT_NAME_LENGTH);
		atcmd_binding[i]->level = level;
		atcmd_binding[i]->level2 = level2;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(unbindatcmd) {
	const char* atcmd;
	int i =  0;

	atcmd = script_getstr(st, 2);

	if( *atcmd == atcommand_symbol || *atcmd == charcommand_symbol )
		atcmd++;

	if( atcmd_binding_count == 0 ) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	ARR_FIND(0, atcmd_binding_count, i, strcmp(atcmd_binding[i]->command, atcmd) == 0);
	if( i < atcmd_binding_count ) {
		int cursor = 0;
		aFree(atcmd_binding[i]);
		atcmd_binding[i] = NULL;
		/* compact the list now that we freed a slot somewhere */
		for( i = 0, cursor = 0; i < atcmd_binding_count; i++ ) {
			if( atcmd_binding[i] == NULL )
				continue;

			if( cursor != i ) {
				memmove(&atcmd_binding[cursor], &atcmd_binding[i], sizeof(struct atcmd_binding_data*));
			}

			cursor++;
		}

		if( (atcmd_binding_count = cursor) == 0 )
			aFree(atcmd_binding);

		script_pushint(st, 1);
	} else
		script_pushint(st, 0);/* not found */

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(useatcmd) {
	return atcommand_sub(st,3);
}

BUILDIN_FUNC(checkre)
{
	int num;

	num=script_getnum(st,2);
	switch(num){
		case 0:
			#ifdef RENEWAL
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		case 1:
			#ifdef RENEWAL_CAST
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		case 2:
			#ifdef RENEWAL_DROP
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		case 3:
			#ifdef RENEWAL_EXP
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		case 4:
			#ifdef RENEWAL_LVDMG
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		case 5:
			#ifdef RENEWAL_ASPD
				script_pushint(st, 1);
			#else
				script_pushint(st, 0);
			#endif
			break;
		default:
			ShowWarning("buildin_checkre: unknown parameter.\n");
			break;
	}
	return SCRIPT_CMD_SUCCESS;
}

/* getrandgroupitem <group_id>{,<quantity>{,<sub_group>{,<identify>{,<char_id>}}}} */
BUILDIN_FUNC(getrandgroupitem) {
	TBL_PC* sd;
	int i, get_count = 0, identify = 0;
	uint16 group, qty = 0;
	uint8 sub_group = 1;
	struct item item_tmp;

	if (!script_charid2sd(6, sd))
		return SCRIPT_CMD_SUCCESS;

	group = script_getnum(st,2);

	if (!group) {
		ShowError("buildin_getrandgroupitem: Invalid group id (%d)!\n",script_getnum(st,2));
		return SCRIPT_CMD_FAILURE;
	}

	FETCH(3, qty);
	FETCH(4, sub_group);
	FETCH(5, identify);

	std::shared_ptr<s_item_group_entry> entry = itemdb_group.get_random_entry(group,sub_group);
	if (!entry)
		return SCRIPT_CMD_FAILURE; //ensure valid itemid

	memset(&item_tmp,0,sizeof(item_tmp));
	item_tmp.nameid   = entry->nameid;
	item_tmp.identify = identify ? 1 : itemdb_isidentified(entry->nameid);
#ifdef Pandas_BattleConfig_Force_Identified
	item_tmp.identify = (battle_config.force_identified & 64 ? 1 : item_tmp.identify);
#endif // Pandas_BattleConfig_Force_Identified

	if (!qty)
		qty = entry->amount;

	//Check if it's stackable.
	if (!itemdb_isstackable(entry->nameid)) {
		item_tmp.amount = 1;
		get_count = qty;
	}
	else {
		item_tmp.amount = qty;
		get_count = 1;
	}

	for (i = 0; i < get_count; i++) {
		// if not pet egg
		if (!pet_create_egg(sd, entry->nameid)) {
			unsigned char flag = 0;
			if ((flag = pc_additem(sd,&item_tmp,item_tmp.amount,LOG_TYPE_SCRIPT))) {
				clif_additem(sd,0,0,flag);
				if (pc_candrop(sd,&item_tmp))
					map_addflooritem(&item_tmp,item_tmp.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
			}
		}
	}

	return SCRIPT_CMD_SUCCESS;
}

/* getgroupitem <group_id>{,<identify>{,<char_id>}};
 * Gives item(s) to the attached player based on item group contents
 */
BUILDIN_FUNC(getgroupitem) {
	TBL_PC *sd;
	int group_id = script_getnum(st,2);
	
	if (!script_charid2sd(4,sd))
		return SCRIPT_CMD_SUCCESS;
	
	if (itemdb_group.pc_get_itemgroup(group_id, (script_hasdata(st, 3) ? script_getnum(st, 3) != 0 : false), sd)) {
		ShowError("buildin_getgroupitem: Invalid group id '%d' specified.\n",group_id);
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/* cleanmap <map_name>;
 * cleanarea <map_name>, <x0>, <y0>, <x1>, <y1>; */
static int atcommand_cleanfloor_sub(struct block_list *bl, va_list ap)
{
	nullpo_ret(bl);
	map_clearflooritem(bl);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(cleanmap)
{
	const char *mapname;
	int16 m;

	mapname = script_getstr(st, 2);
	m = map_mapname2mapid(mapname);
	if (!m)
		return SCRIPT_CMD_FAILURE;

	if ((script_lastdata(st) - 2) < 4) {
		map_foreachinmap(atcommand_cleanfloor_sub, m, BL_ITEM);
	} else {
		int16 x0 = script_getnum(st, 3);
		int16 y0 = script_getnum(st, 4);
		int16 x1 = script_getnum(st, 5);
		int16 y1 = script_getnum(st, 6);
		if (x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0) {
			map_foreachinallarea(atcommand_cleanfloor_sub, m, x0, y0, x1, y1, BL_ITEM);
		} else {
			ShowError("cleanarea: invalid coordinate defined!\n");
			return SCRIPT_CMD_FAILURE;
		}
	}
	return SCRIPT_CMD_SUCCESS;
}

/* Cast a skill on the attached player.
 * npcskill <skill id>, <skill lvl>, <stat point>, <NPC level>;
 * npcskill "<skill name>", <skill lvl>, <stat point>, <NPC level>; */
BUILDIN_FUNC(npcskill)
{
	uint16 skill_id;
	unsigned short skill_level;
	unsigned int stat_point;
	unsigned int npc_level;
	struct npc_data *nd;
	struct map_session_data *sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_SUCCESS;
	
	if (script_isstring(st, 2)) {
		const char *name = script_getstr(st, 2);

		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_npcskill: Invalid skill name %s.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		skill_id = script_getnum(st, 2);

		if (!skill_get_index(skill_id)) {
			ShowError("buildin_npcskill: Invalid skill ID %d.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}
	skill_level	= script_getnum(st, 3);
	stat_point	= script_getnum(st, 4);
	npc_level	= script_getnum(st, 5);
	nd			= (struct npc_data *)map_id2bl(sd->npc_id);

	if (stat_point > battle_config.max_third_parameter) {
		ShowError("npcskill: stat point exceeded maximum of %d.\n",battle_config.max_third_parameter );
		return SCRIPT_CMD_FAILURE;
	}
	if (npc_level > MAX_LEVEL) {
		ShowError("npcskill: level exceeded maximum of %d.\n", MAX_LEVEL);
		return SCRIPT_CMD_FAILURE;
	}
	if (nd == NULL) { //ain't possible, but I don't trust people.
		return SCRIPT_CMD_FAILURE;
	}

	nd->level = npc_level;
	nd->stat_point = stat_point;

	if (!nd->status.hp)
		status_calc_npc(nd, SCO_FIRST);
	else
		status_calc_npc(nd, SCO_NONE);

	if (skill_get_inf(skill_id)&INF_GROUND_SKILL)
		unit_skilluse_pos2(&nd->bl, sd->bl.x, sd->bl.y, skill_id, skill_level,0,0);
	else
		unit_skilluse_id2(&nd->bl, sd->bl.id, skill_id, skill_level,0,0);

	return SCRIPT_CMD_SUCCESS;
}

/* Consumes an item.
 * consumeitem <item id>{,<char_id>};
 * consumeitem "<item name>"{,<char_id>};
 * @param item: Item ID or name
 */
BUILDIN_FUNC(consumeitem)
{
	struct map_session_data *sd;
	std::shared_ptr<item_data> item_data;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	if( script_isstring(st, 2) ){
		const char *name = script_getstr(st, 2);

		item_data = item_db.searchname( name );

		if( item_data == nullptr ){
			ShowError( "buildin_consumeitem: Nonexistant item %s requested.\n", name );
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		t_itemid nameid = script_getnum(st, 2);

		item_data = item_db.find( nameid );

		if( item_data == nullptr ){
			ShowError("buildin_consumeitem: Nonexistant item %u requested.\n", nameid );
			return SCRIPT_CMD_FAILURE;
		}
	}

	run_script( item_data->script, 0, sd->bl.id, 0 );
	return SCRIPT_CMD_SUCCESS;
}

/** Makes a player sit/stand.
 * sit {"<character name>"};
 * stand {"<character name>"};
 * Note: Use readparam(Sitting) which returns 1 or 0 (sitting or standing).
 * @param name: Player name that will be invoked
 */
BUILDIN_FUNC(sit)
{
	TBL_PC *sd;

	if( !script_nick2sd(2,sd) )
		return SCRIPT_CMD_FAILURE;

	if( !pc_issit(sd) ) {
		pc_setsit(sd);
		skill_sit(sd, 1);
		clif_sitting(&sd->bl);
	}
	return SCRIPT_CMD_SUCCESS;
}

/** Makes player to stand
* @param name: Player name that will be set
*/
BUILDIN_FUNC(stand)
{
	TBL_PC *sd;

	if( !script_nick2sd(2,sd) )
		return SCRIPT_CMD_FAILURE;

	if( pc_issit(sd) && pc_setstand(sd, false)) {
		skill_sit(sd, 0);
		clif_standing(&sd->bl);
	}

	return SCRIPT_CMD_SUCCESS;
}

/** Creates an array of bounded item IDs
 * countbound {<type>{,<char_id>}};
 * @param type: 0 - All bound items; 1 - Account Bound; 2 - Guild Bound; 3 - Party Bound
 * @return amt: Total number of different items type found
 */
BUILDIN_FUNC(countbound)
{
	TBL_PC *sd;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	int i, k = 0;
	int type = script_getnum(st,2);

	for( i = 0; i < MAX_INVENTORY; i ++ ) {
		if( sd->inventory.u.items_inventory[i].nameid > 0 && (
			(!type && sd->inventory.u.items_inventory[i].bound) || (type && sd->inventory.u.items_inventory[i].bound == type)
			))
		{
			pc_setreg(sd,reference_uid(add_str("@bound_items"), k),sd->inventory.u.items_inventory[i].nameid);
			pc_setreg(sd,reference_uid(add_str("@bound_amount"), k),sd->inventory.u.items_inventory[i].amount);
			k++;
		}
	}

	script_pushint(st,k);
	return SCRIPT_CMD_SUCCESS;
}

/** Creates new party
 * party_create "<party name>"{,<char id>{,<item share>{,<item share type>}}};
 * @param party_name: String of party name that will be created
 * @param char_id: Chara that will be leader of this new party. If no char_id specified, the invoker will be party leader
 * @param item_share: 0-Each Take. 1-Party Share
 * @param item_share_type: 0-Each Take. 1-Even Share
 * @return val: Result value
 *	-3	- party name is exist
 *	-2	- player is in party already
 *	-1	- player is not found
 *	0	- unknown error
 *	1	- success, will return party id $@party_create_id
 */
BUILDIN_FUNC(party_create)
{
	char party_name[NAME_LENGTH];
	int item1 = 0, item2 = 0;
	TBL_PC *sd = NULL;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->status.party_id ) {
		script_pushint(st,-2);
		return SCRIPT_CMD_FAILURE;
	}

	safestrncpy(party_name,script_getstr(st,2),NAME_LENGTH);
	trim(party_name);
	if( party_searchname(party_name) ) {
		script_pushint(st,-3);
		return SCRIPT_CMD_FAILURE;
	}
	if( script_getnum(st,4) )
		item1 = 1;
	if( script_getnum(st,5) )
		item2 = 1;

	party_create_byscript = 1;
	script_pushint(st,party_create(sd,party_name,item1,item2));
	return SCRIPT_CMD_SUCCESS;
}

/** Adds player to specified party
 * party_addmember <party id>,<char id>;
 * @param party_id: The party that will be entered by player
 * @param char_id: Char id of player that will be joined to the party
 * @return val: Result value
 *	-5	- another character of the same account is in the party
 *	-4	- party is full
 *	-3	- party is not found
 *	-2	- player is in party already
 *	-1	- player is not found
 *	0	- unknown error
 *	1	- success
 */
BUILDIN_FUNC(party_addmember)
{
	int party_id = script_getnum(st,2);
	TBL_PC *sd;
	struct party_data *party;

	if( !(sd = map_charid2sd(script_getnum(st,3))) ) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->status.party_id ) {
		script_pushint(st,-2);
		return SCRIPT_CMD_FAILURE;
	}

	if( !(party = party_search(party_id)) ) {
		script_pushint(st,-3);
		return SCRIPT_CMD_FAILURE;
	}

	if (battle_config.block_account_in_same_party) {
		int i;
		ARR_FIND(0, MAX_PARTY, i, party->party.member[i].account_id == sd->status.account_id);
		if (i < MAX_PARTY) {
			script_pushint(st,-5);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if( party->party.count >= MAX_PARTY ) {
		script_pushint(st,-4);
		return SCRIPT_CMD_FAILURE;
	}
	sd->party_invite = party_id;
	script_pushint(st,party_add_member(party_id,sd));
	return SCRIPT_CMD_SUCCESS;
}

/** Removes player from his/her party. If party_id and char_id is empty remove the invoker from his/her party
 * party_delmember {<char id>,<party_id>};
 * @param: char_id
 * @param: party_id
 * @return val: Result value
 *	-3	- player is not in party
 *	-2	- party is not found
 *	-1	- player is not found
 *	0	- unknown error
 *	1	- success
 */
BUILDIN_FUNC(party_delmember)
{
	TBL_PC *sd = NULL;

	if( !script_hasdata(st,2) && !script_hasdata(st,3) && !script_rid2sd(sd) ) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}
	if( sd || script_charid2sd(2,sd) )
		script_pushint(st,party_removemember2(sd,0,0));
	else
		script_pushint(st,party_removemember2(NULL,script_getnum(st,2),script_getnum(st,3)));
	return SCRIPT_CMD_SUCCESS;
}

/** Changes party leader of specified party (even the leader is offline)
 * party_changeleader <party id>,<char id>;
 * @param party_id: ID of party
 * @param char_id: Char ID of new leader
 * @return val: Result value
 *	-4	- player is party leader already
 *	-3	- player is not in this party
 *	-2	- player is not found
 *	-1	- party is not found
 *	0	- unknown error
 *	1	- success */
BUILDIN_FUNC(party_changeleader)
{
	int i, party_id = script_getnum(st,2);
	TBL_PC *sd = NULL;
	TBL_PC *tsd = NULL;
	struct party_data *party = NULL;

	if( !(party = party_search(party_id)) ) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if( !(tsd = map_charid2sd(script_getnum(st,3))) ) {
		script_pushint(st,-2);
		return SCRIPT_CMD_FAILURE;
	}

	if( tsd->status.party_id != party_id ) {
		script_pushint(st,-3);
		return SCRIPT_CMD_FAILURE;
	}

	ARR_FIND(0,MAX_PARTY,i,party->party.member[i].leader);
	if( i >= MAX_PARTY ) {	//this is should impossible!
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}
	if( party->data[i].sd == tsd ) {
		script_pushint(st,-4);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,party_changeleader(sd,tsd,party));
	return SCRIPT_CMD_SUCCESS;
}

/** Changes party option
 * party_changeoption <party id>,<option>,<flag>;
 * @param party_id: ID of party that will be changed
 * @param option: Type of option
 * @return val: -1 - party is not found, 0 - invalid option, 1 - success
 */
BUILDIN_FUNC(party_changeoption)
{
	struct party_data *party;

	if( !(party = party_search(script_getnum(st,2))) ) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st,party_setoption(party,script_getnum(st,3),script_getnum(st,4)));
	return SCRIPT_CMD_SUCCESS;
}

/** Destroys party with party id.
 * party_destroy <party id>;
 * @param party_id: ID of party that will be destroyed
 */
BUILDIN_FUNC(party_destroy)
{
	int i;
	struct party_data *party;

	if( !(party = party_search(script_getnum(st,2))) ) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	ARR_FIND(0,MAX_PARTY,i,party->party.member[i].leader);
	if( i >= MAX_PARTY || !party->data[i].sd ) { //leader not online
		int j;
		for( j = 0; j < MAX_PARTY; j++ ) {
			TBL_PC *sd = party->data[j].sd;
			if(sd)
				party_member_withdraw(party->party.party_id,sd->status.account_id,sd->status.char_id,sd->status.name,PARTY_MEMBER_WITHDRAW_LEAVE);
			else if( party->party.member[j].char_id )
				intif_party_leave(party->party.party_id,party->party.member[j].account_id,party->party.member[j].char_id,party->party.member[j].name,PARTY_MEMBER_WITHDRAW_LEAVE);
		}
		party_broken(party->party.party_id);
		script_pushint(st,1);
	}
	else //leader leave = party broken
		script_pushint(st,party_leave(party->data[i].sd));
	return SCRIPT_CMD_SUCCESS;
}

/** Returns various information about a player's VIP status. Need to enable VIP system
 * vip_status <type>,{"<character name>"};
 * @param type: Info type, see enum vip_status_type
 * @param name: Character name (Optional)
 */
BUILDIN_FUNC(vip_status) {
#ifdef VIP_ENABLE
	TBL_PC *sd;
	int type;

	if( !script_nick2sd(3,sd) )
		return SCRIPT_CMD_FAILURE;

	type = script_getnum(st, 2);

	switch(type) {
		case VIP_STATUS_ACTIVE: // Get VIP status.
			script_pushint(st, pc_isvip(sd));
			break;
		case VIP_STATUS_EXPIRE: // Get VIP expire date.
			if (pc_isvip(sd)) {
				script_pushint(st, sd->vip.time);
			} else
				script_pushint(st, 0);
			break;
		case VIP_STATUS_REMAINING: // Get remaining time.
			if (pc_isvip(sd)) {
				script_pushint(st, sd->vip.time - time(NULL));
			} else
				script_pushint(st, 0);
			break;
		default:
			ShowError( "buildin_vip_status: Unsupported type %d.\n", type );
			return SCRIPT_CMD_FAILURE;
	}
#else
	script_pushint(st, 0);
#endif
	return SCRIPT_CMD_SUCCESS;
}


/** Adds or removes VIP time in minutes. Need to enable VIP system
 * vip_time <time in mn>,{"<character name>"};
 * @param time: VIP duration in minutes. If time < 0 remove time, else add time.
 * @param name: Character name (optional)
 */
BUILDIN_FUNC(vip_time) {
#ifdef VIP_ENABLE //would be a pain for scripting npc otherwise
	TBL_PC *sd;
	int viptime = script_getnum(st, 2) * 60; // Convert since it's given in minutes.

	if( !script_nick2sd(3,sd) )
		return SCRIPT_CMD_FAILURE;

	chrif_req_login_operation(sd->status.account_id, sd->status.name, CHRIF_OP_LOGIN_VIP, viptime, 7, 0); 
#endif
	return SCRIPT_CMD_SUCCESS;
}


/**
 * Turns a player into a monster and optionally can grant a SC attribute effect.
 * transform <monster name/ID>, <duration>, <sc type>, <val1>, <val2>, <val3>, <val4>;
 * active_transform <monster name/ID>, <duration>, <sc type>, <val1>, <val2>, <val3>, <val4>;
 * @param monster: Monster ID or name
 * @param duration: Transform duration in millisecond (ms)
 * @param sc_type: Type of SC that will be affected during the transformation
 * @param val1: Value for SC
 * @param val2: Value for SC
 * @param val3: Value for SC
 * @param val4: Value for SC
 * @author: malufett
 */
BUILDIN_FUNC(montransform) {
	TBL_PC *sd;
	enum sc_type type;
	int tick, mob_id, val1, val2, val3, val4;
	val1 = val2 = val3 = val4 = 0;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_FAILURE;

	if( script_isstring(st, 2) )
		mob_id = mobdb_searchname(script_getstr(st, 2));
	else
		mob_id = mobdb_checkid(script_getnum(st, 2));

	tick = script_getnum(st, 3);

	if (script_hasdata(st, 4))
		type = (sc_type)script_getnum(st, 4);
	else
		type = SC_NONE;

	if (mob_id == 0) {
		if( script_isstring(st, 2) )
			ShowWarning("buildin_montransform: Attempted to use non-existing monster '%s'.\n", script_getstr(st, 2));
		else
			ShowWarning("buildin_montransform: Attempted to use non-existing monster of ID '%d'.\n", script_getnum(st, 2));
		return SCRIPT_CMD_FAILURE;
	}

	if (mob_id == MOBID_EMPERIUM) {
		ShowWarning("buildin_montransform: Monster 'Emperium' cannot be used.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if (!(type >= SC_NONE && type < SC_MAX)) {
		ShowWarning("buildin_montransform: Unsupported status change id %d\n", type);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 5))
		val1 = script_getnum(st, 5);

	if (script_hasdata(st, 6))
		val2 = script_getnum(st, 6);

	if (script_hasdata(st, 7))
		val3 = script_getnum(st, 7);

	if (script_hasdata(st, 8))
		val4 = script_getnum(st, 8);

	if (tick != 0) {
		if (battle_config.mon_trans_disable_in_gvg && map_flag_gvg2(sd->bl.m)) {
			clif_displaymessage(sd->fd, msg_txt(sd,731)); // Transforming into monster is not allowed in Guild Wars.
			return SCRIPT_CMD_FAILURE;
		}

		if (sd->disguise){
			clif_displaymessage(sd->fd, msg_txt(sd,729)); // Cannot transform into monster while in disguise.
			return SCRIPT_CMD_FAILURE;
		}

		if (!strcmp(script_getfuncname(st), "active_transform")) {
			status_change_end(&sd->bl, SC_ACTIVE_MONSTER_TRANSFORM, INVALID_TIMER); // Clear previous
			sc_start2(NULL, &sd->bl, SC_ACTIVE_MONSTER_TRANSFORM, 100, mob_id, type, tick);
		} else {
			status_change_end(&sd->bl, SC_MONSTER_TRANSFORM, INVALID_TIMER); // Clear previous
			sc_start2(NULL, &sd->bl, SC_MONSTER_TRANSFORM, 100, mob_id, type, tick);
		}
		if (type != SC_NONE)
			sc_start4(NULL, &sd->bl, type, 100, val1, val2, val3, val4, tick);
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Attach script to player for certain duration
 * bonus_script "<script code>",<duration>{,<flag>{,<type>{,<status_icon>{,<char_id>}}}};
 * @param "script code"
 * @param duration
 * @param flag
 * @param icon
 * @param char_id
* @author [Cydh]
 **/
BUILDIN_FUNC(bonus_script) {
	uint16 flag = 0;
	int16 icon = EFST_BLANK;
	uint32 dur;
	uint8 type = 0;
	TBL_PC* sd;
	const char *script_str = NULL;
	struct s_bonus_script_entry *entry = NULL;

#ifndef Pandas_BonusScript_Unique_ID
	if ( !script_charid2sd(7,sd) )
		return SCRIPT_CMD_FAILURE;
#else
	if ( !script_charid2sd(7,sd) ) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
#endif // Pandas_BonusScript_Unique_ID
	
	script_str = script_getstr(st,2);
	dur = 1000 * abs(script_getnum(st,3));
	FETCH(4, flag);
	FETCH(5, type);
	FETCH(6, icon);

	// No Script string, No Duration!
	if (script_str[0] == '\0' || !dur) {
		ShowError("buildin_bonus_script: Invalid! Script: \"%s\". Duration: %d\n", script_str, dur);
#ifdef Pandas_BonusScript_Unique_ID
		script_pushint(st, 0);
#endif // Pandas_BonusScript_Unique_ID
		return SCRIPT_CMD_FAILURE;
	}

	if (strlen(script_str) >= MAX_BONUS_SCRIPT_LENGTH) {
		ShowError("buildin_bonus_script: Script string to long: \"%s\".\n", script_str);
#ifdef Pandas_BonusScript_Unique_ID
		script_pushint(st, 0);
#endif // Pandas_BonusScript_Unique_ID
		return SCRIPT_CMD_FAILURE;
	}

	if (icon <= EFST_BLANK || icon >= EFST_MAX)
		icon = EFST_BLANK;

	if ((entry = pc_bonus_script_add(sd, script_str, dur, (enum efst_type)icon, flag, type))) {
		linkdb_insert(&sd->bonus_script.head, (void *)((intptr_t)entry), entry);
		status_calc_pc(sd,SCO_NONE);

#ifdef Pandas_BonusScript_Unique_ID
		script_pushint(st, entry->bonus_id);
#endif // Pandas_BonusScript_Unique_ID
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
* Removes all bonus script from player
* bonus_script_clear {<flag>,{<char_id>}};
* @param flag 0 - Except permanent bonus, 1 - With permanent bonus
* @param char_id Clear script from this character
* @author [Cydh]
*/
BUILDIN_FUNC(bonus_script_clear) {
	TBL_PC* sd;
	bool flag = false;

	if (!script_charid2sd(3,sd))
		return SCRIPT_CMD_FAILURE;

	if (script_hasdata(st,2))
		flag = script_getnum(st,2) != 0;

	pc_bonus_script_clear(sd,(flag ? BSF_PERMANENT : BSF_REM_ALL));
	return SCRIPT_CMD_SUCCESS;
}

/** Allows player to use atcommand while talking with NPC
* enable_command;
* @author [Cydh], [Kichi]
*/
BUILDIN_FUNC(enable_command) {
	TBL_PC* sd;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;
	sd->state.disable_atcommand_on_npc = 0;
	return SCRIPT_CMD_SUCCESS;
}

/** Prevents player to use atcommand while talking with NPC
* disable_command;
* @author [Cydh], [Kichi]
*/
BUILDIN_FUNC(disable_command) {
	TBL_PC* sd;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;
	sd->state.disable_atcommand_on_npc = 1;
	return SCRIPT_CMD_SUCCESS;
}

/** Get the information of the members of a guild by type.
 * getguildmember <guild_id>{,<type>};
 * @param guild_id: ID of guild
 * @param type: Type of option (optional)
 */
BUILDIN_FUNC(getguildmember)
{
	struct guild *g = NULL;
	uint8 j = 0;

	g = guild_search(script_getnum(st,2));

	if (g) {
		uint8 i, type = 0;
		struct script_data *data = NULL;
		char *varname = NULL;

		if (script_hasdata(st,3))
 			type = script_getnum(st,3);

		if (script_hasdata(st,4)) {
			data = script_getdata(st, 4);
			if (!data_isreference(data)) {
				ShowError("buildin_getguildmember: Error in argument! Please give a variable to store values in.\n");
				return SCRIPT_CMD_FAILURE;
			}
			varname = reference_getname(data);
			if (type <= 0 && varname[strlen(varname)-1] != '$') {
				ShowError("buildin_getguildmember: The array %s is not string type.\n", varname);
				return SCRIPT_CMD_FAILURE;
			}
			if (not_server_variable(*varname)) {
				struct map_session_data *sd;

				if (!script_rid2sd(sd)) {
					ShowError("buildin_getguildmember: Cannot use a player variable '%s' if no player is attached.\n", varname);
					return SCRIPT_CMD_FAILURE;
				}
			}
		}

		for (i = 0; i < MAX_GUILD; i++) {
			if (g->member[i].account_id) {
				switch (type) {
					case 2:
						if (data)
							setd_sub_num( st, NULL, varname, j, g->member[i].account_id, data->ref );
						else
							mapreg_setreg(reference_uid(add_str("$@guildmemberaid"), j),g->member[i].account_id);
						break;
					case 1:
						if (data)
							setd_sub_num( st, NULL, varname, j, g->member[i].char_id, data->ref );
						else
							mapreg_setreg(reference_uid(add_str("$@guildmembercid"), j), g->member[i].char_id);
						break;
					default:
						if (data)
							setd_sub_str( st, NULL, varname, j, g->member[i].name, data->ref );
						else
							mapreg_setregstr(reference_uid(add_str("$@guildmembername$"), j), g->member[i].name);
						break;
				}
				j++;
			}
		}
	}
	mapreg_setreg(add_str("$@guildmembercount"), j);
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}

/** Adds spirit ball to player for 'duration' in milisecond
* addspiritball <count>,<duration>{,<char_id>};
* @param count How many spirit ball will be added
* @param duration How long spiritball is active until it disappears
* @param char_id Target player (Optional)
* @author [Cydh]
*/
BUILDIN_FUNC(addspiritball) {
	uint8 i, count;
	uint16 duration;
	struct map_session_data *sd = NULL;

	if (script_hasdata(st,4)) {
		if (!script_isstring(st,4))
			sd = map_charid2sd(script_getnum(st,4));
		else
			sd = map_nick2sd(script_getstr(st,4),false);
	}
	else
		script_rid2sd(sd);
	if (!sd)
		return SCRIPT_CMD_FAILURE;

	count = script_getnum(st,2);

	if (count == 0)
		return SCRIPT_CMD_SUCCESS;

	duration = script_getnum(st,3);

	for (i = 0; i < count; i++)
		pc_addspiritball(sd,duration,10);
	return SCRIPT_CMD_SUCCESS;
}

/** Deletes the spirit ball(s) from player
* delspiritball <count>{,<char_id>};
* @param count How many spirit ball will be deleted
* @param char_id Target player (Optional)
* @author [Cydh]
*/
BUILDIN_FUNC(delspiritball) {
	uint8 count;
	struct map_session_data *sd = NULL;
	
	if (script_hasdata(st,3)) {
		if (!script_isstring(st,3))
			sd = map_charid2sd(script_getnum(st,3));
		else
			sd = map_nick2sd(script_getstr(st,3),false);
	}
	else
		script_rid2sd(sd);
	if (!sd)
		return SCRIPT_CMD_FAILURE;

	count = script_getnum(st,2);

	if (count == 0)
		count = 1;

	pc_delspiritball(sd,count,0);
	return SCRIPT_CMD_SUCCESS;
}

/** Counts the spirit ball that player has
* countspiritball {<char_id>};
* @param char_id Target player (Optional)
* @author [Cydh]
*/
BUILDIN_FUNC(countspiritball) {
	struct map_session_data *sd;

	if (script_hasdata(st,2)) {
		if (!script_isstring(st,2))
			sd = map_charid2sd(script_getnum(st,2));
		else
			sd = map_nick2sd(script_getstr(st,2),false);
	}
	else
		script_rid2sd(sd);
	if (!sd)
		return SCRIPT_CMD_FAILURE;
	script_pushint(st,sd->spiritball);
	return SCRIPT_CMD_SUCCESS;
}

/** Merges separated stackable items because of guid
* mergeitem {<char_id>};
* @param char_id Char ID (Optional)
* @author [Cydh]
*/
BUILDIN_FUNC(mergeitem) {
	struct map_session_data *sd;

	if (!script_charid2sd(2, sd))
		return SCRIPT_CMD_FAILURE;

	clif_merge_item_open(sd);
	return SCRIPT_CMD_SUCCESS;
}

/** Merges separated stackable items because of guid (Unofficial)
* mergeitem2 {<item_id>,{<char_id>}};
* mergeitem2 {"<item name>",{<char_id>}};
* @param item Item ID/Name for merging specific item (Optional)
* @author [Cydh]
*/
BUILDIN_FUNC(mergeitem2) {
	struct map_session_data *sd;
	struct item *items = NULL;
	uint16 i, count = 0;
	t_itemid nameid = 0;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	if (script_hasdata(st, 2)) {
		if (script_isstring(st, 2)) {// "<item name>"
			const char *name = script_getstr(st, 2);
			std::shared_ptr<item_data> id = item_db.searchname( name );

			if( id == nullptr ){
				ShowError("buildin_mergeitem2: Nonexistant item %s requested.\n", name);
				script_pushint(st, count);
				return SCRIPT_CMD_FAILURE;
			}
			nameid = id->nameid;
		} else {// <item id>
			nameid = script_getnum(st, 2);
			if (!itemdb_exists(nameid)) {
				ShowError("buildin_mergeitem: Nonexistant item %u requested.\n", nameid);
				script_pushint(st, count);
				return SCRIPT_CMD_FAILURE;
			}
		}
	}

	for (i = 0; i < MAX_INVENTORY; i++) {
		struct item *it = &sd->inventory.u.items_inventory[i];

		if (!it || !it->unique_id || it->expire_time || !itemdb_isstackable(it->nameid))
			continue;
		if ((!nameid || (nameid == it->nameid))) {
			uint16 k;
			if (!count) {
				CREATE(items, struct item, 1);
				memcpy(&items[count++], it, sizeof(struct item));
				pc_delitem(sd, i, it->amount, 0, 0, LOG_TYPE_NPC);
				continue;
			}
			for (k = 0; k < count; k++) {
				// Find Match
				if (&items[k] && items[k].nameid == it->nameid && items[k].bound == it->bound && memcmp(items[k].card, it->card, sizeof(it->card)) == 0) {
					items[k].amount += it->amount;
					pc_delitem(sd, i, it->amount, 0, 0, LOG_TYPE_NPC);
					break;
				}
			}
			if (k >= count) {
				// New entry
				RECREATE(items, struct item, count+1);
				memcpy(&items[count++], it, sizeof(struct item));
				pc_delitem(sd, i, it->amount, 0, 0, LOG_TYPE_NPC);
			}
		}
	}

	if (!items) // Nothing todo here
		return SCRIPT_CMD_SUCCESS;

	// Retrieve the items
	for (i = 0; i < count; i++) {
		uint8 flag = 0;
		if (!&items[i])
			continue;
		items[i].id = 0;
		items[i].unique_id = 0;
		if ((flag = pc_additem(sd, &items[i], items[i].amount, LOG_TYPE_NPC)))
			clif_additem(sd, i, items[i].amount, flag);
	}
	aFree(items);
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Update an entry from NPC shop.
 * npcshopupdate "<name>",<item_id>,<price>{,<stock>}
 **/
BUILDIN_FUNC(npcshopupdate) {
	const char* npcname = script_getstr(st, 2);
	struct npc_data* nd = npc_name2id(npcname);
	t_itemid nameid = script_getnum(st, 3);
	int price = script_getnum(st, 4);
#if PACKETVER >= 20131223
	int32 stock = script_hasdata(st,5) ? script_getnum(st,5) : -1;

	if( stock < -1 ){
		ShowError( "buildin_npcshopupdate: Invalid stock amount in marketshop '%s'.\n", nd->exname );
		script_pushint( st, 0 );
		return SCRIPT_CMD_FAILURE;
	}
#endif
	int i;

	if( !nd || ( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_CASHSHOP && nd->subtype != NPCTYPE_ITEMSHOP && nd->subtype != NPCTYPE_POINTSHOP && nd->subtype != NPCTYPE_MARKETSHOP ) ) { // Not found.
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}
	
	if (!nd->u.shop.count) {
		ShowError("buildin_npcshopupdate: Attempt to update empty shop from '%s'.\n", nd->exname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	for (i = 0; i < nd->u.shop.count; i++) {
		if (nd->u.shop.shop_item[i].nameid == nameid) {

			if (price != 0)
				nd->u.shop.shop_item[i].value = price;
#if PACKETVER >= 20131223
			if (nd->subtype == NPCTYPE_MARKETSHOP) {
				nd->u.shop.shop_item[i].qty = stock;
				npc_market_tosql(nd->exname, &nd->u.shop.shop_item[i]);
			}
#endif
		}
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

// Clan System
BUILDIN_FUNC(clan_join){
	struct map_session_data *sd;
	int clan_id = script_getnum(st,2);

	if( !script_charid2sd( 3, sd ) ){
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if( clan_member_join( sd, clan_id, sd->status.account_id, sd->status.char_id ) )
		script_pushint(st, true);
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(clan_leave){
	struct map_session_data *sd;

	if( !script_charid2sd( 2, sd ) ){
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if( clan_member_leave( sd, sd->status.clan_id, sd->status.account_id, sd->status.char_id ) )
		script_pushint(st, true);
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get rid from running script.
 * getattachedrid();
 **/
BUILDIN_FUNC(getattachedrid) {
	script_pushint(st, st->rid);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get variable from a Player
 * getvar <variable>,<char_id>;
 */
BUILDIN_FUNC(getvar) {
	int char_id = script_getnum(st, 3);
	struct map_session_data *sd = map_charid2sd(char_id);
	struct script_data *data = NULL;
	const char *name = NULL;

	if (!sd) {
		ShowError("buildin_getvar: No player found with char id '%d'.\n", char_id);
		return SCRIPT_CMD_FAILURE;
	}

	data = script_getdata(st, 2);
	if (!data_isreference(data)) {
		ShowError("buildin_getvar: Not a variable\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	name = reference_getname(data);

	if (reference_toparam(data)) {
		ShowError("buildin_getvar: '%s' is a parameter - please use readparam instead\n", name);
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if (name[0] == '.' || name[0] == '$' || name[0] == '\'') { // Not a PC variable
		ShowError("buildin_getvar: Invalid scope (not PC variable)\n");
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	get_val_(st, data, sd);
	if (data_isint(data))
		script_pushint(st, conv_num_(st, data, sd));
	else
		script_pushstrcopy(st, conv_str_(st, data, sd));

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Display script message
 * showscript "<message>"{,<GID>,<flag>};
 * @param flag: Specify target
 *   AREA - Message is sent to players in the vicinity of the source (default).
 *   SELF - Message is sent only to player attached.
 **/
BUILDIN_FUNC(showscript) {
	struct block_list *bl = NULL;
	const char *msg = script_getstr(st,2);
	int id = 0;
	send_target target = AREA;

	if (script_hasdata(st,3)) {
		id = script_getnum(st,3);
		bl = map_id2bl(id);
	}
	else {
		bl = st->rid ? map_id2bl(st->rid) : map_id2bl(st->oid);
	}

	if (!bl) {
		ShowError("buildin_showscript: Script not attached. (id=%d, rid=%d, oid=%d)\n", id, st->rid, st->oid);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 4)) {
		target = static_cast<send_target>(script_getnum(st, 4));
		if (target == SELF && map_id2sd(bl->id) == NULL) {
			ShowWarning("script: showscript: self can't be used for non-players objects.\n");
			return SCRIPT_CMD_FAILURE;
		}
	}

	clif_showscript(bl, msg, target);

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Ignore the SECURE_NPCTIMEOUT function.
 * ignoretimeout <flag>{,<char_id>};
 */
BUILDIN_FUNC(ignoretimeout)
{
#ifdef SECURE_NPCTIMEOUT
	struct map_session_data *sd = NULL;

	if (script_hasdata(st,3)) {
		if (!script_isstring(st,3))
			sd = map_charid2sd(script_getnum(st,3));
		else
			sd = map_nick2sd(script_getstr(st,3),false);
	} else
		script_rid2sd(sd);

	if (!sd)
		return SCRIPT_CMD_FAILURE;

	sd->state.ignoretimeout = script_getnum(st,2) > 0;
#endif
	return SCRIPT_CMD_SUCCESS;
}

/**
 * geteleminfo <type>{,<char_id>};
 **/
BUILDIN_FUNC(geteleminfo) {
	TBL_ELEM *ed = NULL;
	TBL_PC *sd = NULL;
	int type = script_getnum(st,2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!(ed = sd->ed)) {
		//ShowDebug("buildin_geteleminfo: Player doesn't have Elemental.\n");
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch (type) {
		case 0: script_pushint(st, ed->elemental.elemental_id); break;
		case 1: script_pushint(st, ed->bl.id); break;
		default:
			ShowError("buildin_geteleminfo: Invalid type '%d'.\n", type);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * opendressroom(<flag>{,<char_id>});
 */
BUILDIN_FUNC(opendressroom)
{
#if PACKETVER >= 20150513
	int flag = 1;
    TBL_PC* sd;

	if( script_hasdata(st,2) )
		flag = script_getnum(st,2);

    if (!script_charid2sd(3, sd))
        return SCRIPT_CMD_FAILURE;

    clif_dressing_room(sd, flag);

    return SCRIPT_CMD_SUCCESS;
#else
    return SCRIPT_CMD_FAILURE;
#endif
}

/**
 * navigateto("<map>"{,<x>,<y>,<flag>,<hide_window>,<monster_id>,<char_id>});
 */
BUILDIN_FUNC(navigateto){
#if PACKETVER >= 20111010
	TBL_PC* sd;
	const char *mapname;
	uint16 x = 0, y = 0, monster_id = 0;
	uint8 flag = NAV_KAFRA_AND_AIRSHIP;
	bool hideWindow = true;

	mapname = script_getstr(st,2);

	if( script_hasdata(st,3) )
		x = script_getnum(st,3);
	if( script_hasdata(st,4) )
		y = script_getnum(st,4);
	if( script_hasdata(st,5) )
		flag = (uint8)script_getnum(st,5);
	if( script_hasdata(st,6) )
		hideWindow = script_getnum(st,6) ? true : false;
	if( script_hasdata(st,7) )
		monster_id = script_getnum(st,7);

	if (!script_charid2sd(8, sd))
        return SCRIPT_CMD_FAILURE;

	clif_navigateTo(sd,mapname,x,y,flag,hideWindow,monster_id);

	return SCRIPT_CMD_SUCCESS;
#else
	return SCRIPT_CMD_FAILURE;
#endif
}

/**
 * adopt("<parent_name>","<baby_name>");
 * adopt(<parent_id>,<baby_id>);
 * https://rathena.org/board/topic/104014-suggestion-add-adopt-or-etc/
 */
BUILDIN_FUNC(adopt)
{
	TBL_PC *sd, *b_sd;
	enum adopt_responses response;

	if (script_isstring(st, 2)) {
		const char *name = script_getstr(st, 2);

		sd = map_nick2sd(name,false);
		if (sd == NULL) {
			ShowError("buildin_adopt: Non-existant parent character %s requested.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		uint32 char_id = script_getnum(st, 2);

		sd = map_charid2sd(char_id);
		if (sd == NULL) {
			ShowError("buildin_adopt: Non-existant parent character %d requested.\n", char_id);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (script_isstring(st, 3)) {
		const char *name = script_getstr(st, 3);

		b_sd = map_nick2sd(name,false);
		if (b_sd == NULL) {
			ShowError("buildin_adopt: Non-existant baby character %s requested.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		uint32 char_id = script_getnum(st, 3);

		b_sd = map_charid2sd(char_id);
		if (b_sd == NULL) {
			ShowError("buildin_adopt: Non-existant baby character %d requested.\n", char_id);
			return SCRIPT_CMD_FAILURE;
		}
	}

	response = pc_try_adopt(sd, map_charid2sd(sd->status.partner_id), b_sd);

	if (response == ADOPT_ALLOWED) {
		TBL_PC *p_sd = map_charid2sd(sd->status.partner_id);

		b_sd->adopt_invite = sd->status.account_id;
		clif_Adopt_request(b_sd, sd, p_sd->status.account_id);
		script_pushint(st, ADOPT_ALLOWED);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, response);
	return SCRIPT_CMD_FAILURE;
}

/**
 * Returns the minimum or maximum of all the given parameters for integer variables.
 *
 * min( <value or array>{,value or array 2,...} );
 * minimum( <value or array>{,value or array 2,...} );
 * max( <value or array>{,value or array 2,...} );
 * maximum( <value or array>{,value or array 2,...} );
*/
BUILDIN_FUNC(minmax){
	char *functionname;
	unsigned int i;
	int64 value;
	// Function pointer for our comparison function (either min or max at the moment)
	int64 (*func)(int64, int64);
	
	// Get the real function name
	functionname = script_getfuncname(st);
	
	// Our data should start at offset 2
	i = 2;

	if( !script_hasdata( st, i ) ){
		ShowError( "buildin_%s: no arguments given!\n", functionname );
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if( strnicmp( functionname, "min", strlen( "min" ) ) == 0 ){
		value = SCRIPT_INT_MAX;
		func = i64min;
	}else if( strnicmp( functionname, "max", strlen( "max" ) ) == 0 ){
		value = SCRIPT_INT_MIN;
		func = i64max;
	}else{
		ShowError( "buildin_%s: Unknown call case for min/max!\n", functionname );
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	// As long as we have data on our script stack
	while( script_hasdata(st,i) ){
		struct script_data *data;
		
		// Get the next piece of data from the script stack
		data = script_getdata( st, i );

		// Is the current parameter a single integer?
		if( data_isint( data ) ){
			value = func( value, script_getnum( st, i ) );
		// Is the current parameter an array variable?
		}else if( data_isreference( data ) ){
			const char *name;
			struct map_session_data* sd;
			unsigned int start, end;

			// Get the name of the variable
			name = reference_getname(data);

			// Check if it's a string variable
			if( is_string_variable( name ) ){
				ShowError( "buildin_%s: illegal type, need integer!\n", functionname );
				script_reportdata( data );
				st->state = END;
				return SCRIPT_CMD_FAILURE;
			}

			// Get the session data, if a player is attached
			sd = st->rid ? map_id2sd(st->rid) : NULL;

			if (not_server_variable(*name) && !script_rid2sd(sd)) {
				ShowError("buildin_%s: Cannot use a player variable '%s' if no player is attached.\n", functionname, name);
				return SCRIPT_CMD_FAILURE;
			}

			// Try to find the array's source pointer
			if( !script_array_src( st, sd, name, reference_getref( data ) ) ){
				ShowError( "buildin_%s: not a array!\n", functionname );
				script_reportdata( data );
				st->state = END;
				return SCRIPT_CMD_FAILURE;
			}

			// Get the start and end indices of the array
			start = reference_getindex( data );
			end = script_array_highest_key( st, sd, name, reference_getref( data ) );

			// Skip empty arrays
			if( start < end ){
				int id;
				
				// For getting the values we need the id of the array
				id = reference_getid( data );

				// Loop through each value stored in the array
				for( ; start < end; start++ ){
					value = func( value, get_val2_num( st, reference_uid( id, start ), reference_getref( data ) ) );
				}
			}
			else {
				value = func( value, 0 );
			}
		}else{
			ShowError( "buildin_%s: not a supported data type!\n", functionname );
			script_reportdata( data );
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}

		// Continue with the next stack entry
		i++;
	}

	script_pushint( st, value );

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Safety Base/Job EXP addition than using `set BaseExp,n;` or `set JobExp,n;`
 * Unlike `getexp` that affected by some adjustments.
 * getexp2 <base_exp>,<job_exp>{,<char_id>};
 * @author [Cydh]
 **/
BUILDIN_FUNC(getexp2) {
	TBL_PC *sd = NULL;
	int64 base_exp = script_getnum64(st, 2);
	int64 job_exp = script_getnum64(st, 3);

	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	if( base_exp == 0 && job_exp == 0 ){
		ShowError( "buildin_getexp2: Called with base and job exp 0.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if (base_exp > 0)
		pc_gainexp(sd, NULL, base_exp, 0, 2);
	else if (base_exp < 0)
		pc_lostexp(sd, base_exp * -1, 0);

	if (job_exp > 0)
		pc_gainexp(sd, NULL, 0, job_exp, 2);
	else if (job_exp < 0)
		pc_lostexp(sd, 0, job_exp * -1);
	return SCRIPT_CMD_SUCCESS;
}

/**
* Force stat recalculation of sd
* recalculatestat;
* @author [secretdataz]
**/
BUILDIN_FUNC(recalculatestat) {
	TBL_PC* sd;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;

	status_calc_pc(sd, SCO_FORCE);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(hateffect){
#if PACKETVER_MAIN_NUM >= 20150507 || PACKETVER_RE_NUM >= 20150429 || defined(PACKETVER_ZERO)
	struct map_session_data* sd;

	if( !script_rid2sd(sd) )
		return SCRIPT_CMD_FAILURE;

	int16 effectID = script_getnum(st,2);
	bool enable = script_getnum(st,3) ? true : false;

	if( effectID <= HAT_EF_MIN || effectID >= HAT_EF_MAX ){
		ShowError( "buildin_hateffect: unsupported hat effect id %d\n", effectID );
		return SCRIPT_CMD_FAILURE;
	}

	auto it = util::vector_get( sd->hatEffects, effectID );

	if( enable ){
		if( it != sd->hatEffects.end() ){
			return SCRIPT_CMD_SUCCESS;
		}

		sd->hatEffects.push_back( effectID );
	}else{
		if( it == sd->hatEffects.end() ){
			return SCRIPT_CMD_SUCCESS;
		}

		util::vector_erase_if_exists( sd->hatEffects, effectID );
	}

	if( !sd->state.connect_new ){
		clif_hat_effect_single( sd, effectID, enable );
	}

#endif
	return SCRIPT_CMD_SUCCESS;
}

/**
* Retrieves param of current random option. Intended for random option script only.
* getrandomoptinfo(<type>);
* @author [secretdataz]
**/
BUILDIN_FUNC(getrandomoptinfo) {
	struct map_session_data *sd;
	int val;
	if (script_rid2sd(sd) && current_equip_item_index != -1 && current_equip_opt_index != -1 && sd->inventory.u.items_inventory[current_equip_item_index].option[current_equip_opt_index].id) {
		int param = script_getnum(st, 2);

		switch (param) {
			case ROA_ID:
				val = sd->inventory.u.items_inventory[current_equip_item_index].option[current_equip_opt_index].id;
				break;
			case ROA_VALUE:
				val = sd->inventory.u.items_inventory[current_equip_item_index].option[current_equip_opt_index].value;
				break;
			case ROA_PARAM:
				val = sd->inventory.u.items_inventory[current_equip_item_index].option[current_equip_opt_index].param;
				break;
			default:
				ShowWarning("buildin_getrandomoptinfo: Invalid attribute type %d (Max %d).\n", param, MAX_ITEM_RDM_OPT);
				val = 0;
				break;
		}
		script_pushint(st, val);
	}
	else {
		script_pushint(st, 0);
	}
	return SCRIPT_CMD_SUCCESS;
}

/**
* Retrieves a random option on an equipped item.
* getequiprandomoption(<equipment slot>,<index>,<type>{,<char id>});
* @author [secretdataz]
*/
BUILDIN_FUNC(getequiprandomoption) {
	struct map_session_data *sd;
	int val;
	short i = -1;
	int pos = script_getnum(st, 2);
	int index = script_getnum(st, 3);
	int type = script_getnum(st, 4);
	if (!script_charid2sd(5, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (index < 0 || index >= MAX_ITEM_RDM_OPT) {
		ShowError("buildin_getequiprandomoption: Invalid random option index %d.\n", index);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (equip_index_check(pos))
		i = pc_checkequip(sd, equip_bitmask[pos]);
	if (i < 0) {
		ShowError("buildin_getequiprandomoption: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	switch (type) {
		case ROA_ID:
			val = sd->inventory.u.items_inventory[i].option[index].id;
			break;
		case ROA_VALUE:
			val = sd->inventory.u.items_inventory[i].option[index].value;
			break;
		case ROA_PARAM:
			val = sd->inventory.u.items_inventory[i].option[index].param;
			break;
		default:
			ShowWarning("buildin_getequiprandomoption: Invalid attribute type %d (Max %d).\n", type, MAX_ITEM_RDM_OPT);
			val = 0;
			break;
	}
	script_pushint(st, val);
	return SCRIPT_CMD_SUCCESS;
}

/**
* Adds a random option on a random option slot on an equipped item and overwrites
* existing random option in the process.
* setrandomoption(<equipment slot>,<index>,<id>,<value>,<param>{,<char id>});
* @author [secretdataz]
*/
BUILDIN_FUNC(setrandomoption) {
	struct map_session_data *sd;
	int pos, index, id, value, param, ep;
	int i = -1;
	if (!script_charid2sd(7, sd))
		return SCRIPT_CMD_FAILURE;
	pos = script_getnum(st, 2);
	index = script_getnum(st, 3);
	id = script_getnum(st, 4);
	value = script_getnum(st, 5);
	param = script_getnum(st, 6);

	std::shared_ptr<s_random_opt_data> opt = random_option_db.find(static_cast<uint16>(id));

	if (opt == nullptr) {
		ShowError("buildin_setrandomoption: Random option ID %d does not exists.\n", id);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
	if (index < 0 || index >= MAX_ITEM_RDM_OPT) {
		ShowError("buildin_setrandomoption: Invalid random option index %d.\n", index);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
	if (equip_index_check(pos))
		i = pc_checkequip(sd, equip_bitmask[pos]);
	if (i >= 0) {
		ep = sd->inventory.u.items_inventory[i].equip;
		log_pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->inventory.u.items_inventory[i]);
		pc_unequipitem(sd, i, 2);
		sd->inventory.u.items_inventory[i].option[index].id = id;
		sd->inventory.u.items_inventory[i].option[index].value = value;
		sd->inventory.u.items_inventory[i].option[index].param = param;
		clif_delitem(sd, i, 1, 3);
		log_pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->inventory.u.items_inventory[i]);
		clif_additem(sd, i, 1, 0);
		pc_equipitem(sd, i, ep);
		script_pushint(st, 1);
		return SCRIPT_CMD_SUCCESS;
	}

	ShowError("buildin_setrandomoption: No item equipped at pos %d (CID=%d/AID=%d).\n", pos, sd->status.char_id, sd->status.account_id);
	script_pushint(st, 0);
	return SCRIPT_CMD_FAILURE;
}

/// Returns the number of stat points needed to change the specified stat by val.
/// If val is negative, returns the number of stat points that would be needed to
/// raise the specified stat from (current value - val) to current value.
/// *needed_status_point(<type>,<val>{,<char id>});
/// @author [secretdataz]
BUILDIN_FUNC(needed_status_point) {
	struct map_session_data *sd;
	int type, val;
	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;
	type = script_getnum(st, 2);
	val = script_getnum(st, 3);

	script_pushint(st, pc_need_status_point(sd, type, val));
	return SCRIPT_CMD_SUCCESS;
}

/// Returns the number of trait stat points needed to change the specified trait stat by val.
/// If val is negative, returns the number of trait stat points that would be needed to
/// raise the specified trait stat from (current value - val) to current value.
/// *needed_trait_point(<type>,<val>{,<char id>});
BUILDIN_FUNC(needed_trait_point) {
	struct map_session_data *sd;

	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	int type = script_getnum( st, 2 );

	if( type < SP_POW || type > SP_CRT ){
		ShowError( "buildin_needed_trait_point: Unknown trait type %d\n", type );
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint( st, pc_need_trait_point( sd, type, script_getnum( st, 3 ) ) );

	return SCRIPT_CMD_SUCCESS;
}

/**
 * jobcanentermap("<mapname>"{,<JobID>});
 * Check if (player with) JobID can enter the map.
 * @param mapname Map name
 * @param JobID Player's JobID (optional)
 **/
BUILDIN_FUNC(jobcanentermap) {
	const char *mapname = script_getstr(st, 2);
	int mapidx = mapindex_name2id(mapname), m = -1;
	int jobid = 0;
	TBL_PC *sd = NULL;

	if (!mapidx) {// Invalid map
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}
	m = map_mapindex2mapid(mapidx);
	if (m == -1) { // Map is on different map server
		ShowError("buildin_jobcanentermap: Map '%s' is not found in this server.\n", mapname);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 3)) {
		jobid = script_getnum(st, 3);
	} else {
		if (!script_rid2sd(sd)) {
			script_pushint(st, false);
			return SCRIPT_CMD_FAILURE;
		}
		jobid = sd->status.class_;
	}

	script_pushint(st, pc_job_can_entermap((enum e_job)jobid, m, sd ? pc_get_group_level(sd) : 0));
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Return alliance information between the two guilds.
 * getguildalliance(<guild id1>,<guild id2>);
 * Return values:
 *	-2 - Guild ID1 does not exist
 *	-1 - Guild ID2 does not exist
 *	 0 - Both guilds have no relation OR guild ID aren't given
 *	 1 - Both guilds are allies
 *	 2 - Both guilds are antagonists
 */
BUILDIN_FUNC(getguildalliance)
{
	struct guild *guild_data1, *guild_data2;
	int guild_id1, guild_id2, i = 0;

	guild_id1 = script_getnum(st,2);
	guild_id2 = script_getnum(st,3);

	if (guild_id1 < 1 || guild_id2 < 1) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (guild_id1 == guild_id2) {
		script_pushint(st, 1);
		return SCRIPT_CMD_SUCCESS;
	}

	guild_data1 = guild_search(guild_id1);
	guild_data2 = guild_search(guild_id2);

	if (guild_data1 == NULL) {
		ShowWarning("buildin_getguildalliance: Requesting non-existent GuildID1 '%d'.\n", guild_id1);
		script_pushint(st, -2);
		return SCRIPT_CMD_FAILURE;
	}
	if (guild_data2 == NULL) {
		ShowWarning("buildin_getguildalliance: Requesting non-existent GuildID2 '%d'.\n", guild_id2);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	ARR_FIND(0, MAX_GUILDALLIANCE, i, guild_data1->alliance[i].guild_id == guild_id2);
	if (i == MAX_GUILDALLIANCE) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (guild_data1->alliance[i].opposition)
		script_pushint(st, 2);
	else
		script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}

/*
 * openstorage2 <storage_id>,<mode>{,<account_id>}
 * mode @see enum e_storage_mode
 **/
BUILDIN_FUNC(openstorage2) {
	int stor_id = script_getnum(st, 2);
	TBL_PC *sd = NULL;

	if (!script_accid2sd(4, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (!storage_exists(stor_id)) {
		ShowError("buildin_openstorage2: Invalid storage_id '%d'!\n", stor_id);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, storage_premiumStorage_load(sd, stor_id, script_getnum(st, 3)));
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Create a new channel
 * channel_create "<chname>","<alias>"{,"<password>"{<option>{,<delay>{,<color>{,<char_id>}}}}};
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_create) {
	struct Channel tmp_chan, *ch = NULL;
	const char *chname = script_getstr(st,2), *pass = NULL;
	int i = channel_chk((char*)chname, NULL, 3);
	TBL_PC *sd = NULL;

	if (i != 0) {
		ShowError("buildin_channel_create: Channel name '%s' is invalid. Errno %d\n", chname, i);
		script_pushint(st,i);
		return SCRIPT_CMD_FAILURE;
	}

	memset(&tmp_chan, 0, sizeof(struct Channel));

	if (script_hasdata(st,8)) {
		tmp_chan.char_id = script_getnum(st,8);
		if (!(sd = map_charid2sd(tmp_chan.char_id))) {
			ShowError("buildin_channel_create: Player with char id '%d' is not found.\n", tmp_chan.char_id);
			script_pushint(st,-5);
			return SCRIPT_CMD_FAILURE;
		}
		tmp_chan.type = CHAN_TYPE_PRIVATE;
		i = 1;
	}
	else {
		tmp_chan.type = CHAN_TYPE_PUBLIC;
		i = 0;
	}

	safestrncpy(tmp_chan.name, chname+1, sizeof(tmp_chan.name));
	safestrncpy(tmp_chan.alias, script_getstr(st,3), sizeof(tmp_chan.alias));
	if (script_hasdata(st,4) && (pass = script_getstr(st,4)) && strcmpi(pass,"null") != 0)
		safestrncpy(tmp_chan.pass, pass, sizeof(tmp_chan.pass));
	if (script_hasdata(st,5))
		tmp_chan.opt = script_getnum(st,5);
	else
		tmp_chan.opt = i ? channel_config.private_channel.opt : CHAN_OPT_BASE;
	if (script_hasdata(st,6))
		tmp_chan.msg_delay = script_getnum(st,6);
	else
		tmp_chan.msg_delay = i ? channel_config.private_channel.delay : 1000;
	if (script_hasdata(st,7))
		tmp_chan.color = script_getnum(st,7);
	else
		tmp_chan.color = i ? channel_config.private_channel.color : channel_getColor("Default");

	if (!(ch = channel_create(&tmp_chan))) {
		ShowError("buildin_channel_create: Cannot create channel '%s'.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}
	if (tmp_chan.char_id)
		channel_join(ch, sd);
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set channel option
 * channel_setopt "<chname>",<option>,<value>;
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_setopt) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);
	int opt = script_getnum(st,3), value = script_getnum(st,4);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_setopt: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	switch (opt) {
		case CHAN_OPT_ANNOUNCE_SELF:
		case CHAN_OPT_ANNOUNCE_JOIN:
		case CHAN_OPT_ANNOUNCE_LEAVE:
		case CHAN_OPT_COLOR_OVERRIDE:
		case CHAN_OPT_CAN_CHAT:
		case CHAN_OPT_CAN_LEAVE:
		case CHAN_OPT_AUTOJOIN:
			if (value)
				ch->opt |= opt;
			else
				ch->opt &= ~opt;
			break;
		case CHAN_OPT_MSG_DELAY:
			ch->msg_delay = value;
			break;
		default:
			ShowError("buildin_channel_setopt: Invalid option %d!\n", opt);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get channel options
 * channel_getopt <chname>,<option>;
 */
BUILDIN_FUNC(channel_getopt) {
	Channel *ch;
	const char *chname = script_getstr(st, 2);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_getopt: Channel name '%s' is invalid.\n", chname);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	int opt = script_getnum(st, 3);

	switch (opt) {
		case CHAN_OPT_ANNOUNCE_SELF:
		case CHAN_OPT_ANNOUNCE_JOIN:
		case CHAN_OPT_ANNOUNCE_LEAVE:
		case CHAN_OPT_COLOR_OVERRIDE:
		case CHAN_OPT_CAN_CHAT:
		case CHAN_OPT_CAN_LEAVE:
		case CHAN_OPT_AUTOJOIN:
			script_pushint(st, (ch->opt & opt) != 0);
			break;
		case CHAN_OPT_MSG_DELAY:
			script_pushint(st, ch->msg_delay);
			break;
		default:
			ShowError("buildin_channel_getopt: Invalid option %d!\n", opt);
			script_pushint(st, false);
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set channel color
 * channel_setcolor "<chname>",<color>;
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_setcolor) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);
	int color = script_getnum(st,3);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_setcolor: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	ch->color = (color & 0x0000FF) << 16 | (color & 0x00FF00) | (color & 0xFF0000) >> 16;//RGB to BGR

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set channel password
 * channel_setpass "<chname>","<password>";
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_setpass) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2), *passwd = script_getstr(st,3);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_setpass: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (!passwd || !strcmpi(passwd,"null"))
		memset(ch->pass, '\0', sizeof(ch->pass));
	else
		safestrncpy(ch->pass, passwd, sizeof(ch->pass));
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Set authorized groups
 * channel_setgroup "<chname>",<group_id>{,...,<group_id>};
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_setgroup) {
	struct Channel *ch = NULL;
	const char *funcname = script_getfuncname(st), *chname = script_getstr(st,2);
	int i, n = 0, group = 0;

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_setgroup: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (funcname[strlen(funcname)-1] == '2') {
		struct script_data *data = script_getdata(st,3);
		const char *varname = reference_getname(data);
		int32 id, idx;

		if (varname[strlen(varname)-1] == '$') {
			ShowError("buildin_channel_setgroup: The array %s is not numeric type.\n", varname);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}

		if (not_server_variable(*varname)) {
			struct map_session_data *sd;

			if (!script_rid2sd(sd)) {
				ShowError("buildin_%s: Cannot use a player variable '%s' if no player is attached.\n", funcname, varname);
				return SCRIPT_CMD_FAILURE;
			}
		}

		n = script_array_highest_key(st, NULL, reference_getname(data), reference_getref(data));
		if (n < 1) {
			ShowError("buildin_channel_setgroup: No group id listed.\n");
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}

		if (ch->groups)
			aFree(ch->groups);
		ch->groups = NULL;
		ch->group_count = 0;

		id = reference_getid(data);
		idx = reference_getindex(data);
		for (i = 0; i < n; i++) {
			group = (int32)get_val2_num( st, reference_uid( id, idx + i ), reference_getref( data ) );
			if (group == 0) {
				script_pushint(st,1);
				return SCRIPT_CMD_SUCCESS;
			}
			RECREATE(ch->groups, unsigned short, ++ch->group_count);
			ch->groups[ch->group_count-1] = group;
			ShowInfo("buildin_channel_setgroup: (2) Added group %d. Num: %d\n", ch->groups[ch->group_count-1], ch->group_count);
		}
	}
	else {
		group = script_getnum(st,3);
		n = script_lastdata(st)-1;

		if (n < 1) {
			ShowError("buildin_channel_setgroup: Please input at least 1 group_id.\n");
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}

		if (ch->groups)
			aFree(ch->groups);
		ch->groups = NULL;
		ch->group_count = 0;

		if (group == 0) { // Removed group list
			script_pushint(st,1);
			return SCRIPT_CMD_SUCCESS;
		}

		CREATE(ch->groups, unsigned short, n);
		for (i = 3; i < n+2; i++) {
			ch->groups[ch->group_count++] = script_getnum(st,i);
			ShowInfo("buildin_channel_setgroup: (1) Added group %d. Num: %d\n", ch->groups[ch->group_count-1], ch->group_count);
		}
	}
	script_pushint(st,n);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Send message on channel
 * channel_chat "<chname>","<message>"{,<color>};
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_chat) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2), *msg = script_getstr(st,3);
	char output[CHAT_SIZE_MAX+1];
	unsigned long color = 0;

	// Check also local channels
	if (chname[0] != '#') { // By Map
		int m = mapindex_name2id(chname);
		if (!m || (m = map_mapindex2mapid(m)) < 0) {
			ShowError("buildin_channel_chat: Invalid map '%s'.\n", chname);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
		if (!(ch = map_getmapdata(m)->channel)) {
			ShowDebug("buildin_channel_chat: Map '%s' doesn't have local channel yet.\n", chname);
			script_pushint(st,0);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	else if (strcmpi(chname+1,channel_config.map_tmpl.name) == 0) {
		TBL_NPC *nd = map_id2nd(st->oid);
		if (!nd || nd->bl.m == -1) {
			ShowError("buildin_channel_chat: Floating NPC needs map name instead '%s'.\n", chname);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
		if (!(ch = map_getmapdata(nd->bl.m)->channel)) {
			ShowDebug("buildin_channel_chat: Map '%s' doesn't have local channel yet.\n", chname);
			script_pushint(st,0);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	else if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_chat: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (!ch) {
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	color = ch->color;
	FETCH(4, color);

	safesnprintf(output, CHAT_SIZE_MAX, "%s %s", ch->alias, msg);
	clif_channel_msg(ch,output,color);
	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Ban player from a channel
 * channel_ban "<chname>",<char_id>;
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_ban) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);
	unsigned int char_id = script_getnum(st,3);
	TBL_PC *tsd;

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_ban: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (ch->char_id == char_id) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	tsd = map_charid2sd(char_id);
	if (tsd && pc_has_permission(tsd,PC_PERM_CHANNEL_ADMIN)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!idb_exists(ch->banned, char_id)) {
		struct chan_banentry *cbe;
		char output[CHAT_SIZE_MAX+1];

		CREATE(cbe, struct chan_banentry, 1);
		cbe->char_id = char_id;
		if (tsd) {
			strcpy(cbe->char_name,tsd->status.name);
			channel_clean(ch,tsd,0);
			safesnprintf(output, CHAT_SIZE_MAX, msg_txt(tsd,769), ch->alias, tsd->status.name); // %s %s has been banned.
		}
		else
			safesnprintf(output, CHAT_SIZE_MAX, msg_txt(tsd,769), ch->alias, "****"); // %s %s has been banned.
		idb_put(ch->banned, char_id, cbe);
		clif_channel_msg(ch,output,ch->color);
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Ban player from a channel
 * channel_unban "<chname>",<char_id>;
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_unban) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);
	unsigned int char_id = script_getnum(st,3);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("buildin_channel_unban: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (ch->char_id == char_id) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (idb_exists(ch->banned, char_id)) {
		char output[CHAT_SIZE_MAX+1];
		TBL_PC *tsd = map_charid2sd(char_id);
		if (tsd)
			safesnprintf(output, CHAT_SIZE_MAX, msg_txt(tsd,770), ch->alias, tsd->status.name); // %s %s has been unbanned
		else
			safesnprintf(output, CHAT_SIZE_MAX, msg_txt(tsd,770), ch->alias, "****"); // %s %s has been unbanned.
		idb_remove(ch->banned, char_id);
		clif_channel_msg(ch,output,ch->color);
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Kick player from a channel
 * channel_kick "<chname>",<char_id>;
 * channel_kick "<chname>","<char_name>";
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_kick) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);
	TBL_PC *tsd = NULL;
	int res = 1;

	if (script_isstring(st, 3)) {
		const char *name = script_getstr(st, 3);

		if (!(tsd = map_nick2sd(name,false))) {
			ShowError("buildin_channel_kick: Player with nick '%s' is not online\n", name);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
	} else {
		int char_id = script_getnum(st, 3);

		if (!(tsd = map_charid2sd(char_id))) {
			ShowError("buildin_channel_kick: Player with char_id '%d' is not online\n", char_id);
			script_pushint(st,0);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (!(ch = channel_name2channel((char *)chname, tsd, 0))) {
		ShowError("buildin_channel_kick: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,0);
		return SCRIPT_CMD_FAILURE;
	}

	if (channel_pc_haschan(tsd, ch) < 0 || ch->char_id == tsd->status.char_id || pc_has_permission(tsd,PC_PERM_CHANNEL_ADMIN)) {
		script_pushint(st,0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch(ch->type){
		case CHAN_TYPE_ALLY: res = channel_pcquit(tsd,3); break;
		case CHAN_TYPE_MAP: res = channel_pcquit(tsd,4); break;
		default: res = channel_clean(ch,tsd,0); break;
	}
	
	if (res == 0) {
		char output[CHAT_SIZE_MAX+1];
		safesnprintf(output, CHAT_SIZE_MAX, msg_txt(tsd,889), ch->alias, tsd->status.name); // "%s %s is kicked"
		clif_channel_msg(ch,output,ch->color);
	}

	script_pushint(st,1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Delete a channel
 * channel_delete "<chname>";
 * @author [Cydh]
 **/
BUILDIN_FUNC(channel_delete) {
	struct Channel *ch = NULL;
	const char *chname = script_getstr(st,2);

	if (!(ch = channel_name2channel((char *)chname, NULL, 0))) {
		ShowError("channel_delete: Channel name '%s' is invalid.\n", chname);
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st,channel_delete(ch,true));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(unloadnpc) {
	const char *name;
	struct npc_data* nd;

	name = script_getstr(st, 2);
	nd = npc_name2id(name);

	if( nd == NULL ){
		ShowError( "buildin_unloadnpc: npc '%s' was not found.\n", name );
		return SCRIPT_CMD_FAILURE;
	} else if ( nd->bl.id == st->oid ) {
		// Supporting self-unload isn't worth the problem it may cause. [Secret]
		ShowError("buildin_unloadnpc: You cannot self-unload NPC '%s'.\n.", name);
		return SCRIPT_CMD_FAILURE;
	}

	npc_unload_duplicates(nd);
	npc_unload(nd, true);
#ifndef Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent
	npc_read_event_script();
#endif // Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Add an achievement to the player's log
 * achievementadd(<achievement ID>{,<char ID>});
 */
BUILDIN_FUNC(achievementadd) {
	struct map_session_data *sd;
	int achievement_id = script_getnum(st, 2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementadd: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if( !sd->state.pc_loaded ){
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementadd: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	if (achievement_add(sd, achievement_id))
		script_pushint(st, true);
	else
		script_pushint(st, false);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Removes an achievement on a player.
 * achievementremove(<achievement ID>{,<char ID>});
 * Just for Atemo. ;)
 */
BUILDIN_FUNC(achievementremove) {
	struct map_session_data *sd;
	int achievement_id = script_getnum(st, 2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementremove: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}

	if( !sd->state.pc_loaded ){
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementremove: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	if (achievement_remove(sd, achievement_id))
		script_pushint(st, true);
	else
		script_pushint(st, false);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Returns achievement progress
 * achievementinfo(<achievement ID>,<type>{,<char ID>});
 */
BUILDIN_FUNC(achievementinfo) {
	struct map_session_data *sd;
	int achievement_id = script_getnum(st, 2);

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementinfo: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if( !sd->state.pc_loaded ){
		script_pushint(st, false);
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementinfo: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	script_pushint(st, achievement_check_progress(sd, achievement_id, script_getnum(st, 3)));
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Award an achievement; Ignores requirements
 * achievementcomplete(<achievement ID>{,<char ID>});
 */
BUILDIN_FUNC(achievementcomplete) {
	struct map_session_data *sd;
	int i, achievement_id = script_getnum(st, 2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementcomplete: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}
	
	if( !sd->state.pc_loaded ){
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementcomplete: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	ARR_FIND(0, sd->achievement_data.count, i, sd->achievement_data.achievements[i].achievement_id == achievement_id);
	if (i == sd->achievement_data.count)
		achievement_add(sd, achievement_id);
	achievement_update_achievement(sd, achievement_id, true);
	script_pushint(st, true);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Checks if the achievement exists on player.
 * achievementexists(<achievement ID>{,<char ID>});
 */
BUILDIN_FUNC(achievementexists) {
	struct map_session_data *sd;
	int i, achievement_id = script_getnum(st, 2);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementexists: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}

	if( !sd->state.pc_loaded ){
		script_pushint(st, false);
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementexists: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	ARR_FIND(0, sd->achievement_data.count, i, sd->achievement_data.achievements[i].achievement_id == achievement_id && sd->achievement_data.achievements[i].completed > 0 );
	script_pushint(st, i < sd->achievement_data.count ? true : false);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Updates an achievement's value.
 * achievementupdate(<achievement ID>,<type>,<value>{,<char ID>});
 */
BUILDIN_FUNC(achievementupdate) {
	struct map_session_data *sd;
	int i, achievement_id, type, value;

	achievement_id = script_getnum(st, 2);
	type = script_getnum(st, 3);
	value = script_getnum(st, 4);

	if (!script_charid2sd(5, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (achievement_db.exists(achievement_id) == false) {
		ShowWarning("buildin_achievementupdate: Achievement '%d' doesn't exist.\n", achievement_id);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if( !sd->state.pc_loaded ){
#ifndef Pandas_NpcExpress_STATCALC
		// Simply ignore it on the first call, because the status will be recalculated after loading anyway
		return SCRIPT_CMD_SUCCESS;
#else
		if( !running_npc_stat_calc_event ){
			ShowError( "buildin_achievementupdate: call was too early. Character %s(CID: '%u') was not loaded yet.\n", sd->status.name, sd->status.char_id );
			return SCRIPT_CMD_FAILURE;
		}else{
			// Simply ignore it on the first call, because the status will be recalculated after loading anyway
			return SCRIPT_CMD_SUCCESS;
		}
#endif // Pandas_NpcExpress_STATCALC
	}

	ARR_FIND(0, sd->achievement_data.count, i, sd->achievement_data.achievements[i].achievement_id == achievement_id);
	if (i == sd->achievement_data.count)
		achievement_add(sd, achievement_id);

	ARR_FIND(0, sd->achievement_data.count, i, sd->achievement_data.achievements[i].achievement_id == achievement_id);
	if (i == sd->achievement_data.count) {
		script_pushint(st, false);
		return SCRIPT_CMD_SUCCESS;
	}

	if (type >= ACHIEVEINFO_COUNT1 && type <= ACHIEVEINFO_COUNT10)
		sd->achievement_data.achievements[i].count[type - 1] = value;
	else if (type == ACHIEVEINFO_COMPLETE || type == ACHIEVEINFO_COMPLETEDATE)
		sd->achievement_data.achievements[i].completed = value;
	else if (type == ACHIEVEINFO_GOTREWARD)
		sd->achievement_data.achievements[i].rewarded = value;
	else {
		ShowWarning("buildin_achievementupdate: Unknown type '%d'.\n", type);
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	achievement_update_achievement(sd, achievement_id, false);
	script_pushint(st, true);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Get an equipment's refine cost
 * getequiprefinecost(<equipment slot>,<type>,<information>{,<char id>})
 * returns -1 on fail
 */
BUILDIN_FUNC(getequiprefinecost) {
	int i = -1, slot, type, info;
	map_session_data *sd;

	slot = script_getnum(st, 2);
	type = script_getnum(st, 3);
	info = script_getnum(st, 4);

	if (!script_charid2sd(5, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (type < 0 || type >= REFINE_COST_MAX) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (equip_index_check(slot))
		i = pc_checkequip(sd, equip_bitmask[slot]);

	if (i < 0) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if( sd->inventory_data[i] == nullptr ){
		script_pushint( st, -1 );
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<s_refine_level_info> level_info = refine_db.findLevelInfo( *sd->inventory_data[i], sd->inventory.u.items_inventory[i] );

	if( level_info == nullptr ){
		script_pushint( st, -1 );
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<s_refine_cost> cost = util::umap_find( level_info->costs, (uint16)type );

	if( cost == nullptr ){
		script_pushint( st, -1 );
		return SCRIPT_CMD_SUCCESS;
	}

	switch( info ){
		case REFINE_MATERIAL_ID:
			script_pushint( st, cost->nameid );
			break;
		case REFINE_ZENY_COST:
			script_pushint( st, cost->zeny );
			break;
		default:
			script_pushint( st, -1 );
			return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}

/**
 * Round, floor, ceiling a number to arbitrary integer precision.
 * round(<number>,<precision>);
 * ceil(<number>,<precision>);
 * floor(<number>,<precision>);
 */
BUILDIN_FUNC(round) {
	int num = script_getnum(st, 2);
	int precision = script_getnum(st, 3);
	char* func = script_getfuncname(st);

	if (precision <= 0) {
		ShowError("buildin_round: Attempted to use zero or negative number as arbitrary precision.\n");
		return SCRIPT_CMD_FAILURE;
	}

	if (strcasecmp(func, "floor") == 0) {
		script_pushint(st, num - (num % precision));
	}
	else if (strcasecmp(func, "ceil") == 0) {
		script_pushint(st, num + precision - (num % precision));
	}
	else {
		script_pushint(st, (int)(round(num / (precision * 1.))) * precision);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getequiptradability) {
	int i, num;
	TBL_PC *sd;

	num = script_getnum(st, 2);

	if (!script_charid2sd(3, sd)) {
		return SCRIPT_CMD_FAILURE;
	}

	if (equip_index_check(num))
		i = pc_checkequip(sd, equip_bitmask[num]);
	else{
		ShowError("buildin_getequiptradability: Unknown equip index '%d'\n",num);
		return SCRIPT_CMD_FAILURE;
	}

	if (i >= 0)
		script_pushint(st, pc_can_trade_item(sd, i));
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}

static inline bool mail_sub( struct script_state *st, struct script_data *data, struct map_session_data *sd, int i, const char **out_name, unsigned int *start, unsigned int *end, int32 *id ){
	const char *name;

	// Check if it is a variable
	if( !data_isreference(data) ){
		ShowError("buildin_mail: argument %d is not a variable.\n", i );
		return false;
	}

	name = reference_getname(data);

	if( is_string_variable(name) ){
		ShowError( "buildin_mail: variable \"%s\" is a string variable.\n", name );
		return false;
	}

	// Check if the variable requires a player
	if( not_server_variable(*name) && sd == NULL ){
		// If no player is attached
		if( !script_rid2sd(sd) ){
			ShowError( "buildin_mail: variable \"%s\" was not a server variable, but no player was attached.\n", name );
			return false;
		}
	}

	// Try to find the array's source pointer
	if( !script_array_src( st, sd, name, reference_getref(data) ) ){
		ShowError( "buildin_mail: variable \"%s\" is not an array.\n", name );
		return false;
	}

	// Store the name for later usage
	*out_name = name;

	// Get the start and end indices of the array
	*start = reference_getindex(data);
	*end = script_array_highest_key( st, sd, name, reference_getref(data) );

	// For getting the values we need the id of the array
	*id = reference_getid(data);

	return true;
}

// mail <destination id>,"<sender name>","<title>","<body>"{,<zeny>{,<item id array>,<item amount array>{,refine{,bound{,<item card0 array>{,<item card1 array>{,<item card2 array>{,<item card3 array>{,<random option id0 array>, <random option value0 array>, <random option paramter0 array>{,<random option id1 array>, <random option value1 array>, <random option paramter1 array>{,<random option id2 array>, <random option value2 array>, <random option paramter2 array>{,<random option id3 array>, <random option value3 array>, <random option paramter3 array>{,<random option id4 array>, <random option value4 array>, <random option paramter4 array>}}}}}}}}};
BUILDIN_FUNC(mail){
	const char *sender, *title, *body, *name;
	struct mail_message msg;
	struct script_data *data;
	struct map_session_data *sd = NULL;
	unsigned int i, j, k, num_items, start, end;
	int32 id;

	memset(&msg, 0, sizeof(struct mail_message));

	msg.dest_id = script_getnum(st,2);

	sender = script_getstr(st, 3);

	if( strlen(sender) > NAME_LENGTH ){
		ShowError( "buildin_mail: sender name can not be longer than %d characters.\n", NAME_LENGTH );
		return SCRIPT_CMD_FAILURE;
	}

	safestrncpy(msg.send_name, sender, NAME_LENGTH);

	title = script_getstr(st, 4);

	if( strlen(title) > MAIL_TITLE_LENGTH ){
		ShowError( "buildin_mail: title can not be longer than %d characters.\n", MAIL_TITLE_LENGTH );
		return SCRIPT_CMD_FAILURE;
	}

	safestrncpy(msg.title, title, MAIL_TITLE_LENGTH);

	body = script_getstr(st, 5);

	if( strlen(body) > MAIL_BODY_LENGTH ){
		ShowError( "buildin_mail: body can not be longer than %d characters.\n", MAIL_BODY_LENGTH );
		return SCRIPT_CMD_FAILURE;
	}

	safestrncpy(msg.body, body, MAIL_BODY_LENGTH);

	if( script_hasdata(st,6) ){
		int zeny = script_getnum(st, 6);

		if( zeny < 0 ){
			ShowError( "buildin_mail: a negative amount of zeny can not be sent.\n" );
			return SCRIPT_CMD_FAILURE;
		}else if( zeny > MAX_ZENY ){
			ShowError( "buildin_mail: amount of zeny %u is exceeding maximum of %u. Capping...\n", zeny, MAX_ZENY );
			zeny = MAX_ZENY;
		}

		msg.zeny = zeny;
	}

	// Items
	num_items = 0;
	while( script_hasdata(st,7) ){
		data = script_getdata(st,7);

		if( !mail_sub( st, data, sd, 7, &name, &start, &end, &id ) ){
			return SCRIPT_CMD_FAILURE;
		}

		num_items = end - start;

		if( num_items == 0 ){
			ShowWarning( "buildin_mail: array \"%s\" contained no items.\n", name );
			break;
		}

		if( num_items > MAIL_MAX_ITEM ){
			ShowWarning( "buildin_mail: array \"%s\" contained %d items, capping to maximum of %d...\n", name, num_items, MAIL_MAX_ITEM );
			num_items = MAIL_MAX_ITEM;
		}

		for( i = 0; i < num_items && start < end; i++, start++ ){
			msg.item[i].nameid = (t_itemid)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );
			msg.item[i].identify = 1;

			if( !itemdb_exists(msg.item[i].nameid) ){
				ShowError( "buildin_mail: invalid item id %u.\n", msg.item[i].nameid );
				return SCRIPT_CMD_FAILURE;
			}
		}

		// Amounts
		if( !script_hasdata(st,8) ){
			ShowError("buildin_mail: missing item count variable at position %d.\n", 8);
			return SCRIPT_CMD_FAILURE;
		}

		data = script_getdata(st,8);

		if( !mail_sub( st, data, sd, 8, &name, &start, &end, &id ) ){
			return SCRIPT_CMD_FAILURE;
		}

		for( i = 0; i < num_items && start < end; i++, start++ ){
			std::shared_ptr<item_data> itm = item_db.find(msg.item[i].nameid);

			msg.item[i].amount = (short)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );

			if( msg.item[i].amount <= 0 ){
				ShowError( "buildin_mail: amount %d for item %u is invalid.\n", msg.item[i].amount, msg.item[i].nameid );
				return SCRIPT_CMD_FAILURE;
			}else if( itemdb_isstackable2(itm.get()) ){
				uint16 max = itm->stack.amount > 0 ? itm->stack.amount : MAX_AMOUNT;

				if( msg.item[i].amount > max ){
					ShowWarning( "buildin_mail: amount %d for item %u is exceeding the maximum of %d. Capping...\n", msg.item[i].amount, msg.item[i].nameid, max );
					msg.item[i].amount = max;
				}
			}else{
				if( msg.item[i].amount > 1 ){
					ShowWarning( "buildin_mail: amount %d is invalid for non-stackable item %u.\n", msg.item[i].amount, msg.item[i].nameid );
					msg.item[i].amount = 1;
				}
			}
		}

		// Refine
		if (!script_hasdata(st, 9)) {
			break;
		}

		data = script_getdata(st, 9);

		if (!mail_sub(st, data, sd, 9, &name, &start, &end, &id)) {
			return SCRIPT_CMD_FAILURE;
		}

		for (i = 0; i < num_items && start < end; i++, start++) {
			std::shared_ptr<item_data> itm = item_db.find(msg.item[i].nameid);

			msg.item[i].refine = (char)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );

			if (!itm->flag.no_refine && (itm->type == IT_WEAPON || itm->type == IT_ARMOR || itm->type == IT_SHADOWGEAR)) {
				if (msg.item[i].refine > MAX_REFINE)
					msg.item[i].refine = MAX_REFINE;
			}
			else
				msg.item[i].refine = 0;
			if (msg.item[i].refine < 0)
				msg.item[i].refine = 0;
		}

		// Bound
		if (!script_hasdata(st, 10)) {
			break;
		}

		data = script_getdata(st,10);

		if( !mail_sub( st, data, sd, 10, &name, &start, &end, &id ) ){
			return SCRIPT_CMD_FAILURE;
		}

		for( i = 0; i < num_items && start < end; i++, start++ ){
			msg.item[i].bound = (char)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );

			if( msg.item[i].bound < BOUND_NONE || msg.item[i].bound >= BOUND_MAX ){
				ShowError( "buildin_mail: bound %d for item %u is invalid.\n", msg.item[i].bound, msg.item[i].nameid );
				return SCRIPT_CMD_FAILURE;
			}
		}

		// Cards
		if( !script_hasdata(st,11) ){
			break;
		}

		for( i = 0, j = 11; i < MAX_SLOTS && script_hasdata(st,j); i++, j++ ){
			data = script_getdata(st,j);

			if( !mail_sub( st, data, sd, j + 1, &name, &start, &end, &id ) ){
				return SCRIPT_CMD_FAILURE;
			}

			for( k = 0; k < num_items && start < end; k++, start++ ){
				msg.item[k].card[i] = (t_itemid)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );

				if( msg.item[k].card[i] != 0 && !itemdb_exists(msg.item[k].card[i]) ){
					ShowError( "buildin_mail: invalid card id %u.\n", msg.item[k].card[i] );
					return SCRIPT_CMD_FAILURE;
				}
			}
		}
	
		// Random Options
		if( !script_hasdata(st,11 + MAX_SLOTS) ){
			break;
		}

		for( i = 0, j = 11 + MAX_SLOTS; i < MAX_ITEM_RDM_OPT && script_hasdata(st,j) && script_hasdata(st,j + 1) && script_hasdata(st,j + 2); i++, j++ ){
			// Option IDs
			data = script_getdata(st, j);

			if( !mail_sub( st, data, sd, j + 1, &name, &start, &end, &id ) ){
				return SCRIPT_CMD_FAILURE;
			}

			for( k = 0; k < num_items && start < end; k++, start++ ){
				msg.item[k].option[i].id = (short)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );
			}

			j++;

			// Option values
			data = script_getdata(st, j);

			if( !mail_sub( st, data, sd, j + 1, &name, &start, &end, &id ) ){
				return SCRIPT_CMD_FAILURE;
			}

			for( k = 0; k < num_items && start < end; k++, start++ ){
				msg.item[k].option[i].value = (short)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );
			}

			j++;

			// Option parameters
			data = script_getdata(st, j);

			if( !mail_sub( st, data, sd, j + 1, &name, &start, &end, &id ) ){
				return SCRIPT_CMD_FAILURE;
			}

			for( k = 0; k < num_items && start < end; k++, start++ ){
				msg.item[k].option[i].param = (char)get_val2_num( st, reference_uid( id, start ), reference_getref( data ) );
			}
		}

		// Break the pseudo scope
		break;
	}

	msg.status = MAIL_NEW;
	msg.type = MAIL_INBOX_NORMAL;
	msg.timestamp = time(NULL);

	intif_Mail_send(0, &msg);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(open_roulette){
#if PACKETVER < 20141022
	ShowError( "buildin_open_roulette: This command requires PACKETVER 2014-10-22 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd;

	if( !battle_config.feature_roulette ){
		ShowError( "buildin_open_roulette: Roulette system is disabled.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( !script_charid2sd( 2, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	clif_roulette_open(sd);

	return SCRIPT_CMD_SUCCESS;
#endif
}

/*==========================================
 * identifyall({<type>{,<account_id>}})
 * <type>:
 *	true: identify the items and returns the count of unidentified items (default)
 *	false: returns the count of unidentified items only
 *------------------------------------------*/
BUILDIN_FUNC(identifyall) {
	TBL_PC *sd;
	bool identify_item = true;

	if (script_hasdata(st, 2))
		identify_item = script_getnum(st, 2) != 0;

	if( !script_hasdata(st, 3) )
		script_rid2sd(sd);
	else
		sd = map_id2sd( script_getnum(st, 3) );

	if (sd == NULL) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}
	script_pushint(st, pc_identifyall(sd, identify_item));

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(is_guild_leader)
{
	struct map_session_data* sd;
	struct guild* guild_data;
	int guild_id;

	if (!script_rid2sd(sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 2))
		guild_id = script_getnum(st, 2);
	else
		guild_id = sd->status.guild_id;

	guild_data = guild_search(guild_id);
	if (guild_data)
		script_pushint(st, (guild_data->member[0].char_id == sd->status.char_id));
	else
		script_pushint(st, false);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(is_party_leader)
{
	struct map_session_data* sd;
	struct party_data* p_data;
	int p_id, i = 0;

	if (!script_rid2sd(sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	if (script_hasdata(st, 2))
		p_id = script_getnum(st, 2);
	else
		p_id = sd->status.party_id;

	p_data = party_search(p_id);
	if (p_data) {
		ARR_FIND( 0, MAX_PARTY, i, p_data->data[i].sd == sd );
		if (i < MAX_PARTY){
			script_pushint(st, p_data->party.member[i].leader);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	script_pushint(st, false);
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( camerainfo ){
#if PACKETVER < 20160525
	ShowError("buildin_camerainfo: This command requires PACKETVER 2016-05-25 or newer.\n");
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd;

	if( !script_charid2sd( 5, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	clif_camerainfo( sd, false, script_getnum( st, 2 ) / 100.0f, script_getnum( st, 3 ) / 100.0f, script_getnum( st, 4 ) / 100.0f );

	return SCRIPT_CMD_SUCCESS;
#endif
}

// This function is only meant to be used inside of achievement conditions
BUILDIN_FUNC(achievement_condition){
	// Push what we get from the script
	script_pushint( st, 2 );

	// Otherwise the script is freed afterwards
	st->state = RERUNLINE;

	return SCRIPT_CMD_SUCCESS;
}

/// Returns a reference to a variable of the specific instance ID.
/// Returns 0 if an error occurs.
///
/// getinstancevar(<variable>, <instance ID>) -> <reference>
BUILDIN_FUNC(getinstancevar)
{
	struct script_data* data = script_getdata(st, 2);

	if (!data_isreference(data)) {
		ShowError("buildin_getinstancevar: %s is not a variable.\n", script_getstr(st, 2));
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	const char* name = reference_getname(data);

	if (*name != '\'') {
		ShowError("buildin_getinstancevar: Invalid scope. %s is not an instance variable.\n", name);
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	int instance_id = script_getnum(st, 3);

	if (instance_id <= 0) {
		ShowError("buildin_getinstancevar: Invalid instance ID %d.\n", instance_id);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<s_instance_data> im = util::umap_find(instances, instance_id);

#ifndef Pandas_Crashfix_GetInstanceVar_Invaild_InstanceID
	if (im->state != INSTANCE_BUSY) {
#else
	if (!im || im->state != INSTANCE_BUSY) {
#endif // Pandas_Crashfix_GetInstanceVar_Invaild_InstanceID
		ShowError("buildin_getinstancevar: Unknown instance ID %d.\n", instance_id);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if (!im->regs.vars)
		im->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);

	push_val2(st->stack, C_NAME, reference_getuid(data), &im->regs);
	return SCRIPT_CMD_SUCCESS;
}

/// Sets the value of an instance variable.
///
/// setinstancevar(<variable>,<value>,<instance ID>)
BUILDIN_FUNC(setinstancevar)
{
	const char *command = script_getfuncname(st);
	struct script_data* data = script_getdata(st, 2);

	if (!data_isreference(data)) {
		ShowError("buildin_%s: %s is not a variable.\n", command, script_getstr(st, 2));
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	const char* name = reference_getname(data);

	if (*name != '\'') {
		ShowError("buildin_%s: Invalid scope. %s is not an instance variable.\n", command, name);
		script_reportdata(data);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	int instance_id = script_getnum(st, 4);

	if (instance_id <= 0) {
		ShowError("buildin_%s: Invalid instance ID %d.\n", command, instance_id);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<s_instance_data> im = util::umap_find(instances, instance_id);

#ifndef Pandas_Crashfix_SetInstanceVar_Invaild_InstanceID
	if (im->state != INSTANCE_BUSY) {
#else
	if (!im || im->state != INSTANCE_BUSY) {
#endif // Pandas_Crashfix_SetInstanceVar_Invaild_InstanceID
		ShowError("buildin_%s: Unknown instance ID %d.\n", command, instance_id);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	script_pushcopy(st, 2);

	struct map_session_data* sd = nullptr;

	if( is_string_variable(name) )
		set_reg_str( st, sd, reference_getuid(data), name, script_getstr( st, 3 ), &im->regs );
	else
		set_reg_num( st, sd, reference_getuid(data), name, script_getnum64( st, 3 ), &im->regs );

	return SCRIPT_CMD_SUCCESS;
}

/*
  convertpcinfo(<char_id>,<type>)
  convertpcinfo(<account_id>,<type>)
  convertpcinfo(<player_name>,<type>)
*/
BUILDIN_FUNC(convertpcinfo) {
	TBL_PC *sd;

	if (script_isstring(st, 2))
		sd = map_nick2sd(script_getstr(st, 2),false);
	else {
		int id = script_getnum(st, 2);
		sd = map_id2sd(id);
		if (!sd)
			sd = map_charid2sd(id);
	}

	int type = script_getnum(st, 3);

	switch (type) {
	case CPC_NAME:
	case CPC_CHAR:
	case CPC_ACCOUNT:
		break;
	default:
		ShowError("buildin_convertpcinfo: Unknown type %d.\n", type);
		script_pushnil(st);
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	if (!sd) {
		if (type == CPC_NAME)
			script_pushstrcopy(st, "");
		else
			script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch (type) {
	case CPC_NAME:
		script_pushstrcopy(st, sd->status.name);
		break;
	case CPC_CHAR:
		script_pushint(st, sd->status.char_id);
		break;
	case CPC_ACCOUNT:
		script_pushint(st, sd->status.account_id);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(isnpccloaked)
{
	npc_data *nd;

	if (script_hasdata(st, 2))
		nd = npc_name2id(script_getstr(st, 2));
	else
		nd = map_id2nd(st->oid);

	if (!nd) {
		if (script_hasdata(st, 2))
			ShowError("buildin_isnpccloaked: %s is a non-existing NPC.\n", script_getstr(st, 2));
		else
			ShowError("buildin_isnpccloaked: non-existing NPC.\n");
		return SCRIPT_CMD_FAILURE;
	}

	map_session_data* sd;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	script_pushint(st, npc_is_cloaked(nd, sd));
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(refineui){
#if PACKETVER < 20161012
	ShowError( "buildin_refineui: This command requires packet version 2016-10-12 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd;

	if( !script_charid2sd(2,sd) ){
		return SCRIPT_CMD_FAILURE;
	}

	if( !battle_config.feature_refineui ){
		ShowError( "buildin_refineui: This command is disabled via configuration.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( !sd->state.refineui_open ){
		clif_refineui_open(sd);
	}

	return SCRIPT_CMD_SUCCESS;
#endif
}

BUILDIN_FUNC(getenchantgrade){
	struct map_session_data *sd;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	int index, position;

	if (script_hasdata(st, 2))
		position = script_getnum(st, 2);
	else
		position = EQI_COMPOUND_ON;

	if (position == EQI_COMPOUND_ON)
		index = current_equip_item_index;
	else if (equip_index_check(position))
		index = pc_checkequip(sd, equip_bitmask[position]);
	else {
		ShowError( "buildin_getenchantgrade: Unknown equip index '%d'\n", position );
		script_pushint(st,-1);
		return SCRIPT_CMD_FAILURE;
	}

	if (index < 0 || index >= MAX_INVENTORY || sd->inventory.u.items_inventory[index].nameid == 0)
		script_pushint(st, -1);
	else
		script_pushint(st, sd->inventory.u.items_inventory[index].enchantgrade);

	return SCRIPT_CMD_SUCCESS;
}

/*==========================================
 * mob_setidleevent( <monster game ID>, "<event label>" )
 *------------------------------------------*/
BUILDIN_FUNC(mob_setidleevent){
	struct block_list* bl;

	if( !script_rid2bl( 2, bl ) ){
		return SCRIPT_CMD_FAILURE;
	}

	if( bl->type != BL_MOB ){
		ShowError( "buildin_mob_setidleevent: the target GID was not a monster.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	struct mob_data* md = (struct mob_data*)bl;
	if (md == nullptr)
		return SCRIPT_CMD_FAILURE;

	const char* idle_event = script_getstr( st, 3 );

	check_event( st, idle_event );
	safestrncpy( md->idle_event, idle_event, EVENT_NAME_LENGTH );

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( openstylist ){
#if PACKETVER >= 20151104
	struct map_session_data* sd;

	if( !script_charid2sd( 2, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	clif_ui_open( *sd, OUT_UI_STYLIST, 0 );

	return SCRIPT_CMD_SUCCESS;
#else
	ShowError( "buildin_openstylist: This command requires packet version 2015-11-04 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#endif
}

BUILDIN_FUNC(getitempos) {
	struct map_session_data* sd;

	if ( !script_rid2sd(sd) ){
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if( current_equip_item_index == -1 ){
		ShowError( "buildin_getitempos: Invalid usage detected. This command should only be used inside item scripts.\n" );
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, sd->inventory.u.items_inventory[current_equip_item_index].equip);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( laphine_synthesis ){
	struct map_session_data* sd;

	if( !script_rid2sd( sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->itemid == 0 ){
		ShowError( "buildin_laphine_synthesis: Called outside of an item script without item id.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->inventory_data[sd->itemindex]->flag.delay_consume == 0 ){
		ShowError( "buildin_laphine_synthesis: Called from item %u, which is not a consumed delayed.\n", sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->state.laphine_synthesis != 0 ){
		ShowError( "buildin_laphine_synthesis: Laphine Synthesis window was already open. Player %s (AID: %u, CID: %u) with item id %u.\n", sd->status.name, sd->status.account_id, sd->status.char_id, sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<s_laphine_synthesis> synthesis = laphine_synthesis_db.find( sd->itemid );

	if( synthesis == nullptr ){
		ShowError( "buildin_laphine_synthesis: %u is not a valid Laphine Synthesis item.\n", sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	clif_laphine_synthesis_open( sd, synthesis );

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( laphine_upgrade ){
	struct map_session_data* sd;

	if( !script_rid2sd( sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->itemid == 0 ){
		ShowError( "buildin_laphine_upgrade: Called outside of an item script without item id.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->inventory_data[sd->itemindex]->flag.delay_consume == 0 ){
		ShowError( "buildin_laphine_upgrade: Called from item %u, which is not a consumed delayed.\n", sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	if( sd->state.laphine_upgrade != 0 ){
		ShowError( "buildin_laphine_upgrade: Laphine Upgrade window was already open. Player %s (AID: %u, CID: %u) with item id %u.\n", sd->status.name, sd->status.account_id, sd->status.char_id, sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<s_laphine_upgrade> upgrade = laphine_upgrade_db.find( sd->itemid );

	if( upgrade == nullptr ){
		ShowError( "buildin_laphine_upgrade: %u is not a valid Laphine Upgrade item.\n", sd->itemid );
		return SCRIPT_CMD_FAILURE;
	}

	clif_laphine_upgrade_open( sd, upgrade );

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(randomoptgroup)
{
	int id = script_getnum(st,2);

	auto group = random_option_group.find(id);

	if (group == nullptr) {
		ShowError("buildin_randomoptgroup: Invalid random option group id (%d)!\n", id);
		return SCRIPT_CMD_FAILURE;
	}

	struct item item_tmp = {};

	group->apply( item_tmp );

	for ( int i = 0; i < MAX_ITEM_RDM_OPT; ++i ) {
		setd_sub_num(st, nullptr, ".@opt_id", i, item_tmp.option[i].id, nullptr);
		setd_sub_num(st, nullptr, ".@opt_value", i, item_tmp.option[i].value, nullptr);
		setd_sub_num(st, nullptr, ".@opt_param", i, item_tmp.option[i].param, nullptr);
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( open_quest_ui ){
#if PACKETVER < 20151202
	ShowError( "buildin_open_quest_ui: This command requires PACKETVER 20151202 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	int quest_id = script_hasdata(st, 2) ? script_getnum(st, 2) : 0;

	if( quest_id != 0 ){
		int i;
		ARR_FIND(0, sd->avail_quests, i, sd->quest_log[i].quest_id == quest_id);
		if (i == sd->avail_quests)
			ShowWarning("buildin_open_quest_ui: Character %d doesn't have quest %d.\n", sd->status.char_id, quest_id);
	}

	clif_ui_open( *sd, OUT_UI_QUEST, quest_id );

	return SCRIPT_CMD_SUCCESS;
#endif
}

BUILDIN_FUNC(openbank){
#if PACKETVER < 20151202
	ShowError( "buildin_openbank: This command requires PACKETVER 20151202 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#else
	struct map_session_data* sd = nullptr;

	if (!script_charid2sd(2, sd)) {
#ifdef Pandas_ScriptCommand_OpenBank
		script_pushint(st, 0);
#endif // Pandas_ScriptCommand_OpenBank
		return SCRIPT_CMD_FAILURE;
	}

	if( !battle_config.feature_banking ){
		ShowError( "buildin_openbank: banking is disabled.\n" );
#ifdef Pandas_ScriptCommand_OpenBank
		script_pushint(st, 0);
#endif // Pandas_ScriptCommand_OpenBank
		return SCRIPT_CMD_FAILURE;
	}

#ifdef Pandas_MapFlag_NoBank
	if (map_getmapflag(sd->bl.m, MF_NOBANK)) {
		// This map prohibit using the bank system.
		clif_messagecolor(&sd->bl, color_table[COLOR_RED], msg_txt_cn(sd, 10), false, SELF);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}
#endif // Pandas_MapFlag_NoBank

	clif_ui_open( *sd, OUT_UI_BANK, 0 );
#ifdef Pandas_ScriptCommand_OpenBank
	script_pushint(st, 1);
#endif // Pandas_ScriptCommand_OpenBank
	return SCRIPT_CMD_SUCCESS;
#endif
}

BUILDIN_FUNC(getbaseexp_ratio){
	struct map_session_data *sd;

	if( !script_charid2sd( 4, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	int32 percent = script_getnum( st, 2 );

	if( percent < 0 || percent > 100 ){
		ShowError( "getbaseexp_ratio: percentage is out of range.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	int32 base_level;

	if( script_hasdata( st, 3 ) ){
		base_level = script_getnum( st, 3 );

		if( base_level < 1 ){
			ShowError( "getbaseexp_ratio: level cannot be below 1.\n" );
			return SCRIPT_CMD_FAILURE;
		}

		uint32 max_level = job_db.get_maxBaseLv( sd->status.class_ );

		if( base_level > max_level ){
			ShowError( "getbaseexp_ratio: level is above maximum base level %u for job %s.\n", max_level, job_name( sd->status.class_ ) );
			return SCRIPT_CMD_FAILURE;
		}
	}else{
		base_level = sd->status.base_level;
	}

	t_exp class_exp = job_db.get_baseExp( sd->status.class_, base_level );

	if( class_exp <= 0 ){
		ShowError( "getbaseexp_ratio: No base experience defined for class %s at base level %d.\n", job_name( sd->status.class_ ), base_level );
		return SCRIPT_CMD_FAILURE;
	}

	double result = ( class_exp * percent ) / 100.0;

	script_pushint64( st, static_cast<t_exp>( result ) );
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getjobexp_ratio){
	struct map_session_data *sd;

	if( !script_charid2sd( 4, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	int32 percent = script_getnum( st, 2 );

	if( percent < 0 || percent > 100 ){
		ShowError( "getjobexp_ratio: percentage is out of range.\n" );
		return SCRIPT_CMD_FAILURE;
	}

	int32 job_level;

	if( script_hasdata( st, 3 ) ){
		job_level = script_getnum( st, 3 );

		if( job_level < 1 ){
			ShowError( "getjobexp_ratio: level cannot be below 1.\n" );
			return SCRIPT_CMD_FAILURE;
		}

		uint32 max_level = job_db.get_maxJobLv( sd->status.class_ );

		if( job_level > max_level ){
			ShowError( "getjobexp_ratio: level is above maximum job level %u for job %s.\n", max_level, job_name( sd->status.class_ ) );
			return SCRIPT_CMD_FAILURE;
		}
	}else{
		job_level = sd->status.job_level;
	}

	t_exp class_exp = job_db.get_baseExp( sd->status.class_, job_level );

	if( class_exp <= 0 ){
		ShowError( "getjobexp_ratio: No job experience defined for class %s at job level %d.\n", job_name( sd->status.class_ ), job_level );
		return SCRIPT_CMD_FAILURE;
	}

	double result = ( class_exp * percent ) / 100.0;

	script_pushint64( st, static_cast<t_exp>( result ) );
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC( enchantgradeui ){
#if PACKETVER_MAIN_NUM >= 20200916 || PACKETVER_RE_NUM >= 20200724
	struct map_session_data* sd;

	if( !script_charid2sd( 2, sd ) ){
		return SCRIPT_CMD_FAILURE;
	}

	clif_ui_open( *sd, OUT_UI_ENCHANTGRADE, 0 );

	return SCRIPT_CMD_SUCCESS;
#else
	ShowError( "buildin_enchantgradeui: This command requires PACKETVER 2020-07-24 or newer.\n" );
	return SCRIPT_CMD_FAILURE;
#endif
}

#include "../custom/script.inc"

// declarations that were supposed to be exported from npc_chat.cpp
#ifdef PCRE_SUPPORT
BUILDIN_FUNC(defpattern);
BUILDIN_FUNC(activatepset);
BUILDIN_FUNC(deactivatepset);
BUILDIN_FUNC(deletepset);
#endif

/** Regular expression matching
 * preg_match(<pattern>,<string>{,<offset>})
 */
BUILDIN_FUNC(preg_match) {
#ifdef PCRE_SUPPORT
	pcre *re;
	pcre_extra *pcreExtra;
	const char *error;
	int erroffset, r, offset = 0;
	int subStrVec[30];

	const char* pattern = script_getstr(st,2);
	const char* subject = script_getstr(st,3);
	if (script_hasdata(st,4))
		offset = script_getnum(st,4);

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	pcreExtra = pcre_study(re, 0, &error);

	r = pcre_exec(re, pcreExtra, subject, (int)strlen(subject), offset, 0, subStrVec, 30);

	pcre_free(re);
	if (pcreExtra != NULL)
		pcre_free(pcreExtra);

	if (r < 0)
		script_pushint(st,0);
	else
		script_pushint(st,(r > 0) ? r : 30 / 3);

	return SCRIPT_CMD_SUCCESS;
#else
	ShowDebug("script:preg_match: cannot run without PCRE library enabled.\n");
	script_pushint(st,0);
	return SCRIPT_CMD_SUCCESS;
#endif
}

#ifdef Pandas_ScriptCommand_SetHeadDir
/* ===========================================================
 * 指令: setheaddir
 * 描述: 调整角色纸娃娃脑袋的朝向
 * 用法: setheaddir <朝向编号>{,<角色编号>};
 * 朝向: 0为看正前方, 1为向右看, 2为向左看
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(setheaddir) {
	struct map_session_data *sd = nullptr;
	int head_dir = script_getnum(st, 2);
	head_dir = cap_value(head_dir, 0, 2);

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_SUCCESS;

	pc_setdir(sd, sd->ud.dir, head_dir);
	clif_changed_dir(&sd->bl, AREA_WOS);
	clif_changed_dir(&sd->bl, SELF);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetHeadDir

#ifdef Pandas_ScriptCommand_SetBodyDir
/* ===========================================================
 * 指令: setbodydir
 * 描述: 用于调整角色纸娃娃身体的朝向
 * 用法: setbodydir <朝向编号>{,<角色编号>};
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(setbodydir) {
	struct map_session_data *sd = nullptr;
	int body_dir = script_getnum(st, 2);
	body_dir = cap_value(body_dir, 0, 7);

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_SUCCESS;

	pc_setdir(sd, body_dir, sd->head_dir);
	clif_changed_dir(&sd->bl, AREA_WOS);
	clif_changed_dir(&sd->bl, SELF);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetBodyDir

#ifdef Pandas_ScriptCommand_InstanceUsers
/* ===========================================================
 * 指令: instance_users
 * 描述: 获取指定的副本实例中已经进入副本地图的人数
 * 用法: instance_users <副本实例编号>;
 * 返回: 成功直接返回副本中的人数, 副本不存在或副本中无人存在则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(instance_users) {
	struct instance_data *im = nullptr;
	int i = 0, users = 0, instance_id = 0;

	instance_id = script_getnum(st, 2);

	std::shared_ptr<s_instance_data> idata = util::umap_find(instances, instance_id);

	if (!idata || idata->state != INSTANCE_BUSY) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	for (const auto& it : idata->map)
		users += max(map_getmapdata(it.m)->users, 0);

	script_pushint(st, users);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_InstanceUsers

#ifdef Pandas_ScriptCommand_CapValue
/* ===========================================================
 * 指令: cap
 * 描述: 确保数值不低于给定的最小值, 不超过给定的最大值
 * 用法: cap <要判断的数值>,<最小值>,<最大值>;
 * 返回: 低于最小值则直接返回最小值, 超过最大值则直接返回最大值, 如果在两者之间则原样返回数值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(cap) {
	int val = script_getnum(st, 2);
	int min = script_getnum(st, 3);
	int max = script_getnum(st, 4);

	script_pushint(st, cap_value(val, min, max));
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_CapValue

#ifdef Pandas_ScriptCommand_MobRemove
/* ===========================================================
 * 指令: mobremove
 * 描述: 根据 GID 移除一个魔物单位 (只是移除, 不会让魔物死亡)
 * 用法: mobremove <魔物的GID>;
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(mobremove) {
	struct block_list *bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));

	if (!bl || bl->type != BL_MOB)
		return SCRIPT_CMD_SUCCESS;

	TBL_MOB* md = (TBL_MOB*)bl;

	if (!md->spawn)
		unit_free(bl, CLR_OUTSIGHT);
	else {
		unit_remove_map(bl, CLR_OUTSIGHT);
		if (!(md->sc.data[SC_KAIZEL] || (md->sc.data[SC_REBIRTH] && !md->state.rebirth)))
			mob_setdelayspawn(md);
#ifdef Pandas_BattleRecord
		map_mobiddb(bl, npc_get_new_npc_id());
#endif // Pandas_BattleRecord
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_MobRemove

#ifdef Pandas_ScriptCommand_BattleIgnore
/* ===========================================================
 * 指令: battleignore
 * 描述: 将角色设置为魔物免战状态, 避免被魔物攻击
 * 用法: battleignore <开关状态>{,<角色编号>};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(battleignore) {
	struct map_session_data *sd = nullptr;
	int immune = script_getnum(st, 2);

	if (!script_charid2sd(2, sd))
		return SCRIPT_CMD_SUCCESS;

	if (cap_value(immune, 0, 1))
		sd->state.block_action |= PCBLOCK_IMMUNE;
	else
		sd->state.block_action &= ~PCBLOCK_IMMUNE;

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BattleIgnore

#ifdef Pandas_ScriptCommand_GetHotkey
/* ===========================================================
 * 指令: gethotkey
 * 描述: 获取指定快捷键位置当前的信息
 * 用法: gethotkey <快捷键位置编号>{,<要获取的数据类型>};
 * 返回: 若携带 <要获取的数据类型> 参数时, 发生错误将返回 -1, 成功则返回查询的值;
		不携带 <要获取的数据类型> 参数时, 发生错误将返回 -1, 成功则将信息保存到变量并返回 1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(gethotkey) {
	struct map_session_data *sd = nullptr;

	if (!script_rid2sd(sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	int hotkey_idx = script_getnum(st, 2);
	if (hotkey_idx < 0 || hotkey_idx > MAX_HOTKEYS) {
		ShowError("buildin_gethotkey: hotkey index %d is out of range (0..%d).\n", hotkey_idx, MAX_HOTKEYS);
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 3)) {
		if (!script_isint(st, 3)) {
			ShowError("buildin_gethotkey: request date type must be a integer value.\n");
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS;
		}

		switch (script_getnum(st, 3))
		{
		case 0:
			script_pushint(st, sd->status.hotkeys[hotkey_idx].type); break;
		case 1:
			script_pushint(st, sd->status.hotkeys[hotkey_idx].id); break;
		case 2:
			script_pushint(st, sd->status.hotkeys[hotkey_idx].lv); break;
		default:
			ShowError("buildin_gethotkey: request date type %d is out of range (0..2).\n", script_getnum(st, 3));
			script_pushint(st, -1);
		}
		return SCRIPT_CMD_SUCCESS;
	}

	pc_setreg(sd, add_str("@hotkey_type"), (int)sd->status.hotkeys[hotkey_idx].type);
	pc_setreg(sd, add_str("@hotkey_id"), (int)sd->status.hotkeys[hotkey_idx].id);
	pc_setreg(sd, add_str("@hotkey_lv"), (int)sd->status.hotkeys[hotkey_idx].lv);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetHotkey

#ifdef Pandas_ScriptCommand_SetHotkey
/* ===========================================================
 * 指令: sethotkey
 * 描述: 设置指定快捷键位置的信息
 * 用法: sethotkey <快捷键位置编号>,<快捷键的类型>,<物品/技能的ID>,<技能等级>;
 * 返回: 设置成功则返回 1, 设置失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(sethotkey) {
	struct map_session_data *sd = nullptr;

	if (!script_rid2sd(sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	int hotkey_idx = script_getnum(st, 2);
	if (hotkey_idx < 0 || hotkey_idx > MAX_HOTKEYS) {
		ShowError("buildin_sethotkey: hotkey index %d is out of range (0..%d).\n", hotkey_idx, MAX_HOTKEYS);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	int hotkey_type = script_getnum(st, 3);
	if (hotkey_type < 0 || hotkey_type > 1) {
		ShowError("buildin_sethotkey: hotkey type %d is out of range (0..1).\n", hotkey_type);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	int hotkey_id = script_getnum(st, 4);
	if (hotkey_type == 0) {	// 物品
		if (!itemdb_exists(hotkey_id)) {
			ShowError("buildin_sethotkey: Nonexistant item %d requested.\n", hotkey_id);
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
	}
	else {	// 技能
		if (!skill_get_index(hotkey_id)) {
			ShowError("buildin_sethotkey: Invalid skill ID %d , please review.\n", hotkey_id);
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	int hotkey_lv = script_getnum(st, 5);
	if (hotkey_type == 0) {	// 物品
		hotkey_lv = 0;
	}

	sd->status.hotkeys[hotkey_idx].type = hotkey_type;
	sd->status.hotkeys[hotkey_idx].id = hotkey_id;
	sd->status.hotkeys[hotkey_idx].lv = hotkey_lv;

	clif_hotkeys_send(sd, 0);

#if PACKETVER_MAIN_NUM >= 20190522 || PACKETVER_RE_NUM >= 20190508 || PACKETVER_ZERO_NUM >= 20190605
	clif_hotkeys_send(sd, 1);
#endif

	clif_inventorylist(sd);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetHotkey

#ifdef Pandas_ScriptCommand_ShowVend
/* ===========================================================
 * 指令: showvend
 * 描述: 使指定的 NPC 头上可以显示露天商店的招牌
 * 用法: showvend "<NPC名称>",<是否显示>{,"<招牌名称>"};
 * 返回: 操作成功则返回 1, 操作失败则返回 0
 * 作者: Jian916, Rewrite By Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(showvend) {
	struct npc_data *nd = nullptr;
	const char *npcname = nullptr, *message = nullptr;
	char buf[NAME_LENGTH + 1] = { 0 };
	int showit = 0;

	npcname = script_getstr(st, 2);
	showit = script_getnum(st, 3);

	if (showit && !script_hasdata(st, 4)) {
		ShowError("buildin_showvend: Can't create vendingboard without any message.\n");
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}
	else if (showit && !script_isstring(st, 4)) {
		ShowError("buildin_showvend: The 'message' param must be a string.\n");
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}
	else if (showit) {
		message = script_getstr(st, 4);
	}

	nd = npc_name2id(npcname);
	if (nd == nullptr) {
		ShowError("buildin_showvend: No such NPC '%s'.\n", npcname);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	switch (showit)
	{
	case 0:
		clif_closevendingboard(&nd->bl, 0);
		memset(nd->vendingboard.message, 0, NAME_LENGTH + 1);
		nd->vendingboard.show = false;
		break;
	default:
		memcpy(buf, message, NAME_LENGTH);
		clif_showvendingboard(&nd->bl, (const char*)buf, 0);
		nd->vendingboard.show = true;
		memcpy(nd->vendingboard.message, buf, NAME_LENGTH + 1);
		break;
	}

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_ShowVend

#ifdef Pandas_ScriptCommand_ViewEquip
/* ===========================================================
 * 指令: viewequip
 * 描述: 查看指定在线角色的装备面板信息
 * 用法: viewequip <目标的角色编号|目标的账号编号>{,<是否强制查看>};
 * 返回: 操作成功则返回 1, 操作失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(viewequip) {
	struct map_session_data *sd = nullptr;
	int cid = script_getnum(st, 2), force = 0;
	struct map_session_data *tsd = map_charid2sd(cid);

	if (!tsd || !script_rid2sd(sd)){
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 3)) {
		if (!script_isint(st, 3)) {
			ShowError("buildin_showvend: The 'force' param must be a integer.\n");
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
		force = cap_value(script_getnum(st, 3), 0, 1);
	}

	if(tsd->status.show_equip || pc_has_permission(sd, PC_PERM_VIEW_EQUIPMENT) || force == 1){
		clif_viewequip_ack(sd, tsd);
		script_pushint(st, 1);
	}
	else{
		clif_msg(sd, VIEW_EQUIP_FAIL);
		script_pushint(st, 0);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_ViewEquip

#ifdef Pandas_ScriptCommand_CountItemIdx
/* ===========================================================
 * 指令: countitemidx
 * 描述: 获取指定背包序号的道具在背包中的数量
 * 用法: countitemidx <背包序号>{,<角色编号>};
 * 返回: 操作成功则返回道具的数量, 操作失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(countitemidx) {
	struct map_session_data *sd = nullptr;
	int idx = -1;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = script_getnum(st, 2);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		ShowWarning("buildin_countitemidx: Index (%d) should be from 0-%d.\n", idx, sd->inventory.max_amount - 1);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!item_db.find(sd->inventory.u.items_inventory[idx].nameid)) {
		ShowWarning("buildin_countitemidx: Invalid Item ID (%u).\n", sd->inventory.u.items_inventory[idx].nameid);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, sd->inventory.u.items_inventory[idx].amount);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_CountItemIdx

#ifdef Pandas_ScriptCommand_IdentifyIdx
/* ===========================================================
 * 指令: identifyidx
 * 描述: 鉴定指定背包序号的道具
 * 用法: identifyidx <背包序号>{,<角色编号>};
 * 返回: 操作成功则返回 1, 操作失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(identifyidx) {
	struct map_session_data *sd = nullptr;
	int idx = -1;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = script_getnum(st, 2);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		ShowWarning("buildin_identifyidx: Index (%d) should be from 0-%d.\n", idx, sd->inventory.max_amount - 1);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!item_db.find(sd->inventory.u.items_inventory[idx].nameid)) {
		ShowWarning("buildin_identifyidx: Invalid Item ID (%u).\n", sd->inventory.u.items_inventory[idx].nameid);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->inventory.u.items_inventory[idx].identify != 1) {
		sd->inventory.u.items_inventory[idx].identify = 1;
		clif_item_identified(sd, idx, 0);
	}

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_IdentifyIdx

#ifdef Pandas_ScriptCommand_UnEquipIdx
/* ===========================================================
 * 指令: unequipidx
 * 描述: 脱下指定背包序号的道具
 * 用法: unequipidx <背包序号>{,<角色编号>};
 * 返回: 操作成功则返回 1, 操作失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(unequipidx) {
	struct map_session_data *sd = nullptr;
	int idx = -1;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = script_getnum(st, 2);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		ShowWarning("buildin_unequipidx: Index (%d) should be from 0-%d.\n", idx, sd->inventory.max_amount - 1);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	std::shared_ptr<item_data> id = item_db.find(sd->inventory.u.items_inventory[idx].nameid);
	if (id == nullptr) {
		ShowWarning("buildin_unequipidx: Invalid Item ID (%u).\n", sd->inventory.u.items_inventory[idx].nameid);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!itemdb_isequip2(id.get())) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->inventory.u.items_inventory[idx].equip == 0) {
		script_pushint(st, 1);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, pc_unequipitem(sd, idx, id->equip) ? 1 : 0);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_UnEquipIdx

#ifdef Pandas_ScriptCommand_EquipIdx
/* ===========================================================
 * 指令: equipidx
 * 描述: 穿戴指定背包序号的道具
 * 用法: equipidx <背包序号>{,<角色编号>};
 * 返回: 操作成功则返回 1, 操作失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(equipidx) {
	struct map_session_data *sd = nullptr;
	int idx = -1;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = script_getnum(st, 2);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		ShowWarning("buildin_equipidx: Index (%d) should be from 0-%d.\n", idx, sd->inventory.max_amount - 1);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}
	
	std::shared_ptr<item_data> id = item_db.find(sd->inventory.u.items_inventory[idx].nameid);
	if (id == nullptr) {
		ShowWarning("buildin_equipidx: Invalid Item ID (%u).\n", sd->inventory.u.items_inventory[idx].nameid);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!itemdb_isequip2(id.get())) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->inventory.u.items_inventory[idx].equip != 0) {
		script_pushint(st, 1);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, pc_equipitem(sd, idx, id->equip) ? 1 : 0);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_EquipIdx

#ifdef Pandas_ScriptCommand_ItemExists
/* ===========================================================
 * 指令: itemexists
 * 描述: 确认物品数据库中是否存在指定物品
 * 用法: itemexists <物品编号/"物品名称">;
 * 返回: 若物品指定的道具编号不存在于物品数据库中则返回 0,
 *      若物品存在且可堆叠则返回正数物品编号, 不可堆叠则返回负数物品编号
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(itemexists) {
	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2))
		id = item_db.searchname(script_getstr(st, 2));
	else
		id = item_db.find(script_getnum(st, 2));

	if (id == nullptr) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, (itemdb_isstackable2(id.get()) ? id->nameid : -(int64)id->nameid));
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_ItemExists

#ifdef Pandas_ScriptCommand_RentTime
/* ===========================================================
 * 指令: renttime
 * 描述: 增加/减少指定位置装备的租赁时间
 * 用法: renttime <EQI装备位置>,<增减的时间秒数>{,<角色编号>};
 * 返回: 操作失败返回 0, 非 0 的正数表示成功增减后新的剩余时间秒数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(renttime) {
	struct map_session_data *sd = nullptr;
	int equip_num = script_getnum(st, 2);
	int second = script_getnum(st, 3);
	int idx = -1, expire_tick = 0;

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!equip_index_check(equip_num)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = pc_checkequip(sd, equip_bitmask[equip_num]);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->inventory.u.items_inventory[idx].expire_time == 0) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

 	sd->inventory.u.items_inventory[idx].expire_time += second;
	expire_tick = (unsigned int)(sd->inventory.u.items_inventory[idx].expire_time - time(NULL));
 	script_pushint(st, expire_tick);

	if (expire_tick > 0) {
		clif_rental_time(sd, sd->inventory.u.items_inventory[idx].nameid, expire_tick);
		pc_inventory_rental_add(sd, expire_tick);
		clif_inventorylist(sd);
	}
	else {
		int i = 0, c = 0;
		for (i = 0; i < sd->status.inventory_slots; i++) {
			if (sd->inventory.u.items_inventory[i].nameid == 0)
				continue;
			if (sd->inventory.u.items_inventory[i].expire_time == 0)
				continue;
			if (sd->inventory.u.items_inventory[i].expire_time <= time(NULL)) {
				if (sd->inventory_data[i]->unequip_script)
					run_script(sd->inventory_data[i]->unequip_script, 0, sd->bl.id, fake_nd->bl.id);
				clif_rental_expired(sd, i, sd->inventory.u.items_inventory[i].nameid);
				pc_delitem(sd, i, sd->inventory.u.items_inventory[i].amount, 0, 0, LOG_TYPE_OTHER);
			}
			else {
				c++;
			}
		}

		if (c <= 0) {
			pc_inventory_rental_clear(sd);
		}
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_RentTime

#ifdef Pandas_ScriptCommand_GetEquipIdx
/* ===========================================================
 * 指令: getequipidx
 * 描述: 获取指定位置装备的背包序号
 * 用法: getequipidx <EQI装备位置>{,<角色编号>};
 * 返回: 获取失败返回各种负数, 非 0 的正数表示获取到的背包序号
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getequipidx) {
	struct map_session_data *sd = nullptr;
	int equip_num = script_getnum(st, 2), idx = -1;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, -3);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!equip_index_check(equip_num)) {
		script_pushint(st, -2);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = pc_checkequip(sd, equip_bitmask[equip_num]);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, idx);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetEquipIdx

#ifdef Pandas_ScriptCommand_GetEquipExpireTick
/* ===========================================================
 * 指令: getequipexpiretick
 * 描述: 获取指定位置装备的租赁到期剩余秒数
 * 用法: getequipexpiretick <EQI装备位置>{,<角色编号>};
 * 返回: 获取失败返回各种负数, 返回 0 表示目标装备非租赁, 其他非 0 正整数则代表剩余秒数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getequipexpiretick) {
	struct map_session_data *sd = nullptr;
	int equip_num = script_getnum(st, 2), idx = -1;
	int64 left_seconds = 0;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, -3);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!equip_index_check(equip_num)) {
		script_pushint(st, -2);
		return SCRIPT_CMD_SUCCESS;
	}

	idx = pc_checkequip(sd, equip_bitmask[equip_num]);
	if (idx < 0 || idx >= sd->inventory.max_amount) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}
	if (!sd->inventory.u.items_inventory[idx].expire_time) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	left_seconds = (int64)(sd->inventory.u.items_inventory[idx].expire_time - time(NULL));
	if (left_seconds < 0) {
		script_pushint(st, -4);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, left_seconds);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetEquipExpireTick

#ifdef Pandas_ScriptCommand_GetInventoryInfo
/* ===========================================================
 * 指令: getinventoryinfo
 * 描述: 查询指定背包序号的道具的详细信息
 * 用法: getinventoryinfo <道具的背包序号>,<要查看的信息类型>{,<角色编号>};
 * 用法: getcartinfo <道具的手推车序号>,<要查看的信息类型>{,<角色编号>};
 * 用法: getguildstorageinfo <道具的公会仓库序号>,<要查看的信息类型>{,<角色编号>};
 * 用法: getstorageinfo <道具的个人仓库/扩充仓库序号>,<要查看的信息类型>{{,<仓库编号>},<角色编号>};
 * 返回: 查询失败返回 -1, 若查询成功则返回你所查询的信息
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getinventoryinfo) {
    struct map_session_data *sd = nullptr;
	struct item_data *id = nullptr;
	int idx = script_getnum(st, 2);
	const char* command = script_getfuncname(st);
	int charid_slot = (!stricmp(command, "getstorageinfo") ? 5 : 4);

	if (!script_charid2sd(charid_slot, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	struct s_storage* stor = nullptr;
	struct item* inventory = nullptr;
	int stor_id = (script_hasdata(st, 4) ? script_getnum(st, 4) : 0);

	if (!script_getstorage(st, sd, &stor, &inventory, stor_id)) {
		return SCRIPT_CMD_FAILURE;
	}
	
	if (st->state == RERUNLINE) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (!stor || !inventory) {
		ShowError("buildin_%s: cannot read inventory or storage data.\n", command);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (idx < 0 || idx >= stor->max_amount) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!itemdb_exists(inventory[idx].nameid)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (inventory[idx].amount <= 0) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	int type = script_getnum(st, 3);
	switch (type)
	{
	case 0:  script_pushint(st, inventory[idx].nameid); break;
	case 1:  script_pushint(st, inventory[idx].amount); break;
	case 2:  script_pushint(st, inventory[idx].equip); break;
	case 3:  script_pushint(st, inventory[idx].refine); break;
	case 4:  script_pushint(st, inventory[idx].identify); break;
	case 5:  script_pushint(st, inventory[idx].attribute); break;
	case 6:  script_pushint(st, inventory[idx].card[0]); break;
	case 7:  script_pushint(st, inventory[idx].card[1]); break;
	case 8:  script_pushint(st, inventory[idx].card[2]); break;
	case 9:  script_pushint(st, inventory[idx].card[3]); break;
	case 10: script_pushint(st, inventory[idx].expire_time); break;
	case 11: script_pushint(st, inventory[idx].unique_id); break;
	case 12: case 13: case 14: case 15: case 16:
		script_pushint(st, inventory[idx].option[type - 12].id); break;
	case 17: case 18: case 19: case 20: case 21:
		script_pushint(st, inventory[idx].option[type - 17].value); break;
	case 22: case 23: case 24: case 25: case 26:
		script_pushint(st, inventory[idx].option[type - 22].param); break;
	case 27: script_pushint(st, inventory[idx].bound); break;
	case 28: script_pushint(st, inventory[idx].enchantgrade); break;
	case 29: script_pushint(st, inventory[idx].equipSwitch); break;
	case 30: script_pushint(st, inventory[idx].favorite); break;
	default:
		ShowWarning("buildin_%s: The type should be in range 0-%d, currently type is: %d.\n", command, 30, type);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetInventoryInfo

#ifdef Pandas_ScriptCommand_StatusCheck
/* ===========================================================
 * 指令: statuscheck
 * 描述: 判断状态是否存在, 并取得相关的状态参数
 * 用法: statuscheck <状态编号>{,<游戏单位编号>};
 * 返回: 获取成功则返回 1, 角色没有该状态则返回 0, 其他错误返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(statuscheck) {
	struct map_session_data *sd = nullptr;
	if (!script_mapid2sd(3, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int id = script_getnum(st, 2);
	if (id <= SC_NONE || id >= SC_MAX) {
		ShowWarning("buildin_statuscheck: Invalid status type given (%d).\n", id);
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->sc.count == 0 || !sd->sc.data[id]) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	pc_setreg(sd, add_str("@sc_val1"), sd->sc.data[id]->val1);
	pc_setreg(sd, add_str("@sc_val2"), sd->sc.data[id]->val2);
	pc_setreg(sd, add_str("@sc_val3"), sd->sc.data[id]->val3);
	pc_setreg(sd, add_str("@sc_val4"), sd->sc.data[id]->val4);

	// 这种返回方式若剩余时间过长的话 @sc_tickleft 保存的数值会被截断, 不可靠
	// 建议还是多使用 rAthena 自带的 getstatus 指令来替代 statuscheck / sc_check
	// 之所以实现 statuscheck / sc_check 完全出于兼容目的考虑
	struct TimerData* timer = (struct TimerData*)get_timer(sd->sc.data[id]->timer);
	t_tick tickleft = (timer ? DIFF_TICK(timer->tick, gettick()) : -1);
	pc_setreg(sd, add_str("@sc_tickleft"), tickleft);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_StatusCheck

#if defined(Pandas_ScriptCommand_RentTimeIdx) || defined(Pandas_ScriptCommand_SetInventoryInfo)
//************************************
// Method:      inventory_rental_update
// Description: 移除过期租赁道具; 最后如果连一件租赁道具都没有, 那么直接删掉计时器
// Parameter:   struct map_session_data * sd
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/5/26 23:23
//************************************
void inventory_rental_update(struct map_session_data* sd) {
	int i = 0, c = 0;

	if (!sd) {
		return;
	}

	for (i = 0; i < sd->status.inventory_slots; i++) {
		if (sd->inventory.u.items_inventory[i].nameid == 0)
			continue;
		if (sd->inventory.u.items_inventory[i].expire_time == 0)
			continue;
		if (sd->inventory.u.items_inventory[i].expire_time <= time(NULL)) {
			if (sd->inventory_data[i]->unequip_script)
				run_script(sd->inventory_data[i]->unequip_script, 0, sd->bl.id, fake_nd->bl.id);
			clif_rental_expired(sd, i, sd->inventory.u.items_inventory[i].nameid);
			pc_delitem(sd, i, sd->inventory.u.items_inventory[i].amount, 0, 0, LOG_TYPE_OTHER);
		}
		else {
			c++;
		}
	}

	if (c <= 0) {
		pc_inventory_rental_clear(sd);
	}
}
#endif // defined(Pandas_ScriptCommand_RentTimeIdx) || defined(Pandas_ScriptCommand_SetInventoryInfo)

#ifdef Pandas_ScriptCommand_RentTimeIdx
/* ===========================================================
 * 指令: renttimeidx
 * 描述: 增加/减少指定背包序号道具的租赁时间
 * 用法: renttimeidx <背包序号>,<增减的时间秒数>{,<角色编号>};
 * 返回: 操作失败返回 0, 非 0 的正数表示成功增减后新的剩余时间秒数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(renttimeidx) {
	struct map_session_data *sd = nullptr;
	int idx = script_getnum(st, 2);
	int second = script_getnum(st, 3);
	int expire_tick = 0;

	if (!script_charid2sd(4, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (idx < 0 || idx >= sd->inventory.max_amount) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->inventory.u.items_inventory[idx].expire_time == 0) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	sd->inventory.u.items_inventory[idx].expire_time += second;
	expire_tick = (unsigned int)(sd->inventory.u.items_inventory[idx].expire_time - time(NULL));
	script_pushint(st, expire_tick);

	if (expire_tick > 0) {
		clif_rental_time(sd, sd->inventory.u.items_inventory[idx].nameid, expire_tick);
		pc_inventory_rental_add(sd, expire_tick);
		clif_inventorylist(sd);
	}
	else {
		inventory_rental_update(sd);
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_RentTimeIdx

#ifdef Pandas_ScriptCommand_PartyLeave
/* ===========================================================
 * 指令: party_leave
 * 描述: 使当前角色或指定角色退出队伍
 * 用法: party_leave {<角色编号>};
 * 返回: 若指定的角色不在线或不在队伍中则返回 0, 若角色成功退出队伍则返回 1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(party_leave) {
	struct map_session_data *sd = nullptr;

	if (!script_charid2sd(2, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, party_leave(sd));
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_PartyLeave

#ifdef Pandas_ScriptCommand_Script4Each
/* ===========================================================
 * 指令: buildin_script4each_sub
 * 描述: 配合 script4each 指令使用的一个内部处理函数
 * -----------------------------------------------------------*/
static int buildin_script4each_sub(struct block_list *bl, va_list ap) {
	struct script_code* script = va_arg(ap, struct script_code*);
	int pos = va_arg(ap, int);

	if (!bl || !script) return 0;

	mapreg_setreg(add_str("$@gid"), bl->id);
	run_script(script, pos, bl->id, 0);
	return 1;
}

/* ===========================================================
 * 指令: script4each / script4eachmob / script4eachnpc
 * 描述: 对指定范围的玩家执行相同的一段脚本
 * 用法: script4each <"{脚本}">,<脚本的执行范围>{,<动态参数>...};
 * 用法: script4eachmob <"{脚本}">,<脚本的执行范围>{,<动态参数>...};
 * 用法: script4eachnpc <"{脚本}">,<脚本的执行范围>{,<动态参数>...};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克 (最早借鉴自 Sense 的代码进行改进)
 * -----------------------------------------------------------*/
BUILDIN_FUNC(script4each) {
	const char *execute_script = script_getstr(st, 2);
	int execute_range = script_getnum(st, 3);

	struct script_code* script = nullptr;
	int pos = 0;
	bool script_needfree = false;

#if !defined(Pandas_Helper_Common_Function) || !defined(Pandas_Redeclaration_Struct_Event_Data)
	ShowWarning("This version is not support 'NPCNAME::EVENT' script in '%s' command.\n", cmdname);
#else
	struct event_data* ev = npc_event_data(execute_script);
	if (ev != nullptr) {
		script = ev->nd->u.scr.script;
		pos = ev->pos;
	}
	else {
		script = parse_script(execute_script, script_getfuncname(st), 0, SCRIPT_IGNORE_EXTERNAL_BRACKETS);
		script_needfree = (script != nullptr);
	}
#endif // !defined(Pandas_Helper_Common_Function) || !defined(Pandas_Redeclaration_Struct_Event_Data)

	struct s_mapiterator *iter = nullptr;
	struct block_list* bl = nullptr;
	enum bl_type bltype = BL_PC;

	// 根据使用的指令名称, 确定后续对什么类型的单位进行遍历
	const char* command = script_getfuncname(st);
	if (stricmp(command, "script4each") == 0) {
		bltype = BL_PC;
		iter = mapit_geteachpc();
	}
	else if (stricmp(command, "script4eachmob") == 0) {
		bltype = BL_MOB;
		iter = mapit_geteachmob();
	}
	else if (stricmp(command, "script4eachnpc") == 0) {
		bltype = BL_NPC;
		iter = mapit_geteachnpc();
	}

	switch (execute_range)
	{
	case 0: {
		// 全服单位 - script4each "{<脚本>}",0;
		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	case 1: {
		// 指定地图上的全部单位 - script4each "{<脚本>}",1,<"地图名称">;
		int map_id = -1;

		if (!script_hasdata(st, 4) || !script_isstring(st, 4)) break;
		if ((map_id = map_mapname2mapid(script_getstr(st, 4))) < 0) break;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype || bl->m != map_id) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	case 2: {
		// 以地图某个点为中心半径距离内的单位 - script4each "{<脚本>}",2,<"地图名称">,<中心坐标x>,<中心坐标y>,<范围>;
		int map_id = -1, map_x = 0, map_y = 0, range = 0;

		if (!script_hasdata(st, 4) || !script_isstring(st, 4)) break;
		if (!script_hasdata(st, 5) || !script_isint(st, 5)) break;
		if (!script_hasdata(st, 6) || !script_isint(st, 6)) break;
		if (!script_hasdata(st, 7) || !script_isint(st, 7)) break;

		if ((map_id = map_mapname2mapid(script_getstr(st, 4))) < 0) break;
		map_x = script_getnum(st, 5);
		map_y = script_getnum(st, 6);
		range = script_getnum(st, 7);

		struct block_list center_bl = { 0 };
		center_bl.m = map_id;
		center_bl.x = map_x;
		center_bl.y = map_y;

		map_foreachinrange(buildin_script4each_sub, &center_bl, range, bltype, script, pos);
		break;
	}
	case 3: {
		// 指定玩家所在的队伍中的全部队伍成员 - script4each "{<脚本>}",3,<角色编号>;
		int party_id = 0;
		struct map_session_data *target_sd = nullptr;

		// 该类型不支持 script4eachmob 和 script4eachnpc 指令
		if (bltype != BL_PC) break;

		if (!script_hasdata(st, 4) || !script_isint(st, 4)) break;
		target_sd = map_charid2sd(script_getnum(st, 4));
		if (!target_sd || (party_id = target_sd->status.party_id) <= 0) break;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype) continue;
			if (((TBL_PC*)bl)->status.party_id != party_id) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	case 4: {
		// 指定玩家所在的公会中的全部公会成员 - script4each "{<脚本>}",4,<角色编号>;
		int guild_id = 0;
		struct map_session_data *target_sd = nullptr;

		// 该类型不支持 script4eachmob 和 script4eachnpc 指令
		if (bltype != BL_PC) break;

		if (!script_hasdata(st, 4) || !script_isint(st, 4)) break;
		target_sd = map_charid2sd(script_getnum(st, 4));
		if (!target_sd || (guild_id = target_sd->status.guild_id) <= 0) break;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype) continue;
			if (((TBL_PC*)bl)->status.guild_id != guild_id) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	case 5: {
		// 指定区域 - script4each "{<脚本>}",5,<"地图名称">,<坐标x0>,<坐标y0>,<坐标x1>,<坐标y1>;
		int map_id = -1, map_x0 = 0, map_y0 = 0, map_x1 = 0, map_y1 = 0;

		if (!script_hasdata(st, 4) || !script_isstring(st, 4)) break;
		if (!script_hasdata(st, 5) || !script_isint(st, 5)) break;
		if (!script_hasdata(st, 6) || !script_isint(st, 6)) break;
		if (!script_hasdata(st, 7) || !script_isint(st, 7)) break;
		if (!script_hasdata(st, 8) || !script_isint(st, 8)) break;

		if ((map_id = map_mapname2mapid(script_getstr(st, 4))) < 0) break;
		map_x0 = script_getnum(st, 5);
		map_y0 = script_getnum(st, 6);
		map_x1 = script_getnum(st, 7);
		map_y1 = script_getnum(st, 8);

		map_foreachinarea(buildin_script4each_sub, map_id, map_x0, map_y0, map_x1, map_y1, bltype, script, pos);
		break;
	}
	case 6: {
		// 指定队伍中的全部队伍成员 - script4each "{<脚本>}",6,<队伍编号>;
		int party_id = 0;

		// 该类型不支持 script4eachmob 和 script4eachnpc 指令
		if (bltype != BL_PC) break;

		if (!script_hasdata(st, 4) || !script_isint(st, 4)) break;
		if ((party_id = script_getnum(st, 4)) <= 0) break;;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype) continue;
			if (((TBL_PC*)bl)->status.party_id != party_id) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	case 7: {
		// 指定公会中的全部公会成员 - script4each "{<脚本>}",7,<公会编号>;
		int guild_id = 0;

		// 该类型不支持 script4eachmob 和 script4eachnpc 指令
		if (bltype != BL_PC) break;

		if (!script_hasdata(st, 4) || !script_isint(st, 4)) break;
		if ((guild_id = script_getnum(st, 4)) <= 0) break;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->type != bltype) continue;
			if (((TBL_PC*)bl)->status.guild_id != guild_id) continue;
			mapreg_setreg(add_str("$@gid"), bl->id);
			run_script(script, pos, bl->id, 0);
		}
		break;
	}
	default:
		ShowWarning("buildin_%s: Invalid execute range '%d'.\n", script_getfuncname(st), execute_range);
		break;
	}

	if (script && script_needfree) script_free_code(script);
	if (iter) mapit_free(iter);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Script4Each

#ifdef Pandas_ScriptCommand_GetSameIpInfo
/* ===========================================================
 * 指令: buildin_getsameipinfo_sub
 * 描述: 配合 getsameipinfo 指令使用的一个内部处理函数
 * -----------------------------------------------------------*/
static int buildin_getsameipinfo_sub(struct map_session_data* pl_sd, va_list ap)
{
	struct map_session_data *sd = va_arg(ap, struct map_session_data*);
	uint32 ipaddr = va_arg(ap, uint32);
	uint32 *count = va_arg(ap, uint32*);
	int32 m = va_arg(ap, int32);	// int16 通过可变参数方式传递, 会被提升为 int32

	if (!ipaddr || !sd | !count) return 0;
	if (!pl_sd || pl_sd->state.autotrade) return 0;
	if (m >= 0 && pl_sd->bl.m != m) return 0;

	if (ipaddr == session[pl_sd->fd]->client_addr) {
		pc_setreg(sd, reference_uid(add_str("@sameip_aid"), (*count)), pl_sd->status.account_id);
		pc_setreg(sd, reference_uid(add_str("@sameip_cid"), (*count)), pl_sd->status.char_id);
		pc_setregstr(sd, reference_uid(add_str("@sameip_name$"), (*count)), pl_sd->status.name);
		(*count)++;
		return 1;
	}

	return 0;
}

/* ===========================================================
 * 指令: getsameipinfo
 * 描述: 获得某个指定 IP 在线的玩家信息
 * 用法: getsameipinfo {<"IP地址">{<,"地图名">}};
 * 返回: 出错返回 -1, 其他含 0 正整数表示查到的此 IP 的在线玩家数
 * 作者: Sola丶小克, 晓晓
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getsameipinfo) {
	struct map_session_data *sd = nullptr;
	uint32 ipaddr = 0, match_count = 0;
	int16 m = -1;

	if (!script_rid2sd(sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 2) && !script_isstring(st, 2)) {
		ShowError("buildin_getsameipinfo: ip address must be string variable\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 2)) {
		ipaddr = str2ip(script_getstr(st, 2));
	}
	else {
		ipaddr = session[sd->fd]->client_addr;
	}

	if (script_hasdata(st, 3)) {
		const char* map_name = script_getstr(st, 3);
		if ((m = map_mapname2mapid(map_name)) < 0) {
			ShowWarning("buildin_getsameipinfo: Invalid map name '%s'.\n", map_name);
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	map_foreachpc(buildin_getsameipinfo_sub, sd, ipaddr, &match_count, m);
	pc_setreg(sd, add_str("@sameip_amount"), match_count);

	script_pushint(st, match_count);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetSameIpInfo

#ifdef Pandas_ScriptCommand_Logout
/* ===========================================================
 * 指令: logout
 * 描述: 使指定的角色立刻登出游戏
 * 用法: logout <登出理由>{,"<角色名称>"|<账号编号>|<角色编号>};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(logout) {
	struct map_session_data *sd = nullptr;
	int reason = script_getnum(st, 2);

	if (script_hasdata(st, 3)) {
		if (script_isstring(st, 3)) {
			sd = map_nick2sd(script_getstr(st, 3), false);
		}
		else {
			int id = script_getnum(st, 3);
			sd = (map_id2sd(id) ? map_id2sd(id) : map_charid2sd(id));
		}
	}
	else {
		script_rid2sd(sd);
	}

	if (!sd) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (!((reason >= 0 && reason <= 18) || (reason >= 100 && reason <= 110) || reason == 111)) {
		ShowWarning("buildin_logout: unknown logout reason %d\n", reason);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->fd) {
		clif_authfail_fd(sd->fd, reason);
	}
	else {
		map_quit(sd);
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Logout

#ifdef Pandas_ScriptCommand_GetAreaGid
/* ===========================================================
 * 指令: buildin_getareagid_sub
 * 描述: 配合 getareagid 指令使用的一个内部处理函数
 * -----------------------------------------------------------*/
static int buildin_getareagid_sub(struct block_list *bl, va_list ap) {
	struct map_session_data *sd = nullptr;
	struct script_state *st = va_arg(ap, struct script_state *);
	struct script_data *retdata = va_arg(ap, struct script_data *);
	uint32* found_count = va_arg(ap, uint32*);

	if (!st || !retdata || !found_count) {
		return 0;
	}

	int64 uid = reference_uid(reference_getid(retdata), (*found_count));
	const char *retname = reference_getname(retdata);
	sd = map_id2sd(st->rid);

	set_reg_num(st, sd, uid, retname, bl->id, reference_getref(retdata));

	(*found_count)++;
	return 1;
}

/* ===========================================================
 * 指令: getareagid
 * 描述: 获取指定范围内特定类型单位的全部 GID
 * 用法: getareagid <返回数组>,<搜索范围>{,<动态参数>...};
 * 返回: 操作失败返回 -1, 其他含 0 正整数表示搜索到的 GID 数量
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getareagid) {
	struct map_session_data *sd = nullptr;
	struct script_data *retdata = script_getdata(st, 2);
	struct reg_db *src_reg_db = nullptr;
	int search_range = script_getnum(st, 3);
	uint32 found_count = 0;

	if (!data_isreference(retdata)) {
		ShowError("buildin_getareagid: error argument! please give a array variable for save gameid.\n");
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	int retid = reference_getid(retdata);
	const char *retname = reference_getname(retdata);

	if (not_server_variable(*retname)) {
		if (!script_rid2sd(sd)) {
			ShowError("buildin_getareagid: '%s' is not server variable, please attach to a player.\n", retname);
			script_reportdata(retdata);
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (!(src_reg_db = script_array_src(st, sd, retname, reference_getref(retdata)))) {
		ShowError("buildin_getareagid: variable '%s' is not a array.\n", retname);
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (is_string_variable(retname)) {
		ShowError("buildin_getareagid: the array variable '%s' must be integer type.\n", retname);
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	// 以下用于清空存放返回数据的数值型数组 (参考 script_cleararray_pc 的实现)
	script_array_ensure_zero(st, NULL, retdata->u.num, reference_getref(retdata));
	struct script_array* sa = static_cast<script_array *>(idb_get(src_reg_db->arrays, retid));
	unsigned int array_len = script_array_highest_key(st, sd, retname, reference_getref(retdata));

	if (sa) {
		// 若给定的数组是存在的, 那么需要清空一下
		unsigned int* list = script_array_cpy_list(sa);
		unsigned int size = sa->size;

		for (unsigned int i = 0; i < size; i++) {
			clear_reg(st, sd, reference_uid(retid, list[i]), retname, reference_getref(retdata));
		}
	}

	switch (search_range)
	{
	case 0: {
		// getareagid <返回数组>,0,<想搜索的单位类型>,<"地图名称">;
		int map_id = -1, unitfilter = BL_ALL;
		std::string mapname;

		if (!script_get_optnum(st, 4, "Unit Type", unitfilter)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optstr(st, 5, "Map Name", mapname)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_mapindex(st, mapname.c_str(), map_id)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }

		map_foreachinmap(buildin_getareagid_sub, map_id, unitfilter, st, retdata, &found_count);
		break;
	}
	case 1: {
		// getareagid <返回数组>,1,<想搜索的单位类型>,<"地图名称">,<中心坐标x>,<中心坐标y>,<范围>;
		int map_id = -1, unitfilter = BL_ALL;
		int map_x = 0, map_y = 0, range = 0;
		std::string mapname;

		if (!script_get_optnum(st, 4, "Unit Type", unitfilter)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optstr(st, 5, "Map Name", mapname)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 6, "Center X", map_x)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 7, "Center Y", map_y)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 8, "Range", range)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_mapindex(st, mapname.c_str(), map_id)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }

		struct block_list center_bl = { 0 };
		center_bl.m = map_id;
		center_bl.x = map_x;
		center_bl.y = map_y;

		map_foreachinrange(buildin_getareagid_sub, &center_bl, range, unitfilter, st, retdata, &found_count);
		break;
	}
	case 2: {
		// getareagid <返回数组>,2,<想搜索的单位类型>,<"地图名称">,<坐标x0>,<坐标y0>,<坐标x1>,<坐标y1>;
		int map_id = -1, unitfilter = BL_ALL;
		int map_x0 = 0, map_y0 = 0, map_x1 = 0, map_y1 = 0;
		std::string mapname;

		if (!script_get_optnum(st, 4, "Unit Type", unitfilter)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optstr(st, 5, "Map Name", mapname)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 6, "x0 coordinate", map_x0)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 7, "y0 coordinate", map_y0)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 8, "x1 coordinate", map_x1)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_optnum(st, 9, "y1 coordinate", map_y1)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }
		if (!script_get_mapindex(st, mapname.c_str(), map_id)) { script_pushint(st, -1); return SCRIPT_CMD_SUCCESS; }

		map_foreachinarea(buildin_getareagid_sub, map_id, map_x0, map_y0, map_x1, map_y1, unitfilter, st, retdata, &found_count);
		break;
	}
	default:
		ShowWarning("buildin_getareagid: Invalid search range '%d'.\n", search_range);
		break;
	}

	script_pushint(st, found_count);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetAreaGid

#ifdef Pandas_ScriptCommand_ProcessHalt
/* ===========================================================
 * 指令: processhalt
 * 描述: 在事件处理代码中使用该指令, 可以中断源代码的后续处理逻辑
 * 用法: processhalt {<是否设置中断>};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(processhalt) {
	struct map_session_data *sd = nullptr;
	char* postfix = NULL;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;

	if (sd->pandas.workinevent == NPCE_MAX) {
		ShowError("buildin_processhalt: Require work in event script.\n");
		return SCRIPT_CMD_FAILURE;
	}

	const char* name = npc_get_script_event_name(sd->pandas.workinevent);
	if (name == nullptr) {
		ShowError("buildin_processhalt: Can not get the event name for event type : %d\n", sd->pandas.workinevent);
		return SCRIPT_CMD_FAILURE;
	}

	std::string evname = std::string(name);
	if (evname.find("Filter") == std::string::npos) {
		ShowError("buildin_processhalt: The '%s' event is not support processhalt.\n", evname.c_str());
		return SCRIPT_CMD_FAILURE;
	}

	bool makehalt = true;
	if (script_hasdata(st, 2) && script_isint(st, 2)) {
		makehalt = (cap_value(script_getnum(st, 2), 0, 1) == 1);
	}

	if (!setProcessHalt(sd, sd->pandas.workinevent, makehalt)) {
		ShowError("buildin_processhalt: An error occurred while setting the '%s' event halt status to '%d'.\n", evname.c_str(), (int32)makehalt);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_ProcessHalt

#ifdef Pandas_ScriptCommand_SetEventTrigger
/* ===========================================================
 * 指令: settrigger
 * 描述: 使用该指令可以设置某个事件或过滤器的触发行为 (禁止触发、下次触发、永久触发)
 * 用法: settrigger <事件的常量名称>,<触发行为>;
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(settrigger) {
	struct map_session_data *sd = nullptr;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;

	uint16 envtype = script_getnum(st, 2);
	if (envtype >= NPCE_MAX) {
		ShowError("buildin_settrigger: Invalid npc event type: %d\n", envtype);
		return SCRIPT_CMD_FAILURE;
	}

	uint16 triggerflag = script_getnum(st, 3);
	if (triggerflag >= EVENT_TRIGGER_MAX) {
		ShowError("buildin_settrigger: Invalid npc event trigger type: %d\n", triggerflag);
		return SCRIPT_CMD_FAILURE;
	}

	const char* name = npc_get_script_event_name(envtype);
	if (name == nullptr) {
		ShowError("buildin_settrigger: Can not get the event name for event type: %d\n", envtype);
		return SCRIPT_CMD_FAILURE;
	}

	if (!setEventTrigger(sd, (npce_event)envtype, (npce_trigger)triggerflag)) {
		ShowError("buildin_processhalt: An error occurred while setting the '%s' event trigger type to '%d'.\n", name, triggerflag);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetEventTrigger

#ifdef Pandas_ScriptCommand_MessageColor
/* ===========================================================
 * 指令: messagecolor
 * 描述: 发送指定颜色的消息文本到聊天窗口中
 * 用法: messagecolor "<消息文本>"{,"<文本颜色代码>",<发送目标>,<游戏单位编号>};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(messagecolor) {
	const char* text = script_getstr(st, 2);

	const char* color = "ffffff";
	if (script_hasdata(st, 3)) {
		color = script_getstr(st, 3);
	}

	struct block_list *bl = nullptr;
	if (!script_rid2bl(5, bl)) {
		return SCRIPT_CMD_FAILURE;
	}

	send_target target = AREA;
	if (script_hasdata(st, 4)) {
		switch (script_getnum(st, 4)) {
			case BC_ALL:	target = ALL_CLIENT;	break;
			case BC_MAP:	target = ALL_SAMEMAP;	break;
			case BC_SELF:	target = SELF;			break;
			case BC_AREA:
			default:		target = AREA;			break;
		}
	}

	clif_messagecolor(bl, strtol(color, (char**)NULL, 16), text, true, target);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_MessageColor

#ifdef Pandas_ScriptCommand_Copynpc
/* ===========================================================
 * 指令: copynpc
 * 描述: 复制指定的 NPC 到一个新的位置
 * 用法: copynpc "<复制出来的新NPC所在地图名称>,<X坐标>,<Y坐标>,<朝向编号>","duplicate(<来源NPC名称>)","<复制出来的新NPC名称>","<图档外观编号>";
 * 用法: copynpc "<复制出来的新NPC所在地图名称>",<X坐标>,<Y坐标>,<朝向编号>,"<来源NPC名称>","<复制出来的新NPC名称>",<图档外观编号>;
 * 返回: 复制成功则返回新 NPC 的 GID, 复制失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(copynpc) {
	// --------------------------------------------------------------------------------------
	// 本指令的绝大部分代码来源自 npc.cpp 的 npc_parse_duplicate 函数
	// 若未来 rAthena 官方修改了 npc_parse_duplicate 那么这里也需要进行对应的调整, 以便确保主流程一致
	// --------------------------------------------------------------------------------------

	const char *w1 = nullptr, *w2 = nullptr, *w3 = nullptr, *w4 = nullptr;
	char w1buf[256] = { 0 }, w2buf[256] = { 0 }, w3buf[256] = { 0 }, w4buf[256] = { 0 };
	DBMap* npcname_db = get_npcname_db_ptr();
	int* npc_script = get_npc_script_ptr();
	int* npc_shop = get_npc_shop_ptr();
	int* npc_warp = get_npc_warp_ptr();
	struct npc_data* local_nd = map_id2nd(st->oid);	// 当前执行了 copynpc 指令的 npc 对象
	const char* filepath = (local_nd ? local_nd->path : "Unable to confirm the file path where the NPC executes 'copynpc' command.");
	const char *start = nullptr, *buffer = nullptr; // npc_parsename 和 npc_parseview 需要用到, 由于没有具体数据, 所以故意不赋值

	// 如果在物品的使用脚本或类似的无 NPC 作为触发主体的地方使用了 copynpc,
	// 那么 filepath 将会是空指针, 而不是某个 npc 脚本的具体路径.
	// 这会导致 npc_parsename 函数中使用 strlen(filepath) 时导致空指针崩溃.
	// 在这种情况下我们将此 npc 归属的路径, 设置为当前函数的名字: buildin_copynpc 以规避此问题
	if (filepath == nullptr) {
		filepath = __func__;
	}

	do
	{
		if (script_lastdata(st) == 5) {
			// 若用四个参数的形式调用 copynpc, 那么四个参数必须全部都是字符串类型
			// 第四个参数可以允许是数值变量, 因为数值变量可以直接被 script_getstr 强制转换成字符串
			if (script_isstring(st, 2) && script_isstring(st, 3) &&
				script_isstring(st, 4) && (script_isstring(st, 5) || script_isint(st, 5))) {
				w1 = script_getstr(st, 2);
				w2 = script_getstr(st, 3);
				w3 = script_getstr(st, 4);
				w4 = script_getstr(st, 5);
				break; // 这里的 break 表示流程正常结束, 无需报错
			}
		}
		else if (script_lastdata(st) == 8) {
			// 若使用七个参数的形式调用 copynpc, 那么七个参数的类型分别需要是: siiissi
			if (script_isstring(st, 2) && script_isint(st, 3) &&
				script_isint(st, 4) && script_isint(st, 5) &&
				script_isstring(st, 6) && script_isstring(st, 7) && script_isint(st, 8)) {

				sprintf(w1buf, "%s,%d,%d,%d", script_getstr(st, 2), script_getnum(st, 3), script_getnum(st, 4), script_getnum(st, 5));
				sprintf(w2buf, "%s", script_getstr(st, 6));
				sprintf(w3buf, "%s", script_getstr(st, 7));
				sprintf(w4buf, "%d", script_getnum(st, 8));

				w1 = w1buf;
				w2 = w2buf;
				w3 = w3buf;
				w4 = w4buf;
				break; // 这里的 break 表示流程正常结束, 无需报错
			}
		}

		ShowError("buildin_copynpc: Invalid parameters, please read the manual for copynpc.\n");
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	} while (false);

	short x, y, m, xs = -1, ys = -1;
	int16 dir;
	char srcname[128];
	int i;
	//const char* end;
	size_t length;

	int src_id;
	int type;
	struct npc_data* nd;
	struct npc_data* dnd;

	//end = strchr(start, '\n');
	length = strlen(w2);

	if (strncmpi(w2, "duplicate(", 10) == 0) {
		// get the npc being duplicated
		if (w2[length - 1] != ')' || length <= 11 || length - 11 >= sizeof(srcname))
		{// does not match 'duplicate(%127s)', name is empty or too long
			//ShowError("npc_parse_script: bad duplicate name in file '%s', line '%d' : %s\n", filepath, strline(buffer, start - buffer), w2);
			//return end;// next line, try to continue
			ShowError("buildin_copynpc: bad duplicate name in file '%s' : %s\n", filepath, w2);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}
		safestrncpy(srcname, w2 + 10, length - 10);
	}
	else {
		safestrncpy(srcname, w2, sizeof(srcname));
	}

	dnd = npc_name2id(srcname);
	if (dnd == NULL) {
		//ShowError("npc_parse_script: original npc not found for duplicate in file '%s', line '%d' : %s\n", filepath, strline(buffer, start - buffer), srcname);
		//return end;// next line, try to continue
		ShowError("buildin_copynpc: original npc not found for duplicate in file '%s' : %s\n", filepath, srcname);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
	src_id = dnd->src_id ? dnd->src_id : dnd->bl.id;
	type = dnd->subtype;

	// get placement
	if ((type == NPCTYPE_SHOP || type == NPCTYPE_CASHSHOP || type == NPCTYPE_ITEMSHOP || type == NPCTYPE_POINTSHOP || type == NPCTYPE_SCRIPT || type == NPCTYPE_MARKETSHOP) && strcmp(w1, "-") == 0) {// floating shop/chashshop/itemshop/pointshop/script
		x = y = dir = 0;
		m = -1;
	}
	else {
		char mapname[MAP_NAME_LENGTH_EXT];

		if (sscanf(w1, "%15[^,],%6hd,%6hd,%4hd", mapname, &x, &y, &dir) != 4) { // <map name>,<x>,<y>,<facing>
			//ShowError("npc_parse_duplicate: Invalid placement format for duplicate in file '%s', line '%d'. Skipping line...\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer, start - buffer), w1, w2, w3, w4);
			//return end;// next line, try to continue
			ShowError("buildin_copynpc: Invalid placement format for duplicate in file '%s'\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, w1, w2, w3, w4);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}
		m = map_mapname2mapid(mapname);
	}

	struct map_data *mapdata = map_getmapdata(m);

	if (m != -1 && (x < 0 || x >= mapdata->xs || y < 0 || y >= mapdata->ys)) {
		//ShowError("npc_parse_duplicate: coordinates %d/%d are out of bounds in map %s(%dx%d), in file '%s', line '%d'\n", x, y, mapdata->name, mapdata->xs, mapdata->ys, filepath, strline(buffer, start - buffer));
		ShowError("buildin_copynpc: coordinates %d/%d are out of bounds in map %s(%dx%d), in file '%s'\n", x, y, mapdata->name, mapdata->xs, mapdata->ys, filepath);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (type == NPCTYPE_WARP && sscanf(w4, "%6hd,%6hd", &xs, &ys) == 2);// <spanx>,<spany>
	else if (type == NPCTYPE_SCRIPT && sscanf(w4, "%*[^,],%6hd,%6hd", &xs, &ys) == 2);// <sprite id>,<triggerX>,<triggerY>
	else if (type == NPCTYPE_WARP) {
		//ShowError("npc_parse_duplicate: Invalid span format for duplicate warp in file '%s', line '%d'. Skipping line...\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer, start - buffer), w1, w2, w3, w4);
		//return end;// next line, try to continue
		ShowError("buildin_copynpc: Invalid span format for duplicate warp in file '%s'\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, w1, w2, w3, w4);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	// --------------------------------------------------------------------------------------
	// 注意: 这里的 start 和 buffer 最终会在 npc_parsename 和 npc_parseview 中
	// 用于计算出报错代码的所在行数. 但是目前脚本引擎里面, 好像并没有办法知道当前的指令处于那一行,
	// 所以也无法模拟出一个可以提示正确行数的 start 和 buffer 的值用于后面的展现.
	// 
	// 我们的原则是尽量少改动 rAthena 的代码, 这里不想大改 npc_parsename 和 npc_parseview 函数.
	// 所以这里不予设置 start 和 buffer 的值, 此做法会让出错的时候无法告诉用户出问题的脚本行数
	// --------------------------------------------------------------------------------------

	nd = npc_create_npc(m, x, y);
	npc_parsename(nd, w3, start, buffer, filepath);
	nd->class_ = m == -1 ? JT_FAKENPC : npc_parseview(w4, start, buffer, filepath);
	nd->speed = 200;
	nd->src_id = src_id;
	nd->bl.type = BL_NPC;
	nd->subtype = (enum npc_subtype)type;
	switch (type) {
	case NPCTYPE_SCRIPT:
		++(*npc_script); // 与 npc_parse_duplicate 不同, 这里是个指针, 需要先解引用后再递增
		nd->u.scr.xs = xs;
		nd->u.scr.ys = ys;
		nd->u.scr.script = dnd->u.scr.script;
		nd->u.scr.label_list = dnd->u.scr.label_list;
		nd->u.scr.label_list_num = dnd->u.scr.label_list_num;
		break;

	case NPCTYPE_SHOP:
	case NPCTYPE_CASHSHOP:
	case NPCTYPE_ITEMSHOP:
	case NPCTYPE_POINTSHOP:
	case NPCTYPE_MARKETSHOP:
		++(*npc_shop); // 与 npc_parse_duplicate 不同, 这里是个指针, 需要先解引用后再递增
		safestrncpy(nd->u.shop.pointshop_str, dnd->u.shop.pointshop_str, strlen(dnd->u.shop.pointshop_str));
		nd->u.shop.itemshop_nameid = dnd->u.shop.itemshop_nameid;
#ifndef Pandas_Fix_Duplicate_Shop_With_FullyShopItemList
		nd->u.shop.shop_item = dnd->u.shop.shop_item;
#else
		// 为了避免被复制出来的[子商店]和[来源商店]使用相同的商品道具信息源,
		// 而导致后面对[来源商店]或任意一个[子商店]的道具进行增删操作时影响到同一个[来源商店]的[子商店]
		// 这里在复制商店 NPC 的时候, 将全部的商品列表完整的复制一份出来, 他们之间相互独立
		CREATE(nd->u.shop.shop_item, struct npc_item_list, dnd->u.shop.count);
		memcpy(nd->u.shop.shop_item, dnd->u.shop.shop_item, sizeof(struct npc_item_list) * dnd->u.shop.count);
#endif // Pandas_Fix_Duplicate_Shop_With_FullyShopItemList
		nd->u.shop.count = dnd->u.shop.count;
		nd->u.shop.discount = dnd->u.shop.discount;
#ifdef Pandas_Support_Pointshop_Variable_DisplayName
		safestrncpy(nd->u.shop.pointshop_str_nick, dnd->u.shop.pointshop_str_nick, sizeof(dnd->u.shop.pointshop_str_nick));
#endif // Pandas_Support_Pointshop_Variable_DisplayName
		break;

	case NPCTYPE_WARP:
		++(*npc_warp); // 与 npc_parse_duplicate 不同, 这里是个指针, 需要先解引用后再递增
		if (!battle_config.warp_point_debug)
			nd->class_ = JT_WARPNPC;
		else
			nd->class_ = JT_GUILD_FLAG;
		nd->u.warp.xs = xs;
		nd->u.warp.ys = ys;
		nd->u.warp.mapindex = dnd->u.warp.mapindex;
		nd->u.warp.x = dnd->u.warp.x;
		nd->u.warp.y = dnd->u.warp.y;
		nd->trigger_on_hidden = dnd->trigger_on_hidden;
		break;
	}

	//Add the npc to its location
	if (m >= 0) {
		map_addnpc(m, nd);
		status_change_init(&nd->bl);
		unit_dataset(&nd->bl);
		nd->ud.dir = (uint8)dir;
		npc_setcells(nd);
		if (map_addblock(&nd->bl)) {
			//return end;
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}
		if (nd->class_ != JT_FAKENPC) {
			status_set_viewdata(&nd->bl, nd->class_);
			if (map_getmapdata(nd->bl.m)->users)
				clif_spawn(&nd->bl);
		}
	}
	else {
		// we skip map_addnpc, but still add it to the list of ID's
		map_addiddb(&nd->bl);
	}
	strdb_put(npcname_db, nd->exname, nd);

	if (type != NPCTYPE_SCRIPT) {
		//return end;
		script_pushint(st, nd->bl.id);
		return SCRIPT_CMD_SUCCESS;
	}

	//-----------------------------------------
	// Loop through labels to export them as necessary
	for (i = 0; i < nd->u.scr.label_list_num; i++) {
		if (npc_event_export(nd, i)) {
// 			ShowWarning("npc_parse_duplicate : duplicate event %s::%s (%s)\n",
// 				nd->exname, nd->u.scr.label_list[i].name, filepath);
			ShowWarning("buildin_copynpc: duplicate event %s::%s (%s)\n", nd->exname, nd->u.scr.label_list[i].name, filepath);
		}
		npc_timerevent_export(nd, i);
	}

	//if (!strcmp(filepath, "INSTANCING")) //Instance NPCs will use this for commands
	//	nd->instance_id = mapdata->instance_id;

	// 如果复制出来的 NPC 出现在一个副本地图中, 那么该 NPC 应隶属该副本
	if (mapdata->instance_id) {
		nd->instance_id = mapdata->instance_id;
	}

	nd->u.scr.timerid = INVALID_TIMER;

	//return end;
	script_pushint(st, nd->bl.id);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Copynpc

#ifdef Pandas_ScriptCommand_GetTimeFmt
/* ===========================================================
 * 指令: gettimefmt
 * 描述: 将当前时间格式化输出成字符串, 是 gettimestr 的改进版
 * 用法: gettimefmt <"时间格式化标准">{,<要转换的秒数>{,<是否格式化成 UTC 时间>}};
 * 返回: 成功则返回被格式化的时间, 失败则返回空字符串
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(gettimefmt) {
	const char *fmtstr = script_getstr(st, 2);
	int is_utc = 0, result = 0, default_len = 32;
	time_t time_tick = time(NULL);

	if (script_hasdata(st, 3)) {
		if (!script_isint(st, 3)) {
			ShowError("buildin_gettimefmt: The 'time_tick' param must be a integer.\n");
			script_pushconststr(st, "");
			return SCRIPT_CMD_FAILURE;
		}
		time_tick = (time_t)script_getnum(st, 3);
	}

	if (script_hasdata(st, 4)) {
		if (!script_isint(st, 4)) {
			ShowError("buildin_gettimefmt: The 'is_utc' param must be a integer.\n");
			script_pushconststr(st, "");
			return SCRIPT_CMD_FAILURE;
		}
		is_utc = cap_value(script_getnum(st, 4), 0, 1);
	}

	struct tm *now_time = (is_utc ? gmtime(&time_tick) : localtime(&time_tick));
	char* buf = (char *)aMalloc(default_len + 1);
	result = strftime(buf, default_len, fmtstr, now_time);

	if (!result) {
		buf = (char *)aRealloc(buf, (default_len * 4) + 1);
		result = strftime(buf, (default_len * 4), fmtstr, now_time);
	}

	if (!result) {
		if (buf) aFree(buf);
		ShowError("buildin_gettimefmt: time format is too long : %s\n", fmtstr);
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	script_pushstr(st, buf);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetTimeFmt

#ifdef Pandas_ScriptCommand_MultiCatchPet
/* ===========================================================
 * 指令: multicatchpet
 * 描述: 与 catchpet 指令类似, 但可以指定更多支持捕捉的魔物编号
 * 用法: multicatchpet <魔物编号>{,<魔物编号>...};
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(multicatchpet) {
	TBL_PC* sd;
	unsigned int i = 2;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;

	if (!script_hasdata(st, i)) {
		ShowError("buildin_%s: no arguments given!\n", script_getfuncname(st));
		st->state = END;
		return SCRIPT_CMD_FAILURE;
	}

	while (script_hasdata(st, i)) {
		struct script_data* data;

		data = script_getdata(st, i);

		if (data_isint(data)) {
			sd->pandas.multi_catch_target_class.push_back(script_getnum(st, i));
		}
		else {
			ShowError("buildin_%s: The No.%d parameter is not integer type.\n", script_getfuncname(st), i - 1);
			script_reportdata(data);
			st->state = END;
			return SCRIPT_CMD_FAILURE;
		}

		i++;
	}

	pet_catch_process1(sd, PET_CATCH_MULTI_TARGET);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_MultiCatchPet

#ifdef Pandas_ScriptCommand_SelfDeletion

//************************************
// Method:      selfdeletion_exec_endtalk
// Description: 当一个玩家结束与某 NPC 的交互时, 检查是否需要自毁此 NPC
// Parameter:   struct script_state * st
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2019/11/25 01:04
//************************************
void selfdeletion_exec_endtalk(struct script_state* st) {
	if (!st->oid) return;

	TBL_NPC* nd = map_id2nd(st->oid);
	if (!nd || nd->pandas.destruction_strategy != 1) return;

	nd->pandas.destruction_timer = add_timer(
		gettick() + 500, selfdeletion_timer, st->oid, st->rid
	);
}

//************************************
// Method:      selfdeletion_timer
// Description: 配合 selfdeletion 指令使用的计时器
// Author:      Sola丶小克(CairoLee)  2019/11/25 01:03
//************************************
TIMER_FUNC(selfdeletion_timer) {
	TBL_NPC* nd = map_id2nd(id);
	if (!nd) return 1;

	if (nd->pandas.destruction_timer != INVALID_TIMER) {
		delete_timer(nd->pandas.destruction_timer, selfdeletion_timer);
		nd->pandas.destruction_timer = INVALID_TIMER;
	}

	if (nd->pandas.destruction_strategy == 1) {
		// 执行结束对话后是否自毁的检测
		// 若当前没有其他玩家正在与此 NPC 交互则自动卸载此 NPC
		struct s_mapiterator* iter = nullptr;
		struct block_list* bl = nullptr;
		iter = mapit_geteachpc();
		bool interacting = false;

		for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
			if (!bl || bl->m != nd->bl.m) continue;

			struct script_state* bl_st = ((TBL_PC*)bl)->st;
			if (bl_st == nullptr) continue;

			if (bl_st->oid == id) {
				interacting = true;
				break;
			}
		}
		if (iter) mapit_free(iter);

		if (!interacting) {
			npc_unload_duplicates(nd);
			npc_unload(nd, true);
#ifndef Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent
			npc_read_event_script();
#endif // Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent
		}
	}
	else {
		npc_unload_duplicates(nd);
		npc_unload(nd, true);
#ifndef Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent
		npc_read_event_script();
#endif // Pandas_Speedup_Unloadnpc_Without_Refactoring_ScriptEvent
	}

	return 1;
}

/* ===========================================================
 * 指令: selfdeletion
 * 描述: 设置 NPC 的自毁策略
 * 用法: selfdeletion <自毁策略>;
 * 返回: 该指令无论成功失败, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(selfdeletion) {
	TBL_PC* sd = map_id2sd(st->rid);
	int option = (script_hasdata(st, 2) ? script_getnum(st, 2) : SELFDEL_NOW);

	// SELFDEL_NOW = 立刻终止全部与此 NPC 相关的玩家对话, 并立刻自毁
	// SELFDEL_WAITFREE = 与最后一个与玩家的交互结束后自毁
	// SELFDEL_CANCEL = 取消与最后一个与玩家的交互结束后自毁

	TBL_NPC* nd = map_id2nd(st->oid);
	bool immediately = (option == SELFDEL_NOW);

	if (sd && sd->state.using_fake_npc) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (nd->bl.id == fake_nd->bl.id) {
		return SCRIPT_CMD_SUCCESS;
	}
	
	if (!immediately) {
		nd->pandas.destruction_strategy = (option == SELFDEL_WAITFREE ? 1 : 0);
		return SCRIPT_CMD_SUCCESS;
	}

	// 能执行到这里表示期望立刻销毁当前的 NPC
	// 先看看当前地图有多少人正在和此 NPC 对话, 且其 st 状态的 mes_active 为 1 的
	// 若发现了则先送此玩家一个关闭按钮, 并将他和此 NPC 的脚本流设置为 END
	struct s_mapiterator* iter = nullptr;
	struct block_list* bl = nullptr;
	iter = mapit_geteachpc();

	for (bl = mapit_first(iter); mapit_exists(iter); bl = mapit_next(iter)) {
		if (!bl || bl->m != nd->bl.m) continue;
		if (bl->id == st->rid) continue; // 不结束当前对话

		struct script_state* bl_st = ((TBL_PC*)bl)->st;
		if (bl_st == nullptr || bl_st->oid != st->oid) continue;

		pc_close_npc(((TBL_PC*)bl), 1 | 4);
	}
	if (iter) mapit_free(iter);

	// 其他正在与当前 NPC 发生对话的角色, 至此已经全部发送了关闭按钮并终止了后续脚本执行
	// 接下来设置一个定时器, 时间到了把当前 NPC 直接 unload 掉
	nd->pandas.destruction_timer = add_timer(
		gettick() + 500, selfdeletion_timer, st->oid, st->rid
	);

	if (sd) {
		// 关闭当前 NPC 和当前玩家的对话, 若存在对话框则送一个关闭按钮
		pc_close_npc(sd, 1 | 4);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SelfDeletion

#ifdef Pandas_ScriptCommand_SetCharTitle
/* ===========================================================
 * 指令: setchartitle
 * 描述: 设置指定玩家的称号ID
 * 用法: setchartitle <称号ID>{,<角色编号>};
 * 返回: 设置成功则返回 1, 设置失败则返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(setchartitle) {
#if PACKETVER < 20150513
	ShowWarning("%s: Title System it requires PACKETVER 2015-05-13 or newer...\n", __func__);
	script_pushint(st, 0);
	return SCRIPT_CMD_FAILURE;
#endif

	TBL_PC* sd = nullptr;
	uint32 title_id = max(script_getnum(st, 2), 0);

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, (npc_change_title_event(sd, title_id, 1) ? 1 : 0));
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetCharTitle

#ifdef Pandas_ScriptCommand_GetCharTitle
/* ===========================================================
 * 指令: getchartitle
 * 描述: 获得指定玩家的称号ID
 * 用法: getchartitle {<角色编号>};
 * 返回: 返回玩家的称号ID (若为 0 则表示此玩家没有称号), 获取失败则返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getchartitle) {
#if PACKETVER < 20150513
	ShowWarning("%s: Title System it requires PACKETVER 2015-05-13 or newer...\n", __func__);
	script_pushint(st, -1);
	return SCRIPT_CMD_FAILURE;
#endif

	TBL_PC* sd = nullptr;

	if (!script_charid2sd(2, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, sd->status.title_id);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetCharTitle

#ifdef Pandas_ScriptCommand_NpcExists
/* ===========================================================
 * 指令: npcexists
 * 描述: 判断指定名称的 NPC 是否存在, 就算不存在控制台也不会报错
 * 用法: npcexists "<NPC名称>"{,<用于保存 GameID 的变量>};
 * 返回: 存在返回 1, 不存在返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(npcexists) {
	struct script_data* vardata = nullptr;
	TBL_PC* sd = nullptr;
	int64 num = 0;
	const char* name = nullptr;

	if (script_hasdata(st, 3)) {
		vardata = script_getdata(st, 3);

		if (!data_isreference(vardata)) {
			ShowWarning("%s: GameID value is not a variable.\n", __func__);
			script_reportdata(vardata);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}

		if (is_string_variable(reference_getname(vardata))) {
			ShowWarning("%s: %s is not a integer variable.\n", __func__, reference_getname(vardata));
			script_reportdata(vardata);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}

		num = reference_getuid(vardata);
		name = reference_getname(vardata);
		char prefix = *name;

		if (not_server_variable(prefix) && !script_rid2sd(sd)) {
			ShowError("%s: variable '%s' for GameID is not a server variable, but no player is attached!\n", __func__, name);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}

		// 将用于保存 GameID 的变量内容设置为 0 
		clear_reg(st, sd, num, name, script_getref(st, 3));
	}

	TBL_NPC* nd = nullptr;
	if ((nd = npc_name2id(script_getstr(st, 2))) != nullptr) {
		script_pushint(st, 1);
		if (vardata != nullptr) {
			set_reg_num(st, sd, num, name, nd->bl.id, script_getref(st, 3));
		}
	}
	else {
		script_pushint(st, 0);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_NpcExists

#ifdef Pandas_ScriptCommand_StorageGetItem
/* ===========================================================
 * 指令: storagegetitem
 * 描述: 往仓库直接创造一个指定的道具
 * 用法: storagegetitem <物品编号>,<数量>{,<账号编号>};
 * 用法: storagegetitem "<物品名称>",<数量>{,<账号编号>};
 * 返回: 添加成功会返回 0, 返回小于 0 则表示有错误
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(storagegetitem) {
	int get_count = 0, i = 0;
	t_itemid nameid = 0;
	unsigned short amount = 0;
	struct item it = { 0 };
	TBL_PC* sd = nullptr;
	unsigned char flag = 0;
	std::shared_ptr<item_data> id;

	if (script_isstring(st, 2)) {// "<item name>"
		const char* name = script_getstr(st, 2);

		id = item_db.searchname(name);

		if (id == nullptr) {
			ShowError("buildin_storagegetitem: Nonexistant item %s requested.\n", name);
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS; //No item created.
		}
		nameid = id->nameid;
	}
	else {// <item id>
		nameid = script_getnum(st, 2);

		id = item_db.find(nameid);

		if (id == nullptr) {
			ShowError("buildin_storagegetitem: Nonexistant item %u requested.\n", nameid);
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS; //No item created.
		}
	}

	// <amount>
	if ((amount = script_getnum(st, 3)) <= 0) {
		script_pushint(st, -2);
		return SCRIPT_CMD_SUCCESS; //return if amount <=0, skip the useles iteration
	}

	memset(&it, 0, sizeof(it));
	it.nameid = nameid;
	it.identify = 1;
	it.bound = BOUND_NONE;

	if (!strcmp(script_getfuncname(st), "storagegetitembound")) {
		char bound = script_getnum(st, 4);
		if (bound < BOUND_NONE || bound >= BOUND_MAX) {
			ShowError("script_storagegetitembound: Not a correct bound type! Type=%d\n", bound);
			script_pushint(st, -3);
			return SCRIPT_CMD_SUCCESS;
		}
		script_mapid2sd(5, sd);
		it.bound = bound;
	}
	else {
		script_mapid2sd(4, sd);
	}

	if (sd == NULL) {
		script_pushint(st, -4);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->state.storage_flag == 1 || sd->state.storage_flag == 3) {
		script_pushint(st, -5);
		return SCRIPT_CMD_SUCCESS;
	}

	//Check if it's stackable.
	if (!itemdb_isstackable2(id.get()))
		get_count = 1;
	else
		get_count = amount;

	if (!itemdb_canstore(&it, pc_get_group_level(sd))) {
		script_pushint(st, -6);
		return SCRIPT_CMD_SUCCESS;
	}

	if (pet_db_search(nameid, PET_EGG)) {
		script_pushint(st, -7);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!sd->storage.state.put) {
		script_pushint(st, -8);
		return SCRIPT_CMD_SUCCESS;
	}

	for (i = 0; i < amount; i += get_count) {
		if (storage_additem(sd, &sd->storage, &it, get_count, true)) {
			if (pc_candrop(sd, &it))
				map_addflooritem(&it, get_count, sd->bl.m, sd->bl.x, sd->bl.y, 0, 0, 0, 0, 0);
			else
				script_pushint(st, -9);
		}
	}

	script_pushint(st, 0);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_StorageGetItem

#ifdef Pandas_ScriptCommand_SetInventoryInfo
/* ===========================================================
 * 指令: setinventoryinfo
 * 描述: 设置指定背包序号道具的部分详细信息, 与 getinventoryinfo 对应
 * 用法: setinventoryinfo <道具的背包序号>,<要设置的信息类型>,<值>{{,<标记位>},<角色编号>};
 * 返回: 设置成功则返回 1, 设置失败或角色不存在则返回 0, 返回负数也是失败 (值表示不同失败原因)
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(setinventoryinfo) {
	struct map_session_data* sd = nullptr;
	int expire_tick = 0, flag = 0;
	int idx = script_getnum(st, 2);
	int type = script_getnum(st, 3);
	uint64 value = script_getnum(st, 4);
	bool need_recalc_status = false;

	if (!script_charid2sd(6, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (idx < 0 || idx >= sd->status.inventory_slots || !sd->inventory_data[idx]) {
		ShowError("buildin_setinventoryinfo: Nonexistant item index.\n");
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 5) && script_isint(st, 5)) {
		//   0 : 维持默认行为
		// & 1 : 跳过角色能力重算
		// & 2 : 跳过全量刷新角色的背包数据
		// & 4 : 忽略卡片插入合理性的校验(必须是卡片, 卡片位置和装备位置符合)
		// & 8 : 插入卡片无需写入 picklog 记录(凭空生成)
		flag = script_getnum(st, 5);
	}

	switch (type)
	{
	case 3:
		value = cap_value(value, 0, MAX_REFINE);
		sd->inventory.u.items_inventory[idx].refine = (char)value;
		need_recalc_status = true;
		break;
	case 4:
		value = cap_value(value, 0, 1);
		sd->inventory.u.items_inventory[idx].identify = (char)value;
		if (sd->inventory.u.items_inventory[idx].equip && !value)
			pc_unequipitem(sd, idx, 3);
		break;
	case 5:
		value = cap_value(value, 0, 1);
		sd->inventory.u.items_inventory[idx].attribute = (char)value;
		if (sd->inventory.u.items_inventory[idx].equip && value)
			pc_unequipitem(sd, idx, 3);
		break;
	case 6: case 7: case 8: case 9: {
		if (!value) {
			sd->inventory.u.items_inventory[idx].card[type - 6] = 0;
			need_recalc_status = true;
			break;
		}

		std::shared_ptr<item_data> card_itemdata = item_db.find((t_itemid)value);
		if (!card_itemdata) {
			ShowWarning("buildin_setinventoryinfo: Nonexistant item : %u.\n", value);
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
		if (!(flag & 4)) {
			if (card_itemdata->type != IT_CARD || (sd->inventory_data[idx]->equip & card_itemdata->equip) == 0) {
				script_pushint(st, -1);
				return SCRIPT_CMD_SUCCESS;
			}
		}
		sd->inventory.u.items_inventory[idx].card[type - 6] = card_itemdata->nameid;
		if (!(flag & 8)) {
			struct item item_tmp = { 0 };
			item_tmp.nameid = card_itemdata->nameid;
			item_tmp.identify = 1;
			if (card_itemdata->flag.guid && !item_tmp.unique_id)
				item_tmp.unique_id = pc_generate_unique_id(sd);
			log_pick_pc(sd, LOG_TYPE_SCRIPT, 1, &item_tmp);
		}
		need_recalc_status = true;
		break;
	}
	case 10:
		value = cap_value(value, 0, INT_MAX);
		sd->inventory.u.items_inventory[idx].expire_time = (unsigned int)value; // Timestamp, not seconds
		expire_tick = (unsigned int)(sd->inventory.u.items_inventory[idx].expire_time - time(NULL));
		if (expire_tick > 0) {
			unsigned int seconds = (unsigned int)(value - time(NULL));
			clif_rental_time(sd, sd->inventory.u.items_inventory[idx].nameid, seconds);
			pc_inventory_rental_add(sd, seconds);
		}
		else {
			inventory_rental_update(sd);
		}
		need_recalc_status = true;
		break;
	case 11:
		sd->inventory.u.items_inventory[idx].unique_id = value;
		break;
	case 12: case 13: case 14: case 15: case 16:
		if (value) {
			sd->inventory.u.items_inventory[idx].option[type - 12].id = (short)value;
		}
		else {
			sd->inventory.u.items_inventory[idx].option[type - 12].id = 0;
			sd->inventory.u.items_inventory[idx].option[type - 12].value = 0;
			sd->inventory.u.items_inventory[idx].option[type - 12].param = 0;
		}
		need_recalc_status = true;
		break;
	case 17: case 18: case 19: case 20: case 21:
		sd->inventory.u.items_inventory[idx].option[type - 17].value = (short)value;
		need_recalc_status = true;
		break;
	case 22: case 23: case 24: case 25: case 26:
		sd->inventory.u.items_inventory[idx].option[type - 22].param = (char)value;
		need_recalc_status = true;
		break;
	case 27:
		if (value < BOUND_NONE || value >= BOUND_MAX) {
			ShowWarning("buildin_setinventoryinfo: Invalid bound type\n");
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
		sd->inventory.u.items_inventory[idx].bound = (char)value;
		break;
	case 28:
		if (value < 0 || value > MAX_ENCHANTGRADE) {
			ShowWarning("buildin_setinventoryinfo: The Value should be in range 0-%d, but you passed %d.\n", MAX_ENCHANTGRADE, value);
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
		sd->inventory.u.items_inventory[idx].enchantgrade = (uint8)value;
		need_recalc_status = true;
		break;
	case 29:
		sd->inventory.u.items_inventory[idx].equipSwitch = (unsigned int)value;
		break;
	case 30:
		value = cap_value(value, 0, 1);
		sd->inventory.u.items_inventory[idx].favorite = (char)value;
		break;
	default:
		ShowWarning("buildin_setinventoryinfo: The type should be in range 3-%d, currently type is: %d.\n", 30, type);
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (need_recalc_status && !(flag & 1))
		status_calc_pc(sd, SCO_NONE);

	if (!(flag & 2))
		clif_inventorylist(sd);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_SetInventoryInfo

#ifdef Pandas_ScriptCommand_UpdateInventory
/* ===========================================================
 * 指令: updateinventory
 * 描述: 重新下发关联玩家的背包数据给客户端 (刷新客户端背包数据)
 * 用法: updateinventory {<角色编号>};
 * 返回: 返回 1 表示成功, 0 表示失败
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(updateinventory) {
	struct map_session_data* sd = nullptr;
	if (!script_charid2sd(2, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	clif_inventorylist(sd);
	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_UpdateInventory

#ifdef Pandas_ScriptCommand_GetCharMacAddress
/* ===========================================================
 * 指令: getcharmac
 * 描述: 获取指定角色登录时使用的 MAC 地址
 * 用法: getcharmac(<账户编号>/<角色编号>/<"角色名称">);
 * 返回: 成功则返回 MAC 地址, 失败则返回空字符串
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getcharmac) {
	struct map_session_data* sd = nullptr;
	if (script_hasdata(st,2)) {
		if (script_isstring(st,2)) {
			sd = map_nick2sd(script_getstr(st,2), false);
		}
		else {
			int id = script_getnum(st,2);
			sd = (map_id2sd(id) ? map_id2sd(id) : map_charid2sd(id));
		}
	}
	else {
		script_rid2sd(sd);
	}

	if (!sd || !strlen(session[sd->fd]->mac_address)) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd && sd->fd && session[sd->fd]) {
		script_pushstrcopy(st, session[sd->fd]->mac_address);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushconststr(st, "");
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetCharMacAddress

#ifdef Pandas_ScriptCommand_GetConstant
/* ===========================================================
 * 指令: getconstant
 * 描述: 查询一个常量字符串对应的数值
 * 用法: getconstant <"常量字符串">;
 * 返回: 成功则返回常量对应的数值, 查询失败则返回 -255
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getconstant) {
	const char* name = script_getstr(st, 2);
	int64 value = 0;

	if (script_get_constant(name, &value)) {
		script_pushint(st, value);
	}
	else {
		script_pushint(st, -255);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetConstant

#ifdef Pandas_ScriptCommand_Preg_Search
/* ===========================================================
 * 指令: preg_search
 * 描述: 使用正则表达式搜索并返回首个匹配的分组内容
 * 用法: preg_search <"字符串">,<"匹配表达式">,<拓展标记位>,<存放匹配结果的字符串数组>;
 * 返回: 返回负数表示错误, 其他正整数表示匹配到的分组个数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(preg_search) {
	struct map_session_data* sd = nullptr;
	std::string text = std::string(script_getstr(st, 2));
	std::string patterns = std::string(script_getstr(st, 3));
	int flag = cap_value(script_getnum(st, 4), 0, 1);
	struct script_data* retdata = script_getdata(st, 5);
	struct reg_db* src_reg_db = nullptr;

	if (!data_isreference(retdata)) {
		ShowError("buildin_preg_search: error argument! please give a array variable for save gameid.\n");
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	int retid = reference_getid(retdata);
	const char* retname = reference_getname(retdata);

	if (not_server_variable(*retname)) {
		if (!script_rid2sd(sd)) {
			ShowError("buildin_preg_search: '%s' is not server variable, please attach to a player.\n", retname);
			script_reportdata(retdata);
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}
	}

	if (!(src_reg_db = script_array_src(st, sd, retname, reference_getref(retdata)))) {
		ShowError("buildin_preg_search: variable '%s' is not a array.\n", retname);
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (!is_string_variable(retname)) {
		ShowError("buildin_preg_search: the array variable '%s' must be string type.\n", retname);
		script_reportdata(retdata);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	// 以下用于清空存放返回数据的字符串数组 (参考 script_cleararray_pc 的实现)
	script_array_ensure_zero(st, NULL, retdata->u.num, reference_getref(retdata));
	struct script_array* sa = static_cast<script_array*>(idb_get(src_reg_db->arrays, retid));
	unsigned int array_len = script_array_highest_key(st, sd, retname, reference_getref(retdata));

	if (sa) {
		// 若给定的数组是存在的, 那么需要清空一下
		unsigned int* list = script_array_cpy_list(sa);
		unsigned int size = sa->size;

		for (unsigned int i = 0; i < size; i++) {
			clear_reg(st, sd, reference_uid(retid, list[i]), retname, reference_getref(retdata));
		}
	}

	try
	{
		boost::regex re;
		if (flag & 1) {
			re = boost::regex(patterns, boost::regex::icase);
		}
		else {
			re = boost::regex(patterns);
		}

		boost::smatch match_result;

		if (!boost::regex_search(text, match_result, re)) {
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS;
		}

		for (int i = 0; i < match_result.size(); i++) {
			int64 uid = reference_uid(retid, i);
			set_reg_str(st, sd, uid, retname, match_result[i].str().c_str(), reference_getref(retdata));
		}

		script_pushint(st, match_result.size());
	}
	catch (const boost::regex_error& e)
	{
		ShowError("%s: throw regex_error : %s\n", __func__, e.what());
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Preg_Search

#ifdef Pandas_ScriptCommand_Aura
/* ===========================================================
 * 指令: aura
 * 描述: 激活指定的光环组合
 * 用法: aura <光环编号>{,<角色编号>};
 * 返回: 成功返回 1 失败返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(aura) {
	uint32 aura_id = script_getnum(st, 2);
	struct map_session_data* sd = nullptr;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	aura_id = max(aura_id, 0);

	if (aura_id && !aura_search(aura_id)) {
		ShowError("buildin_aura: The specified aura id '%d' is invalid.\n", aura_id);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	aura_make_effective(&sd->bl, aura_id);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Aura

#ifdef Pandas_ScriptCommand_UnitAura
/* ===========================================================
 * 指令: unitaura
 * 描述: 用于调整七种单位的光环组合 (但仅 BL_PC 会被持久化)
 * 用法: unitaura <单位编号>,<光环编号>;
 * 返回: 成功返回 1 失败返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(unitaura) {
	uint32 aura_id = script_getnum(st, 3);
	struct s_unit_common_data* ucd = nullptr;
	struct block_list* bl = nullptr;

	if (!script_rid2bl(2, bl)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	ucd = status_get_ucd(bl);
	if (!ucd) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	aura_id = max(aura_id, 0);

	if (aura_id && !aura_search(aura_id)) {
		ShowError("buildin_unitaura: The specified aura id '%d' is invalid.\n", aura_id);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	aura_make_effective(bl, aura_id);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_UnitAura

#ifdef Pandas_ScriptCommand_GetUnitTarget
/* ===========================================================
 * 指令: getunittarget
 * 描述: 该指令用于获取指定单位当前正在攻击的目标单位编号
 * 用法: getunittarget <游戏单位编号>;
 * 返回: 目标的 GameID, 返回 0 表示没有目标
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getunittarget) {
	struct block_list* bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));
	script_pushint(st, 0);

	if (!bl) {
		return SCRIPT_CMD_SUCCESS;
	}

	switch (bl->type) {
	case BL_PC:  script_pushint(st, ((TBL_PC*)bl)->ud.target); break;
	case BL_MOB: script_pushint(st, ((TBL_MOB*)bl)->target_id); break;
	case BL_NPC: script_pushint(st, ((TBL_NPC*)bl)->ud.target); break;
	case BL_HOM: script_pushint(st, ((TBL_HOM*)bl)->ud.target); break;
	case BL_MER: script_pushint(st, ((TBL_MER*)bl)->ud.target); break;
	case BL_PET: script_pushint(st, ((TBL_PET*)bl)->target_id); break;
	case BL_ELEM: script_pushint(st, ((TBL_ELEM*)bl)->ud.target); break;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetUnitTarget

#ifdef Pandas_ScriptCommand_UnlockCmd
/* ===========================================================
 * 指令: unlockcmd
 * 描述: 解锁实时事件和过滤器事件的指令限制, 只能用于实时或过滤器事件
 * 用法: unlockcmd;
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(unlockcmd) {
	struct map_session_data* sd = nullptr;
	sd = map_id2sd(st->rid);

	if (!sd) {
 		return SCRIPT_CMD_SUCCESS;
	}

	if (!npc_event_is_realtime(sd->pandas.workinevent)) {
		ShowError("buildin_unlockcmd: This command can only be used for Filter or Express Event.\n");
		return SCRIPT_CMD_FAILURE;
	}

	st->unlockcmd = 1;
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_UnlockCmd

#ifdef Pandas_ScriptCommand_BattleRecordQuery
/* ===========================================================
 * 指令: batrec_query
 * 描述: 查询指定单位的战斗记录, 查看与交互目标单位产生的具体记录值
 * 用法: batrec_query <记录宿主的单位编号>,<交互目标的单位编号>,<记录类型>{,<聚合规则>};
 * 返回: 返回 -1 表示查无记录, 含 0 正整数表示伤害值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(batrec_query) {
	struct block_list* bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));
	if (!bl) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	int rec_type = script_getnum(st, 4);
	if (rec_type != BRT_DMG_RECEIVE && rec_type != BRT_DMG_CAUSE) {
		ShowError("%s: The battle record type is not invalid.\n", __func__);
		return SCRIPT_CMD_FAILURE;
	}

	int aggregation = BRA_COMBINE;
	if (!script_get_optnum(st, 5, "Aggregation strategy", aggregation, true, BRA_COMBINE)) {
		return SCRIPT_CMD_FAILURE;
	}

	int64 damage = batrec_query(
		bl, script_getnum(st, 3), (e_batrec_type)rec_type, (e_batrec_agg)aggregation
	);

	script_pushint(st, damage);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BattleRecordQuery

#ifdef Pandas_ScriptCommand_BattleRecordRank
/* ===========================================================
 * 指令: batrec_rank
 * 描述: 查询指定单位的战斗记录并对记录的值进行排序, 返回排行榜单
 * 用法: batrec_rank <记录宿主的单位编号>,<返回交互目标的单位编号数组>,<返回记录值数组>,<记录类型>{,<聚合规则>{,<排序规则>}};
 * 返回: 失败返回 -1, 含 0 正整数表示数组中返回的榜单记录数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(batrec_rank) {
	struct map_session_data* sd = nullptr;
	sd = map_id2sd(st->rid);

	struct block_list* bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));
	if (!bl) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	int gid_array_varid = 0;
	char* gid_array_varname = nullptr;
	struct script_data* gid_array_vardata = nullptr;
	if (!script_get_array(st, 3, gid_array_varid, gid_array_varname, gid_array_vardata)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	script_cleararray_st(st, 3);

	int dmg_array_varid = 0;
	char* dmg_array_varname = nullptr;
	struct script_data* dmg_array_vardata = nullptr;
	if (!script_get_array(st, 4, dmg_array_varid, dmg_array_varname, dmg_array_vardata)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	script_cleararray_st(st, 4);

	int rec_type = script_getnum(st, 5);
	if (rec_type != BRT_DMG_RECEIVE && rec_type != BRT_DMG_CAUSE) {
		ShowError("%s: The battle record type is not invalid.\n", __func__);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int aggregation = BRA_COMBINE;
	if (!script_get_optnum(st, 6, "Aggregation strategy", aggregation, true, BRA_COMBINE)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int sort_type = BRS_DESC;
	if (!script_get_optnum(st, 7, "Sort Type", sort_type, true, BRS_DESC)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	batrec_map* origin_rec = nullptr;
	if (!(origin_rec = batrec_getmap(bl, (e_batrec_type)rec_type))) {
		ShowError("%s: The battle record type is not invalid.\n", __func__);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	batrec_map rec;
	batrec_aggregation(origin_rec, rec, (e_batrec_agg)aggregation);

	std::vector<std::pair<uint32, s_batrec_item_ptr>> rec_sorted;
	for (auto& it : rec) {
		rec_sorted.push_back(it);
	}

	if (sort_type == BRS_DESC) {
		std::sort(rec_sorted.begin(), rec_sorted.end(), batrec_cmp_desc);
	}
	else {
		std::sort(rec_sorted.begin(), rec_sorted.end(), batrec_cmp_asc);
	}

	for (int i = 0; i < rec_sorted.size(); i++) {
		int64 uid = reference_uid(gid_array_varid, i);
		set_reg_num(st, sd, uid, gid_array_varname, rec_sorted[i].first, reference_getref(gid_array_vardata));

		uid = reference_uid(dmg_array_varid, i);
		set_reg_num(st, sd, uid, dmg_array_varname, rec_sorted[i].second->damage, reference_getref(dmg_array_vardata));
	}

	script_pushint(st, rec_sorted.size());
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BattleRecordRank

#ifdef Pandas_ScriptCommand_BattleRecordSortout
/* ===========================================================
 * 指令: batrec_sortout
 * 描述: 移除指定单位的战斗记录中交互单位已经不存在 (或下线) 的记录
 * 用法: batrec_sortout <记录宿主的单位编号>{,<记录类型>};
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(batrec_sortout) {
	struct block_list* bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));

	if (!bl) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (!script_hasdata(st, 3)) {
		batrec_sortout(bl);
		return SCRIPT_CMD_SUCCESS;
	}

	int rec_type = script_getnum(st, 3);
	if (rec_type != BRT_DMG_RECEIVE && rec_type != BRT_DMG_CAUSE) {
		ShowError("%s: The battle record type is not invalid.\n", __func__);
		return SCRIPT_CMD_FAILURE;
	}

	batrec_sortout(bl, (e_batrec_type)rec_type);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BattleRecordSortout

#ifdef Pandas_ScriptCommand_BattleRecordReset
/* ===========================================================
 * 指令: batrec_reset
 * 描述: 清除指定单位的战斗记录
 * 用法: batrec_reset <记录宿主的单位编号>;
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(batrec_reset) {
	struct block_list* bl = nullptr;
	bl = map_id2bl(script_getnum(st, 2));
	batrec_reset(bl);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BattleRecordReset

#ifdef Pandas_ScriptCommand_EnableBattleRecord
/* ===========================================================
 * 指令: enable_batrec
 * 描述: 启用指定单位的战斗记录
 * 用法: enable_batrec {<游戏单位编号>};
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(enable_batrec) {
	struct block_list* bl = nullptr;
	int unit_id = st->rid;

	if (script_hasdata(st, 2)) {
		unit_id = script_getnum(st, 2);
	}

	if (!(bl = map_id2bl(unit_id))) {
		return SCRIPT_CMD_SUCCESS;
	}

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) {
		return SCRIPT_CMD_SUCCESS;
	}

	ucd->batrec.dorecord = true;
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_EnableBattleRecord

#ifdef Pandas_ScriptCommand_DisableBattleRecord
/* ===========================================================
 * 指令: disable_batrec
 * 描述: 禁用指定单位的战斗记录
 * 用法: disable_batrec {<游戏单位编号>};
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(disable_batrec) {
	struct block_list* bl = nullptr;
	int unit_id = st->rid;

	if (script_hasdata(st, 2)) {
		unit_id = script_getnum(st, 2);
	}

	if (!(bl = map_id2bl(unit_id))) {
		return SCRIPT_CMD_SUCCESS;
	}

	struct s_unit_common_data* ucd = nullptr;
	if (!(ucd = status_get_ucd(bl))) {
		return SCRIPT_CMD_SUCCESS;
	}

	ucd->batrec.dorecord = false;
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_DisableBattleRecord

#ifdef Pandas_ScriptCommand_Login
/* ===========================================================
 * 指令: login
 * 描述: 将指定的角色以特定的登录模式拉上线
 * 用法: login <角色编号>{,<默认是否坐下>{,<默认身体朝向>{,<默认脑袋朝向>{,<登录模式>}}}};
 * 返回: 成功返回 1, 失败返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(login) {
	script_pushint(st, 0);

	uint32 charid = 0;
	charid = script_getnum(st, 2);

	int sit = 0;
	if (!script_get_optnum(st, 3, "Sitdown or not", sit, true, 0)) {
		return SCRIPT_CMD_FAILURE;
	}
	sit = cap_value(sit, 0, 1);

	int body_dir = DIR_SOUTH;
	if (!script_get_optnum(st, 4, "Body Direction", body_dir, true, DIR_SOUTH)) {
		return SCRIPT_CMD_FAILURE;
	}
	body_dir = cap_value(body_dir, 0, 7);

	int head_dir = 0;
	if (!script_get_optnum(st, 5, "Head Direction", head_dir, true, 0)) {
		return SCRIPT_CMD_FAILURE;
	}
	head_dir = cap_value(head_dir, 0, 2);

	int mode = SUSPEND_MODE_NONE;
	if (!script_get_optnum(st, 6, "Login Mode", mode, true, SUSPEND_MODE_NORMAL)) {
		return SCRIPT_CMD_FAILURE;
	}
	if (!suspend_mode_valid(mode)) {
		mode = SUSPEND_MODE_NORMAL;
	}

	if (suspend_recall(charid, (e_suspend_mode)mode, body_dir, head_dir, sit)) {
		script_pushint(st, 1);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Login

#ifdef Pandas_ScriptCommand_CheckSuspend
/* ===========================================================
 * 指令: checksuspend
 * 描述: 获取指定角色或指定账号当前在线角色的挂机模式
 * 用法: checksuspend {<角色编号|账号编号|"角色名称">};
 * 返回: 角色不存在返回 -1, 否则返回当前的挂机状态
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(checksuspend) {
	TBL_PC* sd = nullptr;

	if (script_hasdata(st, 2)) {
		if (script_isstring(st, 2))
			sd = map_nick2sd(script_getstr(st, 2), false);
		else {
			int id = script_getnum(st, 2);
			sd = map_id2sd(id);
			if (!sd)
				sd = map_charid2sd(id);
		}
	}
	else {
		if (!script_rid2sd(sd)) {
			script_pushint(st, -1);
			return SCRIPT_CMD_SUCCESS;
		}
	}

	if (!sd) {
		script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	if ((sd->state.autotrade & AUTOTRADE_OFFLINE) == AUTOTRADE_OFFLINE)
		script_pushint(st, SUSPEND_MODE_OFFLINE);
	else if ((sd->state.autotrade & AUTOTRADE_AFK) == AUTOTRADE_AFK)
		script_pushint(st, SUSPEND_MODE_AFK);
	else if ((sd->state.autotrade & AUTOTRADE_NORMAL) == AUTOTRADE_NORMAL)
		script_pushint(st, SUSPEND_MODE_NORMAL);
	else
		script_pushint(st, SUSPEND_MODE_NONE);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_CheckSuspend

#ifdef Pandas_ScriptCommand_BonusScriptRemove
/* ===========================================================
 * 指令: bonus_script_remove
 * 描述: 移除指定的 bonus_script 效果脚本
 * 用法: bonus_script_remove <效果脚本编号>{,<角色编号>};
 * 返回: 成功移除则返回 true, 找不到脚本代码或移除失败则返回 false
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(bonus_script_remove) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(3, sd)) {
		script_pushint(st, false);
		return SCRIPT_CMD_FAILURE;
	}

	uint64 bonus_id = script_getnum64(st, 2);
	if (pc_bonus_script_remove(sd, bonus_id))
		script_pushint(st, true);
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BonusScriptRemove

#ifdef Pandas_ScriptCommand_BonusScriptList
/* ===========================================================
 * 指令: bonus_script_list
 * 描述: 用于获取指定角色当前激活的全部 bonus_script 效果脚本编号
 * 用法: bonus_script_list <返回效果脚本编号的数值型数组>{,<角色编号>};
 * 返回: 获取到的脚本代码数量, 发生错误则返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(bonus_script_list) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(3, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int script_array_varid = 0;
	char* script_array_varname = nullptr;
	struct script_data* script_array_vardata = nullptr;
	if (!script_get_array(st, 2, script_array_varid, script_array_varname, script_array_vardata)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	script_cleararray_st(st, 2);

	uint16 count = 0;
	struct linkdb_node* node = NULL;
	struct s_bonus_script_entry* entry = NULL;
	if ((node = sd->bonus_script.head)) {
		while (node) {
			struct linkdb_node* next = node->next;
			entry = (struct s_bonus_script_entry*)node->data;
			if (!entry) continue;

			int64 uid = reference_uid(script_array_varid, count);
			set_reg_num(st, sd, uid, script_array_varname, entry->bonus_id, reference_getref(script_array_vardata));
			count++;
			node = next;
		}
	}

	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BonusScriptList

#ifdef Pandas_ScriptCommand_BonusScriptExists
/* ===========================================================
 * 指令: bonus_script_exists
 * 描述: 查询指定角色是否已经激活了特定的 bonus_script 效果脚本
 * 用法: bonus_script_exists <效果脚本编号>{,<角色编号>};
 * 返回: 效果已经存在则返回 true, 角色不在线或效果不存在否则返回 false
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(bonus_script_exists) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(3, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	uint64 bonus_id = script_getnum64(st, 2);
	if (pc_bonus_script_exists(sd, bonus_id))
		script_pushint(st, true);
	else
		script_pushint(st, false);

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BonusScriptExists

#ifdef Pandas_ScriptCommand_BonusScriptGetId
/* ===========================================================
 * 指令: bonus_script_getid
 * 描述: 用于查询效果脚本代码对应的效果脚本编号
 * 用法: bonus_script_getid <"效果脚本代码">,<返回效果脚本编号数组>{,<角色编号>};
 * 返回: 查询到的记录数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(bonus_script_getid) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(4, sd)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	const char* script_str = nullptr;
	script_str = script_getstr(st, 2);

	int ret_varid = 0;
	char* ret_varname = nullptr;
	struct script_data* ret_vardata = nullptr;
	if (!script_get_array(st, 3, ret_varid, ret_varname, ret_vardata)) {
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	script_cleararray_st(st, 3);

	uint16 count = 0;
	struct linkdb_node* node = NULL;
	struct s_bonus_script_entry* entry = NULL;

	if ((node = sd->bonus_script.head)) {
		while (node) {
			struct linkdb_node* next = node->next;
			entry = (struct s_bonus_script_entry*)node->data;
			if (strcmpi(script_str, StringBuf_Value(entry->script_buf)) == 0) {
				int64 uid = reference_uid(ret_varid, count);
				set_reg_num(st, sd, uid, ret_varname, entry->bonus_id, reference_getref(ret_vardata));
				count++;
			}
			node = next;
		}
	}

	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BonusScriptGetId

#ifdef Pandas_ScriptCommand_BonusScriptInfo
/* ===========================================================
 * 指令: bonus_script_info
 * 描述: 查询指定效果脚本的相关信息
 * 用法: bonus_script_info <效果脚本编号>,<查询类型>{,<角色编号>};
 * 返回: 直接返回所查询的结果值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(bonus_script_info) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(4, sd)) {
		script_pushint(st, -2);
		return SCRIPT_CMD_FAILURE;
	}

	uint64 bonus_id = script_getnum64(st, 2);
	int32 query_type = script_getnum(st, 3);

	bool found = false;
	struct linkdb_node* node = NULL;
	struct s_bonus_script_entry* entry = NULL;

	if ((node = sd->bonus_script.head)) {
		while (node) {
			struct linkdb_node* next = node->next;
			entry = (struct s_bonus_script_entry*)node->data;
			if (bonus_id == entry->bonus_id) {
				found = true;
				break;
			}
			node = next;
		}
	}

	if (!found) {
		script_pushint(st, -3);
		return SCRIPT_CMD_SUCCESS;
	}

	switch (query_type) {
	case 0:		// 效果脚本代码
		script_pushstrcopy(st, StringBuf_Value(entry->script_buf)); break;
	case 1:		// 标记位
		script_pushint(st, entry->flag); break;
	case 2:		// 状态图标编号
		script_pushint(st, entry->icon); break;
	case 3:		// 类型
		script_pushint(st, entry->type); break;
	case 4:		// 剩余时间 (毫秒)
		if (entry->tid == INVALID_TIMER) {
			script_pushint(st, -1);
			break;
		}
		script_pushint(st, DIFF_TICK(get_timer(entry->tid)->tick, gettick()));
		break;
	default:
		ShowWarning("buildin_bonus_script_info: The type should be in range 0-%d, currently type is: %d.\n", 4, query_type);
		script_pushint(st, -4);
		break;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_BonusScriptInfo

#ifdef Pandas_ScriptCommand_ExpandInventoryAdjust
/* ===========================================================
 * 指令: expandinventory_adjust
 * 描述: 增加角色的背包容量上限
 * 用法: expandinventory_adjust <增加多少容量>;
 * 返回: 调整成功返回 1, 失败返回 0
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(expandinventory_adjust) {
	TBL_PC* sd = nullptr;
	if (!script_rid2sd(sd)) {
		return SCRIPT_CMD_FAILURE;
	}

	int expand_count = script_getnum(st, 2);

	if (sd->status.inventory_slots + expand_count <= MAX_INVENTORY &&
		sd->status.inventory_slots + expand_count >= sd->inventory.amount &&
		sd->status.inventory_slots + expand_count >= INVENTORY_BASE_SIZE) {
		sd->status.inventory_slots += expand_count;
		clif_inventory_expansion_info(sd);
		chrif_save(sd, CSAVE_NORMAL);
		script_pushint(st, 1);
	}
	else {
		script_pushint(st, 0);
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_ExpandInventoryAdjust

#ifdef Pandas_ScriptCommand_GetInventorySize
/* ===========================================================
 * 指令: getinventorysize
 * 描述: 查询并获取当前角色的背包容量上限
 * 用法: getinventorysize {<角色编号>};
 * 返回: 找不到角色则返回 0, 否则返回查询到的背包容量
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getinventorysize) {
	TBL_PC* sd = nullptr;
	if (!script_charid2sd(2, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
	script_pushint(st, sd->status.inventory_slots);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetInventorySize

#ifdef Pandas_ScriptCommand_GetMapSpawns
/* ===========================================================
 * 指令: getmapspawns
 * 描述: 获取指定地图的魔物刷新点信息
 * 用法: getmapspawns "<地图名称>"{,<角色编号>};
 * 返回: 成功则返回找到的刷新点数量, 失败则返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getmapspawns) {
	int mapindex = -1;
	std::string mapname = script_getstr(st, 2);
	int char_id = script_hasdata(st, 3) ? script_getnum(st, 3) : 0;

	script_both_setreg(st, "spawn_count", 0, false, -1, char_id);

	if (!script_get_mapindex(st, mapname.c_str(), mapindex, char_id)) {
		ShowError("buildin_getmapspawns: Could not found valid map by map name '%s'\n", mapname.c_str());
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	struct map_data* mapdata = map_getmapdata(mapindex);

	if (!mapdata) {
		script_reportsrc(st);
		script_reportfunc(st);
		ShowError("buildin_getmapspawns: Could not found valid map by map name '%s'\n", mapname.c_str());
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int j = 0;

	for (int i = 0; i < MAX_MOB_LIST_PER_MAP; i++) {
		if (!mapdata || mapdata->moblist[i] == nullptr) continue;
		struct spawn_data* data = mapdata->moblist[i];
		if (!data) continue;
		std::shared_ptr<s_mob_db> mob = mob_db.find(data->id);
		if (!mob) continue;

		script_both_setreg(st, "spawn_mobid", data->id, true, j, char_id);
		script_both_setregstr(st, "spawn_name$", data->name, true, j, char_id);
		script_both_setreg(st, "spawn_num", data->num, true, j, char_id);
		script_both_setreg(st, "spawn_active", data->active, true, j, char_id);
		script_both_setreg(st, "spawn_size", data->state.size, true, j, char_id);
		script_both_setreg(st, "spawn_isboss", data->state.boss, true, j, char_id);
		script_both_setreg(st, "spawn_ai", data->state.ai, true, j, char_id);
		script_both_setreg(st, "spawn_level", (data->level > 0 ? data->level : mob->lv), true, j, char_id);
		script_both_setreg(st, "spawn_delay1", data->delay1, true, j, char_id);
		script_both_setreg(st, "spawn_delay2", data->delay2, true, j, char_id);
		script_both_setregstr(st, "spawn_eventname$", data->eventname, true, j, char_id);

		script_both_setreg(st, "spawn_mapid", data->m, true, j, char_id);
		script_both_setregstr(st, "spawn_mapname$", mapdata->name, true, j, char_id);
		script_both_setreg(st, "spawn_x", data->x, true, j, char_id);
		script_both_setreg(st, "spawn_y", data->y, true, j, char_id);
		script_both_setreg(st, "spawn_xs", data->xs, true, j, char_id);
		script_both_setreg(st, "spawn_ys", data->ys, true, j, char_id);

		j++;
	}

	script_both_setreg(st, "spawn_count", j, false, -1, char_id);
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetMapSpawns

#ifdef Pandas_ScriptCommand_GetMobSpawns
/* ===========================================================
 * 指令: getmobspawns
 * 描述: 查询指定魔物在不同地图的刷新点信息
 * 用法: getmobspawns <魔物编号>{,"<地图名称>"{,<角色编号>}};
 * 返回: 成功则返回找到的刷新点数量, 失败则返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getmobspawns) {
	int mapindex = -1;
	int mob_id = script_getnum(st, 2);
	std::string mapname = (script_hasdata(st, 3) && script_isstring(st, 3)) ? script_getstr(st, 3) : "";
	int char_id = (script_hasdata(st, 4) && script_isint(st, 4)) ? script_getnum(st, 4) : 0;

	script_both_setreg(st, "spawn_count", 0, false, -1, char_id);

	// 若指定了地图名称则顺带查询地图数据信息, 没指定就算了
	if (mapname.length() > 0) {
		if (!script_get_mapindex(st, mapname.c_str(), mapindex, char_id)) {
			ShowError("buildin_getmobspawns: Could not found valid map by map name '%s'\n", mapname.c_str());
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}

		struct map_data* mapdata = map_getmapdata(mapindex);

		if (!mapdata) {
			script_reportsrc(st);
			script_reportfunc(st);
			ShowError("buildin_getmobspawns: Could not found valid map by map name '%s'\n", mapname.c_str());
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
		}
	}

	// 通过 rAthena 内建的刷新点缓存快速确定关联地图
	const std::vector<spawn_info> spawns = mob_get_spawns(mob_id);

	int j = 0;

	for (auto& spawn : spawns) {
		int16 mapid = map_mapindex2mapid(spawn.mapindex);

		// 若设置了仅查询某地图的魔物刷新点信息, 那么非指定地图的都跳过
		if (mapindex != -1 && mapid != mapindex)
			continue;

		// 获取有记载指定魔物刷新点的目标地图数据
		struct map_data* mapdata = map_getmapdata(mapid);

		// 找不到对应地图的数据则直接跳过
		if (!mapdata) continue;

		for (int i = 0; i < MAX_MOB_LIST_PER_MAP; i++) {
			if (!mapdata || mapdata->moblist[i] == nullptr) continue;
			struct spawn_data* data = mapdata->moblist[i];
			if (!data) continue;
			if (data->id != mob_id) continue;
			std::shared_ptr<s_mob_db> mob = mob_db.find(data->id);
			if (!mob) continue;
			
			script_both_setreg(st, "spawn_mobid", data->id, true, j, char_id);
			script_both_setregstr(st, "spawn_name$", data->name, true, j, char_id);
			script_both_setreg(st, "spawn_num", data->num, true, j, char_id);
			script_both_setreg(st, "spawn_active", data->active, true, j, char_id);
			script_both_setreg(st, "spawn_size", data->state.size, true, j, char_id);
			script_both_setreg(st, "spawn_isboss", data->state.boss, true, j, char_id);
			script_both_setreg(st, "spawn_ai", data->state.ai, true, j, char_id);
			script_both_setreg(st, "spawn_level", (data->level > 0 ? data->level : mob->lv), true, j, char_id);
			script_both_setreg(st, "spawn_delay1", data->delay1, true, j, char_id);
			script_both_setreg(st, "spawn_delay2", data->delay2, true, j, char_id);
			script_both_setregstr(st, "spawn_eventname$", data->eventname, true, j, char_id);

			script_both_setreg(st, "spawn_mapid", data->m, true, j, char_id);
			script_both_setregstr(st, "spawn_mapname$", mapdata->name, true, j, char_id);
			script_both_setreg(st, "spawn_x", data->x, true, j, char_id);
			script_both_setreg(st, "spawn_y", data->y, true, j, char_id);
			script_both_setreg(st, "spawn_xs", data->xs, true, j, char_id);
			script_both_setreg(st, "spawn_ys", data->ys, true, j, char_id);

			j++;
		}
	}

	script_both_setreg(st, "spawn_count", j, false, -1, char_id);
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetMobSpawns

#ifdef Pandas_ScriptCommand_GetCalendarTime
/* ===========================================================
 * 指令: getcalendartime
 * 描述: 获取下次出现指定时间的 UNIX 时间戳
 * 用法: getcalendartime <小时>,<分钟>{,<月的第几天>{,<周的第几天>}};
 * 返回: 成功则返回时间戳, 失败则返回 -1
 * 作者: Haru <haru@dotalux.com>
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getcalendartime) {
	struct tm info = { 0 };
	int day_of_month = script_hasdata(st, 4) ? script_getnum(st, 4) : -1;
	int day_of_week = script_hasdata(st, 5) ? script_getnum(st, 5) : -1;
	int year = date_get_year();
	int month = date_get_month();
	int day = date_get_dayofmonth();
	int cur_hour = date_get_hour();
	int cur_min = date_get_min();
	int hour = script_getnum(st, 2);
	int minute = script_getnum(st, 3);

	info.tm_sec = 0;
	info.tm_min = minute;
	info.tm_hour = hour;
	info.tm_mday = day;
	info.tm_mon = month - 1;
	info.tm_year = year - 1900;

	if (day_of_month > -1 && day_of_week > -1) {
		ShowError("buildin_getcalendartime: You must only specify a day_of_week or a day_of_month, not both\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (day_of_month > -1 && (day_of_month < 1 || day_of_month > 31)) {
		ShowError("buildin_getcalendartime: Day of Month in invalid range. Must be between 1 and 31.\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (day_of_week > -1 && (day_of_week < 0 || day_of_week > 6)) {
		ShowError("buildin_getcalendartime: Day of Week in invalid range. Must be between 0 and 6.\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (hour > -1 && (hour > 23)) {
		ShowError("buildin_getcalendartime: Hour in invalid range. Must be between 0 and 23.\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (minute > -1 && (minute > 59)) {
		ShowError("buildin_getcalendartime: Minute in invalid range. Must be between 0 and 59.\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}
	if (hour == -1 || minute == -1) {
		ShowError("buildin_getcalendartime: Minutes and Hours are required\n");
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	if (day_of_month > -1) {
		if (day_of_month < day) { // Next Month
			info.tm_mon++;
		}
		else if (day_of_month == day) { // Today
			if (hour < cur_hour || (hour == cur_hour && minute < cur_min)) { // But past time, next month
				info.tm_mon++;
			}
		}

		// Loops until month has finding a month that has day_of_month
		do {
			time_t t;
			struct tm* lt;
			info.tm_mday = day_of_month;
			t = mktime(&info);
			lt = localtime(&t);
			info = *lt;
		} while (info.tm_mday != day_of_month);
	}
	else if (day_of_week > -1) {
		int cur_wday = date_get_dayofweek();

		if (day_of_week > cur_wday) { // This week
			info.tm_mday += (day_of_week - cur_wday);
		}
		else if (day_of_week == cur_wday) { // Today
			if (hour < cur_hour || (hour == cur_hour && minute <= cur_min)) {
				info.tm_mday += 7; // Next week
			}
		}
		else if (day_of_week < cur_wday) { // Next week
			info.tm_mday += (7 - cur_wday + day_of_week);
		}
	}
	else if (day_of_week == -1 && day_of_month == -1) { // Next occurence of hour/min
		if (hour < cur_hour || (hour == cur_hour && minute < cur_min)) {
			info.tm_mday++;
		}
	}

	script_pushint(st, mktime(&info));

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetCalendarTime

#ifdef Pandas_ScriptCommand_GetSkillInfo
/* ===========================================================
 * 指令: getskillinfo
 * 描述: 获取指定技能在技能数据库中所配置的各项信息
 * 用法: getskillinfo <查询的信息类型>,<技能编号>{,<技能等级>{,<角色编号>}};
 * 用法: getskillinfo <查询的信息类型>,<"技能名称">{,<技能等级>{,<角色编号>}};
 * 返回: 请查阅 doc/pandas_script_commands.txt 中的说明
 * 作者: 聽風
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getskillinfo) {
	int type = 0;
	uint16 skill_id = 0;
	int i = 0, j = 0;
	int skill_lv = (script_hasdata(st, 4) && script_isint(st, 4)) ? script_getnum(st, 4) : 0;
	int char_id = (script_hasdata(st, 5) && script_isint(st, 5)) ? script_getnum(st, 5) : 0;

	type = script_getnum(st, 2);
	if (script_isstring(st, 3)) {
		const char *name = script_getstr(st, 3);
		if (!(skill_id = skill_name2id(name))) {
			ShowError("buildin_getskillinfo: Invalid skill name %s.\n", name);
			return SCRIPT_CMD_FAILURE;
		}
	}
	else {
		skill_id = script_getnum(st, 3);
		if (!skill_get_index(skill_id)) {
			ShowError("buildin_getskillinfo: Invalid skill ID %d.\n", skill_id);
			return SCRIPT_CMD_FAILURE;
		}
	}

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);
	if (skill == nullptr) {
		ShowError("buildin_getskillinfo: Invalid skill ID %d.\n", skill_id);
		return SCRIPT_CMD_FAILURE;
	}

	switch (type) {
		case SKI_CASTTYPE: script_pushint(st, skill_get_casttype(skill_id)); break;
		case SKI_NAME: script_pushstrcopy(st, skill_get_name(skill_id)); break;
		case SKI_DESCRIPTION: script_pushstrcopy(st, skill_get_desc(skill_id)); break;
		case SKI_MAXLEVEL_IN_SKILLTREE: script_pushint(st, skill_tree_get_max(skill_id,skill_lv)); break;
		case SKI_SKILLTYPE: script_pushint(st, skill_get_type(skill_id)); break;
		case SKI_HIT: script_pushint(st, skill_get_hit(skill_id)); break;
		case SKI_TARGETTYPE: script_pushint(st, skill_get_inf(skill_id)); break;
		case SKI_ELEMENT: script_pushint(st, skill_get_ele(skill_id,skill_lv)); break;
		case SKI_MAXLEVEL: script_pushint(st, skill_get_max(skill_id)); break;
		case SKI_RANGE: script_pushint(st, skill_get_range(skill_id,skill_lv)); break;
		case SKI_SPLASHAREA: script_pushint(st, skill_get_splash(skill_id,skill_lv)); break;
		case SKI_HITCOUNT: script_pushint(st, skill_get_num(skill_id,skill_lv)); break;
		case SKI_CASTTIME: script_pushint(st, skill_get_cast(skill_id,skill_lv)); break;
#ifdef RENEWAL_CAST
		case SKI_FIXEDCASTTIME: script_pushint(st, skill_get_fixed_cast(skill_id,skill_lv)); break;
#else
		case SKI_FIXEDCASTTIME: script_pushint(st, -1); break;
#endif // RENEWAL_CAST
		case SKI_AFTERCASTACTDELAY: script_pushint(st, skill_get_delay(skill_id,skill_lv)); break;
		case SKI_AFTERCASTWALKDELAY: script_pushint(st, skill_get_walkdelay(skill_id,skill_lv)); break;
		case SKI_DURATION1: script_pushint(st, skill_get_time(skill_id,skill_lv)); break;
		case SKI_DURATION2: script_pushint(st, skill_get_time2(skill_id,skill_lv)); break;
		case SKI_CASTTIMEFLAGS: script_pushint(st, skill_get_castnodex(skill_id)); break;
		case SKI_CASTDELAYFLAGS: script_pushint(st, skill_get_delaynodex(skill_id)); break;
		case SKI_CASTDEFENSEREDUCTION: script_pushint(st, skill_get_castdef(skill_id)); break;
		case SKI_CASTCANCEL: script_pushint(st, skill_get_castcancel(skill_id)); break;
		case SKI_ACTIVEINSTANCE: script_pushint(st, skill_get_maxcount(skill_id,skill_lv)); break;
		case SKI_KNOCKBACK: script_pushint(st, skill_get_blewcount(skill_id,skill_lv)); break;
		case SKI_COOLDOWN: script_pushint(st, skill_get_cooldown(skill_id,skill_lv)); break;
		case SKI_NONEARNPC_TYPE: script_pushint(st, skill->unit_nonearnpc_type); break;
		case SKI_NONEARNPC_ADDITIONALRANGE: script_pushint(st, skill->unit_nonearnpc_range); break;
		case SKI_COPYFLAGS_SKILL: script_pushint(st, skill->copyable.option); break;
		case SKI_UNIT_ID: script_pushint(st, skill_get_unit_id(skill_id)); break;
		case SKI_UNIT_ALTERNATEID: script_pushint(st, skill_get_unit_id2(skill_id)); break;
		case SKI_UNIT_LAYOUT: script_pushint(st, skill_get_unit_layout_type(skill_id,skill_lv)); break;
		case SKI_UNIT_RANGE: script_pushint(st, skill_get_unit_range(skill_id,skill_lv)); break;
		case SKI_UNIT_INTERVAL: script_pushint(st, skill_get_unit_interval(skill_id)); break;
		case SKI_UNIT_TARGET: script_pushint(st, skill_get_unit_target(skill_id)); break;
		case SKI_REQUIRES_HPCOST: script_pushint(st, skill_get_hp(skill_id,skill_lv)); break;
		case SKI_REQUIRES_SPCOST: script_pushint(st, skill_get_sp(skill_id,skill_lv)); break;
		case SKI_REQUIRES_MAXHPTRIGGER: script_pushint(st, skill_get_mhp(skill_id,skill_lv)); break;
		case SKI_REQUIRES_HPRATECOST: script_pushint(st, skill_get_hp_rate(skill_id,skill_lv)); break;
		case SKI_REQUIRES_SPRATECOST: script_pushint(st, skill_get_sp_rate(skill_id,skill_lv)); break;
		case SKI_REQUIRES_ZENYCOST: script_pushint(st, skill_get_zeny(skill_id,skill_lv)); break;
		case SKI_REQUIRES_WEAPON: script_pushint(st, skill_get_weapontype(skill_id)); break;
		case SKI_REQUIRES_AMMO: script_pushint(st, skill_get_ammotype(skill_id)); break;
		case SKI_REQUIRES_AMMOAMOUNT: script_pushint(st, skill_get_ammo_qty(skill_id,skill_lv)); break;
		case SKI_REQUIRES_STATE: script_pushint(st, skill_get_state(skill_id)); break;
		case SKI_REQUIRES_SPHERECOST: script_pushint(st, skill_get_spiritball(skill_id,skill_lv)); break;

		case SKI_REQUIRES_STATUS:
			for (const auto& sc : skill->require.status) {
				script_both_setreg(st, "skill_requires_status", (int64)sc, true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		case SKI_DAMAGEFLAGS:
			for (i = 0; i < NK_MAX; i++) {
				if (!skill->nk[i]) continue;
				script_both_setreg(st, "skill_damage_flags", skill->nk[i], true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		case SKI_FLAGS:
			for (i = 0; i < INF2_MAX; i++) {
				if (!skill->inf2[i]) continue;
				script_both_setreg(st, "skill_flags", skill->inf2[i], true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		case SKI_UNIT_FLAG:
			for (i = 0; i < UF_MAX; i++) {
				if (!skill->unit_flag[i]) continue;
				script_both_setreg(st, "skill_unit_flag", skill->unit_flag[i], true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		case SKI_REQUIRES_EQUIPMENT:
			for (const auto& item : skill->require.eqItem) {
				script_both_setreg(st, "skill_requires_equipment", (int64)item, true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		case SKI_REQUIRES_ITEMCOST:
			for (i = 0; i < MAX_SKILL_ITEM_REQUIRE; i++) {
				if (!skill->require.itemid[i]) continue;
				script_both_setreg(st, "skill_requires_itemid", skill->require.itemid[i], true, j, char_id);
				script_both_setreg(st, "skill_requires_amount", skill->require.amount[i], true, j, char_id);
				j++;
			}
			script_pushint(st, j);
			break;
		default:
			script_pushint(st, -1);
			return SCRIPT_CMD_FAILURE;
	}
	
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetSkillInfo

#ifdef Pandas_ScriptCommand_Sleep3
/* ===========================================================
 * 指令: sleep3
 * 描述: 休眠一段时间再执行后续脚本, 与 sleep2 类似但忽略报错
 * 用法: sleep3 <休眠毫秒数>;
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: 人鱼姬的思念
 * -----------------------------------------------------------*/
BUILDIN_FUNC(sleep3) {
	// First call(by function call)
	if (st->sleep.tick == 0) {
		int ticks;

		ticks = script_getnum(st, 2);

		if (ticks <= 0) {
			ShowError("buildin_sleep3: negative or zero amount('%d') of milli seconds is not supported\n", ticks);
			return SCRIPT_CMD_FAILURE;
		}

		// sleep for the target amount of time
		st->state = RERUNLINE;
		st->sleep.tick = ticks;
		// Second call(by timer after sleeping time is over)
	}
	else {
		// The unit is still attached - continue the script
		st->state = RUN;
		st->sleep.tick = 0;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Sleep3

#ifdef Pandas_ScriptCommand_GetQuestTime
/* ===========================================================
 * 指令: getquesttime
 * 描述: 查询角色指定任务的时间信息
 * 用法: getquesttime <任务编号>{,<想查询的时间类型>{,<角色编号>}};
 * 返回: 成功返回时间戳, 失败返回 -1
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getquesttime) {
	int quest_id = script_getnum(st, 2);
	int query_time_type = (script_hasdata(st, 3) && script_isint(st, 3)) ? script_getnum(st, 3) : 0;
	struct map_session_data* sd = nullptr;

	if (!script_charid2sd(4, sd)) {
		return SCRIPT_CMD_FAILURE;
	}

	std::shared_ptr<s_quest_db> qi = quest_search(quest_id);
	if (!qi) {
		ShowError("buildin_getquesttime: Quest %d not found in DB.\n", quest_id);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	int i = 0;
	ARR_FIND(0, sd->num_quests, i, sd->quest_log[i].quest_id == quest_id);
	if (i == sd->num_quests) {
		ShowError("buildin_getquesttime: Character %d doesn't have quest %d.\n", sd->status.char_id, quest_id);
		script_pushint(st, -1);
		return SCRIPT_CMD_FAILURE;
	}

	switch (query_time_type)
	{
	case 0:
		script_pushint(st, sd->quest_log[i].time);
		break;
	case 1:
		script_pushint(st, sd->quest_log[i].time - qi->time);
		break;
	case 2:
		script_pushint(st, u64max(sd->quest_log[i].time - time(nullptr), 0));
		break;
	default:
		ShowError("buildin_getquesttime: Invaild type for quest time.\n");
		script_pushint(st, -1);
		break;
	}

	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetQuestTime

#ifdef Pandas_ScriptCommand_UnitSpecialEffect
/* ===========================================================
 * 指令: unitspecialeffect
 * 描述: 使指定游戏单位可以显示某个特效, 并支持控制特效可见范围
 * 用法: unitspecialeffect <游戏单位编号>,<特效编号>{,<谁能看见特效>{,<能看见特效的账号编号>}};
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: 人鱼姬的思念
 * -----------------------------------------------------------*/
BUILDIN_FUNC(unitspecialeffect) {
	struct block_list* bl = nullptr;
	int type = script_getnum(st, 3);
	enum send_target target = AREA;

	bl = map_id2bl(script_getnum(st, 2));
	if (!bl) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (script_hasdata(st, 4)) {
		target = (send_target)script_getnum(st, 4);
	}

	if (type <= EF_NONE || type >= EF_MAX) {
		ShowError("buildin_unitspecialeffect: unsupported effect id %d\n", type);
		return SCRIPT_CMD_FAILURE;
	}

	if (target != SELF) {
		clif_specialeffect(bl, type, target);
		return SCRIPT_CMD_SUCCESS;
	}

	struct map_session_data* sd = nullptr;
	if (!script_mapid2sd(5, sd)) {
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd && sd->bl.type == BL_PC) {
		clif_specialeffect_single(bl, type, sd->fd);
	}
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_UnitSpecialEffect

#ifdef Pandas_ScriptCommand_Next_Dropitem_Special
/* ===========================================================
 * 指令: next_dropitem_special
 * 描述: 对下一个掉落到地面上的物品进行特殊设置
 * 用法: next_dropitem_special <道具绑定类型>,<租赁时长>,<掉落光柱颜色>;
 * 返回: 该指令无论成功与否, 都不会有返回值
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(next_dropitem_special) {
	next_dropitem_special.bound = cap_value(script_getnum(st, 2), BOUND_NONE, BOUND_MAX - 1);
	next_dropitem_special.rent_duration = cap_value(script_getnum(st, 3), 0, INT32_MAX);
	next_dropitem_special.drop_effect = cap_value(script_getnum(st, 4), -1, DROPEFFECT_MAX - 1);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_Next_Dropitem_Special

#ifdef Pandas_ScriptCommand_GetRateIdx
/* ===========================================================
 * 指令: getrateidx
 * 描述: 随机获取一个数值型数组的索引序号, 数组中每个元素的值为权重值
 * 用法: getrateidx <数值型数组变量>;
 * 用法: getrateidx <数值1>{, <数值2>{, ...<数值n>}};
 * 返回: 返回随机命中的索引序号
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getrateidx) {
	uint64 total_weight = 0;
	struct script_data* data = script_getdata(st, 2);
	std::vector<uint64> weights;

	// 如果不是一个数组, 那么将参数逐个累加
	if (!data_isreference(data)) {
		for (int i = 0; i < script_lastdata(st) - 1; i++) {
			uint64 weight = script_getnum(st, i + 2);
			weights.push_back(weight);
			total_weight += weight;
		}
	}
	// 否则说明传递的是一个数组, 从数组中读取内容
	else {
		struct map_session_data* sd = nullptr;
		int varid = reference_getid(data);
		const char* varname = reference_getname(data);

		// 若不是服务器变量, 那么必须要关联到玩家
		if (not_server_variable(*varname)) {
			if (!script_rid2sd(sd)) {
				ShowError("buildin_getrateidx: '%s' is not server variable, please attach to a player.\n", varname);
				script_abort(st);
				return SCRIPT_CMD_FAILURE;
			}
		}

		// 检查是否为数值类型的数组变量, 不是则报错并终止
		if (is_string_variable(varname)) {
			ShowError("buildin_getrateidx: '%s' is not an int array.\n", varname);
			script_abort(st);
			return SCRIPT_CMD_FAILURE;
		}

		// 获取数组的元素个数, 并判断是否在有效范围内
		uint32 array_size = script_array_highest_key(st, sd, varname, reference_getref(data));
		if (array_size == 0 || array_size > SCRIPT_MAX_ARRAYSIZE) {
			ShowError("buildin_getrateidx: The size of '%s' is not in valid range.\n", varname);
			script_abort(st);
			return SCRIPT_CMD_FAILURE;
		}

		for (uint32 i = 0; i < array_size; i++) {
			uint64 weight = get_val2_num(st, reference_uid(varid, i), reference_getref(data));
			weights.push_back(weight);
			total_weight += weight;
		}
	}

	if (!weights.size() || !total_weight) {
		ShowError("buildin_getrateidx: Could not find any valid data for the calculation.\n");
		script_abort(st);
		return SCRIPT_CMD_FAILURE;
	}

	uint64 randNum = rnd() % total_weight;
	uint64 start = 0, end = 0;
	size_t idx = 0;

	for (idx = 0; idx < weights.size(); idx++) {
		if (weights[idx] == 0) {
			continue;
		}

		end = start + weights[idx];
		if (start <= randNum && randNum < end) {
			break;
		}
		start = end;
	}
	
	if (idx == weights.size()) {
		// Shouldn't happen
		ShowError("buildin_getrateidx: Some accidents happened.\n");
		script_abort(st);
		return SCRIPT_CMD_FAILURE;
	}

	script_pushint(st, idx);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetRateIdx

#ifdef Pandas_ScriptCommand_GetBossInfo
/* ===========================================================
 * 指令: getbossinfo
 * 描述: 查询 BOSS 魔物重生时间及其坟墓等信息
 * 用法: getbossinfo {<"地图名称">{,<魔物编号>{,<角色编号>}}};
 * 返回: 返回查询到的记录数
 * 作者: Sola丶小克
 * -----------------------------------------------------------*/
BUILDIN_FUNC(getbossinfo) {
	const char* mapname = nullptr;
	struct map_session_data* sd = nullptr;
	int mobid = 0, char_id = 0, mapid = -1;

	if (script_hasdata(st, 2)) {
		mapname = script_getstr(st, 2);

		if (strcmpi(mapname, "all") == 0) {
			// 查询全部地图
			mapid = -1;
		}
		else if (strcmpi(mapname, "this") == 0) {
			// 查询脚本所关联玩家的当前地图
			if (!script_charid2sd(4, sd)) {
				// todo: warning
				script_pushint(st, 0);
				return SCRIPT_CMD_FAILURE;
			}
			mapid = sd->bl.m;
		}
		else {
			// 按指定的地图名称进行查询
			mapid = map_mapname2mapid(mapname);

			if (mapid < 0) {
				ShowError("buildin_getbossinfo: Unknown map name %s.\n", mapname);
				script_pushint(st, 0);
				return SCRIPT_CMD_FAILURE;
			}
		}
	}

	if (script_hasdata(st, 3)) {
		mobid = script_getnum(st, 3);
	}

	if (script_hasdata(st, 4)) {
		char_id = script_getnum(st, 4);
	}

	script_both_setreg(st, "boss_count", 0, false, -1, char_id);
	
	DBIterator* iter = nullptr;
	struct mob_data* md = nullptr;
	int count = 0;

	iter = db_iterator(get_bossid_db());
	for (md = (struct mob_data*)dbi_first(iter); dbi_exists(iter); md = (struct mob_data*)dbi_next(iter)) {
		if (mapid != -1 && mapid != md->bl.m)
			continue;
		if (mobid != 0 && mobid != md->mob_id)
			continue;
		
		struct npc_data* tomb_nd = nullptr;
		if (md->tomb_nid) {
			tomb_nd = map_id2nd(md->tomb_nid);
		}

		script_both_setreg(st, "boss_mapid", md->bl.m, true, count, char_id);
		script_both_setregstr(st, "boss_mapname$", (md->bl.m >= 0 ? map[md->bl.m].name : ""), true, count, char_id);
		script_both_setreg(st, "boss_x", md->bl.x, true, count, char_id);
		script_both_setreg(st, "boss_y", md->bl.y, true, count, char_id);
		
		script_both_setreg(st, "boss_gid", md->bl.id, true, count, char_id);
		script_both_setreg(st, "boss_spawn", (md->spawn_timer != INVALID_TIMER ? DIFF_TICK(get_timer(md->spawn_timer)->tick, gettick()) : 0), true, count, char_id);
		script_both_setreg(st, "boss_classid", md->mob_id, true, count, char_id);

		script_both_setreg(st, "boss_tomb_mapid", (tomb_nd ? tomb_nd->bl.m : -1), true, count, char_id);
		script_both_setregstr(st, "boss_tomb_mapname$", (tomb_nd && tomb_nd->bl.m >= 0 ? map[tomb_nd->bl.m].name : ""), true, count, char_id);
		script_both_setreg(st, "boss_tomb_x", (tomb_nd ? tomb_nd->bl.x : 0), true, count, char_id);
		script_both_setreg(st, "boss_tomb_y", (tomb_nd ? tomb_nd->bl.y : 0), true, count, char_id);

		script_both_setreg(st, "boss_tomb_gid", (tomb_nd ? tomb_nd->bl.id : 0), true, count, char_id);
		script_both_setreg(st, "boss_tomb_createtime", (tomb_nd ? tomb_nd->u.tomb.kill_time : 0), true, count, char_id);

		script_both_setreg(st, "boss_tomb_respawnsecs", -1, true, count, char_id);
		t_tick respawntime = -1;
		if (tomb_nd && tomb_nd->u.tomb.md->spawn) {
			respawntime = gett_tickimer(tomb_nd->u.tomb.md->spawn_timer);
			if (respawntime != -1) {
				respawntime = DIFF_TICK(respawntime, gettick());
				respawntime = respawntime / 1000;
				script_both_setreg(st, "boss_tomb_respawnsecs", respawntime, true, count, char_id);
				respawntime = respawntime + (int)time(NULL);
			}
		}
		script_both_setreg(st, "boss_tomb_respawntime", respawntime, true, count, char_id);

		script_both_setregstr(st, "boss_tomb_killer_name$", (tomb_nd ? tomb_nd->u.tomb.killer_name : ""), true, count, char_id);
#ifdef Pandas_FuncParams_Mob_MvpTomb_Create
		script_both_setreg(st, "boss_tomb_killer_gid", (tomb_nd ? tomb_nd->u.tomb.killer_gid : 0), true, count, char_id);
#endif // Pandas_FuncParams_Mob_MvpTomb_Create
		
		count++;
	}
	dbi_destroy(iter);

	script_both_setreg(st, "boss_count", count, false, -1, char_id);
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_GetBossInfo

#ifdef Pandas_ScriptCommand_WhoDropItem
/* ===========================================================
 * 指令: whodropitem
 * 描述: 查询指定道具会从哪些魔物身上掉落以及掉落的机率信息
 * 用法: whodropitem <物品编号/"物品名称">{,<返回的最大记录数>{,<角色编号>}};
 * 返回: 返回查询到的记录数量
 * 作者: Sola丶小克 (感谢 "人鱼姬的思念")
 * -----------------------------------------------------------*/
BUILDIN_FUNC(whodropitem) {
	t_itemid nameid = 0;
	int max_result = (script_hasdata(st, 3) && script_isint(st, 3)) ? script_getnum(st, 3) : 0;
	int char_id = (script_hasdata(st, 4) && script_isint(st, 4)) ? script_getnum(st, 4) : 0;
	std::shared_ptr<item_data> id;
	struct map_session_data *sd = nullptr;

	// 清空比较关键的返回值
	script_both_setreg(st, "whodropitem_count", 0, false, 0, char_id);

	if (script_isstring(st, 2)) {// "<item name>"
		const char* name = script_getstr(st, 2);

		id = item_db.searchname(name);

		if (id == nullptr) {
			ShowError("buildin_whodropitem: Nonexistant item %s requested.\n", name);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}
		nameid = id->nameid;
	}
	else {// <item id>
		nameid = script_getnum(st, 2);

		id = item_db.find(nameid);

		if (id == nullptr) {
			ShowError("buildin_whodropitem: Nonexistant item %u requested.\n", nameid);
			script_pushint(st, 0);
			return SCRIPT_CMD_FAILURE;
		}
	}
	
	// 最大返回的结果数量若未被指定, 则默认值为 MAX_SEARCH
	max_result = (max_result ? max_result : MAX_SEARCH);

	// 最大返回的结果数量应该在 1-500 之间 (这里的 500 是拍脑袋的, 通常应该够用了)
	max_result = cap_value(max_result, 1, 500);
	
	// 若连一个掉落此物品的魔物都没有, 那么直接返回 0
	if (id->mob[0].chance == 0) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	// 允许在未关联玩家的情况下执行该指令, 没关联玩家也不报错
	// 但是若明确指定了某个角色, 但目标角色不存在的话, 那么需要报错
	if (char_id && !(sd = map_charid2sd(char_id))) {
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}
	else if (st->rid != 0) {
		sd = map_id2sd(st->rid);
	}

	// 用来临时保存结果的缓存区域 <魔物编号, 掉落机率>
	std::map<uint32, int> result_cache;

	// 构建我们需要的结果, 只查询单个道具其实速度非常快, 不用担心性能过渡开销
	for (auto const& pair : mob_db) {
		if (mob_is_clone(pair.first)) {
			continue;
		}

		for (int d = 0; d < MAX_MOB_DROP_TOTAL; d++) {
			if (pair.second->dropitem[d].nameid != nameid)
				continue;

			if (rathena::util::map_exists(result_cache, pair.first)) {
				if (result_cache[pair.first] < pair.second->dropitem[d].rate)
					result_cache[pair.first] = pair.second->dropitem[d].rate;
			}
			else {
				result_cache[pair.first] = pair.second->dropitem[d].rate;
			}
		}
	}

	// 根据一些掉率调整设置来处理道具的掉率, 这样最后排序的时候才能考虑进去
	for (auto const& it : result_cache) {
		int dropchance = it.second;
		
#ifdef RENEWAL_DROP
		if (sd && battle_config.atcommand_mobinfo_type) {
			std::shared_ptr<s_mob_db> mob = mob_db.find(it.first);
			// 备注: 由于 pc_level_penalty_mod 内部已经判断了 mob 智能指针的有效性, 因此在此处不重复判断
			dropchance = dropchance * pc_level_penalty_mod(sd, PENALTY_DROP, mob) / 100;
			if (dropchance <= 0 && !battle_config.drop_rate0item)
				dropchance = 1;
		}
#endif
		if (sd && pc_isvip(sd)) // Display item rate increase for VIP
			dropchance += (dropchance * battle_config.vip_drop_increase) / 100;

#ifdef Pandas_Database_MobItem_FixedRatio
		// 若严格固定掉率, 那么无视上面的等级惩罚、VIP掉率加成等计算
		if (mobdrop_strict_droprate(nameid, it.first))
			dropchance = it.second;
#endif // Pandas_Database_MobItem_FixedRatio

		result_cache[it.first] = dropchance;
	}

	// 对 result_cache 的内容进行降序排序
	std::vector<std::pair<uint32, int>> result_sortd;
	for (auto const& pair : result_cache) {
		result_sortd.push_back(std::make_pair(pair.first, pair.second));
	}

	// 先按魔物编号从小到大排序
	std::sort(result_sortd.begin(), result_sortd.end(),
		[](const std::pair<uint32, int>& a, const std::pair<uint32, int>& b) {
			return a.first < b.first;
		}
	);

	// 再按掉率从大到小排序
	std::sort(result_sortd.begin(), result_sortd.end(),
		[](const std::pair<uint32, int>& a, const std::pair<uint32, int>& b) {
			return a.second > b.second;
		}
	);

	// 若排序后的 result_sortd 没有任何结果, 那么直接返回即可
	if (result_sortd.empty()) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	// 准备就绪, 开始填充数组内容并返回结果
	int current_index = 0;

	for (const auto& it : result_sortd) {
		int dropchance = it.second;

		std::shared_ptr<s_mob_db> mob = mob_db.find(it.first);
		if (!mob) continue;

		script_both_setreg(st, "whodropitem_mob_id", mob->id, true, current_index, char_id);
		script_both_setregstr(st, "whodropitem_mob_jname$", mob->jname.c_str(), true, current_index, char_id);
		script_both_setreg(st, "whodropitem_chance", dropchance, true, current_index, char_id);
		
		current_index++;

		if (current_index >= max_result)
			break;
	}

	script_both_setreg(st, "whodropitem_count", current_index, false, 0, char_id);
	script_pushint(st, current_index);
	return SCRIPT_CMD_SUCCESS;
}
#endif // Pandas_ScriptCommand_WhoDropItem

// PYHELP - SCRIPTCMD - INSERT POINT - <Section 2>

/// script command definitions
/// for an explanation on args, see add_buildin_func
struct script_function buildin_func[] = {
	// NPC interaction
	BUILDIN_DEF(mes,"s*"),
	BUILDIN_DEF(next,""),
	BUILDIN_DEF(clear,""),
	BUILDIN_DEF(close,""),
	BUILDIN_DEF(close2,""),
	BUILDIN_DEF(menu,"sl*"),
	BUILDIN_DEF(select,"s*"), //for future jA script compatibility
	BUILDIN_DEF(prompt,"s*"),
	//
	BUILDIN_DEF(goto,"l"),
	BUILDIN_DEF(callsub,"l*"),
	BUILDIN_DEF(callfunc,"s*"),
	BUILDIN_DEF(return,"?"),
	BUILDIN_DEF(getarg,"i?"),
	BUILDIN_DEF(jobchange,"i??"),
	BUILDIN_DEF(jobname,"i"),
	BUILDIN_DEF(input,"r??"),
	BUILDIN_DEF(warp,"sii?"),
	BUILDIN_DEF2(warp, "warpchar", "sii?"),
	BUILDIN_DEF(areawarp,"siiiisii??"),
	BUILDIN_DEF(warpparty,"siii???"), // [Fredzilla] [Paradox924X]
	BUILDIN_DEF(warpguild,"siii"), // [Fredzilla]
	BUILDIN_DEF(setlook,"ii?"),
	BUILDIN_DEF(changelook,"ii?"), // Simulates but don't Store it
	BUILDIN_DEF2(setr,"set","rv?"),
	BUILDIN_DEF(setr,"rv??"), // Not meant to be used directly, required for var++/var--
	BUILDIN_DEF(setarray,"rv*"),
	BUILDIN_DEF(cleararray,"rvi"),
	BUILDIN_DEF(copyarray,"rri"),
	BUILDIN_DEF(getarraysize,"r"),
	BUILDIN_DEF(deletearray,"r?"),
	BUILDIN_DEF(getelementofarray,"ri"),
	BUILDIN_DEF(inarray,"rv"),
	BUILDIN_DEF(countinarray,"rr"),
	BUILDIN_DEF(getitem,"vi?"),
	BUILDIN_DEF(rentitem,"vi?"),
	BUILDIN_DEF(rentitem2,"viiiiiiii?"),
	BUILDIN_DEF(getitem2,"viiiiiiii?"),
	BUILDIN_DEF(getnameditem,"vv"),
	BUILDIN_DEF2(grouprandomitem,"groupranditem","i?"),
	BUILDIN_DEF(makeitem,"visii?"),
	BUILDIN_DEF(makeitem2,"visiiiiiiiii?"),
	BUILDIN_DEF(delitem,"vi?"),
	BUILDIN_DEF2(delitem,"storagedelitem","vi?"),
	BUILDIN_DEF2(delitem,"guildstoragedelitem","vi?"),
	BUILDIN_DEF2(delitem,"cartdelitem","vi?"),
	BUILDIN_DEF(delitem2,"viiiiiiii?"),
	BUILDIN_DEF2(delitem2,"storagedelitem2","viiiiiiii?"),
	BUILDIN_DEF2(delitem2,"guildstoragedelitem2","viiiiiiii?"),
	BUILDIN_DEF2(delitem2,"cartdelitem2","viiiiiiii?"),
	BUILDIN_DEF(delitemidx,"i??"),
	BUILDIN_DEF2(enableitemuse,"enable_items",""),
	BUILDIN_DEF2(disableitemuse,"disable_items",""),
	BUILDIN_DEF(cutin,"si"),
	BUILDIN_DEF(viewpoint,"iiiii?"),
	BUILDIN_DEF(viewpointmap, "siiiii"),
	BUILDIN_DEF(heal,"ii?"),
	BUILDIN_DEF(healap,"i?"),
	BUILDIN_DEF(itemheal,"ii?"),
	BUILDIN_DEF(percentheal,"ii?"),
	BUILDIN_DEF(rand,"i?"),
	BUILDIN_DEF(countitem,"v?"),
	BUILDIN_DEF(storagecountitem,"v?"),
	BUILDIN_DEF(guildstoragecountitem,"v?"),
	BUILDIN_DEF(cartcountitem,"v?"),
	BUILDIN_DEF2(countitem,"countitem2","viiiiiii?"),
	BUILDIN_DEF2(storagecountitem,"storagecountitem2","viiiiiii?"),
	BUILDIN_DEF2(guildstoragecountitem,"guildstoragecountitem2","viiiiiii?"),
	BUILDIN_DEF2(cartcountitem,"cartcountitem2","viiiiiii?"),
	BUILDIN_DEF(checkweight,"vi*"),
	BUILDIN_DEF(checkweight2,"rr"),
	BUILDIN_DEF(readparam,"i?"),
	BUILDIN_DEF(getcharid,"i?"),
	BUILDIN_DEF(getnpcid,"i?"),
	BUILDIN_DEF(getpartyname,"i"),
	BUILDIN_DEF(getpartymember,"i??"),
	BUILDIN_DEF(getpartyleader,"i?"),
	BUILDIN_DEF(getguildname,"i"),
	BUILDIN_DEF(getguildmaster,"i"),
	BUILDIN_DEF(getguildmasterid,"i"),
	BUILDIN_DEF(strcharinfo,"i?"),
	BUILDIN_DEF(strnpcinfo,"i"),
	BUILDIN_DEF(getequipid,"??"),
	BUILDIN_DEF(getequipuniqueid,"i?"),
	BUILDIN_DEF(getequipname,"i?"),
	BUILDIN_DEF(getbrokenid,"i?"), // [Valaris]
	BUILDIN_DEF(repair,"i?"), // [Valaris]
	BUILDIN_DEF(repairall,"?"),
	BUILDIN_DEF(getequipisequiped,"i?"),
	BUILDIN_DEF(getequipisenableref,"i?"),
	BUILDIN_DEF(getequiprefinerycnt,"i?"),
	BUILDIN_DEF(getequipweaponlv,"??"),
	BUILDIN_DEF(getequiparmorlv, "??"),
	BUILDIN_DEF(getequippercentrefinery,"i?"),
	BUILDIN_DEF(successrefitem,"i??"),
	BUILDIN_DEF(failedrefitem,"i?"),
	BUILDIN_DEF(downrefitem,"i??"),
	BUILDIN_DEF(statusup,"i?"),
	BUILDIN_DEF(statusup2,"ii?"),
	BUILDIN_DEF(traitstatusup,"i?"),
	BUILDIN_DEF(traitstatusup2,"ii?"),
	BUILDIN_DEF(bonus,"i?"),
	BUILDIN_DEF2(bonus,"bonus2","ivi"),
	BUILDIN_DEF2(bonus,"bonus3","ivii"),
	BUILDIN_DEF2(bonus,"bonus4","ivvii"),
	BUILDIN_DEF2(bonus,"bonus5","ivviii"),
	BUILDIN_DEF(autobonus,"sii??"),
	BUILDIN_DEF(autobonus2,"sii??"),
	BUILDIN_DEF(autobonus3,"siiv?"),
	BUILDIN_DEF(skill,"vi?"),
	BUILDIN_DEF2(skill,"addtoskill","vi?"), // [Valaris]
	BUILDIN_DEF(guildskill,"vi"),
	BUILDIN_DEF(getskilllv,"v"),
	BUILDIN_DEF(getgdskilllv,"iv"),
	BUILDIN_DEF(basicskillcheck,""),
	BUILDIN_DEF(getgmlevel,"?"),
	BUILDIN_DEF(getgroupid,"?"),
	BUILDIN_DEF(end,""),
	BUILDIN_DEF(checkoption,"i?"),
	BUILDIN_DEF(setoption,"i??"),
	BUILDIN_DEF(setcart,"??"),
	BUILDIN_DEF(checkcart,"?"),
	BUILDIN_DEF(setfalcon,"??"),
	BUILDIN_DEF(checkfalcon,"?"),
	BUILDIN_DEF(setriding,"??"),
	BUILDIN_DEF(checkriding,"?"),
	BUILDIN_DEF(checkwug,"?"),
	BUILDIN_DEF(checkmadogear,"?"),
	BUILDIN_DEF(setmadogear,"???"),
	BUILDIN_DEF2(savepoint,"save","sii???"),
	BUILDIN_DEF(savepoint,"sii???"),
	BUILDIN_DEF(gettimetick,"i"),
	BUILDIN_DEF(gettime,"i"),
	BUILDIN_DEF(gettimestr,"si?"),
	BUILDIN_DEF(openstorage,""),
	BUILDIN_DEF(guildopenstorage,""),
	BUILDIN_DEF(guildopenstorage_log,"?"),
	BUILDIN_DEF(guild_has_permission,"i?"),
	BUILDIN_DEF(itemskill,"vi?"),
	BUILDIN_DEF(produce,"i"),
	BUILDIN_DEF(cooking,"i"),
	BUILDIN_DEF(monster,"siisii???"),
	BUILDIN_DEF(getmobdrops,"i"),
	BUILDIN_DEF(areamonster,"siiiisii???"),
	BUILDIN_DEF(killmonster,"ss?"),
	BUILDIN_DEF(killmonsterall,"s?"),
	BUILDIN_DEF(clone,"siisi????"),
	BUILDIN_DEF(doevent,"s"),
	BUILDIN_DEF(donpcevent,"s"),
	BUILDIN_DEF(cmdothernpc,"ss"),
	BUILDIN_DEF(addtimer,"is"),
	BUILDIN_DEF(deltimer,"s"),
	BUILDIN_DEF(addtimercount,"is"),
	BUILDIN_DEF(initnpctimer,"??"),
	BUILDIN_DEF(stopnpctimer,"??"),
	BUILDIN_DEF(startnpctimer,"??"),
	BUILDIN_DEF(setnpctimer,"i?"),
	BUILDIN_DEF(getnpctimer,"i?"),
	BUILDIN_DEF(attachnpctimer,"?"), // attached the player id to the npc timer [Celest]
	BUILDIN_DEF(detachnpctimer,"?"), // detached the player id from the npc timer [Celest]
	BUILDIN_DEF(playerattached,""), // returns id of the current attached player. [Skotlex]
	BUILDIN_DEF(announce,"si??????"),
	BUILDIN_DEF(mapannounce,"ssi?????"),
	BUILDIN_DEF(areaannounce,"siiiisi?????"),
	BUILDIN_DEF(getusers,"i"),
	BUILDIN_DEF(getmapguildusers,"si"),
	BUILDIN_DEF(getmapusers,"s"),
	BUILDIN_DEF(getareausers,"siiii"),
	BUILDIN_DEF(getunits, "i?"),
	BUILDIN_DEF2(getunits, "getmapunits", "is?"),
	BUILDIN_DEF2(getunits, "getareaunits", "isiiii?"),
	BUILDIN_DEF(getareadropitem,"siiiiv"),
	BUILDIN_DEF(enablenpc,"?"),
	BUILDIN_DEF2(enablenpc, "disablenpc", "?"),
	BUILDIN_DEF2(enablenpc, "hideoffnpc", "?"),
	BUILDIN_DEF2(enablenpc, "hideonnpc", "?"),
	BUILDIN_DEF2(enablenpc, "cloakoffnpc", "??"),
	BUILDIN_DEF2(enablenpc, "cloakonnpc", "??"),
	BUILDIN_DEF2(enablenpc, "cloakoffnpcself", "?"),
	BUILDIN_DEF2(enablenpc, "cloakonnpcself", "?"),
	BUILDIN_DEF(sc_start,"iii???"),
	BUILDIN_DEF2(sc_start,"sc_start2","iiii???"),
	BUILDIN_DEF2(sc_start,"sc_start4","iiiiii???"),
	BUILDIN_DEF(sc_end,"i?"),
	BUILDIN_DEF(sc_end_class,"??"),
	BUILDIN_DEF(getstatus, "i??"),
	BUILDIN_DEF(getscrate,"ii?"),
	BUILDIN_DEF(debugmes,"s"),
	BUILDIN_DEF(errormes,"s"),
	BUILDIN_DEF2(catchpet,"pet","i"),
	BUILDIN_DEF2(birthpet,"bpet",""),
	BUILDIN_DEF(catchpet,"i"),
	BUILDIN_DEF(birthpet,""),
	BUILDIN_DEF(resetlvl,"i?"),
	BUILDIN_DEF(resetstatus,"?"),
	BUILDIN_DEF(resetskill,"?"),
	BUILDIN_DEF(resetfeel,"?"),
	BUILDIN_DEF(resethate,"?"),
	BUILDIN_DEF(skillpointcount,"?"),
	BUILDIN_DEF(changebase,"i?"),
	BUILDIN_DEF(changesex,"?"),
	BUILDIN_DEF(changecharsex,"?"),
	BUILDIN_DEF(waitingroom,"si?????"),
	BUILDIN_DEF(delwaitingroom,"?"),
	BUILDIN_DEF(waitingroomkick,"ss"),
	BUILDIN_DEF(getwaitingroomusers, "?"),
	BUILDIN_DEF2(waitingroomkickall,"kickwaitingroomall","?"),
	BUILDIN_DEF(enablewaitingroomevent,"?"),
	BUILDIN_DEF(disablewaitingroomevent,"?"),
	BUILDIN_DEF2(enablewaitingroomevent,"enablearena",""),		// Added by RoVeRT
	BUILDIN_DEF2(disablewaitingroomevent,"disablearena",""),	// Added by RoVeRT
	BUILDIN_DEF(getwaitingroomstate,"i?"),
	BUILDIN_DEF(warpwaitingpc,"sii?"),
	BUILDIN_DEF(attachrid,"i?"),
	BUILDIN_DEF(addrid,"i?????"),
	BUILDIN_DEF(detachrid,""),
	BUILDIN_DEF(isloggedin,"i?"),
	BUILDIN_DEF(setmapflagnosave,"ssii"),
	BUILDIN_DEF(getmapflag,"si?"),
	BUILDIN_DEF(setmapflag,"si??"),
	BUILDIN_DEF(removemapflag,"si?"),
	BUILDIN_DEF(pvpon,"s"),
	BUILDIN_DEF(pvpoff,"s"),
	BUILDIN_DEF(gvgon,"s"),
	BUILDIN_DEF(gvgoff,"s"),
	BUILDIN_DEF(emotion,"i?"),
	BUILDIN_DEF(maprespawnguildid,"sii"),
	BUILDIN_DEF(agitstart,""),	// <Agit>
	BUILDIN_DEF(agitend,""),
	BUILDIN_DEF(agitcheck,""),   // <Agitcheck>
	BUILDIN_DEF(flagemblem,"i"),	// Flag Emblem
	BUILDIN_DEF(getcastlename,"s"),
	BUILDIN_DEF(getcastledata,"si"),
	BUILDIN_DEF(setcastledata,"sii"),
	BUILDIN_DEF(requestguildinfo,"i?"),
	BUILDIN_DEF(getequipcardcnt,"i"),
	BUILDIN_DEF(successremovecards,"i"),
	BUILDIN_DEF(failedremovecards,"ii"),
	BUILDIN_DEF(marriage,"s"),
	BUILDIN_DEF2(wedding_effect,"wedding",""),
	BUILDIN_DEF(divorce,"?"),
	BUILDIN_DEF(ispartneron,"?"),
	BUILDIN_DEF(getpartnerid,"?"),
	BUILDIN_DEF(getchildid,"?"),
	BUILDIN_DEF(getmotherid,"?"),
	BUILDIN_DEF(getfatherid,"?"),
	BUILDIN_DEF(warppartner,"sii"),
	BUILDIN_DEF(getitemname,"v"),
	BUILDIN_DEF(getitemslots,"i"),
	BUILDIN_DEF(makepet,"i"),
	BUILDIN_DEF(getexp,"ii?"),
#ifndef Pandas_ScriptCommand_GetInventoryList
	BUILDIN_DEF(getinventorylist,"?"),
#else
	BUILDIN_DEF(getinventorylist,"??"),
	BUILDIN_DEF2(getinventorylist,"getcartlist","??"),
	BUILDIN_DEF2(getinventorylist,"getguildstoragelist", "??"),
	BUILDIN_DEF2(getinventorylist,"getstoragelist", "???"),
#endif // Pandas_ScriptCommand_GetInventoryList
	BUILDIN_DEF(getskilllist,"?"),
	BUILDIN_DEF(clearitem,"?"),
	BUILDIN_DEF(classchange,"i??"),
	BUILDIN_DEF(misceffect,"i"),
	BUILDIN_DEF(playBGM,"s"),
	BUILDIN_DEF(playBGMall,"s?????"),
	BUILDIN_DEF(soundeffect,"si"),
	BUILDIN_DEF(soundeffectall,"si?????"),	// SoundEffectAll [Codemaster]
	BUILDIN_DEF(strmobinfo,"ii"),	// display mob data [Valaris]
	BUILDIN_DEF(guardian,"siisi??"),	// summon guardians
	BUILDIN_DEF(guardianinfo,"sii"),	// display guardian data [Valaris]
	BUILDIN_DEF(petskillbonus,"iiii"), // [Valaris]
	BUILDIN_DEF(petrecovery,"ii"), // [Valaris]
	BUILDIN_DEF(petloot,"i"), // [Valaris]
	BUILDIN_DEF(petskillattack,"viii"), // [Skotlex]
	BUILDIN_DEF(petskillattack2,"viiii"), // [Valaris]
	BUILDIN_DEF(petskillsupport,"viiii"), // [Skotlex]
	BUILDIN_DEF(skilleffect,"vi"), // skill effect [Celest]
	BUILDIN_DEF(npcskilleffect,"viii"), // npc skill effect [Valaris]
	BUILDIN_DEF(specialeffect,"i??"), // npc skill effect [Valaris]
	BUILDIN_DEF(specialeffect2,"i??"), // skill effect on players[Valaris]
	BUILDIN_DEF(removespecialeffect,"i??"),
	BUILDIN_DEF2(removespecialeffect,"removespecialeffect2","i??"),
	BUILDIN_DEF(nude,"?"), // nude command [Valaris]
	BUILDIN_DEF(mapwarp,"ssii??"),		// Added by RoVeRT
	BUILDIN_DEF(atcommand,"s"), // [MouseJstr]
	BUILDIN_DEF2(atcommand,"charcommand","s"), // [MouseJstr]
	BUILDIN_DEF(movenpc,"sii?"), // [MouseJstr]
	BUILDIN_DEF(message,"ss"), // [MouseJstr]
	BUILDIN_DEF(npctalk,"s???"), // [Valaris]
	BUILDIN_DEF(chatmes,"s?"), // [Jey]
	BUILDIN_DEF(mobcount,"ss"),
	BUILDIN_DEF(getlook,"i?"),
	BUILDIN_DEF(getsavepoint,"i?"),
	BUILDIN_DEF(npcspeed,"i"), // [Valaris]
	BUILDIN_DEF(npcwalkto,"ii"), // [Valaris]
	BUILDIN_DEF(npcstop,""), // [Valaris]
	BUILDIN_DEF(getmapxy,"rrr??"),	//by Lorky [Lupus]
	BUILDIN_DEF(mapid2name,"i"),
	BUILDIN_DEF(checkoption1,"i?"),
	BUILDIN_DEF(checkoption2,"i?"),
	BUILDIN_DEF(guildgetexp,"i"),
	BUILDIN_DEF(guildchangegm,"is"),
	BUILDIN_DEF(logmes,"s"), //this command actls as MES but rints info into LOG file either SQL/TXT [Lupus]
	BUILDIN_DEF(summon,"si??"), // summons a slave monster [Celest]
	BUILDIN_DEF(isnight,""), // check whether it is night time [Celest]
	BUILDIN_DEF(isday,""), // check whether it is day time [Celest]
	BUILDIN_DEF(isequipped,"i*"), // check whether another item/card has been equipped [Celest]
	BUILDIN_DEF(isequippedcnt,"i*"), // check how many items/cards are being equipped [Celest]
	BUILDIN_DEF(cardscnt,"i*"), // check how many items/cards are being equipped in the same arm [Lupus]
	BUILDIN_DEF(getrefine,""), // returns the refined number of the current item, or an item with index specified [celest]
	BUILDIN_DEF(night,""), // sets the server to night time
	BUILDIN_DEF(day,""), // sets the server to day time
#ifdef PCRE_SUPPORT
	BUILDIN_DEF(defpattern,"iss"), // Define pattern to listen for [MouseJstr]
	BUILDIN_DEF(activatepset,"i"), // Activate a pattern set [MouseJstr]
	BUILDIN_DEF(deactivatepset,"i"), // Deactive a pattern set [MouseJstr]
	BUILDIN_DEF(deletepset,"i"), // Delete a pattern set [MouseJstr]
#endif
	BUILDIN_DEF(preg_match,"ss?"),
	BUILDIN_DEF(dispbottom,"s??"), //added from jA [Lupus]
	BUILDIN_DEF(recovery,"i???"),
	BUILDIN_DEF(getpetinfo,"i?"),
	BUILDIN_DEF(gethominfo,"i?"),
	BUILDIN_DEF(addhomintimacy,"i?"),
	BUILDIN_DEF(getmercinfo,"i?"),
	BUILDIN_DEF(checkequipedcard,"i"),
	BUILDIN_DEF(jump_zero,"il"), //for future jA script compatibility
	BUILDIN_DEF(globalmes,"s?"), //end jA addition
	BUILDIN_DEF(unequip,"i?"), // unequip command [Spectre]
	BUILDIN_DEF(getstrlen,"s"), //strlen [Valaris]
	BUILDIN_DEF(charisalpha,"si"), //isalpha [Valaris]
	BUILDIN_DEF(charat,"si"),
	BUILDIN_DEF(setchar,"ssi"),
	BUILDIN_DEF(insertchar,"ssi"),
	BUILDIN_DEF(delchar,"si"),
	BUILDIN_DEF(strtoupper,"s"),
	BUILDIN_DEF(strtolower,"s"),
	BUILDIN_DEF(charisupper, "si"),
	BUILDIN_DEF(charislower, "si"),
	BUILDIN_DEF(substr,"sii"),
	BUILDIN_DEF(explode, "rss"),
	BUILDIN_DEF(implode, "r?"),
	BUILDIN_DEF(sprintf,"s*"),  // [Mirei]
	BUILDIN_DEF(sscanf,"ss*"),  // [Mirei]
	BUILDIN_DEF(strpos,"ss?"),
	BUILDIN_DEF(replacestr,"sss??"),
	BUILDIN_DEF(countstr,"ss?"),
	BUILDIN_DEF(setnpcdisplay,"sv??"),
	BUILDIN_DEF(compare,"ss"), // Lordalfa - To bring strstr to scripting Engine.
	BUILDIN_DEF(strcmp,"ss"),
#ifndef Pandas_ScriptParams_GetItemInfo
	BUILDIN_DEF(getiteminfo,"vi"), //[Lupus] returns Items Buy / sell Price, etc info
#else
	BUILDIN_DEF(getiteminfo, "vi?"), //[Lupus] returns Items Buy / sell Price, etc info
#endif // Pandas_ScriptParams_GetItemInfo
	BUILDIN_DEF(setiteminfo,"vii"), //[Lupus] set Items Buy / sell Price, etc info
	BUILDIN_DEF(getequipcardid,"ii"), //[Lupus] returns CARD ID or other info from CARD slot N of equipped item
	// [zBuffer] List of mathematics commands --->
	BUILDIN_DEF(sqrt,"i"),
	BUILDIN_DEF(pow,"ii"),
	BUILDIN_DEF(distance,"iiii"),
	BUILDIN_DEF2(minmax,"min", "*"),
	BUILDIN_DEF2(minmax,"minimum", "*"),
	BUILDIN_DEF2(minmax,"max", "*"),
	BUILDIN_DEF2(minmax,"maximum", "*"),
	// <--- [zBuffer] List of mathematics commands
	BUILDIN_DEF(md5,"s"),
	// [zBuffer] List of dynamic var commands --->
	BUILDIN_DEF(getd,"s"),
	BUILDIN_DEF(setd,"sv?"),
	BUILDIN_DEF(callshop,"s?"), // [Skotlex]
	BUILDIN_DEF(npcshopitem,"sii*"), // [Lance]
	BUILDIN_DEF(npcshopadditem,"sii*"),
	BUILDIN_DEF(npcshopdelitem,"si*"),
	BUILDIN_DEF(npcshopattach,"s?"),
	BUILDIN_DEF(equip,"i?"),
	BUILDIN_DEF(autoequip,"ii"),
	BUILDIN_DEF(setbattleflag,"si?"),
	BUILDIN_DEF(getbattleflag,"s"),
	BUILDIN_DEF(setitemscript,"is?"), //Set NEW item bonus script. Lupus
	BUILDIN_DEF(disguise,"i?"), //disguise player. Lupus
	BUILDIN_DEF(undisguise,"?"), //undisguise player. Lupus
	BUILDIN_DEF(getmonsterinfo,"ii"), //Lupus
	BUILDIN_DEF(addmonsterdrop,"vii??"), //Akinari [Lupus]
	BUILDIN_DEF(delmonsterdrop,"vi"), //Akinari [Lupus]
	BUILDIN_DEF(axtoi,"s"),
	BUILDIN_DEF(query_sql,"s*"),
	BUILDIN_DEF(query_logsql,"s*"),
	BUILDIN_DEF(query_sql_async,"s*"),
	BUILDIN_DEF(query_logsql_async,"s*"),
	BUILDIN_DEF(escape_sql,"v"),
	BUILDIN_DEF(atoi,"s"),
	BUILDIN_DEF(strtol,"si"),
	// [zBuffer] List of player cont commands --->
	BUILDIN_DEF(rid2name,"i"),
	BUILDIN_DEF(pcfollow,"ii"),
	BUILDIN_DEF(pcstopfollow,"i"),
	BUILDIN_DEF(pcblockmove,"ii"),
	BUILDIN_DEF2(pcblockmove,"unitblockmove","ii"),
	BUILDIN_DEF(pcblockskill,"ii"),
	BUILDIN_DEF2(pcblockskill,"unitblockskill","ii"),
	BUILDIN_DEF(setpcblock, "ii?"),
	BUILDIN_DEF(getpcblock, "?"),
	// <--- [zBuffer] List of player cont commands
	// [zBuffer] List of unit control commands --->
#ifndef Pandas_ScriptCommand_UnitExists
	BUILDIN_DEF(unitexists,"i"),
#else
	BUILDIN_DEF(unitexists, "i?"),
#endif // Pandas_ScriptCommand_UnitExists
	BUILDIN_DEF(getunittype,"i"),
	BUILDIN_DEF(getunitname,"i"),
	BUILDIN_DEF(setunitname,"is"),
	BUILDIN_DEF(setunittitle,"is"),
	BUILDIN_DEF(getunittitle,"i"),
	BUILDIN_DEF(getunitdata,"i*"),
	BUILDIN_DEF(setunitdata,"iii"),
	BUILDIN_DEF(unitwalk,"iii?"),
	BUILDIN_DEF2(unitwalk,"unitwalkto","ii?"),
	BUILDIN_DEF(unitkill,"i"),
	BUILDIN_DEF(unitwarp,"isii"),
	BUILDIN_DEF(unitattack,"iv?"),
	BUILDIN_DEF(unitstopattack,"i"),
	BUILDIN_DEF(unitstopwalk,"i?"),
	BUILDIN_DEF(unittalk,"is?"),
	BUILDIN_DEF_DEPRECATED(unitemote,"ii","20170811"),
	BUILDIN_DEF(unitskilluseid,"ivi????"), // originally by Qamera [Celest]
	BUILDIN_DEF(unitskillusepos,"iviii???"), // [Celest]
// <--- [zBuffer] List of unit control commands
	BUILDIN_DEF(sleep,"i"),
	BUILDIN_DEF(sleep2,"i"),
	BUILDIN_DEF(awake,"s"),
	BUILDIN_DEF(getvariableofnpc,"rs"),
	BUILDIN_DEF(warpportal,"iisii"),
	BUILDIN_DEF2(homunculus_evolution,"homevolution",""),	//[orn]
	BUILDIN_DEF2(homunculus_mutate,"hommutate","?"),
	BUILDIN_DEF(morphembryo,""),
	BUILDIN_DEF2(homunculus_shuffle,"homshuffle",""),	//[Zephyrus]
	BUILDIN_DEF(checkhomcall,""),
	BUILDIN_DEF(eaclass,"??"),	//[Skotlex]
	BUILDIN_DEF(roclass,"i?"),	//[Skotlex]
	BUILDIN_DEF(checkvending,"?"),
	BUILDIN_DEF(checkchatting,"?"),
	BUILDIN_DEF(checkidle,"?"),
	BUILDIN_DEF(checkidlehom,"?"),
	BUILDIN_DEF(checkidlemer,"?"),
	BUILDIN_DEF(openmail,"?"),
	BUILDIN_DEF(openauction,"?"),
	BUILDIN_DEF(checkcell,"siii"),
	BUILDIN_DEF(setcell,"siiiiii"),
	BUILDIN_DEF(getfreecell,"srr?????"),
	BUILDIN_DEF(setwall,"siiiiis"),
	BUILDIN_DEF(delwall,"s"),
	BUILDIN_DEF(checkwall,"s"),
	BUILDIN_DEF(searchitem,"rs"),
	BUILDIN_DEF(mercenary_create,"ii"),
	BUILDIN_DEF(mercenary_delete,"??"),
	BUILDIN_DEF(mercenary_heal,"ii"),
	BUILDIN_DEF(mercenary_sc_start,"iii"),
	BUILDIN_DEF(mercenary_get_calls,"i"),
	BUILDIN_DEF(mercenary_get_faith,"i"),
	BUILDIN_DEF(mercenary_set_calls,"ii"),
	BUILDIN_DEF(mercenary_set_faith,"ii"),
	BUILDIN_DEF(readbook,"ii"),
	BUILDIN_DEF(setfont,"i"),
	BUILDIN_DEF(areamobuseskill,"siiiiviiiii"),
	BUILDIN_DEF(progressbar,"si"),
	BUILDIN_DEF(progressbar_npc, "si?"),
	BUILDIN_DEF(pushpc,"ii"),
	BUILDIN_DEF(buyingstore,"i"),
	BUILDIN_DEF(searchstores,"ii"),
	BUILDIN_DEF(showdigit,"i?"),
	// WoE SE
	BUILDIN_DEF(agitstart2,""),
	BUILDIN_DEF(agitend2,""),
	BUILDIN_DEF(agitcheck2,""),
	// BattleGround
	BUILDIN_DEF(waitingroom2bg,"sii???"),
	BUILDIN_DEF(waitingroom2bg_single,"i????"),
	BUILDIN_DEF(bg_team_setxy,"iii"),
	BUILDIN_DEF(bg_warp,"isii"),
	BUILDIN_DEF(bg_monster,"isiisi?"),
	BUILDIN_DEF(bg_monster_set_team,"ii"),
	BUILDIN_DEF(bg_leave,"?"),
	BUILDIN_DEF2(bg_leave,"bg_desert","?"),
	BUILDIN_DEF(bg_destroy,"i"),
	BUILDIN_DEF(areapercentheal,"siiiiii"),
	BUILDIN_DEF(bg_get_data,"ii"),
	BUILDIN_DEF(bg_getareausers,"isiiii"),
	BUILDIN_DEF(bg_updatescore,"sii"),
	BUILDIN_DEF(bg_join,"i????"),
	BUILDIN_DEF(bg_create,"sii??"),
	BUILDIN_DEF(bg_reserve,"s?"),
	BUILDIN_DEF(bg_unbook,"s"),
	BUILDIN_DEF(bg_info,"si"),

	// Instancing
	BUILDIN_DEF(instance_create,"s??"),
	BUILDIN_DEF(instance_destroy,"?"),
	BUILDIN_DEF(instance_id,"?"),
	BUILDIN_DEF(instance_enter,"s????"),
	BUILDIN_DEF(instance_npcname,"s?"),
	BUILDIN_DEF(instance_mapname,"s?"),
	BUILDIN_DEF(instance_warpall,"sii?"),
	BUILDIN_DEF(instance_announce,"isi?????"),
	BUILDIN_DEF(instance_check_party,"i???"),
	BUILDIN_DEF(instance_check_guild,"i???"),
	BUILDIN_DEF(instance_check_clan,"i???"),
	BUILDIN_DEF(instance_info,"si?"),
	BUILDIN_DEF(instance_live_info,"i?"),
	BUILDIN_DEF(instance_list, "s?"),
	/**
	 * 3rd-related
	 **/
	BUILDIN_DEF(makerune,"i?"),
	BUILDIN_DEF(checkdragon,"?"),//[Ind]
	BUILDIN_DEF(setdragon,"??"),//[Ind]
	BUILDIN_DEF(ismounting,"?"),//[Ind]
	BUILDIN_DEF(setmounting,"?"),//[Ind]
	BUILDIN_DEF(checkre,"i"),
	/**
	 * rAthena and beyond!
	 **/
	BUILDIN_DEF(getargcount,""),
	BUILDIN_DEF(getcharip,"?"),
	BUILDIN_DEF(is_function,"s"),
	BUILDIN_DEF(get_revision,""),
	BUILDIN_DEF(get_githash,""),
	BUILDIN_DEF(freeloop,"?"),
	BUILDIN_DEF(getrandgroupitem,"i????"),
	BUILDIN_DEF(cleanmap,"s"),
	BUILDIN_DEF2(cleanmap,"cleanarea","siiii"),
	BUILDIN_DEF(npcskill,"viii"),
	BUILDIN_DEF(consumeitem,"v?"),
	BUILDIN_DEF(delequip,"i?"),
	BUILDIN_DEF(breakequip,"i?"),
	BUILDIN_DEF(sit,"?"),
	BUILDIN_DEF(stand,"?"),
	//@commands (script based)
	BUILDIN_DEF(bindatcmd, "ss??"),
	BUILDIN_DEF(unbindatcmd, "s"),
	BUILDIN_DEF(useatcmd, "s"),

	//Quest Log System [Inkfish]
	BUILDIN_DEF(questinfo, "i??"),
	BUILDIN_DEF(setquest, "i?"),
	BUILDIN_DEF(erasequest, "i?"),
	BUILDIN_DEF(completequest, "i?"),
	BUILDIN_DEF(checkquest, "i??"),
	BUILDIN_DEF(isbegin_quest,"i?"),
	BUILDIN_DEF(changequest, "ii?"),
	BUILDIN_DEF(showevent, "i??"),
	BUILDIN_DEF(questinfo_refresh, "?"),

	//Bound items [Xantara] & [Akinari]
	BUILDIN_DEF2(getitem,"getitembound","vii?"),
	BUILDIN_DEF2(getitem2,"getitembound2","viiiiiiiii?"),
	BUILDIN_DEF(countbound, "??"),

	// Party related
	BUILDIN_DEF(party_create,"s???"),
	BUILDIN_DEF(party_addmember,"ii"),
	BUILDIN_DEF(party_delmember,"??"),
	BUILDIN_DEF(party_changeleader,"ii"),
	BUILDIN_DEF(party_changeoption,"iii"),
	BUILDIN_DEF(party_destroy,"i"),

	// Clan system
	BUILDIN_DEF(clan_join,"i?"),
	BUILDIN_DEF(clan_leave,"?"),

	BUILDIN_DEF2(montransform, "transform", "vi?????"), // Monster Transform [malufett/Hercules]
	BUILDIN_DEF2(montransform, "active_transform", "vi?????"),
	BUILDIN_DEF(vip_status,"i?"),
	BUILDIN_DEF(vip_time,"i?"),
	BUILDIN_DEF(bonus_script,"si????"),
	BUILDIN_DEF(bonus_script_clear,"??"),
	BUILDIN_DEF(getgroupitem,"i??"),
	BUILDIN_DEF(enable_command,""),
	BUILDIN_DEF(disable_command,""),
	BUILDIN_DEF(getguildmember,"i??"),
	BUILDIN_DEF(addspiritball,"ii?"),
	BUILDIN_DEF(delspiritball,"i?"),
	BUILDIN_DEF(countspiritball,"?"),
	BUILDIN_DEF(mergeitem,"?"),
	BUILDIN_DEF(mergeitem2,"??"),
	BUILDIN_DEF(npcshopupdate,"sii?"),
	BUILDIN_DEF(getattachedrid,""),
	BUILDIN_DEF(getvar,"vi"),
	BUILDIN_DEF(showscript,"s??"),
	BUILDIN_DEF(ignoretimeout,"i?"),
	BUILDIN_DEF(geteleminfo,"i?"),
	BUILDIN_DEF(opendressroom,"i?"),
	BUILDIN_DEF(navigateto,"s???????"),
	BUILDIN_DEF(getguildalliance,"ii"),
	BUILDIN_DEF(adopt,"vv"),
	BUILDIN_DEF(getexp2,"ii?"),
	BUILDIN_DEF(recalculatestat,""),
	BUILDIN_DEF(hateffect,"ii"),
	BUILDIN_DEF(getrandomoptinfo, "i"),
	BUILDIN_DEF(getequiprandomoption, "iii?"),
	BUILDIN_DEF(setrandomoption,"iiiii?"),
	BUILDIN_DEF(needed_status_point,"ii?"),
	BUILDIN_DEF(needed_trait_point, "ii?"),
	BUILDIN_DEF(jobcanentermap,"s?"),
	BUILDIN_DEF(openstorage2,"ii?"),
	BUILDIN_DEF(unloadnpc, "s"),

	// WoE TE
	BUILDIN_DEF(agitstart3,""),
	BUILDIN_DEF(agitend3,""),
	BUILDIN_DEF(agitcheck3,""),
	BUILDIN_DEF(gvgon3,"s"),
	BUILDIN_DEF(gvgoff3,"s"),

	// Channel System
	BUILDIN_DEF(channel_create,"ss?????"),
	BUILDIN_DEF(channel_setopt,"sii"),
	BUILDIN_DEF(channel_getopt,"si"),
	BUILDIN_DEF(channel_setcolor,"si"),
	BUILDIN_DEF(channel_setpass,"ss"),
	BUILDIN_DEF(channel_setgroup,"si*"),
	BUILDIN_DEF2(channel_setgroup,"channel_setgroup2","sr"),
	BUILDIN_DEF(channel_chat,"ss?"),
	BUILDIN_DEF(channel_ban,"si"),
	BUILDIN_DEF(channel_unban,"si"),
	BUILDIN_DEF(channel_kick,"sv"),
	BUILDIN_DEF(channel_delete,"s"),

	// Item Random Option Extension [Cydh]
	BUILDIN_DEF2(getitem2,"getitem3","viiiiiiiirrr?"),
	BUILDIN_DEF2(getitem2,"getitembound3","viiiiiiiiirrr?"),
	BUILDIN_DEF2(rentitem2,"rentitem3","viiiiiiiirrr?"),
	BUILDIN_DEF2(makeitem2,"makeitem3","visiiiiiiiiirrr?"),
	BUILDIN_DEF2(delitem2,"delitem3","viiiiiiiirrr?"),
	BUILDIN_DEF2(countitem,"countitem3","viiiiiiirrr?"),

	// Achievement System
	BUILDIN_DEF(achievementinfo,"ii?"),
	BUILDIN_DEF(achievementadd,"i?"),
	BUILDIN_DEF(achievementremove,"i?"),
	BUILDIN_DEF(achievementcomplete,"i?"),
	BUILDIN_DEF(achievementexists,"i?"),
	BUILDIN_DEF(achievementupdate,"iii?"),

	BUILDIN_DEF(getequiprefinecost,"iii?"),
	BUILDIN_DEF(refineui,"?"),

	BUILDIN_DEF2(round, "round", "ii"),
	BUILDIN_DEF2(round, "ceil", "ii"),
	BUILDIN_DEF2(round, "floor", "ii"),
	BUILDIN_DEF(getequiptradability, "i?"),
	BUILDIN_DEF(mail, "isss*"),
	BUILDIN_DEF(open_roulette,"?"),
	BUILDIN_DEF(identifyall,"??"),
	BUILDIN_DEF(is_guild_leader,"?"),
	BUILDIN_DEF(is_party_leader,"?"),
	BUILDIN_DEF(camerainfo,"iii?"),

	BUILDIN_DEF(achievement_condition,"i"),
	BUILDIN_DEF(getinstancevar,"ri"),
	BUILDIN_DEF2_DEPRECATED(getinstancevar, "getvariableofinstance","ri", "2021-12-13"),
	BUILDIN_DEF(convertpcinfo,"vi"),
	BUILDIN_DEF(isnpccloaked, "??"),

	BUILDIN_DEF(rentalcountitem, "v?"),
	BUILDIN_DEF2(rentalcountitem, "rentalcountitem2", "viiiiiii?"),
	BUILDIN_DEF2(rentalcountitem, "rentalcountitem3", "viiiiiiirrr?"),

	BUILDIN_DEF(getenchantgrade, "??"),

	BUILDIN_DEF(mob_setidleevent, "is"),

	BUILDIN_DEF(setinstancevar,"rvi"),
	BUILDIN_DEF(openstylist, "?"),

	BUILDIN_DEF(getitempos,""),
	BUILDIN_DEF(laphine_synthesis, ""),
	BUILDIN_DEF(laphine_upgrade, ""),
	BUILDIN_DEF(randomoptgroup,"i"),
	BUILDIN_DEF(open_quest_ui, "??"),
	BUILDIN_DEF(openbank,"?"),
	BUILDIN_DEF(getbaseexp_ratio, "i??"),
	BUILDIN_DEF(getjobexp_ratio, "i??"),
	BUILDIN_DEF(enchantgradeui, "?" ),

	// -----------------------------------------------------------------
	// 熊猫模拟器拓展脚本指令 - 开始
	// -----------------------------------------------------------------

#ifdef Pandas_ScriptCommand_SetHeadDir
	BUILDIN_DEF(setheaddir, "i?"),						// 调整角色纸娃娃脑袋的朝向 [Sola丶小克]
#endif // Pandas_ScriptCommand_SetHeadDir
#ifdef Pandas_ScriptCommand_SetBodyDir
	BUILDIN_DEF(setbodydir, "i?"),						// 用于调整角色纸娃娃身体的朝向 [Sola丶小克]
#endif // Pandas_ScriptCommand_SetBodyDir
#ifdef Pandas_ScriptCommand_InstanceUsers
	BUILDIN_DEF(instance_users, "i"),					// 获取指定的副本实例中, 已经进入副本地图的人数 [Sola丶小克]
#endif // Pandas_ScriptCommand_InstanceUsers
#ifdef Pandas_ScriptCommand_CapValue
	BUILDIN_DEF(cap, "iii"),							// 确保数值不低于给定的最小值, 不超过给定的最大值 [Sola丶小克]
	BUILDIN_DEF2(cap, "cap_value", "iii"),				// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_CapValue
#ifdef Pandas_ScriptCommand_MobRemove
	BUILDIN_DEF(mobremove, "i"),						// 根据 GID 移除一个魔物单位 [Sola丶小克]
#endif // Pandas_ScriptCommand_MobRemove
#ifdef Pandas_ScriptCommand_MesClear
	BUILDIN_DEF2(clear, "mesclear", ""),				// 由于 rAthena 已经实现 clear 指令, 这里兼容老版本 mesclear 指令 [Sola丶小克]
#endif // Pandas_ScriptCommand_MesClear
#ifdef Pandas_ScriptCommand_BattleIgnore
	BUILDIN_DEF(battleignore, "i?"),					// 将角色设置为魔物免战状态, 避免被魔物攻击 [Sola丶小克]
#endif // Pandas_ScriptCommand_BattleIgnore
#ifdef Pandas_ScriptCommand_GetHotkey
	BUILDIN_DEF(gethotkey, "i?"),						// 获取指定快捷键位置当前的信息 [Sola丶小克]
	BUILDIN_DEF2(gethotkey, "get_hotkey", "i?"),		// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_GetHotkey
#ifdef Pandas_ScriptCommand_SetHotkey
	BUILDIN_DEF(sethotkey, "iiii"),						// 设置指定快捷键位置的信息 [Sola丶小克]
	BUILDIN_DEF2(sethotkey, "set_hotkey", "iiii"),		// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_SetHotkey
#ifdef Pandas_ScriptCommand_ShowVend
	BUILDIN_DEF(showvend, "si?"),						// 使指定的 NPC 头上可以显示露天商店的招牌 [Jian916]
#endif // Pandas_ScriptCommand_ShowVend
#ifdef Pandas_ScriptCommand_ViewEquip
	BUILDIN_DEF(viewequip, "i?"),						// 查看指定在线角色的装备面板信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_ViewEquip
#ifdef Pandas_ScriptCommand_CountItemIdx
	BUILDIN_DEF(countitemidx, "i?"),					// 获取指定背包序号的道具在背包中的数量 [Sola丶小克]
	BUILDIN_DEF2(countitemidx, "countinventory", "i?"),	// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_CountItemIdx
#ifdef Pandas_ScriptCommand_DelItemIdx
	BUILDIN_DEF2(delitemidx, "delinventory", "i??"),	// 指定一个别名, 以便兼容的老版本或其他服务端 [Sola丶小克]
#endif // Pandas_ScriptCommand_DelItemIdx
#ifdef Pandas_ScriptCommand_IdentifyIdx
	BUILDIN_DEF(identifyidx, "i?"),						// 鉴定指定背包序号的道具 [Sola丶小克]
	BUILDIN_DEF2(identifyidx, "identifybyidx", "i?"),	// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_IdentifyIdx
#ifdef Pandas_ScriptCommand_UnEquipIdx
	BUILDIN_DEF(unequipidx, "i?"),						// 脱下指定背包序号的道具 [Sola丶小克]
	BUILDIN_DEF2(unequipidx, "unequipinventory", "i?"),	// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_UnEquipIdx
#ifdef Pandas_ScriptCommand_EquipIdx
	BUILDIN_DEF(equipidx, "i?"),						// 穿戴指定背包序号的道具 [Sola丶小克]
	BUILDIN_DEF2(equipidx, "equipinventory", "i?"),		// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_EquipIdx
#ifdef Pandas_ScriptCommand_ItemExists
	BUILDIN_DEF(itemexists, "v"),						// 确认物品数据库中是否存在指定物品 [Sola丶小克]
	BUILDIN_DEF2(itemexists, "existitem", "v"),			// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_ItemExists
#ifdef Pandas_ScriptCommand_RentTime
	BUILDIN_DEF(renttime, "ii?"),						// 增加/减少指定位置装备的租赁时间 [Sola丶小克]
	BUILDIN_DEF2(renttime, "setrenttime", "ii?"),		// 指定一个别名, 以便兼容的老版本或其他服务端
	BUILDIN_DEF2(renttime, "resume", "ii?"),			// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_RentTime
#ifdef Pandas_ScriptCommand_GetEquipIdx
	BUILDIN_DEF(getequipidx, "i?"),						// 获取指定位置装备的背包序号 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetEquipIdx
#ifdef Pandas_ScriptCommand_StatusCalc
	BUILDIN_DEF2(recalculatestat, "statuscalc", ""),	// 由于 rAthena 已经实现 recalculatestat 指令, 这里兼容老版本 statuscalc 指令 [Sola丶小克]
	BUILDIN_DEF2(recalculatestat, "status_calc", ""),	// 由于 rAthena 已经实现 recalculatestat 指令, 这里兼容老版本 status_calc 指令
#endif // Pandas_ScriptCommand_StatusCalc
#ifdef Pandas_ScriptCommand_GetEquipExpireTick
	BUILDIN_DEF(getequipexpiretick, "i?"),				// 获取指定位置装备的租赁到期剩余秒数 [Sola丶小克]
	BUILDIN_DEF2(getequipexpiretick, "isrental", "i?"),	// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_GetEquipExpireTick
#ifdef Pandas_ScriptCommand_GetInventoryInfo
	BUILDIN_DEF(getinventoryinfo, "ii?"),							// 查询指定背包序号的道具详细信息 [Sola丶小克]
	BUILDIN_DEF2(getinventoryinfo, "getcartinfo", "ii?"),			// 查询指定手推车序号的道具详细信息 [Sola丶小克]
	BUILDIN_DEF2(getinventoryinfo, "getguildstorageinfo", "ii?"),	// 查询指定公会仓库序号的道具详细信息 [Sola丶小克]
	BUILDIN_DEF2(getinventoryinfo, "getstorageinfo", "ii??"),		// 查询指定个人仓库/扩充仓库序号的道具详细信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetInventoryInfo
#ifdef Pandas_ScriptCommand_StatusCheck
	BUILDIN_DEF(statuscheck, "i?"),						// 判断状态是否存在, 并取得相关的状态参数 [Sola丶小克]
	BUILDIN_DEF2(statuscheck, "sc_check", "i?"),		// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_StatusCheck
#ifdef Pandas_ScriptCommand_RentTimeIdx
	BUILDIN_DEF(renttimeidx, "ii?"),					// 增加/减少指定背包序号道具的租赁时间 [Sola丶小克]
#endif // Pandas_ScriptCommand_RentTimeIdx
#ifdef Pandas_ScriptCommand_PartyLeave
	BUILDIN_DEF(party_leave, "?"),						// 使当前角色或指定角色退出队伍 [Sola丶小克]
#endif // Pandas_ScriptCommand_PartyLeave
#ifdef Pandas_ScriptCommand_Script4Each
	BUILDIN_DEF(script4each, "si?????"),					// 对指定范围的玩家执行相同的一段脚本 [Sola丶小克]
	BUILDIN_DEF2(script4each, "script4eachmob", "si?????"),	// 对指定范围的魔物执行相同的一段脚本
	BUILDIN_DEF2(script4each, "script4eachnpc", "si?????"),	// 对指定范围的 NPC 执行相同的一段脚本
#endif // Pandas_ScriptCommand_Script4Each
#ifdef Pandas_ScriptCommand_SearchArray
	BUILDIN_DEF2(inarray, "searcharray", "rv"),			// 由于 rAthena 已经实现 inarray 指令, 这里兼容老版本 searcharray 指令 [Sola丶小克]
#endif // Pandas_ScriptCommand_SearchArray
#ifdef Pandas_ScriptCommand_GetSameIpInfo
	BUILDIN_DEF(getsameipinfo, "??"),					// 获得某个指定 IP 在线的玩家信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetSameIpInfo
#ifdef Pandas_ScriptCommand_Logout
	BUILDIN_DEF(logout, "i?"),							// 使指定的角色立刻登出游戏 [Sola丶小克]
#endif // Pandas_ScriptCommand_Logout
#ifdef Pandas_ScriptCommand_WarpPartyRevive
	BUILDIN_DEF2(warpparty, "warppartyrevive", "siii???"),	// 与 warpparty 类似, 但可以复活死亡的队友并传送 [Sola丶小克]
	BUILDIN_DEF2(warpparty, "warpparty2", "siii???"),		// 指定一个别名, 以便兼容的老版本或其他服务端
#endif // Pandas_ScriptCommand_WarpPartyRevive
#ifdef Pandas_ScriptCommand_GetAreaGid
	BUILDIN_DEF(getareagid, "ri??????"),				// 获取指定范围内特定类型单位的全部 GID [Sola丶小克]
#endif // Pandas_ScriptCommand_GetAreaGid
#ifdef Pandas_ScriptCommand_ProcessHalt
	BUILDIN_DEF(processhalt, "?"),						// 用于中断源代码的后续处理逻辑 [Sola丶小克]
#endif // Pandas_ScriptCommand_ProcessHalt
#ifdef Pandas_ScriptCommand_SetEventTrigger
	BUILDIN_DEF(settrigger, "ii"),						// 使用该指令可以设置某个事件或过滤器的触发行为 [Sola丶小克]
#endif // Pandas_ScriptCommand_SetEventTrigger
#ifdef Pandas_ScriptCommand_MessageColor
	BUILDIN_DEF(messagecolor, "s???"),					// 发送指定颜色的消息文本到聊天窗口中 [Sola丶小克]
#endif // Pandas_ScriptCommand_MessageColor
#ifdef Pandas_ScriptCommand_Copynpc
	BUILDIN_DEF(copynpc, "???????"),					// 复制指定的 NPC 到一个新的位置 [Sola丶小克]
#endif // Pandas_ScriptCommand_Copynpc
#ifdef Pandas_ScriptCommand_GetTimeFmt
	BUILDIN_DEF(gettimefmt, "s??"),						// 将当前时间格式化输出成字符串, 是 gettimestr 的改进版 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetTimeFmt
#ifdef Pandas_ScriptCommand_MultiCatchPet
	BUILDIN_DEF(multicatchpet, "*"),					// 与 catchpet 指令类似, 但可以指定更多支持捕捉的魔物编号 [Sola丶小克]
		BUILDIN_DEF2(multicatchpet, "mpet", "*"),		// 指定一个别名, 以便简化编码工作量
#endif // Pandas_ScriptCommand_MultiCatchPet
#ifdef Pandas_ScriptCommand_SelfDeletion
	BUILDIN_DEF(selfdeletion, "*"),						// 设置 NPC 的自毁策略 [Sola丶小克]
#endif // Pandas_ScriptCommand_SelfDeletion
#ifdef Pandas_ScriptCommand_SetCharTitle
	BUILDIN_DEF(setchartitle, "i?"),					// 设置指定玩家的称号ID [Sola丶小克]
#endif // Pandas_ScriptCommand_SetCharTitle
#ifdef Pandas_ScriptCommand_GetCharTitle
	BUILDIN_DEF(getchartitle, "?"),						// 获得指定玩家的称号ID [Sola丶小克]
#endif // Pandas_ScriptCommand_GetCharTitle
#ifdef Pandas_ScriptCommand_NpcExists
	BUILDIN_DEF(npcexists, "s?"),						// 判断指定名称的 NPC 是否存在 [Sola丶小克]
#endif // Pandas_ScriptCommand_NpcExists
#ifdef Pandas_ScriptCommand_StorageGetItem
	BUILDIN_DEF(storagegetitem, "vi?"),								// 往仓库直接创造一个指定的道具 [Sola丶小克]
	BUILDIN_DEF2(storagegetitem, "storagegetitembound", "vii?"),	// 与 getitembound 类似, 只不过是将道具直接创建到仓库
#endif // Pandas_ScriptCommand_StorageGetItem
#ifdef Pandas_ScriptCommand_SetInventoryInfo
	BUILDIN_DEF(setinventoryinfo, "iii??"),				// 设置指定背包序号的道具的详细信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_SetInventoryInfo
#ifdef Pandas_ScriptCommand_UpdateInventory
	BUILDIN_DEF(updateinventory, "?"),					// 重新下发关联玩家的背包数据给客户端 [Sola丶小克]
#endif // Pandas_ScriptCommand_UpdateInventory
#ifdef Pandas_ScriptCommand_GetCharMacAddress
	BUILDIN_DEF(getcharmac, "?"),						// 获取指定角色登录时使用的 MAC 地址 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetCharMacAddress
#ifdef Pandas_ScriptCommand_GetConstant
	BUILDIN_DEF(getconstant, "s"),						// 查询一个常量字符串对应的数值 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetConstant
#ifdef Pandas_ScriptCommand_Preg_Search
	BUILDIN_DEF(preg_search, "ssir"),					// 使用正则表达式搜索并返回首个匹配的分组内容 [Sola丶小克]
#endif // Pandas_ScriptCommand_Preg_Search
#ifdef Pandas_ScriptCommand_Aura
	BUILDIN_DEF(aura, "i?"),							// 激活指定的光环组合 [Sola丶小克]
#endif // Pandas_ScriptCommand_Aura
#ifdef Pandas_ScriptCommand_UnitAura
	BUILDIN_DEF(unitaura, "ii"),						// 用于调整七种单位的光环组合 [Sola丶小克]
#endif // Pandas_ScriptCommand_UnitAura
#ifdef Pandas_ScriptCommand_GetUnitTarget
	BUILDIN_DEF(getunittarget, "i"),					// 获取指定单位当前正在攻击的目标单位编号 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetUnitTarget
#ifdef Pandas_ScriptCommand_UnlockCmd
	BUILDIN_DEF(unlockcmd, ""),							// 解锁实时事件和过滤器事件的指令限制 [Sola丶小克]
#endif // Pandas_ScriptCommand_UnlockCmd
#ifdef Pandas_ScriptCommand_BattleRecordQuery
	BUILDIN_DEF(batrec_query, "iii?"),					// 查询指定单位的战斗记录, 查看与交互目标单位产生的具体记录值 [Sola丶小克]
#endif // Pandas_ScriptCommand_BattleRecordQuery
#ifdef Pandas_ScriptCommand_BattleRecordRank
	BUILDIN_DEF(batrec_rank, "irri??"),					// 查询指定单位的战斗记录并对记录的值进行排序, 返回排行榜单 [Sola丶小克]
#endif // Pandas_ScriptCommand_BattleRecordRank
#ifdef Pandas_ScriptCommand_BattleRecordSortout
	BUILDIN_DEF(batrec_sortout, "i?"),					// 移除指定单位的战斗记录中交互单位已经不存在 (或下线) 的记录 [Sola丶小克]
#endif // Pandas_ScriptCommand_BattleRecordSortout
#ifdef Pandas_ScriptCommand_BattleRecordReset
	BUILDIN_DEF(batrec_reset, "i"),						// 清除指定单位的战斗记录 [Sola丶小克]
#endif // Pandas_ScriptCommand_BattleRecordReset
#ifdef Pandas_ScriptCommand_EnableBattleRecord
	BUILDIN_DEF(enable_batrec, "?"),					// 启用指定单位的战斗记录 [Sola丶小克]
#endif // Pandas_ScriptCommand_EnableBattleRecord
#ifdef Pandas_ScriptCommand_DisableBattleRecord
	BUILDIN_DEF(disable_batrec, "?"),					// 禁用指定单位的战斗记录 [Sola丶小克]
#endif // Pandas_ScriptCommand_DisableBattleRecord
#ifdef Pandas_ScriptCommand_Login
	BUILDIN_DEF(login, "i????"),						// 将指定的角色以特定的登录模式拉上线 [Sola丶小克]
#endif // Pandas_ScriptCommand_Login
#ifdef Pandas_ScriptCommand_CheckSuspend
	BUILDIN_DEF(checksuspend, "?"),						// 获取指定角色或指定账号当前在线角色的挂机模式 [Sola丶小克]
#endif // Pandas_ScriptCommand_CheckSuspend
#ifdef Pandas_ScriptCommand_BonusScriptRemove
	BUILDIN_DEF(bonus_script_remove, "i?"),				// 移除指定的 bonus_script 效果脚本 [Sola丶小克]
#endif // Pandas_ScriptCommand_BonusScriptRemove
#ifdef Pandas_ScriptCommand_BonusScriptList
	BUILDIN_DEF(bonus_script_list, "r?"),				// 获取指定角色当前激活的全部 bonus_script 效果脚本编号 [Sola丶小克]
#endif // Pandas_ScriptCommand_BonusScriptList
#ifdef Pandas_ScriptCommand_BonusScriptExists
	BUILDIN_DEF(bonus_script_exists, "i?"),				// 查询指定角色是否已经激活了特定的 bonus_script 效果脚本 [Sola丶小克]
#endif // Pandas_ScriptCommand_BonusScriptExists
#ifdef Pandas_ScriptCommand_BonusScriptGetId
	BUILDIN_DEF(bonus_script_getid, "sr?"),				// 查询效果脚本代码对应的效果脚本编号 [Sola丶小克]
#endif // Pandas_ScriptCommand_BonusScriptGetId
#ifdef Pandas_ScriptCommand_BonusScriptInfo
	BUILDIN_DEF(bonus_script_info, "ii?"),				// 查询指定效果脚本的相关信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_BonusScriptInfo
#ifdef Pandas_ScriptCommand_ExpandInventoryAdjust
	BUILDIN_DEF(expandinventory_adjust, "i"),			// 增加角色的背包容量上限 [Sola丶小克]
#endif // Pandas_ScriptCommand_ExpandInventoryAdjust
#ifdef Pandas_ScriptCommand_GetInventorySize
	BUILDIN_DEF(getinventorysize, "?"),					// 查询并获取当前角色的背包容量上限 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetInventorySize
#ifdef Pandas_ScriptCommand_GetMapSpawns
	BUILDIN_DEF(getmapspawns, "s?"),					// 在此写上脚本指令说明 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetMapSpawns
#ifdef Pandas_ScriptCommand_GetMobSpawns
	BUILDIN_DEF(getmobspawns,"i??"),					// 查询指定魔物在不同地图的刷新点信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetMobSpawns
#ifdef Pandas_ScriptCommand_GetCalendarTime
	BUILDIN_DEF(getcalendartime,"ii??"),				// 获取下次出现指定时间的 UNIX 时间戳 [Haru]
#endif // Pandas_ScriptCommand_GetCalendarTime
#ifdef Pandas_ScriptCommand_GetSkillInfo
	BUILDIN_DEF(getskillinfo, "iv??"),					// 获取指定技能在技能数据库中所配置的各项信息 [聽風]
#endif // Pandas_ScriptCommand_GetSkillInfo
#ifdef Pandas_ScriptCommand_BossMonster
	BUILDIN_DEF2(monster,"boss_monster", "siisii???"),	// 召唤魔物并使之能被 BOSS 雷达探测 (哪怕被召唤魔物本身不是 BOSS) [人鱼姬的思念]
#endif // Pandas_ScriptCommand_BossMonster
#ifdef Pandas_ScriptCommand_Sleep3
	BUILDIN_DEF(sleep3,"i"),							// 休眠一段时间再执行后续脚本, 与 sleep2 类似但忽略报错 [人鱼姬的思念]
#endif // Pandas_ScriptCommand_Sleep3
#ifdef Pandas_ScriptCommand_GetQuestTime
	BUILDIN_DEF(getquesttime,"i??"),					// 查询角色指定任务的时间信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetQuestTime
#ifdef Pandas_ScriptCommand_UnitSpecialEffect
	BUILDIN_DEF(unitspecialeffect, "ii??"),				// 使指定游戏单位可以显示某个特效, 并支持控制特效可见范围 [人鱼姬的思念]
#endif // Pandas_ScriptCommand_UnitSpecialEffect
#ifdef Pandas_ScriptCommand_Next_Dropitem_Special
	BUILDIN_DEF(next_dropitem_special,"iii"),			// 对下一个掉落到地面上的物品进行特殊设置 [Sola丶小克]
#endif // Pandas_ScriptCommand_Next_Dropitem_Special
#ifdef Pandas_ScriptCommand_GetGradeItem
	BUILDIN_DEF2(getitem2,"getgradeitem","viiiiiiiiirrr?"),		// 创造带有指定附魔评级的道具 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetGradeItem
#ifdef Pandas_ScriptCommand_GetRateIdx
	BUILDIN_DEF(getrateidx,"*"),						// 随机获取一个数值型数组的索引序号 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetRateIdx
#ifdef Pandas_ScriptCommand_GetBossInfo
	BUILDIN_DEF(getbossinfo,"???"),						// 查询 BOSS 魔物重生时间及其坟墓等信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_GetBossInfo
#ifdef Pandas_ScriptCommand_WhoDropItem
	BUILDIN_DEF(whodropitem,"v??"),						// 查询指定道具会从哪些魔物身上掉落以及掉落的机率信息 [Sola丶小克]
#endif // Pandas_ScriptCommand_WhoDropItem
	// PYHELP - SCRIPTCMD - INSERT POINT - <Section 3>

#include "../custom/script_def.inc"

	{NULL,NULL,NULL},
};

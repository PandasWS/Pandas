// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "suspend.hpp"

#include "../common/malloc.hpp" // aMalloc, aFree
#include "../common/nullpo.hpp"
#include "../common/showmsg.hpp"
#include "../common/sql.hpp"

#include "map.hpp"
#include "chrif.hpp"
#include "pc.hpp"
#include "channel.hpp"
#include "battle.hpp"
#include "chat.hpp"
#include "trade.hpp"
#include "storage.hpp"
#include "party.hpp"
#include "duel.hpp"
#include "guild.hpp"

static DBMap* suspender_db;
static void suspend_suspender_remove(struct s_suspender* sp, bool remove);
static int suspend_suspender_free(DBKey key, DBData* data, va_list ap);

//************************************
// Method:      suspend_recall_online
// Description: 将全部挂起的角色按照各自的模式召回上线
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 23:35
//************************************
void suspend_recall_online() {
	if (Sql_Query(mmysql_handle,
		"SELECT `account_id`, `char_id`, `sex`, `body_direction`, `head_direction`, `sit`, `mode`, `tick`, `val1`, `val2`, `val3`, `val4` "
		"FROM `%s` "
		"ORDER BY `account_id`;",
		suspend_table) != SQL_SUCCESS)
	{
		Sql_ShowDebug(mmysql_handle);
		return;
	}

	int offline = 0, afk = 0;

	if (Sql_NumRows(mmysql_handle) > 0) {
		DBIterator* iter = NULL;
		struct s_suspender* sp = NULL;

		// Init each autotrader data
		while (SQL_SUCCESS == Sql_NextRow(mmysql_handle)) {
			char* data = NULL;

			sp = NULL;
			CREATE(sp, struct s_suspender, 1);
			Sql_GetData(mmysql_handle, 0, &data, NULL); sp->account_id = atoi(data);
			Sql_GetData(mmysql_handle, 1, &data, NULL); sp->char_id = atoi(data);
			Sql_GetData(mmysql_handle, 2, &data, NULL); sp->sex = (data[0] == 'F') ? SEX_FEMALE : SEX_MALE;
			Sql_GetData(mmysql_handle, 3, &data, NULL); sp->dir = atoi(data);
			Sql_GetData(mmysql_handle, 4, &data, NULL); sp->head_dir = atoi(data);
			Sql_GetData(mmysql_handle, 5, &data, NULL); sp->sit = atoi(data);

			Sql_GetData(mmysql_handle, 6, &data, NULL); sp->mode = e_suspend_mode(atoi(data));
			Sql_GetData(mmysql_handle, 7, &data, NULL); sp->tick = strtoll(data, nullptr, 10);

			Sql_GetData(mmysql_handle, 8, &data, NULL); sp->val1 = atoi(data);
			Sql_GetData(mmysql_handle, 9, &data, NULL); sp->val2 = atoi(data);
			Sql_GetData(mmysql_handle,10, &data, NULL); sp->val3 = atoi(data);
			Sql_GetData(mmysql_handle,11, &data, NULL); sp->val4 = atoi(data);

			// 初始化一个玩家对象
			CREATE(sp->sd, struct map_session_data, 1);
			pc_setnewpc(sp->sd, sp->account_id, sp->char_id, 0, gettick(), sp->sex, 0);

			// 设置 autotrade 标记
			sp->sd->state.autotrade |= AUTOTRADE_ENABLED;
			if (sp->mode == SUSPEND_MODE_OFFLINE) {
				sp->sd->state.autotrade |= AUTOTRADE_OFFLINE;
				offline++;
			}
			else if (sp->mode == SUSPEND_MODE_AFK) {
				sp->sd->state.autotrade |= AUTOTRADE_AFK;
				afk++;
			}

			// 根据战斗配置选项来设置魔物免疫状态
			if (sp->mode != SUSPEND_MODE_NONE && battle_config.suspend_monsterignore & sp->mode)
				sp->sd->state.block_action |= PCBLOCK_IMMUNE;
			else
				sp->sd->state.block_action &= ~PCBLOCK_IMMUNE;

			// 根据战斗配置选项来设置朝向和角色站立状态
			switch (sp->mode)
			{
			case SUSPEND_MODE_OFFLINE:
				if (battle_config.suspend_offline_bodydirection >= 0)
					sp->dir = battle_config.suspend_offline_bodydirection;
				if (battle_config.suspend_offline_headdirection >= 0)
					sp->head_dir = battle_config.suspend_offline_headdirection;
				if (battle_config.suspend_offline_sitdown >= 0)
					sp->sit = battle_config.suspend_offline_sitdown;
				break;
			case SUSPEND_MODE_AFK:
				if (battle_config.suspend_afk_bodydirection >= 0)
					sp->dir = battle_config.suspend_afk_bodydirection;
				if (battle_config.suspend_afk_headdirection >= 0)
					sp->head_dir = battle_config.suspend_afk_headdirection;
				if (battle_config.suspend_afk_sitdown >= 0)
					sp->sit = battle_config.suspend_afk_sitdown;
				break;
			}

			// 向服务器请求上线该玩家
			chrif_authreq(sp->sd, true);
			uidb_put(suspender_db, sp->char_id, sp);
		}
		Sql_FreeResult(mmysql_handle);

		ShowStatus("Done loading '" CL_WHITE "%d" CL_RESET "' suspend player records (Offline: '%d', AFK: '%d').\n", db_size(suspender_db), offline, afk);
	}
}

//************************************
// Method:      suspend_recall_postfix
// Description: 被召回的角色成功上线后需要做的后置处理
// Parameter:   struct map_session_data * sd
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/8 10:14
//************************************
void suspend_recall_postfix(struct map_session_data* sd) {
	nullpo_retv(sd);

	struct s_suspender* sp = NULL;
	if (sp = (struct s_suspender*)uidb_get(suspender_db, sd->status.char_id)) {
		pc_setdir(sd, sp->dir, sp->head_dir);
		clif_changed_dir(&sd->bl, AREA_WOS);
		if (sp->sit) {
			pc_setsit(sd);
			skill_sit(sd, 1);
			clif_sitting(&sd->bl);
		}

		switch (sp->mode)
		{
		case SUSPEND_MODE_AFK:
			if (battle_config.suspend_afk_headtop_viewid) {
				clif_changelook(&sd->bl, LOOK_HEAD_TOP, battle_config.suspend_afk_headtop_viewid);
			}
			break;
		}
	}
}

//************************************
// Method:      suspend_set_unit_idle
// Description: 在 clif_set_unit_idle 函数中对发送给客户端的封包进行修改
// Parameter:   struct map_session_data * sd
// Parameter:   unsigned char * buf
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/8 13:14
//************************************
void suspend_set_unit_idle(struct map_session_data* sd, unsigned char* buf) {
	nullpo_retv(sd);
	nullpo_retv(buf);

	if (sd->bl.type != BL_PC)
		return;

	struct s_suspender* sp = NULL;
	if (sp = (struct s_suspender*)uidb_get(suspender_db, sd->status.char_id)) {
		switch (sp->mode)
		{
		case SUSPEND_MODE_AFK:
			if (battle_config.suspend_afk_headtop_viewid) {
				WBUFW(buf, 24) = battle_config.suspend_afk_headtop_viewid;
			}
			break;
		}
	}
}

//************************************
// Method:      suspend_set_unit_walking
// Description: 在 clif_set_unit_walking 函数中对发送给客户端的封包进行修改
// Parameter:   struct map_session_data * sd
// Parameter:   unsigned char * buf
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/8 13:16
//************************************
void suspend_set_unit_walking(struct map_session_data* sd, unsigned char* buf) {
	nullpo_retv(sd);
	nullpo_retv(buf);

	if (sd->bl.type != BL_PC)
		return;

	struct s_suspender* sp = NULL;
	if (sp = (struct s_suspender*)uidb_get(suspender_db, sd->status.char_id)) {
		switch (sp->mode)
		{
		case SUSPEND_MODE_AFK:
			if (battle_config.suspend_afk_headtop_viewid) {
				WBUFW(buf, 28) = battle_config.suspend_afk_headtop_viewid;
			}
			break;
		}
	}
}

//************************************
// Method:      suspend_active
// Description: 激活某个角色的某种挂起模式, 并使其断开与客户端的连接
// Parameter:   struct map_session_data * sd
// Parameter:   enum e_suspend_mode smode
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 23:05
//************************************
void suspend_active(struct map_session_data* sd, enum e_suspend_mode smode) {
	nullpo_retv(sd);
	long val[4] = { 0 };

	// 设置 autotrade 标记
	sd->state.autotrade |= AUTOTRADE_ENABLED;
	if (smode == SUSPEND_MODE_OFFLINE)
		sd->state.autotrade |= AUTOTRADE_OFFLINE;
	else if (smode == SUSPEND_MODE_AFK)
		sd->state.autotrade |= AUTOTRADE_AFK;

	// 根据战斗配置选项来设置魔物免疫状态
	if (smode != SUSPEND_MODE_NONE && battle_config.suspend_monsterignore & smode)
		sd->state.block_action |= PCBLOCK_IMMUNE;
	else
		sd->state.block_action &= ~PCBLOCK_IMMUNE;

	// 执行不同挂机模式的一些额外设置
	switch (smode)
	{
	case SUSPEND_MODE_AFK:
		if (!pc_issit(sd)) {
			pc_setsit(sd);
			skill_sit(sd, 1);
			clif_sitting(&sd->bl);
		}
		if (battle_config.suspend_afk_headtop_viewid) {
			clif_changelook(&sd->bl, LOOK_HEAD_TOP, battle_config.suspend_afk_headtop_viewid);
		}
		break;
	}

#ifdef Pandas_Struct_Map_Session_Data_Autotrade_Configure
	// 这里需要立刻填充相关的备份信息, 避免在完成指令下线后,
	// 服务器没还重启的情况下, 角色就被 recall 导致朝向等数据无法恢复
	sd->pandas.at_dir = sd->ud.dir;
	sd->pandas.at_head_dir = sd->head_dir;
	sd->pandas.at_sit = pc_issit(sd);
#endif // Pandas_Struct_Map_Session_Data_Autotrade_Configure

	if (Sql_Query(mmysql_handle, "INSERT INTO `%s`(`account_id`, `char_id`, `sex`, `map`, `x`, `y`, `body_direction`, `head_direction`, `sit`, `mode`, `tick`, `val1`, `val2`, `val3`, `val4`) "
		"VALUES( %d, %d, '%c', '%s', %d, %d, '%d', '%d', '%d', '%hu', '%" PRtf "', '%ld', '%ld', '%ld', '%ld' );",
		suspend_table, sd->status.account_id, sd->status.char_id, sd->status.sex == SEX_FEMALE ? 'F' : 'M', map_getmapdata(sd->bl.m)->name,
		sd->bl.x, sd->bl.y, sd->ud.dir, sd->head_dir, pc_issit(sd), int(smode), gettick(), val[0], val[1], val[2], val[3]) != SQL_SUCCESS) {
		Sql_ShowDebug(mmysql_handle);
	}

	// 若正在聊天室内, 则离开聊天室
	if (sd->chatID)
		chat_leavechat(sd, 0);

	// 若正在进行交易, 则立刻取消交易
	if (sd->trade_partner)
		trade_tradecancel(sd);

	// 关闭正在访问的仓库, 防止卡住公会仓库 (感谢"喵了个咪"反馈)
	if (sd->state.storage_flag == 1)
		storage_storage_quit(sd, 0);
	else if (sd->state.storage_flag == 2)
		storage_guild_storage_quit(sd, 0);
	else if (sd->state.storage_flag == 3)
		storage_premiumStorage_quit(sd);

	// 重置仓库访问标记位
	sd->state.storage_flag = 0;

	// 若正在被邀请加入队伍, 则立刻回绝
	if (sd->party_invite > 0)
		party_reply_invite(sd, sd->party_invite, 0);

	// 若正在被邀请加入公会, 则立刻回绝
	if (sd->guild_invite > 0)
		guild_reply_invite(sd, sd->guild_invite, 0);

	// 若正在被邀请创建公会同盟, 则立刻回绝
	if (sd->guild_alliance > 0)
		guild_reply_reqalliance(sd, sd->guild_alliance_account, 0);

	// 若处于决斗状态, 则离开决斗
	if (sd->duel_group > 0)
		duel_leave(sd->duel_group, sd);

	// 离开所有聊天频道
	channel_pcquit(sd, 0xF);

	clif_authfail_fd(sd->fd, 15);

	chrif_save(sd, CSAVE_AUTOTRADE);
}

//************************************
// Method:      suspend_deactive
// Description: 反激活某个角色的挂起模式, 下次地图服务器启动将不再自动上线
// Parameter:   struct map_session_data * sd
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/6 18:15
//************************************
void suspend_deactive(struct map_session_data* sd) {
	nullpo_retv(sd);

	if (Sql_Query(mmysql_handle, "DELETE FROM `%s` WHERE `account_id` = %d;", suspend_table, sd->status.account_id) != SQL_SUCCESS) {
		Sql_ShowDebug(mmysql_handle);
	}

	struct s_suspender* sp = NULL;
	if (sp = (struct s_suspender*)uidb_get(suspender_db, sd->status.char_id)) {
		suspend_suspender_remove(sp, true);
		if (db_size(suspender_db) == 0) {
			suspender_db->clear(suspender_db, suspend_suspender_free);
		}
	}
}

//************************************
// Method:      suspend_suspender_remove
// Description: 是否某个具体的 struct s_suspender 结构体
// Parameter:   struct s_suspender * sp
// Parameter:   bool remove
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/6 18:42
//************************************
static void suspend_suspender_remove(struct s_suspender* sp, bool remove) {
	nullpo_retv(sp);
	if (remove)
		uidb_remove(suspender_db, sp->char_id);
	aFree(sp);
}

//************************************
// Method:      suspend_suspender_free
// Description: 用于释放 suspender_db 数据的子函数
// Parameter:   DBKey key
// Parameter:   DBData * data
// Parameter:   va_list ap
// Returns:     int
// Author:      Sola丶小克(CairoLee)  2020/4/5 22:58
//************************************
static int suspend_suspender_free(DBKey key, DBData* data, va_list ap) {
	struct s_suspender* sp = (struct s_suspender*)db_data2ptr(data);
	if (sp)
		suspend_suspender_remove(sp, false);
	return 0;
}

//************************************
// Method:      do_final_suspend
// Description: 释放玩家挂起子系统
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 22:57
//************************************
void do_final_suspend(void) {
	suspender_db->destroy(suspender_db, suspend_suspender_free);
}

//************************************
// Method:      do_init_suspend
// Description: 初始化玩家挂起子系统
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 22:58
//************************************
void do_init_suspend(void) {
	suspender_db = uidb_alloc(DB_OPT_BASE);
}

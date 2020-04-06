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

static DBMap* suspender_db;
static void suspend_suspender_remove(struct s_suspender* sp, bool remove);
static int suspend_suspender_free(DBKey key, DBData* data, va_list ap);

//************************************
// Method:      suspend_recall_online
// Description: 
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

// 			if (battle_config.feature_autotrade_direction >= 0)
// 				at->dir = battle_config.feature_autotrade_direction;
// 			if (battle_config.feature_autotrade_head_direction >= 0)
// 				at->head_dir = battle_config.feature_autotrade_head_direction;
// 			if (battle_config.feature_autotrade_sit >= 0)
// 				at->sit = battle_config.feature_autotrade_sit;

			// initialize player
			CREATE(sp->sd, struct map_session_data, 1);
			pc_setnewpc(sp->sd, sp->account_id, sp->char_id, 0, gettick(), sp->sex, 0);
			sp->sd->state.autotrade |= AUTOTRADE_ENABLED;

			if (sp->mode == SUSPEND_MODE_OFFLINE)
				sp->sd->state.autotrade |= AUTOTRADE_OFFLINE;
			else if (sp->mode == SUSPEND_MODE_AFK)
				sp->sd->state.autotrade |= AUTOTRADE_AFK;

// 			if (battle_config.autotrade_monsterignore)
// 				sp->sd->state.block_action |= PCBLOCK_IMMUNE;
// 			else
// 				sp->sd->state.block_action &= ~PCBLOCK_IMMUNE;

			chrif_authreq(sp->sd, true);
			uidb_put(suspender_db, sp->char_id, sp);
		}
		Sql_FreeResult(mmysql_handle);

		ShowStatus("Done loading '" CL_WHITE "%d" CL_RESET "' suspend player.\n", db_size(suspender_db));
	}
}

//************************************
// Method:      suspend_active
// Description: 
// Parameter:   struct map_session_data * sd
// Parameter:   enum e_suspend_mode smode
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 23:05
//************************************
void suspend_active(struct map_session_data* sd, enum e_suspend_mode smode) {
	nullpo_retv(sd);
	long val[4] = { 0 };

	sd->state.autotrade |= AUTOTRADE_ENABLED;
	if (smode == SUSPEND_MODE_OFFLINE)
		sd->state.autotrade |= AUTOTRADE_OFFLINE;
	else if (smode == SUSPEND_MODE_AFK)
		sd->state.autotrade |= AUTOTRADE_AFK;

	if (Sql_Query(mmysql_handle, "INSERT INTO `%s`(`account_id`, `char_id`, `sex`, `map`, `x`, `y`, `body_direction`, `head_direction`, `sit`, `mode`, `tick`, `val1`, `val2`, `val3`, `val4`) "
		"VALUES( %d, %d, '%c', '%s', %d, %d, '%d', '%d', '%d', '%hu', '%" PRtf "', '%ld', '%ld', '%ld', '%ld' );",
		suspend_table, sd->status.account_id, sd->status.char_id, sd->status.sex == SEX_FEMALE ? 'F' : 'M', map_getmapdata(sd->bl.m)->name,
		sd->bl.x, sd->bl.y, sd->ud.dir, sd->head_dir, pc_issit(sd), int(smode), gettick(), val[0], val[1], val[2], val[3]) != SQL_SUCCESS) {
		Sql_ShowDebug(mmysql_handle);
	}

	channel_pcquit(sd, 0xF); //leave all chan
	clif_authfail_fd(sd->fd, 15);

	chrif_save(sd, CSAVE_AUTOTRADE);
}

//************************************
// Method:      suspend_deactive
// Description: 
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
// Description: 
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
// Description: 
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
// Description: 
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 22:57
//************************************
void do_final_suspend(void) {
	suspender_db->destroy(suspender_db, suspend_suspender_free);
}

//************************************
// Method:      do_init_suspend
// Description: 
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/4/5 22:58
//************************************
void do_init_suspend(void) {
	suspender_db = uidb_alloc(DB_OPT_BASE);
}

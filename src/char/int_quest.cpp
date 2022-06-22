// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "int_quest.hpp"

#include <stdlib.h>
#include <string.h>

#include "../common/malloc.hpp"
#include "../common/mmo.hpp"
#include "../common/socket.hpp"
#include "../common/sql.hpp"
#include "../common/strlib.hpp"

#include "char.hpp"
#include "inter.hpp"

enum e_quest_bound_type : uint8;

/**
 * Loads the entire questlog for a character.
 *
 * @param char_id Character ID
 * @param count   Pointer to return the number of found entries.
 * @return Array of found entries. It has *count entries, and it is care of the
 *         caller to aFree() it afterwards.
 */
struct quest *mapif_quests_fromsql(uint32 char_id, int *count) {
	struct quest *questlog = NULL;
	struct quest tmp_quest;
	SqlStmt *stmt;

	if( !count )
		return NULL;

	stmt = SqlStmt_Malloc(sql_handle);
	if( stmt == NULL ) {
		SqlStmt_ShowDebug(stmt);
		*count = 0;
		return NULL;
	}

	memset(&tmp_quest, 0, sizeof(struct quest));

	if( SQL_ERROR == SqlStmt_Prepare(stmt, "SELECT `quest_id`, `state`, `time`, `count1`, `count2`, `count3` FROM `%s` WHERE `char_id`=? ", schema_config.quest_db)
	||	SQL_ERROR == SqlStmt_BindParam(stmt, 0, SQLDT_INT, &char_id, 0)
	||	SQL_ERROR == SqlStmt_Execute(stmt)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 0, SQLDT_INT,  &tmp_quest.quest_id, 0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 1, SQLDT_INT,  &tmp_quest.state,    0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 2, SQLDT_UINT, &tmp_quest.time,     0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 3, SQLDT_INT,  &tmp_quest.count[0], 0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 4, SQLDT_INT,  &tmp_quest.count[1], 0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 5, SQLDT_INT,  &tmp_quest.count[2], 0, NULL, NULL)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		*count = 0;
		return NULL;
	}

	*count = (int)SqlStmt_NumRows(stmt);
	if( *count > 0 ) {
		int i = 0;

		questlog = (struct quest *)aCalloc(*count, sizeof(struct quest));
		while( SQL_SUCCESS == SqlStmt_NextRow(stmt) ) {
			if( i >= *count ) //Sanity check, should never happen
				break;
			memcpy(&questlog[i++], &tmp_quest, sizeof(tmp_quest));
		}
		if( i < *count ) {
			//Should never happen. Compact array
			*count = i;
			questlog = (struct quest *)aRealloc(questlog, sizeof(struct quest) * i);
		}
	}

	SqlStmt_Free(stmt);
	return questlog;
}

/**
 * Loads the entire questlog for account.
 *
 * @param account_id Account ID
 * @param count   Pointer to return the number of found entries.
 * @return Array of found entries. It has *count entries, and it is care of the
 *         caller to aFree() it afterwards.
 */
struct quest* mapif_account_quests_fromsql(uint32 char_id, int* count) {
	struct quest* questlog = NULL;
	struct quest tmp_quest;
	SqlStmt* stmt;
	int account_id = char_get_account_id(char_id);

	if (!count)
		return NULL;


	stmt = SqlStmt_Malloc(sql_handle);
	if (stmt == NULL) {
		SqlStmt_ShowDebug(stmt);
		*count = 0;
		return NULL;
	}

	memset(&tmp_quest, 0, sizeof(struct quest));


	if (SQL_ERROR == SqlStmt_Prepare(stmt, "SELECT `quest_id`, `state`, `time`, `count1`, `count2`, `count3` FROM `%s` WHERE `account_id`=?", schema_config.quest_db_acc)
		|| SQL_ERROR == SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, 0)
		|| SQL_ERROR == SqlStmt_Execute(stmt)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 0, SQLDT_INT, &tmp_quest.quest_id, 0, NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 1, SQLDT_INT, &tmp_quest.state, 0, NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 2, SQLDT_UINT, &tmp_quest.time, 0, NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 3, SQLDT_INT, &tmp_quest.count[0], 0, NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 4, SQLDT_INT, &tmp_quest.count[1], 0, NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 5, SQLDT_INT, &tmp_quest.count[2], 0, NULL, NULL)
		) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		*count = 0;
		return NULL;
	}

	*count = (int)SqlStmt_NumRows(stmt);
	if (*count > 0) {
		int i = 0;

		questlog = (struct quest*)aCalloc(*count, sizeof(struct quest));
		while (SQL_SUCCESS == SqlStmt_NextRow(stmt)) {
			if (i >= *count) //Sanity check, should never happen
				break;
			tmp_quest.quest_bound = QUEST_ACCOUNT;
			memcpy(&questlog[i++], &tmp_quest, sizeof(tmp_quest));
		}
		if (i < *count) {
			//Should never happen. Compact array
			*count = i;
			questlog = (struct quest*)aRealloc(questlog, sizeof(struct quest) * i);
		}
	}

	SqlStmt_Free(stmt);
	return questlog;
}

/**
 * Deletes a quest from a character's questlog.
 *
 * @param char_id  Character ID
 * @param quest_id Quest ID
 * @return false in case of errors, true otherwise
 */
bool mapif_quest_delete(uint32 char_id, int quest_id) {

	int account_id = char_get_account_id(char_id);
	bool no_error = true;
	
	//If quest's type was QUEST_CHAR but now it's QUEST_ACCOUNT, 'double delete' is useful
	if (SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `quest_id` = '%d' AND `char_id` = '%d'", schema_config.quest_db, quest_id, char_id))
	{
		Sql_ShowDebug(sql_handle);
		no_error = false;
	}
	

	if (SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `quest_id` = '%d' AND `account_id` = '%d'", schema_config.quest_db_acc, quest_id, account_id))
	{
		Sql_ShowDebug(sql_handle);
		no_error = false;
	}

	
	return no_error;
}

/**
 * Adds a quest to a character's questlog.
 *
 * @param char_id Character ID
 * @param qd      Quest data
 * @return false in case of errors, true otherwise
 */
bool mapif_quest_add(uint32 char_id, struct quest qd) {

	int account_id = char_get_account_id(char_id);
	bool no_error = true;

	switch (qd.quest_bound)
	{
	case QUEST_ACCOUNT:
		if (SQL_ERROR == Sql_Query(sql_handle, "INSERT INTO `%s`(`quest_id`, `account_id`, `state`, `time`, `count1`, `count2`, `count3`) VALUES ('%d', '%d', '%d', '%u', '%d', '%d', '%d')", schema_config.quest_db_acc, qd.quest_id, account_id, qd.state, qd.time, qd.count[0], qd.count[1], qd.count[2]))
		{
			Sql_ShowDebug(sql_handle);
			no_error = false;
		}
		break;
	case QUEST_CHAR:
		if (SQL_ERROR == Sql_Query(sql_handle, "INSERT INTO `%s`(`quest_id`, `char_id`, `state`, `time`, `count1`, `count2`, `count3`) VALUES ('%d', '%d', '%d', '%u', '%d', '%d', '%d')", schema_config.quest_db, qd.quest_id, char_id, qd.state, qd.time, qd.count[0], qd.count[1], qd.count[2]))
		{
			Sql_ShowDebug(sql_handle);
			no_error = false;
		}
		break;
	default:
		no_error = false;
	}

	return no_error;
}

/**
 * Updates a quest in a character's questlog.
 *
 * @param char_id Character ID
 * @param qd      Quest data
 * @return false in case of errors, true otherwise
 */
bool mapif_quest_update(uint32 char_id, struct quest qd) {

	int account_id = char_get_account_id(char_id);
	bool no_error = true;

	switch (qd.quest_bound)
	{
	case QUEST_ACCOUNT:
		if (SQL_ERROR == Sql_Query(sql_handle, "UPDATE `%s` SET `state`='%d', `count1`='%d', `count2`='%d', `count3`='%d' WHERE `quest_id` = '%d' AND `account_id` = '%d'", schema_config.quest_db_acc, qd.state, qd.count[0], qd.count[1], qd.count[2], qd.quest_id, account_id))
		{
			Sql_ShowDebug(sql_handle);
			no_error = false;
		}
		break;
	case QUEST_CHAR:
		if (SQL_ERROR == Sql_Query(sql_handle, "UPDATE `%s` SET `state`='%d', `count1`='%d', `count2`='%d', `count3`='%d' WHERE `quest_id` = '%d' AND `char_id` = '%d'", schema_config.quest_db, qd.state, qd.count[0], qd.count[1], qd.count[2], qd.quest_id, char_id))
		{
			Sql_ShowDebug(sql_handle);
			no_error = false;
		}
		break;
	default:
		no_error = false;
	}

	return true;
}

/**
 * Handles the save request from mapserver for a character's questlog.
 *
 * Received quests are saved, and an ack is sent back to the map server.
 *
 * @see inter_parse_frommap
 */
int mapif_parse_quest_save(int fd) {
	int i, j, k, old_n,old_n2,total_old, new_n = (RFIFOW(fd,2) - 8) / sizeof(struct quest);
	uint32 char_id = RFIFOL(fd,4);
	struct quest *old_qd = NULL,*old_qd_acc = NULL,*tmp_qd = NULL, *new_qd = NULL;
	bool success = true;

	if( new_n > 0 )
		new_qd = (struct quest*)RFIFOP(fd,8);

	old_qd = mapif_quests_fromsql(char_id, &old_n);
	old_qd_acc = mapif_account_quests_fromsql(char_id, &old_n2);

	total_old = old_n + old_n2;

	if (total_old > 0)
	{
		tmp_qd = (struct quest*)aCalloc(total_old, sizeof(struct quest));

		if (old_n > 0)
		{
			i = 0;
			while (i < old_n)
			{
				memcpy(&tmp_qd[i], &old_qd[i], sizeof(struct quest));
				i++;
			}
		}
		if (old_n2 > 0)
		{
			i = 0;
			while (i < old_n2)
			{
				memcpy(&tmp_qd[old_n + i], &old_qd_acc[i], sizeof(struct quest));
				i++;
			}
		}
	}

	for( i = 0; i < new_n; i++ ) {
		ARR_FIND(0, total_old, j, new_qd[i].quest_id == tmp_qd[j].quest_id);
		if( j < total_old) { //Update existing quests
			//Only states and counts are changable.
			ARR_FIND(0, MAX_QUEST_OBJECTIVES, k, new_qd[i].count[k] != tmp_qd[j].count[k]);
			if( k != MAX_QUEST_OBJECTIVES || new_qd[i].state != tmp_qd[j].state )
				success &= mapif_quest_update(char_id, new_qd[i]);

			if( j < (--total_old) ) {
				//Compact array
				memmove(&tmp_qd[j], &tmp_qd[j + 1], sizeof(struct quest) * (total_old - j));
				memset(&tmp_qd[total_old], 0, sizeof(struct quest));
			}
		} else //Add new quests
			success &= mapif_quest_add(char_id, new_qd[i]);
	}

	for( i = 0; i < total_old; i++ ) //Quests not in new_qd but in old_qd are to be erased.
		success &= mapif_quest_delete(char_id, tmp_qd[i].quest_id);

	if( old_qd )
		aFree(old_qd);
	if (old_qd_acc)
		aFree(old_qd_acc);
	if (tmp_qd)
		aFree(tmp_qd);

	//Send ack
	WFIFOHEAD(fd,7);
	WFIFOW(fd,0) = 0x3861;
	WFIFOL(fd,2) = char_id;
	WFIFOB(fd,6) = success ? 1 : 0;
	WFIFOSET(fd,7);

	return 0;
}

/**
 * Sends questlog to the map server
 *
 * NOTE: Completed quests (state == Q_COMPLETE) are guaranteed to be sent last
 * and the map server relies on this behavior (once the first Q_COMPLETE quest,
 * all of them are considered to be Q_COMPLETE)
 *
 * @see inter_parse_frommap
 */
int mapif_parse_quest_load(int fd) {
	uint32 char_id = RFIFOL(fd,2);
	struct quest *tmp_questlog = NULL;
	struct quest* tmp_account_questlog = NULL;
	struct quest* questlog = NULL;
	int num_quests;
	int num_quests2;
	int total_quests;

	tmp_questlog = mapif_quests_fromsql(char_id, &num_quests);
	tmp_account_questlog = mapif_account_quests_fromsql(char_id, &num_quests2);

	total_quests = num_quests + num_quests2;

	if(total_quests > 0)
	{
		questlog = (struct quest*)aCalloc(total_quests, sizeof(struct quest));

		if(num_quests > 0)
		{
			int i = 0;
			while (i < num_quests)
			{
				memcpy(&questlog[i], &tmp_questlog[i], sizeof(struct quest));
				i++;
			}
		}
		if (num_quests2 > 0)
		{
			int i = 0;
			while (i < num_quests2)
			{
				memcpy(&questlog[num_quests+i], &tmp_account_questlog[i], sizeof(struct quest));
				i++;
			}
		}
	}

	WFIFOHEAD(fd, total_quests * sizeof(struct quest) + 8);
	WFIFOW(fd,0) = 0x3860;
	WFIFOW(fd,2) = total_quests * sizeof(struct quest) + 8;
	WFIFOL(fd,4) = char_id;

	if(total_quests > 0 )
		memcpy(WFIFOP(fd,8), questlog, sizeof(struct quest) * total_quests);

	WFIFOSET(fd, total_quests * sizeof(struct quest) + 8);

	if( tmp_questlog )
		aFree(tmp_questlog);
	if (tmp_account_questlog)
		aFree(tmp_account_questlog);
	if (questlog)
		aFree(questlog);

	return 0;
}

/**
 * Parses questlog related packets from the map server.
 *
 * @see inter_parse_frommap
 */
int inter_quest_parse_frommap(int fd) {
	switch(RFIFOW(fd,0)) {
		case 0x3060: mapif_parse_quest_load(fd); break;
		case 0x3061: mapif_parse_quest_save(fd); break;
		default:
			return 0;
	}
	return 1;
}

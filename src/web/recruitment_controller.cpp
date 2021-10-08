// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "recruitment_controller.hpp"

#include <string>

#include "../common/showmsg.hpp"
#include "../common/sql.hpp"
#include "../common/strlib.hpp"

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "web.hpp"

using namespace nlohmann;

#define RECRUITMENT_PAGESIZE 10

HANDLER_FUNC(party_recruitment_add) {
	// TODO: 尝试过 -1、0、2、3 都不是中介所创建失败的返回值
	// 但是客户端中在 msgstringtable 确实有创建失败的文本信息
	// 还需要继续探索或者分析正确的返回值是多少

	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
	REQUIRE_FIELD_EXISTS_STRICT("CharName");
	REQUIRE_FIELD_EXISTS_STRICT("Memo");
	REQUIRE_FIELD_EXISTS_STRICT("MinLV");
	REQUIRE_FIELD_EXISTS_STRICT("MaxLV");
	REQUIRE_FIELD_EXISTS_STRICT("Tanker");
	REQUIRE_FIELD_EXISTS_STRICT("Healer");
	REQUIRE_FIELD_EXISTS_STRICT("Dealer");
	REQUIRE_FIELD_EXISTS_STRICT("Assist");
	REQUIRE_FIELD_EXISTS_STRICT("Type");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto char_name = GET_STRING_FIELD("CharName", "");
	auto memo = GET_STRING_FIELD("Memo", "");
	auto min_level = GET_NUMBER_FIELD("MinLV", 0);
	auto max_level = GET_NUMBER_FIELD("MaxLV", 0);
	auto tanker = GET_NUMBER_FIELD("Tanker", 0);
	auto healer = GET_NUMBER_FIELD("Healer", 0);
	auto dealer = GET_NUMBER_FIELD("Dealer", 0);
	auto assist = GET_NUMBER_FIELD("Assist", 0);
	auto adventure_type = GET_NUMBER_FIELD("Type", 0);

	if (!isVaildCharacter(account_id, char_id)) {
		response_json(res, 400, 3, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	// TODO: 将表结构检查统一提取到某个地方去

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `account_id` FROM `%s` WHERE (`account_id` = ? AND `char_id` = ? AND `world_name` = ?) LIMIT 1",
		recruitment_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &char_id, sizeof(char_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"REPLACE INTO `%s` (`account_id`, `char_id`, `char_name`, `world_name`, `adventure_type`, "
		"`tanker`, `dealer`, `healer`, `assist`, `min_level`, `max_level`, `memo`"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
		recruitment_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &char_id, sizeof(char_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void*)char_name.c_str(), strlen(char_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_INT, &adventure_type, sizeof(adventure_type))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 5, SQLDT_INT, &tanker, sizeof(tanker))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 6, SQLDT_INT, &dealer, sizeof(dealer))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 7, SQLDT_INT, &healer, sizeof(healer))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 8, SQLDT_INT, &assist, sizeof(assist))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 9, SQLDT_INT, &min_level, sizeof(min_level))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 10, SQLDT_INT, &max_level, sizeof(max_level))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 11, SQLDT_STRING, (void*)memo.c_str(), strlen(memo.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		response_json(res, 502, 3, "An error occurred while inserting data.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	SqlStmt_Free(stmt);
	weblock.unlock();

	response_json(res, 200, 1);
}

HANDLER_FUNC(party_recruitment_del) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
	REQUIRE_FIELD_EXISTS_STRICT("MasterAID");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto leader_account_id = GET_NUMBER_FIELD("MasterAID", 0);

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"DELETE FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
		recruitment_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	response_json(res, 200, 1);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(party_recruitment_get) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");

	if (!isVaildCharacter(account_id, char_id)) {
		response_json(res, 400, 3, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	// 用来获取自己发布的内容, 返回值的 Type 字段固定为 1
	// 2021年10月6日测试结论表示：无法通过控制 Type 的值来决定是否展现队长的信息登记入口

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `account_id`, `char_id`, `char_name`, `world_name`, `adventure_type`, "
		"`tanker`, `dealer`, `healer`, `assist`, `min_level`, `max_level`, `memo` "
		"FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
		recruitment_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		response_json(res, 200, 1);
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	s_recruitment p;

	if (SQL_ERROR == SqlStmt_BindColumn(stmt, 0, SQLDT_INT, &p.account_id, sizeof(p.account_id), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 1, SQLDT_INT, &p.char_id, sizeof(p.char_id), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 2, SQLDT_STRING, &p.char_name, sizeof(p.char_name), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 3, SQLDT_STRING, &p.world_name, sizeof(p.world_name), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 4, SQLDT_UINT8, &p.adventure_type, sizeof(p.adventure_type), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 5, SQLDT_UINT8, &p.tanker, sizeof(p.tanker), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 6, SQLDT_UINT8, &p.dealer, sizeof(p.dealer), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 7, SQLDT_UINT8, &p.healer, sizeof(p.healer), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 8, SQLDT_UINT8, &p.assist, sizeof(p.assist), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 9, SQLDT_UINT32, &p.min_level, sizeof(p.min_level), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 10, SQLDT_UINT32, &p.max_level, sizeof(p.max_level), NULL, NULL)
		|| SQL_ERROR == SqlStmt_BindColumn(stmt, 11, SQLDT_STRING, &p.memo, sizeof(p.memo), NULL, NULL)
		|| SQL_ERROR == SqlStmt_NextRow(stmt)
		) {
		response_json(res, 200, 1);	// TODO: 能否返回某种错误
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	safestrncpy(p.char_name, A2UWE(p.char_name).c_str(), NAME_LENGTH);
	safestrncpy(p.world_name, A2UWE(p.world_name).c_str(), 32);
	safestrncpy(p.memo, A2UWE(p.memo).c_str(), 32);

	json response = {
		{"Type", 1},
		{"data", {
			{"AID", p.account_id},
			{"GID", p.char_id},
			{"CharName", p.char_name},
			{"WorldName", p.world_name},
			{"Tanker", p.tanker},
			{"Dealer", p.dealer},
			{"Healer", p.healer},
			{"Assist", p.assist},
			{"MinLV", p.min_level},
			{"MaxLV", p.max_level},
			{"Type", p.adventure_type},
			{"Memo", p.memo},
		}}
	};

	response_json(res, 200, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(party_recruitment_list) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
	REQUIRE_FIELD_EXISTS_STRICT("page");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto page = GET_NUMBER_FIELD("page", 1);

	if (!isVaildCharacter(account_id, char_id)) {
		response_json(res, 400, 3, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != Sql_Query(handle,
		"SELECT COUNT(*) FROM `%s` "
		"WHERE `account_id` != %d AND `world_name` = '%s'",
		recruitment_table, account_id, world_name.c_str()) ||
		SQL_SUCCESS != Sql_NextRow(handle)) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	char* data = nullptr;
	Sql_GetData(handle, 0, &data, NULL);
	int record_cnt = std::atoi(data);
	int max_page = (int)ceil((double)record_cnt / RECRUITMENT_PAGESIZE);
	Sql_FreeResult(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `account_id`, `char_id`, `char_name`, `world_name`, `adventure_type`, "
		"`tanker`, `dealer`, `healer`, `assist`, `min_level`, `max_level`, `memo` "
		"FROM `%s` WHERE (`account_id` != ? AND `world_name` = ?) LIMIT %d, %d",
		recruitment_table, (page - 1) * RECRUITMENT_PAGESIZE, RECRUITMENT_PAGESIZE)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	s_recruitment p;

	SqlStmt_BindColumn(stmt, 0, SQLDT_INT, &p.account_id, sizeof(p.account_id), NULL, NULL);
	SqlStmt_BindColumn(stmt, 1, SQLDT_INT, &p.char_id, sizeof(p.char_id), NULL, NULL);
	SqlStmt_BindColumn(stmt, 2, SQLDT_STRING, &p.char_name, sizeof(p.char_name), NULL, NULL);
	SqlStmt_BindColumn(stmt, 3, SQLDT_STRING, &p.world_name, sizeof(p.world_name), NULL, NULL);
	SqlStmt_BindColumn(stmt, 4, SQLDT_UINT8, &p.adventure_type, sizeof(p.adventure_type), NULL, NULL);
	SqlStmt_BindColumn(stmt, 5, SQLDT_UINT8, &p.tanker, sizeof(p.tanker), NULL, NULL);
	SqlStmt_BindColumn(stmt, 6, SQLDT_UINT8, &p.dealer, sizeof(p.dealer), NULL, NULL);
	SqlStmt_BindColumn(stmt, 7, SQLDT_UINT8, &p.healer, sizeof(p.healer), NULL, NULL);
	SqlStmt_BindColumn(stmt, 8, SQLDT_UINT8, &p.assist, sizeof(p.assist), NULL, NULL);
	SqlStmt_BindColumn(stmt, 9, SQLDT_UINT32, &p.min_level, sizeof(p.min_level), NULL, NULL);
	SqlStmt_BindColumn(stmt, 10, SQLDT_UINT32, &p.max_level, sizeof(p.max_level), NULL, NULL);
	SqlStmt_BindColumn(stmt, 11, SQLDT_STRING, &p.memo, sizeof(p.memo), NULL, NULL);

	json response;
	response["Type"] = 1;

	if (record_cnt) {
		response["totalPage"] = max_page;
		response["data"] = json::array();

		while (SQL_SUCCESS == SqlStmt_NextRow(stmt)) {
			safestrncpy(p.char_name, A2UWE(p.char_name).c_str(), NAME_LENGTH);
			safestrncpy(p.world_name, A2UWE(p.world_name).c_str(), 32);
			safestrncpy(p.memo, A2UWE(p.memo).c_str(), 32);

			json record = {
				{"AID", p.account_id},
				{"GID", p.char_id},
				{"CharName", p.char_name},
				{"WorldName", p.world_name},
				{"Tanker", p.tanker},
				{"Dealer", p.dealer},
				{"Healer", p.healer},
				{"Assist", p.assist},
				{"MinLV", p.min_level},
				{"MaxLV", p.max_level},
				{"Type", p.adventure_type},
				{"Memo", p.memo},
			};
			response["data"].push_back(record);
		}
	}

	response_json(res, 200, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(party_recruitment_search) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
//	REQUIRE_FIELD_EXISTS_STRICT("Memo");	// 玩家不选则客户端不传, 不做校验
	REQUIRE_FIELD_EXISTS_STRICT("MinLV");
	REQUIRE_FIELD_EXISTS_STRICT("MaxLV");
// 	REQUIRE_FIELD_EXISTS_STRICT("Tanker");	// 玩家不选则客户端不传, 不做校验
// 	REQUIRE_FIELD_EXISTS_STRICT("Healer");	// 玩家不选则客户端不传, 不做校验
// 	REQUIRE_FIELD_EXISTS_STRICT("Dealer");	// 玩家不选则客户端不传, 不做校验
// 	REQUIRE_FIELD_EXISTS_STRICT("Assist");	// 玩家不选则客户端不传, 不做校验
//	REQUIRE_FIELD_EXISTS_STRICT("Type");	// 玩家不选则客户端不传, 不做校验
	REQUIRE_FIELD_EXISTS_STRICT("page");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto keyword = GET_STRING_FIELD("Memo", "");
	auto min_level = GET_NUMBER_FIELD("MinLV", 0);
	auto max_level = GET_NUMBER_FIELD("MaxLV", 0);
	auto tanker = GET_NUMBER_FIELD("Tanker", 0);
	auto healer = GET_NUMBER_FIELD("Healer", 0);
	auto dealer = GET_NUMBER_FIELD("Dealer", 0);
	auto assist = GET_NUMBER_FIELD("Assist", 0);
	auto adventure_type = GET_NUMBER_FIELD("Type", 0);
	auto page = GET_NUMBER_FIELD("page", 1);

	if (!isVaildCharacter(account_id, char_id)) {
		response_json(res, 400, 3, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	std::string sqlcmd = "SELECT COUNT(*) FROM `%s` "
		"WHERE `min_level` <= %d AND `max_level` >= %d AND `account_id` != %d ";
	if (adventure_type) {
		char buf[128] = { 0 };
		sprintf(buf, " AND `adventure_type` = %d ", adventure_type);
		sqlcmd += buf;
	}
	if (tanker) sqlcmd += " AND `tanker` = 1 ";
	if (healer) sqlcmd += " AND `healer` = 1 ";
	if (dealer) sqlcmd += " AND `dealer` = 1 ";
	if (assist) sqlcmd += " AND `assist` = 1 ";
	if (keyword.length()) sqlcmd += "AND (`char_name` LIKE '%%%s%%' OR `memo` LIKE '%%%s%%')";

	if (SQL_SUCCESS != Sql_Query(handle, sqlcmd.c_str(), recruitment_table,
		min_level, max_level, account_id, keyword.c_str(), keyword.c_str())
		|| SQL_SUCCESS != Sql_NextRow(handle)) {
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	char* data = nullptr;
	Sql_GetData(handle, 0, &data, NULL);
	int record_cnt = std::atoi(data);
	int max_page = (int)ceil((double)record_cnt / RECRUITMENT_PAGESIZE);
	Sql_FreeResult(handle);

	sqlcmd = "SELECT `account_id`, `char_id`, `char_name`, `world_name`, `adventure_type`, "
		"`tanker`, `dealer`, `healer`, `assist`, `min_level`, `max_level`, `memo` "
		"FROM `%s` WHERE `world_name` = ? AND `min_level` <= %d AND `max_level` >= %d AND `account_id` != %d ";
	if (adventure_type) {
		char buf[128] = { 0 };
		sprintf(buf, " AND `adventure_type` = %d ", adventure_type);
		sqlcmd += buf;
	}
	if (tanker) sqlcmd += " AND `tanker` = 1 ";
	if (healer) sqlcmd += " AND `healer` = 1 ";
	if (dealer) sqlcmd += " AND `dealer` = 1 ";
	if (assist) sqlcmd += " AND `assist` = 1 ";
	if (keyword.length()) sqlcmd += "AND (`char_name` LIKE '%%%s%%' OR `memo` LIKE '%%%s%%')";
	sqlcmd += "LIMIT %d, %d";

	if (keyword.length()) {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt, sqlcmd.c_str(),
			recruitment_table, min_level, max_level, account_id,
			keyword.c_str(), keyword.c_str(), // 若关键字不为空, 则而外需要多两个参数
			(page - 1) * RECRUITMENT_PAGESIZE, RECRUITMENT_PAGESIZE)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
			) {
			response_json(res, 502, 3, "There is an exception in the database table structure.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}
	else {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt, sqlcmd.c_str(),
			recruitment_table, min_level, max_level, account_id,
			page - 1, RECRUITMENT_PAGESIZE)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
			) {
			response_json(res, 502, 3, "There is an exception in the database table structure.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		response_json(res, 200, 1);
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	s_recruitment p;

	SqlStmt_BindColumn(stmt, 0, SQLDT_INT, &p.account_id, sizeof(p.account_id), NULL, NULL);
	SqlStmt_BindColumn(stmt, 1, SQLDT_INT, &p.char_id, sizeof(p.char_id), NULL, NULL);
	SqlStmt_BindColumn(stmt, 2, SQLDT_STRING, &p.char_name, sizeof(p.char_name), NULL, NULL);
	SqlStmt_BindColumn(stmt, 3, SQLDT_STRING, &p.world_name, sizeof(p.world_name), NULL, NULL);
	SqlStmt_BindColumn(stmt, 4, SQLDT_UINT8, &p.adventure_type, sizeof(p.adventure_type), NULL, NULL);
	SqlStmt_BindColumn(stmt, 5, SQLDT_UINT8, &p.tanker, sizeof(p.tanker), NULL, NULL);
	SqlStmt_BindColumn(stmt, 6, SQLDT_UINT8, &p.dealer, sizeof(p.dealer), NULL, NULL);
	SqlStmt_BindColumn(stmt, 7, SQLDT_UINT8, &p.healer, sizeof(p.healer), NULL, NULL);
	SqlStmt_BindColumn(stmt, 8, SQLDT_UINT8, &p.assist, sizeof(p.assist), NULL, NULL);
	SqlStmt_BindColumn(stmt, 9, SQLDT_UINT32, &p.min_level, sizeof(p.min_level), NULL, NULL);
	SqlStmt_BindColumn(stmt, 10, SQLDT_UINT32, &p.max_level, sizeof(p.max_level), NULL, NULL);
	SqlStmt_BindColumn(stmt, 11, SQLDT_STRING, &p.memo, sizeof(p.memo), NULL, NULL);

	json response;
	response["Type"] = 1;

	if (record_cnt) {
		response["totalPage"] = max_page;
		response["data"] = json::array();

		while (SQL_SUCCESS == SqlStmt_NextRow(stmt)) {
			safestrncpy(p.char_name, A2UWE(p.char_name).c_str(), NAME_LENGTH);
			safestrncpy(p.world_name, A2UWE(p.world_name).c_str(), 32);
			safestrncpy(p.memo, A2UWE(p.memo).c_str(), 32);

			json record = {
				{"AID", p.account_id},
				{"GID", p.char_id},
				{"CharName", p.char_name},
				{"WorldName", p.world_name},
				{"Tanker", p.tanker},
				{"Dealer", p.dealer},
				{"Healer", p.healer},
				{"Assist", p.assist},
				{"MinLV", p.min_level},
				{"MaxLV", p.max_level},
				{"Type", p.adventure_type},
				{"Memo", p.memo},
			};
			response["data"].push_back(record);
		}
	}

	response_json(res, 200, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "merchantstore_controller.hpp"

#include <string>

#include "../common/showmsg.hpp"
#include "../common/sql.hpp"

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "web.hpp"

using namespace nlohmann;

#define SUCCESS_RET 1
#define FAILURE_RET 3
#define REQUIRE_FIELD_EXISTS(x) REQUIRE_FIELD_EXISTS_T(x)

HANDLER_FUNC(merchantstore_save) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("Type");
	REQUIRE_FIELD_EXISTS("data");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto store_type = GET_NUMBER_FIELD("Type", 0);
	auto data = GET_STRING_FIELD("data", "");

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, 3, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `account_id` FROM `%s` WHERE (`account_id` = ? AND `char_id` = ? AND `world_name` = ? AND `store_type` = ?) LIMIT 1",
		merchant_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &char_id, sizeof(char_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_INT, &store_type, sizeof(store_type))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"INSERT INTO `%s` (`account_id`, `char_id`, `world_name`, `store_type`, `data`) VALUES (?, ?, ?, ?, ?)",
			merchant_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &char_id, sizeof(char_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_INT, &store_type, sizeof(store_type))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_STRING, (void*)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
			) {
			make_response(res, FAILURE_RET, "An error occurred while inserting data.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}
	else {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"UPDATE `%s` SET `data` = ? WHERE (`account_id` = ? AND `char_id` = ? AND `world_name` = ? AND `store_type` = ?)",
			merchant_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_INT, &char_id, sizeof(char_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_INT, &store_type, sizeof(store_type))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
			) {
			make_response(res, FAILURE_RET, "An error occurred while updating data.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}

	make_response(res, SUCCESS_RET);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(merchantstore_load) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("Type");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto store_type = GET_NUMBER_FIELD("Type", 0);

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist in the account.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `data` FROM `%s` WHERE (`account_id` = ? AND `char_id` = ? AND `world_name` = ? AND `store_type` = ?) LIMIT 1",
		merchant_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &char_id, sizeof(char_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_INT, &store_type, sizeof(store_type))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		make_response(res, SUCCESS_RET);
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	char databuf[10000] = { 0 };

	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_STRING, &databuf, sizeof(databuf), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
		) {
		make_response(res, FAILURE_RET, "An error occurred while binding column.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	databuf[sizeof(databuf) - 1] = 0;

	json response = {};
	response = json::parse(A2UWE(databuf));
	response["Type"] = SUCCESS_RET;
	make_response(res, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

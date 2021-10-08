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

HANDLER_FUNC(merchantstore_save) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
	REQUIRE_FIELD_EXISTS_STRICT("Type");
	REQUIRE_FIELD_EXISTS_STRICT("data");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto store_type = GET_NUMBER_FIELD("Type", 0);
	auto data = GET_STRING_FIELD("data", "");

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
		response_json(res, 502, 3, "There is an exception in the database table structure.");
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
			response_json(res, 502, 3, "An error occurred while inserting data.");
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
			response_json(res, 502, 3, "An error occurred while updating data.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}

	response_json(res, 200, 1);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(merchantstore_load) {
	if (!isAuthorized(req, false)) {
		response_json(res, 403, 3, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS_STRICT("AID");
	REQUIRE_FIELD_EXISTS_STRICT("GID");
	REQUIRE_FIELD_EXISTS_STRICT("WorldName");
	REQUIRE_FIELD_EXISTS_STRICT("Type");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto store_type = GET_NUMBER_FIELD("Type", 0);

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
		response_json(res, 502, 3, "There is an exception in the database table structure.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		response_json(res, 200, 1);
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	char databuf[10000] = { 0 };

	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_STRING, &databuf, sizeof(databuf), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
		) {
		response_json(res, 502, 3, "Could not load the data from database.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	databuf[sizeof(databuf) - 1] = 0;

	json response = {};
	response = json::parse(A2UWE(databuf));
	response["Type"] = 1;
	response_json(res, 200, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

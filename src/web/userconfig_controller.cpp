// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "userconfig_controller.hpp"

#include <string>

#include "../common/showmsg.hpp"
#include "../common/sql.hpp"

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "webutils.hpp"
#include "web.hpp"

#ifndef Pandas_WebServer_Rewrite_Controller_HandlerFunc

HANDLER_FUNC(userconfig_save) {
	if (!isAuthorized(req, false)) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}
	
	auto account_id = std::stoi(req.get_file_value("AID").content);
	auto world_name_str = req.get_file_value("WorldName").content;
	auto world_name = world_name_str.c_str();
	std::string data;

	if (req.has_file("data")) {
		data = req.get_file_value("data").content;
		addToJsonObject(data, "\"Type\": 1");
	} else {
		data = "{\"Type\": 1}";
	}
	SQLLock sl(WEB_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt * stmt = SqlStmt_Malloc(handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"SELECT `account_id` FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
			user_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
				"INSERT INTO `%s` (`account_id`, `world_name`, `data`) VALUES (?, ?, ?)",
				user_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			SqlStmt_ShowDebug(stmt);
			SqlStmt_Free(stmt);
			sl.unlock();
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
	} else {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
				"UPDATE `%s` SET `data` = ? WHERE (`account_id` = ? AND `world_name` = ?)",
				user_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)world_name, strlen(world_name))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			SqlStmt_ShowDebug(stmt);
			SqlStmt_Free(stmt);
			sl.unlock();
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
	}

	SqlStmt_Free(stmt);
	sl.unlock();
	res.set_content(data, "application/json");
}

HANDLER_FUNC(userconfig_load) {
	if (!req.has_file("AID") || !req.has_file("WorldName")) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	// TODO: Figure out when client sends AuthToken for this path, then add packetver check
	// if (!isAuthorized(req)) {
		// ShowError("Not authorized!\n");
		// message.reply(web::http::status_codes::Forbidden);
		// return;
	// }

	auto account_id = std::stoi(req.get_file_value("AID").content);
	auto world_name_str = req.get_file_value("WorldName").content;
	auto world_name = world_name_str.c_str();

	SQLLock sl(WEB_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt * stmt = SqlStmt_Malloc(handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"SELECT `data` FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
			user_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		SqlStmt_Free(stmt);
		ShowDebug("[AccountID: %d, World: \"%s\"] Not found in table, sending new info.\n", account_id, world_name);
		sl.unlock();
		res.set_content("{\"Type\": 1}", "application/json");
		return;
	}

	char databuf[SQL_BUFFER_SIZE];

	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_STRING, &databuf, sizeof(databuf), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	SqlStmt_Free(stmt);
	sl.unlock();

	databuf[sizeof(databuf) - 1] = 0;
	res.set_content(databuf, "application/json");
}

#else

using namespace nlohmann;

#define SUCCESS_RET 1
#define FAILURE_RET 3
#define REQUIRE_FIELD_EXISTS(x) REQUIRE_FIELD_EXISTS_T(x)

HANDLER_FUNC(userconfig_save) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("data");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto data = GET_STRING_FIELD("data", "");

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `data` FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
		user_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	// 客户端只会回传被修改过的那部分数据, 没有修改过的不会发送给服务端
	// 完整的数据是服务端记录的内容 + 本次客户端回传的内容, 因此需要先读取服务端保存的内容然后再进行合并存档

	char databuf[SQL_BUFFER_SIZE] = { 0 };

	if (SQL_SUCCESS == SqlStmt_BindColumn(stmt, 0, SQLDT_STRING, &databuf, sizeof(databuf), NULL, NULL)
		&& SQL_SUCCESS == SqlStmt_NextRow(stmt)
		) {
		databuf[sizeof(databuf) - 1] = 0;

		// 能执行到这里说明数据库中已经有 data 数据, 并且已经被成功读取到 databuf 缓冲区
		// 接下来以 databuf 的内容为基础, 用 data 的内容作为补丁进行合并

		if (strlen(databuf) != 0) {
			json data_from_db = json::parse(A2UWE(databuf));
			json data_from_client = json::parse(req.get_file_value("data").content);

			// 此处需要允许 null 节点对数据进行覆盖
			// 否则当玩家在客户端中重置快捷键设置的时候, 会发送上来一个全部都为 null 的节点,
			// 此时服务端忽略掉它们, 会导致玩家下次登录还会发现快捷键还在
			data_from_db.merge_patch_v2(data_from_client, true);
			data = U2AWE(data_from_db.dump(3));
		}
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"INSERT INTO `%s` (`account_id`, `world_name`, `data`) VALUES (?, ?, ?)",
			user_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			make_response(res, FAILURE_RET, "An error occurred while inserting data.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	} else {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"UPDATE `%s` SET `data` = ? WHERE (`account_id` = ? AND `world_name` = ?)",
			user_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			make_response(res, FAILURE_RET, "An error occurred while updating data.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
	}

	make_response(res, SUCCESS_RET);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

HANDLER_FUNC(userconfig_load) {
	if (!req.has_file("AID") || !req.has_file("WorldName")) {
		make_response(res, FAILURE_RET, "Missing necessary parameters to process the request.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("WorldName");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");

	if (!isVaildAccount(account_id)) {
		make_response(res, FAILURE_RET, "The account specified by the \"AID\" does not exist.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `data` FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
		user_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		//ShowDebug("[AccountID: %d, World: \"%s\"] Not found in table, sending new info.\n", account_id, U2ACE(req.get_file_value("WorldName").content).c_str());
		make_response(res, SUCCESS_RET);
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	char databuf[SQL_BUFFER_SIZE] = { 0 };

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

#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

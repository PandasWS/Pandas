// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "charconfig_controller.hpp"

#include <string>

#include "../common/showmsg.hpp"
#include "../common/sql.hpp"

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "web.hpp"

HANDLER_FUNC(charconfig_save) {
	if (!isAuthorized(req, false)) {
		res.status = 400;
		res.set_content("Error", "text/plain");
		return;
	}
	
	auto account_id = std::stoi(req.get_file_value("AID").content);
	auto world_name_str = req.get_file_value("WorldName").content;
	auto world_name = world_name_str.c_str();
	std::string data;

	if (req.has_file("data")) {
		data = req.get_file_value("data").content;
	} else {
		data = "{\"Type\": 1}";
	}
	SQLLock sl(WEB_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt * stmt = SqlStmt_Malloc(handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"SELECT `account_id` FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?) LIMIT 1",
			char_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = 400;
		res.set_content("Error", "text/plain");
		return;
	}

	if (SqlStmt_NumRows(stmt) <= 0) {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
				"INSERT INTO `%s` (`account_id`, `world_name`, `data`) VALUES (?, ?, ?)",
				char_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			SqlStmt_ShowDebug(stmt);
			SqlStmt_Free(stmt);
			sl.unlock();
			res.status = 400;
			res.set_content("Error", "text/plain");
			return;
		}
	} else {
		if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
				"UPDATE `%s` SET `data` = ? WHERE (`account_id` = ? AND `world_name` = ?)",
				char_configs_table)
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void *)data.c_str(), strlen(data.c_str()))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_INT, &account_id, sizeof(account_id))
			|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, (void *)world_name, strlen(world_name))
			|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
			SqlStmt_ShowDebug(stmt);
			SqlStmt_Free(stmt);
			sl.unlock();
			res.status = 400;
			res.set_content("Error", "text/plain");
			return;
		}
	}

	SqlStmt_Free(stmt);
	sl.unlock();
	res.set_content(data, "application/json");
}

HANDLER_FUNC(charconfig_load) {
	if (!req.has_file("AID") || !req.has_file("WorldName")) {
		res.status = 400;
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
			char_configs_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name, strlen(world_name))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = 400;
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

	char databuf[10000];

	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_STRING, &databuf, sizeof(databuf), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
	) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		sl.unlock();
		res.status = 400;
		res.set_content("Error", "text/plain");
		return;
	}

	SqlStmt_Free(stmt);
	sl.unlock();

	databuf[sizeof(databuf) - 1] = 0;
	res.set_content(databuf, "application/json");
}

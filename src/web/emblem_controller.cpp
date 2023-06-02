﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "emblem_controller.hpp"

#include <fstream>
#include <iostream>
#include <ostream>

#include <common/showmsg.hpp>
#include <common/socket.hpp>

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "web.hpp"

// Max size is 50kb for gif
#define MAX_EMBLEM_SIZE 50000
#define START_VERSION 1

#ifndef Pandas_WebServer_Rewrite_Controller_HandlerFunc

HANDLER_FUNC(emblem_download) {
	if (!isAuthorized(req, false)) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	bool fail = false;
	if (!req.has_file("GDID")) {
		ShowDebug("Missing GuildID field for emblem download.\n");
		fail = true;
	}
	if (!req.has_file("WorldName")) {
		ShowDebug("Missing WorldName field for emblem download.\n");
		fail = true;
	}
	if (fail) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	auto world_name_str = req.get_file_value("WorldName").content;
	auto world_name = world_name_str.c_str();
	auto guild_id = std::stoi(req.get_file_value("GDID").content);

	SQLLock sl(WEB_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt * stmt = SqlStmt_Malloc(handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"SELECT `version`, `file_type`, `file_data` FROM `%s` WHERE (`guild_id` = ? AND `world_name` = ?)",
			guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &guild_id, sizeof(guild_id))
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

	uint32 version = 0;
	char filetype[256];
	char blob[MAX_EMBLEM_SIZE]; // yikes
	uint32 emblem_size;

	if (SqlStmt_NumRows(stmt) <= 0) {
		SqlStmt_Free(stmt);
		ShowError("[GuildID: %d / World: \"%s\"] Not found in table\n", guild_id, world_name);
		sl.unlock();
		res.status = HTTP_NOT_FOUND;
		res.set_content("Error", "text/plain");
		return;
	}


	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &version, sizeof(version), nullptr, nullptr)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 1, SQLDT_STRING, &filetype, sizeof(filetype), nullptr, nullptr)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 2, SQLDT_BLOB, &blob, MAX_EMBLEM_SIZE, &emblem_size, nullptr)
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

	if (emblem_size > MAX_EMBLEM_SIZE) {
		ShowDebug("Emblem is too big, current size is %d and the max length is %d.\n", emblem_size, MAX_EMBLEM_SIZE);
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}
	const char * content_type;
	if (!strcmp(filetype, "BMP"))
		content_type = "image/bmp";
	else if (!strcmp(filetype, "GIF"))
		content_type = "image/gif";
	else {
		ShowError("Invalid image type %s, rejecting!\n", filetype);
		res.status = HTTP_NOT_FOUND;
		res.set_content("Error", "text/plain");
		return;
	}

	res.body.assign(blob, emblem_size);
	res.set_header("Content-Type", content_type);
}


HANDLER_FUNC(emblem_upload) {
	if (!isAuthorized(req, true)) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	bool fail = false;
	if (!req.has_file("GDID")) {
		ShowDebug("Missing GuildID field for emblem upload.\n");
		fail = true;
	}
	if (!req.has_file("WorldName")) {
		ShowDebug("Missing WorldName field for emblem upload.\n");
		fail = true;
	}
	if (!req.has_file("Img")) {
		ShowDebug("Missing Img field for emblem upload.\n");
		fail = true;
	}
	if (!req.has_file("ImgType")) {
		ShowDebug("Missing ImgType for emblem upload.\n");
		fail = true;
	}
	if (fail) {
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	auto world_name_str = req.get_file_value("WorldName").content;
	auto world_name = world_name_str.c_str();
	auto guild_id = std::stoi(req.get_file_value("GDID").content);
	auto imgtype_str = req.get_file_value("ImgType").content;
	auto imgtype = imgtype_str.c_str();
	auto img = req.get_file_value("Img").content;
	auto img_cstr = img.c_str();
	
	if (imgtype_str != "BMP" && imgtype_str != "GIF") {
		ShowError("Invalid image type %s, rejecting!\n", imgtype);
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	auto length = img.length();
	if (length > MAX_EMBLEM_SIZE) {
		ShowDebug("Emblem is too big, current size is %lu and the max length is %d.\n", length, MAX_EMBLEM_SIZE);
		res.status = HTTP_BAD_REQUEST;
		res.set_content("Error", "text/plain");
		return;
	}

	if (imgtype_str == "GIF") {
		if (!web_config.allow_gifs) {
			ShowDebug("Client sent GIF image but GIF image support is disabled.\n");
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
		if (img.substr(0, 3) != "GIF") {
			ShowDebug("Server received ImgType GIF but received file does not start with \"GIF\" magic header.\n");
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
		// TODO: transparency check for GIF emblems
	}
	else if (imgtype_str == "BMP") {
		if (length < 14) {
			ShowDebug("File size is too short\n");
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
		if (img.substr(0, 2) != "BM") {
			ShowDebug("Server received ImgType BMP but received file does not start with \"BM\" magic header.\n");
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
		if (RBUFL(img_cstr, 2) != length) {
			ShowDebug("Bitmap size doesn't match size in file header.\n");
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}

		if (inter_config.emblem_transparency_limit < 100) {
			uint32 offset = RBUFL(img_cstr, 0x0A);
			int i, transcount = 1, tmp[3];
			for (i = offset; i < length - 1; i++) {
				int j = i % 3;
				tmp[j] = RBUFL(img_cstr, i);
				if (j == 2 && (tmp[0] == 0xFFFF00FF) && (tmp[1] == 0xFFFF00) && (tmp[2] == 0xFF00FFFF)) //if pixel is transparent
					transcount++;
			}
			if (((transcount * 300) / (length - offset)) > inter_config.emblem_transparency_limit) {
				ShowDebug("Bitmap transparency check failed.\n");
				res.status = HTTP_BAD_REQUEST;
				res.set_content("Error", "text/plain");
				return;
			}
		}
	}

	SQLLock sl(WEB_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt * stmt = SqlStmt_Malloc(handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
			"SELECT `version` FROM `%s` WHERE (`guild_id` = ? AND `world_name` = ?)",
			guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &guild_id, sizeof(guild_id))
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

	uint32 version = START_VERSION;

	if (SqlStmt_NumRows(stmt) > 0) {
		if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &version, sizeof(version), NULL, NULL)
			|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
		) {
			SqlStmt_ShowDebug(stmt);
			SqlStmt_Free(stmt);
			sl.unlock();
			res.status = HTTP_BAD_REQUEST;
			res.set_content("Error", "text/plain");
			return;
		}
		version += 1;
	}

	// insert new
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"REPLACE INTO `%s` (`version`, `file_type`, `guild_id`, `world_name`, `file_data`) VALUES (?, ?, ?, ?, ?)",
		guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_UINT32, &version, sizeof(version))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)imgtype, strlen(imgtype))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_INT, &guild_id, sizeof(guild_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, (void *)world_name, strlen(world_name))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_BLOB, (void *)img.c_str(), length)
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
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

	std::ostringstream stream;
	stream << "{\"Type\":1,\"version\":" << version << "}";
	res.set_content(stream.str(), "application/json");
}

#else

using namespace nlohmann;

#define SUCCESS_RET 1
#define FAILURE_RET 4
#define REQUIRE_FIELD_EXISTS(x) REQUIRE_FIELD_EXISTS_T(x)

HANDLER_FUNC(emblem_download) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("GDID");
	REQUIRE_FIELD_EXISTS("Version");
	REQUIRE_FIELD_EXISTS("WorldName");

	auto guild_id = GET_NUMBER_FIELD("GDID", 0);
	auto guild_emblem_version = GET_NUMBER_FIELD("Version", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `version`, `file_type`, `file_data` FROM `%s` WHERE (`guild_id` = ? AND `world_name` = ?)",
		guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &guild_id, sizeof(guild_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	uint32 version = 0;
	char filetype[32] = { 0 };
	char* blob = new char[MAX_EMBLEM_SIZE];
	memset(blob, 0, MAX_EMBLEM_SIZE);

	// 基于 blob 构建一个智能指针, 当智能指针被析构时自动释放 blob 申请的内存区域
	std::shared_ptr<char> __blob = std::shared_ptr<char>(blob, [](char* p)->void {delete[] p; });

	uint32 emblem_size = 0;

	if (SqlStmt_NumRows(stmt) <= 0) {
		ShowError("[GuildID: %d / World: \"%s\"] Not found in table\n", guild_id, U2ACE(req.get_file_value("WorldName").content).c_str());
		make_response(res, FAILURE_RET, "An error occurred while binding column.");
		RETURN_STMT_SUCCESS(stmt, weblock);
	}

	if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &version, sizeof(version), nullptr, nullptr)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 1, SQLDT_STRING, &filetype, sizeof(filetype), nullptr, nullptr)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 2, SQLDT_BLOB, blob, MAX_EMBLEM_SIZE, &emblem_size, nullptr)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
	) {
		make_response(res, FAILURE_RET, "Could not load the emblem data from database.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	SqlStmt_Free(stmt);
	weblock.unlock();

	if (version != guild_emblem_version) {
		ShowDebug("Emblem version is not match, current version is %d and the version required by the client is %d.\n", version, guild_emblem_version);
		make_response(res, FAILURE_RET, "The requested emblem version does not match.");
		return;
	}

	if (emblem_size > MAX_EMBLEM_SIZE) {
		ShowDebug("Emblem is too big, current size is %d and the max length is %d.\n", emblem_size, MAX_EMBLEM_SIZE);
		make_response(res, FAILURE_RET, "Emblem is too big, reject.");
		return;
	}

	const char * content_type;
	if (!strcmp(filetype, "BMP"))
		content_type = "image/bmp";
	else if (!strcmp(filetype, "GIF"))
		content_type = "image/gif";
	else {
		ShowError("Invalid image type %s, rejecting!\n", filetype);
		make_response(res, FAILURE_RET, "Unsupported image type, reject.");
		return;
	}

	res.body.assign(blob, emblem_size);
	res.set_header("Content-Type", content_type);
}


HANDLER_FUNC(emblem_upload) {
	if (!isAuthorized(req, true)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("GDID");
	REQUIRE_FIELD_EXISTS("ImgType");
	REQUIRE_FIELD_EXISTS("Img");
	REQUIRE_FIELD_EXISTS("WorldName");

	auto guild_id = GET_NUMBER_FIELD("GDID", 0);
	auto imgtype = GET_STRING_FIELD("ImgType", "");
	auto img = GET_RAWSTR_FIELD("Img", "");
	auto world_name = GET_STRING_FIELD("WorldName", "");

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (imgtype != "BMP" && imgtype != "GIF") {
		ShowError("Invalid image type %s, rejecting!\n", imgtype.c_str());
		make_response(res, FAILURE_RET, "Unsupported image type, reject.");
		return;
	}

	auto length = img.length();
	if (length > MAX_EMBLEM_SIZE) {
		ShowDebug("Emblem is too big, current size is %lu and the max length is %d.\n", length, MAX_EMBLEM_SIZE);
		make_response(res, FAILURE_RET, "The size of the emblem exceeds the maximum limit.");
		return;
	}

	if (imgtype == "GIF") {
		if (!web_config.allow_gifs) {
			ShowDebug("Client sent GIF image but GIF image support is disabled.\n");
			make_response(res, FAILURE_RET, "GIF image support is disabled.");
			return;
		}
		if (img.substr(0, 3) != "GIF") {
			ShowDebug("Server received ImgType GIF but received file does not start with \"GIF\" magic header.\n");
			make_response(res, FAILURE_RET, "Server received ImgType GIF but received file does not start with \"GIF\" magic header.");
			return;
		}
	}
	else if (imgtype == "BMP") {
		if (length < 14) {
			ShowDebug("File size is too short\n");
			make_response(res, FAILURE_RET, "Bitmap file size is too short.");
			return;
		}
		if (img.substr(0, 2) != "BM") {
			ShowDebug("Server received ImgType BMP but received file does not start with \"BM\" magic header.\n");
			make_response(res, FAILURE_RET, "Server received ImgType BMP but received file does not start with \"BM\" magic header.");
			return;
		}
		if (RBUFL(img.c_str(), 2) != length) {
			ShowDebug("Bitmap size doesn't match size in file header.\n");
			make_response(res, FAILURE_RET, "Bitmap size doesn't match size in file header.");
			return;
		}

		if (inter_config.emblem_transparency_limit < 100) {
			uint32 offset = RBUFL(img.c_str(), 0x0A);
			int i, transcount = 1, tmp[3];
			for (i = offset; i < length - 1; i++) {
				int j = i % 3;
				tmp[j] = RBUFL(img.c_str(), i);
				if (j == 2 && (tmp[0] == 0xFFFF00FF) && (tmp[1] == 0xFFFF00) && (tmp[2] == 0xFF00FFFF)) //if pixel is transparent
					transcount++;
			}
			if (((transcount * 300) / (length - offset)) > inter_config.emblem_transparency_limit) {
				ShowDebug("Bitmap transparency check failed.\n");
				make_response(res, FAILURE_RET, "Bitmap transparency check failed.");
				return;
			}
		}
	}

	SQLLock weblock(WEB_SQL_LOCK);
	weblock.lock();
	auto handle = weblock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"SELECT `version` FROM `%s` WHERE (`guild_id` = ? AND `world_name` = ?)",
		guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &guild_id, sizeof(guild_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	uint32 version = START_VERSION;

	if (SqlStmt_NumRows(stmt) > 0) {
		if (SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &version, sizeof(version), NULL, NULL)
			|| SQL_SUCCESS != SqlStmt_NextRow(stmt)
		) {
			make_response(res, FAILURE_RET, "An error occurred while binding column.");
			RETURN_STMT_FAILURE(stmt, weblock);
		}
		version += 1;
	}

	// insert new
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"REPLACE INTO `%s` (`version`, `file_type`, `guild_id`, `world_name`, `file_data`) VALUES (?, ?, ?, ?, ?)",
		guild_emblems_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_UINT32, &version, sizeof(version))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void *)imgtype.c_str(), strlen(imgtype.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_INT, &guild_id, sizeof(guild_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, (void *)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_BLOB, (void *)img.c_str(), length)
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
	) {
		make_response(res, FAILURE_RET, "An error occurred while replaceing data.");
		RETURN_STMT_FAILURE(stmt, weblock);
	}

	json response = {};
	response["Type"] = SUCCESS_RET;
	response["version"] = version;
	make_response(res, response);
	RETURN_STMT_SUCCESS(stmt, weblock);
}

#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

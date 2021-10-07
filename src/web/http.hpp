// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef HTTP_HPP
#define HTTP_HPP


#include <httplib.h>

#ifdef WIN32
#include <Windows.h>
#endif

typedef httplib::Request Request;
typedef httplib::Response Response;

#define HANDLER_FUNC(x) void x (const Request &req, Response &res)
typedef HANDLER_FUNC((*handler_func));

// 读取请求中的数值型字段值, 若指定的字段不存在则返回 def_val 的值
#define GET_NUMBER_FIELD(field, def_val) \
	(req.has_file(field) ? std::stoi(req.get_file_value(field).content) : def_val)

// 读取请求中的字符串字段值, 若指定的字段不存在则返回 def_val 的值 (进行必要的转码)
#define GET_STRING_FIELD(field, def_val) \
	(req.has_file(field) ? U2AWE(req.get_file_value(field).content) : def_val)

// 读取请求中的字符串字段值, 若指定的字段不存在则返回 def_val 的值 (不进行转码)
#define GET_RAWSTR_FIELD(field, def_val) \
	(req.has_file(field) ? req.get_file_value(field).content : def_val)

// 严格要求指定字段存在, 若不存在则返回 400 状态码并给与 Type 赋值 3 以及附赠错误信息
#define REQUIRE_FIELD_EXISTS_STRICT(field) \
	if (!req.has_file(field)) { \
		response_json(res, 400, 3, "Sorry, missing '" ##field "' field for process request."); \
		return; \
	}

// 友好的要求指定字段存在, 若不存在则返回 200 状态码并给与 Type 赋值 1
#define REQUIRE_FIELD_EXISTS(field) \
	if (!req.has_file(field)) { \
		response_json(res, 200, 1); \
		return; \
	}

#endif

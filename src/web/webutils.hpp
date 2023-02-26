// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder


#ifndef WEB_UTILS_HPP
#define WEB_UTILS_HPP

#include "../config/pandas.hpp"	// Pandas_WebServer_Rewrite_Controller_HandlerFunc
#include "../common/utf8.hpp"	// PandasUtf8::utf8ToAnsi, PandasUtf8::ansiToUtf8

#include <string>

#ifndef Pandas_WebServer_Rewrite_Controller_HandlerFunc
#include <nlohmann/json_fwd.hpp>
#else
#include <nlohmann/json.hpp>
#include <httplib.h>

const size_t WORLD_NAME_LENGTH = 32;

void make_response(httplib::Response& res, int type, const std::string& errmes = "", int status_code = 200);
void make_response(httplib::Response& res, nlohmann::json& content, int status_code = 200);
#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

bool mergeData(nlohmann::json &orig, const nlohmann::json &patch, bool merge_null);

#endif

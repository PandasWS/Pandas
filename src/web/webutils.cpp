// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder


#include "webutils.hpp"
#include <string>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>


/**
 * Merge patch into orig recursively
 * if merge_null is true, this operates like json::merge_patch
 * if merge_null is false, then if patch has null, it does not override orig
 * Returns true on success
 */
bool mergeData(nlohmann::json &orig, const nlohmann::json &patch, bool merge_null) {
	if (!patch.is_object()) {
		// then it's a value
		if ((patch.is_null() && merge_null) || (!patch.is_null())) {
			orig = patch;
		}
		return true;
	}

	if (!orig.is_object()) {
		orig = nlohmann::json::object();
	}

	for (auto it = patch.begin(); it != patch.end(); ++it) {
		if (it.value().is_null()) {
			if (merge_null) {
				orig.erase(it.key());
			}
		} else {
			mergeData(orig[it.key()], it.value(), merge_null);
		}
	}
	return true;
}

#ifdef Pandas_WebServer_Rewrite_Controller_HandlerFunc
//************************************
// Method:      make_response
// Description: 构造标准响应对象并将它设置为接口响应内容
// Access:      public 
// Parameter:   httplib::Response & res
// Parameter:   int type (负数则使用 Result 作为字段名, 正数用 Type 作为字段名)
// Parameter:   const std::string & errmes
// Parameter:   int status_code
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/10/03 10:18
//************************************ 
void make_response(httplib::Response& res, int type, const std::string& errmes, int status_code) {
	nlohmann::json content = {};
	content[(type < 0 ? "Result" : "Type")] = abs(type);
	if (!errmes.empty()) {
		content["Error"] = errmes;
	}

	res.status = status_code;
	std::string content_str = content.dump(3);

	if (PandasUtf8::getEncodingByLanguage() == PandasUtf8::PANDAS_ENCODING_BIG5) {
		content_str = PandasUtf8::splashForUtf8(content_str);
	}

	res.set_content(content_str, "application/json");
}

//************************************
// Method:      make_response
// Description: 将给定的 JSON 内容对象设置为接口响应内容
// Access:      public 
// Parameter:   httplib::Response & res
// Parameter:   nlohmann::json & content
// Parameter:   int status_code
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/10/03 10:18
//************************************ 
void make_response(httplib::Response& res, nlohmann::json& content, int status_code) {
	res.status = status_code;
	std::string content_str = content.dump(3);

	if (PandasUtf8::getEncodingByLanguage() == PandasUtf8::PANDAS_ENCODING_BIG5) {
		content_str = PandasUtf8::splashForUtf8(content_str);
	}

	res.set_content(content_str, "application/json");
}
#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

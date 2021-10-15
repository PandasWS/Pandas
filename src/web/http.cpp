// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "http.hpp"

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
	res.set_content(content.dump(3), "application/json");
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
	res.set_content(content.dump(3), "application/json");
}
#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

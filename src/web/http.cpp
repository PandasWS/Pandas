// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "http.hpp"

#ifdef Pandas_WebServer_Rewrite_Controller_HandlerFunc
//************************************
// Method:      response_json
// Description: 构造标准响应 JSON 内容对象并将它设置为接口响应内容
// Access:      public 
// Parameter:   httplib::Response & res
// Parameter:   int status_code
// Parameter:   uint32 type
// Parameter:   const std::string & errmes
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/10/03 10:18
//************************************ 
void response_json(httplib::Response& res, int status_code, int type, const std::string& errmes) {
	nlohmann::json content = {};
	content["Type"] = type;
	if (!errmes.empty()) {
		content["Error"] = errmes;
	}
	res.status = status_code;
	res.set_content(content.dump(3), "application/json");
}

//************************************
// Method:      response_json
// Description: 将给定的 JSON 内容对象设置为接口响应内容
// Access:      public 
// Parameter:   httplib::Response & res
// Parameter:   int status_code
// Parameter:   nlohmann::json & content
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2021/10/03 10:18
//************************************ 
void response_json(httplib::Response& res, int status_code, nlohmann::json& content) {
	res.status = status_code;
	res.set_content(content.dump(3), "application/json");
}
#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

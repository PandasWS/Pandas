// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "translate.hpp"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <iconv.h>
	#include <errno.h>
	#include <string.h>

	#include <stdio.h>
	#include <locale.h>
	#include <langinfo.h>
#endif // _WIN32

#include <boost/regex.hpp>

TranslateDB translate_db;

//************************************
// Method:      getDefaultLocation
// Description: 获取 YAML 数据文件的具体路径
// Returns:     const std::string
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:32
//************************************
const std::string TranslateDB::getDefaultLocation() {
	std::string postfix;

	switch (PandasUtf8::systemLanguage) {
	case PandasUtf8::PANDAS_LANGUAGE_CHS: postfix = "cn"; break;
	case PandasUtf8::PANDAS_LANGUAGE_CHT: postfix = "tw"; break;
	case PandasUtf8::PANDAS_LANGUAGE_THA: postfix = "th"; break;
	}

	if (postfix.empty()) return "";

	std::string location = boost::str(
		boost::format("%1%/msg_conf/translation_%2%.yml") %
		conf_path % postfix
	);

	return location;
}

//************************************
// Method:      showStatus
// Description: 显示当前终端翻译系统的状态
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/25 20:58
//************************************
void TranslateDB::showStatus() {
	std::string szLocation = this->getCurrentFile();
	if (szLocation.empty()) {
		// 若系统语言是英文, 那么就不打印与终端翻译机制相关的信息
		if (PandasUtf8::systemLanguage == PandasUtf8::PANDAS_LANGUAGE_ENG) {
			return;
		}

		ShowInfo("Console translation system was deactivated.\n");
	}
	else {
		ShowInfo("Console translation initialized: " CL_WHITE "'%s'" CL_RESET ".\n", szLocation.c_str());
	}
}

//************************************
// Method:      parseBodyNode
// Description: 解析 Body 节点的主要处理函数
// Parameter:   const ryml::NodeRef & node
// Returns:     uint64
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:32
//************************************
uint64 TranslateDB::parseBodyNode(const ryml::NodeRef& node) {
	std::string original, translation;

	if (!this->asString(node, "Original", original)) {
		return 0;
	}

	auto item = this->find(original);
	bool exists = item != nullptr;

	if (!exists) {
		item = std::make_shared<s_translate_item>();
		item->original = original;
	}

	if (!this->asString(node, "Translation", translation)) {
		return 0;
	}

	item->translation = translation;

	this->parseTags(item->original);
	this->parseTags(item->translation);

	if (!exists) {
		this->put(item->original, item);
	}

	return 1;
}

void TranslateDB::parseTags(std::string& message) {
	if (message.find("[{") != std::string::npos) {
		// 处理特殊宏 EXPAND_AND_QUOTE 的展开
		if (message.find("EXPAND_AND_QUOTE") != std::string::npos) {
			// Step1. 获取要替换的常量名称
			boost::regex re(R"(\[{EXPAND_AND_QUOTE\(\s*(.*)\s*\)}\])", boost::regex::icase);
			boost::smatch match_result;

			if (boost::regex_search(message, match_result, re)) {
				std::string constant = match_result[1].str();

				// Step2. 获取常量对应的值
				std::string constant_val;
				for (auto it : this->m_quoteList) {
					if (it.name == constant) {
						constant_val = it.value;
						break;
					}
				}

				// Step3. 执行替换操作, 完成特殊流程处理
				if (!constant_val.empty()) {
					message = boost::regex_replace(message, re, constant_val);
				}
			}
		}

		// 进行常规的颜色代码等替换操作
		for (auto it : this->m_tagsList) {
			strReplace(message, it.name, it.value);
		}
	}
}

//************************************
// Method:      do_init_translate
// Description: 初始化终端翻译模块
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:32
//************************************
void do_init_translate() {
	translate_db.load();
}

//************************************
// Method:      translate_reload
// Description: 重新加载终端翻译数据库
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:32
//************************************
void translate_reload() {

}

//************************************
// Method:      do_final_translate
// Description: 释放终端翻译模块
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/03 20:32
//************************************
void do_final_translate() {
	translate_db.clear();
}

//************************************
// Method:      translate
// Description: 执行具体的字符串翻译转换操作
// Parameter:   std::string& original
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/14 21:08
//************************************
void translate(std::string& original) {
	auto it = translate_db.find(original);
	if (it != nullptr && !it->translation.empty()) {
		original = it->translation;
	}
}

//************************************
// Method:      translate
// Description: 执行具体的字符串翻译转换操作
// Parameter:   const char * original
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2020/01/23 23:47
//************************************
std::string translate(const char* original) {
	std::string message(original);
	translate(message);
	return message;
}

//************************************
// Method:      translate_status
// Description: 显示当前终端翻译系统的状态
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/01/25 21:21
//************************************
void translate_status() {
	translate_db.showStatus();
}

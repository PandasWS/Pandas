// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "map_artisan.hpp"

#include <cctype> // toupper, tolower
#include <algorithm> // transform

#include <boost/regex.hpp>

#include <common/strlib.hpp>
#include <common/nullpo.hpp>
#include <common/showmsg.hpp>
#include <common/assistant.hpp>

#include "map.hpp"
#include "pc.hpp"

// 参考资料: 
// https://www.zhaokeli.com/article/8289.html

//************************************
// Method:      hasCatchPet
// Description: 判断脚本是否拥有宠物捕捉指令, 并提取它支持捕捉的宠物编号
// Parameter:   const std::string& script
// Parameter:   std::vector<uint32> & pet_mobid
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/10/13 15:59
//************************************
bool hasCatchPet(const std::string& script, std::vector<uint32>& pet_mobid) {
	pet_mobid.clear();

	static const std::vector<std::string> valid_cmds = { "multicatchpet", "catchpet", "mpet", "pet" };
	static const std::string patterns = R"(.*?((multicatchpet|catchpet|mpet|pet)(\s{1,}|\()(\(|)(.*?)(|\))(\s*|);))";
	if (!strContain(valid_cmds, script)) return false;

	try
	{
		boost::regex re(patterns, boost::regex::icase);
		boost::smatch match_result;

		if (!boost::regex_search(script, match_result, re)) return false;
		if (match_result.size() != 8) return false;

		std::string cmd = boost::to_lower_copy(match_result[2].str());
		std::string params = strTrim(match_result[5].str());
		if (!params.length()) return false;

		std::vector<std::string> explode = strExplode(params, ',');

		if (cmd == "multicatchpet" || cmd == "mpet") {
			for (const std::string& it : explode) {
				pet_mobid.push_back(std::stoi(strTrim(it)));
			}
		}
		else {
			for (const std::string& it : explode) {
				pet_mobid.push_back(std::stoi(strTrim(it)));
				break; // 仅获取第一个魔物编号即可
			}
		}

		// 对 vector 的内容进行排序和消重 (unique 依赖 sort)
		std::sort(pet_mobid.begin(), pet_mobid.end());
		pet_mobid.erase(
			std::unique(pet_mobid.begin(), pet_mobid.end()),
			pet_mobid.end()
		);
	}
	catch (const boost::regex_error& e)
	{
		ShowWarning("%s throw regex_error : %s\n", __func__, e.what());
		return false;
	}

	return true;
}

//************************************
// Method:      hasCallfunc
// Description: 判断一个脚本是否调用了 callfunc 指令
// Parameter:   const std::string& script
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2019/10/13 15:59
//************************************
bool hasCallfunc(const std::string& script) {
	if (!strContain("callfunc", script)) return false;
	return isRegexMatched(script, R"(.*?callfunc(\s.*|\(\s*)\"(.*?)\".*?)");
}


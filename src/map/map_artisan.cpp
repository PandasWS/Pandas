// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "map_artisan.hpp"

#include <cctype>	// toupper, tolower
#include <algorithm>	// transform

#include "../common/strlib.hpp"
#include "../common/nullpo.hpp"
#include "../../3rdparty/pcre/include/pcre.h"

#include "map.hpp"
#include "pc.hpp"

// 参考资料: 
// https://www.cnblogs.com/LiuYanYGZ/p/5903946.html

//************************************
// Method:		regexGroupVal
// Description:	使用正则表达式进行匹配并获取特定分组中的内容(以字符串形式)
// Parameter:	std::string patterns	匹配表达式
// Parameter:	std::string content		需要被匹配的来源字符内容
// Parameter:	int groupid				需要读取的分组编号 (分组不存在则失败)
// Parameter:	std::string & val		用于保存匹配后分组内容的引用变量
// Returns:		bool					成功与否
//************************************
bool regexGroupVal(std::string patterns, std::string content, int groupid, std::string & val) {
	pcre *re = NULL;
	pcre_extra *extra = NULL;
	const char *error = NULL;
	int erroffset = -1, r = PCRE_ERROR_NOMATCH, ovector_len = 30, ovector[30] = { 0 };

	do 
	{
		re = pcre_compile(patterns.c_str(), 0, &error, &erroffset, NULL);
		if (!re) break;

		extra = pcre_study(re, 0, &error);
		if (!extra) break;

		r = pcre_exec(re, extra, content.c_str(), content.length(), 0, 0, ovector, ovector_len);

		if (r != PCRE_ERROR_NOMATCH) {
			const char *substring_start = content.c_str() + ovector[2 * groupid];
			int substring_length = ovector[2 * groupid + 1] - ovector[2 * groupid];
			char* matched = new char[substring_length + 1];

			memset(matched, 0, substring_length + 1);
			strncpy(matched, substring_start, substring_length);
			val = std::string(matched);

			delete[] matched;
		}
	} while (false);

	if (!re) pcre_free(re);
	if (!extra) pcre_free(extra);
	return (r != PCRE_ERROR_NOMATCH);
}

//************************************
// Method:		regexMatch
// Description:	使用正则表达式进行匹配, 看是否可以匹配成功
// Parameter:	std::string patterns	匹配表达式
// Parameter:	std::string content		需要被匹配的来源字符内容
// Returns:		bool					成功与否
//************************************
bool regexMatch(std::string patterns, std::string content) {
	pcre *re = NULL;
	pcre_extra *extra = NULL;
	const char *error = NULL;
	int erroffset = -1, r = PCRE_ERROR_NOMATCH, ovector_len = 30, ovector[30] = { 0 };

	do 
	{
		re = pcre_compile(patterns.c_str(), 0, &error, &erroffset, NULL);
		if (!re) break;

		extra = pcre_study(re, 0, &error);
		if (!extra) break;

		r = pcre_exec(re, extra, content.c_str(), content.length(), 0, 0, ovector, ovector_len);
	} while (false);

	if (!re) pcre_free(re);
	if (!extra) pcre_free(extra);
	return (r != PCRE_ERROR_NOMATCH);
}

//************************************
// Method:		hasPet
// Description:	判断道具使用脚本中是否存在 pet 指令, 若有则提取该道具可捕捉的魔物编号
// Parameter:	const char * script			道具的使用脚本
// Parameter:	unsigned int & pet_mobid	提取到的魔物编号
// Returns:		bool						是否成功提取
//************************************
bool hasPet(const char* _script, unsigned int & pet_mobid) {
	std::string patterns = std::string("(?i).*?pet(\\s*?|\\()(\\d{0,5}?)(\\)|\\s*?);.*?");	// Group 2 为可捕捉的魔物编号
	std::string script = std::string(_script);
	std::string val = std::string();

	// 将脚本内容全部转换成小写
	std::transform(
		script.begin(), script.end(), script.begin(),
		static_cast<int(*)(int)>(std::tolower)
	);

	// 先看看有没有包含 pet 字符串, 如果没有那么就不进行正则匹配
	// 以此来提高一点点速度和降低一些资源开销
	if (script.find("pet") == std::string::npos)
		return false;

	if (regexGroupVal(patterns, script, 2, val)) {
		pet_mobid = atoi(val.c_str());
		return true;
	}
	return false;
}

//************************************
// Method:		hasCallfunc
// Description:	判断道具使用脚本中是否存在 callfunc 指令
// Parameter:	const char * _script		道具的使用脚本
// Returns:		bool						是否存在
//************************************
bool hasCallfunc(const char* _script) {
	std::string patterns = std::string("(?i).*?callfunc(\\s.*|\\(\\s*)\"(.*?)\".*?");	// Group 2 为 callfunc 的目标函数名
	std::string script = std::string(_script);
	std::string val = std::string();

	// 将脚本内容全部转换成小写
	std::transform(
		script.begin(), script.end(), script.begin(),
		static_cast<int(*)(int)>(std::tolower)
	);

	// 先看看有没有包含 callfunc 字符串, 如果没有那么就不进行正则匹配
	// 以此来提高一点点速度和降低一些资源开销
	if (script.find("callfunc") == std::string::npos)
		return false;

	return regexMatch(patterns, script);
}

#ifdef Pandas_MapFlag_NoMail
//************************************
// Method:		nomail_artisan_sub
// Description:	注入点过多, 封装一下以便进行 nomail 效果的实现
// Parameter:	struct map_session_data * sd
// Returns:		bool 是否禁用邮件系统
//************************************
bool nomail_artisan_sub(struct map_session_data *sd) {
	nullpo_retr(false, sd);

	if (sd && map_getmapflag(sd->bl.m, MF_NOMAIL)) {
		clif_displaymessage(sd->fd, msg_txt_cn(sd, 95));
		return true;
	}

	return false;
}
#endif // Pandas_MapFlag_NoMail

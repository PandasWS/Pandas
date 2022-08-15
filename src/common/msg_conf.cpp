// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "msg_conf.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "malloc.hpp"
#include "showmsg.hpp"
#include "strlib.hpp"

#ifndef Pandas_Message_Conf
// 当禁用 Pandas_Message_Conf 的时候
// 能够显示出对应的警告信息出来, 告诉用户原因同时避免编译错误 [Sola丶小克]
const char* disabled_msg_txt(int msg_number) {
	ShowWarning("Program will return 'unknow' for msg_number : %d calling by 'msg_txt_cn' function, because the message conf improvements has been disabled.\n", msg_number);
	return "unknow";
}
#endif // Pandas_Message_Conf

/*
 * Return the message string of the specified number by [Yor]
 * (read in table msg_table, with specified lenght table in size)
 */
const char* _msg_txt(int msg_number,int size, char ** msg_table)
{
	if (msg_number >= 0 && msg_number < size &&
		msg_table[msg_number] != NULL && msg_table[msg_number][0] != '\0')
	return msg_table[msg_number];

	return "??";
}


/*
 * Read txt file and store them into msg_table
 */
int _msg_config_read(const char* cfgName,int size, char ** msg_table)
{
	uint16 msg_number, msg_count = 0, line_num = 0;
#ifndef Pandas_Adaptive_Importing_Message_Database
	char line[1024], w1[8], w2[512];
#else
	char line[1024] = { 0 }, w1[11] = { 0 }, w2[512] = { 0 };
#endif // Pandas_Adaptive_Importing_Message_Database
	FILE *fp;
	static int called = 1;

	if ((fp = fopen(cfgName, "r")) == NULL) {
		ShowError("Messages file not found: %s\n", cfgName);
		return -1;
	}

	if ((--called) == 0)
		memset(msg_table, 0, sizeof (msg_table[0]) * size);

	while (fgets(line, sizeof (line), fp)) {
		line_num++;
		if (line[0] == '/' && line[1] == '/')
			continue;
#ifndef Pandas_Adaptive_Importing_Message_Database
		if (sscanf(line, "%7[^:]: %511[^\r\n]", w1, w2) != 2)
			continue;
#else
		if (sscanf(line, "%10[^:]: %511[^\r\n]", w1, w2) != 2)
			continue;
#endif // Pandas_Adaptive_Importing_Message_Database

		if (strcmpi(w1, "import") == 0)
			_msg_config_read(w2,size,msg_table);
#ifdef Pandas_Adaptive_Importing_Message_Database
		else if (strcmpi(w1, "import_chs") == 0) {
			if (PandasUtf8::systemLanguage == PandasUtf8::PANDAS_LANGUAGE_CHS)
				_msg_config_read(w2, size, msg_table);
		}
		else if (strcmpi(w1, "import_cht") == 0) {
			if (PandasUtf8::systemLanguage == PandasUtf8::PANDAS_LANGUAGE_CHT)
				_msg_config_read(w2, size, msg_table);
		}
		else if (strcmpi(w1, "import_tha") == 0) {
			if (PandasUtf8::systemLanguage == PandasUtf8::PANDAS_LANGUAGE_THA)
				_msg_config_read(w2, size, msg_table);
		}
#endif // Pandas_Adaptive_Importing_Message_Database
		else {
			msg_number = atoi(w1);
#ifndef Pandas_LGTM_Optimization
			if (msg_number >= 0 && msg_number < size) {
#else
			// 这里的 msg_number 是一个无符号类型的数值, 所以它绝对不可能是一个负数.
			// 这里只需要判断闭区间即可: https://lgtm.com/rules/2165180573/
			if (msg_number < size) {
#endif // Pandas_LGTM_Optimization
				if (msg_table[msg_number] != NULL)
					aFree(msg_table[msg_number]);
				size_t len = strnlen(w2,sizeof(w2)) + 1;
				msg_table[msg_number] = (char *) aMalloc(len * sizeof (char));
				safestrncpy(msg_table[msg_number], w2, len);
				msg_count++;
			}
#ifdef Pandas_Message_Conf
			else
				ShowWarning("Invalid message ID '%s' at line %d from '%s' file.\n",w1,line_num,cfgName);
#else
			// 若没有启用 Pandas_Message_Conf 宏定义的话
			// 为了避免持续集成判定失败, 这里针对 >= ALL_EXTEND_FIRST_MSG 的 msg_number 降低报错等级
			else {
				if (msg_number < ALL_EXTEND_FIRST_MSG)
					ShowWarning("Invalid message ID '%s' at line %d from '%s' file.\n", w1, line_num, cfgName);
				else
					ShowInfo("Invalid message ID '%s' at line %d from '%s' file.\n", w1, line_num, cfgName);
			}
#endif // Pandas_Message_Conf
		}
	}

	fclose(fp);
	ShowInfo("Done reading " CL_WHITE "'%d'" CL_RESET " messages in " CL_WHITE "'%s'" CL_RESET ".\n",msg_count,cfgName);

	return 0;
}

/*
 * Destroy msg_table (freeup mem)
 */
void _do_final_msg(int size, char ** msg_table){
	int i;
	for (i = 0; i < size; i++)
		aFree(msg_table[i]);
}

#ifndef Pandas_Message_Reorganize
/*
 * lookup a langtype string into his associate langtype number
 * return -1 if not found
 */
int msg_langstr2langtype(char * langtype){
	int lang=-1;
	if (!strncmpi(langtype, "eng",2)) lang = 0;
	else if (!strncmpi(langtype, "rus",2)) lang = 1;
	else if (!strncmpi(langtype, "spn",2)) lang = 2;
	else if (!strncmpi(langtype, "grm",2)) lang = 3;
	else if (!strncmpi(langtype, "chn",2)) lang = 4;
	else if (!strncmpi(langtype, "mal",2)) lang = 5;
	else if (!strncmpi(langtype, "idn",2)) lang = 6;
	else if (!strncmpi(langtype, "frn",2)) lang = 7;
	else if (!strncmpi(langtype, "por",2)) lang = 8;
	else if (!strncmpi(langtype, "tha",2)) lang = 9;

	return lang;
}

/*
 * lookup a langtype into his associate lang string
 * return ?? if not found
 */
const char* msg_langtype2langstr(int langtype){
	switch(langtype){
		case 0: return "English (ENG)";
		case 1: return "Russkiy (RUS)"; //transliteration
		case 2: return "Espanol (SPN)";
		case 3: return "Deutsch (GRM)";
		case 4: return "Hanyu (CHN)"; //transliteration
		case 5: return "Bahasa Malaysia (MAL)";
		case 6: return "Bahasa Indonesia (IDN)";
		case 7: return "Francais (FRN)";
		case 8: return "Portugues Brasileiro (POR)";
		case 9: return "Thai (THA)";
		default: return "??";
	}
}
#else
/*
 * lookup a langtype string into his associate langtype number
 * return -1 if not found
 */
int msg_langstr2langtype(char* langtype) {
	int lang = -1;
	if (!strcmpi(langtype, "eng")) lang = 0;		// 英文
	else if (!strcmpi(langtype, "chs")) lang = 1;	// 简体中文
	else if (!strcmpi(langtype, "chn")) lang = 2;	// 繁体中文的别名
	else if (!strcmpi(langtype, "cht")) lang = 2;	// 繁体中文
	else if (!strcmpi(langtype, "tha")) lang = 3;	// Thai

	return lang;
}

/*
 * lookup a langtype into his associate lang string
 * return ?? if not found
 */
const char* msg_langtype2langstr(int langtype) {
	switch (langtype) {
	case 0: return "English (ENG)";						// 英文
	case 1: return "Chinese Simplified (CHS)";			// 简体中文
	case 2: return "Chinese Traditional (CHT)";			// 繁体中文
	case 3: return "Thai";								// Thai
	default: return "??";
	}
}
#endif // Pandas_Message_Reorganize

/*
 * verify that the choosen langtype is enable
 * return
 *  1 : langage enable
 * -1 : false range
 * -2 : disable
 */
int msg_checklangtype(int lang, bool display){
	uint16 test= (1<<(lang-1));
	if(!lang) return 1; //default english
	else if(lang < 0 || test > LANG_MAX) return -1; //false range
	else if (LANG_ENABLE&test) return 1;
	else if(display) {
		ShowDebug("Unsupported langtype '%d'.\n",lang);
	}
	return -2;
}

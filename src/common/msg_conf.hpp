// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef MSG_CONF_HPP
#define	MSG_CONF_HPP

#include "../config/core.hpp"

#ifndef Pandas_Message_Reorganize
enum lang_types {
	LANG_RUS = 0x01,
	LANG_SPN = 0x02,
	LANG_GRM = 0x04,
	LANG_CHN = 0x08,
	LANG_MAL = 0x10,
	LANG_IDN = 0x20,
	LANG_FRN = 0x40,
	LANG_POR = 0x80,
	LANG_THA = 0x100,
	LANG_MAX
};

#ifndef LANG_ENABLE
	// Multilanguage System.
	// Define which languages to enable (bitmask).
	// 0xFFF will enable all, while 0x000 will enable English only.
	#define LANG_ENABLE 0x000
#endif
#else
enum lang_types {
	LANG_CHS = 0x01,
	LANG_CHT = 0x02,
	LANG_THA = 0x04,
	LANG_MAX
};

#ifndef LANG_ENABLE
	// 除了英语, 我们额外启用两种语言, 分别是: 简体中文, 繁体中文 [Sola丶小克]
	#define LANG_ENABLE (LANG_CHS | LANG_CHT | LANG_THA)
#endif
#endif // Pandas_Message_Reorganize

// =================================================================================
// 追加一部分消息区间给 Pandas 扩展使用 [Sola丶小克]
//
// 截止 2018年11月20日 rAthena 默认的宏定义默认值如下:
// LOGIN_MAX_MSG 的值是 30、CHAR_MAX_MSG 的值是 300、MAP_MAX_MSG 的值是 1550
// 
// 为了能够拥有统一的区间段, Pandas 将会把他们的区段最大值都补充到 4000,
// 并从 3000 开始使用自定义的消息.
//
// 若某天 MAP_MAX_MSG 的值扩大到接近 3000 了, 那么消息区段得重新偏移
//
// 启用此机制后, 会在 login.hpp \ char.hpp \ map.hpp 提供一个 msg_txt_cn 的宏定义函数
// 通过 msg_txt_cn 传入的 msg_number 会自动加 ALL_EXTEND_FIRST_MSG 作为最终的 msg_number
// 并从 conf/msg_conf 中读取对应的消息, 作为返回值.
// =================================================================================
#define ALL_EXTEND_MSG 4000
#define ALL_EXTEND_FIRST_MSG 3000

#ifndef Pandas_Message_Conf
const char* disabled_msg_txt(int msg_number);
#endif // Pandas_Message_Conf
// =================================================================================

//read msg in table
const char* _msg_txt(int msg_number,int size, char ** msg_table);
//store msg from txtfile into msg_table
int _msg_config_read(const char* cfgName,int size, char ** msg_table);
//clear msg_table
void _do_final_msg(int size, char ** msg_table);
//Lookups
int msg_langstr2langtype(char * langtype);
const char* msg_langtype2langstr(int langtype);
// Verify that the choosen langtype is enabled.
int msg_checklangtype(int lang, bool display);

#endif	/* MSG_CONF_HPP */

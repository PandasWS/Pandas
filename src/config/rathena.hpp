// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_CONFIG_HPP_
#define _RATHENA_CN_CONFIG_HPP_

#include "renewal.hpp"
#include "packets.hpp"

#define rAthenaCN

#ifdef rAthenaCN
	#define rAthenaCN_Basic
	#define rAthenaCN_Creative_Work
	#define rAthenaCN_NpcEvent
#endif // rAthenaCN

// ============================================================================
// 基础组 - rAthenaCN_Basic
// ============================================================================

#ifdef rAthenaCN_Basic
	// 定义 rAthenaCN 的版本号
	#define rAthenaCN_Version "v1.8.0"

	// 在启动时显示 rAthenaCN 的 LOGO
	#define rAthenaCN_Show_Logo

	// 在启动时显示免责声明
	#define rAthenaCN_Disclaimer

	// 在启动时显示 rAthenaCN 的版本号
	#define rAthenaCN_Show_Version
#endif // rAthenaCN_Basic

// ============================================================================
// 原创功能组 - rAthenaCN_Creative_Work
// ============================================================================

#ifdef rAthenaCN_Creative_Work
	// 是否启用崩溃转储文件生成机制 [Sola丶小克]
	#ifdef _WIN32
		#define rAthenaCN_Crash_Report
	#endif // _WIN32

	// 扩展信息配置文件 (Msg_conf) 的 ID 最大上限,
	// 同时提供 msg_txt_cn 宏定义函数, 方便在工程中使用自定义信息 [Sola丶小克]
	#define rAthenaCN_Message_Conf

	// 是否支持在 map_athena.conf 中设定封包混淆密钥 [Sola丶小克]
	#ifdef PACKET_OBFUSCATION
		#define rAthenaCN_Support_Specify_PacketKeys
	#endif // PACKET_OBFUSCATION

	// 是否支持读取 UTF8-BOM 编码的配置文件 [Sola丶小克]
	#define rAthenaCN_Support_Read_UTF8BOM_Configure
#endif // rAthenaCN_Creative_Work

// ============================================================================
// NPC事件组 - rAthenaCN_NpcEvent
// ============================================================================

#ifdef rAthenaCN_NpcEvent
	/************************************************************************/
	/* Event 类型的标准事件，这些事件不能被 processhalt 打断                     */
	/************************************************************************/

	// 杀死 MVP 魔物时触发事件 - OnPCKillMvpEvent
	#define rAthenaCN_NpcEvent_KILLMVP
#endif // rAthenaCN_NpcEvent

#endif // _RATHENA_CN_CONFIG_HPP_

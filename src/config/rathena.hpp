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
	#define rAthenaCN_Mapflags
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

	// 是否启用数据库编码自动判定机制 [Sola丶小克]
	#define rAthenaCN_Smart_Codepage

	// 在使用 _M/_F 注册的时候, 能够限制使用中文等字符作为游戏账号 [Sola丶小克]
	// 这里的 PCRE_SUPPORT 在"项目属性 -> C/C++ -> 预处理器"中定义
	#ifdef PCRE_SUPPORT
		#define rAthenaCN_Strict_Userid_Verification
	#endif // PCRE_SUPPORT

	// 是否支持隐藏角色服务器的在线人数 [Sola丶小克]
	#define rAthenaCN_Support_Hide_Online_Players_Count

	// 是否支持禁止创建杜兰族角色 [Sola丶小克]
	#if PACKETVER >= 20151001
		#define rAthenaCN_Reject_Create_Doram_Character
	#endif // PACKETVER >= 20151001

	// 是否支持对战斗配置选项进行完整性检查 [╰づ记忆•斑驳〤 实现] [Sola丶小克 改进]
	#define rAthenaCN_Battle_Config_Verification

	// 是否扩展魔物名称能展现的信息, 比如体型、种族、属性 [Sola丶小克 改进]
	#define rAthenaCN_MobInfomation_Extend
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

// ============================================================================
// 地图标记组 - rAthenaCN_Mapflags
// ============================================================================

#ifdef rAthenaCN_Mapflags
	// 是否启用 mobinfo 地图标记 [Sola丶小克]
	// 该标记用于指定某地图的 show_mob_info 值, 以此控制该地图魔物名称的展现信息
	#define rAthenaCN_MapFlag_Mobinfo
#endif // rAthenaCN_Mapflags

#endif // _RATHENA_CN_CONFIG_HPP_

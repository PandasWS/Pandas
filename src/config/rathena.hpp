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
	#define rAthenaCN_Bugfix
	#define rAthenaCN_NpcEvent
	#define rAthenaCN_Mapflags
	#define rAthenaCN_AtCommands
	#define rAthenaCN_ScriptCommands
	#define rAthenaCN_ScriptResults
	#define rAthenaCN_ScriptParams
#endif // rAthenaCN

// ============================================================================
// 基础组 - rAthenaCN_Basic
// ============================================================================

#ifdef rAthenaCN_Basic
	// 定义 rAthenaCN 的版本号
	#define rAthenaCN_Version "v2.0.0"

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

	// 是否加强 2013-12-23 以及 2013-08-07 客户端的混淆密钥 [Sola丶小克]
	#define rAthenaCN_Packet_Obfuscation_Keys
#endif // rAthenaCN_Creative_Work

// ============================================================================
// 官方BUG修正组 - rAthenaCN_Bugfix
// ============================================================================

#ifdef rAthenaCN_Bugfix
	// 修正 rAthena 在 LGTM 上产生的部分警告 [Sola丶小克]
	#define rAthenaCN_Fix_LGTM_Warning

	// 用 mysql_set_character_set 来设置 MySQL 的编码字符集 [Sola丶小克]
	#define rAthenaCN_Fix_Mysql_SetEncoding

	// 修正在部分情况下角色公会图标刷新不及时的问题 [Sola丶小克]
	#define rAthenaCN_Fix_GuildEmblem_Update
#endif // rAthenaCN_Bugfix

// ============================================================================
// NPC事件组 - rAthenaCN_NpcEvent
// ============================================================================

#ifdef rAthenaCN_NpcEvent
	/************************************************************************/
	/* Filter 类型的过滤事件，这些事件可以被 processhalt 中断                    */
	/************************************************************************/
	// PYHELP - NPCEVENT - INSERT POINT - <Section 1>

	/************************************************************************/
	/* Event  类型的标准事件，这些事件不能被 processhalt 打断                    */
	/************************************************************************/

	// 当玩家杀死 MVP 魔物时触发事件 - OnPCKillMvpEvent [Sola丶小克]
	// 类型: Event 类型 / 常量名称: NPCE_KILLMVP / 变量名称: killmvp_event_name
	#define rAthenaCN_NpcEvent_KILLMVP
	// PYHELP - NPCEVENT - INSERT POINT - <Section 2>
#endif // rAthenaCN_NpcEvent

// ============================================================================
// 地图标记组 - rAthenaCN_Mapflags
// ============================================================================

#ifdef rAthenaCN_Mapflags
	// 是否启用 mobinfo 地图标记 [Sola丶小克]
	// 该标记用于指定某地图的 show_mob_info 值, 以此控制该地图魔物名称的展现信息
	#define rAthenaCN_MapFlag_Mobinfo

	// 是否启用 noautoloot 地图标记 [Sola丶小克]
	// 该标记用于在给定此标记的地图上禁止玩家使用自动拾取功能, 或使已激活的自动拾取功能失效
	#define rAthenaCN_MapFlag_NoAutoLoot

	// 是否启用 notoken 地图标记 [Sola丶小克]
	// 该标记用于禁止玩家在指定的地图上使用“原地复活之证”道具
	#define rAthenaCN_MapFlag_NoToken

	// 是否启用 nocapture 地图标记 [Sola丶小克]
	// 该标记用于禁止玩家在地图上使用宠物捕捉道具或贤者的"随机技能"来捕捉宠物
	#define rAthenaCN_MapFlag_NoCapture

	// 是否启用 hideguildinfo 地图标记 [Sola丶小克]
	// 使当前地图上的玩家无法见到其他人的公会图标、公会名称、职位等信息 (自己依然可见)
	#define rAthenaCN_MapFlag_HideGuildInfo

	// 是否启用 hidepartyinfo 地图标记 [Sola丶小克]
	// 使当前地图上的玩家无法见到其他人的队伍名称 (自己依然可见)
	#define rAthenaCN_MapFlag_HidePartyInfo

	// 是否启用 nomail 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_NoMail

	// 是否启用 nopet 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_NoPet

	// 是否启用 nohomun 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_NoHomun

	// 是否启用 nomerc 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_NoMerc

	// 是否启用 mobdroprate 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_MobDroprate

	// 是否启用 mvpdroprate 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define rAthenaCN_MapFlag_MvpDroprate
	// PYHELP - MAPFLAG - INSERT POINT - <Section 1>
#endif // rAthenaCN_Mapflags

// ============================================================================
// 管理员指令组 - rAthenaCN_AtCommands
// ============================================================================

#ifdef rAthenaCN_AtCommands
	// 是否启用 recallmap 管理员指令 [Sola丶小克]
	// 召唤当前(或指定)地图的玩家来到身边 (处于离线挂店模式的角色不会被召唤)
	#define rAthenaCN_AtCommand_RecallMap
#endif // rAthenaCN_AtCommands

// ============================================================================
// 脚本指令组 - rAthenaCN_ScriptCommands
// ============================================================================

#ifdef rAthenaCN_ScriptCommands
	// 是否启用 setheaddir 脚本指令 [Sola丶小克]
	// 用于调整角色纸娃娃脑袋的朝向 (0 - 正前方; 1 - 向右看; 2 - 向左看)
	#define rAthenaCN_ScriptCommand_SetHeadDir

	// 是否启用 setbodydir 脚本指令 [Sola丶小克]
	// 用于调整角色纸娃娃身体的朝向 (与 NPC 一致, 从 0 到 7 共 8 个方位可选择)
	#define rAthenaCN_ScriptCommand_SetBodyDir

	// 是否启用 openbank 脚本指令 [Sola丶小克]
	// 让指定的角色立刻打开银行界面 (只对拥有随身银行的客户端版本有效)
	#define rAthenaCN_ScriptCommand_OpenBank

	// 是否启用 instance_users 脚本指令 [Sola丶小克]
	// 获取指定的副本实例中已经进入副本地图的人数
	#define rAthenaCN_ScriptCommand_InstanceUsers

	// 是否启用 cap 脚本指令 [Sola丶小克]
	// 确保数值不低于给定的最小值, 不超过给定的最大值
	#define rAthenaCN_ScriptCommand_CapValue

	// 是否启用 mobremove 脚本指令 [Sola丶小克]
	// 根据 GID 移除一个魔物单位 (只是移除, 不会让魔物死亡)
	#define rAthenaCN_ScriptCommand_MobRemove

	// 是否启用 mesclear 脚本指令 [Sola丶小克]
	// 由于 rAthena 已经实现 clear 指令, 这里兼容老版本 mesclear 指令
	#define rAthenaCN_ScriptCommand_MesClear

	// 是否启用 battleignore 脚本指令 [Sola丶小克]
	// 将角色设置为魔物免战状态, 避免被魔物攻击 (与 GM 指令的 monsterignore 效果一致)
	#define rAthenaCN_ScriptCommand_BattleIgnore

	// 是否启用 gethotkey 脚本指令 [Sola丶小克]
	// 获取指定快捷键位置当前的信息 (该指令有一个用于兼容的别名: get_hotkey)
	#define rAthenaCN_ScriptCommand_GetHotkey

	// 是否启用 sethotkey 脚本指令 [Sola丶小克]
	// 设置指定快捷键位置的信息 (该指令有一个用于兼容的别名: set_hotkey)
	#define rAthenaCN_ScriptCommand_SetHotkey

	// 是否启用 showvend 脚本指令 [Jian916]
	// 使指定的 NPC 头上可以显示露天商店的招牌, 点击招牌可触发与 NPC 的对话
	#define rAthenaCN_ScriptCommand_ShowVend

	// 是否启用 viewequip 脚本指令 [Sola丶小克]
	// 使用该指令可以查看指定在线角色的装备面板信息 (注意: v2.0.0 以前是通过账号编号)
	#define rAthenaCN_ScriptCommand_ViewEquip

	// 是否启用 unequipidx 脚本指令 [维护者昵称]
	// 脱下指定背包序号的道具 (该指令有一个用于兼容的别名: unequipinventory)
	#define rAthenaCN_ScriptCommand_UnEquipIdx
	// PYHELP - SCRIPTCMD - INSERT POINT - <Section 1>
#endif // rAthenaCN_ScriptCommands

// ============================================================================
// 脚本返回值拓展组 - rAthenaCN_ScriptResults
// ============================================================================

#ifdef rAthenaCN_ScriptResults
	// 是否拓展 getinventorylist 脚本指令的返回数组 [Sola丶小克]
	#define rAthenaCN_ScriptResults_GetInventoryList
#endif // rAthenaCN_ScriptResults

// ============================================================================
// 脚本参数拓展组 - rAthenaCN_ScriptParams
// ============================================================================

#ifdef rAthenaCN_ScriptParams
	// 是否拓展 readparam 脚本指令的可用参数 [Sola丶小克]
	#define rAthenaCN_ScriptParams_ReadParam
#endif // rAthenaCN_ScriptParams

#endif // _RATHENA_CN_CONFIG_HPP_

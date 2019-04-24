// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _RATHENA_CN_CONFIG_HPP_
#define _RATHENA_CN_CONFIG_HPP_

#include "renewal.hpp"
#include "packets.hpp"

#define Pandas

#ifdef Pandas
	#define Pandas_Basic
	#define Pandas_DatabaseIncrease
	#define Pandas_StructIncrease
	#define Pandas_FuncIncrease
	#define Pandas_CreativeWork
	#define Pandas_Bugfix
	#define Pandas_NpcEvent
	#define Pandas_Mapflags
	#define Pandas_AtCommands
	#define Pandas_ScriptEngine
	#define Pandas_ScriptCommands
	#define Pandas_ScriptResults
	#define Pandas_ScriptParams
#endif // Pandas

// ============================================================================
// 基础组 - Pandas_Basic
// ============================================================================

#ifdef Pandas_Basic
	// 定义 Pandas 的版本号
	#define Pandas_Version "v2.0.0"

	// 在启动时显示 Pandas 的 LOGO
	#define Pandas_Show_Logo

	// 在启动时显示免责声明
	#define Pandas_Disclaimer

	// 在启动时显示 Pandas 的版本号
	#define Pandas_Show_Version

	// 是否启用 npc.cpp 中的自定义辅助函数
	#define Pandas_NpcHelper_CommonFunc
#endif // Pandas_Basic

// ============================================================================
// 数据库增强组 - Pandas_DatabaseIncrease
// ============================================================================

#ifdef Pandas_DatabaseIncrease
	// 是否启用 Pandas 的道具属性数据库 [Sola丶小克]
	// 类似 item_flag 数据库, 不过 rAthena 自己会不断扩充 flag 的定义
	// 为了避免未来可能存在的冲突, 直接创建一个新的数据库来存放对物品属性的自定义扩充
	#define Pandas_Database_ItemProperties
#endif // Pandas_DatabaseIncrease

// ============================================================================
// 数据结构增强组 - Pandas_StructIncrease
// ============================================================================

#ifdef Pandas_StructIncrease
	// 使 item_data 可记录此物品是否为宠物捕捉道具 [Sola丶小克]
	// 结构体修改定位 itemdb.hpp -> item_data.taming_mobid
	#define Pandas_Struct_Item_Data_Taming_Mobid

	// 使 item_data 可记录此物品是否执行了 callfunc 指令 [Sola丶小克]
	// 结构体修改定位 itemdb.hpp -> item_data.has_callfunc
	#define Pandas_Struct_Item_Data_Has_CallFunc

	// 使 item_data 可记录此物品的特殊属性 [Sola丶小克]
	// 效果与 item_data.flag 类似, 只是数据源为 item_properties.yml 
	// 结构体修改定位 itemdb.hpp -> item_data.properties
	// 此选项开关需要依赖 Pandas_Database_ItemProperties 的拓展
	#ifdef Pandas_Database_ItemProperties
		#define Pandas_Struct_Item_Data_Properties
	#endif // Pandas_Database_ItemProperties

	// 使 map_session_data 有一个独立的结构体用来存放 Pandas 的拓展 [Sola丶小克]
	// 结构体修改定位 pc.hpp -> map_session_data.pandas
	#define Pandas_Struct_Map_Session_Data_Pandas

	// 以下选项开关需要依赖 Pandas_Struct_Map_Session_Data_Pandas 的拓展
	#ifdef Pandas_Struct_Map_Session_Data_Pandas
		// 使 map_session_data 可记录当前玩家正在处理哪一个脚本事件 [Sola丶小克]
		// 结构体修改定位 pc.hpp -> map_session_data.pandas.workinevent
		#define Pandas_Struct_Map_Session_Data_WorkInEvent

		// 使 map_session_data 可记录事件中断请求 [Sola丶小克]
		// 结构体修改定位 pc.hpp -> map_session_data.pandas.eventhalt
		#define Pandas_Struct_Map_Session_Data_EventHalt
	#endif // Pandas_Struct_Map_Session_Data_Pandas
#endif // Pandas_StructIncrease

// ============================================================================
// 函数修改组 - Pandas_FuncIncrease
// ============================================================================

#ifdef Pandas_FuncIncrease
	// 在 pc.cpp 中的 pc_equipitem 增加 swapping 参数 [Sola丶小克]
	// 新增的 swapping 用于判断当前的装备穿戴调用是否由装备切换机制引发, 默认为 false
	#define Pandas_FuncParams_PC_EQUIPITEM

	// 调整 pc.cpp 中 pc_equipitem 执行道具绑定的时机 [Sola丶小克]
	#define Pandas_FuncLogic_PC_EQUIPITEM_BOUND_OPPORTUNITY
#endif // Pandas_FuncIncrease

// ============================================================================
// 原创功能组 - Pandas_CreativeWork
// ============================================================================

#ifdef Pandas_CreativeWork
	// 是否启用崩溃转储文件生成机制 [Sola丶小克]
	#ifdef _WIN32
		#define Pandas_Crash_Report
	#endif // _WIN32

	// 扩展信息配置文件 (Msg_conf) 的 ID 最大上限,
	// 同时提供 msg_txt_cn 宏定义函数, 方便在工程中使用自定义信息 [Sola丶小克]
	#define Pandas_Message_Conf

	// 是否支持在 map_athena.conf 中设定封包混淆密钥 [Sola丶小克]
	#ifdef PACKET_OBFUSCATION
		#define Pandas_Support_Specify_PacketKeys
	#endif // PACKET_OBFUSCATION

	// 是否支持读取 UTF8-BOM 编码的配置文件 [Sola丶小克]
	#define Pandas_Support_Read_UTF8BOM_Configure

	// 是否启用数据库编码自动判定机制 [Sola丶小克]
	#define Pandas_Smart_Codepage

	// 在使用 _M/_F 注册的时候, 能够限制使用中文等字符作为游戏账号 [Sola丶小克]
	// 这里的 PCRE_SUPPORT 在"项目属性 -> C/C++ -> 预处理器"中定义
	#ifdef PCRE_SUPPORT
		#define Pandas_Strict_Userid_Verification
	#endif // PCRE_SUPPORT

	// 是否支持隐藏角色服务器的在线人数 [Sola丶小克]
	#define Pandas_Support_Hide_Online_Players_Count

	// 是否支持禁止创建杜兰族角色 [Sola丶小克]
	#if PACKETVER >= 20151001
		#define Pandas_Reject_Create_Doram_Character
	#endif // PACKETVER >= 20151001

	// 是否支持对战斗配置选项进行完整性检查 [╰づ记忆•斑驳〤 实现] [Sola丶小克 改进]
	#define Pandas_Battle_Config_Verification

	// 是否扩展魔物名称能展现的信息, 比如体型、种族、属性 [Sola丶小克 改进]
	#define Pandas_MobInfomation_Extend

	// 是否加强 2013-12-23 以及 2013-08-07 客户端的混淆密钥 [Sola丶小克]
	#define Pandas_Packet_Obfuscation_Keys

	// 使影子装备可以支持插卡, 而不会被强制转换成普通道具 [Sola丶小克]
	#define Pandas_Shadowgear_Support_Card

	// 实现道具特殊属性数据库的实际作用 [Sola丶小克]
	// 截止目前, 该数据库 (item_properties.yml) 支持两个特殊的属性标记, 分别是:
	// & 1 = 避免物品被玩家主动使用而消耗
	// & 2 = 避免物品被作为发动技能的必要道具而消耗
	#define Pandas_Implement_Function_Of_Item_Properties
#endif // Pandas_CreativeWork

// ============================================================================
// 官方BUG修正组 - Pandas_Bugfix
// ============================================================================

#ifdef Pandas_Bugfix
	// 用 mysql_set_character_set 来设置 MySQL 的编码字符集 [Sola丶小克]
	#define Pandas_Fix_Mysql_SetEncoding

	// 修正在部分情况下角色公会图标刷新不及时的问题 [Sola丶小克]
	#define Pandas_Fix_GuildEmblem_Update

	// 修正部分简体、繁体中文字符作为角色名时, 会被变成问号的问题 [Sola丶小克]
	// 例如: "凯撒"中的"凯"字, "聽風"中的"聽"字等
	#define Pandas_Fix_Chinese_Character_Trimmed

	// 修复 item_trade 中限制物品掉落后, 权限足够的 GM 也无法绕过限制的问题 [Sola丶小克]
	#define Pandas_Fix_Item_Trade_FloorDropable

	// 修复 "活动用原地复活之证" 的效果 (道具编号为 6316) [Sola丶小克]
	#define Pandas_Fix_E_Token_Of_Siegfried

	// 修复使用 sommon 脚本指令召唤不存在的魔物, 会导致地图服务器崩溃的问题 [Sola丶小克]
	#define Pandas_Fix_ScriptCommand_Summon_Crash
#endif // Pandas_Bugfix

// ============================================================================
// NPC事件组 - Pandas_NpcEvent
// ============================================================================

#ifdef Pandas_NpcEvent
	/************************************************************************/
	/* Filter 类型的过滤事件，这些事件可以被 processhalt 中断                    */
	/************************************************************************/

	#ifdef Pandas_Struct_Map_Session_Data_EventHalt
		// 当玩家在装备鉴定列表中选择好装备, 并点击“确定”按钮时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCIdentifyFilter
		// 常量名称: NPCF_IDENTIFY / 变量名称: identify_filter_name
		#define Pandas_NpcFilter_IDENTIFY

		// 当玩家进入 NPC 开启的聊天室时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCInChatroomFilter
		// 常量名称: NPCF_ENTERCHAT / 变量名称: enterchat_filter_name
		#define Pandas_NpcFilter_ENTERCHAT

		// 当玩家准备穿戴装备时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCEquipFilter
		// 常量名称: NPCF_EQUIP / 变量名称: equip_filter_name
		#define Pandas_NpcFilter_EQUIP

		// 当玩家准备脱下装备时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCUnequipFilter
		// 常量名称: NPCF_UNEQUIP / 变量名称: unequip_filter_name
		#define Pandas_NpcFilter_UNEQUIP

		// 当玩家准备创建队伍时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCPartyCreateFilter
		// 常量名称: NPCF_CREATE_PARTY / 变量名称: create_party_filter_name
		#define Pandas_NpcFilter_CREATE_PARTY

		// 当玩家准备加入队伍时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCPartyJoinFilter
		// 常量名称: NPCF_JOIN_PARTY / 变量名称: join_party_filter_name
		#define Pandas_NpcFilter_JOIN_PARTY

		// 当玩家准备离开队伍时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCPartyLeaveFilter
		// 常量名称: NPCF_LEAVE_PARTY / 变量名称: leave_party_filter_name
		#define Pandas_NpcFilter_LEAVE_PARTY

		// 当玩家准备创建公会时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCGuildCreateFilter
		// 常量名称: NPCF_CREATE_GUILD / 变量名称: create_guild_filter_name
		#define Pandas_NpcFilter_CREATE_GUILD

		// 当玩家准备加入公会时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCGuildJoinFilter
		// 常量名称: NPCF_JOIN_GUILD / 变量名称: join_guild_filter_name
		#define Pandas_NpcFilter_JOIN_GUILD

		// 当玩家准备离开公会时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCGuildLeaveFilter
		// 常量名称: NPCF_LEAVE_GUILD / 变量名称: leave_guild_filter_name
		#define Pandas_NpcFilter_LEAVE_GUILD

		// 当玩家准备插入卡片时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCInsertCardFilter
		// 常量名称: NPCF_INSERT_CARD / 变量名称: insert_card_filter_name
		#define Pandas_NpcFilter_INSERT_CARD

		// 当玩家准备使用非装备类道具时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCUseItemFilter
		// 常量名称: NPCF_USE_ITEM / 变量名称: use_item_filter_name
		#define Pandas_NpcFilter_USE_ITEM

		// 当玩家准备使用技能时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCUseSkillFilter
		// 常量名称: NPCF_USE_SKILL / 变量名称: use_skill_filter_name
		#define Pandas_NpcFilter_USE_SKILL

		// 当玩家准备应用一个状态时触发过滤器 [Sola丶小克]
		// 事件类型: Filter / 事件名称: OnPCBuffStartFilter
		// 常量名称: NPCF_SC_START / 变量名称: sc_start_filter_name
		#define Pandas_NpcFilter_SC_START
		// PYHELP - NPCEVENT - INSERT POINT - <Section 1>
	#endif // Pandas_Struct_Map_Session_Data_EventHalt

	/************************************************************************/
	/* Event  类型的标准事件，这些事件不能被 processhalt 打断                    */
	/************************************************************************/

	// 当玩家杀死 MVP 魔物时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCKillMvpEvent
	// 常量名称: NPCE_KILLMVP / 变量名称: killmvp_event_name
	#define Pandas_NpcEvent_KILLMVP

	// 当玩家成功鉴定了装备时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCIdentifyEvent
	// 常量名称: NPCE_IDENTIFY / 变量名称: identify_event_name
	#define Pandas_NpcEvent_IDENTIFY

	// 当玩家成功穿戴一件装备时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCEquipEvent
	// 常量名称: NPCE_EQUIP / 变量名称: equip_event_name
	#define Pandas_NpcEvent_EQUIP

	// 当玩家成功脱下一件装备时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCUnequipEvent
	// 常量名称: NPCE_UNEQUIP / 变量名称: unequip_event_name
	#define Pandas_NpcEvent_UNEQUIP

	// 当玩家成功创建队伍后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCPartyCreateEvent
	// 常量名称: NPCE_CREATE_PARTY / 变量名称: create_party_event_name
	#define Pandas_NpcEvent_CREATE_PARTY

	// 当玩家成功加入队伍后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCPartyJoinEvent
	// 常量名称: NPCE_JOIN_PARTY / 变量名称: join_party_event_name
	#define Pandas_NpcEvent_JOIN_PARTY

	// 当玩家成功离开队伍后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCPartyLeaveEvent
	// 常量名称: NPCE_LEAVE_PARTY / 变量名称: leave_party_event_name
	#define Pandas_NpcEvent_LEAVE_PARTY

	// 当人工生命体升级时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCHomLvUpEvent
	// 常量名称: NPCE_HOM_LEVELUP / 变量名称: hom_levelup_event_name
	#define Pandas_NpcEvent_HOM_LEVELUP

	// 当召唤人工生命体时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCHomCallEvent
	// 常量名称: NPCE_HOM_CALL / 变量名称: hom_call_event_name
	#define Pandas_NpcEvent_HOM_CALL

	// 当人工生命体安息时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCHomRestEvent
	// 常量名称: NPCE_HOM_REST / 变量名称: hom_rest_event_name
	#define Pandas_NpcEvent_HOM_REST

	// 当人工生命体死亡时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCHomDeadEvent
	// 常量名称: NPCE_HOM_DEAD / 变量名称: hom_dead_event_name
	#define Pandas_NpcEvent_HOM_DEAD

	// 当人工生命体复活时触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCHomAliveEvent
	// 常量名称: NPCE_HOM_WAKE / 变量名称: hom_wake_event_name
	#define Pandas_NpcEvent_HOM_WAKE

	// 当玩家成功创建公会后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCGuildCreateEvent
	// 常量名称: NPCE_CREATE_GUILD / 变量名称: create_guild_event_name
	#define Pandas_NpcEvent_CREATE_GUILD

	// 当玩家成功加入公会后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCGuildJoinEvent
	// 常量名称: NPCE_JOIN_GUILD / 变量名称: join_guild_event_name
	#define Pandas_NpcEvent_JOIN_GUILD

	// 当玩家成功离开公会后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCGuildLeaveEvent
	// 常量名称: NPCE_LEAVE_GUILD / 变量名称: leave_guild_event_name
	#define Pandas_NpcEvent_LEAVE_GUILD

	// 当玩家成功插入卡片后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCInsertCardEvent
	// 常量名称: NPCE_INSERT_CARD / 变量名称: insert_card_event_name
	#define Pandas_NpcEvent_INSERT_CARD

	// 当玩家成功使用非装备类道具后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCUseItemEvent
	// 常量名称: NPCE_USE_ITEM / 变量名称: use_item_event_name
	#define Pandas_NpcEvent_USE_ITEM

	// 当玩家成功使用技能后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCUseSkillEvent
	// 常量名称: NPCE_USE_SKILL / 变量名称: use_skill_event_name
	#define Pandas_NpcEvent_USE_SKILL

	// 当玩家已成功获得了一个状态后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCBuffStartEvent
	// 常量名称: NPCE_SC_START / 变量名称: sc_start_event_name
	#define Pandas_NpcEvent_SC_START

	// 当玩家已成功解除了一个状态后触发事件 [Sola丶小克]
	// 事件类型: Event / 事件名称: OnPCBuffEndEvent
	// 常量名称: NPCE_SC_END / 变量名称: sc_end_event_name
	#define Pandas_NpcEvent_SC_END
	// PYHELP - NPCEVENT - INSERT POINT - <Section 2>
#endif // Pandas_NpcEvent

// ============================================================================
// 地图标记组 - Pandas_Mapflags
// ============================================================================

#ifdef Pandas_Mapflags
	// 是否启用 mobinfo 地图标记 [Sola丶小克]
	// 该标记用于指定某地图的 show_mob_info 值, 以此控制该地图魔物名称的展现信息
	#define Pandas_MapFlag_Mobinfo

	// 是否启用 noautoloot 地图标记 [Sola丶小克]
	// 该标记用于在给定此标记的地图上禁止玩家使用自动拾取功能, 或使已激活的自动拾取功能失效
	#define Pandas_MapFlag_NoAutoLoot

	// 是否启用 notoken 地图标记 [Sola丶小克]
	// 该标记用于禁止玩家在指定的地图上使用“原地复活之证”道具
	#define Pandas_MapFlag_NoToken

	// 是否启用 nocapture 地图标记 [Sola丶小克]
	// 该标记用于禁止玩家在地图上使用宠物捕捉道具或贤者的"随机技能"来捕捉宠物
	// 此地图标记依赖 Pandas_Struct_Item_Data_Taming_Mobid 的拓展
	#ifdef Pandas_Struct_Item_Data_Taming_Mobid
		#define Pandas_MapFlag_NoCapture
	#endif // Pandas_Struct_Item_Data_Taming_Mobid

	// 是否启用 hideguildinfo 地图标记 [Sola丶小克]
	// 使当前地图上的玩家无法见到其他人的公会图标、公会名称、职位等信息 (自己依然可见)
	#define Pandas_MapFlag_HideGuildInfo

	// 是否启用 hidepartyinfo 地图标记 [Sola丶小克]
	// 使当前地图上的玩家无法见到其他人的队伍名称 (自己依然可见)
	#define Pandas_MapFlag_HidePartyInfo

	// 是否启用 nomail 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_NoMail

	// 是否启用 nopet 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_NoPet

	// 是否启用 nohomun 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_NoHomun

	// 是否启用 nomerc 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_NoMerc

	// 是否启用 mobdroprate 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_MobDroprate

	// 是否启用 mvpdroprate 地图标记 [维护者昵称]
	// TODO: 请在此填写此地图标记的说明
	#define Pandas_MapFlag_MvpDroprate
	// PYHELP - MAPFLAG - INSERT POINT - <Section 1>
#endif // Pandas_Mapflags

// ============================================================================
// 管理员指令组 - Pandas_AtCommands
// ============================================================================

#ifdef Pandas_AtCommands
	// 是否启用 recallmap 管理员指令 [Sola丶小克]
	// 召唤当前(或指定)地图的玩家来到身边 (处于离线挂店模式的角色不会被召唤)
	#define Pandas_AtCommand_RecallMap
#endif // Pandas_AtCommands

// ============================================================================
// 脚本引擎修改组 - Pandas_ScriptEngine
// ============================================================================

#ifdef Pandas_ScriptEngine
	// 使脚本引擎能够支持穿越事件队列机制, 直接执行某些事件 [Sola丶小克]
	#define Pandas_ScriptEngine_Express
#endif // Pandas_ScriptEngine

// ============================================================================
// 脚本指令组 - Pandas_ScriptCommands
// ============================================================================

#ifdef Pandas_ScriptCommands
	// 是否启用 setheaddir 脚本指令 [Sola丶小克]
	// 用于调整角色纸娃娃脑袋的朝向 (0 - 正前方; 1 - 向右看; 2 - 向左看)
	#define Pandas_ScriptCommand_SetHeadDir

	// 是否启用 setbodydir 脚本指令 [Sola丶小克]
	// 用于调整角色纸娃娃身体的朝向 (与 NPC 一致, 从 0 到 7 共 8 个方位可选择)
	#define Pandas_ScriptCommand_SetBodyDir

	// 是否启用 openbank 脚本指令 [Sola丶小克]
	// 让指定的角色立刻打开银行界面 (只对拥有随身银行的客户端版本有效)
	#define Pandas_ScriptCommand_OpenBank

	// 是否启用 instance_users 脚本指令 [Sola丶小克]
	// 获取指定的副本实例中已经进入副本地图的人数
	#define Pandas_ScriptCommand_InstanceUsers

	// 是否启用 cap 脚本指令 [Sola丶小克]
	// 确保数值不低于给定的最小值, 不超过给定的最大值
	#define Pandas_ScriptCommand_CapValue

	// 是否启用 mobremove 脚本指令 [Sola丶小克]
	// 根据 GID 移除一个魔物单位 (只是移除, 不会让魔物死亡)
	#define Pandas_ScriptCommand_MobRemove

	// 是否启用 mesclear 脚本指令 [Sola丶小克]
	// 由于 rAthena 已经实现 clear 指令, 这里兼容老版本 mesclear 指令
	#define Pandas_ScriptCommand_MesClear

	// 是否启用 battleignore 脚本指令 [Sola丶小克]
	// 将角色设置为魔物免战状态, 避免被魔物攻击 (与 GM 指令的 monsterignore 效果一致)
	#define Pandas_ScriptCommand_BattleIgnore

	// 是否启用 gethotkey 脚本指令 [Sola丶小克]
	// 获取指定快捷键位置当前的信息 (该指令有一个用于兼容的别名: get_hotkey)
	#define Pandas_ScriptCommand_GetHotkey

	// 是否启用 sethotkey 脚本指令 [Sola丶小克]
	// 设置指定快捷键位置的信息 (该指令有一个用于兼容的别名: set_hotkey)
	#define Pandas_ScriptCommand_SetHotkey

	// 是否启用 showvend 脚本指令 [Jian916]
	// 使指定的 NPC 头上可以显示露天商店的招牌, 点击招牌可触发与 NPC 的对话
	#define Pandas_ScriptCommand_ShowVend

	// 是否启用 viewequip 脚本指令 [Sola丶小克]
	// 使用该指令可以查看指定在线角色的装备面板信息 (注意: v2.0.0 以前是通过账号编号)
	#define Pandas_ScriptCommand_ViewEquip

	// 是否启用 countitemidx 脚本指令 [Sola丶小克]
	// 获取指定背包序号的道具在背包中的数量 (该指令有一个用于兼容的别名: countinventory)
	#define Pandas_ScriptCommand_CountItemIdx

	// 是否启用 delitemidx 脚本指令 [Sola丶小克]
	// 移除指定背包序号的道具, 其中数量参数可不填, 若不填则表示删除指定道具的全部
	#define Pandas_ScriptCommand_DelItemIdx

	// 是否启用 identifyidx 脚本指令 [Sola丶小克]
	// 鉴定指定背包序号的道具 (该指令有一个用于兼容的别名: identifybyidx)
	#define Pandas_ScriptCommand_IdentifyIdx

	// 是否启用 unequipidx 脚本指令 [Sola丶小克]
	// 脱下指定背包序号的道具 (该指令有一个用于兼容的别名: unequipinventory)
	#define Pandas_ScriptCommand_UnEquipIdx

	// 是否启用 equipidx 脚本指令 [Sola丶小克]
	// 穿戴指定背包序号的道具 (该指令有一个用于兼容的别名: equipinventory)
	#define Pandas_ScriptCommand_EquipIdx

	// 是否启用 itemexists 脚本指令 [Sola丶小克]
	// 确认物品数据库中是否存在指定物品 (该指令有一个用于兼容的别名: existitem)
	#define Pandas_ScriptCommand_ItemExists

	// 是否启用 renttime 脚本指令 [Sola丶小克]
	// 增加/减少指定位置装备的租赁时间 (该指令有一个用于兼容的别名: resume)
	#define Pandas_ScriptCommand_RentTime

	// 是否启用 getequipidx 脚本指令 [Sola丶小克]
	// 获取指定位置装备的背包序号
	#define Pandas_ScriptCommand_GetEquipIdx

	// 是否启用 statuscalc 脚本指令 [Sola丶小克]
	// 由于 rAthena 已经实现 recalculatestat 指令, 这里兼容老版本 statuscalc 指令
	#define Pandas_ScriptCommand_StatusCalc

	// 是否启用 getequipexpiretick 脚本指令 [Sola丶小克]
	// 获取指定位置装备的租赁到期剩余秒数 (该指令有一个用于兼容的别名: isrental)
	#define Pandas_ScriptCommand_GetEquipExpireTick

	// 是否启用 getinventoryinfo 脚本指令 [Sola丶小克]
	// 查询指定背包序号的道具的详细信息
	#define Pandas_ScriptCommand_GetInventoryInfo

	// 是否启用 statuscheck 脚本指令 [Sola丶小克]
	// 判断状态是否存在, 并取得相关的状态参数 (该指令有一个用于兼容的别名: sc_check)
	#define Pandas_ScriptCommand_StatusCheck

	// 是否启用 renttimeidx 脚本指令 [Sola丶小克]
	// 增加/减少指定背包序号道具的租赁时间
	#define Pandas_ScriptCommand_RentTimeIdx

	// 是否启用 party_leave 脚本指令 [Sola丶小克]
	// 使当前角色或指定角色退出队伍 (主要出于兼容目的而实现该指令)
	#define Pandas_ScriptCommand_PartyLeave

	// 是否启用 script4each 脚本指令 [Sola丶小克]
	// 对指定范围的玩家执行相同的一段脚本
	#define Pandas_ScriptCommand_Script4Each

	// 是否启用 searcharray 脚本指令 [Sola丶小克]
	// 由于 rAthena 已经实现 inarray 指令, 这里兼容老版本 searcharray 指令
	#define Pandas_ScriptCommand_SearchArray

	// 是否启用 getsameipinfo 脚本指令 [Sola丶小克]
	// 获得某个指定 IP 在线的玩家信息
	#define Pandas_ScriptCommand_GetSameIpInfo

	// 是否启用 logout 脚本指令 [Sola丶小克]
	// 使指定的角色立刻登出游戏
	#define Pandas_ScriptCommand_Logout

	// 是否启用 warppartyrevive 脚本指令 [Sola丶小克]
	// 与 warpparty 类似, 但可以复活死亡的队友并传送 (该指令有一个用于兼容的别名: warpparty2)
	#define Pandas_ScriptCommand_WarpPartyRevive

	// 是否启用 getareagid 脚本指令 [Sola丶小克]
	// 获取指定范围内特定类型单位的全部 GID (注意: 该指令不再兼容以前 rAthenaCN 的同名指令)
	#define Pandas_ScriptCommand_GetAreaGid

	// 是否启用 processhalt 脚本指令 [Sola丶小克]
	// 在事件处理代码中使用该指令, 可以中断源代码的后续处理逻辑
	// 此选项开关需要依赖 Pandas_Struct_Map_Session_Data_EventHalt 的拓展
	#ifdef Pandas_Struct_Map_Session_Data_EventHalt
		#define Pandas_ScriptCommand_ProcessHalt
	#endif // Pandas_Struct_Map_Session_Data_EventHalt
	// PYHELP - SCRIPTCMD - INSERT POINT - <Section 1>
#endif // Pandas_ScriptCommands

// ============================================================================
// 脚本返回值拓展组 - Pandas_ScriptResults
// ============================================================================

#ifdef Pandas_ScriptResults
	// 是否拓展 getinventorylist 脚本指令的返回数组 [Sola丶小克]
	#define Pandas_ScriptResults_GetInventoryList
#endif // Pandas_ScriptResults

// ============================================================================
// 脚本参数拓展组 - Pandas_ScriptParams
// ============================================================================

#ifdef Pandas_ScriptParams
	// 是否拓展 readparam 脚本指令的可用参数 [Sola丶小克]
	#define Pandas_ScriptParams_ReadParam

	// 是否拓展 getiteminfo 脚本指令的可用参数 [Sola丶小克]
	#define Pandas_ScriptParams_GetItemInfo
#endif // Pandas_ScriptParams

#endif // _RATHENA_CN_CONFIG_HPP_

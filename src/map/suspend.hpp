// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "../common/cbasetypes.hpp"
#include "../common/db.hpp"
#include "../common/timer.hpp"

enum e_suspend_mode : uint8 {
	SUSPEND_MODE_NONE     = 0x0000,
	SUSPEND_MODE_OFFLINE  = 0x0001,		// 离线挂机
	SUSPEND_MODE_AFK      = 0x0002		// 离开模式 (AFK)
};

struct s_suspender {
	uint32 account_id;					// 账号编号 (数据库中的主键)
	uint32 char_id;						// 角色编号 (suspender_db 的主键)
	int m;								// 地图编号
	uint16 x, y;						// 地图的 X 和 Y 坐标
	unsigned char sex,					// 性别 (M 表示男性, F 表示女性)
		dir,							// 纸娃娃身体朝向
		head_dir,						// 纸娃娃头部朝向
		sit;							// 是否坐下
	enum e_suspend_mode mode;			// 模式
	t_tick tick;						// 离线挂机 或 离开模式 的起始时间
	long val1, val2, val3, val4;		// 附加参数
	struct map_session_data* sd;
};

void suspend_recall_online();
void suspend_recall_postfix(struct map_session_data* sd);
void suspend_active(struct map_session_data* sd, enum e_suspend_mode smode);
void suspend_deactive(struct map_session_data* sd);

void do_final_suspend(void);
void do_init_suspend(void);

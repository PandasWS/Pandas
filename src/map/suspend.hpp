// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "../common/cbasetypes.hpp"
#include "../common/db.hpp"
#include "../common/timer.hpp"

enum e_suspend_mode : uint8 {
	SUSPEND_MODE_NONE     = 0x0000,
	SUSPEND_MODE_OFFLINE  = 0x0001,
	SUSPEND_MODE_AFK      = 0x0002
};

struct s_suspender {
	uint32 account_id; ///< Account ID
	uint32 char_id; ///< Char ID
	int m; ///< Map location
	uint16 x, ///< X location
		y; ///< Y location
	unsigned char sex, ///< Suspender's sex
		dir, ///< Body direction
		head_dir, ///< Head direction
		sit; ///< Is sitting?
	enum e_suspend_mode mode;
	t_tick tick;
	long val1, val2, val3, val4;
	struct map_session_data* sd;
};

void suspend_recall_online();
void suspend_active(struct map_session_data* sd, enum e_suspend_mode smode);
void suspend_deactive(struct map_session_data* sd);

void do_final_suspend(void);
void do_init_suspend(void);

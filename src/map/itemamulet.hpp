// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "../common/cbasetypes.hpp"

bool amulet_is(uint16 nameid);
int amulet_pandas_type(uint16 nameid);

bool amulet_is_firstone(struct map_session_data *sd, struct item *item, int amount);
bool amulet_is_lastone(struct map_session_data *sd, int n, int amount);

void amulet_apply_additem(struct map_session_data *sd, int n, bool is_firstone);
void amulet_apply_delitem(struct map_session_data *sd, int n, bool is_lastone);

void amulet_status_calc(struct map_session_data *sd, enum e_status_calc_opt opt);

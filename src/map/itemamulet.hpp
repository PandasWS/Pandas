// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "status.hpp"

#include <common/cbasetypes.hpp>

#ifdef Pandas_Item_Amulet_System

bool amulet_is(t_itemid nameid);
int amulet_pandas_type(t_itemid nameid);

bool amulet_is_firstone(map_session_data *sd, struct item *item, int amount);
bool amulet_is_lastone(map_session_data *sd, int n, int amount);

void amulet_apply_additem(map_session_data *sd, int n, bool is_firstone);
void amulet_apply_delitem(map_session_data *sd, int n, bool is_lastone);

void amulet_status_calc(map_session_data *sd, uint8 opt);

#endif // Pandas_Item_Amulet_System

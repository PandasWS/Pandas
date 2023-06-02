﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INT_HOMUN_HPP
#define INT_HOMUN_HPP

#include <common/cbasetypes.hpp>

struct s_homunculus;

int inter_homunculus_sql_init(void);
void inter_homunculus_sql_final(void);
int inter_homunculus_parse_frommap(int fd);

bool mapif_homunculus_save(struct s_homunculus* hd);
bool mapif_homunculus_load(int homun_id, struct s_homunculus* hd);
bool mapif_homunculus_delete(int homun_id);
bool mapif_homunculus_rename(char *name);

#endif /* INT_HOMUN_HPP */

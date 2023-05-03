// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <common/mmo.hpp>
#include <common/cbasetypes.hpp>
#include <config/pandas.hpp>

#include <map>
#include <memory>
#include "map.hpp"

enum e_batrec_type {
	BRT_DMG_RECEIVE = 0,	// 记录宿主所受到的伤害
	BRT_DMG_CAUSE			// 记录宿主所造成的伤害
};

enum e_batrec_sort {
	BRS_DESC = 0,
	BRS_ASC
};

enum e_batrec_agg {
	BRA_COMBINE = 0,
	BRA_DISCRETE
};

#define batrec_support(bl) ( \
	bl->type == BL_PC || bl->type == BL_MOB || \
	bl->type == BL_PET || bl->type == BL_HOM || \
	bl->type == BL_MER || bl->type == BL_NPC || \
	bl->type == BL_ELEM)

bool batrec_cmp_asc(std::pair<uint32, s_batrec_item_ptr>& l,
	std::pair<uint32, s_batrec_item_ptr>& r);
bool batrec_cmp_desc(std::pair<uint32, s_batrec_item_ptr>& l,
	std::pair<uint32, s_batrec_item_ptr>& r);

void batrec_new(struct block_list* bl);
void batrec_free(struct block_list* bl);

void batrec_sortout(struct block_list* bl);
void batrec_sortout(struct block_list* bl, e_batrec_type type);
void batrec_aggregation(batrec_map* origin_rec, batrec_map& ret_rec, e_batrec_agg agg);
bool batrec_dorecord(struct block_list* bl);
batrec_map* batrec_getmap(struct block_list* bl, e_batrec_type type);

bool batrec_record(struct block_list* mbl, struct block_list* tbl, e_batrec_type type, int damage);
int64 batrec_query(struct block_list* mbl, uint32 id, e_batrec_type type, e_batrec_agg agg);
void batrec_reset(struct block_list* mbl);
void batrec_reset(struct block_list* mbl, e_batrec_type type);

#define batrec_receive(mbl, src, damage) batrec_record(mbl, src, BRT_DMG_RECEIVE, damage)
#define batrec_cause(mbl, target, damage) batrec_record(mbl, target, BRT_DMG_CAUSE, damage)

﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CHAT_HPP
#define CHAT_HPP

#include "map.hpp" // struct block_list, CHATROOM_TITLE_SIZE

class map_session_data;
struct chat_data;

#define MAX_CHAT_USERS 20

struct chat_data {
	struct block_list bl;            // data for this map object
	char title[CHATROOM_TITLE_SIZE]; // room title 
	char pass[CHATROOM_PASS_SIZE];   // password
	bool pub;                        // private/public flag
	uint8 users;                     // current user count
	uint8 limit;                     // join limit
	uint8 trigger;                   // number of users needed to trigger event
	uint32 zeny;						 // required zeny to join
	uint32 minLvl;					 // minimum base level to join
	uint32 maxLvl;					 // maximum base level allowed to join
	map_session_data* usersd[MAX_CHAT_USERS];
	struct block_list* owner;
	char npc_event[EVENT_NAME_LENGTH];
	DBMap* kick_list;				//DBMap of users who were kicked from this chat
};

int chat_createpcchat(map_session_data* sd, const char* title, const char* pass, int limit, bool pub);
int chat_joinchat(map_session_data* sd, int chatid, const char* pass);
int chat_leavechat(map_session_data* sd, bool kicked);
int chat_changechatowner(map_session_data* sd, const char* nextownername);
int chat_changechatstatus(map_session_data* sd, const char* title, const char* pass, int limit, bool pub);
int chat_kickchat(map_session_data* sd, const char* kickusername);

int chat_createnpcchat(struct npc_data* nd, const char* title, int limit, bool pub, int trigger, const char* ev, int zeny, int minLvl, int maxLvl);
int chat_deletenpcchat(struct npc_data* nd);
int chat_enableevent(struct chat_data* cd);
int chat_disableevent(struct chat_data* cd);
int chat_npckickall(struct chat_data* cd);

int chat_npckickchat(struct chat_data* cd, const char* kickusername);

#endif /* CHAT_HPP */

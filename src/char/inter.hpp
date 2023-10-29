﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INTER_HPP
#define INTER_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include <common/cbasetypes.hpp>
#include <common/database.hpp>
#include <common/sql.hpp>

#ifdef Pandas_InterConfig_HideServerIpAddress
	extern int pandas_inter_hide_server_ipaddress;
#endif // Pandas_InterConfig_HideServerIpAddress

struct s_storage_table;

class InterServerDatabase : public TypesafeYamlDatabase<uint32, s_storage_table>{
public:
	InterServerDatabase() : TypesafeYamlDatabase( "INTER_SERVER_DB", 1 ){

	}

	const std::string getDefaultLocation() override;
	uint64 parseBodyNode( const ryml::NodeRef& node ) override;
};

extern InterServerDatabase interServerDb;

int inter_init_sql(const char *file);
void inter_final(void);
int inter_parse_frommap(int fd);
int inter_mapif_init(int fd);
int mapif_disconnectplayer(int fd, uint32 account_id, uint32 char_id, int reason);
void mapif_accinfo_ack(bool success, int map_fd, int u_fd, int u_aid, int account_id, int8 type,
	int group_id, int logincount, int state, const char *email, const char *last_ip, const char *lastlogin,
	const char *birthdate, const char *userid);

int inter_log(const char *fmt,...);

extern unsigned int party_share_level;

extern Sql* sql_handle;
extern Sql* lsql_handle;

int inter_accreg_fromsql(uint32 account_id, uint32 char_id, int fd, int type);

#endif /* INTER_HPP */

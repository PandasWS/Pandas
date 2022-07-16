#pragma once

#include "sql.hpp"
#include <map>
#include <mutex>
#include <string>
#include "socket.hpp"

#include "cbasetypes.hpp"


struct block_list;
enum send_target : uint8;
struct map_data_other_server;
constexpr auto cs_id_factor = 10000000;

struct cross_server_data;
extern bool is_cross_server;
extern bool inherit_source_server_chara_status;
extern bool inherit_source_server_chara_group;
extern char inherit_source_server_chara_group_except[5];
extern const char* CS_CONF_NAME;
extern std::map<int, cross_server_data*> cs_configs_map;
extern std::map<int, int> logintoken_to_cs_id;
extern int marked_cs_id;
extern std::map<int, DBMap*> map_dbs;

struct cross_server_data {
	int server_id = 0;
	char server_name[10];

	char userid[NAME_LENGTH];
	char passwd[NAME_LENGTH];

	std::string char_server_ip = "127.0.0.1";
	int char_server_port = 6121;

};


//config read
int cs_config_read(const char* cfgName);

//common
int get_cs_id(int id);
int get_cs_id_by_fake_name(const char* name);
int get_cs_prefix(int cs_id);
int get_real_id(int id,bool force = false);
int make_fake_id(int id, int cs_id);
bool is_fake_id(int id);
void get_real_name(char* fake_name);
void make_fake_name(int cs_id, char* name);


//fd
//char-serv to map-serv
void chrif_logintoken_pass(int fd, uint32 account_id, uint32 char_id, uint32 login_id1, uint32 login_id2, int cs_id, char* name);

//from char-serv to map-serv
void chrif_logintoken_received(int fd);

//map.cpp
map_data_other_server* findmap(int cs_id, uint32 mapindex);

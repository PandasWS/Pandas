#pragma once

#include "sql.hpp"
#include <map>
#include <string>
#include "socket.hpp"

#include "cbasetypes.hpp"


struct block_list;
enum send_target : uint8;
struct map_data_other_server;
struct mmo_status_cache;
constexpr auto cs_id_factor = 10000000;
constexpr auto MAX_CHAR_SERVERS = 5;

struct cross_server_data;
extern bool is_cross_server;
extern const char* CS_CONF_NAME;
extern std::map<int, cross_server_data*> cs_configs_map;
extern std::map<int, int> logintoken_to_cs_id;
extern int marked_cs_id;
extern std::map<int, DBMap*> map_dbs;

//char fd
extern bool cs_init_done;
extern int cs_chrif_connected[MAX_CHAR_SERVERS];
extern int cs_char_fds[MAX_CHAR_SERVERS];
extern int cs_chrif_state[MAX_CHAR_SERVERS];
extern char cs_charserver_names[MAX_CHAR_SERVERS];
extern int cs_ids[MAX_CHAR_SERVERS];
int check_fd_valid(int fd, int flag = 0);
int chrif_fd_isconnected(int fd);
int chrif_get_char_fd(int cs_id);
int chrif_get_cs_id(int map_fd);
bool chrif_check_all_cs_char_fd_health(void);
bool chrif_set_cs_fd_state(int fd, int state, int connected);
void chrif_set_global_fd_state(int fd, int &char_fd, int& chrif_state, int& chrif_connected);
int switch_char_fd(uint32 id, int& char_fd, int& chrif_state);
int switch_char_fd_cs_id(uint32 cs_id, int& char_fd);

struct cross_server_data {
	int server_id = 0;
	char server_name[6];

	char userid[NAME_LENGTH];
	char passwd[NAME_LENGTH];

	std::string char_server_ip = "127.0.0.1";
	int char_server_port = 6121;

};

struct mmo_status_cache
{
	int cs_id;
	int char_id;
	int account_id;
	int party_id;
	int guild_id;

};


//config read
int cs_config_read(const char* cfgName);

//common
int get_cs_id(int id);
int get_cs_prefix(int cs_id);
int get_real_id(int id,bool force = false);
int make_fake_id(int id, int cs_id);
bool is_fake_id(int id);
void get_real_name(char* fake_name);


//fd
//char-serv to map-serv
void chrif_logintoken_pass(int fd, uint32 account_id, uint32 char_id, uint32 login_id1, uint32 login_id2, int cs_id, char* name);

//from char-serv to map-serv
void chrif_logintoken_received(int fd);

//map.cpp
map_data_other_server* findmap(int cs_id, uint32 mapindex);

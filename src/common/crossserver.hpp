#pragma once

#include "sql.hpp"
#include <vector>
#include <map>
#include <string>
#include "socket.hpp"

#include "cbasetypes.hpp"
#include "nullpo.hpp"
#include "showmsg.hpp"
#include "malloc.hpp"

struct map_data_other_server;
constexpr auto cs_id_factor = 10000000;
constexpr auto MAX_CHAR_SERVERS = 5;

struct cross_server_data;
extern bool is_cross_server;
extern std::map<int, Sql*> map_handlers;
extern std::map<int, Sql*> char_handlers;
extern std::map<int, Sql*> login_handlers;
extern std::map<int, Sql*> log_handlers;
extern std::map<int, Sql*> query_handlers;
extern std::map<int, Sql*> query_async_handlers;
extern std::map<int, Sql*> accountdb_handlers;
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
int chrif_get_cs_id(int fd);
bool chrif_check_all_cs_char_fd_health(void);
bool chrif_set_cs_fd_state(int fd, int state, int connected);
void chrif_set_global_fd_state(int fd, int &char_fd, int& chrif_state, int& chrif_connected);
int switch_char_fd(uint32 id, int& char_fd, int& chrif_state);

enum SqlHandlerType {
	SQLTYPE_NONE = 0, // no used
	SQLTYPE_MAP,
	SQLTYPE_CHAR, // no used
	SQLTYPE_LOGIN, // no used
	SQLTYPE_LOG,
	SQLTYPE_QUERY,
	SQLTYPE_QUERY_ASYNC,
	SQLTYPE_ACCOUNTDB  // no used
};

struct cross_server_data {
	int server_id = 0;
	std::string server_name = "[CS]";

	std::string char_server_ip = "127.0.0.1";
	int char_server_port = 6121;
	int char_server_database_port = 3306;
	std::string char_server_database_ip = "127.0.0.1";
	std::string char_server_database_id = "ragnarok";
	std::string char_server_database_pw;
	std::string char_server_database_db = "ragnarok";

	std::string map_server_ip = "127.0.0.1";
	int map_server_port = 6900;
	int map_server_database_port = 3306;
	std::string map_server_database_ip = "127.0.0.1";
	std::string map_server_database_id = "ragnarok";
	std::string map_server_database_pw;
	std::string map_server_database_db = "ragnarok";

	int log_db_database_port = 3306;
	std::string log_db_database_ip = "127.0.0.1";
	std::string log_db_database_id = "ragnarok";
	std::string log_db_database_pw;
	std::string log_db_database_db = "log";
};


//config read
int cs_config_read(const char* cfgName);

//common
int get_cs_id(int id);
int get_cs_prefix(int cs_id);
int get_real_id(int id);
int make_fake_id(int id, int cs_id);
bool is_fake_id(int id);
map_data_other_server* findmap(int cs_id, uint32 mapindex);

//Sql
Sql* Sql_GetHandler(SqlHandlerType type, int cs_id);
bool switch_handler(Sql*& sql_handler, SqlHandlerType type, int cs_id);
int Sql_Connect(Sql* self, const char* user, const char* passwd, const char* host, uint16 port, const char* db, SqlHandlerType type, int cs_id);

//fd
//char-serv to map-serv
void chrif_logintoken_pass(int fd, uint32 account_id, uint32 char_id, uint32 login_id1, uint32 login_id2, int cs_id, char* name);

//from char-serv to map-serv
void chrif_logintoken_received(int fd);

#include "crossserver.hpp"

//from local conf
bool is_cross_server;
const char* CS_CONF_NAME;
std::map<int, cross_server_data*> cs_configs_map;

//init
bool cs_init_done;
std::map<int, Sql*> map_handlers;
std::map<int, Sql*> char_handlers;
std::map<int, Sql*> login_handlers;
std::map<int, Sql*> log_handlers;
std::map<int, Sql*> query_handlers;
std::map<int, Sql*> query_async_handlers;
std::map<int, Sql*> accountdb_handlers;

int cs_chrif_connected[MAX_CHAR_SERVERS];
int cs_char_fds[MAX_CHAR_SERVERS];
int cs_chrif_state[MAX_CHAR_SERVERS];
char cs_charserver_names[MAX_CHAR_SERVERS];
int cs_ids[MAX_CHAR_SERVERS];

//from char-serv
std::map<int, int> logintoken_to_cs_id;
int marked_cs_id;
std::map<int, DBMap*> map_dbs;

struct map_data_other_server;


int cs_config_read(const char* cfgName)
{
	char line[1024];
#ifndef Pandas_Crashfix_Variable_Init
	char w1[1024], w2[1024];
#else
	char w1[1024] = { 0 }, w2[1024] = { 0 };
#endif // Pandas_Crashfix_Variable_Init
	FILE* fp;

	fp = fopen(cfgName, "r");
	if (fp == NULL) {
		ShowError("File not found: %s\n", cfgName);
		return 1;
	}
	struct cross_server_data* csd;
	CREATE(csd, struct cross_server_data, 1);

	while (fgets(line, sizeof(line), fp))
	{

		if (line[0] == '/' && line[1] == '/')
			continue;

		if (sscanf(line, "%1023[^:]: %1023[^\r\n]", w1, w2) != 2)
			continue;

		if (strcmpi(w1, "server_id") == 0)
			csd->server_id = atoi(w2);
		else if (strcmpi(w1, "server_name") == 0)
			csd->server_name.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_ip") == 0)
			csd->char_server_ip.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_port") == 0)
			csd->char_server_port = atoi(w2);
		else if (strcmpi(w1, "char_server_database_ip") == 0)
			csd->char_server_database_ip.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_database_port") == 0)
			csd->char_server_database_port = atoi(w2);
		else if (strcmpi(w1, "char_server_database_id") == 0)
			csd->char_server_database_id.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_database_pw") == 0)
			csd->char_server_database_pw.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_database_db") == 0)
			csd->char_server_database_db.assign(w2, 1024);
		else if (strcmpi(w1, "map_server_ip") == 0)
			csd->map_server_ip.assign(w2, 1024);
		else if (strcmpi(w1, "map_server_port") == 0)
			csd->map_server_port = atoi(w2);
		else if (strcmpi(w1, "map_server_database_ip") == 0)
			csd->map_server_database_ip.assign(w2, 1024);
		else if (strcmpi(w1, "map_server_database_port") == 0)
			csd->map_server_database_port = atoi(w2);
		else if (strcmpi(w1, "map_server_database_id") == 0)
			csd->map_server_database_id.assign(w2, 1024);
		else if (strcmpi(w1, "map_server_database_pw") == 0)
			csd->map_server_database_pw.assign(w2, 1024);
		else if (strcmpi(w1, "map_server_database_db") == 0)
			csd->map_server_database_db.assign(w2, 1024);
		else if (strcmpi(w1, "log_db_database_ip") == 0)
			csd->log_db_database_ip.assign(w2, 1024);
		else if (strcmpi(w1, "log_db_database_id") == 0)
			csd->log_db_database_id.assign(w2, 1024);
		else if (strcmpi(w1, "log_db_database_pw") == 0)
			csd->log_db_database_pw.assign(w2, 1024);
		else if (strcmpi(w1, "log_db_database_port") == 0)
			csd->log_db_database_port = atoi(w2);
		else if (strcmpi(w1, "log_db_database_db") == 0)
			csd->log_db_database_db.assign(w2, 1024);
		else if (strcmpi(w1, "import") == 0)
			cs_config_read(w2);
	}
	if (csd->server_id > 0)
	{
		auto it = std::make_pair(csd->server_id, csd);
		cs_configs_map.insert(it);
	}


	fclose(fp);

	ShowInfo("" CL_BLUE "[Cross Server]" CL_RESET "Done reading %s.\n", cfgName);

	return 0;
}

/**
 * \brief 算出cross server id.如果不是被跨服fake的 id，则返回0
 * \param id : AID/CID
 * \return 
 */
int get_cs_id(int id)
{
	return id / cs_id_factor;
}


/**
 * \brief 根据cs_id算出附加值,用于fake AID、CID
 * \param cs_id : 跨服id
 * \return 
 */
int get_cs_prefix(int cs_id)
{
	return cs_id * cs_id_factor;
}


/**
 * \brief 从fake id返回real id(即原来服务器上的AID、CID)
 * 不是fake id也不会受到影响
 * \param id : real id
 * \return 
 */
int get_real_id(int id)
{
	if (id <= 0)
		return id;
	return id % cs_id_factor;
}


/**
 * \brief 根据cs_id对id进行fake操作
 * \param id : AID/CID
 * \param cs_id : 跨服id
 * \return 
 */
int make_fake_id(int id, int cs_id)
{
	id = get_real_id(id);
	return id + cs_id * cs_id_factor;
}


/**
 * \brief 多数用于Debug打断点
 * \param id : AID/CID
 * \return 
 */
bool is_fake_id(int id)
{
	if (get_cs_id(id) > 0 && id > 0)
	{
#ifdef Pandas_Fake_Id_Check_Debug
		ShowDebug("" CL_BLUE "[Cross Server]" CL_RESET "Id:" CL_LT_YELLOW "%d" CL_RESET " is fake,it's real id is:" CL_LT_YELLOW "%d" CL_RESET ".\n" , id,get_real_id(id));
#endif
		return true;
	}
		
	return false;
}


/**
 * \brief 根据cs_id切换对应类型连接的数据库hanlder
 * \param type 
 * \param cs_id 
 * \return 
 */
Sql* Sql_GetHandler(SqlHandlerType type, int cs_id)
{
	std::map<int, Sql*>* handler;
	switch (type)
	{
	case SQLTYPE_MAP:
		handler = &map_handlers;
		break;
	case SQLTYPE_CHAR:
		handler = &char_handlers;
		//基本不需要切换 除非1个char对着多个login
		break;
	case SQLTYPE_LOGIN:
		handler = &login_handlers;
		//基本不需要切换
		break;
	case SQLTYPE_LOG:
		handler = &log_handlers;
		break;
	case SQLTYPE_QUERY:
		handler = &query_handlers;
		break;
	case SQLTYPE_QUERY_ASYNC:
		handler = &query_async_handlers;
		break;
	case SQLTYPE_ACCOUNTDB:
		handler = &accountdb_handlers;
		break;
	case SQLTYPE_NONE:
	default:
		return nullptr;
	}

	return handler == nullptr ? nullptr : (handler->find(cs_id) != handler->end() ? handler->find(cs_id)->second : nullptr);
}

/**
 * \brief 切换handler
 * \param sql_handler 由于默认是全局变量，为减少代码量而传地址
 * \param type 
 * \param cs_id 
 * \return 
 */
bool switch_handler(Sql* &sql_handler,SqlHandlerType type, int cs_id)
{
	const auto handler = Sql_GetHandler(type, cs_id);
	if(handler != nullptr)
	{
		sql_handler = handler;
		return true;
	}
	return false;
}


/**
 * Establishes a connection to schema
 * @param self : sql handle
 * @param user : username to access
 * @param passwd : password
 * @param host : hostname
 * @param port : port
 * @param db : schema name
 * @param cs_id : 用于定位是跨服服务器的数据库连接
 * @return
 */
int Sql_Connect(Sql* self, const char* user, const char* passwd, const char* host, uint16 port, const char* db, SqlHandlerType type, int cs_id)
{
	int ret = Sql_Connect(self, user, passwd, host, port, db);

	if (ret != SQL_SUCCESS)
		return ret;

	const auto pr = std::make_pair(cs_id, self);

	switch (type)
	{
	case SQLTYPE_MAP:
		map_handlers.insert(pr);
		break;
	case SQLTYPE_CHAR:
		char_handlers.insert(pr);
		break;
	case SQLTYPE_LOGIN:
		login_handlers.insert(pr);
		break;
	case SQLTYPE_LOG:
		log_handlers.insert(pr);
		break;
	case SQLTYPE_QUERY:
		query_handlers.insert(pr);
		break;
	case SQLTYPE_QUERY_ASYNC:
		query_async_handlers.insert(pr);
		break;
	case SQLTYPE_ACCOUNTDB:
		accountdb_handlers.insert(pr);
		break;
	case SQLTYPE_NONE:
		break;
	}

	return SQL_SUCCESS;
}


/**
 * ZA 0x2b30
 * 发送Token(login_id1) 使得客户端在跨服时能够,mapserv(中立服务器)能够知道该账号来自哪里
 * @param fd
 * @param account_id
 * @param char_id
 * @param login_id1
 * @param login_id2
 * @param cs_id
 * @param name
 */
void chrif_logintoken_pass(int fd, uint32 account_id, uint32 char_id, uint32 login_id1, uint32 login_id2, int cs_id, char* name) {
	int len = 24;
	ShowStatus("" CL_BLUE "[Cross Server]" CL_RESET "Send User:%s Token:%d To Map Server[%d]...\n", name, login_id1, fd);
	WFIFOHEAD(fd, len);
	WFIFOW(fd, 0) = 0x2b30;
	WFIFOW(fd, 2) = len;
	WFIFOL(fd, 4) = account_id;
	WFIFOL(fd, 8) = char_id;
	WFIFOL(fd, 12) = login_id1;
	WFIFOL(fd, 16) = login_id2;
	WFIFOL(fd, 20) = cs_id;
	WFIFOSET(fd, len);
}

/**
 * \brief 主要获得从各个Char Server发来的令牌1(login_id1)->cs_id
 * 这样才比较好知道登陆的用户是来自哪个Cross Server
 * \param fd
 */
void chrif_logintoken_received(int fd) {
	uint32 account_id, char_id;
	uint32 login_id1, login_id2;
	int cs_id;

	account_id = RFIFOL(fd, 4);
	char_id = RFIFOL(fd, 8);
	login_id1 = RFIFOL(fd, 12);
	login_id2 = RFIFOL(fd, 16);
	cs_id = RFIFOL(fd, 20);

	const auto it = std::make_pair(login_id1, cs_id);
	logintoken_to_cs_id.insert(it);
}

//默认检查isValid
int check_fd_valid(int fd, int flag)
{
	if (fd <= 0 || (!flag && !session_isValid(fd))) return -1;
	int index;
	ARR_FIND(0, std::size(cs_char_fds), index, cs_char_fds[index] == fd);
	return index == std::size(cs_char_fds) ? -1 : index;
}

int chrif_fd_isconnected(int fd) {
	int index = check_fd_valid(fd);
	if (index == -1)
		return 0;
	return cs_chrif_state[index];
}

int chrif_get_char_fd(int cs_id) {
	int i;
	ARR_FIND(0, std::size(cs_ids), i, cs_ids[i] == cs_id);
	if (i == std::size(cs_ids)) return -1;
	return cs_char_fds[i];
}

int chrif_get_cs_id(int fd) {
	int index = check_fd_valid(fd, 1);
	if (index == -1) return 0;
	return cs_ids[index] > 0 ? cs_ids[index] : 0;
}

bool chrif_check_all_cs_char_fd_health(void) {
	int i;
	ARR_FIND(0, std::size(cs_chrif_connected), i, cs_ids[i] > 0 && (cs_char_fds[i] <= 0));
	return i == std::size(cs_chrif_connected);
}

bool chrif_set_cs_fd_state(int fd, int state, int connected) {
	if (fd <= 0) return false;
	int index;
	ARR_FIND(0, std::size(cs_char_fds), index, cs_char_fds[index] == fd);
	if (index == std::size(cs_char_fds)) return false;
	if (state != -1) cs_chrif_state[index] = state;//0,1,2
	if (connected != -1) cs_chrif_connected[index] = connected;//0,1
	return true;
}

//设置全局状态
void chrif_set_global_fd_state(int fd,int& char_fd,int& chrif_state,int& chrif_connected) {
	int index = check_fd_valid(fd, 1);
	if (index == -1) return;
	char_fd = fd;
	chrif_state = cs_chrif_state[index];
	chrif_connected = cs_chrif_connected[index];
}


/**
 * \brief 根据传进来的AID/CID自动切换对应的char_fd
 * \param id 
 * \param char_fd 
 * \param chrif_state 
 * \return 
 */
int switch_char_fd(uint32 id, int& char_fd, int& chrif_state) {
	int cs_id = get_cs_id(id);
	int tfd = chrif_get_char_fd(cs_id);
	if (tfd == -1)
		return -1;
	int index = check_fd_valid(char_fd);
	if (index == -1)
		return -1;
	chrif_state = cs_chrif_state[index];
	char_fd = tfd;
	if (!(session_isValid(char_fd) && chrif_state == 2)) {
		return -1;
	}
	return char_fd;
}

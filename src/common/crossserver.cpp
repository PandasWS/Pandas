#include "crossserver.hpp"

#include "strlib.hpp"
#include <vector>
#include "nullpo.hpp"
#include "showmsg.hpp"
#include "malloc.hpp"


//from local conf
bool is_cross_server;
const char* CS_CONF_NAME;
std::map<int, cross_server_data*> cs_configs_map;

//init
bool cs_init_done;

int cs_chrif_connected[MAX_CHAR_SERVERS];
int cs_char_fds[MAX_CHAR_SERVERS];
int cs_chrif_state[MAX_CHAR_SERVERS];
char cs_charserver_names[MAX_CHAR_SERVERS];
int cs_ids[MAX_CHAR_SERVERS];

//from map-serv
int marked_cs_id;

//from char-serv
std::map<int, int> logintoken_to_cs_id;
//TODO:优化成DBMap* 嵌套结构会更好吗?
std::map<int, DBMap*> map_dbs;
struct map_data_other_server;

//from char-serv to map-serv local cache
//多任意key对同一个value其实跟适合查询设计而不是缓存吧
//但是char-serv,map-serv的这种架构下没办法了
DBMap* mmo_status_cache_map;//uint32 char_id -> struct mmo_status_cache*
DBData create_mmo_status_cache(DBKey key, va_list args) {
	struct mmo_status_cache* cache;
	CREATE(cache, struct mmo_status_cache, 1);
	cache->char_id = key.i;
	return db_ptr2data(cache);
}


/**
 * \brief map-serv读取conf/cross_server/base.conf
 * \param cfgName 
 * \return 
 */
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
			safestrncpy(csd->server_name, w2, sizeof(csd->server_name));
		else if (strcmpi(w1, "userid") == 0)
			safestrncpy(csd->userid, w2, sizeof(csd->userid));
		else if (strcmpi(w1, "passwd") == 0)
			safestrncpy(csd->passwd, w2, sizeof(csd->passwd));
		else if (strcmpi(w1, "char_server_ip") == 0)
			csd->char_server_ip.assign(w2, 1024);
		else if (strcmpi(w1, "char_server_port") == 0)
			csd->char_server_port = atoi(w2);
		else if (strcmpi(w1, "import") == 0)
			cs_config_read(w2);
	}
	if (csd->server_id > 0 && csd->char_server_port > 0)
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
 * \param force : 负数也强制转换
 * \return 
 */
int get_real_id(int id,bool force)
{
	if (id <= 0 && !force)
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
 * \brief 移除前缀
 * \param fake_name 
 */
void get_real_name(char* fake_name)
{
	if (cs_configs_map.empty()) return;
	for(auto cs = cs_configs_map.begin();cs != cs_configs_map.end();cs=std::next(cs))
	{
		const auto config = cs->second;
		const auto needle = config->server_name;
		if (strlen(needle) == 0 || needle[0] == '\0') continue;

		const size_t len = strlen(fake_name);
		const size_t len2 = strlen(needle);
		if (len <= len2) continue;
		for (unsigned int i = 0; i < len; ++i) {
			if (fake_name[i] == *needle) {
				// matched starting char -- loop through remaining chars
				const char* h, * n;
				for (h = &fake_name[i], n = needle; *h && *n; ++h, ++n) {
					if (*h != *n) {
						break;
					}
				}
				if (!*n) {
					char* temp;
					CREATE(temp, char, len - i);
					memcpy(temp, fake_name, len);
					memset(fake_name, 0, len);

					for (unsigned int st = len2, c = 0; st < len; st++, c++)
						fake_name[c] = temp[st];
					aFree(temp);
					return;
				}
			}
		}
	}
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
	ARR_FIND(0, ARRAYLENGTH(cs_char_fds), index, cs_char_fds[index] == fd);
	return index == ARRAYLENGTH(cs_char_fds) ? -1 : index;
}

int chrif_fd_isconnected(int fd) {
	int index = check_fd_valid(fd);
	if (index == -1)
		return 0;
	return cs_chrif_state[index];
}

int chrif_get_char_fd(int cs_id) {
	int i;
	ARR_FIND(0, ARRAYLENGTH(cs_ids), i, cs_ids[i] == cs_id);
	if (i == ARRAYLENGTH(cs_ids)) return -1;
	return cs_char_fds[i];
}

int chrif_get_cs_id(int map_fd) {
	int index = check_fd_valid(map_fd, 1);
	if (index == -1) return 0;
	return cs_ids[index] > 0 ? cs_ids[index] : 0;
}

bool chrif_check_all_cs_char_fd_health(void) {
	int i;
	ARR_FIND(0, ARRAYLENGTH(cs_chrif_connected), i, cs_ids[i] > 0 && (cs_char_fds[i] <= 0));
	return i == ARRAYLENGTH(cs_chrif_connected);
}

bool chrif_set_cs_fd_state(int fd, int state, int connected) {
	if (fd <= 0) return false;
	int index;
	ARR_FIND(0, ARRAYLENGTH(cs_char_fds), index, cs_char_fds[index] == fd);
	if (index == ARRAYLENGTH(cs_char_fds)) return false;
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

int switch_char_fd_cs_id(uint32 cs_id, int& char_fd) {
	int tfd = chrif_get_char_fd(cs_id);
	if (tfd == -1)
		return -1;
	int index = check_fd_valid(char_fd);
	if (index == -1)
		return -1;
	char_fd = tfd;
	if (!session_isValid(char_fd)) {
		return -1;
	}
	return char_fd;
}

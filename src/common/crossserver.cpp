#include "crossserver.hpp"

#include "strlib.hpp"
#include <vector>
#include "nullpo.hpp"
#include "showmsg.hpp"
#include "malloc.hpp"


//from local conf
bool is_cross_server;
bool inherit_source_server_chara_status = true;
bool inherit_source_server_chara_group = true;
char inherit_source_server_chara_group_except[5];
const char* CS_CONF_NAME;
std::map<int, cross_server_data*> cs_configs_map;

//init
bool cs_init_done;

//from map-serv
int marked_cs_id;

//from char-serv
std::map<int, int> logintoken_to_cs_id;
//TODO:优化成DBMap* 嵌套结构会更好吗?
std::map<int, DBMap*> map_dbs;
struct map_data_other_server;


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
	}else
		aFree(csd);


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
 * \brief 算出cross server id.如果不是被跨服fake的 id，则返回0
 * \param id : AID/CID
 * \return
 */
int get_cs_id_by_fake_name(const char* name)
{
	for (auto cs = cs_configs_map.begin(); cs != cs_configs_map.end(); cs = std::next(cs))
	{
		const auto config = cs->second;
		const auto needle = config->server_name;
		if (strlen(needle) == 0 || needle[0] == '\0') continue;

		const size_t len = strlen(name);
		const size_t len2 = strlen(needle);
		if (len <= len2) continue;
		for (unsigned int i = 0; i < len; ++i) {
			if (name[i] == *needle) {
				// matched starting char -- loop through remaining chars
				const char* h, * n;
				for (h = &name[i], n = needle; *h && *n; ++h, ++n) {
					if (*h != *n) {
						break;
					}
				}
				if (!*n) {
					return config->server_id;
				}
			}
		}
	}
	return 0;
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
#ifdef Pandas_Fake_Id_Check_Debug
	if (id % cs_id_factor > 0)
		ShowDebug("" CL_BLUE "[Cross Server]" CL_RESET "Id:" CL_LT_YELLOW "%d" CL_RESET " is fake,it's real id is:" CL_LT_YELLOW "%d" CL_RESET ".\n", id, get_real_id(id));
#endif
	
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
 * \brief 增加前缀
 * \param fake_name
 */
void make_fake_name(int cs_id,char* name)
{
	if (cs_configs_map.empty() || cs_id == 0) return;
	const auto cs = cs_configs_map.find(cs_id);
	if (cs == cs_configs_map.end()) return;
	const auto config = cs->second;
	const auto needle = config->server_name;
	if (strlen(needle) == 0 || needle[0] == '\0') return;
	const size_t len = strlen(name);
	const size_t len2 = strlen(needle);
	if (len > len2)
	{
		for (unsigned int i = 0; i < len; ++i) {
			if (name[i] == *needle) {
				// matched starting char -- loop through remaining chars
				const char* h, * n;
				for (h = &name[i], n = needle; *h && *n; ++h, ++n) {
					if (*h != *n) {
						break;
					}
				}
				if (!*n) {
					//已经有前缀
					return;
				}
			}
		}
	}
	char temp_name[NAME_LENGTH];
	safestrncpy(temp_name, name, NAME_LENGTH);
	memset(name, 0, NAME_LENGTH);
	strncpy(name, needle, strlen(needle));
	if(name[strlen(needle)] != '\0')
		name[strlen(needle)] = '\0';
	strncat(name, temp_name, NAME_LENGTH);
	return;
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


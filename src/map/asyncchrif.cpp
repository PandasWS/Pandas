#include <thread>

#include "asyncchrif.hpp"
#include "battle.hpp"

char_fd_status cfs;
std::thread* ch_main_thread;

using namespace std;

int m = 0;
std::mutex mt;
bool async_chrif_reconnect(int cs_id, const std::map<int, cross_server_data*>::iterator it);
#ifdef Pandas_Cross_Server
bool async_chrif_reconnect(int cs_id, const std::map<int, cross_server_data*>::iterator it)
{
	const auto config = it->second;
	chars_server_state* cf = cfs.find(cs_id);
	if (cf == nullptr || cf->get_char_fd() <= 0 || session[cf->get_char_fd()] == nullptr)
	{
		if (cf == nullptr)
		{
			CREATE(cf, class chars_server_state, 1);
			cf->set_csid(cs_id);
			cf->set_state(0);
			cf->set_connected(0);
			cfs.add(cs_id, cf);
		}
		if(cs_id  == 0) {
			ShowStatus("" CL_BLUE "[Cross Server]" CL_RESET "Attempting to connect to CS Char Server. Please wait.\n");
		} else {
			ShowStatus("" CL_BLUE "[Cross Server]" CL_RESET "Attempting to connect to Char Server [%d] Name [%s]. Please wait.\n", cs_id, config->server_name);
		}
		const int tfd = make_connection(host2ip(config->char_server_ip.c_str()), config->char_server_port, false, 10);
		if (tfd == -1)
			return true;

		session[tfd]->func_parse = chrif_parse;
		session[tfd]->flag.server = 1;
		realloc_fifo(tfd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

		chrif_connect(tfd, cs_id);
		cf->set_char_fd(tfd);
		cf->set_state(1);
		cf->set_connected(0);
	}
	return false;
}

TIMER_FUNC(chrif_runtime) {
	int index = 0;
	for (auto it = cs_configs_map.begin(); it != cs_configs_map.end() && index < MAX_CHAR_SERVERS; it++, index++)
	{
		const auto cs_id = it->first;
		async_chrif_reconnect(cs_id, it);
	}
	return 0;
}

void chrif_runtime_sub(void) {
	int index = 0;
	for (auto it = cs_configs_map.begin(); it != cs_configs_map.end() && index < MAX_CHAR_SERVERS; it++, index++)
	{
		const auto cs_id = it->first;
		async_chrif_reconnect(cs_id, it);
	}
}

void chrif_runtime2(void) {
	while (runflag != CORE_ST_STOP) {
		this_thread::sleep_for(chrono::milliseconds(1000));
		int index = 0;
		for (auto it = cs_configs_map.begin(); it != cs_configs_map.end() && index < MAX_CHAR_SERVERS; it++, index++)
		{
			const auto cs_id = it->first;
			async_chrif_reconnect(cs_id, it);
		}
	}
}

void asyncchrif_init(void) {
	int index = 0;
	for (auto it = cs_configs_map.begin(); it != cs_configs_map.end() && index < MAX_CHAR_SERVERS; it++, index++)
	{
		const auto cs_id = it->first;
		const auto config = it->second;
		chars_server_state* cf = cfs.find(cs_id);
		if(cf == nullptr || cf->get_char_fd() <= 0 || session[cf->get_char_fd()] == nullptr)
		{
			if (cf == nullptr)
			{
				CREATE(cf,class chars_server_state, 1);
				cf->set_csid(cs_id);
				cf->set_state(0);
				cf->set_connected(0);
				cfs.add(cs_id, cf);
			}
			if (cs_id == 0) {
				ShowStatus("" CL_BLUE "[Cross Server]" CL_RESET "Attempting to connect to CS Char Server. Please wait.\n");
			}
			else {
				ShowStatus("" CL_BLUE "[Cross Server]" CL_RESET "Attempting to connect to Char Server [%d] Name [%s]. Please wait.\n", cs_id, config->server_name);
			}
			const int tfd = make_connection(host2ip(config->char_server_ip.c_str()), config->char_server_port, false, 10);
			if (tfd == -1)
				continue;

			session[tfd]->func_parse = chrif_parse;
			session[tfd]->flag.server = 1;
			realloc_fifo(tfd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

			chrif_connect(tfd, cs_id);
			cf->set_char_fd(tfd);
			cf->set_state(1);
			cf->set_connected(0);
		}
		
	}
	//主线程阻塞检测
	if(battle_config.sync_every_char)
		add_timer_interval(gettick() + 1000, chrif_runtime, 0, 0, 10 * 1000);
	else
		ch_main_thread = new thread(chrif_runtime2);
}

void asyncchrif_final()
{
	cfs.destroy();
}
#endif

bool check_all_char_fd_health()
{
	return cfs.all_fd_valid();
}


bool chrif_set_cs_fd_state(int fd, int state) {
	const auto cf = cfs.findByFd(fd);
	if (cf == nullptr)
		return false;
	cf->set_state(state);
	return true;
}

//设置全局状态
void chrif_set_global_fd_state(int fd, int& char_fd, int& chrif_state, int& chrif_connected) {
	const auto cf = cfs.findByFd(fd);
	if (cf == nullptr)
		return;
	char_fd = fd;
	chrif_state = cf->get_state();
	chrif_connected = cf->get_connected();
}

//默认检查isValid
int check_fd_valid(int fd, int flag)
{
	if (fd <= 0 || (!flag && !session_isValid(fd))) return -1;
	const auto cf = cfs.findByFd(fd);
	if (cf == nullptr)
		return -1;
	return cf->get_char_fd();
}

int chrif_fd_isconnected(int fd) {
	const auto cf = cfs.findByFd(fd);
	if (cf == nullptr)
		return 0;
	return cf->get_state();
}

int chrif_get_char_fd(int cs_id) {
	const auto cf = cfs.find(cs_id);
	if (cf == nullptr)
		return -1;
	return cf->get_char_fd();
}

int chrif_get_cs_id(int map_fd) {
	const auto cf = cfs.findByFd(map_fd);
	if (cf == nullptr)
		return 0;
	return cf->get_csid() > 0 ? cf->get_csid() : 0;
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
	const auto cf = cfs.find(cs_id);
	if(cf == nullptr)
		return -1;
	int index = check_fd_valid(cf->get_char_fd());
	if (index == -1)
		return -1;
	chrif_state = cf->get_state();
	char_fd = cf->get_char_fd();
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

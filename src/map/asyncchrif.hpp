#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <future>


#include "../common/socket.hpp"
#include "../common/crossserver.hpp"
#include "../common/malloc.hpp"
#include "../common/core.hpp"


//char fd
extern bool cs_init_done;
constexpr auto MAX_CHAR_SERVERS = 5;

class chars_server_state
{
	int cs_id = -1;
	int char_fd = -1;
	int chrif_connected = 0;
	int chrif_state = 0;

public:
	int get_csid()
	{
		return this->cs_id;
	}

	int get_char_fd()
	{
		return this->char_fd;
	}

	int get_connected()
	{
		return this->chrif_connected;
	}

	int get_state()
	{
		return this->chrif_state;
	}

	void set_csid(int cs_id)
	{
		this->cs_id = cs_id;
	}

	void set_char_fd(int fd)
	{
		this->char_fd = fd;
	}

	void set_connected(int connected)
	{
		this->chrif_connected = connected;
	}

	void set_state(int state)
	{
		this->chrif_state = state;
	}
};

class char_fd_status
{
	std::map<int, chars_server_state*> charfd_status;

public:
	void add(int cs_id, chars_server_state* css)
	{
		this->charfd_status.insert(std::make_pair(cs_id, css));
	}

	void del(int cs_id)
	{
		auto it = this->charfd_status.find(cs_id);
		if (it != this->charfd_status.end())
			this->charfd_status.erase(it);
	}

	chars_server_state* find(int cs_id)
	{
		auto it = this->charfd_status.find(cs_id);
		return it == this->charfd_status.end() ? nullptr : it->second;
	}

	chars_server_state* findByFd(int fd)
	{
		auto it = this->charfd_status.begin();
		while (it != this->charfd_status.end())
		{
			const auto next = std::next(it);
			if(it->second->get_char_fd() == fd)
			{
				return it->second;
			}
			it = next;
		}
		return nullptr;
	}

	void destroy()
	{
		auto it = this->charfd_status.begin();
		while (it != this->charfd_status.end())
		{
			const auto next = std::next(it);
			aFree(it->second);
			it->second = nullptr;
			it = next;
		}
		this->charfd_status.clear();
	}

	bool fd_valid_by_cs_id(int cs_is)
	{
		const auto cf = this->find(cs_is);
		if (cf == nullptr) return false;
		return !(cf->get_char_fd() <= 0 || !session_isValid(cf->get_char_fd()));
	}

	bool all_fd_valid()
	{
		auto it = this->charfd_status.begin();
		while (it != this->charfd_status.end())
		{
			const auto next = std::next(it);
			const auto cf = it->second;
			if (cf->get_char_fd() <= 0 || !session_isValid(cf->get_char_fd()))
				return false;
			cf->set_state(2);
			cf->set_connected(1);
			it = next;
		}
		return true;
	}

	void get_valid_fds(int* fds)
	{
		auto it = this->charfd_status.begin();
		int index = 0;
		while (it != this->charfd_status.end())
		{
			const auto next = std::next(it);
			const auto cf = it->second;
			if (cf->get_char_fd() <= 0 || !session_isValid(cf->get_char_fd()))
			{
				it = next;
				continue;
			}
			fds[index] = cf->get_char_fd();
			it = next;
			index++;
		}
	}

	~char_fd_status()
	{
		this->destroy();
	}
};


extern char_fd_status cfs;
extern int chrif_parse(int fd);
#ifndef Pandas_Cross_Server
extern int chrif_connect(int fd);
#else
extern int chrif_connect(int fd, int cs_id);
#endif
extern void asyncchrif_init(void);
extern void asyncchrif_final(void);
extern bool check_all_char_fd_health(void);
extern bool chrif_set_cs_fd_state(int fd, int state);
extern void chrif_set_global_fd_state(int fd, int& char_fd, int& chrif_state, int& chrif_connected);
extern TIMER_FUNC(chrif_runtime);
void chrif_runtime_sub(void);
void chrif_runtime2(void);


int check_fd_valid(int fd, int flag = 0);
int chrif_fd_isconnected(int fd);
int chrif_get_char_fd(int cs_id);
int chrif_get_cs_id(int map_fd);
void chrif_set_global_fd_state(int fd, int& char_fd, int& chrif_state, int& chrif_connected);
int switch_char_fd(uint32 id, int& char_fd, int& chrif_state);
int switch_char_fd_cs_id(uint32 cs_id, int& char_fd);

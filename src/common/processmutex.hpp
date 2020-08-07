// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>
	#include <semaphore.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <string.h>
	#include <memory.h>
#endif // _WIN32

class ProcessMutex {
private:
#ifdef _WIN32
	void* m_pMutex;
#else
	sem_t* m_pSem;
#endif // _WIN32
	std::string m_MutexName;
public:
	ProcessMutex(const char* mutexName = nullptr);

	bool Lock();
	bool Unlock();

	~ProcessMutex();
};

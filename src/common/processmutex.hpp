// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <string>

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

	bool lock();
	bool unlock();
	int wait(long millisecond);

	~ProcessMutex();
};

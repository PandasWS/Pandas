// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "processmutex.hpp"

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <string.h>
	#include <memory.h>
	#include <time.h>
	#include <errno.h>
#endif // _WIN32

//************************************
// Method:      ProcessMutex
// Description: 构造函数
// Parameter:   const char * mutexName
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
ProcessMutex::ProcessMutex(const char* mutexName /* = nullptr */)
{
#ifdef _WIN32
	m_pMutex = nullptr;
	m_MutexName = mutexName;
	m_pMutex = CreateMutex(NULL, false, m_MutexName.c_str());
#else
	m_pSem = nullptr;
	m_MutexName = mutexName;
	m_pSem = sem_open(m_MutexName.c_str(), O_RDWR | O_CREAT, 0644, 1);
#endif // _WIN32
}

//************************************
// Method:      lock
// Description: 锁定互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
bool ProcessMutex::lock()
{
#ifdef _WIN32
	if (m_pMutex == nullptr) {
		return false;
	}

	DWORD nRet = WaitForSingleObject(m_pMutex, INFINITE);
	return (nRet == WAIT_OBJECT_0);
#else
	int ret = sem_wait(m_pSem);
	return (ret == 0);
#endif // _WIN32
}

//************************************
// Method:      unlock
// Description: 解锁互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
bool ProcessMutex::unlock()
{
#ifdef _WIN32
	return (ReleaseMutex(m_pMutex) != 0);
#else
	int ret = sem_post(m_pSem);
	return (ret == 0);
#endif // _WIN32
}

//************************************
// Method:      wait
// Description: 等待互斥量, 支持指定等待时长
// Parameter:   long millisecond
// Returns:     int	成功返回 0, 超时返回 1, 其他错误返回 2
// Author:      Sola丶小克(CairoLee)  2022/04/01 22:22
//************************************ 
int ProcessMutex::wait(long millisecond)
{
#ifdef _WIN32
	DWORD nRet = WaitForSingleObject(m_pMutex, (DWORD)millisecond);
	switch (nRet) {
	case WAIT_OBJECT_0:	return 0;	// 成功
	case WAIT_TIMEOUT: return 1;	// 超时
	default:
		return 2;
	}
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long secs = millisecond / 1000;
	millisecond = millisecond % 1000;

	long timeadd = 0;
	millisecond = millisecond * 1000 * 1000 + ts.tv_nsec;
	timeadd = millisecond / (1000 * 1000 * 1000);
	ts.tv_sec += (timeadd + secs);
	ts.tv_nsec = millisecond % (1000 * 1000 * 1000);

	int nRet = sem_timedwait(m_pSem, & ts);
	switch (nRet) {
	case 0: return 0;				// 成功
	case -1: {
		switch (errno) {
		case ETIMEDOUT: return 1;	// 超时
		default:
			return 2;
		}
	}
	default:
		return 2;
	}
#endif // _WIN32
}

//************************************
// Method:      ~ProcessMutex
// Description: 析构函数
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
ProcessMutex::~ProcessMutex()
{
#ifdef _WIN32
	if (m_pMutex) {
		CloseHandle(m_pMutex);
	}
#else
	int ret = sem_close(m_pSem);
	if (ret != 0) {
		printf("sem_close error %d\n", ret);
	}
	sem_unlink(m_MutexName.c_str());
#endif // _WIN32
}

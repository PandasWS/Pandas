// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "processmutex.hpp"

#ifdef _WIN32

//************************************
// Method:      ProcessMutex
// Description: 构造函数
// Parameter:   const char * mutexName
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
ProcessMutex::ProcessMutex(const char* mutexName /* = nullptr */) {
	m_pMutex = nullptr;
	m_MutexName = mutexName;
	m_pMutex = CreateMutex(NULL, false, m_MutexName.c_str());
}

//************************************
// Method:      Lock
// Description: 锁定互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
bool ProcessMutex::Lock() {
	if (m_pMutex == nullptr) {
		return false;
	}

	DWORD nRet = WaitForSingleObject(m_pMutex, INFINITE);
	return (nRet == WAIT_OBJECT_0);
}

//************************************
// Method:      Unlock
// Description: 解锁互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
bool ProcessMutex::Unlock() {
	return (ReleaseMutex(m_pMutex) != 0);
}

//************************************
// Method:      ~ProcessMutex
// Description: 析构函数
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
ProcessMutex::~ProcessMutex() {
	if (m_pMutex) {
		CloseHandle(m_pMutex);
	}
}
#else

//************************************
// Method:      ProcessMutex
// Description: 构造函数
// Parameter:   const char * mutexName
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
ProcessMutex::ProcessMutex(const char* mutexName /* = nullptr */) {
	m_pSem = nullptr;
	m_MutexName = mutexName;
	m_pSem = sem_open(m_MutexName.c_str(), O_RDWR | O_CREAT, 0644, 1);
}

//************************************
// Method:      Lock
// Description: 锁定互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:26
//************************************
bool ProcessMutex::Lock() {
	int ret = sem_wait(m_pSem);
	return (ret == 0);
}

//************************************
// Method:      Unlock
// Description: 解锁互斥量
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:27
//************************************
bool ProcessMutex::Unlock() {
	int ret = sem_post(m_pSem);
	return (ret == 0);
}

//************************************
// Method:      ~ProcessMutex
// Description: 析构函数
// Returns:     
// Author:      Sola丶小克(CairoLee)  2020/8/7 23:27
//************************************
ProcessMutex::~ProcessMutex() {
	int ret = sem_close(m_pSem);
	if (ret != 0) {
	    printf("sem_close error %d\n", ret);
	}
	sem_unlink(m_MutexName.c_str());
}

#endif // _WIN32

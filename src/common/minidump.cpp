// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "minidump.hpp"
#include "core.hpp"

#include <time.h>
#include <stdlib.h>

#define LEN_TIMESTAMP 15

// 此处涉及一个全局变量 create_fulldump
// 该变量已经在 core.hpp 中 extern, 实际的定义在 core.cpp 中 [Sola丶小克]

#if !defined(Pandas_Version)
	#define Pandas_Version "v0.0.0"
#endif // !defined(Pandas_Version)

inline BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
{
	WCHAR szFileName[_MAX_FNAME] = L"";
	if (pModuleName == 0)
		return FALSE;

	_wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

	if (wcsicmp(szFileName, L"ntdll") == 0)
		return TRUE;

	return FALSE;
}

inline BOOL CALLBACK MiniDumpCallback(
	PVOID							 pParam,
	const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT        pOutput)
{
	if (pInput == 0 || pOutput == 0)
		return FALSE;

	switch (pInput->CallbackType)
	{
	case ModuleCallback:
		if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
			if (!IsDataSectionNeeded(pInput->Module.FullPath))
				pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
	case IncludeModuleCallback:
	case IncludeThreadCallback:
	case ThreadCallback:
	case ThreadExCallback:
		return TRUE;
	default:;
	}

	return FALSE;
}

inline void CreateMiniDump(PEXCEPTION_POINTERS pep, LPCTSTR strFileName)
{
#ifdef Pandas_Crash_Report
	HANDLE hFile = CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		MINIDUMP_CALLBACK_INFORMATION mci;
		MINIDUMP_TYPE mdt;

		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = FALSE;

		mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
		mci.CallbackParam = 0;

		if (create_fulldump == 1)
			mdt = MiniDumpWithFullMemory;
		else
			mdt = MiniDumpNormal;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (pep != 0) ? &mdei : 0, NULL, &mci);

		CloseHandle(hFile);
	}
#endif // Pandas_Crash_Report
}

LONG __stdcall Pandas_UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
#ifdef Pandas_Crash_Report
	char szFileName[_MAX_FNAME] = "";
	char szDriverName[_MAX_FNAME] = "";
	char szDirName[_MAX_FNAME] = "";
	char szDumpFileName[_MAX_FNAME] = "";
	char timestamp[LEN_TIMESTAMP + 1] = "";
	time_t now;

	time(&now);
	strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", localtime(&now));
	timestamp[LEN_TIMESTAMP] = '\0';

	if (GetModuleFileName(NULL, szFileName, _MAX_FNAME)) {
		_splitpath(szFileName, szDriverName, szDirName, szFileName, NULL);
		sprintf(szDumpFileName, "%s%s%s-%s-%s.dmp", szDriverName, szDirName, szFileName, Pandas_Version, timestamp);
	}

	ShowError("============================================================\n");
	ShowError("This application has halted due to an unexpected error.\n");
	ShowError("A minidump file were saved to disk, you can find them here:\n");
	ShowError("%s\n", szDumpFileName);
	ShowError("Please report the crash in our website:\n");
	ShowError("https://github.com/PandasWS/Pandas\n");
	ShowError("We will solve this problem as soon as possible.\n");
	ShowError("============================================================\n");

	CreateMiniDump(pExceptionInfo, szDumpFileName);
	return EXCEPTION_EXECUTE_HANDLER;
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif // Pandas_Crash_Report
}

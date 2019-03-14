// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _MINIDUMP_HPP_
#define _MINIDUMP_HPP_

#include "winapi.hpp"
#include "showmsg.hpp"

#include "../config/pandas.hpp"
#include "../../3rdparty/dbghelp/include/dbghelp.h"

LONG __stdcall Pandas_UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo);

#endif // _MINIDUMP_HPP_

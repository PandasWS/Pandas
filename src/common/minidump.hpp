// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _MINIDUMP_HPP_
#define _MINIDUMP_HPP_

#include "winapi.hpp"
#include "showmsg.hpp"

#include "../config/rathena.hpp"
#include "../../3rdparty/dbghelp/include/dbghelp.h"

LONG __stdcall rAthenaCN_UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo);

#endif // _MINIDUMP_HPP_

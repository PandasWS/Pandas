// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <config/pandas.hpp>	// GIT_BRANCH, GIT_HASH

#ifndef CRASHRPT_APPID
	#define CRASHRPT_APPID ""
#endif // CRASHRPT_APPID

#ifndef CRASHRPT_PUBLICKEY
	#define CRASHRPT_PUBLICKEY ""
#endif // CRASHRPT_PUBLICKEY

void breakpad_initialize();
void breakpad_status();

// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef AUTH_HPP
#define AUTH_HPP

#include "http.hpp"
#include <common/cbasetypes.hpp>

bool isAuthorized(const Request &request, bool checkGuildLeader=false);

#ifdef Pandas_WebServer_Rewrite_Controller_HandlerFunc
bool isVaildCharacter(uint32 account_id, uint32 char_id);
bool isVaildAccount(uint32 account_id);
bool isPartyLeader(uint32 account_id, uint32 char_id);
#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc
#endif

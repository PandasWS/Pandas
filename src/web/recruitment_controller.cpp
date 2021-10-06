// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "recruitment_controller.hpp"

#include <string>

#include "../common/showmsg.hpp"
#include "../common/sql.hpp"

#include "auth.hpp"
#include "http.hpp"
#include "sqllock.hpp"
#include "web.hpp"

using namespace nlohmann;

HANDLER_FUNC(party_recruitment_add) {
	response_json(res, 404, 0);
}

HANDLER_FUNC(party_recruitment_del) {
	response_json(res, 404, 0);
}

HANDLER_FUNC(party_recruitment_get) {
	response_json(res, 404, 0);
}

HANDLER_FUNC(party_recruitment_list) {
	response_json(res, 404, 0);
}

HANDLER_FUNC(party_recruitment_search) {
	response_json(res, 404, 0);
}

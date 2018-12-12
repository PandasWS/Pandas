// Copyright (c) rAthenaCN Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "map_artisan.hpp"

#include "../common/strlib.hpp"
#include "../../3rdparty/pcre/include/pcre.h"

bool hasPet(const char* script, unsigned int* pet_mobid)
{
	pcre *re;
	pcre_extra *extra;
	const char *error;
	int erroffset, r = -1;
	int ovector[30] = { 0 };

	re = pcre_compile("(?i).*?pet (\\d{0,5}?);.*?", 0, &error, &erroffset, NULL);
	extra = pcre_study(re, 0, &error);
	r = pcre_exec(re, extra, script, (int)strlen(script), 0, 0, ovector, 30);

	if (r != PCRE_ERROR_NOMATCH && r == 2) {
		const char *substring_start = script + ovector[2 * 1];
		int substring_length = ovector[2 * 1 + 1] - ovector[2 * 1];
		char matched[1024] = { 0 };
		strncpy(matched, substring_start, substring_length);
		*pet_mobid = atoi(matched);
	}

	pcre_free(re);

	if (extra != NULL) pcre_free(extra);
	return (r != PCRE_ERROR_NOMATCH);
}

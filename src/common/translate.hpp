// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once
#pragma warning( disable : 26812 )

#include <string>
#include <vector>

#include <common/timer.hpp>
#include <common/cbasetypes.hpp>
#include <common/database.hpp>
#include <common/showmsg.hpp>
#include <common/utf8.hpp>
#include <common/assistant.hpp>

#define export_quote(x) #x
#define export_message_tag(a) this->m_tagsList.push_back({"[{"#a"}]", a});
#define export_defined_tag(a) this->m_quoteList.push_back({#a, export_quote(a)});

struct s_translate_item {
	std::string original;
	std::string translation;
};

struct s_message_tag {
	std::string name;
	std::string value;
};

class TranslateDB : public TypesafeYamlDatabase<std::string, s_translate_item> {
private:
	std::vector<s_message_tag> m_tagsList;
	std::vector<s_message_tag> m_quoteList;

	void parseTags(std::string& message);
public:
	TranslateDB() : TypesafeYamlDatabase("CONSOLE_TRANSLATE_DB", 1) {
#ifdef Pandas_Database_Yaml_BeQuiet
		this->quietLevel = 1;
#endif // Pandas_Database_Yaml_BeQuiet

		export_message_tag(CL_RESET);
		export_message_tag(CL_CLS);
		export_message_tag(CL_CLL);

		export_message_tag(CL_BOLD);
		export_message_tag(CL_NORM);
		export_message_tag(CL_NORMAL);
		export_message_tag(CL_NONE);

		export_message_tag(CL_WHITE);
		export_message_tag(CL_GRAY);
		export_message_tag(CL_RED);
		export_message_tag(CL_GREEN);
		export_message_tag(CL_YELLOW);
		export_message_tag(CL_BLUE);
		export_message_tag(CL_MAGENTA);
		export_message_tag(CL_CYAN);

		export_message_tag(CL_BG_BLACK);
		export_message_tag(CL_BG_RED);
		export_message_tag(CL_BG_GREEN);
		export_message_tag(CL_BG_YELLOW);
		export_message_tag(CL_BG_BLUE);
		export_message_tag(CL_BG_MAGENTA);
		export_message_tag(CL_BG_CYAN);
		export_message_tag(CL_BG_WHITE);

		export_message_tag(CL_LT_BLACK);
		export_message_tag(CL_LT_RED);
		export_message_tag(CL_LT_GREEN);
		export_message_tag(CL_LT_YELLOW);
		export_message_tag(CL_LT_BLUE);
		export_message_tag(CL_LT_MAGENTA);
		export_message_tag(CL_LT_CYAN);
		export_message_tag(CL_LT_WHITE);

		export_message_tag(CL_BT_BLACK);
		export_message_tag(CL_BT_RED);
		export_message_tag(CL_BT_GREEN);
		export_message_tag(CL_BT_YELLOW);
		export_message_tag(CL_BT_BLUE);
		export_message_tag(CL_BT_MAGENTA);
		export_message_tag(CL_BT_CYAN);
		export_message_tag(CL_BT_WHITE);

		export_message_tag(CL_WTBL);
		export_message_tag(CL_XXBL);
		export_message_tag(CL_PASS);

		export_message_tag(CL_SPACE);

		export_message_tag(PRtf);
		export_message_tag(PRIuPTR);
		export_message_tag(PRIdPTR);
		export_message_tag(PRIu64);
		export_message_tag(PRId64);
		export_message_tag(PRIu32);
		export_message_tag(PRId32);
		export_message_tag(PRIXPTR);

		export_defined_tag(PACKETVER);
	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const ryml::NodeRef& node) override;
	void showStatus();
};

// extern TranslateDB translate_db;

void do_init_translate();
void translate_reload();
void do_final_translate();

void translate(std::string& original);
std::string translate(const char* original);
void translate_status();

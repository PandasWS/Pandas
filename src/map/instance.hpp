// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"
#include "../common/mmo.hpp"

#include "script.hpp"

enum send_target : uint8;
struct block_list;

extern int16 instance_start;
extern int instance_count;

#define INSTANCE_NAME_LENGTH (60+1)

enum e_instance_state : uint8 {
	INSTANCE_IDLE,
	INSTANCE_BUSY
};

enum e_instance_mode : uint8 {
	IM_NONE,
	IM_CHAR,
	IM_PARTY,
	IM_GUILD,
	IM_CLAN,
	IM_MAX,
};

enum e_instance_enter : uint8 {
	IE_OK,
	IE_NOMEMBER,
	IE_NOINSTANCE,
	IE_OTHER
};

enum e_instance_notify : uint8 {
	IN_NOTIFY = 0,
	IN_DESTROY_LIVE_TIMEOUT,
	IN_DESTROY_ENTER_TIMEOUT,
	IN_DESTROY_USER_REQUEST,
	IN_CREATE_FAIL,
};

struct s_instance_map {
	int16 m, src_m;
};

/// Instance data
struct s_instance_data {
	int id; ///< Instance DB ID
	e_instance_state state; ///< State of instance
	e_instance_mode mode; ///< Mode of instance
	int owner_id; ///< Owner ID of instance
	unsigned int keep_limit; ///< Life time of instance
	int keep_timer; ///< Life time ID
	unsigned int idle_limit; ///< Idle time of instance
	int idle_timer; ///< Idle timer ID
	struct reg_db regs; ///< Instance variables for scripts
	std::vector<s_instance_map> map; ///< Array of maps in instance

	s_instance_data() :
		id(0),
		state(INSTANCE_IDLE),
		mode(IM_PARTY),
		owner_id(0),
		keep_limit(0),
		keep_timer(INVALID_TIMER),
		idle_limit(0),
		idle_timer(INVALID_TIMER),
		regs(),
		map() { }
};

/// Instance DB entry
struct s_instance_db {
	int id; ///< Instance DB ID
	std::string name; ///< Instance name
	uint32 limit, ///< Duration limit
		timeout; ///< Timeout limit
	bool destroyable; ///< Destroyable flag
	struct point enter; ///< Instance entry point
	std::vector<int16> maplist; ///< Maps in instance
};

class InstanceDatabase : public TypesafeYamlDatabase<int32, s_instance_db> {
public:
	InstanceDatabase() : TypesafeYamlDatabase("INSTANCE_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node &node);
};

extern InstanceDatabase instance_db;

extern std::unordered_map<int, std::shared_ptr<s_instance_data>> instances;

std::shared_ptr<s_instance_db> instance_search_db_name(const char* name);
void instance_getsd(int instance_id, struct map_session_data *&sd, enum send_target *target);

int instance_create(int owner_id, const char *name, e_instance_mode mode);
bool instance_destroy(int instance_id);
void instance_destroy_command(map_session_data *sd);
e_instance_enter instance_enter(struct map_session_data *sd, int instance_id, const char *name, short x, short y);
bool instance_reqinfo(struct map_session_data *sd, int instance_id);
bool instance_addusers(int instance_id);
bool instance_delusers(int instance_id);
void instance_generate_mapname(int map_id, int instance_id, char outname[MAP_NAME_LENGTH]);
int16 instance_mapid(int16 m, int instance_id);
int instance_addmap(int instance_id);

void instance_addnpc(std::shared_ptr<s_instance_data> idata);

void do_reload_instance(void);
void do_init_instance(void);
void do_final_instance(void);

#endif /* INSTANCE_HPP */

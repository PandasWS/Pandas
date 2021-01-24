// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef QUEST_HPP
#define QUEST_HPP

#include <string>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"
#include "../common/strlib.hpp"

#include "map.hpp"

#ifdef Pandas_YamlBlastCache_Serialize
#include "../common/serialize.hpp"
#endif // Pandas_YamlBlastCache_Serialize

struct map_session_data;
enum e_size : uint8;

struct s_quest_dropitem {
	t_itemid nameid;
	uint16 count;
	uint16 rate;
	uint16 mob_id;
	//uint8 bound;
	//bool isAnnounced;
	//bool isGUID;
};

struct s_quest_objective {
	uint16 index;
	uint16 mob_id;
	uint16 count;
	uint16 min_level, max_level;
	e_race race;
	e_size size;
	e_element element;
	int16 mapid;
	std::string map_name;
};

struct s_quest_db {
	int32 id;
	time_t time;
	bool time_at;
	std::vector<std::shared_ptr<s_quest_objective>> objectives;
	std::vector<std::shared_ptr<s_quest_dropitem>> dropitem;
	std::string name;
};

// Questlog check types
enum e_quest_check_type : uint8 {
	HAVEQUEST, ///< Query the state of the given quest
	PLAYTIME,  ///< Check if the given quest has been completed or has yet to expire
	HUNTING,   ///< Check if the given hunting quest's requirements have been met
};

class QuestDatabase : public TypesafeYamlDatabase<uint32, s_quest_db> {
#ifdef Pandas_YamlBlastCache_QuestDatabase
private:
	friend class boost::serialization::access;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& boost::serialization::base_object<TypesafeYamlDatabase<uint32, s_quest_db>>(*this);
	}
#endif // Pandas_YamlBlastCache_QuestDatabase
public:
	QuestDatabase() : TypesafeYamlDatabase("QUEST_DB", 2) {
#ifdef Pandas_YamlBlastCache_QuestDatabase
		this->supportSerialize = true;
#endif // Pandas_YamlBlastCache_QuestDatabase
	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
	bool reload();

#ifdef Pandas_YamlBlastCache_QuestDatabase
	bool doSerialize(const std::string& type, void* archive);
#endif // Pandas_YamlBlastCache_QuestDatabase
};

extern QuestDatabase quest_db;

int quest_pc_login(struct map_session_data *sd);

int quest_add(struct map_session_data *sd, int quest_id);
int quest_delete(struct map_session_data *sd, int quest_id);
int quest_change(struct map_session_data *sd, int qid1, int qid2);
int quest_update_objective_sub(struct block_list *bl, va_list ap);
void quest_update_objective(struct map_session_data *sd, struct mob_data* md);
int quest_update_status(struct map_session_data *sd, int quest_id, e_quest_state status);
int quest_check(struct map_session_data *sd, int quest_id, e_quest_check_type type);

std::shared_ptr<s_quest_db> quest_search(int quest_id);

void do_init_quest(void);
void do_final_quest(void);

#ifdef Pandas_YamlBlastCache_QuestDatabase
namespace boost {
	namespace serialization {
		// ======================================================================
		// struct s_quest_db
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_db& t, const unsigned int version)
		{
			ar& t.id;
			ar& t.time;
			ar& t.time_at;
			ar& t.objectives;
			ar& t.dropitem;
			ar& t.name;
		}

		// ======================================================================
		// struct s_quest_objective
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_objective& t, const unsigned int version)
		{
			ar& t.index;
			ar& t.mob_id;
			ar& t.count;
			ar& t.min_level;
			ar& t.max_level;
			ar& t.race;
			ar& t.size;
			ar& t.element;
		}

		// ======================================================================
		// struct s_quest_dropitem
		// ======================================================================

		template <typename Archive>
		void serialize(Archive& ar, struct s_quest_dropitem& t, const unsigned int version)
		{
			ar& t.nameid;
			ar& t.count;
			ar& t.rate;
			ar& t.mob_id;
		}
	} // namespace serialization
} // namespace boost
#endif // Pandas_YamlBlastCache_QuestDatabase

#endif /* QUEST_HPP */

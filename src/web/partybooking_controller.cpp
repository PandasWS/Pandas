// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "partybooking_controller.hpp"

#include <string>

#include <common/showmsg.hpp>
#include <common/sql.hpp>
#include <common/strlib.hpp>

#include "http.hpp"
#include "auth.hpp"
#include "sqllock.hpp"
#include "web.hpp"

#ifndef Pandas_WebServer_Rewrite_Controller_HandlerFunc

const size_t WORLD_NAME_LENGTH = 32;
const size_t COMMENT_LENGTH = 255;

enum e_booking_purpose : uint16{
	BOOKING_PURPOSE_ALL = 0,
	BOOKING_PURPOSE_QUEST,
	BOOKING_PURPOSE_FIELD,
	BOOKING_PURPOSE_DUNGEON,
	BOOKING_PURPOSE_MD,
	BOOKING_PURPOSE_PARADISE,
	BOOKING_PURPOSE_OTHER,
	BOOKING_PURPOSE_MAX
};

struct s_party_booking_entry{
	uint32 account_id;
	uint32 char_id;
	std::string char_name;
	uint16 purpose;
	bool assist;
	bool damagedealer;
	bool healer;
	bool tanker;
	uint16 minimum_level;
	uint16 maximum_level;
	std::string comment;

public:
	std::string to_json( std::string& world_name );
};

std::string s_party_booking_entry::to_json( std::string& world_name ){
	return
		"{ \"AID\": " + std::to_string( this->account_id ) +
		", \"GID\": " + std::to_string( this->char_id ) +
		", \"CharName\": \"" + this->char_name + "\""
		", \"WorldName\": \"" + world_name + "\""
		", \"Tanker\": " + ( this->tanker ? "1": "0" ) +
		", \"Healer\": " + ( this->healer ? "1": "0" ) +
		", \"Dealer\": " + ( this->damagedealer ? "1" : "0" ) +
		", \"Assist\": " + ( this->assist ? "1" : "0" ) +
		", \"MinLV\": " + std::to_string( this->minimum_level ) +
		", \"MaxLV\": " + std::to_string( this->maximum_level ) +
		", \"Memo\": \"" + this->comment + "\""
		", \"Type\": " + std::to_string( this->purpose ) +
		"}";
}

bool party_booking_read( std::string& world_name, std::vector<s_party_booking_entry>& output, const std::string& condition, const std::string& order ){
	SQLLock sl(MAP_SQL_LOCK);
	sl.lock();
	auto handle = sl.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc( handle );
	s_party_booking_entry entry;
	char world_name_escaped[WORLD_NAME_LENGTH * 2 + 1];
	char char_name[NAME_LENGTH ];
	char comment[COMMENT_LENGTH + 1];

	Sql_EscapeString( nullptr, world_name_escaped, world_name.c_str() );

	std::string query = "SELECT `account_id`, `char_id`, `char_name`, `purpose`, `assist`, `damagedealer`, `healer`, `tanker`, `minimum_level`, `maximum_level`, `comment` FROM `" + std::string( partybookings_table ) + "` WHERE `world_name` = ? AND " + condition + order;

	if( SQL_SUCCESS != SqlStmt_Prepare( stmt, query.c_str() )
		|| SQL_SUCCESS != SqlStmt_BindParam( stmt, 0, SQLDT_STRING, (void*)world_name_escaped, strlen( world_name_escaped ) )
		|| SQL_SUCCESS != SqlStmt_Execute( stmt )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 0, SQLDT_UINT32, &entry.account_id, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 1, SQLDT_UINT32, &entry.char_id, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 2, SQLDT_STRING, (void*)char_name, sizeof( char_name ), NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 3, SQLDT_UINT16, &entry.purpose, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 4, SQLDT_UINT8, &entry.assist, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 5, SQLDT_UINT8, &entry.damagedealer, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 6, SQLDT_UINT8, &entry.healer, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 7, SQLDT_UINT8, &entry.tanker, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 8, SQLDT_UINT16, &entry.minimum_level, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 9, SQLDT_UINT16, &entry.maximum_level, 0, NULL, NULL )
		|| SQL_SUCCESS != SqlStmt_BindColumn( stmt, 10, SQLDT_STRING, (void*)comment, sizeof( comment ), NULL, NULL )
	){
		SqlStmt_ShowDebug( stmt );
		SqlStmt_Free( stmt );
		sl.unlock();
		return false;
	}

	while( SQL_SUCCESS == SqlStmt_NextRow( stmt ) ){
		entry.char_name = char_name;
		entry.comment = comment;

		output.push_back( entry );
	}

	SqlStmt_Free( stmt );
	sl.unlock();

	return true;
}

HANDLER_FUNC(partybooking_add){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	uint32 aid = std::stoi( req.get_file_value( "AID" ).content );
	uint32 cid = std::stoi( req.get_file_value( "GID" ).content );

	SQLLock csl( CHAR_SQL_LOCK );
	csl.lock();
	auto chandle = csl.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc( chandle );
	if( SQL_SUCCESS != SqlStmt_Prepare( stmt, "SELECT 1 FROM `%s` WHERE `leader_id` = ? AND `leader_char` = ?", party_table, aid, cid )
		|| SQL_SUCCESS != SqlStmt_BindParam( stmt, 0, SQLDT_UINT32, &aid, sizeof( aid ) )
		|| SQL_SUCCESS != SqlStmt_BindParam( stmt, 1, SQLDT_UINT32, &cid, sizeof( cid ) )
		|| SQL_SUCCESS != SqlStmt_Execute( stmt )
	){
		SqlStmt_ShowDebug( stmt );
		SqlStmt_Free( stmt );
		csl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	if( SqlStmt_NumRows( stmt ) <= 0 ){
		// No party or not party leader
		SqlStmt_Free( stmt );
		csl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	SqlStmt_Free( stmt );
	csl.unlock();

	auto world_name = req.get_file_value( "WorldName" ).content;

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	s_party_booking_entry entry = {};

	entry.account_id = aid;
	entry.char_id = cid;
	entry.char_name = req.get_file_value( "CharName" ).content;
	entry.purpose = std::stoi( req.get_file_value( "Type" ).content );
	entry.assist = std::stoi( req.get_file_value( "Assist" ).content ) != 0;
	entry.damagedealer = std::stoi( req.get_file_value( "Dealer" ).content ) != 0;
	entry.healer = std::stoi( req.get_file_value( "Healer" ).content ) != 0;
	entry.tanker = std::stoi( req.get_file_value( "Tanker" ).content ) != 0;
	entry.minimum_level = std::stoi( req.get_file_value( "MinLV" ).content );
	entry.maximum_level = std::stoi( req.get_file_value( "MaxLV" ).content );
	entry.comment = req.get_file_value( "Memo" ).content;

	if( entry.char_name.length() > NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	if( entry.purpose >= BOOKING_PURPOSE_MAX ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	if( entry.comment.length() > COMMENT_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	char world_name_escaped[WORLD_NAME_LENGTH * 2 + 1];
	char char_name_escaped[NAME_LENGTH * 2 + 1];
	char comment_escaped[COMMENT_LENGTH * 2 + 1];

	Sql_EscapeString( nullptr, world_name_escaped, world_name.c_str() );
	Sql_EscapeString( nullptr, char_name_escaped, entry.char_name.c_str() );
	Sql_EscapeString( nullptr, comment_escaped, entry.comment.c_str() );

	StringBuf buf;

	StringBuf_Init( &buf );

	StringBuf_Printf( &buf, "REPLACE INTO `%s` ( `world_name`, `account_id`, `char_id`, `char_name`, `purpose`, `assist`, `damagedealer`, `healer`, `tanker`, `minimum_level`, `maximum_level`, `comment` ) VALUES ( ", partybookings_table );

	StringBuf_Printf( &buf, "'%s',", world_name_escaped );

	StringBuf_Printf( &buf, "'%u',", entry.account_id );
	StringBuf_Printf( &buf, "'%u',", entry.char_id );
	StringBuf_Printf( &buf, "'%s',", char_name_escaped );
	StringBuf_Printf( &buf, "'%hu',", entry.purpose );
	StringBuf_Printf( &buf, "'%d',", entry.assist );
	StringBuf_Printf( &buf, "'%d',", entry.damagedealer );
	StringBuf_Printf( &buf, "'%d',", entry.healer );
	StringBuf_Printf( &buf, "'%d',", entry.tanker );
	StringBuf_Printf( &buf, "'%hu',", entry.minimum_level );
	StringBuf_Printf( &buf, "'%hu',", entry.maximum_level );
	StringBuf_Printf( &buf, "'%s' );", comment_escaped );

	SQLLock msl( MAP_SQL_LOCK );
	msl.lock();
	auto mhandle = msl.getHandle();

	if( SQL_ERROR == Sql_QueryStr( mhandle, StringBuf_Value( &buf ) ) ){
		Sql_ShowDebug( mhandle );

		StringBuf_Destroy( &buf );
		msl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	StringBuf_Destroy( &buf );
	msl.unlock();

	res.set_content( "{ \"Type\": 1 }", "application/json" );
}

HANDLER_FUNC(partybooking_delete){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	auto world_name = req.get_file_value( "WorldName" ).content;
	auto account_id = std::stoi( req.get_file_value( "AID" ).content );

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	SQLLock sl( MAP_SQL_LOCK );
	sl.lock();
	auto handle = sl.getHandle();

	if( SQL_ERROR == Sql_Query( handle, "DELETE FROM `%s` WHERE `world_name` = '%s' AND `account_id` = '%d'", partybookings_table, world_name.c_str(), account_id ) ){
		Sql_ShowDebug( handle );

		sl.unlock();
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	sl.unlock();

	res.set_content( "{ \"Type\": 1 }", "application/json" );
}

HANDLER_FUNC(partybooking_get){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	auto world_name = req.get_file_value( "WorldName" ).content;
	auto account_id = std::stoi( req.get_file_value( "AID" ).content );

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::vector<s_party_booking_entry> bookings;

	if( !party_booking_read( world_name, bookings, "`account_id` = '" + std::to_string( account_id ) + "'", "" ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::string response;

	if( bookings.empty() ){
		response = "{ \"Type\": 1 }";
	}else{
		response = "{ \"Type\": 1, data: " + bookings.at( 0 ).to_json( world_name ) + " }";
	}

	res.set_content( response, "application/json" );
}

HANDLER_FUNC(partybooking_info){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	auto world_name = req.get_file_value( "WorldName" ).content;
	auto account_id = std::stoi( req.get_file_value( "QueryAID" ).content );

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::vector<s_party_booking_entry> bookings;

	if( !party_booking_read( world_name, bookings, "`account_id` = '" + std::to_string( account_id ) + "'", "" ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::string response;

	if( bookings.empty() ){
		response = "{ \"Type\": 1 }";
	}else{
		response = "{ \"Type\": 1, \"data\": [" + bookings.at( 0 ).to_json( world_name ) + "] }";
	}

	res.set_content( response, "application/json" );
}

HANDLER_FUNC(partybooking_list){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	static const std::string condition( "1=1" );

	auto world_name = req.get_file_value( "WorldName" ).content;

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::vector<s_party_booking_entry> bookings;

	if( !party_booking_read( world_name, bookings, condition, " ORDER BY `created` DESC" ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::string response;

	response = "{ \"Type\": 1, \"totalPage\": ";
	response += std::to_string( bookings.size() );
	response += ", \"data\": [";

	for( size_t i = 0, max = bookings.size(); i < max; i++ ){
		s_party_booking_entry& booking = bookings.at( i );

		response += booking.to_json( world_name );

		if( i < ( max - 1 ) ){
			response += ", ";
		}
	}

	response += "] }";

	res.set_content( response, "application/json" );
}

HANDLER_FUNC(partybooking_search){
	if( !isAuthorized( req, false ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );
		return;
	}

	auto world_name = req.get_file_value( "WorldName" ).content;

	if( world_name.length() > WORLD_NAME_LENGTH ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	s_party_booking_entry entry;

	// Unconditional
	entry.minimum_level = std::stoi( req.get_file_value( "MinLV" ).content );
	entry.maximum_level = std::stoi( req.get_file_value( "MaxLV" ).content );

	// Conditional
	if( req.files.find( "Type" ) != req.files.end() ){
		entry.purpose = std::stoi( req.get_file_value( "Type" ).content );

		if( entry.purpose >= BOOKING_PURPOSE_MAX ){
			res.status = HTTP_BAD_REQUEST;
			res.set_content( "Error", "text/plain" );

			return;
		}
	}else{
		entry.purpose = BOOKING_PURPOSE_ALL;
	}

	if( req.files.find( "Assist" ) != req.files.end() ){
		entry.assist = std::stoi( req.get_file_value( "Assist" ).content ) != 0;
	}else{
		entry.assist = false;
	}

	if( req.files.find( "Dealer" ) != req.files.end() ){
		entry.damagedealer = std::stoi( req.get_file_value( "Dealer" ).content ) != 0;
	}else{
		entry.damagedealer = false;
	}

	if( req.files.find( "Healer" ) != req.files.end() ){
		entry.healer = std::stoi( req.get_file_value( "Healer" ).content ) != 0;
	}else{
		entry.healer = false;
	}

	if( req.files.find( "Tanker" ) != req.files.end() ){
		entry.tanker = std::stoi( req.get_file_value( "Tanker" ).content ) != 0;
	}else{
		entry.tanker = false;
	}

	if( req.files.find( "Memo" ) != req.files.end() ){
		entry.comment = req.get_file_value( "Memo" ).content;
	}else{
		entry.comment = "";
	}

	std::string condition;

	condition = "`minimum_level` = '" + std::to_string( entry.minimum_level ) + "'";
	condition += " AND `maximum_level` = '" + std::to_string( entry.maximum_level ) + "'";

	if( entry.purpose != BOOKING_PURPOSE_ALL ){
		condition += " AND `purpose` = '" + std::to_string( entry.purpose ) + "'";
	}

	if( entry.assist || entry.damagedealer || entry.healer || entry.tanker ){
		bool or_required = false;

		condition += "AND ( ";

		if( entry.assist ){
			if( or_required ){
				condition += " OR ";
			}else{
				or_required = true;
			}

			condition += "`assist` = '1'";
		}

		if( entry.damagedealer ){
			if( or_required ){
				condition += " OR ";
			}else{
				or_required = true;
			}

			condition += "`damagedealer` = '1'";
		}

		if( entry.healer ){
			if( or_required ){
				condition += " OR ";
			}else{
				or_required = true;
			}

			condition += "`healer` = '1'";
		}

		if( entry.tanker ){
			if( or_required ){
				condition += " OR ";
			}else{
				or_required = true;
			}

			condition += "`tanker` = '1'";
		}

		condition += " )";
	}

	if( !entry.comment.empty() ){
		if( entry.comment.length() > COMMENT_LENGTH ){
			res.status = HTTP_BAD_REQUEST;
			res.set_content( "Error", "text/plain" );

			return;
		}

		char escaped_comment[COMMENT_LENGTH * 2 + 1];

		Sql_EscapeString( nullptr, escaped_comment, entry.comment.c_str() );

		condition += " AND `comment` like '%" + std::string( escaped_comment ) + "%'";
	}

	std::vector<s_party_booking_entry> bookings;

	if( !party_booking_read( world_name, bookings, condition, " ORDER BY `created` DESC" ) ){
		res.status = HTTP_BAD_REQUEST;
		res.set_content( "Error", "text/plain" );

		return;
	}

	std::string response;

	response = "{ \"Type\": 1, \"totalPage\": ";
	response += std::to_string( bookings.size() );
	response += ", \"data\": [";

	for( size_t i = 0, max = bookings.size(); i < max; i++ ){
		s_party_booking_entry& booking = bookings.at( i );

		response += booking.to_json( world_name );

		if( i < ( max - 1 ) ){
			response += ", ";
		}
	}

	response += "] }";

	res.set_content( response, "application/json" );
}

#else

using namespace nlohmann;

#define SUCCESS_RET 1
#define FAILURE_RET -3
#define REQUIRE_FIELD_EXISTS(x) REQUIRE_FIELD_EXISTS_R(x)

// 设置冒险者中介所每个页面能显示的最大记录数
// 经过测试 20211117 客户端并不支持滚动条, 所以每个页面最多显示 10 条记录
const size_t SINGLE_PAGESIZE = 10;

const size_t COMMENT_LENGTH = 255;

struct s_party_booking_entry {
	uint32 account_id = 0;
	uint32 char_id = 0;
	std::string char_name;
	uint16 purpose = 0;
	bool assist = false;
	bool damagedealer = false;
	bool healer = false;
	bool tanker = false;
	uint16 minimum_level = 0;
	uint16 maximum_level = 0;
	std::string comment;

public:
	json to_json(std::string& world_name);
};

json s_party_booking_entry::to_json(std::string& world_name) {
	json content = {
		{"AID", this->account_id},
		{"GID", this->char_id},
		{"CharName", A2UWE(this->char_name)},
		{"WorldName", A2UWE(world_name)},
		{"Tanker", (this->tanker ? 1 : 0)},
		{"Healer", (this->healer ? 1 : 0)},
		{"Dealer", (this->damagedealer ? 1 : 0)},
		{"Assist", (this->assist ? 1 : 0)},
		{"MinLV", this->minimum_level},
		{"MaxLV", this->maximum_level},
		{"Memo", A2UWE(this->comment)},
		{"Type", this->purpose},
	};
	return content;
}

bool party_booking_read(std::string& world_name, std::vector<s_party_booking_entry>& output,
	const std::string& condition, const std::string& order = "", const std::string& limit = ""
) {
	s_party_booking_entry entry;
	char char_name[NAME_LENGTH] = { 0 };
	char comment[COMMENT_LENGTH + 1] = { 0 };
	
	SQLLock maplock(MAP_SQL_LOCK);
	maplock.lock();
	auto handle = maplock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	std::string query = "SELECT `account_id`, `char_id`, `char_name`, `purpose`, `assist`, `damagedealer`, `healer`, "
		"`tanker`, `minimum_level`, `maximum_level`, `comment` FROM `" + std::string(partybookings_table) +
		"` WHERE `world_name` = ? AND " + condition + order + limit;

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt, query.c_str())
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &entry.account_id, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 1, SQLDT_UINT32, &entry.char_id, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 2, SQLDT_STRING, (void*)char_name, sizeof(char_name), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 3, SQLDT_UINT16, &entry.purpose, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 4, SQLDT_UINT8, &entry.assist, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 5, SQLDT_UINT8, &entry.damagedealer, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 6, SQLDT_UINT8, &entry.healer, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 7, SQLDT_UINT8, &entry.tanker, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 8, SQLDT_UINT16, &entry.minimum_level, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 9, SQLDT_UINT16, &entry.maximum_level, 0, NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 10, SQLDT_STRING, (void*)comment, sizeof(comment), NULL, NULL)
		) {
		SqlStmt_ShowDebug(stmt);
		SqlStmt_Free(stmt);
		maplock.unlock();
		return false;
	}

	while (SQL_SUCCESS == SqlStmt_NextRow(stmt)) {
		char_name[NAME_LENGTH - 1] = '\0';
		comment[COMMENT_LENGTH - 1] = '\0';
		
		entry.char_name = char_name;
		entry.comment = comment;

		output.push_back(entry);
	}

	SqlStmt_Free(stmt);
	maplock.unlock();

	return true;
}

int party_booking_count(std::string& world_name, const std::string& condition) {
	uint32 record_count = 0;

	SQLLock maplock(MAP_SQL_LOCK);
	maplock.lock();
	auto handle = maplock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	std::string query = "SELECT COUNT(*) FROM `%s` WHERE `world_name` = ? AND " + condition;
	
	if (SQL_SUCCESS != SqlStmt_Prepare(stmt, query.c_str(), partybookings_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		|| SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0, SQLDT_UINT32, &record_count, sizeof(record_count), NULL, NULL)
		|| SQL_SUCCESS != SqlStmt_NextRow(stmt)) {
		SqlStmt_ShowDebug(stmt);
	}
	
	SqlStmt_Free(stmt);
	maplock.unlock();
	return record_count;
}

std::string party_booking_class_condition(bool tanker, bool healer, bool damagedealer, bool assist) {
	std::string condition;

	if (assist || damagedealer || healer || tanker) {
		bool or_required = false;

		condition += " AND ( ";

		if (assist) {
			if (or_required) {
				condition += " OR ";
			}
			else {
				or_required = true;
			}

			condition += "`assist` = '1'";
		}

		if (damagedealer) {
			if (or_required) {
				condition += " OR ";
			}
			else {
				or_required = true;
			}

			condition += "`damagedealer` = '1'";
		}

		if (healer) {
			if (or_required) {
				condition += " OR ";
			}
			else {
				or_required = true;
			}

			condition += "`healer` = '1'";
		}

		if (tanker) {
			if (or_required) {
				condition += " OR ";
			}
			else {
				or_required = true;
			}

			condition += "`tanker` = '1'";
		}

		condition += " )";
	}

	return condition;
}

HANDLER_FUNC(partybooking_add) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("CharName");
	REQUIRE_FIELD_EXISTS("Memo");
	REQUIRE_FIELD_EXISTS("MinLV");
	REQUIRE_FIELD_EXISTS("MaxLV");
	REQUIRE_FIELD_EXISTS("Tanker");
	REQUIRE_FIELD_EXISTS("Healer");
	REQUIRE_FIELD_EXISTS("Dealer");
	REQUIRE_FIELD_EXISTS("Assist");
	REQUIRE_FIELD_EXISTS("Type");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto char_name = GET_STRING_FIELD("CharName", "");
	auto memo = GET_STRING_FIELD("Memo", "");
	auto minimum_level = GET_NUMBER_FIELD("MinLV", 0);
	auto maximum_level = GET_NUMBER_FIELD("MaxLV", 0);
	auto tanker = GET_NUMBER_FIELD("Tanker", 0);
	auto healer = GET_NUMBER_FIELD("Healer", 0);
	auto damagedealer = GET_NUMBER_FIELD("Dealer", 0);
	auto assist = GET_NUMBER_FIELD("Assist", 0);
	auto purpose = GET_NUMBER_FIELD("Type", 0);

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist.");
		return;
	}

	if (!isPartyLeader(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" must be the party leader.");
		return;
	}
	
	SQLLock maplock(MAP_SQL_LOCK);
	maplock.lock();
	auto handle = maplock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"REPLACE INTO `%s` (`world_name`, `account_id`, `char_id`, `char_name`, `purpose`, `assist`, "
		"`damagedealer`, `healer`, `tanker`, `minimum_level`, `maximum_level`, `comment`"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
		partybookings_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_UINT32, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_UINT32, &char_id, sizeof(char_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, (void*)char_name.c_str(), strlen(char_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 4, SQLDT_UINT16, &purpose, sizeof(purpose))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 5, SQLDT_UINT8, &assist, sizeof(assist))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 6, SQLDT_UINT8, &damagedealer, sizeof(damagedealer))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 7, SQLDT_UINT8, &healer, sizeof(healer))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 8, SQLDT_UINT8, &tanker, sizeof(tanker))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 9, SQLDT_UINT16, &minimum_level, sizeof(minimum_level))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 10, SQLDT_UINT16, &maximum_level, sizeof(maximum_level))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 11, SQLDT_STRING, (void*)memo.c_str(), strlen(memo.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		make_response(res, FAILURE_RET, "An error occurred while inserting data.");
		RETURN_STMT_FAILURE(stmt, maplock);
	}

	make_response(res, SUCCESS_RET);
	RETURN_STMT_SUCCESS(stmt, maplock);
}

HANDLER_FUNC(partybooking_delete) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("MasterAID");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto leader_account_id = GET_NUMBER_FIELD("MasterAID", 0);

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	SQLLock maplock(MAP_SQL_LOCK);
	maplock.lock();
	auto handle = maplock.getHandle();
	SqlStmt* stmt = SqlStmt_Malloc(handle);

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt,
		"DELETE FROM `%s` WHERE (`account_id` = ? AND `world_name` = ?)",
		partybookings_table)
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_INT, &account_id, sizeof(account_id))
		|| SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void*)world_name.c_str(), strlen(world_name.c_str()))
		|| SQL_SUCCESS != SqlStmt_Execute(stmt)
		) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		RETURN_STMT_FAILURE(stmt, maplock);
	}

	make_response(res, SUCCESS_RET);
	RETURN_STMT_SUCCESS(stmt, maplock);
}

HANDLER_FUNC(partybooking_get) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist.");
		return;
	}

	std::vector<s_party_booking_entry> bookings;

	if (!party_booking_read(world_name, bookings, "`account_id` = '" + std::to_string(account_id) + "'")) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		return;
	}
	
	json response;
	response["Type"] = SUCCESS_RET;

	if (!bookings.empty()) {
		response["data"] = bookings.at(0).to_json(world_name);
	}

	make_response(res, response);
}

HANDLER_FUNC(partybooking_info) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}
	
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("QueryAID");

	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto leader_account_id = GET_NUMBER_FIELD("QueryAID", 0);

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist.");
		return;
	}

	if (!isVaildAccount(leader_account_id)) {
		make_response(res, FAILURE_RET, "The account specified by the \"QueryAID\" does not exist.");
		return;
	}

	std::vector<s_party_booking_entry> bookings;

	if (!party_booking_read(world_name, bookings, "`account_id` = '" + std::to_string(leader_account_id) + "'")) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		return;
	}

	json response;
	response["Type"] = SUCCESS_RET;

	if (!bookings.empty()) {
		response["data"] = json::array();
		response["data"].push_back(bookings.at(0).to_json(world_name));
	}

	make_response(res, response);
}

HANDLER_FUNC(partybooking_list) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");
	REQUIRE_FIELD_EXISTS("page");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto page = GET_NUMBER_FIELD("page", 1);

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist.");
		return;
	}

	// 构建查询条件
	std::string condition = "`account_id` != '" + std::to_string(account_id) + "'";

	// 获取能查到的总记录数
	int record_count = party_booking_count(world_name, condition);

	// 计算最大页数
	int max_page = (int)ceil((double)record_count / SINGLE_PAGESIZE);

	// 构建 LIMIT 翻页限制
	std::string limit = " LIMIT " + std::to_string((page - 1) * SINGLE_PAGESIZE) + ", " + std::to_string(SINGLE_PAGESIZE);

	// 用于承接结果的 vector 容器
	std::vector<s_party_booking_entry> bookings;

	// 执行查询
	if (!party_booking_read(world_name, bookings, condition, " ORDER BY `created` DESC", limit)) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		return;
	}

	// 构建 HTTP 响应内容
	json response;
	response["Type"] = SUCCESS_RET;

	if (!bookings.empty()) {
		response["totalPage"] = max_page;
		response["data"] = json::array();

		for (size_t i = 0; i < bookings.size(); i++) {
			s_party_booking_entry& booking = bookings.at(i);
			response["data"].push_back(booking.to_json(world_name));
		}
	}

	make_response(res, response);
}

HANDLER_FUNC(partybooking_search) {
	if (!isAuthorized(req, false)) {
		make_response(res, FAILURE_RET, "Authorization verification failure.");
		return;
	}

	REQUIRE_FIELD_EXISTS("AID");
	REQUIRE_FIELD_EXISTS("GID");
	REQUIRE_FIELD_EXISTS("WorldName");
	//	REQUIRE_FIELD_EXISTS("Memo");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	REQUIRE_FIELD_EXISTS("MinLV");
	REQUIRE_FIELD_EXISTS("MaxLV");
	// 	REQUIRE_FIELD_EXISTS("Tanker");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	// 	REQUIRE_FIELD_EXISTS("Healer");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	// 	REQUIRE_FIELD_EXISTS("Dealer");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	// 	REQUIRE_FIELD_EXISTS("Assist");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	//	REQUIRE_FIELD_EXISTS("Type");	// 该字段若玩家不选, 那么客户端不会上传, 因此不做校验
	REQUIRE_FIELD_EXISTS("page");

	auto account_id = GET_NUMBER_FIELD("AID", 0);
	auto char_id = GET_NUMBER_FIELD("GID", 0);
	auto world_name = GET_STRING_FIELD("WorldName", "");
	auto keyword = GET_STRING_FIELD("Memo", "");
	auto minimum_level = GET_NUMBER_FIELD("MinLV", 0);
	auto maximum_level = GET_NUMBER_FIELD("MaxLV", 0);
	auto tanker = GET_NUMBER_FIELD("Tanker", 0);
	auto healer = GET_NUMBER_FIELD("Healer", 0);
	auto damagedealer = GET_NUMBER_FIELD("Dealer", 0);
	auto assist = GET_NUMBER_FIELD("Assist", 0);
	auto purpose = GET_NUMBER_FIELD("Type", 0);
	auto page = GET_NUMBER_FIELD("page", 1);

	if (world_name.length() > WORLD_NAME_LENGTH) {
		make_response(res, FAILURE_RET, "The world name length exceeds limit.");
		return;
	}

	if (!isVaildCharacter(account_id, char_id)) {
		make_response(res, FAILURE_RET, "The character specified by the \"GID\" does not exist.");
		return;
	}

	// 构建查询条件
	std::string condition = "`account_id` != '" + std::to_string(account_id) + "'";

	if (purpose) {
		char buf[128] = { 0 };
		sprintf(buf, " AND `purpose` = %d", purpose);
		condition += buf;
	}
	condition += party_booking_class_condition(tanker, healer, damagedealer, assist);
	if (keyword.length()) {
		condition += " AND (`char_name` LIKE '%" + keyword + "%' OR `comment` LIKE '%" + keyword + "%')";
	}

	condition += " AND `minimum_level` <= " + std::to_string(minimum_level);
	condition += " AND `maximum_level` >= " + std::to_string(maximum_level);

	// 获取能查到的总记录数
	int record_count = party_booking_count(world_name, condition);

	// 计算最大页数
	int max_page = (int)ceil((double)record_count / SINGLE_PAGESIZE);

	// 构建 LIMIT 翻页限制
	std::string limit = " LIMIT " + std::to_string((page - 1) * SINGLE_PAGESIZE) + ", " + std::to_string(SINGLE_PAGESIZE);

	// 用于承接结果的 vector 容器
	std::vector<s_party_booking_entry> bookings;

	// 执行查询
	if (!party_booking_read(world_name, bookings, condition, " ORDER BY `created` DESC", limit)) {
		make_response(res, FAILURE_RET, "An error occurred while executing query.");
		return;
	}

	// 构建 HTTP 响应内容
	json response;
	response["Type"] = SUCCESS_RET;

	if (!bookings.empty()) {
		response["totalPage"] = max_page;
		response["data"] = json::array();

		for (size_t i = 0; i < bookings.size(); i++) {
			s_party_booking_entry& booking = bookings.at(i);
			response["data"].push_back(booking.to_json(world_name));
		}
	}

	make_response(res, response);
}

#endif // Pandas_WebServer_Rewrite_Controller_HandlerFunc

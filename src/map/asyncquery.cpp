#include "asyncquery.hpp"

#include "map.hpp"
#include "log.hpp"

#include <common/showmsg.hpp>
#include <common/strlib.hpp>
#include <common/future.hpp>

#include <thread>

// [Threads]
// Main Thread
// DB Thread

using namespace std;
using namespace rathena::server_core;

extern int map_server_port;
extern std::string map_server_ip;
extern std::string map_server_id;
extern std::string map_server_pw;
extern std::string map_server_db;

extern int log_db_port;
extern std::string log_db_ip;
extern std::string log_db_id;
extern std::string log_db_pw;
extern std::string log_db_db;

size_t DBResultData::Index(int Row, int Column) {
	return Row * ColumnNum + Column;
}

const char* DBResultData::GetData(int Row, int Column) {
	return data.at(Index(Row, Column)).c_str();
}

void DBResultData::SetData(int Row, int Column, Sql* handle) {
	char* _data;
	Sql_GetData(handle, Column, &_data, NULL);
	data.at(Index(Row, Column)) = _data;
}

int8 DBResultData::GetInt8(int Row, int Column) {
	return atoi(GetData(Row, Column));
}

uint8 DBResultData::GetUInt8(int Row, int Column) {
	return atoi(GetData(Row, Column));
}

int16 DBResultData::GetInt16(int Row, int Column) {
	return atoi(GetData(Row, Column));
}

uint16 DBResultData::GetUInt16(int Row, int Column) {
	return atoi(GetData(Row, Column));
}

int32 DBResultData::GetInt32(int Row, int Column) {
	return atoi(GetData(Row, Column));
}

uint32 DBResultData::GetUInt32(int Row, int Column) {
	return strtoul(GetData(Row, Column), nullptr, 10);
}

int64 DBResultData::GetInt64(int Row, int Column) {
	return strtoll(GetData(Row, Column), NULL, 10);
}

uint64 DBResultData::GetUInt64(int Row, int Column) {
	return strtoull(GetData(Row, Column), NULL, 10);
}

thread* db_thread;
Sql* MainDBHandle = NULL, * LogDBHandle = NULL;

JobQueue<dbJob> dbJobs;

Sql* getHandle(dbType type) {
	switch (type) {
	default:
	case dbType::MAIN_DB: return MainDBHandle;
	case dbType::LOG_DB: return LogDBHandle;
	}
}

// Main Thread Function
void asyncquery_addDBJob(dbType dType, string query, futureJobFunc resultFunc) {
	// 将查询任务排入 dbJobs 队列
	dbJobs.push_back({ dType, query, resultFunc });
}

// Main Thread Function
void asyncquery_addDBJob(dbType dType, string query) {
	// 将查询任务排入 dbJobs 队列
	dbJobs.push_back({ dType, query, NULL });
}

// DB Thread Function
void doQuery(dbJob& job) {
	Sql* handle = getHandle(job.dType);
	int sql_result_value;

	// 执行查询, 失败则报错; 成功则看看是否有回调函数
	if (SQL_ERROR == (sql_result_value = Sql_QueryStr(handle, job.query.c_str())))
		Sql_ShowDebug(handle);
	else if (job.resultFunc) {
		// 有回调函数的话，把查询到的数据保存到一个 DBResultData 类中
		DBResultData* r = new DBResultData(
			(size_t)Sql_NumRows(handle),
			(size_t)Sql_NumColumns(handle),
			(size_t)Sql_NumRowsAffected(handle)
		);

		r->sql_result_value = sql_result_value;

		for (size_t Row = 0; Row < r->RowNum && SQL_SUCCESS == Sql_NextRow(handle); Row++)
			for (size_t ColumnNum = 0; ColumnNum < r->ColumnNum; ColumnNum++)
				r->SetData(Row, ColumnNum, handle);

		// 将回调函数排入 future 队列等待执行
		future_add(job.resultFunc, (FutureData)r);
	}

	Sql_FreeResult(handle);
}

// DB Thread Function
void db_runtime(void) {
	while (global_core->is_running()) {
		// 此线程函数每 50 毫秒执行一次 dbJobs.Run 方法
		this_thread::sleep_for(chrono::milliseconds(50));

		// 取出任务并挨个执行 doQuery 方法 (不是并行)
		dbJobs.Run([](dbJob& job) {
			doQuery(job);
		});
	}
}

// Main Thread Function
void asyncquery_init(void) {
	MainDBHandle = Sql_Malloc();

	ShowInfo("Connecting to the Map DB Server(async thread)....\n");
	if (SQL_ERROR == Sql_Connect(MainDBHandle, map_server_id.c_str(), map_server_pw.c_str(), map_server_ip.c_str(), map_server_port, map_server_db.c_str()))
	{
		ShowError("Couldn't connect with uname='%s',passwd='%s',host='%s',port='%d',database='%s'\n",
			map_server_id.c_str(), map_server_pw.c_str(), map_server_ip.c_str(), map_server_port, map_server_db.c_str());
		Sql_ShowDebug(MainDBHandle);
		Sql_Free(MainDBHandle);
		exit(EXIT_FAILURE);
	}
	ShowStatus("Connect success! (Map DB Server(async thread) Connection)\n");

	if (log_config.sql_logs) {
		LogDBHandle = Sql_Malloc();

		ShowInfo("Connecting to the Log DB Server(async thread)....\n");
		if (SQL_ERROR == Sql_Connect(LogDBHandle, log_db_id.c_str(), log_db_pw.c_str(), log_db_ip.c_str(), log_db_port, log_db_db.c_str()))
		{
			ShowError("Couldn't connect with uname='%s',passwd='%s',host='%s',port='%d',database='%s'\n",
				log_db_id.c_str(), log_db_pw.c_str(), log_db_ip.c_str(), log_db_port, log_db_db.c_str());
			Sql_ShowDebug(LogDBHandle);
			Sql_Free(LogDBHandle);
			exit(EXIT_FAILURE);
		}

		ShowStatus("Connect success! (Log DB Server(async thread) Connection)\n");
	}

	// Creating DB Thread
	db_thread = new thread(db_runtime);
}

// Main Thread Function
void asyncquery_final(void) {
	db_thread->join();

	ShowStatus("Close Map DB Server(async thread) Connection....\n");
	Sql_Free(MainDBHandle);
	MainDBHandle = NULL;
	if (log_config.sql_logs) {
		ShowStatus("Close Log DB Server(async thread) Connection....\n");
		Sql_Free(LogDBHandle);
		LogDBHandle = NULL;
	}
}

#pragma once

#include <common/sql.hpp>
#include <common/future.hpp>
#include <vector>
#include <string>
#include <functional>

// DBResultData
// structure that contains data from Sql_Query()
struct DBResultData {
private:
	size_t Index(size_t Row, size_t Column);

public:
	std::vector<std::string> data;
	size_t ColumnNum = 0;
	size_t RowNum = 0;
	size_t NumRowsAffected = 0;
	int sql_result_value = 0;

	DBResultData(size_t rowNum, size_t columnNum, size_t numRowsAffected) : data(rowNum* columnNum) {
		ColumnNum = columnNum;
		RowNum = rowNum;
		NumRowsAffected = numRowsAffected;
	};

	DBResultData(const DBResultData* src) : DBResultData(src->RowNum, src->ColumnNum, src->NumRowsAffected) {
		sql_result_value = src->sql_result_value;
		data.assign(src->data.begin(), src->data.end());
	};

	const char* GetData(size_t Row, size_t Column);
	void SetData(size_t Row, size_t Column, Sql* handle);
	int8 GetInt8(size_t Row, size_t Column);
	uint8 GetUInt8(size_t Row, size_t Column);
	int16 GetInt16(size_t Row, size_t Column);
	uint16 GetUInt16(size_t Row, size_t Column);
	int32 GetInt32(size_t Row, size_t Column);
	uint32 GetUInt32(size_t Row, size_t Column);
	int64 GetInt64(size_t Row, size_t Column);
	uint64 GetUInt64(size_t Row, size_t Column);
};

enum class dbType {
	MAIN_DB,
	LOG_DB,
};

typedef std::function<void(DBResultData& result)> dbJobFunc;

// Job for doing Sql_Query
struct dbJob {
	dbType dType;
	std::string query;
	futureJobFunc resultFunc; // Callback function after finishing Sql_Query()
};

void asyncquery_addDBJob(dbType dType, std::string query, futureJobFunc resultFunc);
void asyncquery_addDBJob(dbType dType, std::string query);
void asyncquery_init(void);
void asyncquery_final(void);

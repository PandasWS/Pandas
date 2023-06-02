// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "performance.hpp"

#include <chrono>
#include <map>

#include <common/showmsg.hpp>
#include <common/cbasetypes.hpp>

using std::chrono::nanoseconds;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

struct s_performance_item {
	nanoseconds duration_ns;
	high_resolution_clock::time_point start_time;
	uint64 total_cnt;

	s_performance_item() {
		duration_ns = nanoseconds::zero();
		total_cnt = 0;
	}
};

std::map<std::string, struct s_performance_item> __performance;

//************************************
// Method:      performance_create
// Description: 创建并初始化一个性能计数器, 若计数器已存在则重置全部数据
// Parameter:   std::string name
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/10 22:32
//************************************
void performance_create(std::string name) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		it->second.duration_ns = nanoseconds::zero();
		it->second.start_time = high_resolution_clock::time_point();
		it->second.total_cnt = 0;
	}
	else {
		s_performance_item sitem;
		sitem.duration_ns = nanoseconds::zero();
		sitem.start_time = high_resolution_clock::time_point();
		sitem.total_cnt = 0;
		__performance[name] = sitem;
	}
}

//************************************
// Method:      performance_start
// Description: 使指定的性能计数器开始计时 (可反复 start 和 stop, 耗时结果会累积)
// Parameter:   std::string name
// Parameter:   const char * debug_file
// Parameter:   const unsigned long debug_line
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/10 22:32
//************************************
void performance_start(std::string name, const char* debug_file, const unsigned long debug_line) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		it->second.start_time = high_resolution_clock::now();
		return;
	}
	ShowDebug("%s: The counter name '%s' is not exists, please initialize before using.\n", __func__, name.c_str());
	ShowDebug("%s: Caller: %s Line: %" PRIuPTR "\n", __func__, debug_file, debug_line);
}


//************************************
// Method:      performance_stop
// Description: 停止指定的性能计数器, 并将时间记录起来 (可反复 start 和 stop, 耗时结果会累积)
// Parameter:   std::string name
// Parameter:   const char * debug_file
// Parameter:   const unsigned long debug_line
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/10 22:32
//************************************
void performance_stop(std::string name, const char* debug_file, const unsigned long debug_line) {
	high_resolution_clock::time_point stop_time = high_resolution_clock::now();
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		nanoseconds duration_curr = duration_cast<nanoseconds>(stop_time - it->second.start_time);
		it->second.duration_ns += duration_curr;
		it->second.total_cnt++;
		return;
	}
	ShowDebug("%s: The counter name '%s' is not exists, please initialize before using.\n", __func__, name.c_str());
	ShowDebug("%s: Caller: %s Line: %" PRIuPTR "\n", __func__, debug_file, debug_line);
}

//************************************
// Method:      performance_get_milliseconds
// Description: 获取指定性能计数器距离上次调用 performance_create 至今的累积耗时
// Access:      public 
// Parameter:   std::string name
// Returns:     int64
// Author:      Sola丶小克(CairoLee)  2020/11/08 16:15
//************************************
int64 performance_get_milliseconds(std::string name) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		milliseconds ms = duration_cast<milliseconds>(it->second.duration_ns);
		return ms.count();
	}
	return 0;
}

//************************************
// Method:      performance_get_totalcount
// Description: 获取指定性能计数器距离上次调用 performance_create 至今的累积调用次数
// Access:      public 
// Parameter:   std::string name
// Returns:     uint64
// Author:      Sola丶小克(CairoLee)  2020/11/08 16:16
//************************************
uint64 performance_get_totalcount(std::string name) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		return it->second.total_cnt;
	}
	return 0;
}

//************************************
// Method:      performance_report
// Description: 输出距离上次调用 performance_create 至今的计数器信息
// Parameter:   std::string name
// Parameter:   const char * debug_file
// Parameter:   const unsigned long debug_line
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2020/02/10 21:58
//************************************
void performance_report(std::string name, const char* debug_file, const unsigned long debug_line) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		milliseconds ms = duration_cast<milliseconds>(it->second.duration_ns);
#ifdef _DEBUG
		ShowDebug("Performance counter " CL_WHITE "'%s'" CL_RESET " fired %" PRIu64 " times, took " CL_WHITE "%" PRIu64 CL_RESET " ms\n", name.c_str(), it->second.total_cnt, ms.count());
#else
		ShowStatus("Performance counter " CL_WHITE "'%s'" CL_RESET " fired %" PRIu64 " times, took " CL_WHITE "%" PRIu64 CL_RESET " ms\n", name.c_str(), it->second.total_cnt, ms.count());
#endif // _DEBUG
		return;
	}
	ShowDebug("%s: The counter name '%s' is not exists, please initialize before using.\n", __func__, name.c_str());
	ShowDebug("%s: Caller: %s Line: %" PRIuPTR "\n", __func__, debug_file, debug_line);
}

//************************************
// Method:      performance_destory
// Description: 销毁性能计数器, 下次再使用需要重新调用 performance_create
// Access:      public 
// Parameter:   std::string name
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/11/08 15:46
//************************************
void performance_destory(std::string name) {
	auto it = __performance.find(name);
	if (it != __performance.end()) {
		__performance.erase(it);
	}
}

//************************************
// Method:      performance_create_and_start
// Description: 创建一个性能计数器并立刻开始计时 (init + start)
// Parameter:   std::string name
// Parameter:   const char * debug_file
// Parameter:   const unsigned long debug_line
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/11 13:52
//************************************
void performance_create_and_start(std::string name, const char* debug_file, const unsigned long debug_line) {
	performance_create(name);
	performance_start(name, debug_file, debug_line);
}

//************************************
// Method:      performance_stop_and_report
// Description: 停止一个性能计数器并输出结果 (stop + report)
// Parameter:   std::string name
// Parameter:   const char * debug_file
// Parameter:   const unsigned long debug_line
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/02/11 13:52
//************************************
void performance_stop_and_report(std::string name, const char* debug_file, const unsigned long debug_line) {
	performance_stop(name, debug_file, debug_line);
	performance_report(name, debug_file, debug_line);
}

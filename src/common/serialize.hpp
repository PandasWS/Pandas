// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <fstream>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/bitset.hpp>

#define ARCHIVE_REGISTER_TYPE(x,type) x.template register_type<type>()
#define ARCHIVEPTR_REGISTER_TYPE(x,type) x->template register_type<type>()
#define SERIALIZE_SET_MEMORY_ZERO(x) memset(&x, 0, sizeof(x))

//************************************
// 此处的疾风缓存版本会被纳入缓存散列的计算过程中
// 注意: 变缓存版本会导致所有缓存过期
//************************************
#define BLASTCACHE_VERSION 20220312

//************************************
// 若想启用二进制格式的缓存, 则启用此段代码(并禁用其他)
// 注意: 变更缓存格式会导致所有缓存过期
//************************************
#define SERIALIZE_SAVE_ARCHIVE boost::archive::binary_oarchive
#define SERIALIZE_SAVE_STREAM_FLAG std::ofstream::binary | std::ofstream::out
#define SERIALIZE_LOAD_ARCHIVE boost::archive::binary_iarchive
#define SERIALIZE_LOAD_STREAM_FLAG std::ifstream::binary | std::ifstream::in

//************************************
// 若想启用文本格式的缓存, 则启用此段代码(并禁用其他)
// 注意: 变更缓存格式会导致所有缓存过期
//************************************
// #define SERIALIZE_SAVE_ARCHIVE boost::archive::text_oarchive
// #define SERIALIZE_SAVE_STREAM_FLAG std::ofstream::out
// #define SERIALIZE_LOAD_ARCHIVE boost::archive::text_iarchive
// #define SERIALIZE_LOAD_STREAM_FLAG std::ifstream::in

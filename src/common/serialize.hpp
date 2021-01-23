#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/bitset.hpp>

#define ARCHIVE_REGISTER_TYPE(x,type) x.template register_type<type>()
#define ARCHIVEPTR_REGISTER_TYPE(x,type) x->template register_type<type>()
#define SERIALIZE_SET_MEMORY_ZERO(x) memset(&x, 0, sizeof(x))

#define SERIALIZE_SAVE_ARCHIVE boost::archive::binary_oarchive
#define SERIALIZE_LOAD_ARCHIVE boost::archive::binary_iarchive

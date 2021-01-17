#pragma once

#include <boost/archive/text_oarchive.hpp> 
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/bitset.hpp>

#include "../config/pandas.hpp"

#define ARCHIVE_REGISTER_TYPE(x,type) x##.template register_type<type>()
#define ARCHIVEPTR_REGISTER_TYPE(x,type) (*x##).template register_type<type>()

class YamlDatabase;
template <typename keytype, typename datatype> class TypesafeYamlDatabase;
template <typename keytype, typename datatype> class TypesafeCachedYamlDatabase;

namespace boost {
	namespace serialization {

		// ======================================================================
		// class YamlDatabase
		// ======================================================================

		template <typename Archive>
		void serialize(
			Archive& ar, YamlDatabase& t, const unsigned int version
		) {
			ar& t.quietLevel;
			ar& t.includeFiles;
		}

		template<class Archive>
		inline void save_construct_data(
			Archive& ar, const YamlDatabase* t, const unsigned int version
		) {
			// save data required to construct instance
			ar << t->type;
			ar << t->version;
			ar << t->minimumVersion;
		}

		template<class Archive>
		inline void load_construct_data(
			Archive& ar, YamlDatabase* t, const unsigned int version
		) {
			// retrieve data from archive required to construct new instance
			std::string type;
			uint16 version_ = 0, minimumVersion = 0;

			ar >> type;
			ar >> version_;
			ar >> minimumVersion;

			// invoke inplace constructor to initialize instance of my_class
			::new(t)YamlDatabase(type, version_, minimumVersion);
		}

		// ======================================================================
		// class TypesafeYamlDatabase<keytype, datatype>
		// ======================================================================

		template <typename Archive, typename keytype, typename datatype>
		void serialize(
			Archive& ar, TypesafeYamlDatabase<keytype, datatype>& t,
			const unsigned int version
		) {
			ar& boost::serialization::base_object<YamlDatabase>(t);
			ar& t.data;
		}

		// ======================================================================
		// class TypesafeCachedYamlDatabase<keytype, datatype>
		// ======================================================================

		template <typename Archive, typename keytype, typename datatype>
		void serialize(
			Archive& ar, TypesafeCachedYamlDatabase<keytype, datatype>& t,
			const unsigned int version
		) {
			ar& boost::serialization::base_object<TypesafeYamlDatabase<keytype, datatype>>(t);
			ar& t.cache;
		}

	} // namespace serialization
} // namespace boost

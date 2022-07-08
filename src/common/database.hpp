// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <unordered_map>
#include <vector>

#include <ryml_std.hpp>
#include <ryml.hpp>

#include "../config/core.hpp"

#include "cbasetypes.hpp"
#include "core.hpp"
#include "utilities.hpp"

#ifdef Pandas_YamlBlastCache_Serialize
#include <set>
#include <string>

#include "cbasetypes.hpp"
#include "serialize.hpp"

class BlastCache {
private:
	void* p = nullptr;
protected:
	friend class BlastCacheEnabled;
	friend class boost::serialization::access;

	std::string type;
	uint16 version;
	bool bEnabledCache = false;
	std::set<std::string> includeFiles;
	size_t datatypeSize = 0;

	template <typename Archive>
	inline void serialize(Archive& ar, const unsigned int version) {
		ar& includeFiles;
	}

	template <typename Archive>
	inline bool fireSerialize(Archive& ar) {
		return this->doSerialize(typeid(ar).name(), static_cast<void*>(&ar));
	}

	bool saveToCache();
	bool loadFromCache();
	bool isCacheEffective();
	bool removeCache();

	const std::string getCacheHash(const std::string& path);
	const std::string getCachePath();
public:
	BlastCache(void* p_, const std::string& type_, uint16 version_) {
		this->p = p_;
		this->type = type_;
		this->version = version_;
	}

	const std::string getCacheHashByName(const std::string& db_name);

	virtual void afterCacheRestore() {

	};

	virtual const std::string getDependsHash() {
		return "";
	};

	virtual bool doSerialize(const std::string& type, void* archive) {
		return false;
	};
};

class BlastCacheEnabled {
private:
	void* p = nullptr;
public:
	BlastCacheEnabled(void* p_) {
		this->p = p_;
		((BlastCache*)p)->bEnabledCache = true;
	}

	virtual bool doSerialize(const std::string& type, void* archive) = 0;
};
#else
class BlastCache {
public:
	BlastCache(void* p_, const std::string& type_, uint16 version_) {
	}
};

class BlastCacheEnabled {
public:
	BlastCacheEnabled(void* p_) {
	}
};
#endif // Pandas_YamlBlastCache_Serialize

class YamlDatabase : public BlastCache {
// Internal stuff
private:
#ifdef Pandas_YamlBlastCache_Serialize
	friend class BlastCache;
#endif // Pandas_YamlBlastCache_Serialize
	std::string type;
	uint16 version;
	uint16 minimumVersion;
	std::string currentFile;

	bool verifyCompatibility( const ryml::Tree& rootNode );
	bool load( const std::string& path );
	void parse( const ryml::Tree& rootNode );
#ifndef Pandas_YamlBlastCache_Serialize
	void parseImports( const ryml::Tree& rootNode );
#else
	bool parseImports( const ryml::Tree& rootNode );
#endif // Pandas_YamlBlastCache_Serialize
	template <typename R> bool asType( const ryml::NodeRef& node, const std::string& name, R& out );

// These should be visible/usable by the implementation provider
protected:
#ifdef Pandas_Database_Yaml_BeQuiet
	// 0 - 正常; &1 = 状态; &2 = 警告; &4 = 错误
	uint16 quietLevel = 0;
	void* p = nullptr;
#endif // Pandas_Database_Yaml_BeQuiet
	ryml::Parser parser;

	// Helper functions
	bool nodeExists( const ryml::NodeRef& node, const std::string& name );
	bool nodesExist( const ryml::NodeRef& node, std::initializer_list<const std::string> names );
	int32 getLineNumber(const ryml::NodeRef& node);
	int32 getColumnNumber(const ryml::NodeRef& node);
	void invalidWarning( const ryml::NodeRef& node, const char* fmt, ... );
	std::string getCurrentFile();

	// Conversion functions
	bool asBool(const ryml::NodeRef& node, const std::string &name, bool &out);
	bool asInt16(const ryml::NodeRef& node, const std::string& name, int16& out );
	bool asUInt16(const ryml::NodeRef& node, const std::string& name, uint16& out);
	bool asInt32(const ryml::NodeRef& node, const std::string &name, int32 &out);
	bool asUInt32(const ryml::NodeRef& node, const std::string &name, uint32 &out);
	bool asInt64(const ryml::NodeRef& node, const std::string &name, int64 &out);
	bool asUInt64(const ryml::NodeRef& node, const std::string &name, uint64 &out);
	bool asFloat(const ryml::NodeRef& node, const std::string &name, float &out);
	bool asDouble(const ryml::NodeRef& node, const std::string &name, double &out);
	bool asString(const ryml::NodeRef& node, const std::string &name, std::string &out);
	bool asUInt16Rate(const ryml::NodeRef& node, const std::string& name, uint16& out, uint16 maximum=10000);
	bool asUInt32Rate(const ryml::NodeRef& node, const std::string& name, uint32& out, uint32 maximum=10000);

	virtual void loadingFinished();
public:
	YamlDatabase( const std::string& type_, uint16 version_, uint16 minimumVersion_ ) : BlastCache(this, type_, version_) {
		this->type = type_;
		this->version = version_;
		this->minimumVersion = minimumVersion_;
#ifdef Pandas_Database_Yaml_BeQuiet
		this->quietLevel = 0;
		this->p = this;
#endif // Pandas_Database_Yaml_BeQuiet
	}

	YamlDatabase( const std::string& type_, uint16 version_ ) : YamlDatabase( type_, version_, version_ ){
		// Empty since everything is handled by the real constructor
	}

	bool load();
	bool reload();

	// Functions that need to be implemented for each type
	virtual void clear() = 0;
	virtual const std::string getDefaultLocation() = 0;
	virtual uint64 parseBodyNode( const ryml::NodeRef& node ) = 0;
};

template <typename keytype, typename datatype> class TypesafeYamlDatabase : public YamlDatabase{
#ifdef Pandas_YamlBlastCache_Serialize
private:
	friend class boost::serialization::access;

	template <typename Archive>
	inline void serialize(Archive& ar, const unsigned int version) {
		ar& boost::serialization::base_object<YamlDatabase>(*this);
		ar& data;
	}
#endif // Pandas_YamlBlastCache_Serialize

protected:
	std::unordered_map<keytype, std::shared_ptr<datatype>> data;

public:
	TypesafeYamlDatabase( const std::string& type_, uint16 version_, uint16 minimumVersion_ ) : YamlDatabase( type_, version_, minimumVersion_ ){
#ifdef Pandas_YamlBlastCache_Serialize
		this->datatypeSize = sizeof(datatype);
#endif // Pandas_YamlBlastCache_Serialize
	}

	TypesafeYamlDatabase( const std::string& type_, uint16 version_ ) : YamlDatabase( type_, version_, version_ ){
#ifdef Pandas_YamlBlastCache_Serialize
		this->datatypeSize = sizeof(datatype);
#endif // Pandas_YamlBlastCache_Serialize
	}

	void clear() override{
		this->data.clear();
	}

	bool empty(){
		return this->data.empty();
	}

	bool exists( keytype key ){
		return this->find( key ) != nullptr;
	}

	virtual std::shared_ptr<datatype> find( keytype key ){
		auto it = this->data.find( key );

		if( it != this->data.end() ){
			return it->second;
		}else{
			return nullptr;
		}
	}

	void put( keytype key, std::shared_ptr<datatype> ptr ){
		this->data[key] = ptr;
	}

	typename std::unordered_map<keytype, std::shared_ptr<datatype>>::iterator begin(){
		return this->data.begin();
	}

	typename std::unordered_map<keytype, std::shared_ptr<datatype>>::iterator end(){
		return this->data.end();
	}

	size_t size(){
		return this->data.size();
	}

	std::shared_ptr<datatype> random(){
		if( this->empty() ){
			return nullptr;
		}

		return rathena::util::umap_random( this->data );
	}

	void erase(keytype key) {
		this->data.erase(key);
	}
};

template <typename keytype, typename datatype> class TypesafeCachedYamlDatabase : public TypesafeYamlDatabase<keytype, datatype>{
private:
#ifdef Pandas_YamlBlastCache_Serialize
	friend class boost::serialization::access;

	template <typename Archive>
	inline void serialize(Archive& ar, const unsigned int version) {
		ar& boost::serialization::base_object<TypesafeYamlDatabase<keytype, datatype>>(*this);
		ar& cache;
	}
#endif // Pandas_YamlBlastCache_Serialize

	std::vector<std::shared_ptr<datatype>> cache;

public:
	TypesafeCachedYamlDatabase( const std::string& type_, uint16 version_, uint16 minimumVersion_ ) : TypesafeYamlDatabase<keytype, datatype>( type_, version_, minimumVersion_ ){
#ifdef Pandas_YamlBlastCache_Serialize
		this->datatypeSize = sizeof(datatype);
#endif // Pandas_YamlBlastCache_Serialize
	}

	TypesafeCachedYamlDatabase( const std::string& type_, uint16 version_ ) : TypesafeYamlDatabase<keytype, datatype>( type_, version_, version_ ){
#ifdef Pandas_YamlBlastCache_Serialize
		this->datatypeSize = sizeof(datatype);
#endif // Pandas_YamlBlastCache_Serialize
	}

	void clear() override{
		TypesafeYamlDatabase<keytype, datatype>::clear();
		cache.clear();
		cache.shrink_to_fit();
	}

	std::shared_ptr<datatype> find( keytype key ) override{
		if( this->cache.empty() || key >= this->cache.size() ){
			return TypesafeYamlDatabase<keytype, datatype>::find( key );
		}else{
			return cache[this->calculateCacheKey( key )];
		}
	}

	std::vector<std::shared_ptr<datatype>> getCache() {
		return this->cache;
	}

	virtual size_t calculateCacheKey( keytype key ){
		return key;
	}

	void loadingFinished() override{
		// Cache all known values
		for (auto &pair : *this) {
			// Calculate the key that should be used
			size_t key = this->calculateCacheKey(pair.first);

			// Check if the key fits into the current cache size
			if (this->cache.capacity() <= key) {
				// Some keys compute to 0, so we allocate a minimum of 500 (250*2) entries
				const static size_t minimum = 250;
				// Double the current size, so we do not have to resize that often
				size_t new_size = std::max( key, minimum ) * 2;

				// Very important => initialize everything to nullptr
				this->cache.resize(new_size, nullptr);
			}

			// Insert the value into the cache
			this->cache[key] = pair.second;
		}

		// Free the memory that was allocated too much
		this->cache.shrink_to_fit();
	}
};

void do_init_database();

#ifdef Pandas_YamlBlastCache_Serialize
namespace boost {
	namespace serialization {
		// ======================================================================
		// class TypesafeYamlDatabase<keytype, datatype>
		// ======================================================================

		template<class Archive, typename keytype, typename datatype>
		inline void save_construct_data(
			Archive& ar, const TypesafeYamlDatabase<keytype, datatype>* t, const unsigned int version
		) {
			// save data required to construct instance
			ar << t->type;
			ar << t->version;
			ar << t->minimumVersion;
		}

		template<class Archive, typename keytype, typename datatype>
		inline void load_construct_data(
			Archive& ar, TypesafeYamlDatabase<keytype, datatype>* t, const unsigned int version
		) {
			// retrieve data from archive required to construct new instance
			std::string type_;
			uint16 version_ = 0, minimumVersion_ = 0;

			ar >> type_;
			ar >> version_;
			ar >> minimumVersion_;

			// invoke inplace constructor to initialize instance of my_class
			::new(t)TypesafeYamlDatabase<keytype, datatype>(type_, version_, minimumVersion_);
		}

		// ======================================================================
		// class TypesafeCachedYamlDatabase<keytype, datatype>
		// ======================================================================

		template<class Archive, typename keytype, typename datatype>
		inline void save_construct_data(
			Archive& ar, const TypesafeCachedYamlDatabase<keytype, datatype>* t, const unsigned int version
		) {
			// save data required to construct instance
			ar << t->type;
			ar << t->version;
			ar << t->minimumVersion;
		}

		template<class Archive, typename keytype, typename datatype>
		inline void load_construct_data(
			Archive& ar, TypesafeCachedYamlDatabase<keytype, datatype>* t, const unsigned int version
		) {
			// retrieve data from archive required to construct new instance
			std::string type_;
			uint16 version_ = 0, minimumVersion_ = 0;

			ar >> type_;
			ar >> version_;
			ar >> minimumVersion_;

			// invoke inplace constructor to initialize instance of my_class
			::new(t)TypesafeCachedYamlDatabase<keytype, datatype>(type_, version_, minimumVersion_);
		}
	} // namespace serialization
} // namespace boost
#endif // Pandas_YamlBlastCache_Serialize

#endif /* DATABASE_HPP */

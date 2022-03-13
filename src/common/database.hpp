// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "../config/core.hpp"

#include "cbasetypes.hpp"
#include "core.hpp"
#include "utilities.hpp"

#ifdef Pandas_YamlBlastCache_Serialize
#include <set>
#include "serialize.hpp"
#endif // Pandas_YamlBlastCache_Serialize

class YamlDatabase{
// Internal stuff
private:
#ifdef Pandas_YamlBlastCache_Serialize
	friend class boost::serialization::access;

	template <typename Archive>
	inline void serialize(Archive& ar, const unsigned int version) {
		ar& quietLevel;
		ar& includeFiles;
	}

	template <typename Archive>
	inline bool fireSerialize(Archive& ar) {
		return this->doSerialize(typeid(ar).name(), static_cast<void*>(&ar));
	}

	bool isEnableSerialize(bool bNoWarning = false);
	bool saveToSerialize();
	bool loadFromSerialize();
	bool isCacheEffective();
	bool removeSerialize();

	std::string getBlashCacheHash(const std::string& path);
	std::string getBlastCachePath();

	std::set<std::string> includeFiles;
#endif // Pandas_YamlBlastCache_Serialize

	std::string type;
	uint16 version;
	uint16 minimumVersion;
	std::string currentFile;
#ifdef Pandas_Database_Yaml_BeQuiet
	uint16 quietLevel;	// 0 - 正常; &1 = 状态; &2 = 警告; &4 = 错误
#endif // Pandas_Database_Yaml_BeQuiet

	bool verifyCompatibility( const YAML::Node& rootNode );
	bool load( const std::string& path );
	void parse( const YAML::Node& rootNode );
#ifndef Pandas_YamlBlastCache_Serialize
	void parseImports( const YAML::Node& rootNode );
#else
	bool parseImports( const YAML::Node& rootNode );
#endif // Pandas_YamlBlastCache_Serialize
	template <typename R> bool asType( const YAML::Node& node, const std::string& name, R& out );

#ifdef Pandas_Database_Yaml_Support_UTF8BOM
	YAML::Node LoadFile(const std::string& filename);
#endif // Pandas_Database_Yaml_Support_UTF8BOM

// These should be visible/usable by the implementation provider
protected:
	// Helper functions
	bool nodeExists( const YAML::Node& node, const std::string& name );
	bool nodesExist( const YAML::Node& node, std::initializer_list<const std::string> names );
	void invalidWarning( const YAML::Node &node, const char* fmt, ... );
	std::string getCurrentFile();

	// Conversion functions
	bool asBool(const YAML::Node &node, const std::string &name, bool &out);
	bool asInt16(const YAML::Node& node, const std::string& name, int16& out );
	bool asUInt16(const YAML::Node& node, const std::string& name, uint16& out);
	bool asInt32(const YAML::Node &node, const std::string &name, int32 &out);
	bool asUInt32(const YAML::Node &node, const std::string &name, uint32 &out);
	bool asInt64(const YAML::Node &node, const std::string &name, int64 &out);
	bool asUInt64(const YAML::Node &node, const std::string &name, uint64 &out);
	bool asFloat(const YAML::Node &node, const std::string &name, float &out);
	bool asDouble(const YAML::Node &node, const std::string &name, double &out);
	bool asString(const YAML::Node &node, const std::string &name, std::string &out);
	bool asUInt16Rate(const YAML::Node& node, const std::string& name, uint16& out, uint16 maximum=10000);
	bool asUInt32Rate(const YAML::Node& node, const std::string& name, uint32& out, uint32 maximum=10000);

	virtual void loadingFinished();

#ifdef Pandas_YamlBlastCache_Serialize
	// 用于设置此类的实例是否支持疾风缓存
	// 如若设为 true 则将尝试对数据进行缓存读写操作,
	// 想要为 YamlDatabase 的子类开启缓存功能之前, 必须先定义需要对哪些结构体进行序列化
	bool supportSerialize = false;

	// 用于记录被缓存的数据类型的大小
	// 以便在通过宏定义修改数据体积后能够在 struct 长度变更时让缓存过期
	uint32 datatypeSize = 0;

	// 用来记录有效的 datatype 大小
	// 启用疾风缓存的情况下若 datatypeSize 不在此列表范围中, 则自动关闭疾风缓存
	std::vector <uint32> validDatatypeSize;
#endif // Pandas_YamlBlastCache_Serialize

public:
	YamlDatabase( const std::string type_, uint16 version_, uint16 minimumVersion_ ){
		this->type = type_;
		this->version = version_;
		this->minimumVersion = minimumVersion_;
#ifdef Pandas_Database_Yaml_BeQuiet
		this->quietLevel = 0;
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
	virtual uint64 parseBodyNode( const YAML::Node& node ) = 0;

#ifdef Pandas_YamlBlastCache_Serialize
	std::string getSpecifyDatabaseBlashCacheHash(const std::string& db_name);

	virtual bool doSerialize(const std::string& type, void* archive) {
		return false;
	}

	virtual void afterSerialize() {

	}

	virtual std::string getAdditionalCacheHash() {
		return "";
	}
#endif // Pandas_YamlBlastCache_Serialize

#ifdef Pandas_Database_Yaml_BeQuiet
	//************************************
	// Method:      setQuietLevel
	// Description: 设置提示信息的静默等级
	// Parameter:   uint16 quietLevel_ ( 0 - 正常; &1 = 状态; &2 = 警告; &4 = 错误 )
	// Returns:     void
	// Author:      Sola丶小克(CairoLee)  2020/01/24 11:43
	//************************************
	void setQuietLevel(uint16 quietLevel_) {
		this->quietLevel = quietLevel_;
	}
#endif // Pandas_Database_Yaml_BeQuiet
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

	template<class Archive>
	friend void save_construct_data(
		Archive& ar, TypesafeYamlDatabase<keytype, datatype>* t, const unsigned int version
	);

	template<class Archive>
	friend void load_construct_data(
		Archive& ar, TypesafeYamlDatabase<keytype, datatype>* t, const unsigned int version
	);
#endif // Pandas_YamlBlastCache_Serialize

protected:
	std::unordered_map<keytype, std::shared_ptr<datatype>> data;

public:
	TypesafeYamlDatabase( const std::string type_, uint16 version_, uint16 minimumVersion_ ) : YamlDatabase( type_, version_, minimumVersion_ ){
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

	template<class Archive>
	friend void save_construct_data(
		Archive& ar, TypesafeCachedYamlDatabase<keytype, datatype>* t, const unsigned int version
	);

	template<class Archive>
	friend void load_construct_data(
		Archive& ar, TypesafeCachedYamlDatabase<keytype, datatype>* t, const unsigned int version
	);
#endif // Pandas_YamlBlastCache_Serialize

	std::vector<std::shared_ptr<datatype>> cache;

public:
	TypesafeCachedYamlDatabase( const std::string type_, uint16 version_, uint16 minimumVersion_ ) : TypesafeYamlDatabase<keytype, datatype>( type_, version_, minimumVersion_ ){
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

		// Restore size after clearing
		size_t cap = cache.capacity();

		cache.clear();
		cache.resize(cap, nullptr);
	}

	std::shared_ptr<datatype> find( keytype key ) override{
		if( this->cache.empty() || key >= this->cache.capacity() ){
			return TypesafeYamlDatabase<keytype, datatype>::find( key );
		}else{
			return cache[this->calculateCacheKey( key )];
		}
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
			if (this->cache.capacity() < key) {
				// Double the current size, so we do not have to resize that often
				size_t new_size = key * 2;

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

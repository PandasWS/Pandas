// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "database.hpp"

#include <iostream>
#include <sstream>

#include "malloc.hpp"
#include "showmsg.hpp"
#include "utilities.hpp"

using namespace rathena;

#ifdef Pandas_YamlBlastCache_Serialize
#include "configparser.hpp"
#include "cryptopp.hpp"
#endif // Pandas_YamlBlastCache_Serialize

#ifdef Pandas_Database_Yaml_BeQuiet
	#define ShowError if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 4) != 4) ::ShowError
	#define ShowWarning if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 2) != 2) ::ShowWarning
	#define ShowStatus if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 1) != 1) ::ShowStatus
#endif // Pandas_Database_Yaml_BeQuiet

bool YamlDatabase::nodeExists( const ryml::NodeRef& node, const std::string& name ){
	return (node.num_children() > 0 && node.has_child(c4::to_csubstr(name)));
}

bool YamlDatabase::nodesExist( const ryml::NodeRef& node, std::initializer_list<const std::string> names ){
	bool missing = false;

	for( const std::string& name : names ){
		if( !this->nodeExists( node, name ) ){
			ShowError( "Missing mandatory node \"%s\".\n", name.c_str() );
			missing = true;
		}
	}

	if( missing ){
		this->invalidWarning( node, "At least one mandatory node did not exist.\n" );
		return false;
	}

	return true;
}

bool YamlDatabase::verifyCompatibility( const ryml::Tree& tree ){
	if( !this->nodeExists( tree.rootref(), "Header" ) ){
		ShowError( "No database \"Header\" was found.\n" );
		return false;
	}

	const ryml::NodeRef& headerNode = tree["Header"];

	std::string tmpType;

	if( !this->asString( headerNode, "Type", tmpType ) ){
		return false;
	}

	if( this->type != tmpType ){
		ShowError( "Database type mismatch: %s != %s.\n", this->type.c_str(), tmpType.c_str() );
		return false;
	}

	uint16 tmpVersion;

	if( !this->asUInt16( headerNode, "Version", tmpVersion ) ){
		return false;
	}

	if( tmpVersion != this->version ){
		if( tmpVersion > this->version ){
			ShowError( "Database version %hu is not supported. Maximum version is: %hu\n", tmpVersion, this->version );
			return false;
		}else if( tmpVersion >= this->minimumVersion ){
			ShowWarning( "Database version %hu is outdated and should be updated. Current version is: %hu\n", tmpVersion, this->version );
			ShowWarning( "Reduced compatibility with %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), this->currentFile.c_str() );
		}else{
			ShowError( "Database version %hu is not supported anymore. Minimum version is: %hu\n", tmpVersion, this->minimumVersion );
			return false;
		}
	}

	return true;
}

bool YamlDatabase::load() {
#ifndef Pandas_YamlBlastCache_Serialize
	bool ret = this->load(this->getDefaultLocation());

	this->loadingFinished();

	return ret;
#else
	// 清空关联文件记录
	this->includeFiles.clear();

	// 如果已经从缓存加载, 那么执行 loadingFinished 后直接返回
	if (this->loadFromCache()) {
		this->loadingFinished();
		return true;
	}

	// 重制用来记录加载过程中是否存在错误和警告的相关变量
	yaml_load_completely_success = true;

	// 如果缓存失效, 那么下面执行全新加载
	bool ret = this->load(this->getDefaultLocation());

	// 失败的话直接把之前的缓存文件也干掉
	if (!yaml_load_completely_success) {
		this->removeCache();
	}

	// 如果加载成功且过程中没有出现任何错误和警告, 那么将加载成功后的数据进行缓存
	if (ret && yaml_load_completely_success) {
		this->saveToCache();
	}

	// 并执行 loadingFinished 进行一些数据初始化工作
	this->loadingFinished();

	return ret;
#endif // Pandas_YamlBlastCache_Serialize
}

bool YamlDatabase::reload(){
	this->clear();

	return this->load();
}

bool YamlDatabase::load(const std::string& path) {
#ifdef Pandas_Console_Translate
	// 若启用了控制台信息翻译机制
	// 那么允许指定一个空的 yaml 文件路径, 表示不加载任何数据
	if (path.empty()) {
		return true;
	}
#endif // Pandas_Console_Translate

#ifdef Pandas_YamlBlastCache_Serialize
	// 这个文件与当前数据库相关, 插入到关联文件记录中
	this->includeFiles.insert(path);
#endif // Pandas_YamlBlastCache_Serialize

#ifdef Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
		performance_create_and_start("yamldatabase_load");
#endif // Pandas_Speedup_Print_TimeConsuming_Of_KeySteps

	ShowStatus("Loading '" CL_WHITE "%s" CL_RESET "'..." CL_CLL "\r", path.c_str());
	FILE* f = fopen(path.c_str(), "r");
	if (f == nullptr) {
		ShowError("Failed to open %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), path.c_str());
		return false;
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	char* buf = (char *)aMalloc(size+1);
	rewind(f);
	size_t real_size = fread(buf, sizeof(char), size, f);
	// Zero terminate
	buf[real_size] = '\0';
	fclose(f);

	parser = {};
	ryml::Tree tree = parser.parse_in_arena(c4::to_csubstr(path), c4::to_csubstr(buf));

	// Required here already for header error reporting
	this->currentFile = path;

	if (!this->verifyCompatibility(tree)){
		ShowError("Failed to verify compatibility with %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), this->currentFile.c_str());
		aFree(buf);
		return false;
	}

	const ryml::NodeRef& header = tree["Header"];

	if( this->nodeExists( header, "Clear" ) ){
		bool clear;

		if( this->asBool( header, "Clear", clear ) && clear ){
			this->clear();
		}
	}

	this->parse( tree );

#ifndef Pandas_YamlBlastCache_Serialize
	this->parseImports( tree );

	aFree(buf);
	return true;
#else
	bool ret = this->parseImports( tree );
	
	aFree(buf);
	return ret;
#endif // Pandas_YamlBlastCache_Serialize
}

void YamlDatabase::loadingFinished(){
	// Does nothing by default, just for hooking
}

void YamlDatabase::parse( const ryml::Tree& tree ){
	uint64 count = 0;

	if( this->nodeExists( tree.rootref(), "Body" ) ){
		const ryml::NodeRef& bodyNode = tree["Body"];
		size_t childNodesCount = bodyNode.num_children();
		size_t childNodesProgressed = 0;
		const char* fileName = this->currentFile.c_str();

		for( const ryml::NodeRef &node : bodyNode ){
			count += this->parseBodyNode( node );

			ShowStatus( "Loading [%" PRIdPTR "/%" PRIdPTR "] entries from '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\r", ++childNodesProgressed, childNodesCount, fileName );
		}

#ifndef Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
		ShowStatus( "Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\n", count, fileName );
#else
		performance_stop("yamldatabase_load");
		ShowStatus( "Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "' (took %" PRIu64 " milliseconds)" CL_CLL "\n", count, fileName, performance_get_milliseconds("yamldatabase_load"));
#endif // Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
	}
}

#ifndef Pandas_YamlBlastCache_Serialize
void YamlDatabase::parseImports( const ryml::Tree& rootNode ){
#else
bool YamlDatabase::parseImports( const ryml::Tree& rootNode ){
	bool bSuccess = true;
#endif // Pandas_YamlBlastCache_Serialize
	if( this->nodeExists( rootNode.rootref(), "Footer" ) ){
		const ryml::NodeRef& footerNode = rootNode["Footer"];

		if( this->nodeExists( footerNode, "Imports") ){
			const ryml::NodeRef& importsNode = footerNode["Imports"];

			for( const ryml::NodeRef &node : importsNode ){
				std::string importFile;

				if( !this->asString( node, "Path", importFile ) ){
					continue;
				}

				if( this->nodeExists( node, "Mode" ) ){
					std::string mode;

					if( !this->asString( node, "Mode", mode ) ){
						continue;
					}

#ifdef RENEWAL
					std::string compiledMode = "Renewal";
#else
					std::string compiledMode = "Prerenewal";
#endif

					if( compiledMode != mode ){
						// Skip this import
						continue;
					}
				}				

#ifndef Pandas_YamlBlastCache_Serialize
				this->load( importFile );
#else
				bSuccess = bSuccess && this->load( importFile );
#endif // Pandas_YamlBlastCache_Serialize
			}
		}
	}
#ifdef Pandas_YamlBlastCache_Serialize
	return bSuccess;
#endif // Pandas_YamlBlastCache_Serialize
}

template <typename R> bool YamlDatabase::asType( const ryml::NodeRef& node, const std::string& name, R& out ){
	if( this->nodeExists( node, name ) ){
		const ryml::NodeRef& dataNode = node[c4::to_csubstr(name)];

		if (dataNode.val_is_null()) {
			this->invalidWarning(node, "Node \"%s\" is missing a value.\n", name.c_str());
			return false;
		}

		try{
			dataNode >> out;
		}catch( std::runtime_error const& ){
			this->invalidWarning( node, "Node \"%s\" cannot be parsed as %s.\n", name.c_str(), typeid( R ).name() );
			return false;
		}

		return true;
	}else{
		this->invalidWarning( node, "Missing node \"%s\".\n", name.c_str() );
		return false;
	}
}

bool YamlDatabase::asBool(const ryml::NodeRef& node, const std::string &name, bool &out) {
	const ryml::NodeRef& targetNode = node[c4::to_csubstr(name)];

	if (targetNode.val_is_null()) {
		this->invalidWarning(node, "Node \"%s\" is missing a value.\n", name.c_str());
		return false;
	}

	std::string str;

	targetNode >> str;

	util::tolower( str );

	if( str == "true" ){
		out = true;
		return true;
	}else if( str == "false" ){
		out = false;
		return true;
	}else{
		this->invalidWarning( targetNode, "Unknown boolean value: \"%s\".\n", str.c_str() );
		return false;
	}
}

bool YamlDatabase::asInt16( const ryml::NodeRef& node, const std::string& name, int16& out ){
	return asType<int16>( node, name, out);
}

bool YamlDatabase::asUInt16(const ryml::NodeRef& node, const std::string& name, uint16& out) {
	return asType<uint16>(node, name, out);
}

bool YamlDatabase::asInt32(const ryml::NodeRef& node, const std::string &name, int32 &out) {
	return asType<int32>(node, name, out);
}

bool YamlDatabase::asUInt32(const ryml::NodeRef& node, const std::string &name, uint32 &out) {
	return asType<uint32>(node, name, out);
}

bool YamlDatabase::asInt64(const ryml::NodeRef& node, const std::string &name, int64 &out) {
	return asType<int64>(node, name, out);
}

bool YamlDatabase::asUInt64(const ryml::NodeRef& node, const std::string &name, uint64 &out) {
	return asType<uint64>(node, name, out);
}

bool YamlDatabase::asFloat(const ryml::NodeRef& node, const std::string &name, float &out) {
	return asType<float>(node, name, out);
}

bool YamlDatabase::asDouble(const ryml::NodeRef& node, const std::string &name, double &out) {
	return asType<double>(node, name, out);
}

bool YamlDatabase::asString(const ryml::NodeRef& node, const std::string &name, std::string &out) {
	return asType<std::string>(node, name, out);
}

bool YamlDatabase::asUInt16Rate( const ryml::NodeRef& node, const std::string& name, uint16& out, uint16 maximum ){
	if( this->asUInt16( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[c4::to_csubstr(name)], "Node \"%s\" with value %" PRIu16 " exceeds maximum of %" PRIu16 ".\n", name.c_str(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[c4::to_csubstr(name)], "Node \"%s\" needs to be at least 1.\n", name.c_str() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

bool YamlDatabase::asUInt32Rate( const ryml::NodeRef& node, const std::string& name, uint32& out, uint32 maximum ){
	if( this->asUInt32( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[c4::to_csubstr(name)], "Node \"%s\" with value %" PRIu32 " exceeds maximum of %" PRIu32 ".\n", name.c_str(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[c4::to_csubstr(name)], "Node \"%s\" needs to be at least 1.\n", name.c_str() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

int32 YamlDatabase::getLineNumber(const ryml::NodeRef& node) {
	return parser.source().has_str() ? (int32)parser.location(node).line : 0;
}

int32 YamlDatabase::getColumnNumber(const ryml::NodeRef& node) {
	return parser.source().has_str() ? (int32)parser.location(node).col : 0;
}

void YamlDatabase::invalidWarning( const ryml::NodeRef& node, const char* fmt, ... ){
	va_list ap;

	va_start(ap, fmt);

	// Remove any remaining garbage of a previous loading line
	ShowMessage( CL_CLL );
	// Print the actual error
	_vShowMessage( MSG_ERROR, fmt, ap );

	va_end(ap);

	ShowError( "Occurred in file '" CL_WHITE "%s" CL_RESET "' on line %d and column %d.\n", this->currentFile.c_str(), this->getLineNumber(node), this->getColumnNumber(node));

#ifdef DEBUG
	std::cout << node;
#endif
}

std::string YamlDatabase::getCurrentFile(){
	return this->currentFile;
}

void on_yaml_error( const char* msg, size_t len, ryml::Location loc, void *user_data ){
	throw std::runtime_error( msg );
}

void do_init_database(){
	ryml::set_callbacks( ryml::Callbacks( nullptr, nullptr, nullptr, on_yaml_error ) );
}

#ifdef Pandas_YamlBlastCache_Serialize
bool BlastCache::saveToCache() {
	if (!this->bEnabledCache) {
		return false;
	}

	try
	{
		uint32 i = 0;
		IniParser rocketConfig("db/cache/rocket.ini");

		// 遍历所有关联的文件, 写入他们的路径和 Hash
		for (const auto& f : this->includeFiles) {
			std::string hash = crypto_GetFileMD5(f);
			rocketConfig.Set(this->type + ".RelatedFile" + std::to_string(i) + "_Path", f.c_str());
			rocketConfig.Set(this->type + ".RelatedFile" + std::to_string(i) + "_Hash", hash.c_str());
			i++;
		}

		// 记录关联文件的总数
		rocketConfig.Set(this->type + ".RelatedFileCount", std::to_string(i));

		// 记录序列化缓存文件的所在路径
		std::string blashPath = this->getCachePath();
		rocketConfig.Set(this->type + ".BlastCachePath", blashPath);

		// 执行序列化 (保存缓存数据)
		bool fireResult = false;
		{
			std::ofstream file(blashPath, SERIALIZE_SAVE_STREAM_FLAG);
			SERIALIZE_SAVE_ARCHIVE saveArchive(file);
			fireResult = this->fireSerialize<SERIALIZE_SAVE_ARCHIVE>(saveArchive);
		}

		// 如果缓存成功, 那么记录缓存的 Hash 和版本号
		if (fireResult) {
			std::string blashHash = this->getCacheHash(blashPath);
			rocketConfig.Set(this->type + ".BlastCacheHash", blashHash);
		}

		return fireResult;
	}
	catch (const std::exception& e)
	{
		ShowError("%s: %s\n", __func__, e.what());
		return false;
	}
}

bool BlastCache::loadFromCache() {
	if (!this->bEnabledCache) {
		return false;
	}

	try
	{
		std::string blashPath = this->getCachePath();
		if (this->isCacheEffective() && isFileExists(blashPath)) {
			performance_create_and_start("blastcache");
			ShowStatus("Loading " CL_WHITE "%s" CL_RESET " from blast cache..." CL_CLL "\r", this->type.c_str());

			std::ifstream file(blashPath, SERIALIZE_LOAD_STREAM_FLAG);
			SERIALIZE_LOAD_ARCHIVE loadArchive(file);
			if (this->fireSerialize<SERIALIZE_LOAD_ARCHIVE>(loadArchive)) {
				performance_stop("blastcache");
				ShowStatus("Done reading " CL_WHITE "%s" CL_RESET " from blast cache, took %" PRIu64 " milliseconds...\n", this->type.c_str(), performance_get_milliseconds("blastcache"));
				performance_destory("blastcache");

				this->afterCacheRestore();
				return true;
			}
			else {
				performance_destory("blastcache");
			}
		}
		return false;
	}
	catch (const std::exception& e)
	{
		ShowError("%s: %s\n", __func__, e.what());
		return false;
	}
}

bool BlastCache::isCacheEffective() {
	IniParser rocketConfig("db/cache/rocket.ini");
	std::string blastCachePath = this->getCachePath();
	std::string blastCacheHash = this->getCacheHash(blastCachePath);
	uint32 count = rocketConfig.Get<uint32>(this->type + ".RelatedFileCount", 0);

	if (rocketConfig.Get<std::string>(this->type + ".BlastCachePath", "") != blastCachePath) {
		return false;
	}

	if (blastCacheHash.length() == 0 ||
		rocketConfig.Get<std::string>(this->type + ".BlastCacheHash", "") != blastCacheHash) {
		return false;
	}

	for (uint32 cur = 0; cur < count; cur++) {
		std::string cfg_path = rocketConfig.Get<std::string>(this->type + ".RelatedFile" + std::to_string(cur) + "_Path", "");
		std::string cfg_hash = rocketConfig.Get<std::string>(this->type + ".RelatedFile" + std::to_string(cur) + "_Hash", "");

		if (!isFileExists(cfg_path) || cfg_hash.length() == 0) {
			return false;
		}

		std::string current_hash = crypto_GetFileMD5(cfg_path);
		if (current_hash != cfg_hash) {
			return false;
		}
	}

	return true;
}

bool BlastCache::removeCache() {
	std::string blashCachePath = this->getCachePath();
	if (isFileExists(blashCachePath)) {
		return deleteFile(blashCachePath);
	}
	return false;
}

const std::string BlastCache::getCacheHash(const std::string& path) {
	if (!isFileExists(path))
		return "";

	std::string filehash = crypto_GetFileMD5(path);
	if (filehash.length() == 0)
		return "";

	std::string depends = this->getDependsHash();

	std::string content = boost::str(
		boost::format("%1%|%2%|%3%|%4%|%5%|%6%|%7%|%8%") %
		getPandasVersion() %
		BLASTCACHE_VERSION %
		typeid(SERIALIZE_LOAD_ARCHIVE).name() %
		typeid(SERIALIZE_SAVE_ARCHIVE).name() %
		this->version %
		filehash % depends %
		this->datatypeSize
	);

	return crypto_GetStringMD5(content);
}

const std::string BlastCache::getCacheHashByName(const std::string& db_name) {
 	IniParser rocketConfig("db/cache/rocket.ini");
 	return rocketConfig.Get<std::string>(db_name + ".BlastCacheHash", "");
}

const std::string BlastCache::getCachePath() {
#if defined(PRERE) || !defined(RENEWAL)
	std::string mode("pre");
#else
	std::string mode("re");
#endif // defined(PRERE) || !defined(RENEWAL)
	return std::string(
		"db/cache/" +
		boost::algorithm::to_lower_copy(this->type) + "_" +
		mode + ".blast"
	);
}
#endif // Pandas_YamlBlastCache_Serialize

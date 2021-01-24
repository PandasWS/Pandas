// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "database.hpp"

#include "showmsg.hpp"

#ifdef Pandas_YamlBlastCache_Serialize
#include "configparser.hpp"
#include "cryptopp.hpp"
#endif // Pandas_YamlBlastCache_Serialize

#ifdef Pandas_Database_Yaml_Support_UTF8BOM
#include "utf8.hpp"
#endif // Pandas_Database_Yaml_Support_UTF8BOM

#ifdef Pandas_Database_Yaml_BeQuiet
	#define ShowError if ((this->quietLevel & 4) != 4) ::ShowError
	#define ShowWarning if ((this->quietLevel & 2) != 2) ::ShowWarning
	#define ShowStatus if ((this->quietLevel & 1) != 1) ::ShowStatus
#endif // Pandas_Database_Yaml_BeQuiet

#ifdef Pandas_Database_Yaml_Support_UTF8BOM
//************************************
// Method:      LoadFile
// Description: 实现 YAML::LoadFile 的替代版本进行 UTF8-BOM 处理
// Parameter:   const std::string & filename
// Returns:     YAML::Node
// Author:      Sola丶小克(CairoLee)  2020/01/28 12:11
//************************************
YAML::Node YamlDatabase::LoadFile(const std::string& filename) {
	std::ifstream fin(filename.c_str());
	if (!fin || fin.bad()) {
		throw YAML::BadFile();
	}

	// 若不是 UTF8-BOM 编码则走原来的流程
	if (PandasUtf8::fmode(fin) != PandasUtf8::FILE_CHARSETMODE_UTF8_BOM) {
		return YAML::Load(fin);
	}

	// 先跳过最开始的标记位, UTF8-BOM 是三个字节
	fin.seekg(3, std::ios::beg);

	// 先读取来源文件的全部数据并保存到内存中
	std::stringstream buffer;
	buffer << fin.rdbuf();
	std::string contents(buffer.str());
	fin.close();

	// 执行对应的编码转换, 并传递给 YAML::Load 进行加载
	contents = PandasUtf8::utf8ToAnsi(contents);
	return YAML::Load(contents);
}
#endif // Pandas_Database_Yaml_Support_UTF8BOM

bool YamlDatabase::nodeExists( const YAML::Node& node, const std::string& name ){
	try{
		const YAML::Node &subNode = node[name];

		if( subNode.IsDefined() && !subNode.IsNull() ){
			return true;
		}else{
			return false;
		}
	// TODO: catch( ... ) instead?
	}catch( const YAML::InvalidNode& ){
		return false;
	}catch( const YAML::BadSubscript& ){
		return false;
	}
}

bool YamlDatabase::nodesExist( const YAML::Node& node, std::initializer_list<const std::string> names ){
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

bool YamlDatabase::verifyCompatibility( const YAML::Node& rootNode ){
	if( !this->nodeExists( rootNode, "Header" ) ){
		ShowError( "No database \"Header\" was found.\n" );
		return false;
	}

	const YAML::Node& headerNode = rootNode["Header"];

	if( !this->nodeExists( headerNode, "Type" ) ){
		ShowError( "No database \"Type\" was found.\n" );
		return false;
	}

	const YAML::Node& typeNode = headerNode["Type"];
	const std::string& tmpType = typeNode.as<std::string>();

	if( tmpType != this->type ){
		ShowError( "Database type mismatch: %s != %s.\n", this->type.c_str(), tmpType.c_str() );
		return false;
	}

	uint16 tmpVersion;

	if( !this->asUInt16( headerNode, "Version", tmpVersion ) ){
		ShowError("Invalid header \"Version\" type for %s database.\n", this->type.c_str());
		return false;
	}

	if( tmpVersion != this->version ){
		if( tmpVersion > this->version ){
			ShowError( "Database version %hu is not supported. Maximum version is: %hu\n", tmpVersion, this->version );
			return false;
		}else if( tmpVersion >= this->minimumVersion ){
			ShowWarning( "Database version %hu is outdated and should be updated. Current version is: %hu\n", tmpVersion, this->minimumVersion );
		}else{
			ShowError( "Database version %hu is not supported anymore. Minimum version is: %hu\n", tmpVersion, this->minimumVersion );
			return false;
		}
	}

	return true;
}

#ifdef Pandas_YamlBlastCache_Serialize
//************************************
// Method:      isCacheEffective
// Description: 当前序列化缓存是否还有效
// Access:      private 
// Returns:     bool 有效返回 true 否则返回 false
// Author:      Sola丶小克(CairoLee)  2021/01/17 20:32
//************************************ 
bool YamlDatabase::isCacheEffective() {
	IniParser rocketConfig("db/cache/rocket.ini");
	std::string blashPath = this->getBlastCachePath();
	std::string blashHash = this->getBlashCacheHash(blashPath);
	uint32 count = rocketConfig.Get<uint32>(this->type + ".COUNT", 0);

	if (rocketConfig.Get<std::string>(this->type + ".CACHE", "") != blashPath) {
		return false;
	}

	if (blashHash.length() == 0 ||
		rocketConfig.Get<std::string>(this->type + ".BLAST", "") != blashHash) {
		return false;
	}

	for (uint32 cur = 0; cur < count; cur++) {
		std::string cfg_path = rocketConfig.Get<std::string>(this->type + ".FILE_" + std::to_string(cur), "");
		std::string cfg_hash = rocketConfig.Get<std::string>(this->type + ".HASH_" + std::to_string(cur), "");

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

//************************************
// Method:      getBlastCachePath
// Description: 获取当前数据库的缓存文件保存路径
// Access:      private 
// Returns:     std::string
// Author:      Sola丶小克(CairoLee)  2021/01/19 22:52
//************************************ 
std::string YamlDatabase::getBlastCachePath() {
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

//************************************
// Method:      getBlashCacheHash
// Description: 获取缓存文件的散列特征
// Access:      private 
// Parameter:   const std::string & path
// Returns:     std::string 失败则返回空字符串, 成功则返回 MD5 散列
// Author:      Sola丶小克(CairoLee)  2021/01/24 13:54
//************************************ 
std::string YamlDatabase::getBlashCacheHash(const std::string& path) {
	if (!isFileExists(path))
		return "";

	std::string filehash = crypto_GetFileMD5(path);
	if (filehash.length() == 0)
		return "";

	std::string content = boost::str(
		boost::format("%1%|%2%|%3%|%4%") %
		BLASTCACHE_VERSION %
		typeid(SERIALIZE_LOAD_ARCHIVE).name() %
		typeid(SERIALIZE_SAVE_ARCHIVE).name() %
		filehash
	);

	return crypto_GetStringMD5(content);
}

//************************************
// Method:      loadFromSerialize
// Description: 从序列化缓存文件中恢复当前对象
// Access:      private 
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/01/14 00:21
//************************************ 
bool YamlDatabase::loadFromSerialize() {
	if (!this->supportSerialize) {
		return false;
	}

	try
	{
		std::string blashPath = this->getBlastCachePath();
		if (this->isCacheEffective() && isFileExists(blashPath)) {
			performance_create_and_start("yaml_blastcache");
			ShowStatus("Loading " CL_WHITE "%s" CL_RESET " from blast cache..." CL_CLL "\r", this->type.c_str());

			std::ifstream file(blashPath, SERIALIZE_LOAD_STREAM_FLAG);
			SERIALIZE_LOAD_ARCHIVE loadArchive(file);
			if (this->fireSerialize<SERIALIZE_LOAD_ARCHIVE>(loadArchive)) {
				performance_stop("yaml_blastcache");
				ShowStatus("Done reading " CL_WHITE "%s" CL_RESET " from blast cache, took %" PRIu64 " milliseconds...\n", this->type.c_str(), performance_get_milliseconds("yaml_blastcache"));
				performance_destory("yaml_blastcache");

				this->afterSerialize();
				return true;
			}
			else {
				performance_destory("yaml_blastcache");
			}
		}
		return false;
	}
	catch (const std::exception& e)
	{
		ShowError("%s: %s\n", __func__, e.what());
		return false;
	}
};

//************************************
// Method:      saveToSerialize
// Description: 将当前对象保存到序列化缓存文件中去
// Access:      private 
// Returns:     bool
// Author:      Sola丶小克(CairoLee)  2021/01/14 00:21
//************************************ 
bool YamlDatabase::saveToSerialize() {
	if (!this->supportSerialize) {
		return false;
	}

	try
	{
		uint32 i = 0;
		IniParser rocketConfig("db/cache/rocket.ini");
		for (auto f : this->includeFiles) {
			std::string hash = crypto_GetFileMD5(f);
			rocketConfig.Set(this->type + ".FILE_" + std::to_string(i), f.c_str());
			rocketConfig.Set(this->type + ".HASH_" + std::to_string(i), hash.c_str());
			i++;
		}
		rocketConfig.Set(this->type + ".COUNT", std::to_string(i));

		std::string blashPath = this->getBlastCachePath();
		rocketConfig.Set(this->type + ".CACHE", blashPath);

		bool fireResult = false;
		{
			std::ofstream file(blashPath, SERIALIZE_SAVE_STREAM_FLAG);
			SERIALIZE_SAVE_ARCHIVE saveArchive(file);
			fireResult = this->fireSerialize<SERIALIZE_SAVE_ARCHIVE>(saveArchive);
		}

		if (fireResult) {
			std::string blashHash = this->getBlashCacheHash(blashPath);
			rocketConfig.Set(this->type + ".BLAST", blashHash);
		}

		return fireResult;
	}
	catch (const std::exception& e)
	{
		ShowError("%s: %s\n", __func__, e.what());
		return false;
	}
};
#endif // Pandas_YamlBlastCache_Serialize

bool YamlDatabase::load(){
#ifndef Pandas_YamlBlastCache_Serialize
	bool ret = this->load( this->getDefaultLocation() );

	this->loadingFinished();
#else
	this->includeFiles.clear();

	if (this->loadFromSerialize()) {
		this->loadingFinished();
		return true;
	}

	bool ret = this->load( this->getDefaultLocation() );
	this->loadingFinished();
	this->saveToSerialize();
#endif // Pandas_YamlBlastCache_Serialize

	return ret;
}

bool YamlDatabase::reload(){
	this->clear();

	return this->load();
}

bool YamlDatabase::load(const std::string& path) {
	YAML::Node rootNode;

#ifdef Pandas_Console_Translate
	// 若启用了控制台信息翻译机制
	// 那么允许指定一个空的 yaml 文件路径, 表示不加载任何数据
	if (path.empty()) {
		return true;
	}
#endif // Pandas_Console_Translate

#ifdef Pandas_YamlBlastCache_Serialize
	this->includeFiles.insert(path);
#endif // Pandas_YamlBlastCache_Serialize

	try {
#ifdef Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
		performance_create_and_start("yamldatabase_load");
#endif // Pandas_Speedup_Print_TimeConsuming_Of_KeySteps

		ShowStatus( "Loading '" CL_WHITE "%s" CL_RESET "'..." CL_CLL "\r", path.c_str() );
#ifndef Pandas_Database_Yaml_Support_UTF8BOM
		rootNode = YAML::LoadFile(path);
#else
		rootNode = this->LoadFile(path);
#endif // Pandas_Database_Yaml_Support_UTF8BOM
	}
	catch(YAML::Exception &e) {
		ShowError("Failed to read %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), path.c_str());
		ShowError("%s (Line %d: Column %d)\n", e.msg.c_str(), e.mark.line, e.mark.column);
		return false;
	}

	// Required here already for header error reporting
	this->currentFile = path;

	if (!this->verifyCompatibility(rootNode)){
		ShowError("Failed to verify compatibility with %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), this->currentFile.c_str());
		return false;
	}

	const YAML::Node& header = rootNode["Header"];

	if( this->nodeExists( header, "Clear" ) ){
		bool clear;

		if( this->asBool( header, "Clear", clear ) && clear ){
			this->clear();
		}
	}

	this->parse( rootNode );

	this->parseImports( rootNode );

	return true;
}

void YamlDatabase::loadingFinished(){
	// Does nothing by default, just for hooking
}

void YamlDatabase::parse( const YAML::Node& rootNode ){
	uint64 count = 0;

	if( this->nodeExists( rootNode, "Body" ) ){
		const YAML::Node& bodyNode = rootNode["Body"];
		size_t childNodesCount = bodyNode.size();
		size_t childNodesProgressed = 0;
		const char* fileName = this->currentFile.c_str();

		for( const YAML::Node &node : bodyNode ){
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

void YamlDatabase::parseImports( const YAML::Node& rootNode ){
	if( this->nodeExists( rootNode, "Footer" ) ){
		const YAML::Node& footerNode = rootNode["Footer"];

		if( this->nodeExists( footerNode, "Imports") ){
			for( const YAML::Node& node : footerNode["Imports"] ){
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

				this->load( importFile );
			}
		}
	}
}

template <typename R> bool YamlDatabase::asType( const YAML::Node& node, const std::string& name, R& out ){
	if( this->nodeExists( node, name ) ){
		const YAML::Node& dataNode = node[name];

		try {
			R value = dataNode.as<R>();

			out = value;

			return true;
		}catch( const YAML::BadConversion& ){
			this->invalidWarning( dataNode, "Unable to parse \"%s\".\n", name.c_str() );
			return false;
		}
	}else{
		this->invalidWarning( node, "Missing node \"%s\".\n", name.c_str() );
		return false;
	}
}

bool YamlDatabase::asBool(const YAML::Node &node, const std::string &name, bool &out) {
	return asType<bool>(node, name, out);
}

bool YamlDatabase::asInt16( const YAML::Node& node, const std::string& name, int16& out ){
	return asType<int16>( node, name, out);
}

bool YamlDatabase::asUInt16(const YAML::Node& node, const std::string& name, uint16& out) {
	return asType<uint16>(node, name, out);
}

bool YamlDatabase::asInt32(const YAML::Node &node, const std::string &name, int32 &out) {
	return asType<int32>(node, name, out);
}

bool YamlDatabase::asUInt32(const YAML::Node &node, const std::string &name, uint32 &out) {
	return asType<uint32>(node, name, out);
}

bool YamlDatabase::asInt64(const YAML::Node &node, const std::string &name, int64 &out) {
	return asType<int64>(node, name, out);
}

bool YamlDatabase::asUInt64(const YAML::Node &node, const std::string &name, uint64 &out) {
	return asType<uint64>(node, name, out);
}

bool YamlDatabase::asFloat(const YAML::Node &node, const std::string &name, float &out) {
	return asType<float>(node, name, out);
}

bool YamlDatabase::asDouble(const YAML::Node &node, const std::string &name, double &out) {
	return asType<double>(node, name, out);
}

bool YamlDatabase::asString(const YAML::Node &node, const std::string &name, std::string &out) {
	return asType<std::string>(node, name, out);
}

bool YamlDatabase::asUInt16Rate( const YAML::Node& node, const std::string& name, uint16& out, uint16 maximum ){
	if( this->asUInt16( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[name], "Node \"%s\" with value %" PRIu16 " exceeds maximum of %" PRIu16 ".\n", name.c_str(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[name], "Node \"%s\" needs to be at least 1.\n", name.c_str() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

bool YamlDatabase::asUInt32Rate( const YAML::Node& node, const std::string& name, uint32& out, uint32 maximum ){
	if( this->asUInt32( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[name], "Node \"%s\" with value %" PRIu32 " exceeds maximum of %" PRIu32 ".\n", name.c_str(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[name], "Node \"%s\" needs to be at least 1.\n", name.c_str() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

void YamlDatabase::invalidWarning( const YAML::Node &node, const char* fmt, ... ){
	va_list ap;

	va_start(ap, fmt);

	// Remove any remaining garbage of a previous loading line
	ShowMessage( CL_CLL );
	// Print the actual error
	_vShowMessage( MSG_ERROR, fmt, ap );

	va_end(ap);

	ShowError( "Occurred in file '" CL_WHITE "%s" CL_RESET "' on line %d and column %d.\n", this->currentFile.c_str(), node.Mark().line + 1, node.Mark().column );

#ifdef DEBUG
	YAML::Emitter out;

	out << node;

	ShowMessage( "%s\n", out.c_str() );
#endif
}

std::string YamlDatabase::getCurrentFile(){
	return this->currentFile;
}

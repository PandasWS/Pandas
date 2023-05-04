// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "database.hpp"

#include <iostream>
#include <sstream>

#include "malloc.hpp"
#include "showmsg.hpp"
#include "utilities.hpp"

using namespace rathena;

#ifdef Pandas_Database_Yaml_BeQuiet
	#define ShowError if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 4) != 4) ::ShowError
	#define ShowWarning if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 2) != 2) ::ShowWarning
	#define ShowStatus if (!this->p || (((YamlDatabase*)this->p)->quietLevel & 1) != 1) ::ShowStatus
#endif // Pandas_Database_Yaml_BeQuiet

bool YamlDatabase::nodeExists( const ryml::NodeRef& node, const std::string& name ){
#ifndef Pandas_UserExperience_Yaml_Error
	return (node.num_children() > 0 && node.has_child(c4::to_csubstr(name)));
#else
	// - 将技能数据库中某个 Requires 节点的 SpCost 删掉, 但别进行缩进对齐
	// - 加载的时候就会之别抛出错误崩溃掉
	
	bool bResult = false;
	
	try {
		bResult = (node.num_children() > 0 && node.has_child(c4::to_csubstr(name)));
	}
	catch (std::runtime_error const& err) {
		ShowError("Error while looking for the '%s' node: %s\n", name.c_str(), err.what());
		ShowError("Occurred in file '" CL_WHITE "%s" CL_RESET "' on line %d and column %d.\n", this->currentFile.c_str(), this->getLineNumber(node), this->getColumnNumber(node));

#ifdef DEBUG
		std::cout << node;
#endif // DEBUG

		return false;
	}

	return bResult;
#endif // Pandas_UserExperience_Yaml_Error
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
	bool ret = this->load(this->getDefaultLocation());

	this->loadingFinished();

	return ret;
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
#ifndef Pandas_Support_UTF8BOM_Files
	char* buf = (char *)aMalloc(size+1);
	rewind(f);
	size_t real_size = fread(buf, sizeof(char), size, f);
#else
	// 潜在的编码转换需要, 将字节数与 wchar_t 的大小相乘
	size = size * sizeof(wchar_t) + 1;
	char* buf = (char *)aMalloc(size);
	rewind(f);
	// 加载终端翻译数据库的时候标记位需要传递为 0x2
	// 表示在将其 UTF8 内容转换成 BIG5 编码的时候需要在低字节位为 0x5C 的字符后追加反斜杠
	// 这样处理后在终端显示出对应的汉字，比如：連線成功 最末尾的【功】才能正常显示
	int flag = (this->type == "CONSOLE_TRANSLATE_DB" ? 0x2 : 0);
	size_t real_size = _fread(buf, sizeof(char), size, f, flag);
#endif // Pandas_Support_UTF8BOM_Files
	// Zero terminate
	buf[real_size] = '\0';
	fclose(f);

	parser = {};
	ryml::Tree tree;

	try{
		tree = parser.parse_in_arena(c4::to_csubstr(path), c4::to_csubstr(buf));
	}catch( const std::runtime_error& e ){
		ShowError( "Failed to load %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), path.c_str() );
		ShowError( "There is likely a syntax error in the file.\n" );
		ShowError( "Error message: %s\n", e.what() );
		aFree(buf);
		return false;
	}

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

	this->parseImports( tree );

	aFree(buf);
	return true;
}

void YamlDatabase::loadingFinished(){
	// Does nothing by default, just for hooking
}

void YamlDatabase::parse( const ryml::Tree& tree ){
	uint64 count = 0;

	if( this->nodeExists( tree.rootref(), "Body" ) ){
		const ryml::NodeRef& bodyNode = tree["Body"];
		size_t childNodesCount = bodyNode.num_children();
		const char* fileName = this->currentFile.c_str();
#ifdef DEBUG
		size_t childNodesProgressed = 0;
#endif

		ShowStatus("Loading '" CL_WHITE "%" PRIdPTR CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'\n", childNodesCount, fileName);

		for( const ryml::NodeRef &node : bodyNode ){
			count += this->parseBodyNode( node );
#ifdef DETAILED_LOADING_OUTPUT
			ShowStatus( "Loading [%" PRIdPTR "/%" PRIdPTR "] entries from '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\r", ++childNodesProgressed, childNodesCount, fileName );
#endif
		}

#ifndef Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
		ShowStatus( "Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\n", count, fileName );
#else
		performance_stop("yamldatabase_load");
		ShowStatus( "Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "' (took %" PRIu64 " milliseconds)" CL_CLL "\n", count, fileName, performance_get_milliseconds("yamldatabase_load"));
#endif // Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
	}
#ifdef Pandas_UserExperience_Output_Ending_Even_Body_Node_Is_Not_Exists
	else {
		// 当数据文件中不存在 Body 节点时也依然输出结尾信息
		const char* fileName = this->currentFile.c_str();
#ifndef Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
		ShowStatus("Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\n", count, fileName);
#else
		performance_stop("yamldatabase_load");
		ShowStatus("Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "' (took %" PRIu64 " milliseconds)" CL_CLL "\n", count, fileName, performance_get_milliseconds("yamldatabase_load"));
#endif // Pandas_Speedup_Print_TimeConsuming_Of_KeySteps
	}
#endif // Pandas_UserExperience_Output_Ending_Even_Body_Node_Is_Not_Exists
}

void YamlDatabase::parseImports( const ryml::Tree& rootNode ){
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

				if (this->nodeExists(node, "Generator")) {
					bool isGenerator;
					if (!this->asBool(node, "Generator", isGenerator)) {
						continue;
					}
					if (!(shouldLoadGenerator && isGenerator))
						continue; // skip import
				}

				this->load( importFile );
			}
		}
	}
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
#ifndef Pandas_UserExperience_Yaml_Error
	return parser.source().has_str() ? (int32)parser.location(node).line : 0;
#else
	try {
		// 读取到的行号应该 + 1 才是正确的行号
		return parser.source().has_str() ? (int32)parser.location(node).line + 1 : 0;
	}
	catch (std::runtime_error) {
#ifdef DEBUG
		std::cout << node;
#endif // DEBUG
		return 0;
	}
#endif // Pandas_UserExperience_Yaml_Error
}

int32 YamlDatabase::getColumnNumber(const ryml::NodeRef& node) {
#ifndef Pandas_UserExperience_Yaml_Error
	return parser.source().has_str() ? (int32)parser.location(node).col : 0;
#else
	try {
		return parser.source().has_str() ? (int32)parser.location(node).col : 0;
	}
	catch (std::runtime_error) {
#ifdef DEBUG
		std::cout << node;
#endif // DEBUG
		return 0;
	}
#endif // Pandas_UserExperience_Yaml_Error
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

void YamlDatabase::setGenerator(bool shouldLoad) {
	shouldLoadGenerator = shouldLoad;
}

void on_yaml_error( const char* msg, size_t len, ryml::Location loc, void *user_data ){
#ifndef Pandas_UserExperience_Yaml_Error
	throw std::runtime_error( msg );
#else
	std::shared_ptr<char> mes(new char[len + 1], std::default_delete<char[]>());
	memset(mes.get(), 0, len + 1);
	memcpy(mes.get(), msg, len);
	std::string text = strTrim(mes.get());
	throw std::runtime_error(text.c_str());
#endif // Pandas_UserExperience_Yaml_Error
}

void do_init_database(){
	ryml::set_callbacks( ryml::Callbacks( nullptr, nullptr, nullptr, on_yaml_error ) );
}

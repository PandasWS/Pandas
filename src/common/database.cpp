﻿// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "database.hpp"

#include "showmsg.hpp"

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
		if( node[name] ){
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

bool YamlDatabase::load(){
	return this->load( this->getDefaultLocation() );
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

	try {
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

	this->loadingFinished();

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

		ShowStatus( "Done reading '" CL_WHITE "%" PRIu64 CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\n", count, fileName );
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

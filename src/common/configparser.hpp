#pragma once

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "assistant.hpp"

using namespace boost::property_tree;

class IniParser {
private:
	std::string m_path;
	ptree m_data;
public:
	IniParser(const std::string& path) {
		try
		{
			m_path = path;
			if (isFileExists(path)) {
				ini_parser::read_ini(m_path, m_data);
			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	~IniParser() {
		this->Save();
	}

	void Save(const std::string& path = "") {
		try
		{
			std::string inipath = (path.length() ? path : m_path);
			if (ensureDirectories(inipath)) {
				ini_parser::write_ini(inipath, m_data);
			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	template <typename T>
	T Get(const std::string& cfgpath) {
		return m_data.get<T>(cfgpath);
	}

	template <typename T>
	void Set(const std::string& cfgpath, const T& value) {
		m_data.put(cfgpath, value);
	}
};

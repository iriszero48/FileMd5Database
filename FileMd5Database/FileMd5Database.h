#pragma once

#include <filesystem>
#include <map>

using K = std::string;
using V = std::tuple<std::string, uint64_t, std::string>;
using Database = std::map<K, V>;

inline void FileMd5DatabaseAdd(const std::string& deviceName, const std::filesystem::path& file, Database& fmd, const bool logOutput = true);

void FileMd5DatabaseBuilder(const std::string& deviceName, const std::string& path, Database& fmd, const bool logOutput = true);

void FileMd5DatabaseQuery(Database& fmd, const std::string& queryMethod, const std::string& queryData, const std::string& keyword);

void Export(Database& fmd, const std::string& path, const std::string& format);
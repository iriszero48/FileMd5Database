#include "FileMd5Database.h"

#include <deque>
#include <regex>

#include <omp.h>

#include "Md5.h"
#include "File.h"
#include "CSV.h"
#include "String.h"
#include "Unified.h"

inline void FileMd5DatabaseAdd(const std::string& deviceName, const std::filesystem::path& file, Database& fmd, const bool logOutput)
{
	try
	{
		const auto nativePath = PathPerfix file.native();
		const auto fullPath = deviceName + ":" + file.u8string();
		const auto md5 = Md5File(nativePath);
		uintmax_t size;
		try
		{
			size = file_size(file);
		}
		catch (const std::exception& e)
		{
			LogErr(file.native().c_str(), e.what());
			size = 0;
		}
		const auto modificationTime = FileLastModified(nativePath);
		fmd[fullPath] = std::make_tuple(md5, size, modificationTime);
		if (logOutput) Log(file.native().c_str(), md5.c_str(), static_cast<uint64_t>(size), modificationTime.c_str());
	}
	catch (const std::exception& e)
	{
		LogErr(file.native().c_str(), e.what());
	}
}

void FileMd5DatabaseBuilder(const std::string& deviceName, const std::string& path, Database& fmd, const bool logOutput)
{
	std::error_code errorCode;
	const std::error_code nonErrorCode;
	std::deque<std::filesystem::path> queue{};
	const std::filesystem::directory_iterator end;
	queue.emplace_back(std::filesystem::path(path));
	while (!queue.empty())
	{
		try
		{
			for (std::filesystem::directory_iterator file(queue.front(), std::filesystem::directory_options::none, errorCode); file != end; ++file)
			{
				if (errorCode != nonErrorCode)
				{
					LogErr(file->path().native().c_str(), errorCode.message().c_str());
					errorCode.clear();
					continue;
				}
				if (file->is_symlink())
				{
					continue;
				}
				if (file->is_regular_file())
				{
					FileMd5DatabaseAdd(deviceName, file->path(), fmd, logOutput);
				}
				else if (file->is_directory())
				{
					queue.emplace_back(file->path());
				}
			}
		}
		catch (const std::exception& e)
		{
			LogErr(queue.front().native().c_str(), e.what());
		}
		queue.pop_front();
	}
}

void FileMd5DatabaseQuery(Database& fmd, const std::string& queryMethod, const std::string& queryData, const std::string& keyword)
{
#define Search(init,value,judge)\
{\
	int count = 0;\
	init;\
	OmpParallel\
	{\
		size_t cnt = 0;\
		int ithread = omp_get_thread_num();\
		int nthreads = omp_get_num_threads();\
		for(auto pair = fmd.begin(); pair != fmd.end(); ++pair, cnt++)\
		{\
			if(cnt % nthreads != ithread) continue;\
			const auto data = value;\
			if (judge)\
			{\
				printf("<%s,<%s,%" PRIu64 ",%s>>\n", pair->first.c_str(), std::get<0>(pair->second).c_str(), std::get<1>(pair->second), std::get<2>(pair->second).c_str());\
			}\
		}\
	}\
}

#define ContainSearch(value) Search(,value,data.find(keyword) != std::string::npos)
#define StartWithSearch(value) Search(,value,StartWithFunc)
#define RegexSearch(value) Search(std::regex re(keyword),value,std::regex_match(data, re))

	if (queryMethod == "contain" && queryData == "path") ContainSearch(pair->first)
	else if (queryMethod == "contain" && queryData == "md5") ContainSearch(std::get<0>(pair->second))
	else if (queryMethod == "contain" && queryData == "fileModificationTime") ContainSearch(std::get<2>(pair->second))
	else if (queryMethod == "startWith" && queryData == "path") StartWithSearch(pair->first)
	else if (queryMethod == "startWith" && queryData == "md5") StartWithSearch(std::get<0>(pair->second))
	else if (queryMethod == "startWith" && queryData == "fileModificationTime") StartWithSearch(std::get<2>(pair->second))
	else if (queryMethod == "regex" && queryData == "path") RegexSearch(pair->first)
	else if (queryMethod == "regex" && queryData == "md5") RegexSearch(std::get<0>(pair->second))
	else if (queryMethod == "regex" && queryData == "fileModificationTime") RegexSearch(std::get<2>(pair->second))
	else if (queryMethod == "eq" && queryData == "md5&size")
	{
		std::map<std::string, uint64_t> md5{};
		for (auto& pair : fmd)
		{
			const auto m = std::get<0>(pair.second);
			const auto s = std::get<1>(pair.second);
			if (!m.empty() && s != 0)
			{
				if (md5.find(m) == md5.end())
				{
					md5[m] = s;
				}
				else
				{
					if (md5[m] != s)
					{
						printf("<%s,<%s,%" PRIu64 ",%s>>\n", pair.first.c_str(), std::get<0>(pair.second).c_str(), std::get<1>(pair.second), std::get<2>(pair.second).c_str());
					}
				}
			}
		}
	}
}

void Export(Database& fmd, const std::string& path, const std::string& format)
{
	if (format == "csv")
	{
		CsvFile csv(path, ",");
		for (const auto& pair : fmd)
		{
			const auto splitPos = pair.first.find(":");
			auto _path = pair.first.substr(splitPos + 1);
			csv << ReplaceString(_path, "\\", "/")
				<< pair.first.substr(0, splitPos)
				<< std::get<0>(pair.second)
				<< std::to_string(std::get<1>(pair.second))
				<< std::get<2>(pair.second)
				<< CsvFile::EndRow;
		}
	}
}
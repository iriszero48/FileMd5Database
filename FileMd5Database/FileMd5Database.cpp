#include "FileMd5Database.h"

#include <deque>
#include <regex>
#include <sstream>
#include <queue>
#include <execution>

#include "CSV.h"
#include "Md5.h"
#include "String.h"
#include "Time.h"
#include "Macro.h"

#define LogInfo(path,md5,size,time) Log.Write("<",ToString(path),",<" ,md5, "," ,std::to_string(size), ",", time ,">>")
#define LogErr(path, message) Log.Write<LogLevel::Error>("[Error] [",ToString(path),"] [", MacroFunctionName,"] [" __FILE__ ":" MacroLine "] ", message)

ArgumentOptionCpp(LogLevel, Kill, None, Error, Info, Debug)
ArgumentOptionCpp(MatchMethod, Contain, StartWith, Regex)
ArgumentOptionCpp(Data, Path, Md5, Size, FileModificationTime)
ArgumentOptionCpp(ExportFormat, CSV, JSON)
ArgumentOptionCpp(AlterType, DeviceName, DriveLetter)

inline std::string ToString(const std::filesystem::path& path)
{
	try
	{
		std::ostringstream oss;
		oss << path;
		return oss.str();
	}
	catch (...)
	{
		return path.u8string();
	}
}

static void LogWorker()
{
	std::ofstream fs{};
	while (true)
	{
		const auto& [l, s] = Log.Chan.Read();
		if (l == LogLevel::Kill)
		{
			puts(s.c_str());
			return;
		}
		if (l <= Log.Level)
		{
			if (Log.Console)
			{
				if (l == LogLevel::Error)
				{
					fputs(s.c_str(), stderr);
				}
				else
				{
					puts(s.c_str());
				}
			}
			if (!Log.File.empty())
			{
				fs.open(Log.File, std::ios::app | std::ios::binary);
				fs << s << "\n";
				fs.close();
			}
		}
	}
}

inline std::string FileLastModified(const std::filesystem::path& file)
{
	try
	{
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			last_write_time(file)
			- decltype(std::filesystem::last_write_time({}))::clock::now()
			+ std::chrono::system_clock::now()));
		if (static_cast<int64_t>(time) < 0)
		{
			throw std::runtime_error("negative time value " + std::to_string(static_cast<int64_t>(time)));
		}
		tm local{};
		Time::Local(&local, &time);
		std::ostringstream ss{};
		ss << std::put_time(&local, "%F %T");
		return ss.str();
	}
	catch (const std::exception& ex)
	{
		LogErr(file, ex.what());
		return "";
	}
}

ModelRef::ModelRef(const std::string_view& path, const std::string_view& md5, const uint64_t size, const std::string_view& time) :Path(path), Md5(md5), Size(size), Time(time) {};

ModelRef::ModelRef()
{
	static const auto* nilStr = "";
	Path = nilStr;
	Md5 = nilStr;
	Size = 0;
	Time = nilStr;
}

void FileMd5DatabaseInit(const LogLevel& level, const std::filesystem::path& file, bool console)
{
	Log.Level = level;
	Log.File = file;
	Log.Console = console;
	Log.LogThread = std::thread(LogWorker);
	Log.LogThread.detach();
}

void FileMd5DatabaseEnd()
{
	Log.Write<LogLevel::Kill>("{ok}.");
}

inline void FileMd5DatabaseAdd(const std::string& deviceName, const std::filesystem::path& file, Database& fmd)
{
	try
	{
		const auto fixPath =
#ifdef MacroWindows
			std::filesystem::u8path(R"(\\?\)" + file.u8string());
#else
			file;
#endif
		if (Log.Level >= LogLevel::Debug) Log.Write<LogLevel::Debug>("-> ", ToString(fixPath));
		auto fullPath = deviceName;
		String::StringCombine(fullPath, ":", file.u8string());
		if (Log.Level >= LogLevel::Debug) Log.Write<LogLevel::Debug>("-> ", ToString(std::filesystem::u8path(fullPath)));
		const auto md5 = Md5File(fixPath);
		if (Log.Level >= LogLevel::Debug) Log.Write<LogLevel::Debug>("-> ", md5);
		uintmax_t size;
		try
		{
			size = file_size(fixPath);
		}
		catch (const std::exception& e)
		{
			LogErr(file, e.what());
			size = 0;
		}
		if (Log.Level >= LogLevel::Debug) Log.Write<LogLevel::Debug>("-> ", std::to_string(size));
		const auto modificationTime = FileLastModified(fixPath);
		if (Log.Level >= LogLevel::Debug) Log.Write<LogLevel::Debug>("-> ", modificationTime);
		fmd[fullPath] = V(md5, size, modificationTime);
		LogInfo(file, md5, size, modificationTime);
	}
	catch (const std::exception& e)
	{
		LogErr(file, e.what());
	}
}

void FileMd5DatabaseBuilder(const std::string& deviceName, const std::filesystem::path& path, Database& fmd)
{
	std::error_code errorCode;
	const std::error_code nonErrorCode;
	std::deque<std::filesystem::path> queue{};
	const std::filesystem::directory_iterator end;
	queue.emplace_back(path);
	while (!queue.empty())
	{
		try
		{
			for (std::filesystem::directory_iterator file(queue.front(), std::filesystem::directory_options::none, errorCode); file != end; ++file)
			{
				if (errorCode != nonErrorCode)
				{
					LogErr(file->path(), errorCode.message());
					errorCode.clear();
					continue;
				}
				if (file->is_symlink())
				{
					continue;
				}
				if (file->is_regular_file())
				{
					FileMd5DatabaseAdd(deviceName, file->path(), fmd);
				}
				else if (file->is_directory())
				{
					queue.emplace_back(file->path());
				}
			}
		}
		catch (const std::exception& e)
		{
			LogErr(queue.front(), e.what());
		}
		queue.pop_front();
	}
}

void FileMd5DatabaseQuery(const std::vector<Model>& fmdRaw, const MatchMethod& matchMethod, const Data& queryData,
	const Data& sortBy, const std::string& keyword, const uint64_t limit, const bool desc)
{
	puts(("load " + std::to_string(fmdRaw.size())).c_str());
	std::vector<ModelRef> fmd(fmdRaw.size());
	std::transform(std::execution::par_unseq, fmdRaw.begin(), fmdRaw.end(), fmd.begin(), [](const Model& model) { return ModelRef(std::string_view(model.Path.str, model.Path.size), std::string_view(model.Md5.str, model.Md5.size), model.Size, std::string_view(model.Time.str, model.Time.size)); });

#define QueryImpl(cmp, matcher)\
	{\
		std::sort(std::execution::par_unseq, fmd.begin(), fmd.end(), [](const ModelRef& a, const ModelRef& b)\
		{\
			return cmp;\
		}); \
		std::atomic_uint64_t count = 0; \
		std::copy_if(fmd.begin(), fmd.end(), std::back_inserter(res), [&](const ModelRef& model)\
		{\
				if (count.load() < limit)\
				{\
					if (matcher)\
					{\
						++count; \
						return true; \
					}\
				}\
					return false; \
		}); \
	}

#define IntCmp(member) std::less<>()((a).member, (b).member)
#define StringCmp(member) std::lexicographical_compare((a).member.begin(), (a).member.end(), (b).member.begin(), (b).member.end())
#define IntMatcher(member) std::to_string(model.member) == keyword
#define StringContainMatcher(member) std::search(model.member.begin(), model.member.end(), std::boyer_moore_horspool_searcher(keyword.begin(), keyword.end())) != model.member.end()
#define StringStartWithMatcher(member) std::equal(keyword.begin(), keyword.end(), model.member.begin())
#define StringRegexMatcher(member) std::regex_match(std::string(model.member), std::regex(keyword))

#define DescBranch(cmpFull, matchFull)\
	{\
		if (desc)\
		{\
			QueryImpl(!(cmpFull), matchFull)\
		}\
		else\
		{\
			QueryImpl(cmpFull, matchFull)\
		}\
	}

#define MatchMethodBranch(cmpFull, member)\
	{\
		if (matchMethod == MatchMethod::Contain) DescBranch(cmpFull, StringContainMatcher(member))\
		else if (matchMethod == MatchMethod::StartWith) DescBranch(cmpFull, StringStartWithMatcher(member))\
		else if (matchMethod == MatchMethod::Regex) DescBranch(cmpFull, StringRegexMatcher(member))\
	}
	
#define QueryDataBranch(cmpFull)\
	{\
		if (queryData == Data::FileModificationTime) MatchMethodBranch(cmpFull, Time)\
		else if (queryData == Data::Md5) MatchMethodBranch(cmpFull, Md5)\
		else if (queryData == Data::Path) MatchMethodBranch(cmpFull, Path)\
		else if (queryData == Data::Size) DescBranch(cmpFull, IntMatcher(Size))\
	}
	
	std::vector<ModelRef> res{};
	
	if (sortBy == Data::FileModificationTime) QueryDataBranch(StringCmp(Time))
	else if (sortBy == Data::Md5) QueryDataBranch(StringCmp(Md5))
	else if (sortBy == Data::Path) QueryDataBranch(StringCmp(Path))
	else if (sortBy == Data::Size) QueryDataBranch(IntCmp(Size))

	for (auto& model : res)
	{
		try
		{
			model.Path = std::filesystem::u8path(model.Path).string();
		}
		catch (...)
		{

		}
	}
	const auto pathLen = std::max_element(res.begin(), res.end(), [](const ModelRef& a, const ModelRef& b) { return std::less<>()(a.Path.length(), b.Path.length()); })->Path.length();
	std::vector<uint64_t> sizeLens{};
	std::transform(res.begin(), res.end(), std::back_inserter(sizeLens), [](const ModelRef& x) { return std::to_string(x.Size).length(); });
	const auto sizeLen = *std::max_element(sizeLens.begin(), sizeLens.end());
	for (const auto& model : res)
	{
		const auto sizeStr = std::to_string(model.Size);
		std::string msg{};
		String::StringCombine(msg, model.Path, std::string(pathLen - model.Path.length(), ' '), " | ", model.Md5, " | ", std::string(sizeLen - sizeStr.length(), ' '), sizeStr, " | ", model.Time);
		puts(msg.c_str());
	}
}

/*
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


	std::find
{
int count = 0; 
init; 
OmpParallel
{
size_t cnt = 0; 
int ithread = omp_get_thread_num(); 
int nthreads = omp_get_num_threads(); 
for (auto pair = fmd.begin(); pair != fmd.end(); ++pair, cnt++)
{
if (cnt % nthreads != ithread) continue; 
const auto data = value; 
if (judge)
{
printf("<%s,<%s,%" PRIu64 ",%s>>\n", pair->first.c_str(), std::get<0>(pair->second).c_str(), std::get<1>(pair->second), std::get<2>(pair->second).c_str()); \
}
}
}
}
	//if (queryMethod == "contain" && queryData == "path") ContainSearch(pair->first)
	//else if (queryMethod == "contain" && queryData == "md5") ContainSearch(std::get<0>(pair->second))
	//else if (queryMethod == "contain" && queryData == "fileModificationTime") ContainSearch(std::get<2>(pair->second))
	//else if (queryMethod == "startWith" && queryData == "path") StartWithSearch(pair->first)
	//else if (queryMethod == "startWith" && queryData == "md5") StartWithSearch(std::get<0>(pair->second))
	//else if (queryMethod == "startWith" && queryData == "fileModificationTime") StartWithSearch(std::get<2>(pair->second))
	//else if (queryMethod == "regex" && queryData == "path") RegexSearch(pair->first)
	//else if (queryMethod == "regex" && queryData == "md5") RegexSearch(std::get<0>(pair->second))
	//else if (queryMethod == "regex" && queryData == "fileModificationTime") RegexSearch(std::get<2>(pair->second))
	//else if (queryMethod == "eq" && queryData == "md5&size")
	//{
	//	std::map<std::string, uint64_t> md5{};
	//	for (auto& pair : fmd)
	//	{
	//		const auto m = std::get<0>(pair.second);
	//		const auto s = std::get<1>(pair.second);
	//		if (!m.empty() && s != 0)
	//		{
	//			if (md5.find(m) == md5.end())
	//			{
	//				md5[m] = s;
	//			}
	//			else
	//			{
	//				if (md5[m] != s)
	//				{
	//					printf("<%s,<%s,%" PRIu64 ",%s>>\n", pair.first.c_str(), std::get<0>(pair.second).c_str(), std::get<1>(pair.second), std::get<2>(pair.second).c_str());
	//				}
	//			}
	//		}
	//	}
	//}
}
*/
void Export(Database& fmd, const std::string& path, const ExportFormat& format)
{
	if (format == ExportFormat::CSV)
	{
		CsvFile csv(path, ",");
		for (const auto& [k, v] : fmd)
		{
			const auto splitPos = k.find(":");
			auto _path = k.substr(splitPos + 1);
			std::replace(_path.begin(), _path.end(), '\\', '/');
			csv << _path
				<< k.substr(0, splitPos)
				<< std::get<0>(v)
				<< std::to_string(std::get<1>(v))
				<< std::get<2>(v)
				<< CsvFile::EndRow;
		}
	}
}
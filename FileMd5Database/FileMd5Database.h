#pragma once

#include <filesystem>
#include <future>
#include <map>
#include <string>

#include "Thread.h"
#include "Arguments.h"
#include "String.h"

ArgumentOptionHpp(LogLevel, Kill, None, Error, Info, Debug)
ArgumentOptionHpp(MatchMethod, Contain, StartWith, Regex)
ArgumentOptionHpp(Data, Path, Md5, Size, FileModificationTime)
ArgumentOptionHpp(ExportFormat, CSV, JSON)
ArgumentOptionHpp(AlterType, DeviceName, DriveLetter)

class Logger
{
public:
	LogLevel Level = LogLevel::Info;
	std::filesystem::path File = {};
	bool Console = true;

	using MsgType = std::tuple<LogLevel, std::string>;
	std::thread LogThread{};
	Thread::Channel<MsgType> Chan{};

	template<LogLevel Level = LogLevel::Info, class...Args>
	void Write(Args&&... args)
	{
		WriteImpl<Level>(std::forward<Args>(args)...);
	}

private:
	template<LogLevel Level, class...Args>
	void WriteImpl(Args&&... args)
	{
		std::string msg{};
		(msg.append(args), ...);
		Chan.Write(MsgType(Level, msg));
	}
};

static Logger Log;

using K = std::string;
using V = std::tuple<std::string, uint64_t, std::string>;
using Database = std::map<K, V>;

struct StrPtr
{
	char* str;
	uint64_t size;
};

struct Model
{
	StrPtr Path;
	StrPtr Md5;
	uint64_t Size;
	StrPtr Time;
};

struct ModelStr
{
	std::string Path;
	std::string Md5;
	std::string Size;
	std::string Time;
};

struct ModelRef
{
	std::string_view Path;
	std::string_view Md5;
	uint64_t Size;
	std::string_view Time;

	ModelRef(const std::string_view& path, const std::string_view& md5, uint64_t size, const std::string_view& time);
	ModelRef();
};

void FileMd5DatabaseInit(const LogLevel& level = LogLevel::Info, const std::filesystem::path& file = {}, bool console = true);

void FileMd5DatabaseEnd();

inline void FileMd5DatabaseAdd(const std::string& deviceName, const std::filesystem::path& file, Database& fmd);

void FileMd5DatabaseBuilder(const std::string& deviceName, const std::filesystem::path& path, Database& fmd, std::vector<std::string> skips);

void FileMd5DatabaseQuery(const std::vector<Model>& fmdRaw,
	const MatchMethod& matchMethod,
	const Data& queryData,
	const Data& sortBy,
	const std::string& keyword,
	uint64_t limit,
	bool desc);

void Export(Database& fmd, const std::string& path, const ExportFormat& format);
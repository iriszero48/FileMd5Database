#pragma once

#include <filesystem>
#include <future>
#include <map>
#include <string>
#include <execution>

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

	struct MsgType
	{
		LogLevel Level;
		std::string Msg;
	};
	
	std::thread LogThread{};
	Thread::Channel<MsgType> Chan{};

	template<LogLevel Level = LogLevel::Info, typename...Args>
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
		Chan.Write(MsgType{ Level, msg });
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

template <typename T = void> struct IntCmp {};
template <typename T = void> struct StringCmp {};

template<>
struct IntCmp<void>
{
	template<typename T>
	constexpr auto operator()(const T& a, const T& b) const
	{
		return std::less<>()(a, b);
	}
};

template<>
struct StringCmp<void>
{
	template<typename T>
	constexpr auto operator()(const T& a, const T& b) const
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}
};

template<Data DataValue, typename Model> struct DataToMember {};

template<typename Model>
struct DataToMember<Data::FileModificationTime, Model> { constexpr auto operator()(const Model& model) const { return model.Time; } };

template<typename Model>
struct DataToMember<Data::Md5, Model> { constexpr auto operator()(const Model& model) const { return model.Md5; } };

template<typename Model>
struct DataToMember<Data::Path, Model> { constexpr auto operator()(const Model& model) const { return model.Path; } };

template<typename Model>
struct DataToMember<Data::Size, Model> { constexpr auto operator()(const Model& model) const { return model.Size; } };

template<Data DataValue>
struct ModelCmp
{
	template<typename Model, typename Cmp>
	constexpr auto operator()(const Cmp& cmp, const Model& a, const Model& b)
	{
		return cmp(DataToMember<DataValue, Model>()(a), DataToMember<DataValue, Model>()(b));
	}
};

template<Data DataValue>
struct ModelIntCmp
{
	template<typename Model>
	constexpr auto operator()(const Model& a, const Model& b)
	{
		return ModelCmp<DataValue>()(IntCmp<>(), a, b);
	}
};

template<Data DataValue>
struct ModelStringCmp
{
	template<typename Model>
	constexpr auto operator()(const Model& a, const Model& b)
	{
		return ModelCmp<DataValue>()(StringCmp<>(), a, b);
	}
};

template<typename Fmd, typename Cmp>
constexpr auto ModelSort(Fmd& fmd, const Cmp& cmp)
{
	std::sort(std::execution::par_unseq, fmd.begin(), fmd.end(), cmp);
}

template<typename Fmd>
constexpr void ModelSortBranch(Fmd& fmd, const Data& sortBy)
{
	if (sortBy == Data::FileModificationTime) ModelSort(fmd, ModelStringCmp<Data::FileModificationTime>());
	else if (sortBy == Data::Md5) ModelSort(fmd, ModelStringCmp<Data::Md5>());
	else if (sortBy == Data::Path) ModelSort(fmd, ModelStringCmp<Data::Path>());
	else if (sortBy == Data::Size) ModelSort(fmd, ModelIntCmp<Data::Size>());
}

template<MatchMethod Method, typename Fmd>
constexpr void ModelMatchBranchImpl(Fmd& fmd, const Data& sortBy)
{
	if (sortBy == Data::FileModificationTime) ModelSort(fmd, ModelStringCmp<Data::FileModificationTime>());
	else if (sortBy == Data::Md5) ModelSort(fmd, ModelStringCmp<Data::Md5>());
	else if (sortBy == Data::Path) ModelSort(fmd, ModelStringCmp<Data::Path>());
	else if (sortBy == Data::Size) ModelSort(fmd, ModelIntCmp<Data::Size>());
}

template<MatchMethod Method, typename Fmd>
constexpr void ModelMatchBranch(Fmd& fmd, const Data& sortBy)
{
	if (sortBy == Data::FileModificationTime) ModelSort(fmd, ModelStringCmp<Data::FileModificationTime>());
	else if (sortBy == Data::Md5) ModelSort(fmd, ModelStringCmp<Data::Md5>());
	else if (sortBy == Data::Path) ModelSort(fmd, ModelStringCmp<Data::Path>());
	else if (sortBy == Data::Size) ModelSort(fmd, ModelIntCmp<Data::Size>());
}

template<typename Fmd>
void ModelRevBranch(Fmd& fmd, const bool rev)
{
	if (rev)
	{
		std::reverse(std::execution::par_unseq, fmd.begin(), fmd.end());
	}
}


void FileMd5DatabaseInit(const LogLevel& level = LogLevel::Info, const std::filesystem::path& file = {}, bool console = true);

void FileMd5DatabaseEnd();

void FileMd5DatabaseAdd(const std::string& deviceName, const std::filesystem::path& file, Database& fmd);

void FileMd5DatabaseBuilder(const std::string& deviceName, const std::filesystem::path& path, Database& fmd, std::vector<std::string> skips);

void FileMd5DatabaseQuery(const std::vector<Model>& fmdRaw,
	const MatchMethod& matchMethod,
	const Data& queryData,
	const Data& sortBy,
	const std::string& keyword,
	uint64_t limit,
	bool desc);

void Export(Database& fmd, const std::string& path, const ExportFormat& format);
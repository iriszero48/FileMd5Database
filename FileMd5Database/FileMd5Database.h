#pragma once

#include <filesystem>
#include <future>
#include <map>
#include <string>
#include <execution>
#include <regex>

#include "Convert.h"
#include "Thread.h"
#include "Arguments.h"

ArgumentOptionHpp(LogLevel, Kill, None, Error, Info, Debug)
ArgumentOptionHpp(MatchMethod, Contain, StartWith, EndWith, Regex, Eq, Gt, Lt)
ArgumentOptionHpp(Data, Path, Md5, Size, Time)
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
struct DataToMember<Data::Time, Model> { constexpr auto operator()(const Model& model) const { return model.Time; } };

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
constexpr auto ModelSortImpl(Fmd& fmd, const Cmp& cmp)
{
	std::sort(std::execution::par_unseq, fmd.begin(), fmd.end(), cmp);
}

template<typename Fmd>
constexpr void ModelSort(Fmd& fmd, const Data& sortBy)
{
	if      (sortBy == Data::Time) ModelSortImpl(fmd, ModelStringCmp<Data::Time>());
	else if (sortBy == Data::Md5 ) ModelSortImpl(fmd, ModelStringCmp<Data::Md5 >());
	else if (sortBy == Data::Path) ModelSortImpl(fmd, ModelStringCmp<Data::Path>());
	else if (sortBy == Data::Size) ModelSortImpl(fmd, ModelIntCmp   <Data::Size>());
}

template<typename Fmd>
void ModelReverse(Fmd& fmd)
{
	std::reverse(std::execution::par_unseq, fmd.begin(), fmd.end());
}

struct RegexMatch
{
	explicit RegexMatch(const std::string& keyword) : Keyword(keyword) { }

	template<typename V>
	bool operator()(const V& v) const
	{
		return std::regex_match(std::string(v), Keyword);
	}

	bool operator()(const std::uint64_t v) const
	{
		return operator()(Convert::ToString(v));
	}
	
	std::regex Keyword;
};


struct ContainMatch
{
	explicit ContainMatch(std::string keyword) : Keyword(std::move(keyword)) { }

	template<typename V>
	bool operator()(const V& v) const
	{
		return std::search(v.begin(), v.end(), std::boyer_moore_horspool_searcher(Keyword.begin(), Keyword.end())) != v.end();
	}

	bool operator()(const std::uint64_t v) const
	{
		return operator()(Convert::ToString(v));
	}

	std::string Keyword;
};

struct StartWithMatch
{
	explicit StartWithMatch(std::string keyword) : Keyword(std::move(keyword)) { }

	template<typename V>
	bool operator()(const V& v) const
	{
		if (v.length() < Keyword.length())
		{
			return false;
		}
		return std::equal(Keyword.begin(), Keyword.end(), v.begin());
	}

	bool operator()(const std::uint64_t v) const
	{
		return operator()(Convert::ToString(v));
	}

	std::string Keyword;
};

struct EndWithMatch
{
	explicit EndWithMatch(std::string keyword) : Keyword(std::move(keyword)) { }

	template<typename V>
	bool operator()(const V& v) const
	{
		if (v.length() < Keyword.length())
		{
			return false;
		}
		return std::equal(Keyword.rbegin(), Keyword.rend(), v.rbegin());
	}

	bool operator()(const std::uint64_t v) const
	{
		return operator()(Convert::ToString(v));
	}

	std::string Keyword;
};

struct BaseEq
{
	template<typename Lv, typename Rv>
	bool operator()(const Lv& lv, const Rv& rv) const
	{
		return lv == rv;
	}
};

struct BaseLt
{
	template<typename Lv, typename Rv>
	bool operator()(const Lv& lv, const Rv& rv) const
	{
		return lv < rv;
	}
};

struct BaseGt
{
	template<typename Lv, typename Rv>
	bool operator()(const Lv& lv, const Rv& rv) const
	{
		return lv > rv;
	}
};

template<typename TKeyword, typename SubMatch>
struct BaseMatchImpl
{
	explicit BaseMatchImpl(const std::string& keyword)
	{
		if constexpr (std::is_integral_v<TKeyword>)
		{
			Keyword = Convert::FromString<std::uint64_t>(keyword);
		}
		else
		{
			Keyword = keyword;
		}
	}

	template<typename V>
	bool operator()(const V& v) const
	{
		return SubMatch()(v, Keyword);
	}
	
	TKeyword Keyword;
};

template<typename SubMatch, Data DataValue> struct BaseMatch                       { using Type = BaseMatchImpl<std::string,   SubMatch>; };
template<typename SubMatch>                 struct BaseMatch<SubMatch, Data::Size> { using Type = BaseMatchImpl<std::uint64_t, SubMatch>; };

template<Data MatchData, bool Neg, typename TMatcher>
struct ModelMatcher
{
	explicit ModelMatcher(const std::string& keyword): Matcher(keyword) {}

	template<typename V>
	constexpr ModelRef operator()(const V& value) const
	{
		auto res = Matcher(DataToMember<MatchData, V>()(value));
		if constexpr (Neg) res = !res;
		return res ? value : ModelRef();
	}

	TMatcher Matcher;
};

inline bool IsNilModel(const ModelRef& model)
{
	return model.Path.length() != 0;
}

template<MatchMethod Method, Data MatchData, bool Neg, typename T>
void ModelMatchImplImplImpl(const T& data, std::vector<ModelRef>& result, const std::string& keyword)
{
	std::vector<ModelRef> tmp(data.size());
	     if constexpr (Method == MatchMethod::Contain  ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, ContainMatch                               >(keyword));
	else if constexpr (Method == MatchMethod::Regex    ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, RegexMatch                                 >(keyword));
	else if constexpr (Method == MatchMethod::StartWith) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, StartWithMatch                             >(keyword));
	else if constexpr (Method == MatchMethod::EndWith  ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, EndWithMatch                               >(keyword));
	else if constexpr (Method == MatchMethod::Eq       ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, typename BaseMatch<BaseEq, MatchData>::Type>(keyword));
	else if constexpr (Method == MatchMethod::Lt       ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, typename BaseMatch<BaseLt, MatchData>::Type>(keyword));
	else if constexpr (Method == MatchMethod::Gt       ) std::transform(std::execution::par_unseq, data.begin(), data.end(), tmp.begin(), ModelMatcher<MatchData, Neg, typename BaseMatch<BaseGt, MatchData>::Type>(keyword));
	else static_assert(true, "not impl");
	result.resize(std::count_if(std::execution::par_unseq, tmp.begin(), tmp.end(), IsNilModel));
	std::copy_if(std::execution::par_unseq, tmp.begin(), tmp.end(), result.begin(), IsNilModel);
}

template<MatchMethod Method, Data MatchData, typename T>
void ModelMatchImplImpl(const T& data, std::vector<ModelRef>& result, const bool neg, const std::string& keyword)
{
	if (neg) ModelMatchImplImplImpl<Method, MatchData, true >(data, result, keyword);
	else     ModelMatchImplImplImpl<Method, MatchData, false>(data, result, keyword);
}

template<MatchMethod Method, typename T>
void ModelMatchImpl(const T& data, std::vector<ModelRef>& result, const Data& matchData, const bool neg, const std::string& keyword)
{
	if      (matchData == Data::Time) ModelMatchImplImpl<Method,Data::Time>(data, result, neg, keyword);
	else if (matchData == Data::Md5 ) ModelMatchImplImpl<Method,Data::Md5 >(data, result, neg, keyword);
	else if (matchData == Data::Path) ModelMatchImplImpl<Method,Data::Path>(data, result, neg, keyword);
	else if (matchData == Data::Size) ModelMatchImplImpl<Method,Data::Size>(data, result, neg, keyword);
	else static_assert(true, "not impl");
}

template<typename T>
void ModelMatch(const T& data, std::vector<ModelRef>& result, const MatchMethod& matchMethod, const Data& matchData, const bool neg, const std::string& keyword)
{
	if		(matchMethod == MatchMethod::Contain  ) ModelMatchImpl<MatchMethod::Contain  >(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::Regex    ) ModelMatchImpl<MatchMethod::Regex    >(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::StartWith) ModelMatchImpl<MatchMethod::StartWith>(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::EndWith  ) ModelMatchImpl<MatchMethod::EndWith  >(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::Eq       ) ModelMatchImpl<MatchMethod::Eq       >(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::Lt       ) ModelMatchImpl<MatchMethod::Lt       >(data, result, matchData, neg, keyword);
	else if (matchMethod == MatchMethod::Gt       ) ModelMatchImpl<MatchMethod::Gt       >(data, result, matchData, neg, keyword);
	else static_assert(true, "not impl");
}

void ModelPrinter(const std::vector<ModelRef>& fmd);

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
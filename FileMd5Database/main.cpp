#include <filesystem>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <execution>
#include <random>


#include "Macro.h"
#include "Arguments.h"
#include "FileMd5Database.h"
#include "FileMd5DatabaseSerialization.h"
#include "Convert.h"
#include "String.h"

#ifndef MacroWindows

#include <malloc.h>

#endif

ArgumentOption(DbOperator, Build, Add, Query, Concat, Export, Alter)

static Database FileMd5Database{};

class StackTrace
{
public:
	struct Info
	{
		std::filesystem::path Path;
		uint64_t Line;
		std::string FunctionName;
		uint64_t FunctionAddress;
		uint64_t Count;
	};

	void Push(const Info& info)
	{
		stack.push_back(info);
	}

	void Pop()
	{
		stack.erase(stack.begin() + stack.size() - 1);
	}
	
	void PrintStackTrace(const uint64_t count)
	{
		stack.at(stack.size() - 1).Count = count;
		std::for_each(stack.rbegin(), stack.rend(), [&, d = stack.size()](const Info& info) mutable 
		{
			const auto& [p, l, fn, fa, c] = info;
			std::string record{};
			String::StringCombine(record, p.filename().string(), ":", Convert::ToString(l), ": ", fn, "[0x", Convert::ToString(fa, 16), "] --- stack(depth:", Convert::ToString(d--), ", size:", Convert::ToString(c), ") ---");
			puts(record.c_str());
		});
		std::string end{};
		String::StringCombine(end, "--- (depth ", Convert::ToString(stack.size()), ") ---");
		puts(end.c_str());
	};
private:
	std::vector<Info> stack{};
};

static StackTrace Stack{};

int main(int argc, char* argv[])
{
#define ArgumentsFunc(arg) [&](decltype(arg)::ConvertFuncParamType value) -> decltype(arg)::ConvertResult
#define ArgumentsValue(arg) args.Value<decltype(arg)::ValueType>(arg)

	ArgumentsParse::Arguments args{};

#pragma region Args
	ArgumentsParse::Argument<DbOperator, 1> dbOp
	{
		{},
		"database operator" + DbOperatorDesc(),
		ArgumentsFunc(dbOp)
		{
			return {ToDbOperator(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<std::filesystem::path> dbPathArg
	{
		"-p",
		"database file path"
	};
	ArgumentsParse::Argument deviceName
	{
		"--device",
		"device name"
	};
	ArgumentsParse::Argument<std::filesystem::path> rootPath
	{
		"--root",
		"root path"
	};
	ArgumentsParse::Argument<std::vector<std::string>> skip
	{
		"--skip",
		"skip file(file1;file2;...)",
		decltype(skip)::ValueType{},
		ArgumentsFunc(skip)
		{
			const auto vf = std::string(value) + ";";
			const std::regex re(R"([^;]+?;)");
			auto begin = std::sregex_iterator(vf.begin(), vf.end(), re);
			const auto end = std::sregex_iterator();
			std::vector<std::string> files{};
			for (auto i = begin; i != end; ++i)
			{
				const auto match = i->str();
				const auto f = match.substr(0, match.length() - 1);
				files.push_back(f);
			}
			return { files, {} };
		}
	};
	ArgumentsParse::Argument<std::filesystem::path> filePath
	{
		"--file",
		"file path"
	};
	ArgumentsParse::Argument<MatchMethod> matchMethod
	{
		"--method",
		"match method" + MatchMethodDesc(ToString(MatchMethod::Contain)),
		MatchMethod::Contain,
		ArgumentsFunc(matchMethod)
		{
			return {ToMatchMethod(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<Data> queryData
	{
		"--data",
		"query data" + DataDesc(ToString(Data::Path)),
		Data::Path,
		ArgumentsFunc(queryData)
		{
			return {ToData(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<Data> sortBy
	{
		"--sort",
		"sort by" + DataDesc(ToString(Data::Path)),
		Data::Path,
		ArgumentsFunc(sortBy)
		{
			return {ToData(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<uint64_t> limit
	{
		"--limit",
		"limit[500]",
		500,
		ArgumentsFunc(limit)
		{
			return {Convert::FromString<uint64_t>(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<bool, 0> desc
	{
		"--desc",
		"switch to desc sort",
		false,
		ArgumentsFunc(desc)
		{
			return {true, {}};
		}
	};
	ArgumentsParse::Argument keyword
	{
		"--keyword",
		"query keyword"
	};
	ArgumentsParse::Argument paths
	{
		"--paths",
		"file paths"
	};
	ArgumentsParse::Argument<ExportFormat> exportFormat
	{
		"--exoprtFormat",
		"exoprt format " + ExportFormatDesc(),
		ArgumentsFunc(exportFormat)
		{
			return {ToExportFormat(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument exportPath
	{
		"--exportPath",
		"export path"
	};
	ArgumentsParse::Argument<AlterType> alterType
	{
		"--alterType",
		"alter type " + AlterTypeDesc(),
		ArgumentsFunc(alterType)
		{
			return {ToAlterType(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument value
	{
		"--value",
		"alter value"
	};
	ArgumentsParse::Argument<std::filesystem::path> logPath
	{
		"--log",
		"log file",
		""
	};
	ArgumentsParse::Argument<LogLevel> logLevel
	{
		"--loglevel",
		"log level",
		LogLevel::Info,
		ArgumentsFunc(logLevel)
		{
			return {ToLogLevel(std::string(value)), {}};
		}
	};
	ArgumentsParse::Argument<bool, 0> consoleLog
	{
		"--disableconsolelog",
		"disable console log",
		true,
		ArgumentsFunc(consoleLog)
		{
			return {false, {}};
		}
	};
	ArgumentsParse::Argument<bool, 0> interactive
	{
		"--interactive",
		"interactive mode",
		false,
		ArgumentsFunc(interactive)
		{
			return {true, {}};
		}
	};
	args.Add(dbOp);
	args.Add(dbPathArg);
	args.Add(deviceName);
	args.Add(rootPath);
	args.Add(skip);
	args.Add(filePath);
	args.Add(matchMethod);
	args.Add(queryData);
	args.Add(sortBy);
	args.Add(limit);
	args.Add(desc);
	args.Add(keyword);
	args.Add(paths);
	args.Add(exportFormat);
	args.Add(exportPath);
	args.Add(alterType);
	args.Add(value);
	args.Add(logPath);
	args.Add(logLevel);
	args.Add(consoleLog);
	args.Add(interactive);
#pragma endregion Args

	auto logStarted = false;
	
#define Ex
	
#ifdef Ex
	try
#endif
	{
		args.Parse(argc, argv);

		if (ArgumentsValue(interactive))
		{
			const auto ll = ArgumentsValue(logLevel);
			const std::function<bool(std::vector<ModelRef>&)> Interactive = [&](std::vector<ModelRef>& fmd) -> bool
			{
				Stack.Push({ __FILE__, __LINE__ - 2, "Interactive", reinterpret_cast<uint64_t>(std::addressof(Interactive)), fmd.size() });
				while (true)
				{
					Stack.PrintStackTrace(fmd.size());
#ifndef MacroWindows
					if (ll >= LogLevel::Debug)
					{
						malloc_stats();
					}
#endif
					std::cout << "->";
					std::string line;
					std::getline(std::cin, line);
					static const auto matchFunc = [Interactive](std::vector<ModelRef>& fmd, const MatchMethod& method, const bool neg, const std::string& args, const bool help)
					{
						if (help)
						{
							std::cout << DataDesc() << " keyword";
							return false;
						}
						const auto sp = args.find(' ');
						const auto by = *ToData(args.substr(0, sp));
						const auto kw = args.substr(sp + 1);
						std::vector<ModelRef> res{};
						ModelMatch(fmd, res, method, by, neg, kw);
						Interactive(res);
						return false;
					};
					std::unordered_map < std::string, std::function<bool(const std::string&, bool) >> ops
					{
						{"help",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							std::vector<std::string> out{};
							for (const auto& [k, _] : ops)
							{
								out.push_back(k);
							}
							std::sort(out.begin(), out.end());
							for (const auto& k : out)
							{
								std::cout << k << " ";
								ops.at(k)({}, true);
								std::cout << "\n";
							}
							return false;
						} },
						{ "load",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << "DatabasePath";
								return false;
							}
							std::vector<Model> data{};
							DeserializationAsModel(data, args);
							puts(Convert::ToString(data.size()).c_str());
							std::vector<ModelRef> fmd(data.size());
							std::transform(std::execution::par_unseq, data.begin(), data.end(), fmd.begin(), [](const Model& model) { return ModelRef(std::string_view(model.Path.str, model.Path.size), std::string_view(model.Md5.str, model.Md5.size), model.Size, std::string_view(model.Time.str, model.Time.size)); });
							std::vector<char*> address(data.size());
							std::transform(std::execution::par_unseq, data.begin(), data.end(), address.begin(), [](const Model& model) { return model.Path.str; });
							data.clear();
							data.shrink_to_fit();
							Interactive(fmd);
							std::for_each(std::execution::par_unseq, address.begin(), address.end(), [](const char* addr) { delete[] addr; });
#ifndef MacroWindows
							malloc_trim(0);
#endif
							return false;
						} },
						{ "sort",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << DataDesc();
								return false;
							}
							ModelSort(fmd, *ToData(args));
							return false;
						} },
						{ "!sort",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << DataDesc();
								return false;
							}
							ModelSort(fmd, *ToData(args));
							ModelReverse(fmd);
							return false;
						} },
						{ "rev",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							ModelReverse(fmd);
							return false;
						} },
						{ "shuffle",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							std::random_device rd;
							std::mt19937 g(rd());
							std::shuffle(fmd.begin(), fmd.end(), g);
							return false;
						} },
						{  "regex"    , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Regex,     false, args, help); } },
						{ "!regex"    , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Regex,     true , args, help); } },
						{  "startwith", [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::StartWith, false, args, help); } },
						{ "!startwith", [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::StartWith, true , args, help); } },
						{  "endwith"  , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::EndWith,   false, args, help); } },
						{ "!endwith"  , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::EndWith,   true , args, help); } },
						{  "contain"  , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Contain,   false, args, help); } },
						{ "!contain"  , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Contain,   true , args, help); } },
						{  "eq"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Eq,        false, args, help); } },
						{ "!eq"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Eq,        true , args, help); } },
						{  "lt"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Lt,        false, args, help); } },
						{ "!lt"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Lt,        true , args, help); } },
						{  "gt"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Gt,        false, args, help); } },
						{ "!gt"       , [&](const std::string& args = {}, const bool help = false){ return matchFunc(fmd, MatchMethod::Gt,        true , args, help); } },
						{ "maxlength",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							puts(Convert::ToString(std::max_element(std::execution::par_unseq, fmd.begin(), fmd.end(), [](const ModelRef& a, const ModelRef& b) { return std::less<>()(a.Path.length(), b.Path.length()); })->Path.length()).c_str());
							return false;
						} },
						{ "sum",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							std::vector<uint64_t> tmp(fmd.size());
							std::transform(std::execution::par_unseq, fmd.begin(), fmd.end(), tmp.begin(), [](const ModelRef& model) { return model.Size; });
							puts(Convert::ToString(std::reduce(std::execution::par_unseq, tmp.begin(), tmp.end())).c_str());
							return false;
						} },
						{ "skip",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << "int";
								return false;
							}
							const auto n = Convert::FromString<uint64_t>(args);
							if (fmd.size() > n)
							{
								const auto newN = fmd.size() - n;
								std::vector<ModelRef> res(newN);
								std::copy_n(std::execution::par_unseq, fmd.begin() + n, newN, res.begin());
								Interactive(res);
							}
							return false;
						} },
						{ "take",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << "int";
								return false;
							}
							const auto n = Convert::FromString<uint64_t>(args);
							const auto newN = std::min(n, static_cast<uint64_t>(fmd.size()));
							std::vector<ModelRef> res(newN);
							std::copy_n(std::execution::par_unseq, fmd.begin(), newN, res.begin());
							Interactive(res);
							return false;
						} },
						{ "count",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							puts(Convert::ToString(fmd.size()).c_str());
							return false;
						} },
						{ "back",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							return true;
						} },
						{ "exit",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							exit(EXIT_SUCCESS);
						} },
						{ "show",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								std::cout << "int";
								return false;
							}
							const auto limit = Convert::FromString<uint64_t>(args);
							if (fmd.empty()) return false;
							std::vector<ModelStr> res{};
							std::for_each(fmd.begin(), fmd.end(),[&, i = static_cast<uint64_t>(0)](const ModelRef& model) mutable
							{
								if (i < limit)
								{
									++i;
									ModelStr mod;
									try
									{
										mod.Path = std::filesystem::u8path(model.Path).string();
									}
									catch (...)
									{
										mod.Path = model.Path;
									}
									mod.Md5 = model.Md5;
									mod.Size = Convert::ToString(model.Size);
									mod.Time = model.Time;
									res.emplace_back(mod);
								}
							});
							const auto maxPathLen = std::max_element(res.begin(), res.end(), [](const ModelStr& a, const ModelStr& b) { return std::less<>()(a.Path.length(), b.Path.length()); })->Path.length();
							const auto maxSizeLen = std::max_element(res.begin(), res.end(), [](const ModelStr& a, const ModelStr& b) { return std::less<>()(a.Size.length(), b.Size.length()); })->Size.length();
							for (const auto& [p, m, s, t] : res)
							{
								std::cout << p << std::string(maxPathLen - p.length(), ' ') << " | " << (m.empty() ? std::string(32, ' ') : m) << " | " << std::string(maxSizeLen - s.length(), ' ') << s << " | " << (t.empty() ? std::string(19, ' ') : t) << "\n";
							}
							return false;
						} },
						{ "device",[&](const std::string& args = {}, const bool help = false)
						{
							if (help)
							{
								return false;
							}
							std::vector<std::string_view> devices(fmd.size());
							std::transform(std::execution::par_unseq, fmd.begin(), fmd.end(), devices.begin(), [](const ModelRef& model) { return model.Path.substr(0, model.Path.find(':')); });
							const auto last = std::unique(std::execution::par_unseq, devices.begin(), devices.end());
							devices.erase(last, devices.end());
							devices.shrink_to_fit();
							std::sort(devices.begin(), devices.end());
							std::copy(devices.begin(), devices.end(), std::ostream_iterator<std::string_view>(std::cout, "\n"));
							return false;
						} },
					};
					const auto sp = line.find(' ');
#ifdef Ex
					try
#endif
					{
						if (ops.at(line.substr(0, sp))(std::filesystem::path(line.substr(sp + 1)).u8string(), false))
						{
							Stack.Pop();
							return true;
						}
					}
#ifdef Ex
					catch (const std::exception& ex)
					{
						puts(ex.what());
					}
#endif
				}
			};
			std::vector<ModelRef> empty{};
			Interactive(empty);
			return EXIT_SUCCESS;
		}
		
		const auto databaseFilePath = ArgumentsValue(dbPathArg);

		FileMd5DatabaseInit(ArgumentsValue(logLevel), ArgumentsValue(logPath), ArgumentsValue(consoleLog));
		logStarted = true;
		
		std::unordered_map<DbOperator, std::function<void()>>
		{
			{ DbOperator::Build, [databaseFilePath, args, deviceName, rootPath, skip]()
			{
				if (exists(databaseFilePath)) Deserialization(FileMd5Database, databaseFilePath);
				FileMd5DatabaseBuilder(ArgumentsValue(deviceName), ArgumentsValue(rootPath), FileMd5Database, ArgumentsValue(skip));
				Serialization(FileMd5Database, databaseFilePath);
			} },
			{ DbOperator::Add, [databaseFilePath, args, deviceName, filePath]()
			{
				if (exists(databaseFilePath)) Deserialization(FileMd5Database, databaseFilePath);
				FileMd5DatabaseAdd(deviceName, ArgumentsValue(filePath), FileMd5Database);
				Serialization(FileMd5Database, databaseFilePath);
			} },
			{ DbOperator::Query, [databaseFilePath, args, matchMethod, queryData, sortBy, keyword, limit, desc]()
			{
				std::vector<Model> fmd{};
				DeserializationAsModel(fmd, databaseFilePath);
				FileMd5DatabaseQuery(fmd,
					ArgumentsValue(matchMethod),
					ArgumentsValue(queryData),
					ArgumentsValue(sortBy),
					std::filesystem::path(ArgumentsValue(keyword)).u8string(),
					ArgumentsValue(limit),
					ArgumentsValue(desc));
			} },
			{ DbOperator::Concat, [databaseFilePath, args, paths]()
			{
				const auto p = ArgumentsValue(paths) + ";";
				const std::regex re(R"([^;]+?;)");
				auto begin = std::sregex_iterator(p.begin(), p.end(), re);
				const auto end = std::sregex_iterator();
				for (auto i = begin; i != end; ++i)
				{
					const auto match = i->str();
					const auto dbp = match.substr(0, match.length() - 1);
					std::cout << "Deserialization " << dbp << " ... ";
					Deserialization(FileMd5Database, dbp);
					std::cout << "[done]\n";
				}
				std::cout << "Serialization " << databaseFilePath << " ... ";
				Serialization(FileMd5Database, databaseFilePath);
				std::cout << "[done]\n";
			} },
			{ DbOperator::Export, [databaseFilePath, args, exportFormat, exportPath]()
			{
				Deserialization(FileMd5Database, databaseFilePath);
				Export(FileMd5Database, ArgumentsValue(exportPath), ArgumentsValue(exportFormat));
			} },
			{ DbOperator::Alter, [databaseFilePath, args, alterType, value]()
			{
				Database fmd;
				Deserialization(fmd, databaseFilePath);
				const auto newValue = ArgumentsValue(value);
				std::unordered_map<AlterType, std::function<void()>>
				{
					{ AlterType::DeviceName, [&]()
					{
						for (const auto& [k, v] : fmd)
						{
							auto nk = newValue;
							String::StringCombine(nk, ":", k.substr(k.find(':') + 1));
							FileMd5Database[nk] = v;
						}
					} },
					{ AlterType::DriveLetter, [&]()
					{
						for (const auto& [k, v] : fmd)
						{
							auto newPath = k;
							newPath[k.find(':') + 1] = newValue[0];
							FileMd5Database[newPath] = v;
						}
					} }
				}.at(ArgumentsValue(alterType))();
				Serialization(FileMd5Database, databaseFilePath);
			} },
		}.at(ArgumentsValue(dbOp))();
	}
#ifdef Ex
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << "\n" << args.GetDesc() << R"(
Build:
    --device --root -p [--skip]
Add:
    --device --file -p
Query:
    --keyword -p [--data] [--desc] [--limit] [--method] [--sort]
Concat:
    --paths -p
Export:
    --exoprtFormat --exportPath -p
Alter:
    --alterType --value -p

Interactive:
    --interactive

Log:
    [--disableconsolelog] [--log] [--loglevel]
)";
	}
#endif
	if (logStarted)
	{
		FileMd5DatabaseEnd();
	}
}

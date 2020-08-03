#include "Unified.h"
#include "FileMd5Database.h"
#include "FileMd5DatabaseSerialization.h"

static Database FileMd5Database{};

class ParameterError final : public std::exception
{
public:
	[[nodiscard]] char const* what() const noexcept override
	{
		return " " __FILE__ ":" Line " " __DATE__ " " __TIME__
			R"(
Parameter Error
Usage: databaseFilePath command [options]
	build deviceName rootPath [format[binary|text]] [logOutput[default:true|false]]
	add deviceName filePath [format[binary|text]] [logOutput[default:true|false]]
	query queryMethod[contain|startWith|regex] queryData[path|md5|size|fileModificationTime] keyword [format[binary|text]] [limit(default:100))]
	concat format[binary|text] sourceDatabaseFile1 sourceDatabaseFile2 [sourceDatabaseFile...]
	convert destFormat[text|binary] sourceFormat[text|binary] sourceDatabaseFile
	export exportFormat[csv|json] exportPath [databaseFormat[binary|text]]
	alter [DeviceName|DriveLetter] newValue [databaseFormat[binary|text]]

default format: binary
)";
	}
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc >= 5)
		{
			const std::string databaseFilePath = argv[1];
			const std::string command = argv[2];
			if (command == "build" && argc <= 6)
			{
				const std::string deviceName = argv[3];
				const std::string rootPath = argv[4];
				const std::string format = argc == 6 ? argv[5] : "";
				const std::string logOutput = argc == 7 ? argv[6] : "true";
				if (std::filesystem::exists(databaseFilePath)) Deserialization(FileMd5Database, databaseFilePath, format);

#if (defined _WIN32 || defined _WIN64)
				setlocale(LC_ALL, "");
#endif
				
				FileMd5DatabaseBuilder(deviceName, rootPath, FileMd5Database, logOutput == "true");
				Serialization(FileMd5Database, databaseFilePath, format);
			}
			else if (command == "add" && argc <= 6)
			{
				const std::string deviceName = argv[3];
				const std::string filePath = argv[4];
				const std::string format = argc == 6 ? argv[5] : "";
				const std::string logOutput = argc == 7 ? argv[6] : "true";
				if (std::filesystem::exists(databaseFilePath)) Deserialization(FileMd5Database, databaseFilePath, format);

#if (defined _WIN32 || defined _WIN64)
				setlocale(LC_ALL, "");
#endif

				FileMd5DatabaseAdd(deviceName, filePath, FileMd5Database, logOutput == "true");
				Serialization(FileMd5Database, databaseFilePath, format);
			}
			else if (command == "query" && argc >= 6 && argc <= 7)
			{
				const std::string queryMethod = argv[3];
				const std::string queryData = argv[4];
				const std::string keyword = argv[5];
				const std::string format = argc == 7 ? argv[6] : "";
				const auto limit = argc == 8 ? std::strtoull(argv[7], &argv[7], 10) : 100;
				Deserialization(FileMd5Database, databaseFilePath, format);
				printf("%" PRIu64 "\n", static_cast<uint64_t>(FileMd5Database.size()));

#if (defined _WIN32 || defined _WIN64)
				system("chcp 65001");
#endif

				FileMd5DatabaseQuery(FileMd5Database, queryMethod, queryData, keyword);
			}
			else if (command == "concat")
			{
				const std::string format = argv[3];
				for (auto i = 4; i < argc; ++i)
				{
					Database temp{};
					Deserialization(temp, argv[i], format);
					for (const auto& value : temp) FileMd5Database.insert(value);
				}
				Serialization(FileMd5Database, databaseFilePath, format);
			}
			else if (command == "convert" && argc == 6)
			{
				const std::string destFormat = argv[3];
				const std::string sourceFormat = argv[4];
				const std::string sourceDatabaseFile = argv[5];
				FormatConvert(FileMd5Database, destFormat, databaseFilePath, sourceFormat, sourceDatabaseFile);
			}
			else if(command == "export" && argc <= 6)
			{
				const std::string exportFormat = argv[3];
				const std::string exportPath = argv[4];
				const std::string format = argc == 6 ? argv[5] : "";
				Deserialization(FileMd5Database, databaseFilePath, format);
				Export(FileMd5Database, exportPath, exportFormat);
			}
			else if (command == "alter" && argc <= 6)
			{
				const std::string op = argv[3];
				const std::string newValue = argv[4];
				const std::string format = argc == 6 ? argv[5] : "";
				Database temp{};
				Deserialization(temp, databaseFilePath, format);
				if (op == "DeviceName")
				{
					for (const auto& value : temp)
					{
						FileMd5Database[newValue + ":" + value.first.substr(value.first.find(":") + 1)] = value.second;
					}
				}
				else if (op == "DriveLetter")
				{
					for (const auto& value : temp)
					{
						auto newPath = value.first;
						newPath[value.first.find(":") + 1] = newValue[0];
						FileMd5Database[newPath] = value.second;
					}
				}

				Serialization(FileMd5Database, databaseFilePath, format);
			}
			else
			{
				throw ParameterError();
			}
		}
		else
		{
			throw ParameterError();
		}
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, Line":%s\n", e.what());
		exit(EXIT_FAILURE);
	}
}
// g++ FileMd5Database.cpp -o FileMd5Database.out -std=c++17 -lboost_serialization -lboost_locale -lboost_filesystem -fopenmp -lboost_system -O2
// g++ FileMd5Database.cpp -o FileMd5Database.out -std=c++17 -l:libboost_serialization.a -l:libboost_locale.a -l:libboost_filesystem.a -fopenmp -l:libboost_system.a -O2
// g++ 8 needs -lstdc++fs
#pragma once

#include <fstream>
#include <memory>

class CsvFile
{
	std::ofstream fs;
	const std::uint64_t bufferSize = 1024 * 1024;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(bufferSize);
	bool isFirst;
	const std::string separator;
	const std::string escapeSeq;
	const std::string specialChars;
public:
	explicit CsvFile(const std::string& filename, std::string separator = ";");

	CsvFile() = delete;

	~CsvFile();

	void Flush();

	void EndRow();

	CsvFile& operator << (CsvFile& (*val)(CsvFile&));

	CsvFile& operator << (const char* val);

	CsvFile& operator << (const std::string& val);

	template<typename T>
	CsvFile& operator << (const T& val)
	{
		return Write(val);
	}

	static CsvFile& EndRow(CsvFile& file);

	static CsvFile& Flush(CsvFile& file);

private:
	template<typename T>
	CsvFile& Write(const T& val)
	{
		if (!isFirst)
		{
			fs << separator;
		}
		else
		{
			isFirst = false;
		}
		fs << val;
		return *this;
	}

	[[nodiscard]] std::string Escape(const std::string& val) const;
};

#pragma once

#include <fstream>

class CsvFile
{
	std::ofstream fs_;
	bool is_first_;
	const std::string separator_;
	const std::string escape_seq_;
	const std::string special_chars_;
public:
	explicit CsvFile(const std::string filename, const std::string separator = ";");

	CsvFile() = delete;

	~CsvFile();

	void Flush();

	void Endrow();

	CsvFile& operator << (CsvFile& (*val)(CsvFile&));

	CsvFile& operator << (const char* val);

	CsvFile& operator << (const std::string& val);

	template<typename T>
	CsvFile& operator << (const T& val)
	{
		return Write(val);
	}

	static CsvFile& Endrow(CsvFile& file);

	static CsvFile& Flush(CsvFile& file);

private:
	template<typename T>
	CsvFile& Write(const T& val)
	{
		if (!is_first_)
		{
			fs_ << separator_;
		}
		else
		{
			is_first_ = false;
		}
		fs_ << val;
		return *this;
	}

	std::string Escape(const std::string& val);
};

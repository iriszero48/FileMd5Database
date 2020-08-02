#include "CSV.h"

#include <sstream>

CsvFile::CsvFile(const std::string filename, const std::string separator)
	: is_first_(true),
	separator_(separator),
	escape_seq_("\""),
	special_chars_("\"")
{
	fs_.exceptions(std::ios::failbit | std::ios::badbit);
	fs_.open(filename);
}

CsvFile::~CsvFile()
{
	Flush();
	fs_.close();
}

void CsvFile::Flush()
{
	fs_.flush();
}

void CsvFile::Endrow()
{
	fs_ << "\n";
	is_first_ = true;
}

CsvFile& CsvFile::operator<<(CsvFile&(* val)(CsvFile&))
{
	return val(*this);
}

CsvFile& CsvFile::operator<<(const char* val)
{
	return Write(Escape(val));
}

CsvFile& CsvFile::operator<<(const std::string& val)
{
	return Write(Escape(val));
}

CsvFile& CsvFile::Endrow(CsvFile& file)
{
	file.Endrow();
	return file;
}

CsvFile& CsvFile::Flush(CsvFile& file)
{
	file.Flush();
	return file;
}

std::string CsvFile::Escape(const std::string& val)
{
	std::ostringstream result;
	result << '"';
	std::string::size_type to, from = 0u, len = val.length();
	while (from < len &&
		std::string::npos != (to = val.find_first_of(special_chars_, from)))
	{
		result << val.substr(from, to - from) << escape_seq_ << val[to];
		from = to + 1;
	}
	result << val.substr(from) << '"';
	return result.str();
}






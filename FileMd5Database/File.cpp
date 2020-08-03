#include "File.h"

#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

#include <iomanip>

std::string FileLastModified(const NativeStringType& path)
{
	try
	{
		std::ostringstream oss{};
		const auto lwt = last_write_time(boost::filesystem::path(path));
		if (lwt < 0)
		{
			LogErr(path.c_str(), "get last_write_time() failed.");
			return "";
		}
		oss << std::put_time(static_cast<const tm*>(std::localtime(static_cast<time_t const*>(&lwt))), "%F %T");
		return oss.str();
	}
	catch (const std::exception& e)
	{
		LogErr(path.c_str(), e.what());
		return "";
	}
}

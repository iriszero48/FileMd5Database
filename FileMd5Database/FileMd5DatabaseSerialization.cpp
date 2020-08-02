#include "FileMd5DatabaseSerialization.h"

#include <fstream>

#define SerializationText Serialization<boost::archive::text_oarchive>
#define DeserializationText Deserialization<boost::archive::text_iarchive>

#define SerializationBinary Serialization<boost::archive::binary_oarchive>
#define DeserializationBinary Deserialization<boost::archive::binary_iarchive>

void Serialization(Database& fmd, const std::string& databasePath, const std::string& format)
{
	if (format == "text")
	{
		SerializationText(fmd, databasePath);
	}
	else
	{
		Serialization(fmd, databasePath);
	}
}

void Deserialization(Database& fmd, const std::string& databasePath, const std::string& format)
{
	if (format == "text")
	{
		DeserializationText(fmd, databasePath);
	}
	else
	{
		Deserialization(fmd, databasePath);
	}
}

void FormatConvert(Database& fmd, const std::string& destFormat, const std::string& databaseFilePath, const std::string& sourceFormat, const std::string& sourceDatabaseFile)
{
	Deserialization(fmd, sourceDatabaseFile, sourceFormat);
	Serialization(fmd, databaseFilePath, destFormat);
}
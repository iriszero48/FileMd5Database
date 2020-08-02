#pragma once

#include "FileMd5Database.h"

#include <fstream>

#include <boost/serialization/map.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Log.h"
#include "TupleSerialization.h"

using oDefaultFormat = boost::archive::binary_oarchive;
using iDefaultFormat = boost::archive::binary_iarchive;

template<typename Format = oDefaultFormat>
void Serialization(Database& fmd, const std::string& databasePath)
{
	try
	{
		std::ofstream dbFile(databasePath, std::ios::binary);
		Format db(dbFile);
		db << fmd;
		dbFile.close();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, Line":%s:%s\n", databasePath.c_str(), e.what());
		exit(EXIT_FAILURE);
	}
}

template<typename Format = iDefaultFormat>
void Deserialization(Database& fmd, const std::string& databasePath)
{
	try
	{
		std::ifstream dbFile(databasePath, std::ios::binary);
		Format db(dbFile);
		db >> fmd;
		dbFile.close();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, Line":%s:%s\n", databasePath.c_str(), e.what());
		exit(EXIT_FAILURE);
	}
}

#define SerializationText Serialization<boost::archive::text_oarchive>
#define DeserializationText Deserialization<boost::archive::text_iarchive>

#define SerializationBinary Serialization<boost::archive::binary_oarchive>
#define DeserializationBinary Deserialization<boost::archive::binary_iarchive>

void Serialization(Database& fmd, const std::string& databasePath, const std::string& format);

void Deserialization(Database& fmd, const std::string& databasePath, const std::string& format);

void FormatConvert(Database& fmd, const std::string& destFormat, const std::string& databaseFilePath, const std::string& sourceFormat, const std::string& sourceDatabaseFile);

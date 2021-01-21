#pragma once

#include <fstream>

#include "FileMd5Database.h"

void Serialization(const Database& fmd, const std::filesystem::path& databasePath);

void Deserialization(Database& fmd, const std::filesystem::path& databasePath);

void DeserializationAsModel(std::vector<Model>& fmd, const std::filesystem::path& databasePath);

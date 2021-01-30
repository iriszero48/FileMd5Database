#include "FileMd5DatabaseSerialization.h"

#include <cstring>
#include <fstream>

#include "Bit.h"

template<typename T>
union IntBytes
{
	T data;
	char bytes[sizeof(T)];
};

using Uint64Bytes = IntBytes<uint64_t>;

static const auto NilString = "";

void Serialization(const Database& fmd, const std::filesystem::path& databasePath)
{
	remove(databasePath);
	std::ofstream fs(databasePath, std::ios::binary | std::ios::out);
	const auto fsBufSize = 1024 * 1024;
	const auto fsBuf = std::make_unique<char[]>(fsBufSize);
	fs.rdbuf()->pubsetbuf(fsBuf.get(), fsBufSize);
	char nil[32]{ 0 };
	for (const auto& [path,v] : fmd)
	{
		const auto& [md5, size, date] = v;
		Uint64Bytes pathLen{ path.length() };
		Uint64Bytes sizeStd{ size };
		if constexpr (Bit::Endian::Native != Bit::Endian::Little)
		{
			pathLen.data = Bit::EndianSwap(pathLen.data);
			sizeStd.data = Bit::EndianSwap(sizeStd.data);
		}
		fs.write(pathLen.bytes, sizeof(Uint64Bytes));
		fs << path;
		fs.write(md5.empty() ? nil : md5.c_str(), 32);
		fs.write(sizeStd.bytes, sizeof(Uint64Bytes));
		fs.write(date.empty() ? nil : date.c_str(), 19);
	}
	fs.close();
}

static char NilStrData[] = "";
static const StrPtr NilStr{ NilStrData, 0 };

#define NullOr(q,o,v) ((q) ? (o) : (v))

template<typename T = Database>
void DeserializationImpl(T& fmd, const std::filesystem::path& databasePath)
{
	std::ifstream fs(databasePath, std::ios::binary | std::ios::in);
	const auto fsBufSize = 1024 * 1024;
	const auto fsBuf = std::make_unique<char[]>(fsBufSize);
	fs.rdbuf()->pubsetbuf(fsBuf.get(), fsBufSize);
	Uint64Bytes uint64Buf{ 0 };
	constexpr auto md5Len = 32;
	constexpr auto sizeLen = 8;
	constexpr auto timeLen = 19;
	constexpr auto vLen = md5Len + sizeLen + timeLen;

	char* buf = nullptr;
	
	if constexpr (std::is_same<T, Database>::value)
	{
		buf = new char[65536];
		memset(buf, 0, 65536);
	}
	while (true)
	{
		fs.read(uint64Buf.bytes, sizeof(Uint64Bytes));
		if (fs.eof()) { break; }
		if constexpr (Bit::Endian::Native != Bit::Endian::Little)
		{
			uint64Buf.data = Bit::EndianSwap(uint64Buf.data);
		}
		const auto pathLen = uint64Buf.data;
		const auto totalLen = pathLen + vLen;
		if constexpr (std::is_same<T, std::vector<Model>>::value)
		{
			buf = new char[totalLen];
		}
		fs.read(buf, totalLen);
		memcpy(uint64Buf.bytes, buf + pathLen + md5Len, sizeof(Uint64Bytes));
		if constexpr (Bit::Endian::Native != Bit::Endian::Little)
		{
			uint64Buf.data = Bit::EndianSwap(uint64Buf.data);
		}
		const auto size = uint64Buf.data;
		
		const auto md5Offset = pathLen;
		const auto md5Begin = buf + md5Offset;
		const auto md5NullQ = buf[md5Offset] == 0;
		
		const auto timeOffset = pathLen + md5Len + sizeLen;
		const auto timeBegin = buf + timeOffset;
		const auto timeNullQ = buf[timeOffset] == 0;
		
		if constexpr (std::is_same<T, Database>::value)
		{
			const auto md5 = NullOr(md5NullQ, std::string(), std::string(md5Begin, md5Len));
			const auto time = NullOr(timeNullQ, std::string(), std::string(timeBegin, timeLen));
			fmd.emplace(std::string(buf, pathLen), std::make_tuple(md5, size, time));
		}
		else if constexpr (std::is_same<T, std::vector<Model>>::value)
		{
			const auto path = StrPtr{ buf, pathLen };
			const auto md5 = NullOr(md5NullQ, NilStr, (StrPtr{ md5Begin, md5Len }));
			const auto time = NullOr(timeNullQ, NilStr, (StrPtr{ timeBegin, timeLen }));
			fmd.push_back(Model{ path, md5, size, time });
		}
	}
	if constexpr (std::is_same<T, Database>::value)
	{
		delete[] buf;
	}
}

void Deserialization(Database& fmd, const std::filesystem::path& databasePath)
{
	DeserializationImpl<Database>(fmd, databasePath);
}

void DeserializationAsModel(std::vector<Model>& fmd, const std::filesystem::path& databasePath)
{
	DeserializationImpl<std::vector<Model>>(fmd, databasePath);
}

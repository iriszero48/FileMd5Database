#pragma once

#include <string>
#include <filesystem>

namespace Cryptography
{
	class Md5
	{
	public:
		union DigestData
		{
			struct Integer { std::uint32_t A, B, C, D; } DWord;
			std::uint8_t Word[16];
		};

		Md5();
		void Append(std::uint8_t* buf, std::uint64_t len);
		void Append(std::istream& stream);
		[[maybe_unused]] DigestData Digest();
		std::string HexDigest();

	private:
		DigestData data;
		std::uint8_t buffer[64]{ 0 };
		std::uint8_t bufferLen = 0;
		std::uint64_t length = 0;

		bool finished = false;
		std::string hexDigest{};

		void Append64(std::uint8_t* buf, std::uint64_t n);
	};
}

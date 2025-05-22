#pragma once
#include <string>
#include <filesystem>
#include <sstream>

namespace vkr
{
	std::wstring UTF8ToUTF16(const std::string& utf8Str);
	std::string ReadFileToString(const std::filesystem::path& filename);

	bool IsInt(const std::string& str);
	int32_t ToInt(const std::string& str);
	bool IsFloat(const std::string& str);
	float ToFloat(const std::string& str);
}
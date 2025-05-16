#pragma once
#include <string>
#include <filesystem>

namespace vkr
{
	std::wstring UTF8ToUTF16(const std::string& utf8Str);
	std::string ReadFileToString(const std::filesystem::path& filename);
}
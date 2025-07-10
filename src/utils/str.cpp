#include "str.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdexcept>

#include <fstream>
#include <sstream>

namespace vkr
{
	std::wstring UTF8ToUTF16(const std::string& str)
	{
		if (str.empty())
			return {};

		int32_t wideLen = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), nullptr, 0);
		if (wideLen == 0) 
		{
			throw std::runtime_error("MultiByteToWideChar failed to calculate length.");
		}

		std::wstring wstr(wideLen, 0);
		int32_t result = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), &wstr[0], wideLen );
		if (result == 0) 
		{
			throw std::runtime_error("MultiByteToWideChar failed during conversion.");
		}

		return wstr;
	}

	std::string UTF16ToUTF8(const std::wstring& utf16Str)
	{
		if (utf16Str.empty())
			return {};

		int32_t utf8Len = WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), static_cast<int32_t>(utf16Str.size()), nullptr, 0, nullptr, nullptr);
		if (utf8Len == 0)
		{
			throw std::runtime_error("WideCharToMultiByte failed to calculate length.");
		}

		std::string str(utf8Len, 0);
		int32_t result = WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), static_cast<int32_t>(utf16Str.size()), &str[0], utf8Len, nullptr, nullptr);
		if (result == 0)
		{
			throw std::runtime_error("WideCharToMultiByte failed during conversion.");
		}

		return str;
	}

	std::string ReadFileToString(const std::filesystem::path& filename)
	{
		std::ifstream file(filename, std::ios::in | std::ios::binary);
		if (!file) 
		{
			throw std::runtime_error("Failed to open file");
		}

		std::ostringstream contents;
		contents << file.rdbuf();
		return contents.str();
	}

	bool IsInt(const std::string& str)
	{
		return !str.empty() && std::find_if(str.begin(),str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
	}

	int32_t ToInt(const std::string& str)
	{
		return std::stoi(str);
	}

	bool IsFloat(const std::string& str)
	{
		std::istringstream iss(str);
		float f;
		iss >> std::noskipws >> f;
		return iss.eof() && !iss.fail();
	}

	float ToFloat(const std::string& str)
	{
		return std::stof(str);
	}

}
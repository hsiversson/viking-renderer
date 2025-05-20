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

		int wideLen = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
		if (wideLen == 0) 
		{
			throw std::runtime_error("MultiByteToWideChar failed to calculate length.");
		}

		std::wstring wstr(wideLen, 0);
		int result = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstr[0], wideLen );
		if (result == 0) 
		{
			throw std::runtime_error("MultiByteToWideChar failed during conversion.");
		}

		return wstr;
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
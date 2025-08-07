#pragma once

namespace vkr::SystemPaths
{
	bool Init();

	const std::filesystem::path& GetContentDirectory();
	std::filesystem::path GetInContentDirectory(const std::filesystem::path& pathToAppend);
	const std::filesystem::path& GetUserDirectory();

	const std::filesystem::path& GetExeDirectory();
	const std::filesystem::path& GetExePath();
}